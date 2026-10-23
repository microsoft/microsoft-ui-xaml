# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.
param(
    [ValidateSet('amd64fre', 'amd64chk', 'arm64fre', 'arm64chk')]
    [string]$Flavor = 'amd64fre',
    [switch]$Run
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..\..\..')).Path
$properties = @{}
Get-Content "$root\BuildOutput\ProfilerHost.$Flavor.properties" | ForEach-Object {
    $key, $value = $_ -split '=', 2
    $properties[$key] = $value
}
if ($properties.SelfContained -ne 'true' -or $properties.PackageType -ne 'None') {
    throw 'The evaluated host must use WindowsAppSDKSelfContained=true and WindowsPackageType=None.'
}
$output = $properties.OutDir
$executable = $properties.TargetPath
$product = [IO.Path]::GetFullPath($properties.Product)
if (!(Test-Path $executable) -or !(Test-Path $properties.ApplicationManifest)) {
    throw 'Build the supported selfcontained host before staging.'
}
# Retire only the obsolete external manifest owned by the earlier host probe.
Remove-Item "$executable.manifest" -ErrorAction SilentlyContinue
$deployment = @{}
foreach ($line in Get-Content "$output\ProfilerHostPayloads.txt") {
    $source, $destination = $line -split '\|', 2
    $destination = [IO.Path]::GetFullPath($destination)
    $name = [IO.Path]::GetFileName($destination)
    $hash = (Get-FileHash $destination -Algorithm SHA256).Hash
    if ($hash -ne (Get-FileHash $source -Algorithm SHA256).Hash) { throw "Payload mismatch: $destination" }
    $local = Join-Path $product $name
    if ((Test-Path $local) -and $hash -ne (Get-FileHash $local -Algorithm SHA256).Hash) {
        throw "Local Product payload was replaced by a package: $name"
    }
    $deployment[$destination] = [ordered]@{ Source = $source; Path = $destination; SHA256 = $hash }
}
if (!$deployment.ContainsKey([IO.Path]::GetFullPath("$output\Microsoft.UI.Xaml.dll"))) {
    throw 'The evaluated SDK deployment does not include the local WinUI runtime.'
}
$deployment.Values | ConvertTo-Json -Depth 4 | Set-Content "$output\ProfilerDeployment.json"
Write-Output "SELFCONTAINED_DEPLOYMENT: $output Foundation=$($properties.Foundation) IXP=$($properties.IXP) Base=$($properties.Base)"
if (!$Run) { return }
if ($Flavor.StartsWith('arm64') -and $env:PROCESSOR_ARCHITECTURE -ne 'ARM64') {
    throw 'ARM64 cross-build is not runtime execution on this host.'
}
$stdout = "$output\ProfilerHostRuntime.log"
$stderr = "$output\ProfilerHostRuntime.err.log"
$process = Start-Process $executable -ArgumentList '--verify-deployment' -WorkingDirectory $output `
    -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    do {
        Start-Sleep -Milliseconds 100
        $ready = (Test-Path $stdout) -and ((Get-Content $stdout -Raw) -match 'HOST_READY')
        $process.Refresh()
    } while (!$ready -and !$process.HasExited -and [DateTime]::UtcNow -lt $deadline)
    if (!$ready) { throw "Selfcontained host failed readiness. See $stdout and $stderr" }
    $loaded = @()
    foreach ($module in $process.Modules) {
        $actualPath = [IO.Path]::GetFullPath($module.FileName)
        if ($actualPath.StartsWith([IO.Path]::GetFullPath($output), [StringComparison]::OrdinalIgnoreCase) -and
            $module.ModuleName.EndsWith('.dll', [StringComparison]::OrdinalIgnoreCase) -and
            !$deployment.ContainsKey($actualPath)) {
            throw "Unmanifested runtime payload was loaded: $actualPath"
        }
        $expected = @($deployment.Values | Where-Object { [IO.Path]::GetFileName($_.Path) -ieq $module.ModuleName })
        if (!$expected.Count) { continue }
        if (!$deployment.ContainsKey($actualPath)) { throw "Runtime loaded outside the evaluated deployment: $actualPath" }
        $hash = (Get-FileHash $actualPath -Algorithm SHA256).Hash
        if ($hash -ne $deployment[$actualPath].SHA256) { throw "Loaded runtime hash mismatch: $actualPath" }
        $loaded += [ordered]@{ Path = $actualPath; SHA256 = $hash }
        Write-Output "LOADED_VERIFIED: $actualPath SHA256=$hash"
    }
    if (!($loaded | Where-Object { [IO.Path]::GetFileName($_.Path) -ieq 'Microsoft.UI.Xaml.dll' })) {
        throw 'The actual process did not load the deployed WinUI product.'
    }
    $loaded | ConvertTo-Json -Depth 4 | Set-Content "$output\ProfilerLoadedModules.json"
    if (!$process.WaitForExit(20000)) { throw 'Selfcontained host did not finish its bounded smoke test.' }
    $process.Refresh()
    Get-Content $stdout
    if ($process.ExitCode -ne 0) { throw "Selfcontained host exited $($process.ExitCode)" }
}
finally {
    $process.Refresh()
    if (!$process.HasExited) { Stop-Process -Id $process.Id }
}

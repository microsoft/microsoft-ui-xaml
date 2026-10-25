param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [ValidateSet('x64', 'ARM64')][string]$Architecture = 'x64',
    [switch]$Restore,
    [switch]$Solution
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$tool = Join-Path $root 'tools\WinUIProfiler'
$flavor = if ($Architecture -eq 'x64') { 'amd64' } else { 'arm64' }
$flavor += if ($Configuration -eq 'Debug') { 'chk' } else { 'fre' }
$project = Join-Path $tool 'WinUIProfiler.App\WinUIProfiler.App.csproj'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path $vswhere)) { throw 'Visual Studio MSBuild is required for the WinUI XAML compiler.' }
$msbuild = @(& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\amd64\MSBuild.exe') |
    Select-Object -First 1
if (!$msbuild) { throw 'The x64 Visual Studio MSBuild installation was not found.' }
Push-Location $root
try {
    # Product init selects its own .NET SDK. Keep that environment out of the
    # standalone tool's independently pinned SDK resolution.
    $powershell = (Get-Process -Id $PID).Path
    $nativeProject = Join-Path $root 'dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj'
    $evaluation = @(& $powershell -NoProfile -File .\initrun.ps1 -Flavor $flavor msb /q $nativeProject /getProperty:ArtifactsBinDir,MSBuildProjectFullPath)
    if ($LASTEXITCODE -ne 0) { throw 'Local Product property evaluation failed.' }
    $jsonStart = [Array]::IndexOf($evaluation, '{')
    if ($jsonStart -lt 0) {
        throw "MSBuild did not return evaluated Product properties as JSON. Evaluation output:`n$($evaluation -join [Environment]::NewLine)"
    }
    $evaluated = ($evaluation[$jsonStart..($evaluation.Count - 1)] -join [Environment]::NewLine | ConvertFrom-Json).Properties
    if ($evaluated.MSBuildProjectFullPath -ine $nativeProject -or
        ![IO.Path]::IsPathFullyQualified([string]$evaluated.ArtifactsBinDir)) {
        throw 'MSBuild Product properties do not identify the expected native project and absolute artifact directory.'
    }
    $product = [IO.Path]::GetFullPath((Join-Path $evaluated.ArtifactsBinDir 'Product')) + '\'
    $buildProject = if ($Solution) { Join-Path $tool 'WinUIProfiler.sln' } else { $project }
    $arguments = @($buildProject, '/t:Build', "/p:Configuration=$Configuration", "/p:Platform=$Architecture",
        "/p:ProfilerLocalProduct=$product", '/v:minimal', '/nologo')
    if ($Solution) {
        $vs = [IO.Path]::GetFullPath((Join-Path (Split-Path $msbuild) '..\..\..\..'))
        $toolset = Get-ChildItem (Join-Path $vs "MSBuild\Microsoft\VC\v*\Platforms\$Architecture\PlatformToolsets\v*") -Directory |
            Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name
        if (!$toolset) { throw "Installed C++ platform toolset for $Architecture was not found." }
        $arguments += "/p:PlatformToolset=$toolset"
    }
    if ($Restore) { $arguments += '/restore' }
    Set-Location $tool
    & $msbuild @arguments
    if ($LASTEXITCODE -ne 0) { throw 'Profiler application build failed.' }
    $outputRecord = Join-Path $tool "WinUIProfiler.App\obj\ProfilerAppOutput.$Architecture.$Configuration.txt"
    $output = [IO.Path]::GetFullPath((Get-Content $outputRecord -Raw).Trim()).TrimEnd('\') + '\'
    $vs = [IO.Path]::GetFullPath((Join-Path (Split-Path $msbuild) '..\..\..\..'))
    $metricsToolset = Get-ChildItem (Join-Path $vs "MSBuild\Microsoft\VC\v*\Platforms\$Architecture\PlatformToolsets\v*") -Directory |
        Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name
    if (!$metricsToolset) { throw "The $Architecture native metric build tools are required." }
    & $msbuild (Join-Path $tool 'WinUIProfiler.Native\WinUIProfiler.Native.vcxproj') /t:Build `
        "/p:Configuration=$Configuration" "/p:Platform=$Architecture" "/p:PlatformToolset=$metricsToolset" /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) { throw 'Native metric collector build failed.' }
    $metricsSource = Join-Path $tool "WinUIProfiler.Native\bin\$Architecture\$Configuration\WinUIProfiler.Native.dll"
    $metricsDestination = Join-Path $output 'WinUIProfiler.Native.dll'
    Copy-Item -LiteralPath $metricsSource -Destination $metricsDestination -Force
    $metricsHash = (Get-FileHash -LiteralPath $metricsSource -Algorithm SHA256).Hash
    if ((Get-FileHash -LiteralPath $metricsDestination -Algorithm SHA256).Hash -ne $metricsHash) {
        throw 'Native metric collector copy hash mismatch.'
    }
    $payloadHashes = @()
    foreach ($payloadArchitecture in @('x64', 'ARM64')) {
        $payloadToolset = Get-ChildItem (Join-Path $vs "MSBuild\Microsoft\VC\v*\Platforms\$payloadArchitecture\PlatformToolsets\v*") -Directory |
            Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name
        if (!$payloadToolset) { throw "The $payloadArchitecture injector/TAP build tools are required." }
        $dest = Join-Path $output "ConnectionPayloads\$payloadArchitecture"
        $null = New-Item -ItemType Directory -Path $dest -Force
        $hashes = @{}
        foreach ($component in @('Injector', 'Tap')) {
            $native = Join-Path $tool "WinUIProfiler.$component\WinUIProfiler.$component.vcxproj"
            & $msbuild $native /t:Build "/p:Configuration=$Configuration" "/p:Platform=$payloadArchitecture" "/p:PlatformToolset=$payloadToolset" /v:minimal /nologo
            if ($LASTEXITCODE -ne 0) { throw "$payloadArchitecture $component build failed." }
            $extension = if ($component -eq 'Injector') { 'exe' } else { 'dll' }
            $file = "WinUIProfiler.$component.$extension"
            $source = Join-Path $tool "WinUIProfiler.$component\bin\$payloadArchitecture\$Configuration\$file"
            $destination = Join-Path $dest $file
            Copy-Item -LiteralPath $source -Destination $destination -Force
            $hash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
            if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $hash) {
                throw "Connection payload copy hash mismatch: $destination"
            }
            $hashes[$component] = $hash
        }
        $payloadHashes += [ordered]@{
            Architecture = $payloadArchitecture
            HelperSHA256 = $hashes.Injector
            TapSHA256 = $hashes.Tap
        }
    }
    [ordered]@{ SchemaVersion = 1; Payloads = @($payloadHashes) } |
        ConvertTo-Json -Depth 5 | Set-Content (Join-Path $output 'ConnectionPayloads\manifest.json') -Encoding UTF8
    $properties = @{}
    Get-Content (Join-Path $output 'ProfilerApp.properties') | ForEach-Object {
        $key, $value = $_ -split '=', 2
        $properties[$key] = $value
    }
    if ($properties.SelfContained -ne 'true' -or $properties.DotNetSelfContained -ne 'true' -or
        $properties.PackageType -ne 'None' -or $properties.Platform -ne $Architecture) {
        throw 'The evaluated application is not the requested unpackaged, self-contained architecture.'
    }
    $files = @{}
    foreach ($line in Get-Content (Join-Path $output 'ProfilerAppPayloads.txt')) {
        $source, $destination = $line -split '\|', 2
        $source = [IO.Path]::GetFullPath($source)
        $destination = [IO.Path]::GetFullPath($destination)
        if (!$destination.StartsWith($output, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Runtime payload escapes the evaluated output: $destination"
        }
        $hash = (Get-FileHash $destination -Algorithm SHA256).Hash
        if ($hash -ne (Get-FileHash $source -Algorithm SHA256).Hash) { throw "Payload mismatch: $destination" }
        $relative = $destination.Substring($output.Length)
        $local = Join-Path $product $relative
        if ((Test-Path $local -PathType Leaf) -and $hash -ne (Get-FileHash $local -Algorithm SHA256).Hash) {
            throw "A package replaced the matched local runtime payload: $relative"
        }
        if ($files.ContainsKey($destination) -and $files[$destination].SHA256 -ne $hash) {
            throw "Conflicting runtime payloads: $destination"
        }
        $files[$destination] = [ordered]@{ Source = $source; Path = $destination; SHA256 = $hash }
    }
    $xaml = Join-Path $output 'Microsoft.UI.Xaml.dll'
    if (!$files.ContainsKey($xaml)) { throw 'The complete local WinUI component was not deployed.' }
    $reader = [IO.BinaryReader]::new([IO.File]::OpenRead($xaml))
    try {
        $reader.BaseStream.Position = 0x3c
        $reader.BaseStream.Position = $reader.ReadUInt32()
        if ($reader.ReadUInt32() -ne 0x4550) { throw 'The local WinUI runtime is not a PE image.' }
        $expectedMachine = if ($Architecture -eq 'x64') { 0x8664 } else { 0xaa64 }
        if ($reader.ReadUInt16() -ne $expectedMachine) { throw 'The local WinUI architecture does not match the application.' }
    }
    finally { $reader.Dispose() }
    $manifest = Join-Path $output 'ProfilerAppDeployment.json'
    [ordered]@{
        Architecture = $Architecture
        Executable = $properties.Executable
        Product = $product
        NativeMetricsPayload = [ordered]@{ Source = $metricsSource; Path = $metricsDestination; SHA256 = $metricsHash }
        RuntimePayloads = @($files.Values)
    } | ConvertTo-Json -Depth 5 | Set-Content $manifest -Encoding UTF8
    [pscustomobject]@{ Executable = $properties.Executable; Deployment = $manifest; OutputDirectory = $output }
}
finally { Pop-Location }

[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [Alias('Architecture')][ValidateSet('x64', 'ARM64')][string]$Platform = 'x64'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio C++ build tools and MSBuild are required.' }
$msbuild = @(& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\amd64\MSBuild.exe') |
    Select-Object -First 1
if (!$msbuild) { throw 'Visual Studio x64 MSBuild was not found.' }
foreach ($architecture in @('x64', 'ARM64')) {
    foreach ($project in @('WinUIProfiler.Tap', 'WinUIProfiler.Injector')) {
        & $msbuild (Join-Path $root "$project\$project.vcxproj") /t:Build "/p:Configuration=$Configuration" "/p:Platform=$architecture" /nologo /v:quiet
        if ($LASTEXITCODE -ne 0) { throw "Building $project for $architecture failed." }
    }
}
& $msbuild (Join-Path $root 'WinUIProfiler.Native\WinUIProfiler.Native.vcxproj') /t:Build `
    "/p:Configuration=$Configuration" "/p:Platform=$Platform" /nologo /v:quiet
if ($LASTEXITCODE -ne 0) { throw 'Building the native process metrics library failed.' }
& $msbuild (Join-Path $root 'WinUIProfiler.App\WinUIProfiler.App.csproj') /restore /t:Build `
    "/p:Configuration=$Configuration" "/p:Platform=$Platform" /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw 'Building the self-contained profiler executable failed.' }
$outputFile = Join-Path $root "WinUIProfiler.App\obj\ProfilerAppOutput.$Platform.$Configuration.txt"
$output = (Get-Content -LiteralPath $outputFile -Raw).Trim()
if (![IO.Path]::IsPathRooted($output) -or $output -ne [IO.Path]::GetFullPath($output)) {
    throw 'The evaluated executable output is not an absolute canonical path.'
}
$payloadDirectory = Join-Path $output 'ConnectionPayloads'
$null = New-Item -ItemType Directory -Path $payloadDirectory -Force
$entries = foreach ($architecture in @('x64', 'ARM64')) {
    $directory = Join-Path $payloadDirectory $architecture
    $null = New-Item -ItemType Directory -Path $directory -Force
    foreach ($item in @(@('WinUIProfiler.Tap', 'dll'), @('WinUIProfiler.Injector', 'exe'))) {
        $file = "$($item[0]).$($item[1])"
        $source = Join-Path $root "$($item[0])\bin\$architecture\$Configuration\$file"
        $destination = Join-Path $directory $file
        Copy-Item -LiteralPath $source -Destination $destination
        if ((Get-FileHash -LiteralPath $source).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) {
            throw "Deployment hash mismatch: $destination"
        }
    }
    [ordered]@{
        Architecture = $architecture
        HelperSHA256 = (Get-FileHash -LiteralPath (Join-Path $directory 'WinUIProfiler.Injector.exe')).Hash
        TapSHA256 = (Get-FileHash -LiteralPath (Join-Path $directory 'WinUIProfiler.Tap.dll')).Hash
    }
}
$manifest = [ordered]@{ SchemaVersion = 1; Payloads = @($entries) } | ConvertTo-Json -Depth 4
[IO.File]::WriteAllText((Join-Path $payloadDirectory 'manifest.json'), $manifest, [Text.UTF8Encoding]::new($false))
$native = Join-Path $root "WinUIProfiler.Native\bin\$Platform\$Configuration\WinUIProfiler.Native.dll"
Copy-Item -LiteralPath $native -Destination (Join-Path $output 'WinUIProfiler.Native.dll')
foreach ($file in @('WinUIProfiler.App.exe', 'Microsoft.UI.Xaml.dll', 'WinUIProfiler.Native.dll')) {
    if (!(Test-Path -LiteralPath (Join-Path $output $file) -PathType Leaf)) { throw "Required executable payload missing: $file" }
}
Write-Output "Profiler executable: $(Join-Path $output 'WinUIProfiler.App.exe')"

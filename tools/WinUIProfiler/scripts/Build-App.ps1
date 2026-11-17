[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [Alias('Architecture')][ValidateSet('x64', 'ARM64')][string]$Platform = 'x64'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$missingVisualStudio = 'Visual Studio 2022 or 2026 with MSBuild and the C++ x64 and ARM64 build tools is required. ' +
    'Run .\scripts\Install-Prerequisites.ps1 to install it.'
if (!(Test-Path -LiteralPath $vswhere)) { throw $missingVisualStudio }
$msbuild = @(& $vswhere -latest -products '*' -version '[17.0,19.0)' -requires Microsoft.Component.MSBuild `
        Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.Tools.ARM64 `
        -find 'MSBuild\**\Bin\amd64\MSBuild.exe') | Select-Object -First 1
if (!$msbuild) { throw $missingVisualStudio }
if (!(Get-Command dotnet -CommandType Application -ErrorAction SilentlyContinue)) {
    throw 'The .NET SDK is required. Run .\scripts\Install-Prerequisites.ps1 to install it.'
}
$appProject = Join-Path $root 'WinUIProfiler.App\WinUIProfiler.App.csproj'
# dotnet takes the SDK from the global.json above the working directory, so it runs from this tool's folder.
Push-Location -LiteralPath $root
try { & dotnet restore $appProject "-p:Configuration=$Configuration" "-p:Platform=$Platform" -nologo -v:quiet }
finally { Pop-Location }
if ($LASTEXITCODE -ne 0) { throw 'Restoring the SDK metadata for the app and tree snapshot TAP failed.' }
foreach ($architecture in @('x64', 'ARM64')) {
    foreach ($project in @('WinUIProfiler.Tap', 'WinUIProfiler.Injector')) {
        & $msbuild (Join-Path $root "$project\$project.vcxproj") /t:Build "/p:Configuration=$Configuration" "/p:Platform=$architecture" /nologo /v:quiet
        if ($LASTEXITCODE -ne 0) { throw "Building $project for $architecture failed." }
    }
    $null = & (Join-Path $PSScriptRoot 'Build-Launcher.ps1') -Architecture $architecture
}
& $msbuild (Join-Path $root 'WinUIProfiler.Native\WinUIProfiler.Native.vcxproj') /t:Build `
    "/p:Configuration=$Configuration" "/p:Platform=$Platform" /nologo /v:quiet
if ($LASTEXITCODE -ne 0) { throw 'Building the native process metrics library failed.' }
Push-Location -LiteralPath $root
try {
    & dotnet build $appProject --no-restore "-p:Configuration=$Configuration" "-p:Platform=$Platform" -nologo -v:minimal
}
finally { Pop-Location }
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
$launcherDirectory = Join-Path $output 'NativeLauncher'
$launchers = foreach ($architecture in @('x64', 'ARM64')) {
    $directory = Join-Path $launcherDirectory $architecture
    $null = New-Item -ItemType Directory -Path $directory -Force
    foreach ($file in @('ProfilerLauncher.exe', 'ProfilerRedirect.dll', 'DETOURS-LICENSE.md')) {
        $source = Join-Path $root "artifacts\launcher\win-$($architecture.ToLowerInvariant())\$file"
        $destination = Join-Path $directory $file
        Copy-Item -LiteralPath $source -Destination $destination
        if ((Get-FileHash -LiteralPath $source).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) {
            throw "Deployment hash mismatch: $destination"
        }
    }
    [ordered]@{
        Architecture = $architecture
        LauncherSHA256 = (Get-FileHash -LiteralPath (Join-Path $directory 'ProfilerLauncher.exe')).Hash
        RedirectSHA256 = (Get-FileHash -LiteralPath (Join-Path $directory 'ProfilerRedirect.dll')).Hash
    }
}
$manifest = [ordered]@{ SchemaVersion = 1; Launchers = @($launchers) } | ConvertTo-Json -Depth 4
[IO.File]::WriteAllText((Join-Path $launcherDirectory 'manifest.json'), $manifest, [Text.UTF8Encoding]::new($false))
$native =  Join-Path $root "WinUIProfiler.Native\bin\$Platform\$Configuration\WinUIProfiler.Native.dll"
Copy-Item -LiteralPath $native -Destination (Join-Path $output 'WinUIProfiler.Native.dll')
foreach ($file in @('WinUIProfiler.App.exe', 'Microsoft.UI.Xaml.dll', 'WinUIProfiler.Native.dll')) {
    if (!(Test-Path -LiteralPath (Join-Path $output $file) -PathType Leaf)) { throw "Required executable payload missing: $file" }
}
Write-Output "Profiler executable: $(Join-Path $output 'WinUIProfiler.App.exe')"

[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [ValidateSet('x64', 'ARM64')][string]$Platform = 'x64',
    [ValidateSet('Build', 'Stage')][string]$Operation = 'Build',
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
if ($Operation -eq 'Build') {
    $metadata = @(
        'microsoft.windowsappsdk.winui\2.3.9\metadata\Microsoft.UI.Xaml.winmd',
        'microsoft.windowsappsdk.interactiveexperiences\2.1.9\metadata\10.0.18362.0\Microsoft.UI.winmd',
        'microsoft.windowsappsdk.foundation\2.3.12\metadata\Microsoft.Windows.Foundation.winmd',
        'microsoft.web.webview2\1.0.3719.77\lib\Microsoft.Web.WebView2.Core.winmd'
    )
    if (@($metadata | Where-Object { !(Test-Path -LiteralPath (Join-Path $root ".packages\$_") -PathType Leaf) }).Count) {
        # TAP uses public SDK projections; this does not add a WinUI dependency to the console host.
        Push-Location -LiteralPath $root
        try {
            & dotnet restore (Join-Path $root 'WinUIProfiler.App\WinUIProfiler.App.csproj') `
                "-p:Configuration=$Configuration" "-p:Platform=$Platform" -nologo -v:quiet
            if ($LASTEXITCODE -ne 0) { throw 'Restoring public SDK metadata for the native TAP failed.' }
        }
        finally { Pop-Location }
    }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio with the C++ x64 and ARM64 build tools is required.' }
    $msbuild = @(& $vswhere -latest -products '*' -version '[17.0,19.0)' -requires Microsoft.Component.MSBuild `
        Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.Tools.ARM64 `
        -find 'MSBuild\**\Bin\amd64\MSBuild.exe') | Select-Object -First 1
    if (!$msbuild) { throw 'Visual Studio with the C++ x64 and ARM64 build tools is required.' }
    foreach ($architecture in @('x64', 'ARM64')) {
        foreach ($project in @('WinUIProfiler.Tap', 'WinUIProfiler.Injector')) {
            & $msbuild (Join-Path $root "$project\$project.vcxproj") /t:Build "/p:Configuration=$Configuration" "/p:Platform=$architecture" /nologo /v:quiet
            if ($LASTEXITCODE -ne 0) { throw "Building $project for $architecture failed." }
        }
        $null = & (Join-Path $PSScriptRoot 'Build-Launcher.ps1') -Architecture $architecture
    }
    & $msbuild (Join-Path $root 'WinUIProfiler.Native\WinUIProfiler.Native.vcxproj') /t:Build `
        "/p:Configuration=$Configuration" "/p:Platform=$Platform" /nologo /v:quiet
    if ($LASTEXITCODE -ne 0) { throw 'Building native process metrics failed.' }
    return
}
if (!$OutputDirectory -or ![IO.Path]::IsPathRooted($OutputDirectory) -or
    $OutputDirectory -ne [IO.Path]::GetFullPath($OutputDirectory)) { throw 'Staging needs an absolute output directory.' }
function Copy-Verified([string]$Source, [string]$Destination) {
    Copy-Item -LiteralPath $Source -Destination $Destination
    if ((Get-FileHash -LiteralPath $Source).Hash -ne (Get-FileHash -LiteralPath $Destination).Hash) {
        throw "Deployment hash mismatch: $Destination"
    }
}
$payloadDirectory = Join-Path $OutputDirectory 'ConnectionPayloads'
$entries = foreach ($architecture in @('x64', 'ARM64')) {
    $directory = Join-Path $payloadDirectory $architecture
    $null = New-Item -ItemType Directory -Path $directory -Force
    foreach ($item in @(@('WinUIProfiler.Tap', 'dll'), @('WinUIProfiler.Injector', 'exe'))) {
        $file = "$($item[0]).$($item[1])"
        Copy-Verified (Join-Path $root "$($item[0])\bin\$architecture\$Configuration\$file") (Join-Path $directory $file)
    }
    [ordered]@{
        Architecture = $architecture
        HelperSHA256 = (Get-FileHash -LiteralPath (Join-Path $directory 'WinUIProfiler.Injector.exe')).Hash
        TapSHA256 = (Get-FileHash -LiteralPath (Join-Path $directory 'WinUIProfiler.Tap.dll')).Hash
    }
}
$manifest = [ordered]@{ SchemaVersion = 1; Payloads = @($entries) } | ConvertTo-Json -Depth 4
[IO.File]::WriteAllText((Join-Path $payloadDirectory 'manifest.json'), $manifest, [Text.UTF8Encoding]::new($false))
$launcherDirectory = Join-Path $OutputDirectory 'NativeLauncher'
$launchers = foreach ($architecture in @('x64', 'ARM64')) {
    $directory = Join-Path $launcherDirectory $architecture
    $null = New-Item -ItemType Directory -Path $directory -Force
    foreach ($file in @('ProfilerLauncher.exe', 'ProfilerRedirect.dll', 'DETOURS-LICENSE.md')) {
        Copy-Verified (Join-Path $root "artifacts\launcher\win-$($architecture.ToLowerInvariant())\$file") (Join-Path $directory $file)
    }
    [ordered]@{
        Architecture = $architecture
        LauncherSHA256 = (Get-FileHash -LiteralPath (Join-Path $directory 'ProfilerLauncher.exe')).Hash
        RedirectSHA256 = (Get-FileHash -LiteralPath (Join-Path $directory 'ProfilerRedirect.dll')).Hash
    }
}
$manifest = [ordered]@{ SchemaVersion = 1; Launchers = @($launchers) } | ConvertTo-Json -Depth 4
[IO.File]::WriteAllText((Join-Path $launcherDirectory 'manifest.json'), $manifest, [Text.UTF8Encoding]::new($false))
Copy-Verified (Join-Path $root "WinUIProfiler.Native\bin\$Platform\$Configuration\WinUIProfiler.Native.dll") `
    (Join-Path $OutputDirectory 'WinUIProfiler.Native.dll')

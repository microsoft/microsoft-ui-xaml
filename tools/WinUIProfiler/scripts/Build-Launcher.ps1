[CmdletBinding()]
param(
    [ValidateSet('x64', 'ARM64')][string]$Architecture = 'x64'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Native tools write progress to stderr; Windows PowerShell treats that as an error under 'Stop'.
function Invoke-Git {
    $ErrorActionPreference = 'Continue'
    $output = @(& git @args 2>&1 | ForEach-Object { "$_" })
    if ($LASTEXITCODE -ne 0) { throw "git $($args -join ' ') failed:`n$($output -join "`n")" }
    $output
}

$root = Split-Path $PSScriptRoot
$source = Join-Path $root 'WinUIProfiler.Launcher'
$arch = $Architecture.ToLowerInvariant()
$dependencies = Get-Content -LiteralPath (Join-Path $source 'dependencies.json') -Raw | ConvertFrom-Json
$repository = [string]$dependencies.detours.repository
$revision = [string]$dependencies.detours.revision
if ($revision -cnotmatch '^[0-9a-f]{40}$') { throw 'The Detours dependency must pin a full commit SHA.' }
if (!(Get-Command git -ErrorAction SilentlyContinue)) { throw 'Git is required to download the pinned Detours source.' }

$dependency = Join-Path $root "artifacts\launcher\dependencies\detours-$revision"
if (!(Test-Path -LiteralPath $dependency)) {
    $download = "$dependency.$([Guid]::NewGuid().ToString('N').Substring(0, 8))"
    try {
        $null = Invoke-Git clone --quiet --no-checkout $repository $download
        $null = Invoke-Git -C $download checkout --quiet --detach $revision
        Move-Item -LiteralPath $download -Destination $dependency
    } finally {
        if (Test-Path -LiteralPath $download) { Remove-Item -LiteralPath $download -Recurse -Force }
    }
}
$actual = (Invoke-Git -C $dependency rev-parse HEAD | Select-Object -Last 1).Trim()
if ($actual -cne $revision) { throw "Unexpected Detours revision in $dependency; delete the folder and build again." }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio C++ build tools are required to build the private WinUI launcher.' }
$component = if ($arch -eq 'arm64') { 'Microsoft.VisualStudio.Component.VC.Tools.ARM64' } else {
    'Microsoft.VisualStudio.Component.VC.Tools.x86.x64'
}
$visualStudio = @(& $vswhere -latest -products '*' -requires $component -property installationPath) | Select-Object -First 1
if (!$visualStudio) { throw "The Visual Studio C++ $Architecture build tools ($component) are missing." }

$output = Join-Path $root "artifacts\launcher\win-$arch"
$null = New-Item -ItemType Directory -Path $output -Force
foreach ($name in @('ProfilerLauncher.exe', 'ProfilerRedirect.dll')) {
    Remove-Item -LiteralPath (Join-Path $output $name) -Force -ErrorAction SilentlyContinue
}
$hostArchitecture = try {
    [Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLowerInvariant()
} catch { 'x64' }
$log = Join-Path $output 'build.log'
$build = Join-Path $source 'build.cmd'
& $env:ComSpec /d /c "call `"$build`" `"$visualStudio`" `"$dependency`" `"$source`" `"$output`" $arch $hostArchitecture > `"$log`" 2>&1"
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $log -Tail 30 | Write-Host
    throw "Building the private WinUI launcher for $Architecture failed. See $log."
}
$license = [IO.File]::ReadAllText((Join-Path $dependency 'LICENSE.md')).Replace("`r`n", "`n")
[IO.File]::WriteAllText((Join-Path $output 'DETOURS-LICENSE.md'), $license, [Text.UTF8Encoding]::new($false))
foreach ($name in @('ProfilerLauncher.exe', 'ProfilerRedirect.dll')) {
    if (!(Test-Path -LiteralPath (Join-Path $output $name) -PathType Leaf)) { throw "The launcher build did not produce $name." }
}
Write-Output "Private WinUI launcher ($Architecture): $output"

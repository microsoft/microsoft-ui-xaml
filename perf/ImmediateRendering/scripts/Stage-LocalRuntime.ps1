<#
.SYNOPSIS
Copies a locally built WinUI runtime next to RenderBench.exe so runs measure local bits (with the
Microsoft-WinUI-RenderingBackend seam) instead of the shipping WinAppSDK package.

.DESCRIPTION
Copies everything from BuildOutput\packaging\<Configuration>\runtimes\win10-x64\native. A partial swap (only
Microsoft.UI.Xaml.dll) crashes with 0xC000027B because the WinUI component DLLs must match each other.
Use -Restore to rebuild the harness and go back to the package runtime.
#>
[CmdletBinding()]
param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',
    [string]$BenchDir,
    [switch]$Restore
)

$ErrorActionPreference = 'Stop'
$kitRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
if (-not $BenchDir) {
    $BenchDir = Join-Path $kitRoot 'bin\RenderBench\x64\Release\net10.0-windows10.0.26100.0\win-x64'
}
if ($Restore) {
    dotnet build (Join-Path $kitRoot 'RenderBench\RenderBench.csproj') -c Release --no-incremental -v q
    return
}

$native = Join-Path $RepoRoot "BuildOutput\packaging\$Configuration\runtimes\win10-x64\native"
if (-not (Test-Path (Join-Path $native 'Microsoft.UI.Xaml.dll'))) {
    throw "No local runtime at $native. Build the product first (.\initrun.ps1 -Flavor amd64fre .\build.cmd /q product)."
}
if (-not (Test-Path (Join-Path $BenchDir 'RenderBench.exe'))) {
    throw "RenderBench.exe not found in $BenchDir. Build it first."
}
$files = Get-ChildItem $native -File -Recurse
foreach ($file in $files) {
    $destination = Join-Path $BenchDir $file.FullName.Substring($native.Length + 1)
    New-Item -ItemType Directory -Force (Split-Path $destination) | Out-Null
    Copy-Item $file.FullName $destination -Force
}
$mux = Get-Item (Join-Path $BenchDir 'Microsoft.UI.Xaml.dll')
Write-Host "Staged $($files.Count) files from $native"
Write-Host "Microsoft.UI.Xaml.dll $($mux.Length) bytes, $($mux.LastWriteTime)"

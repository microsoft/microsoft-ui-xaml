<#
.SYNOPSIS
  Produces a redistributable, double-clickable OSFrameworkLens build.

  Output:  dist\OSFrameworkLens\OSFrameworkLens.exe  (~150 MB, self-contained win-x64)

  Single-file self-contained — no .NET install required on target machines.
  Just copy the dist\OSFrameworkLens folder anywhere and double-click the exe.
#>

[CmdletBinding()]
param(
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Set-Location $PSScriptRoot

$dist   = Join-Path $PSScriptRoot 'dist'
$appOut = Join-Path $dist          'OSFrameworkLens'

if (Test-Path $dist) { Remove-Item $dist -Recurse -Force }

$publishProps = @(
    '-p:PublishSingleFile=true'
    '-p:SelfContained=true'
    '-p:IncludeNativeLibrariesForSelfExtract=true'
    '-p:EnableCompressionInSingleFile=true'
    '-p:DebugType=embedded'
    '-p:DebugSymbols=false'
)

Write-Host '==> Publishing main app...' -ForegroundColor Cyan
& dotnet publish src\OSFrameworkLens -c $Configuration -r win-x64 -o $appOut --nologo @publishProps
if ($LASTEXITCODE -ne 0) { throw "App publish failed (exit $LASTEXITCODE)" }

Write-Host '==> Final output:' -ForegroundColor Green
Get-ChildItem $appOut |
    Sort-Object Length -Descending |
    ForEach-Object { '{0,12:N0} bytes   {1}' -f $_.Length, $_.Name } |
    Write-Host

$mainExe = Join-Path $appOut 'OSFrameworkLens.exe'
Write-Host ''
Write-Host "Double-click to launch:" -ForegroundColor Green
Write-Host "  $mainExe"

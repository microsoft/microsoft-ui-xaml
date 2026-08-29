# Builds, publishes, and packages a Microsoft Performance Toolkit plugin into a
# .ptix file usable by the WPA Plugin Manager.
#
# This script is intentionally plugin-agnostic: it works for ANY plugin whose
# project directory contains a .csproj + a pluginManifest.json. Drop it next to
# any plugin's project file and run it.
#
# Steps:
#   1. dotnet publish -> PluginPackage\ (used by the dev workflow with
#      `wpa -addsearchdir PluginPackage`).
#   2. Mirror PluginPackage -> PtixSource\, stripping Microsoft.Performance.SDK*
#      DLLs, .pdb files, and pluginManifest.json. The Plugins.Cli refuses to
#      pack if the SDK runtime is bundled (WPA loads its own copy).
#   3. plugintool pack -> <id>-<version>.ptix at the project root.
#      <id> and <version> come from pluginManifest.json's "identity" block, so
#      the output name is correct for whichever plugin this is run against.
#
# Prerequisites:
#   dotnet tool install --global Microsoft.Performance.Toolkit.Plugins.Cli --version 0.1.77-preview
#
# Note: the console logger in plugintool 0.1.77-preview won't flush on early
# exit unless Logging__Console__FormatterName=simple is set, so we set it here
# to surface validation errors.
#
# Parameters (all optional — sensible defaults work for the typical layout):
#   -Configuration   Debug | Release. Default: Debug.
#   -Manifest        Path to pluginManifest.json. Default: <script-dir>\pluginManifest.json.
#   -PublishDir      Where `dotnet publish` writes binaries. Default: <script-dir>\PluginPackage.
#   -StagingDir      Mirrored, SDK-stripped folder fed to plugintool. Default: <script-dir>\PtixSource.
#   -OutputDir       Where the final .ptix is written. Default: <script-dir>.

[CmdletBinding()]
param(
    [string]$Configuration = 'Debug',
    [string]$Manifest,
    [string]$PublishDir,
    [string]$StagingDir,
    [string]$OutputDir
)

$ErrorActionPreference = 'Stop'
Set-Location -Path $PSScriptRoot

if (-not $Manifest)   { $Manifest   = Join-Path $PSScriptRoot 'pluginManifest.json' }
if (-not $PublishDir) { $PublishDir = Join-Path $PSScriptRoot 'PluginPackage' }
if (-not $StagingDir) { $StagingDir = Join-Path $PSScriptRoot 'PtixSource' }
if (-not $OutputDir)  { $OutputDir  = $PSScriptRoot }

if (-not (Test-Path $Manifest)) {
    throw "Manifest not found: $Manifest"
}

# plugintool is required for step 3 — fail fast with an actionable message.
if (-not (Get-Command plugintool -ErrorAction SilentlyContinue)) {
    throw "plugintool not found on PATH. Install with: dotnet tool install --global Microsoft.Performance.Toolkit.Plugins.Cli --version 0.1.77-preview"
}

$manifestJson = Get-Content $Manifest -Raw | ConvertFrom-Json
$pluginId     = $manifestJson.identity.id
$version      = $manifestJson.identity.version
if (-not $pluginId)  { throw "pluginManifest.json is missing identity.id" }
if (-not $version)   { throw "pluginManifest.json is missing identity.version" }

$ptixPath = Join-Path $OutputDir "$pluginId-$version.ptix"

Write-Host "[1/3] dotnet publish -c $Configuration -o `"$PublishDir`""
& dotnet publish -c $Configuration -o $PublishDir --nologo
if ($LASTEXITCODE -ne 0) { throw "dotnet publish failed (exit $LASTEXITCODE)" }

Write-Host "`n[2/3] Mirror PluginPackage -> PtixSource (stripping SDK runtime DLLs)"
if (Test-Path $StagingDir) { Remove-Item $StagingDir -Recurse -Force }
Copy-Item $PublishDir $StagingDir -Recurse
Get-ChildItem $StagingDir -Filter 'Microsoft.Performance.SDK*.dll' | Remove-Item -Force
Get-ChildItem $StagingDir -Filter '*.pdb' | Remove-Item -Force
Get-ChildItem $StagingDir -Filter 'pluginManifest.json' | Remove-Item -Force

Write-Host "`n[3/3] plugintool pack -> $ptixPath"
$env:Logging__Console__FormatterName = 'simple'
$env:Logging__Console__FormatterOptions__SingleLine = 'true'
& plugintool pack -s $StagingDir -m $Manifest -o $ptixPath -w
if ($LASTEXITCODE -ne 0) { throw "plugintool pack failed (exit $LASTEXITCODE)" }

$info = Get-Item $ptixPath
Write-Host "`nDone: $ptixPath ($([Math]::Round($info.Length/1MB,2)) MB)" -ForegroundColor Green

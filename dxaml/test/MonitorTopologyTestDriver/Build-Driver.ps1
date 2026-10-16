# SPDX-License-Identifier: MS-PL
# Compile/package only. Never installs, loads, signs, or trusts a driver.
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [switch]$CompileOnly,
    [string]$MSBuildPath = 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe'
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $MSBuildPath -PathType Leaf)) {
    throw "MSBuild was not found at '$MSBuildPath'. Supply -MSBuildPath for an existing VS 2022 C++/WDK installation. This script installs nothing."
}

$project = Join-Path $PSScriptRoot 'MonitorTopologyTestDriver\MonitorTopologyTestDriver.vcxproj'
$target = if ($CompileOnly) { 'ClCompile' } else { 'Build' }
& $MSBuildPath $project /nologo /m:1 "/t:$target" `
    "/p:Configuration=$Configuration" /p:Platform=x64 `
    /p:WindowsTargetPlatformVersion=10.0.26100.0 `
    /p:SignMode=Off /p:EnableTestSign=false /p:RunCodeAnalysis=false `
    /p:ImportDirectoryBuildProps=false /p:ImportDirectoryBuildTargets=false `
    /verbosity:minimal
if ($LASTEXITCODE -ne 0) {
    throw "Native build failed (exit $LASTEXITCODE). Requires existing VS 2022 C++, WDK 10.0.26100.0, UMDF 2.25 and IddCx 1.4. No driver was installed or loaded."
}

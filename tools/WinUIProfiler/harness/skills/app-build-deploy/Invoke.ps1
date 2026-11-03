[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Config,
    [switch]$DryRun,
    [switch]$SkipBuild
)

& (Join-Path $PSScriptRoot "..\..\Invoke-ProfilerHarness.ps1") `
    -Stage app-build-deploy -Config $Config -DryRun:$DryRun -SkipBuild:$SkipBuild


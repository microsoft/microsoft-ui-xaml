[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Config,
    [ValidateSet("agency", "github")][string]$Engine = "agency",
    [string]$ScenarioPath,
    [switch]$DryRun
)

& (Join-Path $PSScriptRoot "..\..\Invoke-ProfilerHarness.ps1") `
    -Stage profile-and-apply-changes -Config $Config -Engine $Engine `
    -ScenarioPath $ScenarioPath -DryRun:$DryRun


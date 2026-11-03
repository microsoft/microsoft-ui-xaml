[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Config,
    [Parameter(Mandatory)][string]$Prompt,
    [Parameter(Mandatory)][string]$Output,
    [ValidateSet("agency", "github")][string]$Engine = "agency",
    [switch]$DryRun
)

& (Join-Path $PSScriptRoot "..\..\Invoke-ProfilerHarness.ps1") `
    -Stage create-automation-test -Config $Config -ScenarioPrompt $Prompt `
    -ScenarioPath $Output -Engine $Engine -DryRun:$DryRun


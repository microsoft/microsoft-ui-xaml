[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Config,
    [Parameter(Mandatory)][string]$Prompt,
    [Parameter(Mandatory)][string]$Output,
    [ValidateSet("agency", "github")][string]$Engine = "agency",
    [switch]$DryRun,
    [ValidateSet("local", "hyperv", "ixp")][string]$TargetKind,
    [switch]$PrerequisitesConfirmed,
    [switch]$ApproveVmExecution
)

$targetParameters = @{ PrerequisitesConfirmed = $PrerequisitesConfirmed; ApproveVmExecution = $ApproveVmExecution }
if ($PSBoundParameters.ContainsKey("TargetKind")) { $targetParameters.TargetKind = $TargetKind }
& (Join-Path $PSScriptRoot "..\..\Invoke-ProfilerHarness.ps1") `
    -Stage create-automation-test -Config $Config -ScenarioPrompt $Prompt `
    -ScenarioPath $Output -Engine $Engine -DryRun:$DryRun @targetParameters

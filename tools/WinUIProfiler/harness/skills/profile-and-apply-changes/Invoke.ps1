[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Config,
    [ValidateSet("agency", "github")][string]$Engine = "agency",
    [string]$ScenarioPath,
    [switch]$DryRun,
    [ValidateSet("local", "hyperv", "ixp")][string]$TargetKind,
    [switch]$PrerequisitesConfirmed,
    [switch]$ApproveVmExecution
)

$targetParameters = @{ PrerequisitesConfirmed = $PrerequisitesConfirmed; ApproveVmExecution = $ApproveVmExecution }
if ($PSBoundParameters.ContainsKey("TargetKind")) { $targetParameters.TargetKind = $TargetKind }
& (Join-Path $PSScriptRoot "..\..\Invoke-ProfilerHarness.ps1") `
    -Stage profile-and-apply-changes -Config $Config -Engine $Engine `
    -ScenarioPath $ScenarioPath -DryRun:$DryRun @targetParameters

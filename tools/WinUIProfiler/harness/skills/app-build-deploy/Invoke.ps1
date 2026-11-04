[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Config,
    [switch]$DryRun,
    [switch]$SkipBuild,
    [ValidateSet("local", "hyperv", "ixp")][string]$TargetKind,
    [switch]$PrerequisitesConfirmed,
    [switch]$ApproveVmExecution
)

$targetParameters = @{ PrerequisitesConfirmed = $PrerequisitesConfirmed; ApproveVmExecution = $ApproveVmExecution }
if ($PSBoundParameters.ContainsKey("TargetKind")) { $targetParameters.TargetKind = $TargetKind }
& (Join-Path $PSScriptRoot "..\..\Invoke-ProfilerHarness.ps1") `
    -Stage app-build-deploy -Config $Config -DryRun:$DryRun -SkipBuild:$SkipBuild @targetParameters

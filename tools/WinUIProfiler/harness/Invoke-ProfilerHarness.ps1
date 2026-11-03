[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet(
        "validate",
        "system-ready",
        "app-build-deploy",
        "profiler-build-deploy",
        "create-automation-test",
        "capture",
        "profile-and-apply-changes")]
    [string]$Stage,

    [Parameter(Mandatory)]
    [string]$Config,

    [string]$ScenarioPrompt,
    [string]$ScenarioPath,

    [ValidateSet("agency", "github")]
    [string]$Engine,

    [switch]$DryRun,
    [switch]$SkipRepositoryPreparation,
    [switch]$SkipBuild,

    [ValidateSet("local", "hyperv", "ixp")]
    [string]$TargetKind,
    [switch]$PrerequisitesConfirmed,
    [switch]$ApproveVmExecution
)

$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "ProfilerHarness.psm1") -Force

$parameters = @{
    Stage = $Stage
    ConfigPath = $Config
    DryRun = $DryRun
    SkipRepositoryPreparation = $SkipRepositoryPreparation
    SkipBuild = $SkipBuild
    PrerequisitesConfirmed = $PrerequisitesConfirmed
    ApproveVmExecution = $ApproveVmExecution
}
if ($PSBoundParameters.ContainsKey("TargetKind")) { $parameters.TargetKind = $TargetKind }
if ($PSBoundParameters.ContainsKey("ScenarioPrompt")) { $parameters.ScenarioPrompt = $ScenarioPrompt }
if ($PSBoundParameters.ContainsKey("ScenarioPath")) { $parameters.ScenarioPath = $ScenarioPath }
if ($PSBoundParameters.ContainsKey("Engine")) { $parameters.Engine = $Engine }
Invoke-ProfilerHarness @parameters

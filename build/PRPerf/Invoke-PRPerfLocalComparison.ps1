[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $TrialRoot,
    [Parameter(Mandatory)][string] $TargetRoot,
    [Parameter(Mandatory)][ValidatePattern('^[0-9a-fA-F]{40}$')][string] $TrialCommit,
    [Parameter(Mandatory)][string] $TrialBuildId,
    [string] $BaselineCommit = '',
    [string] $BaselineBuildId = '',
    [Parameter(Mandatory)][string] $AgentName,
    [Parameter(Mandatory)][string] $OutputDirectory,
    [Parameter(Mandatory)][string] $ThresholdPath
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'PRPerfResults.psm1') -Force

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$measure = Join-Path $PSScriptRoot 'Measure-PRPerfLocalScenario.ps1'

$trialBinary = Select-PRPerfMeasurementBinary -Root $TrialRoot
Write-Host "Measuring the PR binary: $trialBinary"
& $measure -BinaryPath $trialBinary -Commit $TrialCommit -BuildId $TrialBuildId `
    -AgentName $AgentName -OutputPath (Join-Path $OutputDirectory 'pr-results.json')

$haveBaseline = $BaselineCommit -match '^[0-9a-fA-F]{40}$' -and
    (Test-Path -LiteralPath $TargetRoot -PathType Container)

$targetBinary = $null
if ($haveBaseline) {
    try {
        $targetBinary = Select-PRPerfMeasurementBinary -Root $TargetRoot
    } catch {
        Write-Host "##vso[task.logissue type=warning]The baseline drop had no binary to measure: $($_.Exception.Message)"
        $haveBaseline = $false
    }
}

if ($haveBaseline) {
    Write-Host "Measuring the baseline binary: $targetBinary"
    & $measure -BinaryPath $targetBinary -Commit $BaselineCommit -BuildId $BaselineBuildId `
        -AgentName $AgentName -OutputPath (Join-Path $OutputDirectory 'target-results.json')
    $effectiveCommit = $BaselineCommit
    $effectiveBuildId = $BaselineBuildId
} else {
    # With no baseline the honest thing is to measure the same binary on both sides and let
    # the comparer refuse to draw a conclusion from it. Measuring one side only would leave
    # an empty table, which reads exactly like a run that collected nothing at all.
    Write-Host '##vso[task.logissue type=warning]No baseline build was available, so both sides measure this build. The comparison will report Inconclusive.'
    & $measure -BinaryPath $trialBinary -Commit $TrialCommit -BuildId $TrialBuildId `
        -AgentName $AgentName -OutputPath (Join-Path $OutputDirectory 'target-results.json')
    $effectiveCommit = $TrialCommit
    $effectiveBuildId = $TrialBuildId
}

Write-Host "##vso[task.setvariable variable=effectiveTargetCommit]$effectiveCommit"
Write-Host "##vso[task.setvariable variable=effectiveTargetBuildId]$effectiveBuildId"
Write-Host "##vso[task.setvariable variable=perfThresholdPath]$ThresholdPath"

# ADE verification: reuse the immutable baseline or build and capture the single candidate.
#Requires -Version 7
param(
    [Parameter(Mandatory)][string]$RepositoryPath,
    [Parameter(Mandatory)][string]$ContextPath,
    [Parameter(Mandatory)][string]$ContextSha256
)
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\ProfilerHarness.psm1" -Force -DisableNameChecking
Assert-FileHash $ContextPath $ContextSha256
$context = Read-JsonFile $ContextPath
if ($context.schema -eq 'ui-profiler/trial/v1') {
    Assert-TrialContext $context
} elseif ($context.schema -eq 'ui-profiler/investigation/v2') {
    Assert-FileHash $context.contracts.path $context.contracts.sha256
    Assert-OriginalSource $context
}
Assert-FileHash $context.baseline.path $context.baseline.sha256
$baseline = Read-JsonFile $context.baseline.path
Assert-Measurement $baseline
Assert-FileHash $context.scenarioPath $context.scenarioSha256
Assert-FileHash $context.scenarioScript $context.scenarioScriptSha256
if ((Get-ProfilerHash $context.profilerCli) -ne $context.profilerSha256 -or
    $baseline.sourceSha256 -ne $context.originalSourceSha256 -or
    $baseline.scenarioSha256 -ne $context.scenarioSha256 -or
    $baseline.profilerSha256 -ne $context.profilerSha256) { throw 'Baseline inputs or profiler payload changed.' }
if ((Get-SourceHash $RepositoryPath) -eq $context.originalSourceSha256) {
    Write-Output 'Baseline verification reused its source-matched, hash-verified recording; no capture.'
    exit 0
}
if ($context.schema -eq 'ui-profiler/trial/v1') {
    Assert-ScopedCandidate $RepositoryPath $context.allowedPaths
} elseif ($context.schema -eq 'ui-profiler/investigation/v2') {
    Assert-MinimalCandidate $RepositoryPath $context.protectedPaths
} else { Assert-MinimalCandidate $RepositoryPath }
$candidate = Invoke-Measurement $context $RepositoryPath 'candidate'
$candidatePath = Join-Path $context.directory 'candidate\measurement.json'
$comparisonDirectory = Join-Path $context.directory 'multidimensional'
$comparison = Compare-Measurements $context.baseline.path $candidatePath $comparisonDirectory $context.profilerCli
if ($context.schema -in @('ui-profiler/investigation/v2', 'ui-profiler/trial/v1')) { Assert-OriginalSource $context }
Write-Output "Multidimensional comparison: $comparisonDirectory\comparison.md"
Write-Output ($comparison.counts | ConvertTo-Json -Compress)

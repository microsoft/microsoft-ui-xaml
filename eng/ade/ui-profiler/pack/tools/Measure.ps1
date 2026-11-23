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
Assert-MinimalCandidate $RepositoryPath
$candidate = Invoke-Measurement $context $RepositoryPath 'candidate'
$candidatePath = Join-Path $context.directory 'candidate\measurement.json'
$comparisonDirectory = Join-Path $context.directory 'multidimensional'
$comparison = Compare-Measurements $context.baseline.path $candidatePath $comparisonDirectory $context.profilerCli
Write-Output "Multidimensional comparison: $comparisonDirectory\comparison.md"
Write-Output ($comparison.counts | ConvertTo-Json -Compress)

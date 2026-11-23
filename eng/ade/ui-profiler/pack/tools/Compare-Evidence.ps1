# Reanalyze an existing pair without a build, app launch, agent request or capture.
#Requires -Version 7
param(
    [Parameter(Mandatory)][string]$BaselinePath,
    [Parameter(Mandatory)][string]$CandidatePath,
    [Parameter(Mandatory)][string]$ProfilerCli,
    [Parameter(Mandatory)][string]$Directory
)
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\ProfilerHarness.psm1" -Force -DisableNameChecking
$comparison = Compare-Measurements $BaselinePath $CandidatePath $Directory $ProfilerCli
Write-Output "Multidimensional comparison: $Directory\comparison.md"
Write-Output ($comparison.counts | ConvertTo-Json -Compress)

#Requires -Version 7
param(
    [Parameter(Mandatory)][string]$RepositoryPath,
    [Parameter(Mandatory)][string]$ContextPath,
    [Parameter(Mandatory)][string]$ContextSha256
)
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\ProfilerHarness.psm1" -Force -DisableNameChecking
Assert-FileHash $ContextPath $ContextSha256
Import-Module "$PSScriptRoot\NativeTrials.psm1" -Force -DisableNameChecking
Invoke-NativeTrialCheck -RepositoryPath $RepositoryPath -Context (Read-JsonFile $ContextPath)

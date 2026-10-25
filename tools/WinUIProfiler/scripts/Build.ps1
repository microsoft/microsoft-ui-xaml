param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [ValidateSet('x64', 'ARM64')][string]$Architecture = 'x64',
    [switch]$Restore
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'Build-App.ps1') -Solution -Configuration $Configuration -Architecture $Architecture -Restore:$Restore

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
<#
.SYNOPSIS
Runs the pinned package ensure script from an Azure Pipelines task.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("CheckFetch", "Push")]
    [string]$Mode,

    [Parameter(Mandatory = $true)]
    [string]$PackagesDirectory,

    [Parameter(Mandatory = $true)]
    [string]$SummaryPath,

    [switch]$SkipFeedPresenceCheck,

    [switch]$ResolveOnly,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$InternalFeedSource
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = "Stop"

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$scriptPath = Join-Path $PSScriptRoot "EnsureOssPinnedPackages.ps1"

if (-not (Test-Path -LiteralPath $scriptPath)) {
    throw "Pinned package script '$scriptPath' does not exist."
}

$arguments = @{
    Mode = $Mode
    RepositoryRoot = $repositoryRoot
    PackagesDirectory = $PackagesDirectory
    SummaryPath = $SummaryPath
    InternalFeedSource = $InternalFeedSource
}

if ($SkipFeedPresenceCheck) {
    $arguments.SkipFeedPresenceCheck = $true
}

if ($ResolveOnly) {
    $arguments.ResolveOnly = $true
}

Write-Host "Using pinned package script '$scriptPath'."
Write-Host "Using repository root '$repositoryRoot'."

& $scriptPath @arguments

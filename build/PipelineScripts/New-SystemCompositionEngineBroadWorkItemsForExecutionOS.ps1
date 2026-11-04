# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$TestBinaryPath,

    [Parameter(Mandatory)]
    [string]$OutputProjDirectory,

    [Parameter(Mandatory)]
    [string]$TaefExePath,

    [ValidatePattern('^[0-9]+$')]
    [string]$TargetOSVersion
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

if ([string]::IsNullOrWhiteSpace($TargetOSVersion)) {
    $currentVersionKey = 'HKLM:\Software\Microsoft\Windows NT\CurrentVersion'
    try {
        $currentVersion = Get-ItemProperty -LiteralPath $currentVersionKey -ErrorAction Stop
        $TargetOSVersion = [string]$currentVersion.CurrentBuildNumber
    }
    catch {
        throw (
            "Could not read the execution worker Windows build from " +
            "$currentVersionKey\CurrentBuildNumber. $($_.Exception.Message)")
    }
}

[uint32]$parsedTargetOSVersion = 0
if (-not [uint32]::TryParse($TargetOSVersion, [ref]$parsedTargetOSVersion) -or
    $parsedTargetOSVersion -eq 0) {
    throw (
        "The execution worker Windows build must be a nonzero number. " +
        "Received '$TargetOSVersion'.")
}
$TargetOSVersion = [string]$parsedTargetOSVersion

$generator = Join-Path $PSScriptRoot 'New-SystemCompositionEngineBroadWorkItem.ps1'
if (-not (Test-Path -LiteralPath $generator -PathType Leaf)) {
    throw "The broad work-item generator was not found at $generator."
}

$excludedTestsManifest = Join-Path `
    $PSScriptRoot `
    'SystemCompositionEngineExcludedTests-WPF.csv'
if (-not (Test-Path -LiteralPath $excludedTestsManifest -PathType Leaf)) {
    throw "The WPF excluded-test manifest was not found at $excludedTestsManifest."
}

$excludedTests = @(Import-Csv -LiteralPath $excludedTestsManifest)
$expectedExcludedTests = @(
    $excludedTests |
        ForEach-Object {
            "$([string]$_.WorkItem)`t$([string]$_.TestName)"
        } |
        Sort-Object
)

$workItemDefinitions = @(
    [pscustomobject]@{
        TestFilePattern = 'Test\Microsoft.UI.Xaml.Tests.External.*.dll'
        OutputFileName = 'RunTestsInHelix-SystemCompositionEngineBroad-WPF-External.proj'
    }
    [pscustomobject]@{
        TestFilePattern = 'Test\Microsoft.UI.Xaml.Tests.Managed.*.dll'
        OutputFileName = 'RunTestsInHelix-SystemCompositionEngineBroad-WPF-Managed.proj'
    }
)

Write-Host (
    "Generating broad WPF work items with exact known-failure exclusions on execution " +
    "worker OS build $TargetOSVersion.")

$outputProjFiles = @()
$generationResults = @()
foreach ($workItemDefinition in $workItemDefinitions) {
    $outputProjFile = Join-Path $OutputProjDirectory $workItemDefinition.OutputFileName
    $outputProjFiles += $outputProjFile
    $generationResults += @(& $generator `
        -TestFilePattern $workItemDefinition.TestFilePattern `
        -TestBinaryPath $TestBinaryPath `
        -OutputProjFile $outputProjFile `
        -TaefExePath $TaefExePath `
        -ExcludedTestsManifestPath $excludedTestsManifest `
        -TargetOSVersion $TargetOSVersion)
}

$generatedWorkItems = @(
    foreach ($outputProjFile in $outputProjFiles) {
        [xml]$plan = Get-Content -LiteralPath $outputProjFile -Raw
        $plan.SelectNodes('/Project/ItemGroup/HelixWorkItem') |
            ForEach-Object { [string]$_.Include }
    }
)
$duplicateWorkItems = @(
    $generatedWorkItems |
        Group-Object |
        Where-Object Count -ne 1 |
        ForEach-Object Name
)
if ($generatedWorkItems.Count -eq 0) {
    throw (
        "No WPF master-backed work items remain after applying " +
        "'$excludedTestsManifest'.")
}
if ($duplicateWorkItems.Count -ne 0) {
    throw (
        "Generated WPF work items contain duplicate names: " +
        "$($duplicateWorkItems -join ', ').")
}

$validatedExcludedTests = @(
    $generationResults |
        ForEach-Object { @($_.ValidatedExcludedTests) }
)
$appliedExcludedTests = @(
    $generationResults |
        ForEach-Object { @($_.MatchedExcludedTests) }
)
$missingExcludedTests = @(
    $expectedExcludedTests |
        Where-Object { $validatedExcludedTests -cnotcontains $_ }
)
$unexpectedExcludedTests = @(
    $validatedExcludedTests |
        Where-Object { $expectedExcludedTests -cnotcontains $_ }
)
$duplicateExcludedTests = @(
    $validatedExcludedTests |
        Group-Object |
        Where-Object Count -ne 1 |
        ForEach-Object Name
)
if ($missingExcludedTests.Count -ne 0 -or
    $unexpectedExcludedTests.Count -ne 0 -or
    $duplicateExcludedTests.Count -ne 0) {
    throw (
        "Generated WPF work items do not match the excluded-test manifest. " +
        "Missing=[$($missingExcludedTests -join ', ')] " +
        "Unexpected=[$($unexpectedExcludedTests -join ', ')] " +
        "Duplicates=[$($duplicateExcludedTests -join ', ')].")
}

$unexpectedAppliedExcludedTests = @(
    $appliedExcludedTests |
        Where-Object { $expectedExcludedTests -cnotcontains $_ }
)
$duplicateAppliedExcludedTests = @(
    $appliedExcludedTests |
        Group-Object |
        Where-Object Count -ne 1 |
        ForEach-Object Name
)
if ($unexpectedAppliedExcludedTests.Count -ne 0 -or
    $duplicateAppliedExcludedTests.Count -ne 0) {
    throw (
        "Generated WPF work items applied invalid excluded tests. " +
        "Unexpected=[$($unexpectedAppliedExcludedTests -join ', ')] " +
        "Duplicates=[$($duplicateAppliedExcludedTests -join ', ')].")
}

$inactiveExcludedTests = @(
    $expectedExcludedTests |
        Where-Object { $appliedExcludedTests -cnotcontains $_ }
)
Write-Host (
    "Generated $($workItemDefinitions.Count) WPF work-item project(s) with " +
    "$($generatedWorkItems.Count) work item(s) after validating exactly " +
    "$($validatedExcludedTests.Count) known-failure exclusion(s), applying " +
    "$($appliedExcludedTests.Count) on execution worker OS build $TargetOSVersion, and " +
    "leaving $($inactiveExcludedTests.Count) valid exclusion(s) inactive due to the " +
    "current selection metadata.")

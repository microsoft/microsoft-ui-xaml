# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet(
        'Test\Microsoft.UI.Xaml.Tests.External.*.dll',
        'Test\Microsoft.UI.Xaml.Tests.Managed.*.dll')]
    [string]$TestFilePattern,

    [Parameter(Mandatory)]
    [string]$TestBinaryPath,

    [Parameter(Mandatory)]
    [string]$OutputProjFile,

    [Parameter(Mandatory)]
    [string]$TaefExePath,

    [Parameter(Mandatory)]
    [string]$ExcludedTestsManifestPath,

    [Parameter(Mandatory)]
    [ValidatePattern('^[0-9]+$')]
    [string]$TargetOSVersion
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

try {
    [uint32]$parsedTargetOSVersion = 0
    if (-not [uint32]::TryParse($TargetOSVersion, [ref]$parsedTargetOSVersion) -or
        $parsedTargetOSVersion -eq 0) {
        throw "TargetOSVersion must be a nonzero Windows build number. Received '$TargetOSVersion'."
    }
    $TargetOSVersion = [string]$parsedTargetOSVersion

    foreach ($requiredPath in @($TestBinaryPath, $TaefExePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath)) {
            throw "Required path was not found: $requiredPath"
        }
    }
    if (-not (Test-Path -LiteralPath $ExcludedTestsManifestPath -PathType Leaf)) {
        throw "The excluded-test manifest was not found at $ExcludedTestsManifestPath."
    }

    $manifestLines = @(Get-Content -LiteralPath $ExcludedTestsManifestPath)
    if ($manifestLines.Count -eq 0) {
        throw "The excluded-test manifest '$ExcludedTestsManifestPath' has no header."
    }
    $manifestProbe = @(
        @($manifestLines[0], '"__workitem__","__test__"') |
            ConvertFrom-Csv
    )
    $manifestColumns = @($manifestProbe[0].PSObject.Properties.Name)
    foreach ($requiredColumn in @('WorkItem', 'TestName')) {
        if ($manifestColumns -notcontains $requiredColumn) {
            throw (
                "The excluded-test manifest '$ExcludedTestsManifestPath' is missing " +
                "the '$requiredColumn' column.")
        }
    }

    $excludedTests = @(Import-Csv -LiteralPath $ExcludedTestsManifestPath)
    if ($excludedTests.Count -eq 0) {
        throw (
            "The excluded-test manifest '$ExcludedTestsManifestPath' contains no " +
            "excluded tests.")
    }
    $invalidManifestRows = @(
        $excludedTests |
            Where-Object {
                [string]::IsNullOrWhiteSpace([string]$_.WorkItem) -or
                [string]::IsNullOrWhiteSpace([string]$_.TestName) -or
                ([string]$_.TestName).Contains("'")
            }
    )
    if ($invalidManifestRows.Count -ne 0) {
        throw (
            "The excluded-test manifest '$ExcludedTestsManifestPath' contains " +
            "$($invalidManifestRows.Count) invalid row or rows.")
    }

    $seenTestIdentities = [System.Collections.Generic.HashSet[string]]::new(
        [StringComparer]::Ordinal)
    $duplicateTests = @(
        foreach ($excludedTest in $excludedTests) {
            $identity = (
                "$([string]$excludedTest.WorkItem)`t" +
                [string]$excludedTest.TestName)
            if (-not $seenTestIdentities.Add($identity)) {
                $identity
            }
        }
    )
    if ($duplicateTests.Count -ne 0) {
        throw (
            "The excluded-test manifest '$ExcludedTestsManifestPath' contains duplicate " +
            "work-item/test identities: $($duplicateTests -join ', ').")
    }

    $excludedTestsByWorkItem = @{}
    foreach ($workItemGroup in ($excludedTests | Group-Object WorkItem)) {
        $excludedTestsByWorkItem[$workItemGroup.Name] = @(
            $workItemGroup.Group |
                ForEach-Object { [string]$_.TestName } |
                Sort-Object
        )
    }

    $repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
    $generator = Join-Path $repositoryRoot 'Helix\common\pipeline\GenerateHelixWorkItems.ps1'
    if (-not (Test-Path -LiteralPath $generator -PathType Leaf)) {
        throw "The standard work-item generator was not found at $generator."
    }

    $platformQuery = (
        "(@TestPass:IncludeOnlyOn='*Desktop*' OR " +
        "(NOT(@TestPass:IncludeOnlyOn=*) AND " +
        "NOT(@TestPass:ExcludeOn='*Desktop*')))")
    $hostingQuery = "(@Hosting:Mode='WPF' OR NOT(@Hosting:Mode='*'))"
    $osVersionQuery = (
        "(@TestPass:MinOSVer<=$TargetOSVersion OR NOT(@TestPass:MinOSVer=*)) AND " +
        "(@TestPass:MaxOSVer>=$TargetOSVersion OR NOT(@TestPass:MaxOSVer=*))")
    $catalogBaseQuery = (
        "$platformQuery AND @HasAssociatedMasterFile='True' AND $hostingQuery")
    $taefBaseQuery = "$catalogBaseQuery AND $osVersionQuery"
    $workItemPrefix = 'WPF'
    $taefExtraParameters = (
        "/p:CompositionEngineType=System " +
        "/p:CompositionEngineLaunchRequestPath=" +
        "%HELIX_CORRELATION_PAYLOAD%\Test\.winui-composition-engine\system " +
        "/reportLoadingIssue /p:HostingMode=WPF")

    Write-Host (
        "Selecting broad WPF system composition engine tests while excluding " +
        "known failures for " +
        "execution worker OS build $TargetOSVersion.")

    $catalogOutputProjFile = Join-Path `
        ([IO.Path]::GetTempPath()) `
        ("SystemCompositionEngineCatalog-{0}.proj" -f [guid]::NewGuid().ToString('N'))
    try {
        $catalogGenerationOutput = @(& $generator `
            -TestFilePattern $TestFilePattern `
            -TestBinaryDirectoryPath $TestBinaryPath `
            -OutputProjFile $catalogOutputProjFile `
            -TaefBaseQuery $catalogBaseQuery `
            -TaefExtraParameters $taefExtraParameters `
            -TestTimeout '00:30:00' `
            -TestNamePrefix $workItemPrefix `
            -WorkItemPrefix $workItemPrefix `
            -TestExecutionMultiplier 1 `
            -RunIgnoredTests $true `
            -TaefExePath $TaefExePath `
            -FailOnTaefDiscoveryError `
            -ExcludedTestsByWorkItem $excludedTestsByWorkItem)

        $generationOutput = @(& $generator `
            -TestFilePattern $TestFilePattern `
            -TestBinaryDirectoryPath $TestBinaryPath `
            -OutputProjFile $OutputProjFile `
            -TaefBaseQuery $taefBaseQuery `
            -TaefExtraParameters $taefExtraParameters `
            -TestTimeout '00:30:00' `
            -TestNamePrefix $workItemPrefix `
            -WorkItemPrefix $workItemPrefix `
            -TestExecutionMultiplier 1 `
            -RunIgnoredTests $false `
            -TaefExePath $TaefExePath `
            -FailOnTaefDiscoveryError `
            -ExcludedTestsByWorkItem $excludedTestsByWorkItem `
            -AllowMissingExcludedTests)
    }
    finally {
        Remove-Item `
            -LiteralPath $catalogOutputProjFile `
            -Force `
            -ErrorAction SilentlyContinue
    }

    $catalogGenerationResults = @(
        $catalogGenerationOutput |
            Where-Object {
                $null -ne $_ -and
                $_.PSObject.Properties.Name -contains 'MatchedExcludedTests'
            }
    )
    if ($catalogGenerationResults.Count -ne 1) {
        throw (
            "The standard work-item generator returned " +
            "$($catalogGenerationResults.Count) catalog exclusion result object(s); " +
            "expected exactly one.")
    }
    $catalogGenerationResult = $catalogGenerationResults[0]

    $generationResults = @(
        $generationOutput |
            Where-Object {
                $null -ne $_ -and
                $_.PSObject.Properties.Name -contains 'MatchedExcludedTests'
            }
    )
    if ($generationResults.Count -ne 1) {
        throw (
            "The standard work-item generator returned $($generationResults.Count) " +
            "exclusion result object(s); expected exactly one.")
    }
    $generationResult = $generationResults[0]

    $unexpectedAppliedExclusions = @(
        @($generationResult.MatchedExcludedTests) |
            Where-Object {
                @($catalogGenerationResult.MatchedExcludedTests) -cnotcontains $_
            }
    )
    if ($unexpectedAppliedExclusions.Count -ne 0) {
        throw (
            "OS-eligible exclusions were not present in the validated broad catalog: " +
            "$($unexpectedAppliedExclusions -join ', ').")
    }

    if (-not (Test-Path -LiteralPath $OutputProjFile -PathType Leaf)) {
        throw "The work-item generator did not create $OutputProjFile."
    }

    [xml]$plan = Get-Content -LiteralPath $OutputProjFile -Raw
    $workItems = @($plan.SelectNodes('/Project/ItemGroup/HelixWorkItem'))
    $oversizedCommands = @(
        $workItems |
            Where-Object { ([string]$_.Command).Length -ge 8000 } |
            ForEach-Object {
                "$([string]$_.Include)=$(([string]$_.Command).Length)"
            }
    )
    if ($oversizedCommands.Count -ne 0) {
        throw (
            "Generated work-item commands may exceed the cmd.exe limit: " +
            "$($oversizedCommands -join ', ').")
    }

    $invalidCommands = @(
        $workItems |
            Where-Object {
                $command = [string]$_.Command
                $command -notlike "*@HasAssociatedMasterFile='True'*" -or
                $command -notlike "*@TestPass:MinOSVer<=$TargetOSVersion*" -or
                $command -notlike "*@TestPass:MaxOSVer>=$TargetOSVersion*" -or
                $command -notlike '*/p:CompositionEngineType=System*' -or
                $command -notlike (
                    '*/p:CompositionEngineLaunchRequestPath=' +
                    '%HELIX_CORRELATION_PAYLOAD%' +
                    '\Test\.winui-composition-engine\system*') -or
                $command -notlike '*/p:HostingMode=WPF*'
            } |
            ForEach-Object { [string]$_.Include }
    )
    if ($invalidCommands.Count -ne 0) {
        throw (
            "Generated work items are missing required system composition engine selection " +
            "or host parameters: $($invalidCommands -join ', ').")
    }

    Write-Host (
        "Generated $($workItems.Count) WPF work item(s) for OS build " +
        "$TargetOSVersion after validating " +
        "$(@($catalogGenerationResult.MatchedExcludedTests).Count) catalog exclusion(s), " +
        "applying $(@($generationResult.MatchedExcludedTests).Count) OS-eligible " +
        "exclusion(s), and skipping " +
        "$($generationResult.SkippedWorkItemCount) empty work item(s) in $OutputProjFile.")

    [pscustomobject]@{
        ValidatedExcludedTests = @($catalogGenerationResult.MatchedExcludedTests)
        MatchedExcludedTests = @($generationResult.MatchedExcludedTests)
        GeneratedWorkItems = @(
            $workItems |
                ForEach-Object { [string]$_.Include }
        )
        SkippedWorkItemCount = [int]$generationResult.SkippedWorkItemCount
    }
}
catch {
    throw
}

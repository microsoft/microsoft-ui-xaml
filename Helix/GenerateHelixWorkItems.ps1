[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)] 
    [string]$TestFilePattern,

    [Parameter(Mandatory = $true)] 
    [string]$TestBinaryPath,

    [Parameter(Mandatory = $true)] 
    [string]$OutputProjFile,

    [Parameter(Mandatory = $true)] 
    [ValidateSet("DevTestSuite", "ScenarioTestSuite")]
    [string]$JobTestSuiteName,

    [string]$CustomTaefQuery,

    [ValidateSet("System", "")]
    [string]$CompositionEngineType = "",

    [ValidateSet("UAP", "WPF", "Win32Explicit", "")]
    [string]$HostingMode,

    [ValidateSet("18362", "17763", "17134", "19042", "19045", "22000", "22621", "22631", "26200")]
    [string]$TargetOSVersion = "18362",

    [int]$TestExecutionMultiplier = 1,

    [bool]$RunIgnoredTests = $false,

    [bool]$IsValidateWindowsAppSDKRun = $false,

    [string]$TaefExePath,

    [string]$ExcludedTestsManifestPath
)

$testTimeout = "00:30:00"

$TestClassification = "Integration"
if($JobTestSuiteName -eq "ScenarioTestSuite")
{
    $TestClassification = "ScenarioTestSuite"
}

$taefPlatformQuery = "(@TestPass:IncludeOnlyOn='*Desktop*' OR (NOT(@TestPass:IncludeOnlyOn=*) AND NOT(@TestPass:ExcludeOn='*Desktop*')))"
if ($CompositionEngineType -eq "System" -and $HostingMode -ne "WPF")
{
    throw "CompositionEngineType=System is supported only with HostingMode=WPF."
}
if ($HostingMode)
{
    $taefHostModeQuery ="AND @Hosting:Mode='$HostingMode'"
}
$classificationQuery="AND @Classification='$TestClassification'"
$includeOnOsVerQuery="AND (@TestPass:MinOSVer<=$TargetOSVersion OR NOT @TestPass:MinOSVer=*) AND (@TestPass:MaxOSVer>=$TargetOSVersion OR NOT @TestPass:MaxOSVer=*)"

$taefBaseQuery = "$taefPlatformQuery $taefHostModeQuery $classificationQuery $includeOnOsVerQuery"
if($IsValidateWindowsAppSDKRun)
{
    $taefBaseQuery = "$taefBaseQuery AND NOT @IgnoreForValidateWindowsAppSDK='True'"
}


if($CustomTaefQuery)
{
    $taefBaseQuery = "$taefBaseQuery AND $CustomTaefQuery"
}

$WorkItemPrefix = ""
$testnameprefix = ""
if($HostingMode -eq "WPF")
{
    $WorkItemPrefix = "WPF"
    $testnameprefix = "WPF"
}
elseif($HostingMode -eq "Win32Explicit")
{
    $WorkItemPrefix = "Win32Explicit"
    $testnameprefix = "Win32Explicit"
}

$taefExtraParameters = ""
if ($CompositionEngineType -eq "System")
{
    $taefExtraParameters = (
        "/p:CompositionEngineType=System " +
        "/p:CompositionEngineLaunchRequestPath=" +
        "%HELIX_CORRELATION_PAYLOAD%\Test\.winui-composition-engine\system")
}

if ($HostingMode) {
    $taefExtraParameters =
        "$taefExtraParameters /p:HostingMode=$HostingMode".Trim()
}

$TestBinaryDirectoryPath = $TestBinaryPath

$excludedTestsByWorkItem = $null
$expectedExcludedTestIdentities = @()
if ($ExcludedTestsManifestPath)
{
    if (-not (Test-Path -LiteralPath $ExcludedTestsManifestPath -PathType Leaf))
    {
        throw "The excluded-test manifest was not found at $ExcludedTestsManifestPath."
    }

    $manifestLines = @(Get-Content -LiteralPath $ExcludedTestsManifestPath)
    if ($manifestLines.Count -eq 0)
    {
        throw "The excluded-test manifest '$ExcludedTestsManifestPath' has no header."
    }

    $manifestProbe = @(
        @($manifestLines[0], '"__workitem__","__test__","KnownFailure","0x00000000"') |
            ConvertFrom-Csv
    )
    $manifestColumns = @($manifestProbe[0].PSObject.Properties.Name)
    foreach ($requiredColumn in @(
        'WorkItem',
        'TestName',
        'FailureCategory',
        'FailureSignature'))
    {
        if ($manifestColumns -notcontains $requiredColumn)
        {
            throw (
                "The excluded-test manifest '$ExcludedTestsManifestPath' is missing " +
                "the '$requiredColumn' column.")
        }
    }

    $excludedTests = @(Import-Csv -LiteralPath $ExcludedTestsManifestPath)
    if ($excludedTests.Count -eq 0)
    {
        throw "The excluded-test manifest '$ExcludedTestsManifestPath' has no test rows."
    }

    $invalidManifestRows = @(
        $excludedTests |
            Where-Object {
                [string]::IsNullOrWhiteSpace([string]$_.WorkItem) -or
                [string]::IsNullOrWhiteSpace([string]$_.TestName) -or
                [string]::IsNullOrWhiteSpace([string]$_.FailureCategory) -or
                [string]::IsNullOrWhiteSpace([string]$_.FailureSignature) -or
                ([string]$_.TestName).Contains("'") -or
                ([string]$_.FailureCategory) -notmatch '^[A-Za-z][A-Za-z0-9]*$'
            }
    )
    if ($invalidManifestRows.Count -ne 0)
    {
        throw (
            "The excluded-test manifest '$ExcludedTestsManifestPath' contains " +
            "$($invalidManifestRows.Count) invalid row or rows.")
    }

    $seenTestIdentities = [System.Collections.Generic.HashSet[string]]::new(
        [StringComparer]::Ordinal)
    $duplicateTests = @(
        foreach ($excludedTest in $excludedTests)
        {
            $identity = (
                "$([string]$excludedTest.WorkItem)`t" +
                [string]$excludedTest.TestName)
            if (-not $seenTestIdentities.Add($identity))
            {
                $identity
            }
        }
    )
    if ($duplicateTests.Count -ne 0)
    {
        throw (
            "The excluded-test manifest '$ExcludedTestsManifestPath' contains duplicate " +
            "work-item/test identities: $($duplicateTests -join ', ').")
    }

    $expectedExcludedTestIdentities = @(
        $seenTestIdentities |
            Sort-Object
    )
    $excludedTestsByWorkItem = @{}
    foreach ($workItemGroup in ($excludedTests | Group-Object WorkItem))
    {
        $excludedTestsByWorkItem[$workItemGroup.Name] = @(
            $workItemGroup.Group |
                ForEach-Object { [string]$_.TestName } |
                Sort-Object
        )
    }
}

$generatorParameters = @{
    TestFilePattern = $TestFilePattern
    TestBinaryDirectoryPath = $TestBinaryDirectoryPath
    OutputProjFile = $OutputProjFile
    TestExecutionMultiplier = $TestExecutionMultiplier
    RunIgnoredTests = $RunIgnoredTests
    WorkItemPrefix = $WorkItemPrefix
    TaefBaseQuery = $taefBaseQuery
    TestTimeout = $testTimeout
    TaefExtraParameters = $taefExtraParameters
    TestNamePrefix = $testnameprefix
    TaefExePath = $TaefExePath
}
if ($null -ne $excludedTestsByWorkItem)
{
    $generatorParameters.ExcludedTestsByWorkItem = $excludedTestsByWorkItem
}

$generationOutput = @(
    & $PSScriptRoot\common\pipeline\GenerateHelixWorkItems.ps1 @generatorParameters
)

if ($null -ne $excludedTestsByWorkItem)
{
    $generationResults = @(
        $generationOutput |
            Where-Object {
                $null -ne $_ -and
                $_.PSObject.Properties.Name -contains 'MatchedExcludedTests'
            }
    )
    if ($generationResults.Count -ne 1)
    {
        throw (
            "The standard work-item generator returned $($generationResults.Count) " +
            "exclusion result object(s); expected exactly one.")
    }

    $generationResult = $generationResults[0]
    $matchedExcludedTestIdentities = @(
        @($generationResult.MatchedExcludedTests) |
            Sort-Object
    )
    $unmatchedExclusions = @(
        $expectedExcludedTestIdentities |
            Where-Object { $matchedExcludedTestIdentities -cnotcontains $_ }
    )
    $unexpectedExclusions = @(
        $matchedExcludedTestIdentities |
            Where-Object { $expectedExcludedTestIdentities -cnotcontains $_ }
    )
    if ($unmatchedExclusions.Count -ne 0 -or $unexpectedExclusions.Count -ne 0)
    {
        throw (
            "Excluded-test discovery did not exactly match the manifest. " +
            "Unmatched: $($unmatchedExclusions -join ', '). " +
            "Unexpected: $($unexpectedExclusions -join ', ').")
    }

    Write-Host (
        "Matched all $($matchedExcludedTestIdentities.Count) excluded tests from " +
        "'$ExcludedTestsManifestPath'. Generated " +
        "$($generationResult.GeneratedWorkItemCount) work item(s) and skipped " +
        "$($generationResult.SkippedWorkItemCount) fully excluded work item(s).")
    $generationResult
}
else
{
    $generationOutput
}
Param(
    [int]$MinimumExpectedTestsExecutedCount = 1,

    [string]$AccessToken = $env:SYSTEM_ACCESSTOKEN,
    [string]$HelixAccessToken,
    [string]$CollectionUri = $env:SYSTEM_COLLECTIONURI,
    [string]$TeamProject = $env:SYSTEM_TEAMPROJECT,
    [string]$BuildUri = $env:BUILD_BUILDURI,
    
    [string]$HelixTypeJobFilter # e.g. "DevTestSuite", "ScenarioTestSuite", "pgo/x86", "pgo/x64"
)

Write-Host "MinimumExpectedTestsExecutedCount: $MinimumExpectedTestsExecutedCount"
Write-Host "CollectionUri:                     $CollectionUri"
Write-Host "TeamProject:                       $TeamProject"
Write-Host "BuildUri:                          $BuildUri"
Write-Host "HelixTypeJobFilter:                $HelixTypeJobFilter"

if ((!$HelixAccessToken) -and ($env:HelixAccessToken))
{
    $HelixAccessToken = $env:HelixAccessToken
}

if($env:taefquery)
{
    # If we are running a custom taef query, skip the min test count check
    $MinimumExpectedTestsExecutedCount = 1
}

$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$AccessToken")))"
}

. "$PSScriptRoot/AzurePipelinesHelperScripts.ps1"

Write-Host "Checking test results..."

$queryUri = GetQueryTestRunsUri -CollectionUri $CollectionUri -TeamProject $TeamProject -BuildUri $BuildUri -IncludeRunDetails
Write-Host "queryUri = $queryUri"

$testRuns = Invoke-RestMethod -Uri $queryUri -Method Get -Headers $azureDevOpsRestApiHeaders
[System.Collections.Generic.List[string]]$failingTests = @()
[System.Collections.Generic.List[string]]$unreliableTests = @()
[System.Collections.Generic.List[string]]$unexpectedResultTest = @()
[System.Collections.Generic.List[string]]$skippedTests = @()

[System.Collections.Generic.List[string]]$processedTestRunIds = @()
$totalTestsExecutedCount = 0

# We only process the last testRun with a given name and jobtype (based on completedDate).
# This is to handle the case where a failing Pipeline Job was re-run.
# The name of a testRun is set to the Helix queue that it was run on (e.g. windows.10.amd64.client19h1.xaml)
# If we have multiple test runs on the same queue they must have different Helix JobTypes

foreach ($testRun in ($testRuns.value | Sort-Object -Property "completedDate" -Descending))
{
    $jobType = Get-HelixJobTypeFromTestRun $testRun $HelixAccessToken
    if($HelixTypeJobFilter)
    {
        if(!($jobType -like "$HelixTypeJobFilter*"))
        {
            Write-Host "Skipping test run '$($testRun.name)' since jobType '$jobType' does not match HelixTypeJobFilter '$HelixTypeJobFilter'"
            continue
        }
    }

    $testRunId = "$($testRun.name)-$jobType"
    if ($processedTestRunIds -contains $testRunId)
    {
        Write-Host "Skipping test run '$($testRun.name)' with jobType '$jobType', since we have already processed a test run of that name and jobType."
        continue
    }
    Write-Host "Processing results from test run '$($testRun.name)' with jobType '$jobType'"
    $processedTestRunIds.Add($testRunId)

    $totalTestsExecutedCount += $testRun.totalTests

    # Azure DevOps Rest API returns at most 1000 results at a time. We process the results in pages of 1000.
    $totalTests = $testrun.totalTests
    $sizeOfPage = 1000
    $pagesOfTests = [int][Math]::Ceiling($totalTests / $sizeOfPage)

    For ($page=0; $page -lt $pagesOfTests; $page++) 
    {
        $numItemsToSkip = $page * $sizeOfPage

        Write-Host "Retrieving test results..."

        $testRunResultsUri = "$($testRun.url)/results?`$top=$sizeOfPage&`$skip=$numItemsToSkip&api-version=5.1"
        $testResults = Invoke-RestMethod -Uri $testRunResultsUri -Method Get -Headers $azureDevOpsRestApiHeaders

        foreach ($testResult in $testResults.value)
        {
            $shortTestCaseTitle = $testResult.testCaseTitle -replace "release.[a-zA-Z0-9]+.Windows.UI.Xaml.Tests.MUXControls.",""

            if ($testResult.outcome -eq "Failed")
            {
                if (-not $failingTests.Contains($shortTestCaseTitle))
                {
                    $failingTests.Add($shortTestCaseTitle)
                }
            }
            elseif ($testResult.outcome -eq "Warning")
            {
                if (-not $unreliableTests.Contains($shortTestCaseTitle))
                {
                    $unreliableTests.Add($shortTestCaseTitle)
                }
            }
            elseif ($testResult.outcome -eq "NotExecuted")
            {
                if (-not $skippedTests.Contains($shortTestCaseTitle))
                {
                    $skippedTests.Add($shortTestCaseTitle)
                }
            }
            elseif ($testResult.outcome -ne "Passed")
            {
                # We should only see tests with result "Passed", "Failed", "NotExecuted" or "Warning"
                if (-not $unexpectedResultTest.Contains($shortTestCaseTitle))
                {
                    $unexpectedResultTest.Add($shortTestCaseTitle)
                }
            }
        }
    }
}

# Sort the list of tests to ensure that they're consistently ordered before outputting them.
$unreliableTests = $unreliableTests | Sort-Object
$failingTests = $failingTests | Sort-Object
$unexpectedResultTest = $unexpectedResultTest | Sort-Object
$skippedTests = $skippedTests | Sort-Object

if($skippedTests.Count -gt 0)
{
    Write-Host @"
Skipped tests:
$($skippedTests -join "$([Environment]::NewLine)")

"@
}

if ($unreliableTests.Count -gt 0)
{
    Write-Host @"
##vso[task.logissue type=warning;]Unreliable tests:
##vso[task.logissue type=warning;]$($unreliableTests -join "$([Environment]::NewLine)##vso[task.logissue type=warning;]")

"@
}

if ($failingTests.Count -gt 0)
{
    Write-Host @"
##vso[task.logissue type=error;]Failing tests:
##vso[task.logissue type=error;]$($failingTests -join "$([Environment]::NewLine)##vso[task.logissue type=error;]")

"@
}

if ($unexpectedResultTest.Count -gt 0)
{
    Write-Host @"
##vso[task.logissue type=error;]Tests with unexpected results:
##vso[task.logissue type=error;]$($unexpectedResultTest -join "$([Environment]::NewLine)##vso[task.logissue type=error;]")
"@
}

if($totalTestsExecutedCount -lt $MinimumExpectedTestsExecutedCount)
{
    Write-Error "Expected at least $MinimumExpectedTestsExecutedCount tests to be executed."
    Write-Error "Actual executed test count is: $totalTestsExecutedCount"
    Write-Host @"
##vso[task.logissue type=error;]Expected at least $MinimumExpectedTestsExecutedCount tests to be executed.
##vso[task.logissue type=error;]Actual executed test count is: $totalTestsExecutedCount
"@
    Write-Host "##vso[task.complete result=Failed;]"
}
elseif ($failingTests.Count -gt 0)
{
    Write-Host "At least one test failed."
    Write-Host "##vso[task.complete result=Failed;]"
}
elseif ($unexpectedResultTest.Count -gt 0)
{
    Write-Host "At least one test with an unexpected result was found."
    Write-Host "##vso[task.complete result=Failed;]"
}
elseif ($unreliableTests.Count -gt 0)
{
    Write-Host "All tests eventually passed, but some initially failed."
    Write-Host "##vso[task.complete result=Succeeded;]"
}
else
{
    Write-Host "All tests passed."
    Write-Host "##vso[task.complete result=Succeeded;]"
}

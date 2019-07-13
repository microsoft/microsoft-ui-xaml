Param(
    [Parameter(Mandatory = $true)] 
    [int]$MinimumExpectedTestsExecutedCount,

    [string]$AccessToken = $env:SYSTEM_ACCESSTOKEN,
    [string]$CollectionUri = $env:SYSTEM_COLLECTIONURI,
    [string]$TeamProject = $env:SYSTEM_TEAMPROJECT,
    [string]$BuildUri = $env:BUILD_BUILDURI,
    [int]$JobAttempt = $env:SYSTEM_JOBATTEMPT
)

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

$timesSeenByRunName = @{}
$totalTestsExecutedCount = 0

foreach ($testRun in ($testRuns.value | Sort-Object -Property "completedDate"))
{
    if (-not $timesSeenByRunName.ContainsKey($testRun.name))
    {
        $timesSeenByRunName[$testRun.name] = 0
    }
    
    $timesSeen = $timesSeenByRunName[$testRun.name] + 1
    $timesSeenByRunName[$testRun.name] = $timesSeen

    # The same build can have multiple test runs associated with it if the build owner opted to re-run a test run.
    # We should only pay attention to the current attempt version.
    if ($timesSeen -ne $JobAttempt)
    {
        continue
    }

    $totalTestsExecutedCount += $testRun.totalTests

    $testRunResultsUri = "$($testRun.url)/results?api-version=5.0"
    $testResults = Invoke-RestMethod -Uri "$($testRun.url)/results?api-version=5.0" -Method Get -Headers $azureDevOpsRestApiHeaders
        
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
    }
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

if($totalTestsExecutedCount -lt $MinimumExpectedTestsExecutedCount)
{
    Write-Host "Expected at least $MinimumExpectedTestsExecutedCount tests to be executed."
    Write-Host "Actual executed test count is: $totalTestsExecutedCount"
    Write-Host "##vso[task.complete result=Failed;]"
}
elseif ($failingTests.Count -gt 0)
{
    Write-Host "At least one test failed."
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

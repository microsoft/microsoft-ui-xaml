Param(
    # Access tokens can be created at https://dev.azure.com/ms/_usersSettings/tokens, then passed into here.
    # The access token must contain at least the permissions "Build (Read)" and "Test Management (Read)".
    # DO NOT SHARE YOUR ACCESS TOKEN WITH ANYONE! They allow Azure DevOps APIs to perform actions on your behalf.
    [string]$AccessToken = $env:SYSTEM_ACCESSTOKEN,

    [string]$CollectionUri = "https://dev.azure.com/ms/",
    [string]$TeamProject = "microsoft-ui-xaml",
    [string]$BranchName = "refs/heads/main",
    [int]$BuildDefinitionId = 20,
    [int]$DaysPrior = 14,
    [string]$OutputCsvFile = "UnreliableTestReport.csv"
)

$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$AccessToken")))"
}

if (-not $CollectionUri.EndsWith("/"))
{
    $CollectionUri += "/"
}

# "o" returns the DateTime formatted as ISO 8601, which is the standard representation that the Azure DevOps REST API uses.
$minTime = (Get-Date).Subtract([TimeSpan]::FromDays($DaysPrior)).ToUniversalTime().GetDateTimeFormats("o")

$buildQueryUri = $CollectionUri + $TeamProject + "/_apis/build/Builds?branchName=$BranchName&minTime=$minTime&definitions=$BuildDefinitionId&statusFilter=Completed&deletedFilter=excludeDeleted&api-version=5.0"

Write-Host "Querying $buildQueryUri for builds..." -NoNewline
$builds = Invoke-RestMethod -Uri $buildQueryUri -Method Get -Headers $azureDevOpsRestApiHeaders
Write-Host " Done."
Write-Host "Gathering test runs from builds..." -NoNewline

[System.Collections.ArrayList]$testRunsToReportOn = @()

for ($i = 0; $i -lt $builds.count; $i++)
{
    $build = $builds.value[$i]

    Write-Progress -Activity "$($build.buildNumber)" -PercentComplete (100 * $i / $builds.count)

    $testRunQueryUri = $CollectionUri + $TeamProject + "/_apis/test/runs?buildUri=$($build.uri)&api-version=5.0"
    $testRuns = Invoke-RestMethod $testRunQueryUri -Method Get -Headers $azureDevOpsRestApiHeaders

    foreach ($testRun in $testRuns.value)
    {
        # Ignore any test runs that aren't complete.
        if ($testRun.state -ne "Completed")
        {
            continue
        }

        $testRun | Add-Member -NotePropertyName buildName -NotePropertyValue $build.buildNumber
        $testRun | Add-Member -NotePropertyName buildId -NotePropertyValue $build.id
        $testRunsToReportOn.Add($testRun) | Out-Null
    }
}

Write-Host " Done."
Write-Host ""

class TestHistory {
    [string]$TestName
    [int]$TotalRuns
    [int]$TotalPassedRuns
    [int]$TotalFailedRuns
    [System.Collections.Generic.List[string]]$BuildUrlsWithFailedRuns = [System.Collections.Generic.List[string]]::new()
}

$testHistoryByName = @{}

for ($i = 0; $i -lt $testRunsToReportOn.Count; $i++)
{
    $testRun = $testRunsToReportOn[$i]
    Write-Progress -Activity "$($testRun.buildName) ($($testRun.name))" -PercentComplete (100 * $i / $testRunsToReportOn.Count)
    Write-Host "Processing run $($testRun.buildName) ($($testRun.name))..." -NoNewline

    $testResults = Invoke-RestMethod -Uri "$($testRun.url)/results?api-version=5.0" -Method Get -Headers $azureDevOpsRestApiHeaders
        
    foreach ($testResult in $testResults.value)
    {
        $shortTestCaseTitle = $testResult.testCaseTitle -replace "release.[a-zA-Z0-9]+\.",""
        $shortTestCaseTitle = $shortTestCaseTitle -replace "Windows\.UI\.Xaml\.Tests\.MUXControls\.",""
        $shortTestCaseTitle = $shortTestCaseTitle -replace "MUXControls\.",""

        if ($testHistoryByName.ContainsKey($shortTestCaseTitle))
        {
            $testHistory = $testHistoryByName[$shortTestCaseTitle]
        }
        else
        {
            $testHistory = [TestHistory]::new()
            $testHistory.TestName = $shortTestCaseTitle
            $testHistoryByName[$shortTestCaseTitle] = $testHistory
        }

        if ($testResult.outcome -ne "Passed")
        {
            # A test that didn't pass may have sub-results, so we'll retrieve those for consideration.
            # Sometimes this call can fail due to random network stuff, so we'll retry up to five times before giving up.
            $retryCount = 5

            while ($retryCount -gt 0)
            {
                try
                {
                    $testResultsWithSubResults = Invoke-RestMethod -Uri "$($testRun.url)/results/$($testResult.id)?detailsToInclude=SubResults&api-version=5.0" -Method Get -Headers $azureDevOpsRestApiHeaders
                    break
                }
                catch
                {
                    $retryCount--
                }
            }

            # If we were unsuccessful at retrieving sub-result information, then we'll just ignore this run.
            if ($retryCount -eq 0)
            {
                continue
            }

            $shouldLogBuild = $false

            # If this test has sub-results, we'll take those into account.
            # Otherwise, we'll just mark this as a single failed test.
            if ($testResultsWithSubResults.subResults.Count -gt 0)
            {
                foreach ($testSubResult in $testResultsWithSubResults.subResults)
                {
                    $testHistory.TotalRuns++

                    if ($testSubResult.outcome -eq "Failed")
                    {
                        $testHistory.TotalFailedRuns++
                        $shouldLogBuild = $true
                    }
                }
            }
            else
            {
                $testHistory.TotalRuns++
                $testHistory.TotalFailedRuns++
                $shouldLogBuild = $true
            }
        }
        else
        {
            $testHistory.TotalRuns++
            $shouldLogBuild = $false
        }

        if ($shouldLogBuild)
        {
            $buildUrl = $CollectionUri + $TeamProject + "/_build/results?buildId=$($testRun.buildId)&view=ms.vss-test-web.build-test-results-tab"

            if (-not $testHistory.BuildUrlsWithFailedRuns.Contains($buildUrl))
            {
                $testHistory.BuildUrlsWithFailedRuns.Add($buildUrl)
            }
        }
    }

    Write-Host " Done."
}

Write-Host ""
Write-Host "Sorting tests to put the least-passing on top..." -NoNewline

# PowerShell doesn't let you specify a sorting function; instead, what it allows you to do is define implicit properties to sort on.
# In this case, we're sorting on the number of failed runs as a percentage of the total runs, then on the test names.
# We negate the value to sort in descending order, rather than ascending.  We could use the -Descending flag, but that would cause us
# to also sort the test names in descending order, which we don't want.
$testHistoryList = $testHistoryByName.Values | Sort-Object @{ e={ -$_.TotalFailedRuns / $_.TotalRuns } },"TestName"
Write-Host " Done."

Write-Host "Writing report to $OutputCsvFile..." -NoNewline
Out-File -FilePath $OutputCsvFile -Encoding utf8 -InputObject "Test name,Failure rate,Build URLs with failed runs"
$significantDigitsToRoundTo = 2

foreach ($testHistory in $testHistoryList)
{
    # If a test has no failures in the time span, we'll exclude it from our report -
    # tests with a 100% pass rate need not be paid attention to here.
    if ($testHistory.TotalFailedRuns -eq 0)
    {
        continue
    }

    $failedRunsString = "$([Math]::Round(100 * $testHistory.TotalFailedRuns/$testHistory.TotalRuns, $significantDigitsToRoundTo))% ($($testHistory.TotalFailedRuns)/$($testHistory.TotalRuns))"

    Out-File -FilePath $OutputCsvFile -Encoding utf8 -InputObject "$($testHistory.TestName),$failedRunsString," -Append

    foreach ($buildUri in $testHistory.BuildUrlsWithFailedRuns)
    {
        Out-File -FilePath $OutputCsvFile -Encoding utf8 -InputObject ",,$buildUri" -Append
    }
}

Write-Host " Done."
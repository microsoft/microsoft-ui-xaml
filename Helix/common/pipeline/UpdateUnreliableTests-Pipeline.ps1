[CmdLetBinding()]
Param(
    [int]$RerunPassesRequiredToAvoidFailure = 5,

    [string]$AccessToken = $env:SYSTEM_ACCESSTOKEN,
    [string]$CollectionUri = $env:SYSTEM_COLLECTIONURI,
    [string]$TeamProject = $env:SYSTEM_TEAMPROJECT,
    [string]$BuildUri = $env:BUILD_BUILDURI,

    [string]$TestRunTitle,
    [string]$SubResultsDirPath,

    # Don't actually update results in AzDO. Useful for testing the script locally.
    [switch]$ReadOnlyTestMode
)


Write-Host "RerunPassesRequiredToAvoidFailure: $RerunPassesRequiredToAvoidFailure"
Write-Host "CollectionUri:                     $CollectionUri"
Write-Host "TeamProject:                       $TeamProject"
Write-Host "BuildUri:                          $BuildUri"
Write-Host "TestRunTitle:                      $TestRunTitle"
Write-Host "SubResultsDirPath:                 $SubResultsDirPath"
Write-Host "ReadOnlyTestMode:                  $ReadOnlyTestMode"

. "$PSScriptRoot/AzurePipelinesHelperScripts.ps1"


$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$AccessToken")))"
}

$queryUri = GetQueryTestRunsUri -CollectionUri $CollectionUri -TeamProject $TeamProject -BuildUri $BuildUri -IncludeRunDetails
Write-Host "queryUri = $queryUri"

# To account for unreliable tests, we'll iterate through all of the tests associated with this build, check to see any tests that were unreliable
# (denoted by being marked as "skipped"), and if so, we'll instead mark those tests with a warning and enumerate all of the attempted runs
# with their pass/fail states as well as any relevant error messages for failed attempts.
$testRuns = Invoke-RestMethod -Uri $queryUri -Method Get -Headers $azureDevOpsRestApiHeaders

$testRunsToProcess = @($testRuns.value | where {$_.name -eq $TestRunTitle} | Sort-Object -Property "completedDate" -Descending)

if($testRunsToProcess.Count -eq 0)
{
    Write-Host "No matching test run found with title '$TestRunTitle'. This is expected when no tests were executed (e.g. all tests filtered out by MinOSVer)."
    return 0
}

#Only process the most recent test run to handle cases where the Job got re-run:
$testRun = $testRunsToProcess[0]

if(!$ReadOnlyTestMode)
{
    Write-Host "Marking test run `"$($testRun.name)`" as in progress so we can change its results to account for unreliable tests."
    Invoke-RestMethod -Uri "$($testRun.url)?api-version=5.0" -Method Patch -Body (ConvertTo-Json @{ "state" = "InProgress" }) -Headers $azureDevOpsRestApiHeaders -ContentType "application/json" | Out-Null
}

# Azure DevOps Rest API returns at most 1000 results at a time. We process the results in pages of 1000.
$totalTests = $testrun.totalTests
$sizeOfPage = 1000
$pagesOfTests = [int][Math]::Ceiling($totalTests / $sizeOfPage)

$totalTestsSeen = 0
For ($page=0; $page -lt $pagesOfTests; $page++) 
{
    $numItemsToSkip = $page * $sizeOfPage

    Write-Host "Retrieving test results..."
    $testRunResultsUri = "$($testRun.url)/results?`$top=$sizeOfPage&`$skip=$numItemsToSkip&api-version=5.1"
    $testResults = Invoke-RestMethod -Uri $testRunResultsUri -Method Get -Headers $azureDevOpsRestApiHeaders

    $totalTestsSeen += $testResults.count
    
    foreach ($testResult in $testResults.value)
    {
        $testNeedsSubResultProcessing = $false
        if ($testResult.outcome -eq "NotExecuted")
        {
            $testNeedsSubResultProcessing = $testResult.errorMessage -like "*.json"
        }
        elseif($testResult.outcome -eq "Failed")
        {
            $testNeedsSubResultProcessing = $testResult.errorMessage -like "*.json"
        }

        if ($testNeedsSubResultProcessing)
        {
            Write-Host "  Test $($testResult.testCaseTitle) was detected as unreliable. Updating..."

            $testCaseTitle = $testResult.testCaseTitle

            if(!($testCaseTitle.Contains(".") -or $testCaseTitle.Contains(":")))
            {
                Write-Error "TestCaseTitle ($testCaseTitle) is expected to contain '.' or '::'"
                exit 1
            }

            $subresultsFileName = $testResult.errorMessage
            $subResultFilePath = Join-Path $SubResultsDirPath $subresultsFileName
            $subResultsJson = Get-Content $subResultFilePath

            $rerunResults = ConvertFrom-Json $subResultsJson
            

            [System.Collections.Generic.List[System.Collections.Hashtable]]$rerunDataList = @()
            $attemptCount = 0
            $passCount = 0
            $totalDuration = 0
            
            foreach ($rerun in $rerunResults.results)
            {
                $rerunData = @{
                    "displayName" = "Attempt #$($attemptCount + 1) - $($testResult.testCaseTitle)";
                    "durationInMs" = $rerun.duration;
                    "outcome" = $rerun.outcome;
                }

                if ($rerun.outcome -eq "Passed")
                {
                    $passCount++
                }
                
                if ($attemptCount -gt 0)
                {
                    $rerunData["sequenceId"] = $attemptCount
                }

                Write-Host "    Attempt #$($attemptCount + 1): $($rerun.outcome)"
                
                if ($rerun.outcome -ne "Passed")
                {
                    $fullErrorMessage = "$($rerunResults.errors[$rerun.errorIndex - 1])"

                    $rerunData["errorMessage"] = $fullErrorMessage
                }
                
                $attemptCount++
                $totalDuration += $rerun.duration
                $rerunDataList.Add($rerunData)
            }

            $overallOutcome = "Warning"
            
            if ($attemptCount -eq 2)
            {
                Write-Host "  Test $($testResult.testCaseTitle) passed on the immediate rerun, so we'll mark it as unreliable."
            }
            elseif ($passCount -ge $RerunPassesRequiredToAvoidFailure)
            {
                Write-Host "  Test $($testResult.testCaseTitle) passed on $passCount of $attemptCount attempts, which is greater than or equal to the $RerunPassesRequiredToAvoidFailure passes required to avoid being marked as failed. Marking as unreliable."
            }
            else
            {
                Write-Host "  Test $($testResult.testCaseTitle) passed on only $passCount of $attemptCount attempts, which is less than the $RerunPassesRequiredToAvoidFailure passes required to avoid being marked as failed. Marking as failed."
                $overallOutcome = "Failed"
            }
            
            $updateBody = ConvertTo-Json @(@{ "id" = $testResult.id; "outcome" = $overallOutcome; "errorMessage" = "$subresultsFileName"; "durationInMs" = $totalDuration; "subResults" = $rerunDataList; "resultGroupType" = "rerun" }) -Depth 5
            if(!$ReadOnlyTestMode)
            {
                Invoke-RestMethod -Uri $testRunResultsUri -Method Patch -Headers $azureDevOpsRestApiHeaders -Body $updateBody -ContentType "application/json" | Out-Null
            }
        }
        else
        {
            # Tests that pass on retry are initially reported as Ignored/NotExecuted. We update the result to Warning to make things a bit clearer.
            if ($testResult.outcome -eq "NotExecuted")
            {
                $overallOutcome = "Warning"
                $updateBody = ConvertTo-Json @(@{ "id" = $testResult.id; "outcome" = $overallOutcome; "errorMessage" = $testResult.errorMessage; }) -Depth 5
                if(!$ReadOnlyTestMode)
                {
                    Invoke-RestMethod -Uri $testRunResultsUri -Method Patch -Headers $azureDevOpsRestApiHeaders -Body $updateBody -ContentType "application/json" | Out-Null
                }
            }
        }
        
    }
}

Write-Host "Processed $totalTestsSeen / $totalTests tests"
if($totalTestsSeen -ne $totalTests)
{
    Write-Error "Expected to recieve $totalTests tests."
    exit 1
}

    
Write-Host "Finished updates. Re-marking test run `"$($testRun.name)`" as completed."
if(!$ReadOnlyTestMode)
{
    Invoke-RestMethod -Uri "$($testRun.url)?api-version=5.0" -Method Patch -Body (ConvertTo-Json @{ "state" = "Completed" }) -Headers $azureDevOpsRestApiHeaders -ContentType "application/json" | Out-Null
}

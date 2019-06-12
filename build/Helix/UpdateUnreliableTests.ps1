[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)] 
    [int]$RerunPassesRequiredToAvoidFailure
)

. "$PSScriptRoot/AzurePipelinesHelperScripts.ps1"


$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$($env:SYSTEM_ACCESSTOKEN)")))"
}

$queryUri = GetQueryTestRunsUri
Write-Host "queryUri = $queryUri"

# To account for unreliable tests, we'll iterate through all of the tests associated with this build, check to see any tests that were unreliable
# (denoted by being marked as "skipped"), and if so, we'll instead mark those tests with a warning and enumerate all of the attempted runs
# with their pass/fail states as well as any relevant error messages for failed attempts.
$testRuns = Invoke-RestMethod -Uri $queryUri -Method Get -Headers $azureDevOpsRestApiHeaders
      
foreach ($testRun in $testRuns.value)
{
    $testRunResultsUri = "$($testRun.url)/results?api-version=5.0"
        
    Write-Host "Marking test run `"$($testRun.name)`" as in progress so we can change its results to account for unreliable tests."
    Invoke-RestMethod -Uri "$($testRun.url)?api-version=5.0" -Method Patch -Body (ConvertTo-Json @{ "state" = "InProgress" }) -Headers $azureDevOpsRestApiHeaders -ContentType "application/json" | Out-Null

    Write-Host "Retrieving test results..."
    $testResults = Invoke-RestMethod -Uri $testRunResultsUri -Method Get -Headers $azureDevOpsRestApiHeaders
        
    foreach ($testResult in $testResults.value)
    {
        if ($testResult.outcome -eq "NotExecuted")
        {
            Write-Host "  Test $($testResult.testCaseTitle) was detected as unreliable. Updating..."
            
            $rerunResults = ConvertFrom-Json $testResult.errorMessage
            [System.Collections.Generic.List[System.Collections.Hashtable]]$rerunDataList = @()
            $attemptCount = 0
            $passCount = 0
            $totalDuration = 0
            
            foreach ($rerun in $rerunResults.results)
            {
                $rerunData = @{
                    "displayName" = "Attempt #$($attemptCount) - $($testResult.testCaseTitle)";
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
                    $screenshots = "$($rerunResults.blobPrefix)/$($rerun.screenshots -join @"
$($rerunResults.blobSuffix)
$($rerunResults.blobPrefix)
"@)$($rerunResults.blobSuffix)"

                    # We subtract 1 from the error index because we added 1 so we could use 0
                    # as a default value not injected into the JSON in order to keep its size down.
                    # We did this because there's a maximum size enforced for the errorMessage parameter
                    # in the Azure DevOps REST API.
                    $fullErrorMessage = @"
Log: $($rerunResults.blobPrefix)/$($rerun.log)$($rerunResults.blobSuffix)

Screenshots:
$screenshots

Error log:
$($rerunResults.errors[$rerun.errorIndex - 1])
"@

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
            elseif ($passCount -gt $RerunPassesRequiredToAvoidFailure)
            {
                Write-Host "  Test $($testResult.testCaseTitle) passed on $passCount of $attemptCount attempts, which is greater than or equal to the $RerunPassesRequiredToAvoidFailure passes required to avoid being marked as failed. Marking as unreliable."
            }
            else
            {
                Write-Host "  Test $($testResult.testCaseTitle) passed on only $passCount of $attemptCount attempts, which is less than the $RerunPassesRequiredToAvoidFailure passes required to avoid being marked as failed. Marking as failed."
                $overallOutcome = "Failed"
            }
            
            $updateBody = ConvertTo-Json @(@{ "id" = $testResult.id; "outcome" = $overallOutcome; "errorMessage" = " "; "durationInMs" = $totalDuration; "subResults" = $rerunDataList; "resultGroupType" = "rerun" }) -Depth 5
            Invoke-RestMethod -Uri $testRunResultsUri -Method Patch -Headers $azureDevOpsRestApiHeaders -Body $updateBody -ContentType "application/json" | Out-Null
        }
    }
        
    Write-Host "Finished updates. Re-marking test run `"$($testRun.name)`" as completed."
    Invoke-RestMethod -Uri "$($testRun.url)?api-version=5.0" -Method Patch -Body (ConvertTo-Json @{ "state" = "Completed" }) -Headers $azureDevOpsRestApiHeaders -ContentType "application/json" | Out-Null
}

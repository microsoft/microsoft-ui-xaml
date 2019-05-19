Write-Host "SYSTEM_ACCESSTOKEN: $($env:SYSTEM_ACCESSTOKEN)"
      
$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$($env:SYSTEM_ACCESSTOKEN)")))"
}

# To account for unreliable tests, we'll iterate through all of the tests associated with this build, check to see any tests that were unreliable
# (denoted by being marked as "skipped"), and if so, we'll instead mark those tests with a warning and enumerate all of the attempted runs
# with their pass/fail states as well as any relevant error messages for failed attempts.
$testRuns = Invoke-RestMethod -Uri "https://dev.azure.com/ms/microsoft-ui-xaml/_apis/test/runs?buildUri=$($env:BUILD_BUILDURI)" -Method Get -Headers $azureDevOpsRestApiHeaders
      
foreach ($testRun in $testRuns.value)
{
    $testRunResultsUri = "$($testRun.url)/results?api-version=5.0"
        
    Write-Host "Marking test run `"$($testRun.name)`" as in progress so we can change its results to account for unreliable tests."
    Invoke-RestMethod -Uri "$($testRun.url)?api-version=5.0" -Method Patch -Body (ConvertTo-Json @{ "state" = "InProgress" }) -Headers $azureDevOpsRestApiHeaders -ContentType "application/json"

    Write-Host "Retrieving test results..."
    $testResults = Invoke-RestMethod -Uri "$($testRun.url)/results?api-version=5.0" -Method Get -Headers $azureDevOpsRestApiHeaders
        
    foreach ($testResult in $testResults.value)
    {
        if ($testResult.outcome -eq "NotExecuted")
        {
            Write-Host "  Test $($testResult.testCaseTitle) was detected as unreliable. Updating..."
            
            $rerunResults = ConvertFrom-Json $testResult.errorMessage
            [System.Collections.Generic.List[System.Collections.Hashtable]]$rerunDataList = @()
            $attemptCount = 1
            $totalDuration = 0
            
            foreach ($rerun in $rerunResults.results)
            {
                $rerunData = @{
                    "displayName" = "Attempt #$($attemptCount) - $($testResult.testCaseTitle)";
                    "durationInMs" = $rerun.duration;
                    "outcome" = $rerun.outcome;
                }
                
                if ($attemptCount -gt 1)
                {
                    # -1 to make this zero-indexed.
                    $rerunData["sequenceId"] = $attemptCount - 1
                }

                Write-Host "    Attempt #$($attemptCount): $($rerun.outcome)"
                
                if ($rerun.outcome -ne "Passed")
                {
                    $screenshots = "$($rerunResults.blobPrefix)$($rerun.screenshots -join @"
$($rerunResults.blobSuffix)
$($rerunResults.blobPrefix)
"@)$($rerunResults.blobSuffix)"

                    # We subtract 1 from the error index because we added 1 so we could use 0
                    # as a default value not injected into the JSON, in order to keep its size down.
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
                
                $totalDuration += $rerun.duration
                $rerunDataList.Add($rerunData)
            }
            
            $updateBody = ConvertTo-Json @(@{ "id" = $testResult.id; "outcome" = "Warning"; "durationInMs" = $totalDuration; "subResults" = $rerunDataList; "resultGroupType" = "rerun" }) -Depth 5
            Invoke-RestMethod -Uri $testRunResultsUri -Method Patch -Headers $azureDevOpsRestApiHeaders -Body $updateBody -ContentType "application/json"
        }
    }
        
    Write-Host "Finished updates. Re-marking test run `"$($testRun.name)`" as completed."
    Invoke-RestMethod -Uri "$($testRun.url)?api-version=5.0" -Method Patch -Body (ConvertTo-Json @{ "state" = "Completed" }) -Headers $azureDevOpsRestApiHeaders -ContentType "application/json"
}

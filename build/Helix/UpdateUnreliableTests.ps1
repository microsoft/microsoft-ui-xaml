Write-Host "SYSTEM_ACCESSTOKEN: $($env:SYSTEM_ACCESSTOKEN)"
      
$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$($env:SYSTEM_ACCESSTOKEN)")))"
}
      
Write-Host "Invoke-RestMethod -Uri `"https://dev.azure.com/ms/microsoft-ui-xaml/_apis/test/runs?buildUri=$($env:BUILD_BUILDURI)`" -Method Get -Headers $azureDevOpsRestApiHeaders"
Invoke-RestMethod -Uri "https://dev.azure.com/ms/microsoft-ui-xaml/_apis/test/runs?buildUri=$($env:BUILD_BUILDURI)" -Method Get -Headers $azureDevOpsRestApiHeaders
$testRuns = Invoke-RestMethod -Uri "https://dev.azure.com/ms/microsoft-ui-xaml/_apis/test/runs?buildUri=$($env:BUILD_BUILDURI)" -Method Get -Headers $azureDevOpsRestApiHeaders
      
foreach ($testRun in $testRuns.value)
{
    $testRunResultsUri = "$($testRun.url)/results?api-version=5.0"
        
    Write-Host "Marking test as in progress, so we can change its values if need be."
    Write-Host "Invoke-RestMethod -Uri `"$($testRun.url)?api-version=5.0`" -Method Patch -Body (ConvertTo-Json @{ `"state`" = `"InProgress`" }) -Headers $azureDevOpsRestApiHeaders -ContentType `"application/json`""
    Invoke-RestMethod -Uri "$($testRun.url)?api-version=5.0" -Method Patch -Body (ConvertTo-Json @{ "state" = "InProgress" }) -Headers $azureDevOpsRestApiHeaders -ContentType "application/json"
        
    Write-Host "Invoke-RestMethod -Uri `"$($testRun.url)/results?api-version=5.0`" -Method Get -Headers $azureDevOpsRestApiHeaders"
    Invoke-RestMethod -Uri $testRunResultsUri -Method Get -Headers $azureDevOpsRestApiHeaders
    $testResults = Invoke-RestMethod -Uri "$($testRun.url)/results?api-version=5.0" -Method Get -Headers $azureDevOpsRestApiHeaders
        
    foreach ($testResult in $testResults.value)
    {
        Write-Host "Test $($testResult.testCaseTitle) : $($testResult.outcome)"
          
        if ($testResult.outcome -eq "NotExecuted")
        {
            Write-Host "(Actually passed on re-run. Updating.)"
            
            $rerunResults = ConvertFrom-Json $testResult.errorMessage
            [System.Collections.Generic.List[System.Collections.Generic.Dictionary[string, string]]]$rerunDataList = @()
            $attemptCount = 0
            $totalDuration = 0
            
            foreach ($rerun in $rerunResults.results)
            {
                $rerunData = @{
                    "displayName" = "Attempt #$attemptCount - $($testResult.testCaseTitle)";
                    "durationInMs" = $rerun.duration;
                    "outcome" = $rerun.outcome;
                }
                
                if ($attemptCount -gt 0)
                {
                    $rerunData["sequenceId"] = $attemptCount
                }
                
                if ($rerun.outcome -eq "Passed")
                {
                    $screenshots = "$($rerunResults.blobPrefix)$($rerun.screenshots -join @"
$($rerunResults.blobSuffix)
$($rerunResults.blobPrefix)
"@)$($rerunResults.blobSuffix)"

                    $fullErrorMessage = @"
Log: $($rerunResults.blobPrefix)/$($rerun.log)$($rerunResults.blobSuffix)

Screenshots:
$screenshots

Error log:
$($rerunResults.errors[$rerun.errorIndex])
"@

                    $rerunData["errorMessage"] = $fullErrorMessage
                }
                
                $attemptCount++
                $totalDuration += $rerun.duration
                
                $rerunDataList.Add($rerunData)
            }
            
            $updateBody = ConvertTo-Json @(@{ "id" = $testResult.id; "outcome" = "Warning"; durationInMs = $totalDuration; "subResults" = $rerunDataList; "resultGroupType" = "rerun" })
            Write-Host "Invoke-RestMethod -Uri $testRunResultsUri -Method Patch -Headers $azureDevOpsRestApiHeaders -Body $updateBody -ContentType `"application/json`""
            Write-Host "`$updateBody:"
            $updateBody
            Invoke-RestMethod -Uri $testRunResultsUri -Method Patch -Headers $azureDevOpsRestApiHeaders -Body $updateBody -ContentType "application/json"
        }
    }
        
    Write-Host "Marking test as completed again."
    Write-Host "Invoke-RestMethod -Uri `"$($testRun.url)?api-version=5.0`" -Method Patch -Body (ConvertTo-Json @{ `"state`" = `"Completed`" }) -Headers $azureDevOpsRestApiHeaders -ContentType `"application/json`""
    Invoke-RestMethod -Uri "$($testRun.url)?api-version=5.0" -Method Patch -Body (ConvertTo-Json @{ "state" = "Completed" }) -Headers $azureDevOpsRestApiHeaders -ContentType "application/json"
}

Write-Host "SYSTEM_ACCESSTOKEN: $($env:SYSTEM_ACCESSTOKEN)"
      
$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$($env:SYSTEM_ACCESSTOKEN)")))"
}

$testRuns = Invoke-RestMethod -Uri "https://dev.azure.com/ms/microsoft-ui-xaml/_apis/test/runs?buildUri=$($env:BUILD_BUILDURI)" -Method Get -Headers $azureDevOpsRestApiHeaders
[System.Collections.Generic.List[string]]$failingTests = @()
[System.Collections.Generic.List[string]]$unreliableTests = @()
      
foreach ($testRun in $testRuns.value)
{
    $testRunResultsUri = "$($testRun.url)/results?api-version=5.0"
    $testResults = Invoke-RestMethod -Uri "$($testRun.url)/results?api-version=5.0" -Method Get -Headers $azureDevOpsRestApiHeaders
        
    foreach ($testResult in $testResults.value)
    {
        if ($testResult.outcome -eq "Failed")
        {
            $failingTests.Add($testResult.testCaseTitle)
        }
        elseif ($testResult.outcome -eq "NotExecuted")
        {
            $unreliableTests.Add($testResult.testCaseTitle)
        }
    }
}

if ($failingTests.Count -gt 0)
{
    Write-Error @"
##vso[task.logissue type=error;]Failing tests:

$($failingTests -join [Environment]::NewLine)
"@
}

if ($unreliableTests.Count -gt 0)
{
    Write-Warning @"
##vso[task.logissue type=warning;]Unreliable tests:

$($unreliableTests -join [Environment]::NewLine)
"@
}
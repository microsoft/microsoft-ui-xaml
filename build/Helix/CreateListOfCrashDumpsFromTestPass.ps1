Param(
    [string]$AccessToken = $env:SYSTEM_ACCESSTOKEN,
    [string]$CollectionUri = $env:SYSTEM_COLLECTIONURI,
    [string]$TeamProject = $env:SYSTEM_TEAMPROJECT,
    [string]$BuildUri = $env:BUILD_BUILDURI,
    [string]$OutputFilePath = "dumplinks.html"
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

[System.Collections.Generic.List[string]]$workItems = @()

Out-File -FilePath $outputFilePath -InputObject "<h1>Dumps</h1>"

foreach ($testRun in $testRuns.value)
{
    $testRunResultsUri = "$($testRun.url)/results?api-version=5.0"
    $testResults = Invoke-RestMethod -Uri "$($testRun.url)/results?api-version=5.0" -Method Get -Headers $azureDevOpsRestApiHeaders

    Out-File -FilePath $outputFilePath -Append -InputObject "<h2>$($testRun.name)</h2>"
        
    foreach ($testResult in $testResults.value)
    {
        $info = ConvertFrom-Json $testResult.comment
        $helixJobId = $info.HelixJobId
        $helixWorkItemName = $info.HelixWorkItemName

        $workItem = "$helixJobId-$helixWorkItemName"

        if (-not $workItems.Contains($workItem))
        {
            $workItems.Add($workItem)

            Out-File -FilePath $outputFilePath -Append -InputObject "<h3>$helixWorkItemName</h3>"

            $filesQueryUri = "https://helix.dot.net/api/2019-06-17/jobs/$helixJobId/workitems/$helixWorkItemName/files"

            $files = Invoke-RestMethod -Uri $filesQueryUri -Method Get

            $dumps = $files | where { $_.Name.EndsWith(".dmp") }

            if($dumps.Count -gt 0)
            {
                Out-File -FilePath $outputFilePath -Append -InputObject "<ul>"
                foreach($file in $dumps)
                {
                    Out-File -FilePath $outputFilePath -Append -InputObject "<li><a href=$($file.Link)>$($file.Name)</a></li>"
                }
                Out-File -FilePath $outputFilePath -Append -InputObject "</ul>"
            }
        }        
    }
}
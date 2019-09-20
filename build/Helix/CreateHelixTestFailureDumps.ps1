Param(
    [string]$AccessToken = $env:SYSTEM_ACCESSTOKEN,
    [string]$CollectionUri = $env:SYSTEM_COLLECTIONURI,
    [string]$TeamProject = $env:SYSTEM_TEAMPROJECT,
    [string]$BuildUri = $env:BUILD_BUILDURI,
    [string]$OutputFilePath = "dumplinks.html"
)

function Generate-File-Links
{
    Param ([Array[]]$files,[string]$headerName)
    if($files.Count -gt 0)
    {
        Out-File -FilePath $outputFilePath -Append -InputObject "<h4>$headerName</h4>"
        Out-File -FilePath $outputFilePath -Append -InputObject "<ul>"
        foreach($file in $files)
        {
            Out-File -FilePath $outputFilePath -Append -InputObject "<li><a href=$($file.Link)>$($file.Name)</a></li>"
        }
        Out-File -FilePath $outputFilePath -Append -InputObject "</ul>"
    }
}

#Write empty string to create the file
Out-File -FilePath $outputFilePath -Append -InputObject ""

$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$AccessToken")))"
}

. "$PSScriptRoot/AzurePipelinesHelperScripts.ps1"

$queryUri = GetQueryTestRunsUri -CollectionUri $CollectionUri -TeamProject $TeamProject -BuildUri $BuildUri -IncludeRunDetails
Write-Host "queryUri = $queryUri"

$testRuns = Invoke-RestMethod -Uri $queryUri -Method Get -Headers $azureDevOpsRestApiHeaders

[System.Collections.Generic.List[string]]$workItems = @()

foreach ($testRun in $testRuns.value)
{
    $testRunResultsUri = "$($testRun.url)/results?api-version=5.0"
    $testResults = Invoke-RestMethod -Uri "$($testRun.url)/results?api-version=5.0" -Method Get -Headers $azureDevOpsRestApiHeaders
    $isTestRunNameShown = $false
        
    foreach ($testResult in $testResults.value)
    {
        $info = ConvertFrom-Json $testResult.comment
        $helixJobId = $info.HelixJobId
        $helixWorkItemName = $info.HelixWorkItemName

        $workItem = "$helixJobId-$helixWorkItemName"

        if (-not $workItems.Contains($workItem))
        {
            $workItems.Add($workItem)
            $filesQueryUri = "https://helix.dot.net/api/2019-06-17/jobs/$helixJobId/workitems/$helixWorkItemName/files"
            $files = Invoke-RestMethod -Uri $filesQueryUri -Method Get

            $screenShots = $files | where { $_.Name.EndsWith(".jpg") }
            $dumps = $files | where { $_.Name.EndsWith(".dmp") }
            $visualTreeMasters = $files | where { $_.Name.EndsWith(".xml") -And (-Not $_.Name.Contains('testResults')) }
            if($screenShots.Count+$dumps.Count+$visualTreeMasters.Count -gt 0)
            {
                if(-Not $isTestRunNameShown)
                {
                    Out-File -FilePath $outputFilePath -Append -InputObject "<h2>$($testRun.name)</h2>"
                    $isTestRunNameShown = $true
                }
                Out-File -FilePath $outputFilePath -Append -InputObject "<h3>$helixWorkItemName</h3>"
                Generate-File-Links $screenShots "Screenshots"
                Generate-File-Links $dumps "Crash Dumps"
                Generate-File-Links $visualTreeMasters "Visual Tree Masters"
            }
        }        
    }
}
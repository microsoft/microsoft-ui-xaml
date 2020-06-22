Param(
    [string]$AccessToken = $env:SYSTEM_ACCESSTOKEN,
    [string]$CollectionUri = $env:SYSTEM_COLLECTIONURI,
    [string]$TeamProject = $env:SYSTEM_TEAMPROJECT,
    [string]$BuildUri = $env:BUILD_BUILDURI,
    [string]$OutputFolder = "HelixOutput"
)

$helixLinkFile = "$OutputFolder\LinksToHelixTestFiles.html"
$visualTreeMasterFolder = "$OutputFolder\VisualTreeMasters"

function Generate-File-Links
{
    Param ([Array[]]$files,[string]$sectionName)
    if($files.Count -gt 0)
    {
        Out-File -FilePath $helixLinkFile -Append -InputObject "<div class=$sectionName>"
        Out-File -FilePath $helixLinkFile -Append -InputObject "<h4>$sectionName</h4>"
        Out-File -FilePath $helixLinkFile -Append -InputObject "<ul>"
        foreach($file in $files)
        {
            Out-File -FilePath $helixLinkFile -Append -InputObject "<li><a href=$($file.Link)>$($file.Name)</a></li>"
        }
        Out-File -FilePath $helixLinkFile -Append -InputObject "</ul>"
        Out-File -FilePath $helixLinkFile -Append -InputObject "</div>"
    }
}

#Create output directory
New-Item $OutputFolder -ItemType Directory

$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$AccessToken")))"
}

. "$PSScriptRoot/AzurePipelinesHelperScripts.ps1"

$queryUri = GetQueryTestRunsUri -CollectionUri $CollectionUri -TeamProject $TeamProject -BuildUri $BuildUri -IncludeRunDetails
Write-Host "queryUri = $queryUri"

$testRuns = Invoke-RestMethod -Uri $queryUri -Method Get -Headers $azureDevOpsRestApiHeaders
$webClient = New-Object System.Net.WebClient
[System.Collections.Generic.List[string]]$workItems = @()

foreach ($testRun in $testRuns.value)
{
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
            $pgcFiles = $files | where { $_.Name.EndsWith(".pgc") }
            if ($screenShots.Count + $dumps.Count + $visualTreeMasters.Count + $pgcFiles.Count -gt 0)
            {
                if(-Not $isTestRunNameShown)
                {
                    Out-File -FilePath $helixLinkFile -Append -InputObject "<h2>$($testRun.name)</h2>"
                    $isTestRunNameShown = $true
                }
                Out-File -FilePath $helixLinkFile -Append -InputObject "<h3>$helixWorkItemName</h3>"
                Generate-File-Links $screenShots "Screenshots"
                Generate-File-Links $dumps "CrashDumps"
                Generate-File-Links $visualTreeMasters "VisualTreeMasters"
                Generate-File-Links $pgcFiles "PGC files"
                $misc = $files | where { ($screenShots -NotContains $_) -And ($dumps -NotContains $_) -And ($visualTreeMasters -NotContains $_) -And ($pgcFiles -NotContains $_) }
                Generate-File-Links $misc "Misc"

                if( -Not (Test-Path $visualTreeMasterFolder) )
                {
                    New-Item $visualTreeMasterFolder -ItemType Directory
                }
                foreach($masterFile in $visualTreeMasters)
                {
                    $destination = "$visualTreeMasterFolder\$($masterFile.Name)"
                    Write-Host "Copying $($masterFile.Name) to $destination"
                    $webClient.DownloadFile($masterFile.Link, $destination)
                }

                foreach($pgcFile in $pgcFiles)
                {
                    $flavorPath = $pgcFile.Name.Split('.')[0]
                    $archPath = $pgcFile.Name.Split('.')[1]
                    $fileName = $pgcFile.Name.Remove(0, $flavorPath.length + $archPath.length + 2)
                    $fullPath = "$OutputFolder\PGO\$flavorPath\$archPath"
                    $destination = "$fullPath\$fileName"

                    Write-Host "Copying $($pgcFile.Name) to $destination"

                    if (-Not (Test-Path $fullPath))
                    {
                        New-Item $fullPath -ItemType Directory
                    }

                    $webClient.DownloadFile($pgcFile.Link, $destination)
                }
            }
        }
    }
}

if(Test-Path $visualTreeMasterFolder)
{
    Write-Host "Merge duplicated master files..."
    $masterFiles = Get-ChildItem $visualTreeMasterFolder
    $prefixList = @()
    foreach($file in $masterFiles)
    {
        $prefix = $file.BaseName.Split('-')[0]
        if($prefixList -NotContains $prefix)
        {
            $prefixList += $prefix
        }
    }

    foreach($prefix in $prefixList)
    {
        $filesToDelete = @()
        $versionedMasters = $masterFiles | Where { $_.BaseName.StartsWith("$prefix-") } | Sort-Object -Property Name -Descending
        if($versionedMasters.Count > 1)
        {
            for ($i=0; $i -lt $versionedMasters.Length-1; $i++)
            {
                $v1 = Get-Content $versionedMasters[$i].FullName
                $v2 = Get-Content $versionedMasters[$i+1].FullName
                $diff = Compare-Object $v1 $v2
                if($diff.Length -eq 0)
                {
                    $filesToDelete += $versionedMasters[$i]
                }
            }
            $filesToDelete | ForEach-Object {
                Write-Host "Deleting $($_.Name)"
                Remove-Item $_.FullName
            }
        }

        Write-Host "Renaming $($versionedMasters[-1].Name) to $prefix.xml"
        Move-Item $versionedMasters[-1].FullName "$visualTreeMasterFolder\$prefix.xml" -Force
    }
}

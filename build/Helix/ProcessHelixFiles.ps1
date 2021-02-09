Param(
    [string]$AccessToken = $env:SYSTEM_ACCESSTOKEN,
    [string]$HelixAccessToken = $env:HelixAccessToken,
    [string]$CollectionUri = $env:SYSTEM_COLLECTIONURI,
    [string]$TeamProject = $env:SYSTEM_TEAMPROJECT,
    [string]$BuildUri = $env:BUILD_BUILDURI,
    [string]$OutputFolder = "HelixOutput"
)

Write-Host "CollectionUri:      $CollectionUri"
Write-Host "TeamProject:        $TeamProject"
Write-Host "BuildUri:           $BuildUri"
Write-Host "OutputFolder:       $OutputFolder"

$helixLinkFile = "$OutputFolder\LinksToHelixTestFiles.html"
$visualTreeVerificationFolder = "$OutputFolder\UpdatedVisualTreeVerificationFiles"

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
            $url = Append-HelixAccessTokenToUrl $file.Link "{Your-Helix-Access-Token-Here}"
            Out-File -FilePath $helixLinkFile -Append -InputObject "<li>$($url)</li>"
        }
        Out-File -FilePath $helixLinkFile -Append -InputObject "</ul>"
        Out-File -FilePath $helixLinkFile -Append -InputObject "</div>"
    }
}

function Log-Error
{
    Param ([string]$message)

    # We want to log the error slightly differently depending if we are running in AzDO or not.
    if($env:TF_BUILD)
    {
        Write-Host "##vso[task.logissue type=error;]$message"
    }
    else
    {
        Write-Error "$message" -ErrorAction Continue
    }
}

function Append-HelixAccessTokenToUrl
{
    Param ([string]$url, [string]$token)
    if($token)
    {
        if($url.Contains("?"))
        {
            $url = "$($url)&access_token=$($token)"
        }
        else
        {
            $url = "$($url)?access_token=$($token)"
        }
    }
    return $url
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
            $filesQueryUri = "https://helix.dot.net/api/2019-06-17/jobs/$helixJobId/workitems/$helixWorkItemName/files?access_token=$HelixAccessToken"
            $files = Invoke-RestMethod -Uri $filesQueryUri -Method Get

            $screenShots = $files | where { $_.Name.EndsWith(".jpg") }
            $dumps = $files | where { $_.Name.EndsWith(".dmp") }
            $visualTreeVerificationFiles = $files | where { $_.Name.EndsWith(".xml") -And (-Not $_.Name.Contains('testResults')) }
            $pgcFiles = $files | where { $_.Name.EndsWith(".pgc") }
            if ($screenShots.Count + $dumps.Count + $visualTreeVerificationFiles.Count + $pgcFiles.Count -gt 0)
            {
                if(-Not $isTestRunNameShown)
                {
                    Out-File -FilePath $helixLinkFile -Append -InputObject "<h2>$($testRun.name)</h2>"
                    $isTestRunNameShown = $true
                }
                Out-File -FilePath $helixLinkFile -Append -InputObject "<h3>$helixWorkItemName</h3>"
                Generate-File-Links $screenShots "Screenshots"
                Generate-File-Links $dumps "CrashDumps"
                Generate-File-Links $visualTreeVerificationFiles "visualTreeVerificationFiles"
                Generate-File-Links $pgcFiles "PGC files"
                $misc = $files | where { ($screenShots -NotContains $_) -And ($dumps -NotContains $_) -And ($visualTreeVerificationFiles -NotContains $_) -And ($pgcFiles -NotContains $_) }
                Generate-File-Links $misc "Misc"

                if( -Not (Test-Path $visualTreeVerificationFolder) )
                {
                    New-Item $visualTreeVerificationFolder -ItemType Directory
                }
                foreach($verificationFile in $visualTreeVerificationFiles)
                {
                    $destination = "$visualTreeVerificationFolder\$($verificationFile.Name)"
                    $fileurl = Append-HelixAccessTokenToUrl $verificationFile.Link  $HelixAccessToken
                    try
                    {
                        $webClient.DownloadFile($fileurl, $destination)
                    }
                    catch
                    {
                        Log-Error "Failed to download $($verificationFile.Name) to $destination : $($_.Exception.Message) -- URL: $($verificationFile.Link)"
                    }
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

                    $fileurl = Append-HelixAccessTokenToUrl $pgcFile.Link $HelixAccessToken
                    $webClient.DownloadFile($fileurl, $destination)
                }
            }
        }
    }
}

if(Test-Path $visualTreeVerificationFolder)
{
    $verificationFiles = Get-ChildItem $visualTreeVerificationFolder
    $prefixList = @()

    foreach($file in $verificationFiles)
    {
        Write-Host "Copying $($file.Name) to $visualTreeVerificationFolder"
        Move-Item $file.FullName "$visualTreeVerificationFolder\$($file.Name)" -Force
    }
}

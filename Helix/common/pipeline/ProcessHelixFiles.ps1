Param(
    [string]$AccessToken = $env:SYSTEM_ACCESSTOKEN,
    [string]$HelixAccessToken,
    [string]$CollectionUri = $env:SYSTEM_COLLECTIONURI,
    [string]$TeamProject = $env:SYSTEM_TEAMPROJECT,
    [string]$BuildUri = $env:BUILD_BUILDURI,
    [string]$OutputFolder = "HelixOutput",
    [Parameter(Mandatory=$false)][Switch]$ProcessAllJobs,

    [string]$HelixTypeJobFilter # e.g. "DevTestSuite", "ScenarioTestSuite", "pgo/x86", "pgo/x64"
)

Write-Host "CollectionUri:      $CollectionUri"
Write-Host "TeamProject:        $TeamProject"
Write-Host "BuildUri:           $BuildUri"
Write-Host "OutputFolder:       $OutputFolder"
Write-Host "ProcessAllJobs:     $ProcessAllJobs"
Write-Host "HelixTypeJobFilter: $HelixTypeJobFilter"

if ((!$HelixAccessToken) -and ($env:HelixAccessToken))
{
    $HelixAccessToken = $env:HelixAccessToken
}

$ErrorActionPreference = "Stop"

$helixLinkFileName = "LinksToHelixCrashDumps.html"
if($HelixTypeJobFilter)
{
    $helixLinkFileName = "LinksToHelixCrashDumps-$($HelixTypeJobFilter.Replace("/", "_")).html"
}
$helixLinkFile = Join-Path $OutputFolder $helixLinkFileName

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
            #$url = Append-HelixAccessTokenToUrl $file.Link "{Your-Helix-Access-Token-Here}"
            Out-File -FilePath $helixLinkFile -Append -InputObject "<li>$($file.Link)</li>"
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

function Ensure-Path
{
    Param ([string]$path)
    if(!(Test-Path $path))
    {
        New-Item $path -ItemType Directory
    }
}

#Create output directory
Ensure-Path $OutputFolder

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

$isFileHeaderShown = $false
$fileHeader = @"
<h1>Info</h1>
<p>To access these files, you will need to get a personal access token from: <a href="https://helix.dot.net/">https://helix.dot.net/</a>.
<b>Warning</b>: Do not share this token with anyone.<br>
To obtain your token, log into the Helix site with your GitHub account, which should be connected to the Microsoft organization. <br>
The token can be found under your profile, which is linked at the top of the Helix page. Your profile should also list Microsoft under your GitHub Organizations.<br>
Copy your token into the URIs below, excluding the braces.</p>
"@

foreach ($testRun in $testRuns.value)
{
    $jobType = Get-HelixJobTypeFromTestRun $testRun $HelixAccessToken
    if($HelixTypeJobFilter)
    {
        if(!($jobType -like "$HelixTypeJobFilter*"))
        {
            Write-Host "Skipping test run '$($testRun.name)' since jobType '$jobType' does not match HelixTypeJobFilter '$HelixTypeJobFilter'"
            continue
        }
    }
    
    Write-Host "Processing results from test run '$($testRun.name)' with jobType '$jobType'"

    $isTestRunNameShown = $false

    # Azure DevOps Rest API returns at most 1000 results at a time. We process the results in pages of 1000.
    $totalTests = $testrun.totalTests
    $sizeOfPage = 1000
    $pagesOfTests = [int][Math]::Ceiling($totalTests / $sizeOfPage)

    For ($page=0; $page -lt $pagesOfTests; $page++)
    {
        $numItemsToSkip = $page * $sizeOfPage

        Write-Host "Retrieving test results..."
        $testRunResultsUri = "$($testRun.url)/results?`$top=$sizeOfPage&`$skip=$numItemsToSkip&api-version=5.1"
        $testResults = Invoke-RestMethod -Uri $testRunResultsUri -Method Get -Headers $azureDevOpsRestApiHeaders

        foreach ($testResult in $testResults.value)
        {
            if ($testResult.outcome -ne "Passed" -or $ProcessAllJobs)
            {
                $info = ConvertFrom-Json ([System.Web.HttpUtility]::HtmlDecode($testResult.comment))
                $helixJobId = $info.HelixJobId
                $helixWorkItemName = $info.HelixWorkItemName
                $workItem = "$helixJobId-$helixWorkItemName"


                if (-not $workItems.Contains($workItem))
                {
                    Write-Host "WorkItem: $workItem"
                    $workItems.Add($workItem)
                    $filesQueryUri = Append-HelixAccessTokenToUrl "https://helix.dot.net/api/2019-06-17/jobs/$helixJobId/workitems/$helixWorkItemName/files" $HelixAccessToken

                    $files = @() # Clear $files so we don't retain a stale value in case of an error
                    try
                    {
                        $files = Invoke-RestMethodWithRetries $filesQueryUri
                    }
                    catch
                    {
                        Log-Error "Failed to query for files for $helixWorkItemName - $($_.Exception.Message)"
                        continue
                    }

                    $testRunOutputDir = Join-Path $OutputFolder $testRun.name
                    Ensure-Path $testRunOutputDir
                    $workItemOutputDir = Join-Path $testRunOutputDir $helixWorkItemName
                    Ensure-Path $workItemOutputDir

                    foreach($file in $files)
                    {
                        # We don't upload dump files since they are too large.
                        # We don't upload the _subresults.json since they usually not useful for investigating test failures.
                        if(!$file.Name.EndsWith(".dmp") -and !$file.Name.EndsWith("_subresults.json"))
                        {
                            $destination = "$workItemOutputDir\$($file.Name)"
                            Write-Host "Copying $($file.Name) to $destination"

                            # file itself might be in a subdirectory.
                            Ensure-Path(Split-Path -Path $destination)

                            $fileurl = Append-HelixAccessTokenToUrl $file.Link  $HelixAccessToken

                            try
                            {
                                Download-FileWithRetries $fileurl $destination
                            }
                            catch
                            {
                                Log-Error "Failed to download $($file.Name): $($_.Exception.Message)"
                            }
                        }
                    }

                    $dumps = @($files | where { $_.Name.EndsWith(".dmp") })
                    if($dumps.Count -gt 0)
                    {
                        if(-Not $isFileHeaderShown)
                        {
                            Out-File -FilePath $helixLinkFile -Append -InputObject $fileHeader
                            $isFileHeaderShown = $true
                        }
                        if(-Not $isTestRunNameShown)
                        {
                            Out-File -FilePath $helixLinkFile -Append -InputObject "<h2>$($testRun.name)</h2>"
                            $isTestRunNameShown = $true
                        }
                        Out-File -FilePath $helixLinkFile -Append -InputObject "<h3>$helixWorkItemName</h3>"
                        Generate-File-Links $dumps "CrashDumps"
                    }
                }
            }
        }
    }
}

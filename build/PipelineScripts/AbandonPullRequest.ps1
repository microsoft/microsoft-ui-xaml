# Payload Tracking Tool for WindowsAppSDK
# But could be adapted to work with any repository with version.details.xml

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

Param(
    [String]$pullRequestId = "",
    [String]$organization = "",
    [String]$project = "",
    [String]$repository = "",
    [String]$pat
)

if ([String]::IsNullOrEmpty($pat))
{
    Write-Host "AzDo PAT Required"
    exit 1;
}

$exitCode = 0
$env:AZDO_PAT = $pat
$env:AZDO_API_VERSION = "7.1-preview"
$env:AZDO_ENDPOINT = "https://dev.azure.com"

function Get-AzDo-Auth-Header
{
    $b64pat = [Convert]::ToBase64String([System.Text.Encoding]::ASCII.GetBytes(":$($env:AZDO_PAT)"))
    $header = @{
        Authorization="Basic $b64pat"
    }
    return $header
}

function Patch-Request
{
    Param(
        [Parameter(Mandatory=$true)]
        [String]$request,
        [Parameter(Mandatory=$true)]
        [PSobject]$header,
        [PSobject]$body,
        [String]$mediaType = "application/json"
    )
    Write-Host "PATCH " $request
    $response = Invoke-WebRequest -Uri $request -Method PATCH `
                                -Headers $header -Body $body `
                                -ContentType $mediaType
    if ($response.statuscode -ge 200 -and $response.statuscode -lt 300)
    {
        if ($mediaType -eq "application/json")
        {
            return ConvertFrom-Json $response.Content
        }
        if ($mediaType -eq "text/plain")
        {
            return $response
        }
    }
    Write-Host "Exit with response code: " $response.statuscode
}

function Patch-PullRequest-Status
{
    Param(
        [Parameter(Mandatory=$true)]
        [String] $organization,
        [Parameter(Mandatory=$true)]
        [String] $project,
        [Parameter(Mandatory=$true)]
        [String] $repository,
        [Parameter(Mandatory=$true)]
        [String] $pullRequestId,
        [Parameter(Mandatory=$true)]
        [String] $status
    )
    $headers = Get-AzDo-Auth-Header
    $api = "_apis/git/repositories/$repository/pullrequests/$pullRequestId"
    $request = "$env:AZDO_ENDPOINT/$organization/$project/" + $api + "?api-version=$env:AZDO_API_VERSION"
    $body = ConvertTo-Json @{
        status = "$status"
    }
    Write-Host $body
    $response = Patch-Request -request $request -Body $body -header $headers
    Write-Host $response
}

function AbandonPullRequest
{
    Param(
        [Parameter(Mandatory=$true)]
        [String] $organization,
        [Parameter(Mandatory=$true)]
        [String] $project,
        [Parameter(Mandatory=$true)]
        [String] $repository,
        [Parameter(Mandatory=$true)]
        [String] $pullRequestId
    )
    Patch-PullRequest-Status -organization $organization -project $project -repository $repository -pullRequestId $pullRequestId -status "abandoned"

}

AbandonPullRequest -organization $organization -project $project -repository $repository -pullRequestId $pullRequestId

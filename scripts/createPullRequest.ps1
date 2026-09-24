# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

#
# Creates a pull request of any changes made to the local branch with a given title and description
#

[CmdletBinding()]
param(
  $AzureDevOpsPat,
  $branch,
  $title,
  $description
)

Write-Host "Branch name: $branch"
if($branch -eq $null -or $branch -eq "")
{
    Write-Host "Please provide a branch name." -ForegroundColor Red
    Exit 1
}

if($AzureDevOpsPat -eq $null -or $AzureDevOpsPat -eq "")
{
    Write-Host "Please provide a Azure DevOps PAT." -ForegroundColor Red
    Exit 1
}

$AzureDevOpsAuthenicationHeader = @{Authorization = 'Basic ' + [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes(":$($AzureDevOpsPat)")) }
    
$apiCallString = "$($env:WinUIPullRequestsApiUri)?api-version=5.1"

$pullRequestParameters = @{
    "sourceRefName"="refs/heads/$branch"
    "targetRefName"="refs/heads/main"
    "title"="$title"
    "description"="$description"
    "reviewers"=@()
}

$body = (ConvertTo-Json $pullRequestParameters)

    
Write-Host "Getting porting branch pull requests..."
Write-Host
Write-Host "API call: $apiCallString"
Write-Host

$pr = Invoke-RestMethod -Uri $apiCallString -Body $body -Method Post -ContentType "application/json" -Headers $AzureDevOpsAuthenicationHeader
$pr

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

# Get personal access tokens here:
#
#   GitHub: https://github.com/settings/tokens
#   Azure DevOps: use your organization token settings page.
#
# NB: Keep these tokens private. They allow scripts to perform actions on your behalf.

Param(
    [Parameter(Mandatory = $true)]
    [string]$AzureDevOpsRelease,
    [Parameter(Mandatory = $true)]
    [string]$AzureDevOpsOrganization,
    [Parameter(Mandatory = $true)]
    [string]$AzureDevOpsProject,
    [Parameter(Mandatory = $true)]
    [string]$GitHubPersonalAccessToken,
    [Parameter(Mandatory = $false)]
    [string]$AzureDevOpsPersonalAccessToken,
    [Parameter(Mandatory = $true)]
    [string]$BaseAreaPath,
    [Parameter(Mandatory = $true)]
    [string]$ControlsAreaPath,
    [Parameter(Mandatory = $true)]
    [string]$InputAreaPath,
    [Parameter(Mandatory = $true)]
    [string]$IoAreaPath,
    [Parameter(Mandatory = $true)]
    [string]$LifetimeAreaPath,
    [Parameter(Mandatory = $true)]
    [string]$MarkupAreaPath,
    [Parameter(Mandatory = $true)]
    [string]$MrtAreaPath,
    [Parameter(Mandatory = $true)]
    [string]$RenderingAreaPath,
    [Parameter(Mandatory = $false)]
    [string]$OnBehalfOfIdentity = "",
    [Parameter()]
    [switch]$CommitChanges,
    [Parameter()]
    [switch]$UseSystemAccessToken
)

. "$PSScriptRoot\MirrorGitHubIssuesHelperFunctions.ps1"

# This script reflects changes to a GitHub issue to its corresponding Azure DevOps work item,
# or, if the Azure DevOps work item has been modified more recently than the GitHub issue,
# it reflects back to the GitHub issue the open/closed status of the Azure DevOps work item
# as well as the reason for closure if the work item is closed, plus the milestone in which
# the associated change will be included.
#
# To do that, we'll first retrieve all open GitHub issues not marked with "discussion",
# "feature request", or "question" labels, and then pair them with their associated
# Azure DevOps work item (if one exists).
#
# If no associated Azure DevOps work item exists, we'll create one.
# If one does exist, then we'll next consider which of the two have been edited last.
#
# If the GitHub issue has been edited last, we'll bring over its title, description, and open/closed status
# to the Azure DevOps work item.
#
# If the Azure DevOps work item has been edited last, we'll bring over its open/closed status and
# reason for closure (if closed) to the GitHub issue.

$gitHubAuthorizationHeader = "Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$($GitHubPersonalAccessToken)")))"

if ($UseSystemAccessToken)
{
    if ([string]::IsNullOrEmpty($env:SYSTEM_ACCESSTOKEN))
    {
        throw "-UseSystemAccessToken was specified but the SYSTEM_ACCESSTOKEN environment variable is not set. The pipeline step must map env: SYSTEM_ACCESSTOKEN: `$(System.AccessToken)."
    }
    $azureDevOpsAuthorizationHeader = "Bearer $env:SYSTEM_ACCESSTOKEN"
    Write-Host "ADO auth mode: System.AccessToken (bearer)"

    # Under System.AccessToken the caller is the build service identity, which may not be in the target
    # process template's limitedToValues list for OSG.CreatedOnBehalfOf or Microsoft.VSTS.Common.ActivatedBy.
    # ADO server-side rules default both fields to the caller identity on create, which fails validation. We
    # explicitly set them to the configured identity so the bug attribution matches the historical PAT-era identity.
    # The PAT path needs no override because the PAT was minted as that same user, so server defaults already match.
    $onBehalfOfIdentity = $OnBehalfOfIdentity
}
else
{
    if ([string]::IsNullOrEmpty($AzureDevOpsPersonalAccessToken))
    {
        throw "AzureDevOpsPersonalAccessToken is required when -UseSystemAccessToken is not specified."
    }
    $azureDevOpsAuthorizationHeader = "Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$($AzureDevOpsPersonalAccessToken)")))"
    Write-Host "ADO auth mode: PAT (basic)"
    $onBehalfOfIdentity = $null
}

# Base URIs for the Azure DevOps work item tracking REST API and web UI, sourced from pipeline secrets.
$azureDevOpsWitApiBaseUri = $env:OSAzureDevOpsWitApiUri
$azureDevOpsWorkItemWebBaseUri = "$env:OSAzureDevOpsWorkItemsWebUri/edit"

$gitHubRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"=$gitHubAuthorizationHeader
}

$gitHubRestSearchApiHeaders = @{
    "Accept"="application/vnd.github+json"
    "Authorization"=$gitHubAuthorizationHeader
}

$gitHubRestPatchApiHeaders = @{
    "Accept"="application/vnd.github+json"
    "Authorization"=$gitHubAuthorizationHeader
}

$gitHubRestApiMarkdownHeaders = @{
    "Accept"="application/vnd.github.v3+json"
    "Authorization"=$gitHubAuthorizationHeader
}

$gitHubRestApiRawMarkdownHeaders = @{
    "Accept"="application/vnd.github.v3+json"
    "Content-Type"="text/plain"
    "Authorization"=$gitHubAuthorizationHeader
}

$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"=$azureDevOpsAuthorizationHeader
}

$azureDevOpsRestPostApiHeaders = @{
    "Accept"="application/json"
    # Bodies are sent as UTF-8 byte[] (see ConvertTo-AzureDevOpsRequestBody) because PS 5.1's
    # Invoke-RestMethod string-body path emits Windows-1252 regardless of this charset directive.
    # Keeping the directive for correctness against any future runtime / consumer that honors it.
    "Content-Type"="application/json; charset=utf-8"
    "Authorization"=$azureDevOpsAuthorizationHeader
}

$azureDevOpsRestPatchApiHeaders = @{
    "Accept"="application/json-patch+json"
    "Content-Type"="application/json-patch+json; charset=utf-8"
    "Authorization"=$azureDevOpsAuthorizationHeader
}

$azureDevOpsRestAttachmentUploadApiHeaders = @{
    "Accept"="application/json"
    "Content-Type"="application/octet-stream"
    "Authorization"=$azureDevOpsAuthorizationHeader
}

function Invoke-WithRetry {
    param(
        [scriptblock]$Script,
        [int]$MaxAttempts = 5,
        [int]$DelaySeconds = 5
    )
    for ($attempt = 1; $attempt -le $MaxAttempts; $attempt++) {
        try { 
            return & $Script 
        }
        catch {
            if ($attempt -eq $MaxAttempts) { 
                Write-Host "Final attempt $attempt failed. Throwing exception."
                throw 
            }
            Start-Sleep -Seconds $DelaySeconds
        }
    }
}

# To begin, we'll retrieve all of the open issues from the GitHub repo.

$issueCount = (Invoke-RestMethod "https://api.github.com/search/issues?q=repo:microsoft/microsoft-ui-xaml&per_page=1" -Headers $gitHubRestSearchApiHeaders).total_count

[System.Collections.Generic.List[object]]$gitHubIssues = @()

$retrievingIssuesStatus = "Retrieving issues from the GitHub repository..."
Write-Host $retrievingIssuesStatus

# Link Header-based pagination
$issues = 0
$nextUri = "https://api.github.com/repos/microsoft/microsoft-ui-xaml/issues?state=all&direction=asc&sort=created&per_page=100"

while ($nextUri)
{
    $percentComplete = 100 * $issues / $issueCount 

    # This can happen if a new issue comes in while we're in the middle of iterating.
    if ($percentComplete -gt 100)
    {
        $percentComplete = 100
    }

    Write-Progress -Activity $retrievingIssuesStatus -PercentComplete $percentComplete

    $currentPageResponse = Invoke-WithRetry { Invoke-WebRequest -Uri $nextUri -Headers $gitHubRestApiHeaders -UseBasicParsing }

    $currentPageIssues = @()
    if ($currentPageResponse.Content) {
        $currentPageIssues = @((ConvertFrom-Json $currentPageResponse.Content))
    }

    if ($currentPageIssues.Count -gt 0) {
        $gitHubIssues.AddRange($currentPageIssues)
        $issues += $currentPageIssues.Count
    }

    $linkHeader = $currentPageResponse.Headers['Link']
    $nextPageUri = $null
    if ($linkHeader) {
        foreach ($part in ($linkHeader -split ',')) {
            if ($part -match '<(?<url>[^>]+)>\s*;\s*rel="next"') {
                $nextPageUri = $Matches['url']
                break
            }
        }
    }

    $nextUri = $nextPageUri
}

Write-Progress -Activity $retrievingIssuesStatus -Completed

Write-Host "Sorting GitHub issues and removing pull requests and non-bug issues..."

# The issues API also includes pull requests.  We'll sort them and strip those from the list.
# We'll also exclude from consideration any issue marked as "discussion", "feature proposal",
# "needs-triage", or "question". We don't want to port any of those to Azure DevOps.
$excludedLabels = @(
    "discussion",
    "feature proposal",
    "needs-triage",
    "question"
)

# Transient labels that are applied and then re-applied should cause us to fail to initially
# mirror over a bug, but if we've already mirrored a bug, they shouldn't cause us to remove the
# MirroredFromGitHub tab.
$transientLabels = @(
    "needs-triage"
)

$gitHubIssues = $gitHubIssues | Sort-Object -Property "number" -Unique
$gitHubIssues = [System.Linq.Enumerable]::ToList(
    [System.Linq.Enumerable]::Where($gitHubIssues, [Func[object,bool]] {
        param($issue)
        -not $issue.pull_request -and 
        -not [System.Linq.Enumerable]::Any($issue.labels, [Func[object,bool]] {
            param($label)
            $excludedLabels.Contains($label.name)
        })
    }))

# Next, we'll run an Azure DevOps query that retrieves the existing work items that have been mirrored from GitHub.
$azureDevOpsQueryResults = Invoke-RestMethod "$azureDevOpsWitApiBaseUri/wiql/$($env:OSAzureDevOpsMirrorQueryId)?api-version=5.1" -Method Get -Headers $azureDevOpsRestApiHeaders

# Next, we'll associate GitHub issues with their corresponding Azure DevOps work items.
[System.Collections.Generic.List[System.Tuple[object,object]]]$azureDevOpsWorkItemGitHubIssuePairs = @()

$associatingWorkItemsStatus = "Associating Azure DevOps work items with GitHub issues..."
Write-Host $associatingWorkItemsStatus

$workItemIndex = 0
$workItemCount = $azureDevOpsQueryResults.workItems.Count

foreach ($azureDevOpsWorkItem in $azureDevOpsQueryResults.workItems)
{
    Write-Progress -Activity $associatingWorkItemsStatus -PercentComplete (100 * $workItemIndex / $workItemCount)
    $workItemIndex++

    $azureDevOpsWorkItemDetails = Invoke-WithRetry { Invoke-RestMethod $azureDevOpsWorkItem.url -Method Get -Headers $azureDevOpsRestApiHeaders }
    $gitHubIssueNumber = $azureDevOpsWorkItemDetails.fields.'Microsoft.VSTS.Common.CustomString09' -ireplace "[^0-9]", ""

    if ([string]::IsNullOrWhiteSpace($gitHubIssueNumber))
    {
        continue
    }

    $gitHubIssue = Get-GitHubIssueByNumber $gitHubIssues $gitHubIssueNumber

    if ($gitHubIssue)
    {
        $azureDevOpsWorkItemGitHubIssuePairs.Add([System.Tuple[object,object]]::new($azureDevOpsWorkItemDetails, $gitHubIssue))
    }
    else
    {
        # If an Azure DevOps work item doesn't have an associated GitHub issue, we still want to include it.
        # That will occur when the GitHub issue has been closed, in which case we'll want to also close the
        # Azure DevOps work item.
        $azureDevOpsWorkItemGitHubIssuePairs.Add([System.Tuple[object,object]]::new($azureDevOpsWorkItemDetails, $null))
    }
}

Write-Progress -Activity $associatingWorkItemsStatus -Completed

Write-Host "Removing closed GitHub issues..."

$gitHubOpenIssues = [System.Linq.Enumerable]::ToList(
    [System.Linq.Enumerable]::Where($gitHubIssues, [Func[object,bool]] {
        param($issue)
        $issue.state -eq "open"
    }))

$addingOpenIssuesStatus = "Adding open issues from the GitHub repository with no associated Azure DevOps work item..."
Write-Host $addingOpenIssuesStatus

$openIssueIndex = 0
$openIssueCount = $gitHubOpenIssues.Count

# We'll also add appropriate GitHub bugs without associated Azure DevOps work items - those are ones
# where we need to create new Azure DevOps bugs for them.
foreach ($gitHubIssue in $gitHubOpenIssues)
{
    Write-Progress -Activity $addingOpenIssuesStatus -PercentComplete (100 * $openIssueIndex / $openIssueCount)
    $openIssueIndex++

    if (-not [System.Linq.Enumerable]::Any($azureDevOpsWorkItemGitHubIssuePairs, [Func[System.Tuple[object, object],bool]] {
            param($pair)
            $pair.Item2.number -eq $gitHubIssue.number
        }))
    {
        $azureDevOpsWorkItemGitHubIssuePairs.Add([System.Tuple[object,object]]::new($null, $gitHubIssue))
    }
}

Write-Progress -Activity $addingOpenIssuesStatus -Completed

# We'll put our bugs in order as follows:
#
#   1. First, bugs with a GitHub issue but no associated Azure DevOps work item.
#      These will result in new Azure DevOps work items being added.
#   2. Second, bugs with both a GitHub issue and an associated Azure DevOps work item.
#      These will result in the less recently modified item being updated.
#   3. Finally, bugs with an Azure DevOps work item but no GitHub issue.
#      These should have their MirroredFromGitHub tag removed.
#
#  Within these categories, we'll put things first in order of GitHub issue number
#  and then in order of Azure DevOps work item ID.

$azureDevOpsWorkItemGitHubIssuePairs.Sort([System.Comparison[System.Tuple[object,object]]]{
    param($pair1, $pair2)

    $gitHubIssue1 = $pair1.Item2
    $gitHubIssue2 = $pair2.Item2
    $azureDevOpsWorkItem1 = $pair1.Item1
    $azureDevOpsWorkItem2 = $pair2.Item1

    if ($gitHubIssue1 -and $gitHubIssue2)
    {
        if (($azureDevOpsWorkItem1 -and $azureDevOpsWorkItem2) -or
            (-not $azureDevOpsWorkItem1 -and -not $azureDevOpsWorkItem2))
        {
            return $gitHubIssue1.number - $gitHubIssue2.number
        }
        elseif (-not $azureDevOpsWorkItem1)
        {
            return -1
        }
        elseif (-not $azureDevOpsWorkItem2)
        {
            return 1
        }
    }
    elseif (-not $gitHubIssue1 -and -not $gitHubIssue2)
    {
        return ([int]$azureDevOpsWorkItem1.id) - ([int]$azureDevOpsWorkItem2.id)
    }
    elseif ($gitHubIssue1 -and -not $gitHubIssue2)
    {
        return -1
    }
    else # -not $gitHubIssue1 -and $gitHubIssue2
    {
        return 1
    }
})

$changesMade = $false

# If we don't want to commit our changes, and instead are just making sure things look good,
# then add the "validateOnly=true" argument to ensure that we don't commit anything.
if (-not $CommitChanges)
{
    $validateOnlyString = "&validateOnly=true"
}
else
{
    $validateOnlyString = [string]::Empty
}

$mirroringIssuesStatus = "Mirroring GitHub issues and Azure DevOps work items..."
Write-Host $mirroringIssuesStatus

$pairIndex = 0
$pairCount = $azureDevOpsWorkItemGitHubIssuePairs.Count

Write-Host

foreach ($workItemIssuePair in $azureDevOpsWorkItemGitHubIssuePairs)
{
    Write-Progress -Activity $mirroringIssuesStatus -PercentComplete (100 * $pairIndex / $pairCount)
    $pairIndex++

    $azureDevOpsWorkItem = $workItemIssuePair.Item1
    $gitHubIssue = $workItemIssuePair.Item2

    if ($gitHubIssue)
    {
        # JSON does not like smart quotes.
        $issueTitle = $gitHubIssue.title.Replace("`“", "`"").Replace("`”", "`"").Replace("’", "'")

        # We'll prepend the title with "[GitHub]" so it's easy to see what issues were mirrored.
        $issueTitle = "[GitHub] $issueTitle"

        # The typing of the issue number is inconsistent - GitHub returns it as an integer value, while Azure DevOps returns it as a string.
        # For consistency, we'll coerce the Github value to also be a string, since otherwise comparisons will fail.
        [string]$issueNumber = $gitHubIssue.number
        
        # If this issue number has an associated Azure DevOps work item, then we want to perform mirroring.
        # If the GitHub issue has been changed more recently than the Azure DevOps work item, then we'll update
        # the title, description, and open/closed status of the Azure DevOps work item.
        # If the Azure DevOps work item has changed more recently, then we'll mirror the open/closed status
        # back to the GitHub issue.
        if ($azureDevOpsWorkItem)
        {
            $gitHubIssueLastUpdatedTime = [DateTime]::Parse($gitHubIssue.updated_at, [Globalization.CultureInfo]::InvariantCulture, [Globalization.DateTimeStyles]::RoundtripKind)
            $azureDevOpsWorkItemLastUpdatedTime = [DateTime]::Parse($azureDevOpsWorkItem.fields.'System.ChangedDate', [Globalization.CultureInfo]::InvariantCulture, [Globalization.DateTimeStyles]::RoundtripKind)

            if ($gitHubIssueLastUpdatedTime -gt $azureDevOpsWorkItemLastUpdatedTime)
            {
                [System.Collections.Generic.List[System.Object]]$updateRequestParameters = @()
                [System.Collections.Generic.List[string]]$changeStrings = @()

                if ($issueTitle -cne $azureDevOpsWorkItem.fields.'System.Title')
                {
                    $changesMade = $true

                    $updateRequestParameters.Add((Get-WorkItemParameter "System.Title" $issueTitle "replace"))
                    $changeStrings.Add("Updated title: $($azureDevOpsWorkItem.fields.'System.Title') -> ${issueTitle}")
                }

                $tags = $azureDevOpsWorkItem.fields.'System.Tags'

                foreach ($label in $gitHubIssue.labels)
                {
                    $azureDevOpsTag = "GitHub_$($label.name)"

                    if (-not $tags.Contains($azureDevOpsTag))
                    {
                        $tags = "${tags}; $azureDevOpsTag"
                    }
                }

                if ($tags -ine $azureDevOpsWorkItem.fields.'System.Tags')
                {
                    $changesMade = $true

                    $updateRequestParameters.Add((Get-WorkItemParameter "System.Tags" $tags "replace"))
                    $changeStrings.Add("Updated tags.")
                }

                $previousEngagementCountString = $azureDevOpsWorkItem.fields.'OSG.NumInstances' -ireplace "[^0-9]", ""
                $previousEngagementCount = 0

                if (-not [string]::IsNullOrEmpty($previousEngagementCountString))
                {
                    $previousEngagementCount = [System.Int32]::Parse($previousEngagementCountString)
                }
                else
                {
                    $previousEngagementCount = 0
                }

                $engagementCount = Get-GitHubIssueEngagementCount $gitHubIssue $gitHubRestApiHeaders

                if ($previousEngagementCount -ne $engagementCount)
                {
                    $updateRequestParameters.Add((Get-WorkItemParameter "OSG.NumInstances" $engagementCount "replace"))
                    $changeStrings.Add("Updated engagement count: $previousEngagementCount -> $engagementCount")
                }

                $shouldCloseBug = $false
                $commentText = $null

                if (-not (Get-IsAzureDevOpsWorkItemOpen $azureDevOpsWorkItem) -and $gitHubIssue.state -ieq "open")
                {
                    $changesMade = $true

                    if ($azureDevOpsWorkItem.fields.'System.WorkItemType' -ieq "Bug")
                    {
                        $updateRequestParameters.Add((Get-WorkItemParameter "System.State" "Active" "replace"))
                        $updateRequestParameters.Add((Get-WorkItemParameter "System.AssignedTo" @{
                            "displayName"="Active"
                            "id"=$null
                        } "replace"))
                        if ($onBehalfOfIdentity)
                        {
                            $updateRequestParameters.Add((Get-WorkItemParameter "Microsoft.VSTS.Common.ActivatedBy" $onBehalfOfIdentity "replace"))
                        }
                    }
                    else
                    {
                        $updateRequestParameters.Add((Get-WorkItemParameter "System.State" "Proposed" "replace"))
                    }

                    $changeStrings.Add("Reopened.")
                    $commentText = "Reopening because the associated GitHub issue was reopened."
                }
                elseif ((Get-IsAzureDevOpsWorkItemOpen $azureDevOpsWorkItem) -and $gitHubIssue.state -ieq "closed")
                {
                    $changesMade = $true

                    $closureAndReason = Get-AzureDevOpsClosureAndReason $gitHubIssue $azureDevOpsWorkItem
                    $updateRequestParameters.Add((Get-WorkItemParameter "System.State" $closureAndReason.Closure "replace"))

                    if ($closureAndReason.Reason)
                    {
                        $updateRequestParameters.Add((Get-WorkItemParameter "Microsoft.VSTS.Common.ResolvedReason" $closureAndReason.Reason "replace"))
                        $shouldCloseBug = $true
                    }

                    if ($onBehalfOfIdentity)
                    {
                        $updateRequestParameters.Add((Get-WorkItemParameter "Microsoft.VSTS.Common.ResolvedBy" $onBehalfOfIdentity "replace"))
                    }

                    $changeStrings.Add("Closed.")

                    if ($closureAndReason.Reason)
                    {
                        $commentText = "$($closureAndReason.Closure) as $($closureAndReason.Reason) because the associated GitHub issue was closed as $($gitHubIssue.state_reason)."
                    }
                    else
                    {
                        $commentText = "$($closureAndReason.Closure) because the associated GitHub issue was closed as $($gitHubIssue.state_reason)."
                    }
                }

                if ($updateRequestParameters.Count -gt 0)
                {
                    $updateWorkItemUri = "$azureDevOpsWitApiBaseUri/workitems/$($azureDevOpsWorkItem.id)?api-version=5.1$validateOnlyString"
                    $updateSucceeded = $true
                    $exception = $null

                    try
                    {
                        $response = Invoke-RestMethod $updateWorkItemUri -Method Patch -Headers $azureDevOpsRestPatchApiHeaders -Body (ConvertTo-AzureDevOpsRequestBody $updateRequestParameters)
                    }
                    catch
                    {
                        $exception = $_
                        $updateSucceeded = $false
                    }

                    if ($updateSucceeded)
                    {
                        Write-Host @"
Updated Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()): ${issueTitle}
  GitHub issue: https://github.com/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)
  Azure DevOps $($response.fields.'System.WorkItemType'.ToLower()): $azureDevOpsWorkItemWebBaseUri/$($response.id)
    - $($changeStrings -join "$([Environment]::NewLine)    - ")
"@
                    }
                    else
                    {
                        Write-Error @"
Failed updating Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()): ${issueTitle}
  GitHub issue: https://github.com/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)
  Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()): $azureDevOpsWorkItemWebBaseUri/$($azureDevOpsWorkItem.id)
    - $($changeStrings -join "$([Environment]::NewLine)    - ")
  Error message: $exception
"@
                    }

                    $commentSucceeded = $true
                    $exception = $null
                    
                    if ($commentText -and $CommitChanges)
                    {
                        try
                        {
                            $response = Invoke-RestMethod "$azureDevOpsWitApiBaseUri/workitems/$($azureDevOpsWorkItem.id)/comments?api-version=7.0-preview.3" -Method Post -Headers $azureDevOpsRestPostApiHeaders -Body (ConvertTo-AzureDevOpsRequestBody @{ "text" = $commentText })
                        }
                        catch
                        {
                            $exception = $_
                            $commentSucceeded = $false
                        }

                        if (-not $commentSucceeded)
                        {
                        Write-Error @"
Failed to add a comment to Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()).
  Error message: $exception
"@
                        }
                    }
                }
        
                # Azure DevOps doesn't let us send a bug directly to the Closed state - we need to go through Resolved first.
                # Now that we've done that, we can close the bug.
                if ($shouldCloseBug)
                {
                    $closeStateParameters = @((Get-WorkItemParameter "System.State" "Closed" "replace"))
                    if ($onBehalfOfIdentity)
                    {
                        $closeStateParameters += (Get-WorkItemParameter "Microsoft.VSTS.Common.ClosedBy" $onBehalfOfIdentity "replace")
                    }
                    $updateStateRequestBody = ConvertTo-AzureDevOpsRequestBody $closeStateParameters
                    $updateSucceeded = $true
                    $exception = $null

                    try
                    {
                        # Don't bother if we aren't committing changes - this will throw an error since we didn't actually resolve the bug earlier.
                        if ($CommitChanges)
                        {
                            $response = Invoke-RestMethod $updateWorkItemUri -Method Patch -Headers $azureDevOpsRestPatchApiHeaders -Body $updateStateRequestBody
                        }
                    }
                    catch
                    {
                        $exception = $_
                        $updateSucceeded = $false
                    }
                    
                    if (-not $updateSucceeded)
                    {
                        Write-Error @"
Failed to close Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()).
  Error message: $exception
"@
                    }
                }
            }
            else
            {
                if (-not (Get-IsAzureDevOpsWorkItemOpen $azureDevOpsWorkItem) -and $gitHubIssue.state -eq "open")
                {
                    $changesMade = $true
                    $resolvedLabel = Get-GitHubIssueResolvedLabel $azureDevOpsWorkItem

                    if ($resolvedLabel -eq "closed-Fixed")
                    {
                        $stateReason = "completed"
                    }
                    else
                    {
                        $stateReason = "not_planned"
                    }

                    $gitHubLabels = [System.Linq.Enumerable]::ToList([System.Linq.Enumerable]::Select($gitHubIssue.labels, [Func[object,string]] {
                        param($label)
                        $label.name
                    }))

                    $gitHubLabels.Add($resolvedLabel)

                    $updateRequestBody = @{
                        "state" = "closed"
                        "state_reason" = $stateReason
                        "labels" = $gitHubLabels
                    }

                    # If the bug was closed as fixed, then we'll also add the milestone if it matches an existing one in the GitHub repo.
                    if ($resolvedLabel -eq "closed-Fixed")
                    {
                        $milestones = Invoke-RestMethod "https://api.github.com/repos/microsoft/microsoft-ui-xaml/milestones" -Headers $gitHubRestApiHeaders

                        foreach ($milestone in $milestones)
                        {
                            if ($milestone.title -ieq $azureDevOpsWorkItem.fields.'Microsoft.VSTS.Common.Release')
                            {
                                $updateRequestBody.Add("milestone", $milestone.number)
                                break
                            }
                        }
                    }
                    
                    $updateIssueUri = "https://api.github.com/repos/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)"
                    $updateSucceeded = $true
                    $exception = $null
                    
                    if ($CommitChanges)
                    {
                        try
                        {
                            $response = Invoke-RestMethod $updateIssueUri -Method Patch -Headers $gitHubRestPatchApiHeaders -Body (ConvertTo-Json $updateRequestBody)
                        }
                        catch
                        {
                            $exception = $_
                            $updateSucceeded = $false
                        }
                    }
            
                    if ($updateSucceeded)
                    {
                        Write-Host @"
Closed GitHub issue: ${issueTitle}
  GitHub issue: https://github.com/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)
  Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()): $azureDevOpsWorkItemWebBaseUri/$($azureDevOpsWorkItem.id)
"@
                    }
                    else
                    {
                        Write-Error @"
Failed to close GitHub issue: ${issueTitle}
  GitHub issue: https://github.com/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)
  Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()): $azureDevOpsWorkItemWebBaseUri/$($azureDevOpsWorkItem.id)
  Error message: $exception
"@
                    }
                }
                elseif ((Get-IsAzureDevOpsWorkItemOpen $azureDevOpsWorkItem) -and $gitHubIssue.state -eq "closed")
                {
                    $changesMade = $true

                    $gitHubLabels = [System.Linq.Enumerable]::ToList([System.Linq.Enumerable]::Select($gitHubIssue.labels, [Func[object,string]] {
                        param($label)
                        $label.name
                    }))

                    $gitHubLabels = [System.Linq.Enumerable]::ToList(
                        [System.Linq.Enumerable]::Where($gitHubLabels, [Func[object,bool]] {
                            param($label)
                            -not $label.StartsWith("closed-")
                        }))

                    $updateRequestBody = @{
                        "state" = "open"
                        "state_reason" = "reopened"
                        "labels" = $gitHubLabels
                    }
                    
                    $updateIssueUri = "https://api.github.com/repos/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)"
                    $updateSucceeded = $true
                    $exception = $null
                    
                    if ($CommitChanges)
                    {
                        try
                        {
                            $response = Invoke-RestMethod $updateIssueUri -Method Patch -Headers $gitHubRestPatchApiHeaders -Body (ConvertTo-Json $updateRequestBody)
                        }
                        catch
                        {
                            $exception = $_
                            $updateSucceeded = $false
                        }
                    }
            
                    if ($updateSucceeded)
                    {
                        Write-Host @"
Reopened GitHub issue: ${issueTitle}
    GitHub issue: https://github.com/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)
    Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()): $azureDevOpsWorkItemWebBaseUri/$($azureDevOpsWorkItem.id)
"@
                    }
                    else
                    {
                        Write-Error @"
Failed to reopen GitHub issue: ${issueTitle}
    GitHub issue: https://github.com/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)
    Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()): $azureDevOpsWorkItemWebBaseUri/$($azureDevOpsWorkItem.id)
    Error message: $exception
"@
                    }
                }
            }
        }
        else
        {
            # If this issue number has no associated Azure DevOps work item, then we want to create one.
            $changesMade = $true

            $issueDescription = Get-AzureDevOpsDescription $gitHubIssue $gitHubRestApiMarkdownHeaders $gitHubRestApiRawMarkdownHeaders
            $issueAreaPath = Get-AzureDevOpsAreaPath $gitHubIssue $BaseAreaPath $ControlsAreaPath $InputAreaPath $IoAreaPath $LifetimeAreaPath $MarkupAreaPath $MrtAreaPath $RenderingAreaPath
            $engagementCount = Get-GitHubIssueEngagementCount $gitHubIssue $gitHubRestApiHeaders

            $tags = "MirroredFromGitHub"

            foreach ($label in $gitHubIssue.labels)
            {
                $tags = "${tags}; GitHub_$($label.name)"
            }

            $createSucceeded = $true
            $exception = $null

            # GitHub issues can have uploaded images and videos attached to them. In order to keep everything in Azure DevOps,
            # we'll also upload those associated files to Azure DevOps and update file paths to the Azure DevOps attachment.
            [System.Collections.Generic.List[string]]$attachmentUrls = @()
            
            $uploadedFiles = [System.Text.RegularExpressions.Regex]::Matches($issueDescription, "[^`"]+/([^`"/]+)\?jwt=[^`"]*") | Select-Object -Unique
            foreach ($uploadedFile in $uploadedFiles)
            {
                try
                {
                    $uploadedFile = [System.Text.RegularExpressions.Match]$uploadedFile
                        
                    if ($CommitChanges)
                    {
                        $fileName = $uploadedFile.Groups[1].Value
                        $tempFileName = "${env:TEMP}\$fileName"
                        $uploadedFileBytes = (Invoke-WebRequest -Uri $uploadedFile.Value -UseBasicParsing).Content
                        $uploadResponse = Invoke-RestMethod "$azureDevOpsWitApiBaseUri/attachments?fileName=$fileName&api-version=7.1-preview.3" -Method Post -Headers $azureDevOpsRestAttachmentUploadApiHeaders -Body $uploadedFileBytes
                        $attachmentUrls.Add($uploadResponse.url)

                        $issueDescription = $issueDescription.Replace($uploadedFile.Value, $uploadResponse.url)
                    }
                    else
                    {
                        # Even if we're not committing changes, GitHub attachment URLs can have tokens that CredScan misidentifies as secrets,
                        # so we'll update file paths to "placeholder" to avoid that.
                        $issueDescription = $issueDescription.Replace($uploadedFile.Value, "placeholder")
                    }
                }
                catch
                {
                    $exception = $_
                    $createSucceeded = $false
                    break
                }
            }

            if ($createSucceeded)
            {
                $createWorkItemParameters = @(
                    (Get-WorkItemParameter "System.Title" $issueTitle),
                    (Get-WorkItemParameter "System.Description" $issueDescription),
                    (Get-WorkItemParameter "Microsoft.VSTS.TCM.ReproSteps" $issueDescription),
                    (Get-WorkItemParameter "System.AreaPath" $issueAreaPath),
                    (Get-WorkItemParameter "System.TeamProject" $AzureDevOpsProject),
                    (Get-WorkItemParameter "System.IterationPath" $AzureDevOpsProject),
                    (Get-WorkItemParameter "System.WorkItemType" "Bug"),
                    (Get-WorkItemParameter "System.State" "Active"),
                    (Get-WorkItemParameter "OSG.NumInstances" $engagementCount),
                    (Get-WorkItemParameter "Microsoft.VSTS.Common.CustomString09" $issueNumber),
                    (Get-WorkItemParameter "Microsoft.VSTS.Common.Release" $AzureDevOpsRelease),
                    (Get-WorkItemParameter "OSG.ProductFamily" "Applications"),
                    (Get-WorkItemParameter "OSG.Product" "WinUI"),
                    (Get-WorkItemParameter "System.Tags" $tags),
                    (Get-WorkItemParameter "System.AssignedTo" @{
                        "displayName"="Active"
                        "id"=$null
                    })
                )

                if ($onBehalfOfIdentity)
                {
                    $createWorkItemParameters += (Get-WorkItemParameter "OSG.CreatedOnBehalfOf" $onBehalfOfIdentity)
                    $createWorkItemParameters += (Get-WorkItemParameter "Microsoft.VSTS.Common.ActivatedBy" $onBehalfOfIdentity)
                }

                $createWorkItemBody = ConvertTo-AzureDevOpsRequestBody $createWorkItemParameters -Depth 3
                $createWorkItemUri = "$azureDevOpsWitApiBaseUri/workitems/`$bug?api-version=5.1$validateOnlyString"

                try
                {
                    $response = Invoke-RestMethod $createWorkItemUri -Method Post -Headers $azureDevOpsRestPatchApiHeaders -Body $createWorkItemBody

                    # Now we'll associate any file attachments with the work item we've created.
                    foreach ($attachmentUrl in $attachmentUrls)
                    {
                        $updateParameters = @(
                            @{
                                "op" = "add"
                                "path" = "/relations/-"
                                "value" = @{
                                    "rel" = "AttachedFile"
                                    "url" = $attachmentUrl
                                    "attributes" = @{
                                        "comment" = "Attachment for GitHub issue $($gitHubIssue.number)"
                                    }
                                }
                            }
                        )
                        
                        $updateWorkItemUri = "$azureDevOpsWitApiBaseUri/workitems/$($response.id)?api-version=5.1$validateOnlyString"
                        $updateResponse = Invoke-RestMethod $updateWorkItemUri -Method Patch -Headers $azureDevOpsRestPatchApiHeaders -Body (ConvertTo-AzureDevOpsRequestBody $updateParameters -Depth 3)
                    }
                }
                catch
                {
                    $exception = $_
                    $createSucceeded = $false
                }
            }
            
            if ($createSucceeded)
            {
                Write-Host @"
Created Azure DevOps $($response.fields.'System.WorkItemType'.ToLower()): ${issueTitle}
  GitHub issue: https://github.com/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)
  Azure DevOps $($response.fields.'System.WorkItemType'.ToLower()): $azureDevOpsWorkItemWebBaseUri/$($response.id)
"@
            }
            else
            {
                Write-Error @"
Failed to create Azure DevOps bug: ${issueTitle}
  GitHub issue: https://github.com/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)
  Azure DevOps bug: (none)
  Error message: $exception
"@
            }
        }
    }
    else
    {
        # Any Azure DevOps work items that did not have an associated GitHub issue should have the MirroredFromGitHub tag removed,
        # as it would not appear to come from the list of mirrored GitHub bugs that we're tracking, and either the tag was added in error
        # or the GitHub issue had a label added to remove it from mirroring consideration (e.g. "feature proposal").
        $issueNumber = $azureDevOpsWorkItem.fields.'Microsoft.VSTS.Common.CustomString09' -ireplace "[^0-9]", ""

        # Let's try to find the GitHub issue from the stored number in the Azure DevOps work item.

        try
        {
            $gitHubIssue = Invoke-RestMethod "https://api.github.com/repos/microsoft/microsoft-ui-xaml/issues/$issueNumber" -Headers $gitHubRestApiHeaders
        }
        catch
        {
            $gitHubIssue = $null
        }

        $excludingLabel = $null

        if ($gitHubIssue -and
            -not $gitHubIssue.message -and
            $gitHubIssue.message -ine "Not Found")
        {
            foreach ($label in $gitHubIssue.labels)
            {
                foreach ($excludedLabel in $excludedLabels)
                {
                    if ($label.name -ieq $excludedLabel -and -not $transientLabels.Contains($excludedLabel))
                    {
                        $excludingLabel = $excludedLabel
                        break
                    }
                }

                if ($excludingLabel)
                {
                    break
                }
            }
            
            # If there was a GitHub issue found and no non-transient excluding label was applied, then we'll leave the Azure DevOps item alone.
            # This can happen, for example, when "needs-triage" is transiently reapplied to an issue.
            if (-not $excludingLabel)
            {
                continue
            }
        }

        $changesMade = $true
            
        $updateStateRequestBody = ConvertTo-AzureDevOpsRequestBody @(
            (Get-WorkItemParameter "System.Tags" $azureDevOpsWorkItem.fields.'System.Tags'.Replace("MirroredFromGitHub", "") "replace"))

        $updateWorkItemUri = "$azureDevOpsWitApiBaseUri/workitems/$($azureDevOpsWorkItem.id)?api-version=5.1$validateOnlyString"
        $updateSucceeded = $true
        $exception = $null

        try
        {
            $response = Invoke-RestMethod $updateWorkItemUri -Method Patch -Headers $azureDevOpsRestPatchApiHeaders -Body $updateStateRequestBody
        }
        catch
        {
            $exception = $_
            $updateSucceeded = $false
        }
            
        if ($updateSucceeded)
        {
            Write-Host @"
Removed MirroredFromGitHub tag from Azure DevOps $($response.fields.'System.WorkItemType'.ToLower()): $($response.fields.'System.Title')
  GitHub issue: $(if ($gitHubIssue) { "https://github.com/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)" } else { "(none)" })
  Azure DevOps $($response.fields.'System.WorkItemType'.ToLower()): $azureDevOpsWorkItemWebBaseUri/$($response.id)
"@
        }
        else
        {
            Write-Error @"
Failed to remove MirroredFromGitHub tag from Azure DevOps $($response.fields.'System.WorkItemType'.ToLower()): $($azureDevOpsWorkItem.fields.'System.Title')
  GitHub issue: $(if ($gitHubIssue) { "https://github.com/microsoft/microsoft-ui-xaml/issues/$($gitHubIssue.number)" } else { "(none)" })
  Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()): $azureDevOpsWorkItemWebBaseUri/$($azureDevOpsWorkItem.id)
  Error message: $exception
"@
        }

        $commentSucceeded = $true
        $exception = $null

        if ($excludingLabel)
        {
            $commentText = "Removing MirroredFromGitHub tag because the label `"$excludingLabel`" was added to the associated GitHub issue."
        }
        elseif (-not $gitHubIssue)
        {
            $commentText = "Removing MirroredFromGitHub tag because no associated GitHub issue was found."
        }
                    
        if ($commentText -and $CommitChanges)
        {
            try
            {
                $response = Invoke-RestMethod "$azureDevOpsWitApiBaseUri/workitems/$($azureDevOpsWorkItem.id)/comments?api-version=7.0-preview.3" -Method Post -Headers $azureDevOpsRestPostApiHeaders -Body (ConvertTo-AzureDevOpsRequestBody @{ "text" = $commentText })
            }
            catch
            {
                $exception = $_
                $commentSucceeded = $false
            }

            if (-not $commentSucceeded)
            {
                Write-Error @"
Failed to add a comment to Azure DevOps $($azureDevOpsWorkItem.fields.'System.WorkItemType'.ToLower()).
Error message: $exception
"@
            }
        }
    }
}

Write-Progress -Activity $mirroringIssuesStatus -Completed

# If we didn't have any changes to be made at all, we'll report as much.
if (-not $changesMade)
{
    Write-Host "No Azure DevOps mirroring needed."
}

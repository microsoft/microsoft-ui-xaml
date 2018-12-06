Param(
    [Parameter(Mandatory = $true)]
    [string]$SyncedToCommitId,
    [Parameter(Mandatory = $true)]
    [string]$PersonalAccessToken
)

$env:ERRORLEVEL = "0"
$LASTEXITCODE = 0

$global:osPortingBranch = "user/uxpc/DEPControlsPortingBranch"
$global:osOfficialBranch = "user/uxpc/DEPControlsPortStagingBranch"

$global:githubRestApiHeaders = @{
    "Accept"="application/vnd.github.v3+json"
}

# For usage in a basic authorization header, a personal access token must be encoded in base 64 and used as a password with an empty username.
$global:vstsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$($PersonalAccessToken)")))"
}

function Get-Commit
{
    Param(
        [Parameter(Mandatory = $true)]
        [string]$commitId
    )

    $apiCallString = "https://api.github.com/repos/Microsoft/microsoft-ui-xaml/commits/$commitId"
    
    Write-Host "Getting commit $commitId..."
    Write-Host
    Write-Host "API call: $apiCallString"
    Write-Host
    $result = Invoke-RestMethod -Uri $apiCallString -Method Get -Headers $global:githubRestApiHeaders
    Write-Host "Result:"
    Write-Host
    Write-Host ($result | Out-String)
    Write-Host

    $result
}

function Get-CommitsBetween
{
    Param(
        [Parameter(Mandatory = $true)]
        [string]$firstCommitId,
        [Parameter(Mandatory = $true)]
        [string]$lastCommitId
    )
    $syncFromDate = (Get-Commit $firstCommitId).commit.committer.date
    $syncToDate = (Get-Commit $lastCommitId).commit.committer.date

    $apiCallString = "https://api.github.com/repos/Microsoft/microsoft-ui-xaml/commits?since=$syncFromDate&until=$syncToDate"
    
    Write-Host "Getting commits between $firstCommitId and $lastCommitId..."
    Write-Host
    Write-Host "API call: $apiCallString"
    Write-Host
    $result = Invoke-RestMethod -Uri $apiCallString -Method Get -Headers $global:githubRestApiHeaders
    Write-Host "Result:"
    Write-Host
    Write-Host ($result | Out-String)
    Write-Host

    $result
}

function Get-PortingBranchPullRequests
{
    $apiCallString = "https://microsoft.visualstudio.com/defaultcollection/os/_apis/git/repositories/os/pullRequests?api-version=3.0-preview&sourceRefName=refs/heads/$($global:osPortingBranch)"
    
    Write-Host "Getting porting branch pull requests..."
    Write-Host
    Write-Host "API call: $apiCallString"
    Write-Host
    $result = Invoke-RestMethod -Uri $apiCallString -Method Get -Headers $global:vstsRestApiHeaders
    Write-Host "Result:"
    Write-Host
    Write-Host ($result | Out-String)
    Write-Host

    $result
}

function Get-PullRequestTitle
{
    return "Microsoft.UI.Xaml to Windows port ($((Get-Date -Format F)))"
}

function New-PortingBranchPullRequest
{
    $pullRequestParameters = @{
        "sourceRefName"="refs/heads/$($global:osPortingBranch)"
        "targetRefName"="refs/heads/$($global:osOfficialBranch)"
        "title"=(Get-PullRequestTitle)
        "description"=""
        "reviewers"=@()
    }

    $apiCallString = "https://microsoft.visualstudio.com/defaultcollection/os/_apis/git/repositories/os/pullRequests?api-version=3.0-preview&sourceRefName=refs/heads/$($global:osPortingBranch)"
    $body = (ConvertTo-Json $pullRequestParameters)
    
    Write-Host "Creating a new porting branch pull request..."
    Write-Host
    Write-Host "API call: $apiCallString"
    Write-Host
    Write-Host "Body: $body"
    Write-Host
    $result = Invoke-RestMethod -Uri $apiCallString -Body $body -Method Post -ContentType "application/json" -Headers $global:vstsRestApiHeaders 2>&1> $null
    Write-Host "Result:"
    Write-Host
    Write-Host ($result | Out-String)
    Write-Host

    $result
}

function Update-PortingBranchPullRequestTitle
{
    Param(
        [Parameter(Mandatory = $true)]
        [string]$pullRequestId
    )
    
    $apiCallString = "https://microsoft.visualstudio.com/defaultcollection/os/_apis/git/repositories/os/pullRequests/$($pullRequestId)?api-version=3.0-preview"
    $body = (ConvertTo-Json @{ "title"=(Get-PullRequestTitle) })
    
    Write-Host "Updating title of $pullRequestId to `"$((Get-PullRequestTitle))`"..."
    Write-Host
    Write-Host "API call: $apiCallString"
    Write-Host
    Write-Host "Body: $body"
    Write-Host
    $result = Invoke-RestMethod -Uri $apiCallString -Body $body -Method Patch -ContentType "application/json" -Headers $global:vstsRestApiHeaders
    Write-Host "Result:"
    Write-Host
    Write-Host ($result | Out-String)
    Write-Host

    $result
}

function Update-PortingBranchPullRequestDescription
{
    Param(
        [Parameter(Mandatory = $true)]
        [string]$pullRequestId,
        [Parameter(Mandatory = $true)]
        [string]$pullRequestDescription
    )
    
    $apiCallString = "https://microsoft.visualstudio.com/defaultcollection/os/_apis/git/repositories/os/pullRequests/$($pullRequestId)?api-version=3.0-preview"
    $body = (ConvertTo-Json @{ "description"=$pullRequestDescription })
    
    Write-Host "Updating body of pull request $pullRequestId to the following:"
    Write-Host
    Write-Host $pullRequestDescription
    Write-Host
    Write-Host "API call: $apiCallString"
    Write-Host
    Write-Host "Body: $body"
    Write-Host
    $result = Invoke-RestMethod -Uri $apiCallString -Body $body -Method Patch -ContentType "application/json" -Headers $global:vstsRestApiHeaders
    Write-Host "Result:"
    Write-Host
    Write-Host ($result | Out-String)
    Write-Host

    $result
}

function Get-AssociatedWorkItems
{
    Param(
        [Parameter(Mandatory = $true)]
        [string]$pullRequestId
    )
    
    $apiCallString = "https://microsoft.visualstudio.com/defaultcollection/os/_apis/git/repositories/os/pullRequests/$($pullRequestId)/workitems?api-version=3.0-preview"
    
    Write-Host "Getting work items associated with pull request $pullRequestId..."
    Write-Host
    Write-Host "API call: $apiCallString"
    Write-Host
    $result = Invoke-RestMethod $apiCallString -Headers $global:vstsRestApiHeaders
    Write-Host "Result:"
    Write-Host
    Write-Host ($result | Out-String)
    Write-Host

    $result
}

function Add-AssociatedWorkItem
{
    Param(
        [Parameter(Mandatory = $true)]
        $pullRequest,
        [Parameter(Mandatory = $true)]
        [string]$workItemId

    )
    
    $addWorkItemParameters = 
    @(
        @{
            "op"=0
            "path"="/relations/-"
            "value"=@{
                "attributes"=@{
                    "name"="Pull Request"
                }
                "rel"="ArtifactLink"
                "url"="$($pullRequest.artifactId)"
            }
        }
    )
    
    $apiCallString = "https://microsoft.visualstudio.com/defaultcollection/_apis/wit/workItems/$($workItemId)?api-version=3.0"
    $body = (ConvertTo-Json $addWorkItemParameters -Depth 3)

    Write-Host "Associating work item $workItemId with pull request..."
    Write-Host
    Write-Host "API call: $apiCallString"
    Write-Host
    Write-Host "Body: $body"
    Write-Host
    $result = Invoke-RestMethod -Uri $apiCallString -Body $body -Method Patch -ContentType "application/json-patch+json" -Headers $global:vstsRestApiHeaders
    Write-Host "Result:"
    Write-Host
    Write-Host ($result | Out-String)
    Write-Host

    $result
}

function Get-WasError
{
    return ($env:ERRORLEVEL -ne $null -and $env:ERRORLEVEL.Length -gt 0 -and $env:ERRORLEVEL -ne "0") -or ($LASTEXITCODE -ne 0)
}

function Report-FailedStep
{
    Param(
        [Parameter(Mandatory = $true)] 
        [string]$stepName
    )

    Write-Error "$stepName FAILED."
    exit 1
}

# The first thing we need to do is determine all of the commits that have been submitted to the repository from which we've synced
# between the last time we synced and the current commit we're syncing to, since we'll want to account for all of those commits
# in our commit message.
$lastSyncedToCommit = (Get-Content .lastSyncedToCommit -Raw)

Write-Host
Write-Host "Getting commits between the previous commit and the current commit..."
Write-Host

$commitList = (Get-CommitsBetween $lastSyncedToCommit $SyncedToCommitId)

Write-Host "Retrieving sync-from and sync-to dates..."
Write-Host

$syncFromDate = (Get-Commit $lastSyncedToCommit).commit.committer.date
$syncToDate = (Get-Commit $SyncedToCommitId).commit.committer.date

if ($syncFromDate.Length -eq 0)
{
    Report-FailedStep "Getting sync-from date"
}

if ($syncToDate.Length -eq 0)
{
    Report-FailedStep "Getting sync-to date"
}

Write-Host "Last synced-to commit ID: $lastSyncedToCommit (checked in $syncFromDate)"
Write-Host "New synced-to commit ID:  $SyncedToCommitId (checked in $syncToDate)"
Write-Host
Write-Host "Syncing $($commitList.count) commit(s)..."
Write-Host

if ($commitList.Count -eq 0)
{
    Write-Host "No commits to sync.  Exiting."
    exit 0
}
elseif ($commitList.Count -gt 2)
{
    # If we have more than one MUXControls commit that we're porting over
    # (we check count > 2 since we're ignore the last-synced to commit),
    # then we don't want to attach a name to the accompanying OS repo commit.
    # We'll use the generic name in that case.
    Write-Host "user.name -> `"UXP Controls Automated Porting System`""
    & git config --global user.name "UXP Controls Automated Porting System"
    Write-Host "user.email -> `"uxpc@microsoft.com`""
    & git config --global user.email "uxpc@microsoft.com"
}

[string]$commitMessage = ""
[string]$commitSeparator = [Environment]::NewLine + [Environment]::NewLine + "----------" + [Environment]::NewLine + [Environment]::NewLine
[System.Collections.Generic.List[string]]$workItemList = @()

$commitList | ForEach-Object {
    # We'll want to ignore the last-synced to commit, since that's already been accounted for.
    if ($_.sha -ne $lastSyncedToCommit)
    {
        if ($commitMessage.Length -gt 0)
        {
            $commitMessage += $commitSeparator
        }

        $commitComment = (Get-Commit $_.sha).commit.message
        $commitMessage += $commitComment
        $commitMessage += [Environment]::NewLine + [Environment]::NewLine
        $commitMessage += "Microsoft.UI.Xaml commit $($_.sha) by $($_.commit.author.name) ($($_.commit.author.email)) on $((Get-Date $_.commit.committer.date -Format F))"

        # There appears to be no API to extract work items attached to a commit (pull requests yes; commits no),
        # so we'll use a simple regex to pull that information out of the commit message, since it has a consistent form.
        foreach ($match in ([regex]"Related work items: #([0-9]+)(?:, #([0-9]+))*").Matches($commitComment))
        {
            for ($i = 1; $i -lt $match.Groups.Count; $i++)
            {
                for ($j = 0; $j -lt $match.Groups[$i].Captures.Count; $j++)
                {
                    $workItemList.Add($match.Groups[$i].Captures[$j].Value)
                }
            }
        }
    }
}

# Pull request and commit descriptions cannot be longer than 4000 characters,
# so if ours happens to be longer than that (e.g. if we had a huge number of build breaks
# causing us to have a large backlog) then we'll truncate it and indicate with an ellipsis that we did so.
if ($commitMessage.Length -gt 4000)
{
    $commitMessage = $commitMessage.Substring(0, 3997) + "..."
}

Write-Host

# Before we commit anything, we need to update the last-synced-to commit to the commit we're synced to.
Write-Host "Updating last-synced-to commit file..."
Set-Content .lastSyncedToCommit $SyncedToCommitId -Encoding Ascii -NoNewline
if (Get-WasError) { Report-FailedStep "Updating last-synced-to commit" }

# We should also ensure that we've cleaned up everything that we don't want to commit.
Write-Host "Cleaning files that should not be committed..."
& git clean -df
if (Get-WasError) { Report-FailedStep "Cleaning files that should not be committed" }

# In order to make cherry-picking of changes easier, we should commit our metadata-tracking files
# separately from the actual WUXC-related files, so we'll reset those files in order to make them
# not be added as part of the main commit.
& git reset .cachedFileHashes
& git reset .expectedEnlistmentFileHashes
if (Get-WasError) { Report-FailedStep "Resetting metadata files" }

# Now we're ready to commit.  "git commit --file -" tells Git to read the commit message from standard input,
# so we'll pipe the commit message to that in order to give Git our script-constructed commit message.
Write-Host "Committing changes..."
$commitMessage | & git commit --file -
if (Get-WasError) { Report-FailedStep "Commit" }

# The metadata-tracking file updates aren't related to the change we're porting itself,
# so we don't want to attribute that commit to the person who made the Microsoft.UI.Xaml checkin.
Write-Host "user.name -> `"UXP Controls Automated Porting System`""
& git config --global user.name "UXP Controls Automated Porting System"
Write-Host "user.email -> `"uxpc@microsoft.com`""
& git config --global user.email "uxpc@microsoft.com"

# Now we'll commit our metadata-tracking files in a separate commit.
& git add .lastSyncedToCommit
& git add .cachedFileHashes
& git add .expectedEnlistmentFileHashes
if (Get-WasError) { Report-FailedStep "Adding metadata files" }

"Updated metadata files after syncing to Microsoft.UI.Xaml commit ID $SyncedToCommitId." | & git commit --file -
if (Get-WasError) { Report-FailedStep "Committing metadata files" }

Write-Host "Pushing changes..."
& git push
if (Get-WasError) { Report-FailedStep "Push" }

$portingBranchPullRequests = Get-PortingBranchPullRequests
$pullRequestAction = "Updated"

if ($portingBranchPullRequests.count -eq 0)
{
    Write-Host "Existing pull request not found.  Creating new pull request..."
    New-PortingBranchPullRequest
    $pullRequestAction = "Created"
}

$portingBranchPullRequests = Get-PortingBranchPullRequests

if ($portingBranchPullRequests.count -eq 0)
{
    Write-Error "Could not create pull request!"
    exit 1
}
else
{
    Write-Host "Updating pull request with current date and new commit messages..."

    # No one but the build account should be making pull requests in this branch,
    # so we'll just assume that the first pull request is the one we want.
    $pullRequestId = $portingBranchPullRequests.value[0].pullRequestId
    $pullRequest = Update-PortingBranchPullRequestTitle $pullRequestId

    $pullRequestDescription = $pullRequest.description

    if ($pullRequestDescription.Length -gt 0)
    {
        $pullRequestDescription = $commitMessage + $commitSeparator + $pullRequestDescription
    }
    else
    {
        $pullRequestDescription = $commitMessage
    }

    if ($pullRequestDescription.Length -gt 4000)
    {
        $pullRequestDescription = $pullRequestDescription.Substring(0, 3997) + "..."
    }

    $pullRequest = Update-PortingBranchPullRequestDescription $pullRequestId $pullRequestDescription

    foreach ($workItemId in $workItemList)
    {
        # First we need to see if this work item is already associated with this pull request.
        $workItems = Get-AssociatedWorkItems $pullRequestId
        $isAlreadyAssociated = $false

        foreach ($workItem in $workItems.value)
        {
            if ($workItem.id -like $workItemId)
            {
                $isAlreadyAssociated = $true
            }
        }

        if (-not $isAlreadyAssociated)
        {
            Add-AssociatedWorkItem $pullRequest $workItemId
        }
    }
}

Write-Host
Write-Host "$pullRequestAction pull request: https://microsoft.visualstudio.com/OS/OS%20Team/_git/os/pullrequest/$pullRequestId#_a=overview"
Write-Host
Write-Host "Commit succeeded!"

Set-StrictMode -Version Latest

function New-GitHubPRPerfHeaders {
    param(
        [Parameter(Mandatory)][string] $Token
    )

    return @{
        Authorization = 'Bearer ' + $Token
        Accept = 'application/vnd.github+json'
        'X-GitHub-Api-Version' = '2022-11-28'
        'User-Agent' = 'WinUI-PRPerf'
    }
}

function Get-PRPerfContextFromPipeline {
    param(
        [hashtable] $Environment
    )

    if ($null -eq $Environment) {
        $Environment = @{}
        foreach ($name in @(
            'BUILD_REPOSITORY_PROVIDER',
            'BUILD_REPOSITORY_NAME',
            'SYSTEM_PULLREQUEST_PULLREQUESTNUMBER',
            'SYSTEM_PULLREQUEST_SOURCECOMMITID',
            'SYSTEM_PULLREQUEST_TARGETBRANCH'
        )) {
            $Environment[$name] = [Environment]::GetEnvironmentVariable($name)
        }
    }

    $provider = [string]$Environment['BUILD_REPOSITORY_PROVIDER']
    if ($provider -ne 'GitHub' -and $provider -ne 'GitHubEnterprise') {
        throw "PRPerf GitHub publishing requires BUILD_REPOSITORY_PROVIDER to be GitHub or GitHubEnterprise; actual value was '$provider'."
    }

    $repositoryName = [string]$Environment['BUILD_REPOSITORY_NAME']
    $repositoryParts = @($repositoryName -split '/', 2)
    if ($repositoryParts.Count -ne 2 -or
        [string]::IsNullOrWhiteSpace($repositoryParts[0]) -or
        [string]::IsNullOrWhiteSpace($repositoryParts[1])) {
        throw "BUILD_REPOSITORY_NAME must be in 'owner/repo' format for GitHub-backed PRPerf runs; actual value was '$repositoryName'."
    }

    $pullRequestNumberText = [string]$Environment['SYSTEM_PULLREQUEST_PULLREQUESTNUMBER']
    $pullRequestNumber = 0
    if (-not [int]::TryParse($pullRequestNumberText, [ref]$pullRequestNumber) -or $pullRequestNumber -le 0) {
        throw "SYSTEM_PULLREQUEST_PULLREQUESTNUMBER must be a positive integer; actual value was '$pullRequestNumberText'."
    }

    $sourceCommit = [string]$Environment['SYSTEM_PULLREQUEST_SOURCECOMMITID']
    if ($sourceCommit -notmatch '^[0-9a-fA-F]{40}$') {
        throw "SYSTEM_PULLREQUEST_SOURCECOMMITID must be a 40-character commit SHA; actual value was '$sourceCommit'."
    }

    $targetBranch = [string]$Environment['SYSTEM_PULLREQUEST_TARGETBRANCH']
    if ([string]::IsNullOrWhiteSpace($targetBranch)) {
        throw 'SYSTEM_PULLREQUEST_TARGETBRANCH must be set.'
    }

    return [pscustomobject]@{
        Owner = $repositoryParts[0]
        Repository = $repositoryParts[1]
        PullRequestNumber = $pullRequestNumber
        SourceCommit = $sourceCommit
        TargetBranch = $targetBranch
    }
}

function Get-GitHubPRPerfCommits {
    param(
        [Parameter(Mandatory)] $PullRequest
    )

    # GitHub head.sha is only valid for supersession checks. ADO GitHub PR
    # builds run refs/pull/<N>/merge, so Build.SourceVersion is the ephemeral
    # merge SHA and is the correct trial-side key for ADO build lookup.
    $pullRequestHeadCommit = [string]$PullRequest.head.sha
    return [pscustomobject]@{
        PullRequestHeadCommit = $pullRequestHeadCommit
        SourceCommit = $pullRequestHeadCommit
        TargetCommit = [string]$PullRequest.base.sha
    }
}

function Test-GitHubPRPerfRequestCurrent {
    param(
        [Parameter(Mandatory)][ValidatePattern('^[0-9a-fA-F]{40}$')][string] $ExpectedSourceCommit,
        [Parameter(Mandatory)] $CurrentPullRequest
    )

    $currentCommit = $null
    $headProperty = $CurrentPullRequest.PSObject.Properties['head']
    if ($null -ne $headProperty -and $null -ne $headProperty.Value) {
        $shaProperty = $headProperty.Value.PSObject.Properties['sha']
        if ($null -ne $shaProperty) {
            $currentCommit = $shaProperty.Value
        }
    }
    return -not [string]::IsNullOrWhiteSpace([string]$currentCommit) -and
        [string]$currentCommit -ieq $ExpectedSourceCommit
}

Export-ModuleMember -Function New-GitHubPRPerfHeaders, Get-PRPerfContextFromPipeline, Get-GitHubPRPerfCommits, Test-GitHubPRPerfRequestCurrent

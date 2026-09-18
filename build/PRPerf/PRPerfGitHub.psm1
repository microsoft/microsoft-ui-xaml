Set-StrictMode -Version Latest

# The run-perf label is the opt-in signal, and its matching rule lives in
# PRPerfRequest.psm1 so the GitHub and Azure DevOps paths cannot drift apart.
Import-Module (Join-Path $PSScriptRoot 'PRPerfRequest.psm1') -Force

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

function Test-GitHubPRPerfRequested {
    param(
        [Parameter(Mandatory)] $PullRequest
    )

    $labelsProperty = $PullRequest.PSObject.Properties['labels']
    if ($null -eq $labelsProperty -or $null -eq $labelsProperty.Value) {
        return $false
    }

    $labels = @($labelsProperty.Value | Where-Object {
        $null -ne $_ -and $null -ne $_.PSObject.Properties['name']
    })
    if ($labels.Count -eq 0) {
        return $false
    }

    return Test-PRPerfLabel -Labels $labels
}


function ConvertTo-GitHubPRPerfStatusState {
    <#
        The PR performance stage is informational: it must never present as a failed check.
        A 'failure' or 'error' commit status renders red on the pull request and is one
        branch-protection toggle away from blocking merges, so every outcome reports
        'success' and the actual verdict is carried in the comment body.
    #>
    param([string] $OverallState)

    return 'success'
}
function Format-GitHubPRPerfErrorDetail {
    <#
        Turns a failed GitHub API response into a line a human can act on. The gate
        previously logged only the exception message, which for Invoke-RestMethod is
        "(403) Forbidden" with no body: that cannot distinguish a token never
        authorized for the organization's single sign-on from an exhausted rate
        limit, and those need opposite fixes.

        The result is written to a pipeline log that is readable by anyone who can
        see the build, so it deliberately reads only GitHub's own message plus
        non-secret rate-limit and SSO headers. Request headers, which carry the
        bearer token, are never echoed.
    #>
    param(
        [int] $StatusCode,
        [string] $Body,
        [hashtable] $ResponseHeaders
    )

    $parts = @("HTTP $StatusCode")

    $message = $null
    if (-not [string]::IsNullOrWhiteSpace($Body)) {
        try {
            $message = ($Body | ConvertFrom-Json).message
        } catch {
            # A non-JSON body is still worth reporting, but only the leading portion:
            # GitHub serves an HTML error page in some failure modes.
            $message = $Body.Trim()
            if ($message.Length -gt 200) { $message = $message.Substring(0, 200) }
        }
    }
    if (-not [string]::IsNullOrWhiteSpace($message)) {
        $parts += $message
    }

    if ($null -ne $ResponseHeaders) {
        # Header lookup is explicit rather than case-insensitive indexing because the
        # caller may pass an ordinary hashtable, which is case-sensitive by default.
        $lookup = @{}
        foreach ($key in $ResponseHeaders.Keys) {
            $lookup[[string]$key.ToString().ToLowerInvariant()] = $ResponseHeaders[$key]
        }

        if ($lookup.ContainsKey('x-github-sso')) {
            $parts += "the token has not been authorized for the organization's single sign-on: $($lookup['x-github-sso'])"
        }

        if ($lookup.ContainsKey('x-ratelimit-remaining')) {
            $remaining = $lookup['x-ratelimit-remaining']
            $limit = if ($lookup.ContainsKey('x-ratelimit-limit')) { $lookup['x-ratelimit-limit'] } else { '?' }
            $parts += "rate limit $remaining/$limit remaining"
        }
    }

    return ($parts -join ' | ')
}

function Get-GitHubPRPerfErrorDetailFromRecord {
    <#
        Extracts whatever diagnosis an error record can offer. Not every failure on
        this path is an HTTP error -- DNS failures and proxy blocks arrive with no
        Response at all -- so reaching for the response must never itself throw, or
        a network problem would surface as a property-access error instead of the
        real cause.
    #>
    param($ErrorRecord)

    $detail = $ErrorRecord.Exception.Message
    $response = $null
    try { $response = $ErrorRecord.Exception.Response } catch { }
    if ($null -eq $response) {
        return $detail
    }

    $body = ''
    $responseHeaders = @{}
    try { $body = (New-Object System.IO.StreamReader($response.GetResponseStream())).ReadToEnd() } catch { }
    try { foreach ($name in $response.Headers.AllKeys) { $responseHeaders[$name] = $response.Headers[$name] } } catch { }
    try { $detail = Format-GitHubPRPerfErrorDetail -StatusCode ([int]$response.StatusCode) -Body $body -ResponseHeaders $responseHeaders } catch { }

    return $detail
}

function Get-GitHubPRPerfPullRequest {
    <#
        Reads a pull request, preferring the configured token and retrying once
        without it.

        microsoft/microsoft-ui-xaml is public, so pull request labels are readable
        with no credential at all. The gate was skipping every pull request because
        the configured token is rejected with 403, which meant a credential problem
        was blocking access to data that is public anyway.

        The token is still tried first, and only first: an authenticated read has a
        far higher rate limit, so making the unauthenticated path the default would
        turn the gate intermittent once the shared quota is exhausted. This path only
        ever reads; posting the comment still requires a working token.
    #>
    param(
        [string] $Uri,
        [string] $Token,
        [scriptblock] $Invoker
    )

    if ($null -eq $Invoker) {
        $Invoker = { param($Uri, $Headers) Invoke-RestMethod -Uri $Uri -Headers $Headers -Method Get }
    }

    $anonymousHeaders = @{
        Accept = 'application/vnd.github+json'
        'X-GitHub-Api-Version' = '2022-11-28'
        'User-Agent' = 'WinUI-PRPerf'
    }

    if (-not [string]::IsNullOrWhiteSpace($Token)) {
        try {
            return & $Invoker $Uri (New-GitHubPRPerfHeaders -Token $Token)
        } catch {
            Write-Host "##vso[task.logissue type=warning]The configured GitHub token was rejected reading $Uri ($(Get-GitHubPRPerfErrorDetailFromRecord -ErrorRecord $_)). Retrying without it, which is sufficient for a public repository."
        }
    }

    return & $Invoker $Uri $anonymousHeaders
}

function Test-GitHubPRPerfCommitIsAncestor {
    <#
        Answers whether Candidate is contained in Descendant's history.

        The perf job checks out with fetchDepth 1, so the local clone cannot answer
        an ancestry question. GitHub's compare endpoint can, without any history:
        compare/<Candidate>...<Descendant> reports Descendant relative to Candidate,
        so "ahead" means Descendant already contains Candidate.

        A failure answers no. Treating an unanswerable comparison as yes could pick a
        main build carrying work this pull request has never seen and then blame its
        cost on this author.
    #>
    param(
        [Parameter(Mandatory)][string] $Candidate,
        [Parameter(Mandatory)][string] $Descendant,
        [Parameter(Mandatory)] $Headers,
        [Parameter(Mandatory)][string] $Repository,
        [scriptblock] $Invoke
    )

    if ($null -eq $Invoke) {
        $Invoke = { param($Uri, $RequestHeaders) Invoke-RestMethod -Uri $Uri -Headers $RequestHeaders -Method Get }
    }

    $uri = "https://api.github.com/repos/$Repository/compare/$Candidate...$Descendant"
    try {
        $comparison = & $Invoke $uri $Headers
    } catch {
        Write-Host "##vso[task.logissue type=warning]Could not compare $Candidate with $Descendant ($(Get-GitHubPRPerfErrorDetailFromRecord -ErrorRecord $_)). Treating it as not an ancestor."
        return $false
    }

    $status = [string]$comparison.status
    return $status -eq 'ahead' -or $status -eq 'identical'
}

Export-ModuleMember -Function New-GitHubPRPerfHeaders, Get-PRPerfContextFromPipeline, Get-GitHubPRPerfCommits, Test-GitHubPRPerfRequestCurrent, Test-GitHubPRPerfRequested, ConvertTo-GitHubPRPerfStatusState, Format-GitHubPRPerfErrorDetail, Get-GitHubPRPerfErrorDetailFromRecord, Get-GitHubPRPerfPullRequest, Test-GitHubPRPerfCommitIsAncestor



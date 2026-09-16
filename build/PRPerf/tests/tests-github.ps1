$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

function Assert-GitHubEqual($Expected, $Actual, [string] $Message) {
    if ($Expected -ne $Actual) {
        throw "$Message Expected='$Expected' Actual='$Actual'"
    }
}

function Get-GitHubFixture([string] $Name) {
    Get-Content -LiteralPath (Join-Path $root "fixtures\$Name") -Raw | ConvertFrom-Json
}

function Get-GitHubCommentPageFixture([string] $Name) {
    [object[]] $comments = Get-Content -LiteralPath (Join-Path $root "fixtures\$Name") -Raw | ConvertFrom-Json
    foreach ($comment in $comments) {
        $comment
    }
}

function Import-GitHubModule {
    Import-Module (Join-Path $root 'PRPerfGitHub.psm1') -Force
}

function New-GitHubComparisonFile {
    param([string] $OverallState = 'Passed')

    $path = Join-Path $PSScriptRoot "github-comparison-$([guid]::NewGuid()).json"
    [pscustomobject]@{
        schemaVersion = 1
        overallState = $OverallState
        target = [pscustomobject]@{ commit = ('b' * 40); buildId = '200' }
        trial = [pscustomobject]@{ commit = ('a' * 40); buildId = '100' }
        issues = @()
        scenarios = @(
            [pscustomobject]@{
                name = 'ObjectCreation.BasicControls'
                metrics = @(
                    [pscustomobject]@{
                        name = 'CpuTimeMs'
                        unit = 'ms'
                        target = [pscustomobject]@{ Median = 100.0; CoefficientOfVariation = 0.01 }
                        trial = [pscustomobject]@{ Median = 101.0; CoefficientOfVariation = 0.01 }
                        absoluteDelta = 1.0
                        percentDelta = 1.0
                        classification = 'Passed'
                    }
                )
            }
        )
    } | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $path -Encoding UTF8
    return $path
}

function Invoke-GitHubPublisher {
    param(
        [object[]] $Comments = @(),
        [object] $CommentPages,
        [string] $ExpectedSourceCommit,
        [string] $CurrentHeadSha = ('a' * 40)
    )

    $comparisonPath = New-GitHubComparisonFile
    $global:GitHubRestCalls = @()
    $global:GitHubMockComments = $Comments
    $global:GitHubMockCommentPages = $CommentPages
    $global:GitHubCurrentHeadSha = $CurrentHeadSha
    try {
        function global:Invoke-RestMethod {
            param(
                [string] $Method,
                [string] $Uri,
                $Headers,
                [string] $Body,
                [string] $ContentType
            )

            $global:GitHubRestCalls += [pscustomobject]@{
                Method = $Method
                Uri = $Uri
                Headers = $Headers
                Body = $Body
                ContentType = $ContentType
            }
            if ($Method -eq 'Get' -and $Uri -like '*/pulls/12') {
                return [pscustomobject]@{
                    head = [pscustomobject]@{ sha = $global:GitHubCurrentHeadSha }
                    base = [pscustomobject]@{ sha = ('b' * 40) }
                }
            }
            if ($Method -eq 'Get' -and $Uri -like '*/issues/12/comments*') {
                if ($null -ne $global:GitHubMockCommentPages) {
                    $pageNumber = 1
                    if ($Uri -match '[?&]page=([0-9]+)') {
                        $pageNumber = [int]$Matches[1]
                    }
                    return @($global:GitHubMockCommentPages[$pageNumber - 1])
                }
                return $global:GitHubMockComments
            }
            return [pscustomobject]@{ id = 1 }
        }

        $publisherArguments = @{
            Owner = 'microsoft'
            Repository = 'microsoft-ui-xaml'
            PullRequestNumber = 12
            ComparisonPath = $comparisonPath
            ArtifactUrl = 'https://artifacts'
            PipelineUrl = 'https://pipeline'
            Token = 'secret-token'
        }
        if (-not [string]::IsNullOrWhiteSpace($ExpectedSourceCommit)) {
            $publisherArguments.ExpectedSourceCommit = $ExpectedSourceCommit
        }
        & (Join-Path $root 'Publish-PRPerfResultGitHub.ps1') @publisherArguments
        return @($global:GitHubRestCalls)
    } finally {
        Remove-Item function:global:Invoke-RestMethod -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $comparisonPath -Force -ErrorAction SilentlyContinue
    }
}

function Test-GitHubHeadersUseRealBearerToken {
    Import-GitHubModule
    $headers = New-GitHubPRPerfHeaders -Token 'secret-token'

    Assert-GitHubEqual 'Bearer secret-token' $headers.Authorization 'GitHub Authorization header mismatch.'
    if ($headers.Authorization -eq ('*' * 6)) {
        throw 'GitHub Authorization header must not be the redacted placeholder.'
    }
    Assert-GitHubEqual 'application/vnd.github+json' $headers.Accept 'GitHub Accept header mismatch.'
    Assert-GitHubEqual '2022-11-28' $headers.'X-GitHub-Api-Version' 'GitHub API version header mismatch.'
    if ([string]::IsNullOrWhiteSpace([string]$headers.'User-Agent')) {
        throw 'GitHub User-Agent header is required.'
    }
}

function Assert-GitHubCallsUseBearerToken {
    param([Parameter(Mandatory)][object[]] $Calls)

    foreach ($call in $Calls) {
        Assert-GitHubEqual 'Bearer secret-token' $call.Headers.Authorization "GitHub outbound request Authorization header mismatch for $($call.Method) $($call.Uri)."
        if ($call.Headers.Authorization -eq ('*' * 6)) {
            throw "GitHub outbound request used the literal redacted placeholder for $($call.Method) $($call.Uri)."
        }
    }
}

function Test-GitHubContextParsesAdoPipelineVariables {
    Import-GitHubModule
    $context = Get-PRPerfContextFromPipeline -Environment @{
        BUILD_REPOSITORY_PROVIDER = 'GitHub'
        BUILD_REPOSITORY_NAME = 'microsoft/microsoft-ui-xaml'
        SYSTEM_PULLREQUEST_PULLREQUESTNUMBER = '123'
        SYSTEM_PULLREQUEST_SOURCECOMMITID = ('a' * 40)
        SYSTEM_PULLREQUEST_TARGETBRANCH = 'refs/heads/main'
    }

    Assert-GitHubEqual 'microsoft' $context.Owner 'GitHub owner mismatch.'
    Assert-GitHubEqual 'microsoft-ui-xaml' $context.Repository 'GitHub repository mismatch.'
    Assert-GitHubEqual 123 $context.PullRequestNumber 'GitHub pull request number mismatch.'
    Assert-GitHubEqual ('a' * 40) $context.SourceCommit 'GitHub source commit mismatch.'
    Assert-GitHubEqual 'refs/heads/main' $context.TargetBranch 'GitHub target branch mismatch.'
}

function Test-GitHubContextRejectsNonGitHubProvider {
    Import-GitHubModule
    $message = $null
    try {
        Get-PRPerfContextFromPipeline -Environment @{
            BUILD_REPOSITORY_PROVIDER = 'TfsGit'
            BUILD_REPOSITORY_NAME = 'project/repo'
            SYSTEM_PULLREQUEST_PULLREQUESTNUMBER = '123'
            SYSTEM_PULLREQUEST_SOURCECOMMITID = ('a' * 40)
            SYSTEM_PULLREQUEST_TARGETBRANCH = 'refs/heads/main'
        } | Out-Null
    } catch {
        $message = $_.Exception.Message
    }

    if ($message -notlike "*GitHub*GitHubEnterprise*") {
        throw "Expected a clear non-GitHub provider error; got '$message'."
    }
}

function Test-GitHubContextSplitsOwnerAndRepository {
    Import-GitHubModule
    $context = Get-PRPerfContextFromPipeline -Environment @{
        BUILD_REPOSITORY_PROVIDER = 'GitHubEnterprise'
        BUILD_REPOSITORY_NAME = 'owner-name/repo-name'
        SYSTEM_PULLREQUEST_PULLREQUESTNUMBER = '42'
        SYSTEM_PULLREQUEST_SOURCECOMMITID = ('c' * 40)
        SYSTEM_PULLREQUEST_TARGETBRANCH = 'refs/heads/winui3/main'
    }

    Assert-GitHubEqual 'owner-name' $context.Owner 'GitHub Enterprise owner mismatch.'
    Assert-GitHubEqual 'repo-name' $context.Repository 'GitHub Enterprise repository mismatch.'
}

function Test-GitHubCommitExtractionUsesHeadAndBaseSha {
    Import-GitHubModule
    $pr = Get-GitHubFixture 'github-pr.json'

    $commits = Get-GitHubPRPerfCommits -PullRequest $pr

    Assert-GitHubEqual ('a' * 40) $commits.SourceCommit 'Source commit must come from head.sha.'
    Assert-GitHubEqual ('a' * 40) $commits.PullRequestHeadCommit 'PullRequestHeadCommit must make head.sha supersession-only usage explicit.'
    Assert-GitHubEqual ('b' * 40) $commits.TargetCommit 'Target commit must come from base.sha.'
}

function Test-GitHubSupersededPrDetected {
    Import-GitHubModule
    $current = [pscustomobject]@{ head = [pscustomobject]@{ sha = ('c' * 40) } }

    Assert-GitHubEqual $false (Test-GitHubPRPerfRequestCurrent -ExpectedSourceCommit ('a' * 40) -CurrentPullRequest $current) 'Moved GitHub PR head must be superseded.'
}

function Test-GitHubPublisherCreatesCommentWhenMarkerAbsent {
    $calls = Invoke-GitHubPublisher

    Assert-GitHubEqual 3 $calls.Count 'GitHub publisher request count mismatch.'
    Assert-GitHubEqual 'Get' $calls[0].Method 'GitHub publisher must list comments first.'
    Assert-GitHubEqual 'Post' $calls[1].Method 'GitHub publisher must create a comment when marker is absent.'
    if ($calls[1].Uri -notlike 'https://api.github.com/repos/microsoft/microsoft-ui-xaml/issues/12/comments') {
        throw "GitHub create-comment URL mismatch: $($calls[1].Uri)"
    }
    $body = $calls[1].Body | ConvertFrom-Json
    if ($body.body -notlike '*<!-- winui-pr-perf-result -->*') {
        throw 'Created GitHub comment must include the stable marker.'
    }
}

function Test-GitHubPublisherUpdatesCommentWhenMarkerFound {
    $comments = @([pscustomobject]@{ id = 99; body = '<!-- winui-pr-perf-result --> old' })

    $calls = Invoke-GitHubPublisher -Comments $comments

    Assert-GitHubEqual 3 $calls.Count 'GitHub publisher request count mismatch.'
    Assert-GitHubEqual 'Patch' $calls[1].Method 'GitHub publisher must update the marked comment.'
    if ($calls[1].Uri -notlike 'https://api.github.com/repos/microsoft/microsoft-ui-xaml/issues/comments/99') {
        throw "GitHub update-comment URL mismatch: $($calls[1].Uri)"
    }
}

function Test-GitHubPublisherUpdatesMarkerOnSecondCommentPage {
    $page1 = @(Get-GitHubCommentPageFixture 'github-comments-page-1.json')
    $page2 = @(Get-GitHubCommentPageFixture 'github-comments-page-2.json')

    $commentPages = New-Object object[] 2
    $commentPages[0] = $page1
    $commentPages[1] = $page2

    $calls = Invoke-GitHubPublisher -CommentPages $commentPages

    Assert-GitHubEqual 4 $calls.Count 'GitHub publisher must request page 2 before updating the marked comment.'
    Assert-GitHubEqual 'Get' $calls[0].Method 'GitHub publisher must list comments page 1 first.'
    if ($calls[0].Uri -notlike '*per_page=100&page=1') {
        throw "GitHub first comments page URL mismatch: $($calls[0].Uri)"
    }
    Assert-GitHubEqual 'Get' $calls[1].Method 'GitHub publisher must list comments page 2 when page 1 is full.'
    if ($calls[1].Uri -notlike '*per_page=100&page=2') {
        throw "GitHub second comments page URL mismatch: $($calls[1].Uri)"
    }
    Assert-GitHubEqual 'Patch' $calls[2].Method 'GitHub publisher must update the marked comment found on page 2.'
    if ($calls[2].Uri -notlike 'https://api.github.com/repos/microsoft/microsoft-ui-xaml/issues/comments/321') {
        throw "GitHub update-comment URL mismatch for paged marker: $($calls[2].Uri)"
    }
    if (($calls | Where-Object { $_.Method -eq 'Post' -and $_.Uri -like '*/issues/12/comments' }).Count -ne 0) {
        throw 'GitHub publisher must not create a duplicate comment when the marker is found on page 2.'
    }
}

function Test-GitHubPublisherUsesAccessTokenForAllOutboundRequests {
    $page1 = @(Get-GitHubCommentPageFixture 'github-comments-page-1.json')
    $page2 = @(Get-GitHubCommentPageFixture 'github-comments-page-2.json')

    $commentPages = New-Object object[] 2
    $commentPages[0] = $page1
    $commentPages[1] = $page2

    $calls = Invoke-GitHubPublisher -CommentPages $commentPages -ExpectedSourceCommit ('a' * 40)

    Assert-GitHubEqual 5 $calls.Count 'GitHub publisher request count mismatch for PR refetch, paged comments, update, and status.'
    Assert-GitHubCallsUseBearerToken -Calls $calls
}

function Test-GitHubPublisherMarksSupersededResultInconclusiveAndNotPassed {
    $calls = Invoke-GitHubPublisher -ExpectedSourceCommit ('a' * 40) -CurrentHeadSha ('c' * 40)

    Assert-GitHubEqual 4 $calls.Count 'GitHub superseded publisher request count mismatch.'
    $commentBody = ($calls | Where-Object { $_.Method -eq 'Post' -and $_.Uri -like '*/issues/12/comments' })[0].Body | ConvertFrom-Json
    $commentLines = @($commentBody.body -split "`n" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) -and $_ -notlike '<!--*-->' })
    Assert-GitHubEqual '## Superseded by a newer PR commit' $commentLines[0] 'Superseded GitHub comments must lead with a superseded heading.'
    if ($commentLines[0] -like '*passed*') {
        throw 'Superseded GitHub comments must not present a passing heading as the leading heading.'
    }
    if ($commentBody.body -notlike '*Artifacts*https://artifacts*' -or $commentBody.body -notlike '*Pipeline run*https://pipeline*') {
        throw 'Superseded GitHub comments must retain artifact and pipeline links.'
    }

    $statusBody = ($calls | Where-Object { $_.Method -eq 'Post' -and $_.Uri -like '*/statuses/*' })[0].Body | ConvertFrom-Json
    Assert-GitHubEqual 'error' $statusBody.state 'Superseded GitHub status must use the Inconclusive state mapping.'
    if ($statusBody.state -eq 'success') {
        throw 'Superseded GitHub status must not be published as success.'
    }
    Assert-GitHubEqual 'Superseded by a newer commit - perf results no longer apply.' $statusBody.description 'Superseded GitHub status description mismatch.'
}


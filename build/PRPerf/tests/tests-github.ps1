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
    # The stage is informational, so the commit status is always 'success' and must never
    # render as a failed check. The superseded verdict is carried by the comment and the
    # status description instead, which is what a human actually reads.
    Assert-GitHubEqual 'success' $statusBody.state 'An informational stage must always publish a success status.'
    if ($statusBody.description -like '*passed*') {
        throw 'Superseded GitHub status description must not claim a pass.'
    }
    Assert-GitHubEqual 'Superseded by a newer commit - perf results no longer apply.' $statusBody.description 'Superseded GitHub status description mismatch.'

    # Guards the original defect: the body below the superseded heading previously still
    # contained the full green "passed" report for data already known to be untrustworthy.
    if ($commentBody.body -like '*Perf regression test passed*') {
        throw 'Superseded GitHub comments must not still contain the passing report body.'
    }
}


function New-GitHubLabeledPullRequest {
    param([object[]] $LabelNames)

    $labels = @()
    foreach ($name in $LabelNames) {
        $labels += [pscustomobject]@{ name = $name }
    }
    return [pscustomobject]@{
        head = [pscustomobject]@{ sha = ('a' * 40) }
        base = [pscustomobject]@{ sha = ('b' * 40) }
        labels = $labels
    }
}

function Test-GitHubRunPerfLabelRequestsAPerfRun {
    Import-GitHubModule
    $pullRequest = New-GitHubLabeledPullRequest -LabelNames @('run-perf')
    Assert-GitHubEqual $true (Test-GitHubPRPerfRequested -PullRequest $pullRequest) 'The run-perf label must request a perf run.'
}

function Test-GitHubRunPerfLabelMatchIsCaseInsensitive {
    Import-GitHubModule
    $pullRequest = New-GitHubLabeledPullRequest -LabelNames @('Run-Perf')
    Assert-GitHubEqual $true (Test-GitHubPRPerfRequested -PullRequest $pullRequest) 'The run-perf label must match case-insensitively.'
}

function Test-GitHubRunPerfLabelIsFoundAmongOtherLabels {
    Import-GitHubModule
    $pullRequest = New-GitHubLabeledPullRequest -LabelNames @('bug', 'run-perf', 'area-Scroller')
    Assert-GitHubEqual $true (Test-GitHubPRPerfRequested -PullRequest $pullRequest) 'The run-perf label must be found alongside unrelated labels.'
}

function Test-GitHubMissingRunPerfLabelSkipsPerfRun {
    Import-GitHubModule
    $pullRequest = New-GitHubLabeledPullRequest -LabelNames @('bug', 'area-Scroller')
    Assert-GitHubEqual $false (Test-GitHubPRPerfRequested -PullRequest $pullRequest) 'A pull request without the run-perf label must not request a perf run.'
}

function Test-GitHubEmptyLabelCollectionSkipsPerfRun {
    Import-GitHubModule
    $pullRequest = New-GitHubLabeledPullRequest -LabelNames @()
    Assert-GitHubEqual $false (Test-GitHubPRPerfRequested -PullRequest $pullRequest) 'A pull request with no labels must not request a perf run.'
}

function Test-GitHubAbsentLabelsPropertySkipsPerfRun {
    Import-GitHubModule
    $pullRequest = [pscustomobject]@{ head = [pscustomobject]@{ sha = ('a' * 40) } }
    Assert-GitHubEqual $false (Test-GitHubPRPerfRequested -PullRequest $pullRequest) 'A pull request payload without a labels property must not request a perf run.'
}

function Test-GitHubNullLabelsPropertySkipsPerfRun {
    Import-GitHubModule
    $pullRequest = [pscustomobject]@{ labels = $null }
    Assert-GitHubEqual $false (Test-GitHubPRPerfRequested -PullRequest $pullRequest) 'A pull request with a null labels property must not request a perf run.'
}

function Test-GitHubLabelsResemblingRunPerfDoNotRequestAPerfRun {
    Import-GitHubModule
    foreach ($name in @('run-perf-2', 'runperf', 'perf', 'no-run-perf', 'run perf')) {
        $pullRequest = New-GitHubLabeledPullRequest -LabelNames @($name)
        Assert-GitHubEqual $false (Test-GitHubPRPerfRequested -PullRequest $pullRequest) "Label '$name' must not be treated as the run-perf label."
    }
}

function Test-GitHubStatusIsNeverFailingBecauseTheStageIsInformational {
    # The perf stage is informational and must never present as a failed check on the PR: a
    # failure/error commit status renders red and is one branch-protection toggle away from
    # blocking merges. The verdict belongs in the comment body instead.
    foreach ($state in @('Passed', 'RegressionWarning', 'Inconclusive', 'Superseded', 'Anything')) {
        $actual = ConvertTo-GitHubPRPerfStatusState -OverallState $state
        if ($actual -ne 'success') {
            throw "Overall state '$state' produced GitHub status '$actual'; an informational stage must always report 'success'."
        }
    }
}


function Test-GitHubErrorDetailNamesSsoAuthorizationAs403Cause {
    Import-GitHubModule
    # The gate logged only "(403) Forbidden", which cannot distinguish a token that
    # was never SSO-authorized for the org from an exhausted rate limit. Those need
    # opposite fixes, so the cause has to reach the log.
    $detail = Format-GitHubPRPerfErrorDetail -StatusCode 403 `
        -Body '{"message":"Resource protected by organization SAML enforcement.","documentation_url":"https://docs.github.com/rest"}' `
        -ResponseHeaders @{ 'X-GitHub-SSO' = 'required; url=https://github.com/orgs/microsoft/sso' }

    if ($detail -notmatch '403') { throw "Detail must report the status code. Got: $detail" }
    if ($detail -notmatch 'SAML enforcement') { throw "Detail must include GitHub's own message. Got: $detail" }
    if ($detail -notmatch 'single sign-on') { throw "Detail must name SSO authorization as the cause. Got: $detail" }
}

function Test-GitHubErrorDetailNamesRateLimitAs403Cause {
    Import-GitHubModule
    # The other 403: an unauthenticated or exhausted token. Reporting the remaining
    # quota distinguishes it from the SSO case without guesswork.
    $detail = Format-GitHubPRPerfErrorDetail -StatusCode 403 `
        -Body '{"message":"API rate limit exceeded for 20.1.2.3."}' `
        -ResponseHeaders @{ 'X-RateLimit-Remaining' = '0'; 'X-RateLimit-Limit' = '60' }

    if ($detail -notmatch 'rate limit') { throw "Detail must name the rate limit. Got: $detail" }
    if ($detail -notmatch '0/60') { throw "Detail must report remaining quota. Got: $detail" }
}

function Test-GitHubErrorDetailNeverEchoesTheToken {
    Import-GitHubModule
    # This string is written to a public pipeline log, so it must carry diagnosis
    # and nothing else. A leaked credential would be far worse than a silent gate.
    $detail = Format-GitHubPRPerfErrorDetail -StatusCode 401 `
        -Body '{"message":"Bad credentials"}' `
        -ResponseHeaders @{ Authorization = 'Bearer ghp_supersecretvalue'; 'X-GitHub-SSO' = 'partial-results' }

    if ($detail -match 'ghp_supersecretvalue') { throw "Detail leaked the token: $detail" }
    if ($detail -match '(?i)authorization') { throw "Detail must not echo the Authorization header. Got: $detail" }
    if ($detail -notmatch 'Bad credentials') { throw "Detail must include GitHub's message. Got: $detail" }
}

function Test-GateReportsWhetherATokenWasSupplied {
    # An unset variable group and a rejected token both end in "skipping", but only
    # one is fixed by authorizing a token. The log must say which.
    $yaml = Get-Content (Join-Path $PSScriptRoot '..\..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw
    $gateStart = $yaml.IndexOf('name: gate')
    $gateEnd = $yaml.IndexOf('- job: RunPRPerf')
    $gate = $yaml.Substring($gateStart, $gateEnd - $gateStart)

    if ($gate -notmatch 'Format-GitHubPRPerfErrorDetail') {
        throw 'The gate must report GitHub failure detail instead of only the exception message.'
    }
    if ($gate -notmatch '(?i)token') {
        throw 'The gate must report whether a token was supplied.'
    }
}

function Test-PullRequestReadFallsBackToUnauthenticatedOnAuthFailure {
    Import-GitHubModule
    # microsoft/microsoft-ui-xaml is public, so reading labels needs no credential.
    # The stage was skipping every pull request because the configured token is
    # rejected with 403, which is a credential problem standing in the way of data
    # that is public anyway. No write ever uses this path.
    $attempts = @()
    $invoker = {
        param($Uri, $Headers)
        $attempts += , $Headers
        if ($Headers.ContainsKey('Authorization')) { throw 'The remote server returned an error: (403) Forbidden.' }
        return [pscustomobject]@{ labels = @([pscustomobject]@{ name = 'run-perf' }) }
    }.GetNewClosure()

    $pr = Get-GitHubPRPerfPullRequest -Uri 'https://api.github.com/repos/o/r/pulls/1' -Token 'broken' -Invoker $invoker

    if ($null -eq $pr) { throw 'Expected the unauthenticated retry to return the pull request.' }
    if (-not (Test-GitHubPRPerfRequested -PullRequest $pr)) { throw 'Expected the retried read to expose the label.' }
}

function Test-PullRequestReadPrefersTheTokenWhenItWorks {
    Import-GitHubModule
    # The fallback must not become the normal path: an authenticated read has a far
    # higher rate limit, and silently dropping the credential would make the gate
    # fail intermittently once the shared unauthenticated quota is exhausted.
    $sawAuthorization = $false
    $invoker = {
        param($Uri, $Headers)
        if ($Headers.ContainsKey('Authorization')) { $script:sawAuthorization = $true }
        return [pscustomobject]@{ labels = @() }
    }

    $null = Get-GitHubPRPerfPullRequest -Uri 'https://api.github.com/repos/o/r/pulls/1' -Token 'good' -Invoker $invoker
    if (-not $script:sawAuthorization) { throw 'The first attempt must use the token.' }
}

function Test-PullRequestReadThrowsWhenBothAttemptsFail {
    Import-GitHubModule
    # A gate that cannot read labels must skip loudly, never assume the label is
    # present. Swallowing this would run perf on every pull request.
    $invoker = { param($Uri, $Headers) throw 'network down' }

    $threw = $false
    $errorRecord = $null
    try { $null = Get-GitHubPRPerfPullRequest -Uri 'https://api.github.com/repos/o/r/pulls/1' -Token 'broken' -Invoker $invoker }
    catch { $threw = $true; $errorRecord = $_ }

    if (-not $threw) { throw 'Expected a total failure to surface to the caller.' }
    # Without this the test passes while the function does not exist at all, since a
    # CommandNotFoundException is also a throw.
    if ($errorRecord.CategoryInfo.Reason -eq 'CommandNotFoundException') {
        throw 'Get-GitHubPRPerfPullRequest does not exist.'
    }
    if ($errorRecord.Exception.Message -notmatch 'network down') {
        throw "Expected the underlying failure to surface. Got: $($errorRecord.Exception.Message)"
    }
}

function Test-ErrorDetailFromRecordSurvivesAnErrorWithNoHttpResponse {
    Import-GitHubModule
    # Not every failure on this path is an HTTP error: DNS failures and proxy
    # blocks throw with no Response at all. Reaching for a missing response must
    # not itself throw, or a network problem would surface as a confusing
    # property-access error instead of the real cause.
    $record = $null
    try { throw 'network down' } catch { $record = $_ }

    $detail = Get-GitHubPRPerfErrorDetailFromRecord -ErrorRecord $record
    if ($detail -notmatch 'network down') {
        throw "Expected the original message to survive. Got: $detail"
    }
}

function Test-TokenRejectionIsReportedWithDiagnosableDetail {
    # The fallback catches the 403 before the gate's own handler can format it, so
    # without this the only thing reaching the log is "(403) Forbidden" again --
    # the exact blind spot the detail formatter was added to close.
    $module = Get-Content (Join-Path $PSScriptRoot '..\PRPerfGitHub.psm1') -Raw
    $start = $module.IndexOf('function Get-GitHubPRPerfPullRequest')
    if ($start -lt 0) { throw 'Get-GitHubPRPerfPullRequest is missing.' }
    $body = $module.Substring($start)

    if ($body -notmatch 'Get-GitHubPRPerfErrorDetailFromRecord') {
        throw 'The token-rejection warning must report formatted detail, not the bare exception message.'
    }
}

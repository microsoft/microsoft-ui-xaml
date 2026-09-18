$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Import-Module (Join-Path $root 'PRPerfResults.psm1') -Force
Import-Module (Join-Path $root 'PRPerfComment.psm1') -Force

function Assert-Equal($Expected, $Actual, [string] $Message) {
    if ($Expected -ne $Actual) {
        throw "$Message Expected='$Expected' Actual='$Actual'"
    }
}

function Test-MarkdownContainsStableMarkerAndState {
    $comparison = Compare-PRPerfFiles `
        -TargetPath (Join-Path $root 'fixtures\target-pass.json') `
        -TrialPath (Join-Path $root 'fixtures\trial-regression.json') `
        -ThresholdPath (Join-Path $root 'pr-perf-thresholds.json')
    $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'
    if ($markdown -notlike '*<!-- winui-pr-perf-result -->*') { throw 'Stable marker missing.' }
    if ($markdown -notlike '*Regression warning*') { throw 'State heading missing.' }
    if ($markdown -notlike '*Lifecycle-MinApp.Cpp.MUX*') { throw 'Scenario row missing.' }
}

function Test-MarkdownNumbersAreInvariantAcrossCultures {
    $comparison = New-TestComparison
    $metric = $comparison.scenarios[0].metrics[0]
    $metric.target.Median = 1234.5
    $metric.trial.Median = 2345.75
    $metric.absoluteDelta = 1111.25
    $metric.percentDelta = 12.5
    $metric.target.CoefficientOfVariation = 0.0125
    $metric.trial.CoefficientOfVariation = 0.025

    $originalCulture = [System.Threading.Thread]::CurrentThread.CurrentCulture
    try {
        [System.Threading.Thread]::CurrentThread.CurrentCulture = [System.Globalization.CultureInfo]::InvariantCulture
        $invariantMarkdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'
        [System.Threading.Thread]::CurrentThread.CurrentCulture = [System.Globalization.CultureInfo]::GetCultureInfo('de-DE')
        $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'
    } finally {
        [System.Threading.Thread]::CurrentThread.CurrentCulture = $originalCulture
    }

    Assert-Equal $invariantMarkdown $markdown 'Markdown must be byte-stable across cultures.'
    if ($markdown -notlike '*| 1,234.50 ms | 2,345.75 ms | 1,111.25 ms | 12.50% | 1.25% | 2.50% |*') {
        throw "Markdown numbers were not formatted with invariant culture: $markdown"
    }
}

function Test-MarkdownEscapesUntrustedTableCells {
    $comparison = New-TestComparison
    $scenario = $comparison.scenarios[0]
    $metric = $scenario.metrics[0]
    $scenario.name = "Scenario|One`r`nTwo"
    $metric.name = "Metric|One`nTwo"
    $metric.unit = "m|s`runit"
    $metric.classification = "Pass|ed`r`nstate"

    $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'
    $expectedRow = '| Scenario\|One Two | Metric\|One Two | 251.00 m\|s unit | 253.50 m\|s unit | 2.50 m\|s unit | 1.00% | 0.49% | 0.39% | Pass\|ed state |'
    if ($markdown -notlike "*$expectedRow*") {
        throw "Markdown row did not escape table cell values. Expected='$expectedRow' Actual='$markdown'"
    }
}

function Test-MarkdownEscapesBackslashesBeforePipes {
    $comparison = New-TestComparison
    $scenario = $comparison.scenarios[0]
    $metric = $scenario.metrics[0]
    $scenario.name = 'Scenario\|One'
    $metric.name = 'Metric\|One'
    $metric.unit = 'm\|s'
    $metric.classification = 'Pass\|ed'

    $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'
    $expectedRow = '| Scenario\\\|One | Metric\\\|One | 251.00 m\\\|s | 253.50 m\\\|s | 2.50 m\\\|s | 1.00% | 0.49% | 0.39% | Pass\\\|ed |'
    $row = @($markdown -split "`n" | Where-Object { $_ -like '| Scenario*' -and $_ -notlike '| Scenario |*' })[0]
    Assert-Equal $expectedRow $row 'Markdown row must escape backslashes before pipes.'

    $delimiterCount = 0
    for ($index = 0; $index -lt $row.Length; $index++) {
        if ($row[$index] -ne '|') { continue }
        $backslashCount = 0
        for ($previous = $index - 1; $previous -ge 0 -and $row[$previous] -eq '\'; $previous--) {
            $backslashCount++
        }
        if (($backslashCount % 2) -eq 0) {
            $delimiterCount++
        }
    }
    Assert-Equal 10 $delimiterCount 'Rendered row column delimiter count mismatch.'
}

function Test-MarkdownIncludesInconclusiveIssueForMissingTargetBuild {
    $comparison = New-TestComparison -OverallState 'Inconclusive'
    $comparison.issues = @("No succeeded build with artifact 'drop_amd64fre' exists for target commit '$('b' * 40)'.")
    $comparison.scenarios = @()

    $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'

    if ($markdown -notlike "*target commit '$('b' * 40)'*") {
        throw 'Inconclusive markdown must name the missing target commit.'
    }
    if ($markdown -notlike '*Artifacts*https://artifacts*' -or $markdown -notlike '*Pipeline run*https://pipeline*') {
        throw 'Inconclusive markdown must retain artifact and pipeline links.'
    }
}

function Test-MarkdownIncludesTimeoutIssueAndLinks {
    $comparison = New-TestComparison -OverallState 'Inconclusive'
    $comparison.issues = @('Scenario timeout while running PRPerf-ObjectCreation.BasicControls.')
    $comparison.scenarios = @()

    $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'

    if ($markdown -notlike '*Scenario timeout while running PRPerf-ObjectCreation.BasicControls.*') {
        throw 'Inconclusive markdown must include the scenario timeout message.'
    }
    if ($markdown -notlike '*Artifacts*https://artifacts*' -or $markdown -notlike '*Pipeline run*https://pipeline*') {
        throw 'Scenario timeout markdown must include artifact and pipeline links.'
    }
}

function Test-FindCommentReturnsMarkedThread {
    $threads = @([pscustomobject]@{
        id = 41
        comments = @([pscustomobject]@{ id = 7; content = '<!-- winui-pr-perf-result --> old' })
    })
    $found = Find-PRPerfComment -Threads $threads
    Assert-Equal 41 $found.ThreadId 'Thread ID mismatch.'
    Assert-Equal 7 $found.CommentId 'Comment ID mismatch.'
}

function New-TestComparison {
    param([string] $OverallState = 'Passed')

    $comparison = Compare-PRPerfFiles `
        -TargetPath (Join-Path $root 'fixtures\target-pass.json') `
        -TrialPath (Join-Path $root 'fixtures\trial-pass.json') `
        -ThresholdPath (Join-Path $root 'pr-perf-thresholds.json')
    $comparison.overallState = $OverallState
    return $comparison
}

function Invoke-TestPublisher {
    param(
        [string] $OverallState = 'Passed',
        [object[]] $Threads = @(),
        [string] $ExpectedSourceCommit
    )

    $comparisonPath = Join-Path $PSScriptRoot "publisher-$([guid]::NewGuid()).json"
    $global:PRPerfRestCalls = @()
    $global:PRPerfMockThreads = $Threads
    $global:PRPerfFailStatus = $false
    try {
        New-TestComparison -OverallState $OverallState |
            ConvertTo-Json -Depth 12 |
            Set-Content -LiteralPath $comparisonPath -Encoding UTF8

        function global:Invoke-RestMethod {
            param(
                [string] $Method,
                [string] $Uri,
                $Headers,
                [string] $Body,
                [string] $ContentType
            )

            $global:PRPerfRestCalls += [pscustomobject]@{
                Method = $Method
                Uri = $Uri
                Headers = $Headers
                Body = $Body
                ContentType = $ContentType
            }
            if ($Uri -like '*/statuses?*' -and $global:PRPerfFailStatus) {
                throw 'status publication failed'
            }
            if ($Method -eq 'Get') {
                if ($Uri -like '*/pullRequests/123?api-version=7.1') {
                    return [pscustomobject]@{
                        lastMergeSourceCommit = [pscustomobject]@{ commitId = ('a' * 40) }
                    }
                }
                return [pscustomobject]@{ value = $global:PRPerfMockThreads }
            }
            return [pscustomobject]@{ id = 1 }
        }

        $publisherArguments = @{
            CollectionUri = 'https://dev.azure.com/example/'
            Project = 'WinUI'
            RepositoryId = 'repo-id'
            PullRequestId = 123
            ComparisonPath = $comparisonPath
            ArtifactUrl = 'https://artifacts'
            PipelineUrl = 'https://pipeline'
            AccessToken = 'secret-token'
        }
        if (-not [string]::IsNullOrWhiteSpace($ExpectedSourceCommit)) {
            $publisherArguments.ExpectedSourceCommit = $ExpectedSourceCommit
        }
        & (Join-Path $root 'Publish-PRPerfResult.ps1') @publisherArguments
        return @($global:PRPerfRestCalls)
    } finally {
        Remove-Item function:global:Invoke-RestMethod -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $comparisonPath -Force -ErrorAction SilentlyContinue
    }
}

function Test-PublisherUsesAccessTokenForAllRequests {
    $calls = Invoke-TestPublisher -ExpectedSourceCommit ('a' * 40)

    Assert-Equal 4 $calls.Count 'Publisher request count mismatch when refetching the PR.'
    foreach ($call in $calls) {
        Assert-Equal 'Bearer secret-token' $call.Headers.Authorization "Publisher sent the wrong Authorization header to $($call.Uri)."
        if ($call.Headers.Authorization -eq ('*' * 6)) {
            throw "Publisher sent the literal redacted placeholder to $($call.Uri)."
        }
    }
}

function Test-PublisherUsesSharedAuthorizationHeaderHelper {
    $script = Get-Content -LiteralPath (Join-Path $root 'Publish-PRPerfResult.ps1') -Raw
    if ($script -notmatch [regex]::Escape('New-PRPerfAuthorizationHeaders -AccessToken $AccessToken')) {
        throw 'Publisher must construct outbound REST headers through New-PRPerfAuthorizationHeaders.'
    }
}

function Test-PublisherUpdatesMarkedCommentAndPostsStatus {
    $calls = Invoke-TestPublisher -Threads @([pscustomobject]@{
        id = 41
        comments = @([pscustomobject]@{ id = 7; content = '<!-- winui-pr-perf-result --> old' })
    })

    Assert-Equal 3 $calls.Count 'Publisher request count mismatch.'
    Assert-Equal 'Get' $calls[0].Method 'Publisher must discover threads first.'
    Assert-Equal 'Patch' $calls[1].Method 'Publisher must update the marked comment.'
    if ($calls[1].Uri -notlike '*/threads/41/comments/7?api-version=7.1-preview.1') {
        throw "Marked comment URL mismatch: $($calls[1].Uri)"
    }
    Assert-Equal 'Post' $calls[2].Method 'Publisher must publish a status.'
    if ($calls[2].Uri -notlike '*/statuses?api-version=7.1-preview.1') {
        throw "Status URL mismatch: $($calls[2].Uri)"
    }
}

function Test-PublisherCreatesActiveThreadWhenMarkerIsAbsent {
    $calls = Invoke-TestPublisher

    Assert-Equal 3 $calls.Count 'Publisher request count mismatch.'
    Assert-Equal 'Post' $calls[1].Method 'Publisher must create a thread.'
    if ($calls[1].Uri -notlike '*/threads?api-version=7.1-preview.1') {
        throw "Thread URL mismatch: $($calls[1].Uri)"
    }
    $threadBody = $calls[1].Body | ConvertFrom-Json
    Assert-Equal 'active' $threadBody.status 'New thread must be active.'
    if ($threadBody.comments[0].content -notlike '*<!-- winui-pr-perf-result -->*') {
        throw 'New comment must contain the stable marker.'
    }
}

function Test-PublisherMapsComparisonStatesToInformationalStatuses {
    $stateCases = @(
        @{ Comparison = 'Passed'; Status = 'succeeded'; Description = 'Perf regression test passed' },
        @{ Comparison = 'RegressionWarning'; Status = 'failed'; Description = 'Performance regression warning' },
        @{ Comparison = 'Inconclusive'; Status = 'error'; Description = 'Performance result inconclusive' }
    )

    foreach ($stateCase in $stateCases) {
        $calls = Invoke-TestPublisher -OverallState $stateCase.Comparison
        $statusBody = $calls[-1].Body | ConvertFrom-Json
        Assert-Equal $stateCase.Status $statusBody.state "$($stateCase.Comparison) status mismatch."
        Assert-Equal $stateCase.Description $statusBody.description "$($stateCase.Comparison) description mismatch."
        Assert-Equal 'pr-smoke' $statusBody.context.name 'Status context name mismatch.'
        Assert-Equal 'winui-perf' $statusBody.context.genre 'Status context genre mismatch.'
        Assert-Equal 'https://pipeline' $statusBody.targetUrl 'Status target URL mismatch.'
    }
}

function Test-PublisherThrowsWhenStatusPublicationFails {
    $comparisonPath = Join-Path $PSScriptRoot "publisher-$([guid]::NewGuid()).json"
    $global:PRPerfFailStatus = $true
    $global:PRPerfRestCalls = @()
    try {
        New-TestComparison |
            ConvertTo-Json -Depth 12 |
            Set-Content -LiteralPath $comparisonPath -Encoding UTF8
        function global:Invoke-RestMethod {
            param([string] $Method, [string] $Uri, $Headers, [string] $Body, [string] $ContentType)
            if ($Method -eq 'Get') { return [pscustomobject]@{ value = @() } }
            if ($Uri -like '*/statuses?*') { throw 'status publication failed' }
            return [pscustomobject]@{ id = 1 }
        }

        $failureMessage = $null
        try {
            & (Join-Path $root 'Publish-PRPerfResult.ps1') `
                -CollectionUri 'https://dev.azure.com/example' `
                -Project 'WinUI' `
                -RepositoryId 'repo-id' `
                -PullRequestId 123 `
                -ComparisonPath $comparisonPath `
                -ArtifactUrl 'https://artifacts' `
                -PipelineUrl 'https://pipeline' `
                -AccessToken 'secret-token'
        } catch {
            $failureMessage = $_.Exception.Message
        }
        Assert-Equal 'status publication failed' $failureMessage 'Publisher must surface status publication failure.'
    } finally {
        Remove-Item function:global:Invoke-RestMethod -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $comparisonPath -Force -ErrorAction SilentlyContinue
    }
}

function Test-PublisherThrowsAfterCommentWhenStatusPublicationFails {
    $comparisonPath = Join-Path $PSScriptRoot "publisher-$([guid]::NewGuid()).json"
    $global:PRPerfRestCalls = @()
    try {
        New-TestComparison |
            ConvertTo-Json -Depth 12 |
            Set-Content -LiteralPath $comparisonPath -Encoding UTF8
        function global:Invoke-RestMethod {
            param([string] $Method, [string] $Uri, $Headers, [string] $Body, [string] $ContentType)
            $global:PRPerfRestCalls += [pscustomobject]@{ Method = $Method; Uri = $Uri; Body = $Body }
            if ($Method -eq 'Get') { return [pscustomobject]@{ value = @() } }
            if ($Uri -like '*/statuses?*') { throw 'status publication failed' }
            return [pscustomobject]@{ id = 1 }
        }

        $failureMessage = $null
        try {
            & (Join-Path $root 'Publish-PRPerfResult.ps1') `
                -CollectionUri 'https://dev.azure.com/example' `
                -Project 'WinUI' `
                -RepositoryId 'repo-id' `
                -PullRequestId 123 `
                -ComparisonPath $comparisonPath `
                -ArtifactUrl 'https://artifacts' `
                -PipelineUrl 'https://pipeline' `
                -AccessToken 'secret-token'
        } catch {
            $failureMessage = $_.Exception.Message
        }

        Assert-Equal 'status publication failed' $failureMessage 'Publisher must surface status publication failure.'
        if (@($global:PRPerfRestCalls | Where-Object { $_.Uri -like '*/threads?*' -and $_.Method -eq 'Post' }).Count -ne 1) {
            throw 'Publisher must write the comparison comment before surfacing a status publication failure.'
        }
    } finally {
        Remove-Item function:global:Invoke-RestMethod -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $comparisonPath -Force -ErrorAction SilentlyContinue
    }
}

function Test-PublisherMarksSupersededResultNotApplicable {
    $comparisonPath = Join-Path $PSScriptRoot "publisher-$([guid]::NewGuid()).json"
    $global:PRPerfRestCalls = @()
    try {
        New-TestComparison |
            ConvertTo-Json -Depth 12 |
            Set-Content -LiteralPath $comparisonPath -Encoding UTF8
        function global:Invoke-RestMethod {
            param([string] $Method, [string] $Uri, $Headers, [string] $Body, [string] $ContentType)
            $global:PRPerfRestCalls += [pscustomobject]@{ Method = $Method; Uri = $Uri; Body = $Body }
            if ($Method -eq 'Get' -and $Uri -match '/pullRequests/123\?api-version=7\.1$') {
                return [pscustomobject]@{
                    lastMergeSourceCommit = [pscustomobject]@{ commitId = 'c' * 40 }
                }
            }
            if ($Method -eq 'Get') { return [pscustomobject]@{ value = @() } }
            return [pscustomobject]@{ id = 1 }
        }

        & (Join-Path $root 'Publish-PRPerfResult.ps1') `
            -CollectionUri 'https://dev.azure.com/example' `
            -Project 'WinUI' `
            -RepositoryId 'repo-id' `
            -PullRequestId 123 `
            -ComparisonPath $comparisonPath `
            -ArtifactUrl 'https://artifacts' `
            -PipelineUrl 'https://pipeline' `
            -AccessToken 'secret-token' `
            -ExpectedSourceCommit ('a' * 40)

        $commentBody = @($global:PRPerfRestCalls | Where-Object { $_.Uri -like '*/threads?*' -and $_.Method -eq 'Post' })[0].Body | ConvertFrom-Json
        if ($commentBody.comments[0].content -notlike 'Superseded by a newer PR commit*') {
            throw 'Superseded comments must be visibly prepended with the supersession message.'
        }
        if ($commentBody.comments[0].content -notlike '*Artifacts*https://artifacts*' -or
            $commentBody.comments[0].content -notlike '*Pipeline run*https://pipeline*') {
            throw 'Superseded comments must retain artifact and pipeline links.'
        }
        $statusBody = @($global:PRPerfRestCalls | Where-Object { $_.Uri -like '*/statuses?*' })[0].Body | ConvertFrom-Json
        Assert-Equal 'notApplicable' $statusBody.state 'Superseded result status mismatch.'
    } finally {
        Remove-Item function:global:Invoke-RestMethod -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $comparisonPath -Force -ErrorAction SilentlyContinue
    }
}

function Test-ComparatorWritesMarkdownWithExactlyOneMarker {
    $outputDirectory = Join-Path $PSScriptRoot "comparison-output-$([guid]::NewGuid())"
    try {
        & (Join-Path $root 'Compare-PRPerfResults.ps1') `
            -TargetPath (Join-Path $root 'fixtures\target-pass.json') `
            -TrialPath (Join-Path $root 'fixtures\trial-pass.json') `
            -ThresholdPath (Join-Path $root 'pr-perf-thresholds.json') `
            -OutputDirectory $outputDirectory `
            -ArtifactUrl 'https://artifacts' `
            -PipelineUrl 'https://pipeline'

        $markdownPath = Join-Path $outputDirectory 'comparison.md'
        if (-not (Test-Path -LiteralPath $markdownPath -PathType Leaf)) {
            throw "Comparator did not create '$markdownPath'."
        }
        $markdown = Get-Content -LiteralPath $markdownPath -Raw
        Assert-Equal 1 ([regex]::Matches($markdown, '<!-- winui-pr-perf-result -->').Count) 'Marker count mismatch.'
    } finally {
        Remove-Item -LiteralPath $outputDirectory -Recurse -Force -ErrorAction SilentlyContinue
    }
}


function Test-MarkdownNamesTheKindOfBaselineItComparedAgainst {
    # A main baseline and a same-branch baseline mean very different things. Rendering them
    # identically would let a reader believe a number was attributed to this pull request
    # when it was not.
    $comparison = New-TestComparison
    $comparison.target | Add-Member -NotePropertyName baselineKind -NotePropertyValue 'same-branch' -Force
    $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'
    if ($markdown -notlike '*same-branch*') { throw "The baseline kind is missing from the comment.`n$markdown" }
}

function Test-MarkdownLeavesTheTargetLineAloneWhenTheKindIsUnknown {
    $comparison = New-TestComparison
    $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'
    if ($markdown -notlike "*(build $($comparison.target.buildId))*") { throw "The plain target line was damaged.`n$markdown" }
}

function Test-MarkdownIgnoresABaselineKindItDoesNotRecognise {
    # The compare step runs on every path, including ones where the pipeline variable was
    # never set, so an unexpanded literal must not be printed to the pull request as fact.
    $comparison = New-TestComparison
    $comparison.target | Add-Member -NotePropertyName baselineKind -NotePropertyValue '$(perfBaselineKind)' -Force
    $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'
    if ($markdown -like '*perfBaselineKind*') { throw "An unexpanded pipeline variable reached the comment.`n$markdown" }
}

function Test-MarkdownOmitsXamlRegionsWhenNoneWereMeasured {
    # The app launch trace is the experimental half of this feature. When it is switched
    # off, or fails on the agent, the comparison that does work must read exactly as it
    # did before, with nothing hinting that anything was missing.
    $comparison = New-TestComparison
    $without = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline'
    $withEmpty = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline' -XamlRegions @{}

    Assert-Equal $without $withEmpty 'An empty region set must not change the comment at all.'
    if ($withEmpty -like '*XAML*') { throw 'No XAML section may appear when nothing was measured.' }
}

function Test-MarkdownReportsMeasuredXamlRegions {
    # These numbers come from one app launch on the pull request build only, with nothing
    # to compare against, so they are reported as an observation and never as a verdict.
    $comparison = New-TestComparison
    $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline' `
        -XamlRegions ([ordered]@{ XamlInitializeMs = 25.5; XamlFrameMs = 250.25 })

    if ($markdown -notlike '*XamlInitializeMs*') { throw 'A measured region must be listed.' }
    if ($markdown -notlike '*25.50*') { throw 'A measured region value must be shown.' }
    if ($markdown -notlike '*250.25*') { throw 'A measured region value must be shown.' }
    if ($markdown -notlike '*no baseline*') { throw 'The section must say these numbers have nothing to compare against.' }
}

function Test-MarkdownXamlRegionsNeverChangeTheVerdict {
    # A informational section must not be able to turn a passing run into a failing one.
    $comparison = New-TestComparison
    $markdown = New-PRPerfMarkdown -Comparison $comparison -ArtifactUrl 'https://artifacts' -PipelineUrl 'https://pipeline' `
        -XamlRegions ([ordered]@{ XamlInitializeMs = 9999.0 })

    if ($markdown -notlike "*$($comparison.overallState)*" -and $comparison.overallState -eq 'RegressionWarning') {
        throw 'The heading must still reflect the comparison.'
    }
    if ($markdown -like '*Performance result inconclusive*') { throw 'Region numbers must not make the run inconclusive.' }
}

function Test-XamlRegionsAreEmptyWhenTheTraceFileIsAbsent {
    # The trace step is allowed to be switched off or to fail, so its file is often not
    # there. That is an ordinary outcome and must not disturb the comment.
    $regions = Get-PRPerfXamlRegions -Path (Join-Path ([System.IO.Path]::GetTempPath()) "prperf-missing-$([guid]::NewGuid()).json")

    Assert-Equal 0 @($regions.Keys).Count 'A missing trace file must yield no regions.'
}

function Test-XamlRegionsAreEmptyWhenTheTraceFileIsUnreadable {
    # A half written or truncated file must lose only the informational section. Losing
    # the whole comment over it would trade a working result for a broken one.
    $path = Join-Path ([System.IO.Path]::GetTempPath()) "prperf-bad-$([guid]::NewGuid()).json"
    Set-Content -LiteralPath $path -Value '{ this is not json'
    try {
        $regions = Get-PRPerfXamlRegions -Path $path
        Assert-Equal 0 @($regions.Keys).Count 'An unreadable trace file must yield no regions.'
    } finally {
        Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue
    }
}

function Test-XamlRegionsAreReadFromTheTraceFile {
    $path = Join-Path ([System.IO.Path]::GetTempPath()) "prperf-trace-$([guid]::NewGuid()).json"
    Set-Content -LiteralPath $path -Value '{ "XamlInitializeMs": 25.5, "XamlFrameMs": 250.25 }'
    try {
        $regions = Get-PRPerfXamlRegions -Path $path

        Assert-Equal 25.5 $regions['XamlInitializeMs'] 'A measured region must be read back.'
        Assert-Equal 250.25 $regions['XamlFrameMs'] 'A measured region must be read back.'
    } finally {
        Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue
    }
}

function Test-XamlRegionsIgnoreValuesThatAreNotNumbers {
    # Anything that is not a plain number cannot be a duration. Rendering it would put a
    # meaningless figure in front of a reviewer as though it had been measured.
    $path = Join-Path ([System.IO.Path]::GetTempPath()) "prperf-trace-$([guid]::NewGuid()).json"
    Set-Content -LiteralPath $path -Value '{ "XamlInitializeMs": 25.5, "XamlFrameMs": "unknown" }'
    try {
        $regions = Get-PRPerfXamlRegions -Path $path

        Assert-Equal 25.5 $regions['XamlInitializeMs'] 'The usable region must survive.'
        Assert-Equal 1 @($regions.Keys).Count 'A value that is not a number must be dropped.'
    } finally {
        Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue
    }
}

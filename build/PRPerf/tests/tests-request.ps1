$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

function Assert-Equal($Expected, $Actual, [string] $Message) {
    if ($Expected -ne $Actual) {
        throw "$Message Expected='$Expected' Actual='$Actual'"
    }
}

function Import-PRPerfRequestModule {
    Import-Module (Join-Path $root 'PRPerfRequest.psm1') -Force
}

function Test-RunPerfLabelIsCaseInsensitive {
    Import-PRPerfRequestModule
    $labels = @([pscustomobject]@{ name = 'Run-Perf' })
    if (-not (Test-PRPerfLabel -Labels $labels)) {
        throw 'run-perf label was not detected.'
    }
}

function Test-RunPerfLabelIsRequired {
    Import-PRPerfRequestModule
    $labels = @([pscustomobject]@{ name = 'documentation' })
    if (Test-PRPerfLabel -Labels $labels) {
        throw 'An unrelated label was accepted.'
    }
}

function Test-RequestIdentityChangesWithSourceCommit {
    Import-PRPerfRequestModule
    $a = New-PRPerfRequestIdentity `
        -RepositoryId 'repo' `
        -PullRequestId 12 `
        -SourceCommit ('a' * 40) `
        -TargetCommit ('b' * 40) `
        -ConfigVersion 'pr-smoke-v1'
    $b = New-PRPerfRequestIdentity `
        -RepositoryId 'repo' `
        -PullRequestId 12 `
        -SourceCommit ('c' * 40) `
        -TargetCommit ('b' * 40) `
        -ConfigVersion 'pr-smoke-v1'

    if ($a -eq $b) {
        throw 'Request identity must include source commit.'
    }
    if ($a -notmatch '^[0-9a-f]{64}$') {
        throw "Request identity is not a lowercase SHA-256 value: '$a'."
    }
}

function Test-RequestCurrentAcceptsSameSourceCommit {
    Import-PRPerfRequestModule
    $commit = 'a' * 40
    $current = [pscustomobject]@{
        lastMergeSourceCommit = [pscustomobject]@{ commitId = $commit }
    }

    if (-not (Test-PRPerfRequestCurrent -ExpectedSourceCommit $commit -CurrentPR $current)) {
        throw 'The immutable request source commit should remain current when the PR still points at it.'
    }
}

function Test-RequestCurrentRejectsChangedSourceCommit {
    Import-PRPerfRequestModule
    $current = [pscustomobject]@{
        lastMergeSourceCommit = [pscustomobject]@{ commitId = 'c' * 40 }
    }

    if (Test-PRPerfRequestCurrent -ExpectedSourceCommit ('a' * 40) -CurrentPR $current) {
        throw 'A result for an older PR source commit must be marked superseded.'
    }
}

function Test-ExactBuildRejectsNearbyCommit {
    Import-PRPerfRequestModule
    $builds = Get-Content (Join-Path $root 'fixtures\builds.json') -Raw | ConvertFrom-Json
    $selected = Select-ExactPRPerfBuild `
        -Builds $builds.value `
        -Commit ('b' * 40) `
        -ArtifactName 'drop_amd64fre'

    Assert-Equal ('b' * 40) $selected.sourceVersion 'Wrong commit selected.'
    Assert-Equal 200 $selected.id 'The exact build with the required artifact was not selected.'
}

function Test-ExactBuildRequiresArtifact {
    Import-PRPerfRequestModule
    $builds = @(
        [pscustomobject]@{
            id = 300
            sourceVersion = ('a' * 40)
            status = 'completed'
            result = 'succeeded'
            finishTime = '2026-09-15T10:30:00Z'
            artifacts = @([pscustomobject]@{ name = 'symbols' })
        }
    )

    $threw = $false
    try {
        Select-ExactPRPerfBuild `
            -Builds $builds `
            -Commit ('a' * 40) `
            -ArtifactName 'drop_amd64fre'
    } catch {
        $threw = $true
    }
    Assert-Equal $true $threw 'A build without the required artifact must be rejected.'
}

function Test-LiveAuthorizationHeaderUsesAccessToken {
    Import-PRPerfRequestModule
    $token = 'unit-test-token'
    $headers = New-PRPerfAuthorizationHeaders -AccessToken $token

    if ($headers.Authorization -ne ('Bearer ' + $token)) {
        throw 'Live authorization must construct its header from SYSTEM_ACCESSTOKEN.'
    }
    if ($headers.Authorization -eq ('*' * 6)) {
        throw 'Live authorization still uses the literal redacted placeholder.'
    }
}


function Test-FixtureRunCopiesTargetAndTrialWithoutPerfMachine {
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    foreach ($fixture in @('target-pass.json', 'trial-pass.json')) {
        if ($yaml -notmatch [regex]::Escape($fixture)) {
            throw "Fixture run does not copy '$fixture'."
        }
    }
    if ($yaml -match '(?i)invoke-perf-machine|run-perf-machine') {
        throw 'Fixture mode must not invoke the perf machine.'
    }
}

function Test-PerfRunFallbackWritesInconclusiveFailureArtifacts {
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    foreach ($required in @(
        'Create fallback PR perf comparison artifact',
        'condition: always()',
        'overallState = ''Inconclusive''',
        'requestIdentity = ''${{ parameters.requestIdentity }}''',
        'sourceCommit = ''${{ parameters.sourceCommit }}''',
        'targetCommit = ''${{ parameters.targetCommit }}''',
        'sourceBuildId = ''${{ parameters.sourceBuildId }}''',
        'targetBuildId = ''${{ parameters.targetBuildId }}''',
        'stageName = ''$(System.StageName)''',
        'jobName = ''$(System.JobName)''',
        'failureMessage',
        'logUrl = ''$(pipelineUrl)''',
        'scenarios = @()'
    )) {
        if ($yaml -notmatch [regex]::Escape($required)) {
            throw "Fallback artifact step is missing '$required'."
        }
    }
}

function Get-PRPerfFallbackInlineScript {
        $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw
        $match = [regex]::Match(
            $yaml,
            "(?s)displayName:\s*Create fallback PR perf comparison artifact.*?script:\s*\|\r?\n(?<script>.*?)(?:\r?\n\s{4}- task:|\z)")
        if (-not $match.Success) {
            throw 'Fallback artifact inline script was not found.'
        }
        $scriptLines = @($match.Groups['script'].Value -split "\r?\n" | ForEach-Object {
            if ($_.StartsWith('          ')) {
                $_.Substring(10)
            } else {
                $_
            }
        })
        return ($scriptLines -join "`n").TrimEnd()
}

function Test-PerfRunFallbackCapturesActualFailedTimelineRecord {
        $script = Get-PRPerfFallbackInlineScript
        $comparisonDirectory = Join-Path $PSScriptRoot "fallback-$([guid]::NewGuid())"
        $savedToken = $env:SYSTEM_ACCESSTOKEN
        $global:PRPerfTimelineRequested = $false
        try {
            New-Item -ItemType Directory -Path $comparisonDirectory -Force | Out-Null
            $script = $script.
                Replace('$(comparisonDirectory)', $comparisonDirectory).
                Replace('$(System.StageName)', 'RunPRPerf').
                Replace('$(System.JobName)', 'RunPRPerf').
                Replace('$(pipelineUrl)', 'https://pipeline').
                Replace('$(artifactUrl)', 'https://artifacts').
                Replace('$(prPerfSourcesDirectory)', (Split-Path -Parent (Split-Path -Parent $root))).
                Replace('$(Build.BuildId)', '1234').
                Replace('$(System.CollectionUri)', 'https://dev.azure.com/example/').
                Replace('$(System.TeamProject)', 'WinUI').
                Replace('${{ parameters.sourceCommit }}', ('a' * 40)).
                Replace('${{ parameters.targetCommit }}', ('b' * 40)).
                Replace('${{ parameters.sourceBuildId }}', '100').
                Replace('${{ parameters.targetBuildId }}', '200').
                Replace('${{ parameters.requestIdentity }}', ('c' * 64))

            $env:SYSTEM_ACCESSTOKEN = 'unit-test-token'
            function global:Invoke-RestMethod {
                param([string] $Method, [string] $Uri, $Headers)

                $global:PRPerfTimelineRequested = $true
                if ($Method -ne 'Get' -or $Uri -notlike '*/_apis/build/builds/1234/timeline?api-version=7.1') {
                    throw "Unexpected fallback timeline request: $Method $Uri"
                }
                Assert-Equal 'Bearer unit-test-token' $Headers.Authorization 'Fallback timeline request authorization mismatch.'
                return [pscustomobject]@{
                    records = @(
                        [pscustomobject]@{ id = 'stage-1'; type = 'Stage'; name = 'RunPRPerf'; result = 'failed' },
                        [pscustomobject]@{ id = 'job-1'; parentId = 'stage-1'; type = 'Job'; name = 'RunPRPerf'; result = 'failed' },
                        [pscustomobject]@{
                            id = 'task-1'
                            parentId = 'job-1'
                            type = 'Task'
                            name = 'Run trial PR performance scenarios'
                            result = 'failed'
                            log = [pscustomobject]@{ url = 'https://dev.azure.com/example/WinUI/_apis/build/builds/1234/logs/17' }
                            issues = @([pscustomobject]@{
                                type = 'error'
                                message = 'Scenario timeout while running PRPerf-ObjectCreation.BasicControls.'
                            })
                        }
                    )
                }
            }

            & ([scriptblock]::Create($script))

            Assert-Equal $true $global:PRPerfTimelineRequested 'Fallback must query the current build timeline.'
            $comparison = Get-Content -LiteralPath (Join-Path $comparisonDirectory 'comparison.json') -Raw | ConvertFrom-Json
            Assert-Equal 'Inconclusive' $comparison.overallState 'Fallback comparison state mismatch.'
            Assert-Equal 0 @($comparison.scenarios).Count 'Fallback must not synthesize scenario measurements.'
            Assert-Equal 'RunPRPerf' $comparison.failure.stageName 'Fallback failure stage mismatch.'
            Assert-Equal 'RunPRPerf' $comparison.failure.jobName 'Fallback failure job mismatch.'
            Assert-Equal 'Run trial PR performance scenarios' $comparison.failure.taskName 'Fallback must name the actual failed task.'
            Assert-Equal 'Scenario timeout while running PRPerf-ObjectCreation.BasicControls.' $comparison.failure.failureMessage 'Fallback must preserve the real timeline error.'
            Assert-Equal 'https://dev.azure.com/example/WinUI/_apis/build/builds/1234/logs/17' $comparison.failure.logUrl 'Fallback failure log URL mismatch.'
            if ($comparison.issues[0] -notlike '*Scenario timeout while running PRPerf-ObjectCreation.BasicControls.*') {
                throw 'Fallback issues must include the real timeline error.'
            }
        } finally {
            Remove-Item function:global:Invoke-RestMethod -ErrorAction SilentlyContinue
            if ($null -eq $savedToken) {
                Remove-Item env:SYSTEM_ACCESSTOKEN -ErrorAction SilentlyContinue
            } else {
                $env:SYSTEM_ACCESSTOKEN = $savedToken
            }
            Remove-Item -LiteralPath $comparisonDirectory -Recurse -Force -ErrorAction SilentlyContinue
        }
}

function Test-ComparisonAndPublicationAlwaysRun {
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    foreach ($displayName in @(
        'Compare PR performance results',
        'Verify PR perf comparison artifact contents',
        'Publish PR perf result'
    )) {
        $taskPattern = "(?s)displayName:\s*$([regex]::Escape($displayName)).{0,300}?condition:\s*always\(\)"
        if ($yaml -notmatch $taskPattern) {
            throw "'$displayName' must run under condition: always()."
        }
    }
}

function Test-ComparisonArtifactIsCollectedByOneBranch {
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    # OneBranch rejects explicit publish steps and instead uploads whatever the
    # job leaves in ob_outputDirectory, so the comparison files reach the drop
    # only while comparisonDirectory stays nested inside that directory.
    if ($yaml -match 'PublishPipelineArtifact|PublishBuildArtifacts') {
        throw 'OneBranch forbids explicit publish tasks; artifacts must come from ob_outputDirectory.'
    }

    if ($yaml -notmatch 'ob_outputDirectory:\s*\$\(Build\.ArtifactStagingDirectory\)') {
        throw 'ob_outputDirectory must be the artifact staging directory.'
    }

    if ($yaml -notmatch 'comparisonDirectory:\s*\$\(Build\.ArtifactStagingDirectory\)\\PRPerf') {
        throw 'comparisonDirectory must sit inside ob_outputDirectory so OneBranch uploads it.'
    }

    foreach ($file in @('comparison.json', 'comparison.md')) {
        if ($yaml -notmatch [regex]::Escape($file)) {
            throw "The comparison artifact does not include '$file'."
        }
    }
}

function Test-TargetDiagnosticsAreCopiedBeforeRawCleanup {
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    $targetConversion = $yaml.IndexOf('displayName: Convert target raw performance results', [System.StringComparison]::Ordinal)
    $targetDiagnostics = $yaml.IndexOf('displayName: Preserve target PR performance diagnostics', [System.StringComparison]::Ordinal)
    $targetCleanup = $yaml.IndexOf('displayName: Clear target raw performance results', [System.StringComparison]::Ordinal)

    if ($targetConversion -lt 0 -or $targetDiagnostics -le $targetConversion -or $targetCleanup -le $targetDiagnostics) {
        throw 'Target diagnostics must be preserved after target conversion and before current-shift cleanup.'
    }
}

function Test-ComparisonArtifactIncludesTargetAndTrialDiagnostics {
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    foreach ($directory in @('target-diagnostics', 'trial-diagnostics')) {
        if ($yaml -notmatch [regex]::Escape("`$(comparisonDirectory)\$directory")) {
            throw "PRPerfComparison does not include '$directory'."
        }
    }
    foreach ($pattern in @('*.raw.csv', 'shift.agg.csv', '*.stat.csv')) {
        if ($yaml -notmatch [regex]::Escape($pattern)) {
            throw "Diagnostic preservation is missing '$pattern'."
        }
    }
    if ($yaml -notmatch 'No selected raw CSV files') {
        throw 'Missing required selected raw CSVs must fail loudly.'
    }
    if ($yaml -notmatch 'No optional .* diagnostics') {
        throw 'Missing optional aggregate/stat diagnostics must be logged.'
    }
}

function Test-DiscoverDryRunWritesOneImmutableQueueBody {
    $outputPath = Join-Path $PSScriptRoot "dispatch-$([guid]::NewGuid()).json"
    $savedEnvironment = @{
        PRPerfRepositoryId = $env:PRPerfRepositoryId
        PRPerfSourcePipelineId = $env:PRPerfSourcePipelineId
        PRPerfTargetPipelineId = $env:PRPerfTargetPipelineId
        PRPerfPipelineId = $env:PRPerfPipelineId
        PRPerfArtifactName = $env:PRPerfArtifactName
        SYSTEM_ACCESSTOKEN = $env:SYSTEM_ACCESSTOKEN
    }

    try {
        $env:PRPerfRepositoryId = 'repo'
        $env:PRPerfSourcePipelineId = '501'
        $env:PRPerfTargetPipelineId = '502'
        $env:PRPerfPipelineId = '503'
        $env:PRPerfArtifactName = 'drop_amd64fre'
        Remove-Item env:SYSTEM_ACCESSTOKEN -ErrorAction SilentlyContinue

        & (Join-Path $root 'Dispatch-PRPerf.ps1') `
            -Mode Discover `
            -FixtureDirectory (Join-Path $root 'fixtures') `
            -DryRunOutputPath $outputPath

        $body = Get-Content -LiteralPath $outputPath -Raw | ConvertFrom-Json
        Assert-Equal 'refs/heads/users/example/perf-change' $body.resources.repositories.self.refName 'Source ref mismatch.'
        Assert-Equal ('a' * 40) $body.resources.repositories.self.version 'Queued repository version mismatch.'
        Assert-Equal '12' $body.templateParameters.pullRequestId 'Pull request mismatch.'
        Assert-Equal 'repo' $body.templateParameters.repositoryId 'Repository mismatch.'
        Assert-Equal ('a' * 40) $body.templateParameters.sourceCommit 'Source commit mismatch.'
        Assert-Equal ('b' * 40) $body.templateParameters.targetCommit 'Target commit mismatch.'
        Assert-Equal '100' $body.templateParameters.sourceBuildId 'Source build mismatch.'
        Assert-Equal '200' $body.templateParameters.targetBuildId 'Target build must be exact and have the artifact.'
        Assert-Equal $true $body.templateParameters.useFixtures 'Fixture execution was not requested.'
        Assert-Equal 64 $body.templateParameters.requestIdentity.Length 'Request identity length mismatch.'
    } finally {
        foreach ($name in $savedEnvironment.Keys) {
            Set-Item -Path "env:$name" -Value $savedEnvironment[$name]
        }
        if (Test-Path -LiteralPath $outputPath) {
            Remove-Item -LiteralPath $outputPath -Force
        }
    }


}


function Test-DiscoverDryRunSkipsDuplicateQueuedRequestIdentity {
    Import-PRPerfRequestModule
    $outputPath = Join-Path $PSScriptRoot "dispatch-duplicate-$([guid]::NewGuid()).json"
    $fixtureDirectory = Join-Path $PSScriptRoot "fixtures-duplicate-$([guid]::NewGuid())"
    $savedEnvironment = @{
        PRPerfRepositoryId = $env:PRPerfRepositoryId
        PRPerfSourcePipelineId = $env:PRPerfSourcePipelineId
        PRPerfTargetPipelineId = $env:PRPerfTargetPipelineId
        PRPerfPipelineId = $env:PRPerfPipelineId
        PRPerfArtifactName = $env:PRPerfArtifactName
        SYSTEM_ACCESSTOKEN = $env:SYSTEM_ACCESSTOKEN
    }

    try {
        New-Item -ItemType Directory -Path $fixtureDirectory -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $root 'fixtures\active-prs.json') -Destination $fixtureDirectory
        Copy-Item -LiteralPath (Join-Path $root 'fixtures\pr-labels.json') -Destination $fixtureDirectory
        Copy-Item -LiteralPath (Join-Path $root 'fixtures\builds.json') -Destination $fixtureDirectory
        $requestIdentity = New-PRPerfRequestIdentity `
            -RepositoryId 'repo' `
            -PullRequestId 12 `
            -SourceCommit ('a' * 40) `
            -TargetCommit ('b' * 40) `
            -ConfigVersion 'pr-smoke-v1'
        @{
            value = @(
                @{
                    id = 900
                    status = 'inProgress'
                    templateParameters = @{ requestIdentity = $requestIdentity }
                    tags = @('PRPerf', 'PRPerf-PR-12', "PRPerf-Request-$($requestIdentity.Substring(0, 16))")
                }
            )
        } | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $fixtureDirectory 'perf-runs.json') -Encoding UTF8

        $env:PRPerfRepositoryId = 'repo'
        $env:PRPerfSourcePipelineId = '501'
        $env:PRPerfTargetPipelineId = '502'
        $env:PRPerfPipelineId = '503'
        $env:PRPerfArtifactName = 'drop_amd64fre'
        Remove-Item env:SYSTEM_ACCESSTOKEN -ErrorAction SilentlyContinue

        & (Join-Path $root 'Dispatch-PRPerf.ps1') `
            -Mode Discover `
            -FixtureDirectory $fixtureDirectory `
            -DryRunOutputPath $outputPath

        $bodyJson = (Get-Content -LiteralPath $outputPath -Raw).Trim()
        Assert-Equal '[]' $bodyJson 'An already queued request identity must not be queued again.'
    } finally {
        foreach ($name in $savedEnvironment.Keys) {
            Set-Item -Path "env:$name" -Value $savedEnvironment[$name]
        }
        Remove-Item -LiteralPath $fixtureDirectory -Recurse -Force -ErrorAction SilentlyContinue
        if (Test-Path -LiteralPath $outputPath) {
            Remove-Item -LiteralPath $outputPath -Force
        }
    }
}

function Test-LiveDispatchSkipsDuplicateQueuedRequestIdentityBeforeTagsExist {
    Import-PRPerfRequestModule
    $requestIdentity = New-PRPerfRequestIdentity `
        -RepositoryId 'repo' `
        -PullRequestId 12 `
        -SourceCommit ('a' * 40) `
        -TargetCommit ('b' * 40) `
        -ConfigVersion 'pr-smoke-v1'
    $savedEnvironment = @{
        PRPerfRepositoryId = $env:PRPerfRepositoryId
        PRPerfSourcePipelineId = $env:PRPerfSourcePipelineId
        PRPerfTargetPipelineId = $env:PRPerfTargetPipelineId
        PRPerfPipelineId = $env:PRPerfPipelineId
        PRPerfArtifactName = $env:PRPerfArtifactName
        SYSTEM_ACCESSTOKEN = $env:SYSTEM_ACCESSTOKEN
        SYSTEM_COLLECTIONURI = $env:SYSTEM_COLLECTIONURI
        SYSTEM_TEAMPROJECT = $env:SYSTEM_TEAMPROJECT
    }
    $global:PRPerfDispatchCalls = @()
    $global:PRPerfQueueCalls = @()

    try {
        $env:PRPerfRepositoryId = 'repo'
        $env:PRPerfSourcePipelineId = '501'
        $env:PRPerfTargetPipelineId = '502'
        $env:PRPerfPipelineId = '503'
        $env:PRPerfArtifactName = 'drop_amd64fre'
        $env:SYSTEM_ACCESSTOKEN = 'dispatch-token'
        $env:SYSTEM_COLLECTIONURI = 'https://dev.azure.com/example/'
        $env:SYSTEM_TEAMPROJECT = 'WinUI'

        function global:Invoke-RestMethod {
            param(
                [string] $Method,
                [string] $Uri,
                $Headers,
                [string] $Body,
                [string] $ContentType
            )

            $global:PRPerfDispatchCalls += [pscustomobject]@{
                Method = $Method
                Uri = $Uri
                Headers = $Headers
                Body = $Body
                ContentType = $ContentType
            }
            Assert-Equal 'Bearer dispatch-token' $Headers.Authorization "Dispatch authorization mismatch for $Uri."

            if ($Uri -like '*/_apis/git/repositories/repo/pullRequests?searchCriteria.status=active&api-version=7.1') {
                return [pscustomobject]@{
                    value = @([pscustomobject]@{
                        pullRequestId = 12
                        sourceRefName = 'refs/heads/users/example/perf-change'
                        repository = [pscustomobject]@{ id = 'repo' }
                        lastMergeSourceCommit = [pscustomobject]@{ commitId = ('a' * 40) }
                        lastMergeTargetCommit = [pscustomobject]@{ commitId = ('b' * 40) }
                    })
                }
            }
            if ($Uri -like '*/pullRequests/12/labels?api-version=7.1-preview.1') {
                return [pscustomobject]@{ value = @([pscustomobject]@{ name = 'Run-Perf' }) }
            }
            if ($Uri -like '*/_apis/build/builds?definitions=503*') {
                if ($Uri -like '*tagFilters=PRPerf*') {
                    return [pscustomobject]@{ value = @() }
                }
                return [pscustomobject]@{
                    value = @([pscustomobject]@{
                        id = 900
                        status = 'notStarted'
                        parameters = (@{ requestIdentity = $requestIdentity } | ConvertTo-Json -Compress)
                        tags = @()
                    })
                }
            }
            if ($Uri -like '*/_apis/build/builds?definitions=501*') {
                return [pscustomobject]@{
                    value = @([pscustomobject]@{
                        id = 100
                        sourceVersion = ('a' * 40)
                        status = 'completed'
                        result = 'succeeded'
                        finishTime = '2026-09-15T08:00:00Z'
                    })
                }
            }
            if ($Uri -like '*/_apis/build/builds?definitions=502*') {
                return [pscustomobject]@{
                    value = @([pscustomobject]@{
                        id = 200
                        sourceVersion = ('b' * 40)
                        status = 'completed'
                        result = 'succeeded'
                        finishTime = '2026-09-15T08:30:00Z'
                    })
                }
            }
            if ($Uri -like '*/_apis/build/builds/100/artifacts?api-version=7.1' -or
                $Uri -like '*/_apis/build/builds/200/artifacts?api-version=7.1') {
                return [pscustomobject]@{ value = @([pscustomobject]@{ name = 'drop_amd64fre' }) }
            }
            if ($Uri -like '*/_apis/pipelines/503/runs?api-version=7.1-preview.1') {
                $global:PRPerfQueueCalls += $Body
                return [pscustomobject]@{ id = 901 }
            }
            throw "Unexpected dispatch request: $Method $Uri"
        }

        & (Join-Path $root 'Dispatch-PRPerf.ps1') -Mode Discover

        $recentRunCalls = @($global:PRPerfDispatchCalls | Where-Object { $_.Uri -like '*/_apis/build/builds?definitions=503*' })
        Assert-Equal 1 $recentRunCalls.Count 'Dispatcher recent-run lookup count mismatch.'
        if ($recentRunCalls[0].Uri -like '*tagFilters=PRPerf*') {
            throw 'Dispatcher must not depend on PRPerf tags when looking for already queued runs.'
        }
        Assert-Equal 0 $global:PRPerfQueueCalls.Count 'An already queued request identity without tags must not be queued again.'
    } finally {
        Remove-Item function:global:Invoke-RestMethod -ErrorAction SilentlyContinue
        foreach ($name in $savedEnvironment.Keys) {
            if ($null -eq $savedEnvironment[$name]) {
                Remove-Item -Path "env:$name" -ErrorAction SilentlyContinue
            } else {
                Set-Item -Path "env:$name" -Value $savedEnvironment[$name]
            }
        }
    }
}

function Test-SingleDryRunResolvesRequestedPullRequest {
    $outputPath = Join-Path $PSScriptRoot "dispatch-single-$([guid]::NewGuid()).json"
    $savedEnvironment = @{
        PRPerfRepositoryId = $env:PRPerfRepositoryId
        PRPerfSourcePipelineId = $env:PRPerfSourcePipelineId
        PRPerfTargetPipelineId = $env:PRPerfTargetPipelineId
        PRPerfPipelineId = $env:PRPerfPipelineId
        PRPerfArtifactName = $env:PRPerfArtifactName
    }

    try {
        $env:PRPerfRepositoryId = 'repo'
        $env:PRPerfSourcePipelineId = '501'
        $env:PRPerfTargetPipelineId = '502'
        $env:PRPerfPipelineId = '503'
        $env:PRPerfArtifactName = 'drop_amd64fre'

        & (Join-Path $root 'Dispatch-PRPerf.ps1') `
            -Mode Single `
            -PullRequestId 12 `
            -FixtureDirectory (Join-Path $root 'fixtures') `
            -DryRunOutputPath $outputPath

        $body = Get-Content -LiteralPath $outputPath -Raw | ConvertFrom-Json
        Assert-Equal '12' $body.templateParameters.pullRequestId 'Single mode resolved the wrong pull request.'
        Assert-Equal ('a' * 40) $body.templateParameters.sourceCommit 'Single mode source commit mismatch.'
        Assert-Equal ('b' * 40) $body.templateParameters.targetCommit 'Single mode target commit mismatch.'
    } finally {
        foreach ($name in $savedEnvironment.Keys) {
            Set-Item -Path "env:$name" -Value $savedEnvironment[$name]
        }
        if (Test-Path -LiteralPath $outputPath) {
            Remove-Item -LiteralPath $outputPath -Force
        }
    }
}

function Test-GitHubPublishStepRejectsUnexpandedPipelineMacros {
    # The call site passes runtime macros such as $(System.PullRequest.PullRequestNumber).
    # Azure Pipelines leaves an undefined macro as literal text, which is not blank, so a
    # plain IsNullOrWhiteSpace fallback does not fire and [int] then throws - which would
    # fail the publish step and leave the pull request with no comment at all.
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    if ($yaml -notmatch 'pullRequestNumber -notmatch ''\^\\d\+\$''') {
        throw 'The GitHub publish step must validate the pull request number is numeric before casting it.'
    }
    if ($yaml -notmatch 'expectedSourceCommit -notmatch ''\^\[0-9a-fA-F\]\{40\}\$''') {
        throw 'The GitHub publish step must validate the source commit is a real SHA before using it.'
    }
}



function Test-PerfCheckoutPinsAnExplicitPathForEveryJob {
    # The agent relocates an unpinned checkout ("Repository is current at
    # ...\s\microsoft-ui-xaml, move to ...\s"), and the container step then
    # failed to find build\PRPerf\PRPerfGitHub.psm1 under $(Build.SourcesDirectory).
    # Pinning the checkout path makes the source location deterministic.
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    $checkouts = [regex]::Matches($yaml, '(?m)^(?<indent>\s*)- checkout: self\r?\n(?<body>(?:\k<indent>\s+\S.*\r?\n)*)')
    if ($checkouts.Count -lt 2) {
        throw "Expected at least two 'checkout: self' steps, found $($checkouts.Count)."
    }
    foreach ($checkout in $checkouts) {
        if ($checkout.Groups['body'].Value -notmatch '(?m)^\s+path:\s*\S') {
            throw 'Every "checkout: self" step must pin an explicit path so the sources directory is deterministic.'
        }
    }
}

function Test-PerfScriptsResolveFromThePinnedCheckoutPath {
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    if ($yaml -notmatch '(?m)^\s+prPerfSourcesDirectory:\s*\S') {
        throw 'The template must define prPerfSourcesDirectory pointing at the pinned checkout path.'
    }
    if ($yaml -match [regex]::Escape("Join-Path '`$(Build.SourcesDirectory)' 'build\PRPerf")) {
        throw 'PR perf scripts must not be resolved from the unpinned $(Build.SourcesDirectory).'
    }
}

function Test-PerfGateCannotFailTheStageOnModuleLoadFailure {
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw
    $match = [regex]::Match($yaml, "(?s)name: gate.*?script:\s*\|\r?\n(?<script>.*?)(?:\r?\n  - job:|\z)")
    if (-not $match.Success) {
        throw 'The gate inline script was not found.'
    }
    $script = $match.Groups['script'].Value
    $importIndex = $script.IndexOf('Import-Module')
    $tryIndex = $script.IndexOf('try {')
    if ($importIndex -lt 0 -or $tryIndex -lt 0) {
        throw 'The gate script must import the module inside a try block.'
    }
    if ($importIndex -lt $tryIndex) {
        throw 'The gate must import its module inside the try/catch so a load failure skips perf instead of failing the stage.'
    }
}


function Test-PerfStageDoesNotWaitOnTheProductBuild {
    # The perf stage neither consumes build output nor blocks the pull request, so
    # queueing it behind the ~2.5 hour product build only delays informational
    # feedback. Running it in parallel keeps the signal close to the push.
    $callSite = Get-Content (Join-Path $root '..\WinUI-GitHub-PR.yml') -Raw
    $match = [regex]::Match(
        $callSite,
        "(?s)- template: AzurePipelinesTemplates\\WinUI-PRPerf-Run\.yml\r?\n\s+parameters:\r?\n(?<params>.*?)(?:\r?\n\s{4}-\s|\z)")
    if (-not $match.Success) {
        throw 'The PR perf template call site was not found.'
    }
    if ($match.Groups['params'].Value -match '(?m)^\s*dependsOn:\s*\S') {
        throw 'The perf stage must not declare dependsOn, so it runs in parallel with the product build.'
    }
}


function Test-PerfStageDeclaresNoImplicitStageDependency {
    # Azure Pipelines makes a stage depend on the preceding stage unless it
    # declares an explicit empty dependency, so simply omitting dependsOn put
    # the perf stage at the end of the whole pipeline instead of alongside it.
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    if ($yaml -notmatch '(?m)^\s*dependsOn:\s*\[\]\s*$') {
        throw 'The perf stage must declare "dependsOn: []" when no dependency is supplied, or it inherits the previous stage.'
    }
}


function Test-PerfJobOnlyRequestsTheDedicatedPoolWhenAsked {
    # The WinUI-PerfTest queue is not authorized for the PR pipeline and its single
    # agent is offline, so naming it left the whole stage pending on a permission
    # prompt and no comment was ever posted. The dedicated pool must be opt-in.
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    if ($yaml -notmatch '(?m)^- name: perfPoolName\s*$') {
        throw 'The template must expose a perfPoolName parameter so the dedicated pool is opt-in.'
    }
    if ($yaml -match "(?m)^\s*name: WinUI-PerfTest\s*$") {
        throw 'The template must not hard-code the WinUI-PerfTest pool.'
    }
    if ($yaml -notmatch [regex]::Escape("if ne(parameters.perfPoolName, '')")) {
        throw 'The dedicated pool must only be requested when perfPoolName is supplied.'
    }

    $callSite = Get-Content (Join-Path $root '..\WinUI-GitHub-PR.yml') -Raw
    if ($callSite -match '(?m)^\s*perfPoolName:\s*WinUI-PerfTest\s*$') {
        throw 'The PR pipeline must not request the unauthorized, offline WinUI-PerfTest pool.'
    }
}

function Test-NoPerfStepCanFailTheStage {
    # 1ES strips job-level continueOnError during template expansion: the expanded
    # YAML for this pipeline contains no job-level continueOnError at all, only
    # step-level ones. The whole design rests on perf never failing a pull request,
    # so the guarantee has to live where it actually survives expansion.
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw
    $jobStart = $yaml.IndexOf('- job: GatePRPerf')
    if ($jobStart -lt 0) {
        throw 'Could not locate the perf jobs in the template.'
    }
    $jobsText = $yaml.Substring($jobStart)

    $taskMatches = @([regex]::Matches($jobsText, '(?m)^(\s*)- task: \S+\s*$'))
    if ($taskMatches.Count -eq 0) {
        throw 'Expected the perf jobs to declare tasks.'
    }

    $offenders = @()
    for ($i = 0; $i -lt $taskMatches.Count; $i++) {
        $start = $taskMatches[$i].Index
        $end = if ($i + 1 -lt $taskMatches.Count) { $taskMatches[$i + 1].Index } else { $jobsText.Length }
        $stepText = $jobsText.Substring($start, $end - $start)
        if ($stepText -notmatch '(?m)^\s*continueOnError:\s*true\s*$') {
            $offenders += ($stepText -split "`n")[0].Trim()
        }
    }

    if ($offenders.Count -gt 0) {
        throw ("Every perf step must set continueOnError so the informational stage cannot fail the run. Missing on: {0}" -f ($offenders -join '; '))
    }
}

function Test-PerfJobPinsTheImageWheneverItNamesAPool {
    # Every other test job in this pipeline pairs its custom pool with an
    # ImageOverride demand. Without one the agent is handed an arbitrary image, so
    # the tooling a measurement needs is present or absent at random and a run
    # cannot be reproduced. A named pool must always carry demands.
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    if ($yaml -notmatch '(?m)^- name: perfPoolDemands\s*$') {
        throw 'The template must expose a perfPoolDemands parameter so the image can be pinned.'
    }
    if ($yaml -notmatch [regex]::Escape("if ne(parameters.perfPoolDemands, '')")) {
        throw 'Demands must only be emitted when perfPoolDemands is supplied.'
    }
    if ($yaml -notmatch '(?m)^\s*demands: \$\{\{ parameters\.perfPoolDemands \}\}\s*$') {
        throw 'The perf job must forward perfPoolDemands to the pool.'
    }

    $callSite = Get-Content (Join-Path $root '..\WinUI-GitHub-PR.yml') -Raw
    $namesAPool = $callSite -match '(?m)^\s*perfPoolName:\s*(\S+)\s*$'
    $pinsAnImage = $callSite -match '(?m)^\s*perfPoolDemands:\s*\S'
    if ($namesAPool -and -not $pinsAnImage) {
        throw 'The PR pipeline names a perf pool without pinning an image via perfPoolDemands.'
    }
}

function Test-PerfJobReportsMeasurementToolingAvailability {
    # Pointing the job at a shared lab pool only helps if that image actually
    # carries the tracing tools the harness shells out to. Reporting what is
    # present turns a silent no-op into evidence, and must never fail the job.
    $yaml = Get-Content (Join-Path $root '..\AzurePipelinesTemplates\WinUI-PRPerf-Run.yml') -Raw

    foreach ($probe in @('xperf', 'wpr', 'python')) {
        if ($yaml -notmatch [regex]::Escape($probe)) {
            throw "The perf job must report whether '$probe' is available on the agent image."
        }
    }
    if ($yaml -notmatch 'whoami') {
        throw 'The perf job must report whether it is elevated, which ETW tracing requires.'
    }

    $probeStep = [regex]::Match(
        $yaml,
        '(?ms)^\s*- task: powershell@2\s*\r?\n\s*displayName: Report measurement tooling availability.*?(?=^\s*- task: |\Z)',
        [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if (-not $probeStep.Success) {
        throw 'The perf job must contain a "Report measurement tooling availability" step.'
    }
    if ($probeStep.Value -notmatch 'continueOnError:\s*true') {
        throw 'The tooling probe is diagnostic only and must never fail the job.'
    }
}

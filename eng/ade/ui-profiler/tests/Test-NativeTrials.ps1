# Synthetic scope/approval/report checks; no native build, model, UI or capture.
#Requires -Version 7
[CmdletBinding()]
param([switch]$LocalReportOnly)
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
Import-Module "$PSScriptRoot\..\pack\tools\NativeTrials.psm1" -Force -DisableNameChecking
$temp = Join-Path $PSScriptRoot ('.native-trial-contracts-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory $temp)
function Refuses([scriptblock]$Action, [string]$Message) {
    $failed = $false
    try { & $Action } catch { $failed = $true }
    if (!$failed) { throw $Message }
}
try {
    & (Get-Module NativeTrials) {
        param($directory)
        # All ADE, measurement and Git behavior in these reporting cases is synthetic.
        # Only local fixture/report files are read or written; no external process runs.
        function Assert-OriginalSource { param($Context) }
        function Assert-TrialContext { param($Context) }
        function Assert-NativeTrialContext { param($Context) }
        function Assert-Measurement { param($Measurement)
            if ($Measurement.fixture -cne 'synthetic-only') { throw 'Unexpected measurement in offline local-report test.' }
        }
        function Invoke-AdeRuntime { throw 'Local reporting must never invoke ADE or remote actions.' }
        function Assert-ScopedCandidate {
            param($RepositoryPath, $AllowedPaths, $Level)
            if (!$AllowedPaths -or !(Test-Path -LiteralPath (Join-Path $RepositoryPath $AllowedPaths[0]) -PathType Leaf)) {
                throw 'Synthetic workcopy has no verifiable scoped patch.'
            }
        }
        function Get-SourceHash {
            param($RepositoryPath, $Files)
            if ($RepositoryPath.EndsWith('\snapshot')) { return '9'*64 }
            $file = @(Get-ChildItem -LiteralPath $RepositoryPath -Recurse -File -ErrorAction Stop)
            if ($file.Count -ne 1) { throw 'Synthetic workcopy is unavailable.' }
            Get-Sha256 $file[0].FullName
        }
        function Invoke-LocalTrialGit {
            param($Repository, $Arguments)
            $file = @(Get-ChildItem -LiteralPath $Repository -Recurse -File)[0]
            $relative = [IO.Path]::GetRelativePath($Repository, $file.FullName).Replace('\', '/')
            $text = if ($Arguments[0] -eq 'cat-file') { 'synthetic baseline source' }
                elseif ($Arguments -contains '--name-status') { "M`t$relative" }
                else { "synthetic current scoped diff for $relative`n" }
            @{text=$text;bytes=[Text.Encoding]::UTF8.GetBytes($text)}
        }
        function Expect([bool]$Condition, [string]$Message) { if (!$Condition) { throw $Message } }
        $setDirectory = Join-Path $directory 'local-report'
        [void](New-Item -ItemType Directory -Force $setDirectory)
        $app = Join-Path $directory 'original-app'
        $native = Join-Path $directory 'original-native'
        [void](New-Item -ItemType Directory -Force $app, $native)
        $recommendationPath = Join-Path $setDirectory 'recommendations.json'
        $entries = @(
            @{id='planned';level='application';title='Planned only';verdict='propose';summary='Prose must not become an applied fix.'},
            @{id='no-data';level='framework';title='Missing data';verdict='insufficient-evidence';summary='Missing measurements.'},
            @{id='pending';level='application';title='Pending exact approval';verdict='propose';summary='Not applied.'},
            @{id='failed';level='control';title='Failed candidate';verdict='recommend';summary='A proposal is not success.'},
            @{id='no-improvement';level='control';title='Measured but not improved';verdict='recommend';summary='Actual local change, not a speedup.'},
            @{id='measured';level='framework';title='Measured local change';verdict='recommend';summary='Synthetic source hypothesis.'},
            @{id='mismatched';level='application';title='Unbound local change';verdict='propose';summary='Must not claim measured application.'}
        )
        Write-JsonFile $recommendationPath @{coverage=@{application='covered';control='covered';framework='covered'}
            runtimeSourceParity='Synthetic fixture only';recommendations=$entries}
        $recommendations = @{path=$recommendationPath;sha256=Get-Sha256 $recommendationPath}
        $trials = @(
            foreach ($entry in $entries | Where-Object { $_.id -notin @('planned', 'no-data') }) {
                $trialDirectory = Join-Path $setDirectory "trials\$($entry.id)"
                $runDirectory = Join-Path $trialDirectory 'ade-runs\synthetic-run'
                $contextPath = Join-Path $trialDirectory 'pack\knowledge\trial-context.json'
                [void](New-Item -ItemType Directory -Force $runDirectory, (Split-Path $contextPath -Parent))
                $workcopy = Join-Path $directory "workcopies\$($entry.id)"
                $relative = switch ($entry.level) {
                    application { 'Control.cs' }
                    control { 'controls\dev\ControlX\ControlX.cpp' }
                    framework { 'dxaml\xcp\core\ControlX.cpp' }
                }
                if ($entry.id -ne 'failed') {
                    $source = Join-Path $workcopy $relative
                    [void](New-Item -ItemType Directory -Force (Split-Path $source -Parent))
                    [IO.File]::WriteAllText($source, "synthetic actual local candidate for $($entry.id)")
                }
                Write-JsonFile (Join-Path $runDirectory 'proposal.json') @{workCopy=$workcopy}
                $proposalSha256 = Get-Sha256 (Join-Path $runDirectory 'proposal.json')
                $summaryPath = Join-Path $trialDirectory 'ade-run.json'
                Write-JsonFile $summaryPath @{status='AwaitingApproval';runId='synthetic-run'
                    runDir=$runDirectory;proposalSha256=$proposalSha256}
                $trialContext = @{schema='ui-profiler/trial/v1';directory=$trialDirectory;recommendationId=$entry.id
                    originalRepository=$app;originalSourceSha256=('8'*64);recommendations=$recommendations;allowedPaths=@($relative)}
                if ($entry.level -ne 'application') {
                    $trialContext.schema='ui-profiler/native-trial/v1'
                    $trialContext.native=@{level=$entry.level;originalRepository=$native}
                }
                Write-JsonFile $contextPath $trialContext
                $reference = @{id=$entry.id;directory=$trialDirectory;runId='synthetic-run';proposalSha256=$proposalSha256
                    context=@{path=$contextPath;sha256=Get-Sha256 $contextPath}
                    proposal=@{path=$summaryPath;sha256=Get-Sha256 $summaryPath}}
                if ($entry.id -ne 'pending') {
                    Write-JsonFile (Join-Path $runDirectory 'approval.json') @{proposalSha256=$proposalSha256}
                    [IO.File]::WriteAllText((Join-Path $runDirectory 'diff.patch'), 'synthetic ADE-owned diff; do not overwrite')
                    $outcome = @{schema='ui-profiler/independent-trial-outcome/v1';recommendationId=$entry.id
                        proposalSha256=$proposalSha256;state='failed';error='Synthetic failure; no verified workcopy.'
                        chainVerified=$false;adeChecksPassed=$false;originalRepositoryUnchanged=$true;comparison=$null}
                    if ($entry.id -ne 'failed') {
                        $outcome.state='verified-comparison';$outcome.chainVerified=$true;$outcome.adeChecksPassed=$true;$outcome.error=$null
                        foreach ($side in @('baseline', 'candidate')) {
                            $measurement = @{fixture='synthetic-only';sourceSha256=('8'*64)}
                            $candidateHash = if ($entry.id -eq 'mismatched') { '0'*64 } else { Get-Sha256 $source }
                            if ($entry.level -eq 'application') {
                                if ($side -eq 'candidate') { $measurement.sourceSha256=$candidateHash }
                            } else { $measurement.nativeRuntime=@{sourceSha256=$candidateHash} }
                            Write-JsonFile (Join-Path $trialDirectory "$side.json") $measurement
                        }
                        $candidateValue = if ($entry.id -eq 'no-improvement') { 12 } else { 8 }
                        $comparisonPath = Join-Path $trialDirectory 'comparison.json'
                        Write-JsonFile $comparisonPath @{schema='ui-profiler/comparison/v2'
                            baseline=@{path=(Join-Path $trialDirectory 'baseline.json');sha256=Get-Sha256 (Join-Path $trialDirectory 'baseline.json')}
                            candidate=@{path=(Join-Path $trialDirectory 'candidate.json');sha256=Get-Sha256 (Join-Path $trialDirectory 'candidate.json')}
                            metrics=@(
                                @{family='ticks';scope='scenario';metric='mean';unit='ms';primary=$true;status='available'
                                    baseline=10;candidate=$candidateValue;delta=($candidateValue-10)
                                    reductionPercent=(10*(10-$candidateValue));change=$(if ($candidateValue -lt 10) {'observed-improvement'} else {'regression'})},
                                @{family='gpu';scope='scenario';metric='max';unit='bytes';primary=$true;status='not_comparable'
                                    baseline=$null;candidate=$null;delta=$null;reductionPercent=$null;change='unavailable'}
                            )}
                        $outcome.comparison=@{path=$comparisonPath;sha256=Get-Sha256 $comparisonPath}
                    }
                    Write-JsonFile (Join-Path $trialDirectory 'outcome.json') $outcome
                }
                $reference
            }
        )
        $context = @{directory=$setDirectory;recommendations=$recommendations;trials=$trials}
        $beforeGitCount = [Environment]::GetEnvironmentVariable('GIT_CONFIG_COUNT', 'Process')
        $reference = Write-RecommendationTrialReport $context
        $report = Read-JsonFile $reference.path
        Expect ($report.actionItems.Count -eq 7) 'Local action items were omitted without remote opt-in.'
        Expect ($report.remoteActions.azureWorkItems -eq 'not-created') 'Local report implies an Azure task was created.'
        foreach ($id in @('planned', 'pending')) {
            $row = @($report.rows | Where-Object id -CEQ $id)[0]
            Expect (!$row.fix.actuallyApplied -and $row.fix.disposition -eq 'planned') 'Planned recommendation was called applied.'
        }
        $failed = @($report.rows | Where-Object id -CEQ 'failed')[0]
        Expect ($failed.actionItem.status -eq 'failed' -and $failed.fix.disposition -eq 'unavailable' -and $failed.fix.adeDiff) 'Failure omitted retained ADE evidence or invented an applied fix.'
        $noImprovement = @($report.rows | Where-Object id -CEQ 'no-improvement')[0]
        Expect ($noImprovement.fix.actuallyApplied -and $noImprovement.fix.measuredCandidateBound -and
            $noImprovement.actionItem.status -eq 'follow-up-no-improvement') 'No-improvement candidate was hidden or labelled a successful optimization.'
        $measured = @($report.rows | Where-Object id -CEQ 'measured')[0]
        Expect ($measured.fix.disposition -eq 'applied-local-measured' -and $measured.actionItem.status -eq 'locally-verified') 'Actual source-bound patch evidence missing.'
        Assert-FileHash $measured.fix.diff.path $measured.fix.diff.sha256
        Expect ($measured.fix.changedFiles.Count -eq 1 -and $measured.fix.changedFiles[0].candidateSha256) 'Changed-file byte hashes missing.'
        $mismatched = @($report.rows | Where-Object id -CEQ 'mismatched')[0]
        Expect ($mismatched.fix.disposition -eq 'applied-local-unverified' -and !$mismatched.fix.measuredCandidateBound) 'Unbound local source patch was claimed measured.'
        $markdownPath = Join-Path $setDirectory 'recommendations-report.md'
        $markdown = [IO.File]::ReadAllText($markdownPath)
        foreach ($text in @('Local actionable tasks', 'Actual fix disposition and local evidence', 'regression', 'unavailable',
            'no-demonstrated-primary-improvement', 'ADE-owned diff', 'candidate SHA-256')) {
            Expect ($markdown.Contains($text)) "Local report omitted $text."
        }
        Expect ([Environment]::GetEnvironmentVariable('GIT_CONFIG_COUNT', 'Process') -ceq $beforeGitCount) 'Read-only local scope check leaked Git configuration.'
        # Retain exact evidence before discarding a disposable workcopy.
        $measuredOutcomePath = Join-Path $measured.trial.directory 'outcome.json'
        $retainedOutcome = Read-JsonFile $measuredOutcomePath
        $retainedOutcome | Add-Member -NotePropertyName localFix -NotePropertyValue $measured.fix
        Write-JsonFile $measuredOutcomePath $retainedOutcome
        Remove-Item -LiteralPath (Join-Path $directory 'workcopies\measured') -Recurse -Force
        $retainedReport = Read-JsonFile (Write-RecommendationTrialReport $context).path
        Expect (@($retainedReport.rows | Where-Object id -CEQ 'measured')[0].fix.measuredCandidateBound) 'Completed patch evidence disappeared with a disposable workcopy.'
        [IO.File]::AppendAllText($measured.fix.diff.path, ' synthetic tamper')
        $tamperedReport = Read-JsonFile (Write-RecommendationTrialReport $context).path
        Expect (@($tamperedReport.rows | Where-Object id -CEQ 'measured')[0].fix.disposition -eq 'unavailable') 'Corrupt retained diff was accepted as an actual measured patch.'
        # Exercise default Report and early-failing Run: both must leave local reports,
        # without the publication switch or any ADE, GitHub or Azure invocation.
        $marker = Join-Path $setDirectory 'marker.json'
        Write-JsonFile $marker @{fixture='synthetic-only'}
        $markerReference = @{path=$marker;sha256=Get-Sha256 $marker}
        $context.schema='ui-profiler/recommendation-trials/v1'
        $context.module=$markerReference;$context.sourceInvestigation=$markerReference;$context.contracts=$markerReference
        $context.implementation=@{harness=$markerReference;runtime=$markerReference}
        $context.packSnapshot=@{directory=(Join-Path $directory 'snapshot');files=@('synthetic');sha256=('9'*64)}
        $context.originalRepository=$app;$context.originalSourceSha256='8'*64
        Write-JsonFile (Join-Path $setDirectory 'context.json') $context
        [void](Invoke-RecommendationTrials -Phase Report -Directory $setDirectory)
        Expect (Test-Path -LiteralPath $markdownPath) 'Default Report omitted its local Markdown.'
        $context.Remove('report')
        Write-JsonFile (Join-Path $setDirectory 'context.json') $context
        Remove-Item -LiteralPath $reference.path, $markdownPath
        $refused = $false
        try { [void](Invoke-RecommendationTrials -Phase Run -Directory $setDirectory -RecommendationId pending) }
        catch { $refused = $_.Exception.Message -like '*approval missing*' }
        Expect $refused 'Default Run did not stop safely before an unapproved synthetic operation.'
        Expect ((Test-Path -LiteralPath $reference.path) -and (Test-Path -LiteralPath $markdownPath)) 'Early-failing Run omitted the always-local report.'
        Write-Output 'Local report regression checks passed: planned/pending/failed/no-data, measured and no-improvement fixes, scoped diff/file hashes, retained/tampered evidence, default Report and early-failing Run; zero external actions.'
    } $temp
    if ($LocalReportOnly) { return }
    & (Get-Module ProfilerHarness) {
        $before = [pscustomobject]@{
            schema = 'ui-profiler/control-analysis/v1'
            time = @{ perInstanceCapabilities = @{ templating = @{
                status = 'available'; observedPairs = 4; observedElapsed = @{sum=20;max=8}
            } } }
        }
        $after = $before | ConvertTo-Json -Depth 10 | ConvertFrom-Json
        $after.time.perInstanceCapabilities.templating.observedElapsed.sum = 15
        $rows = @(Compare-ControlAnalysis $before $after)
        if ($rows[1].delta -ne -5 -or $rows[1].reductionPercent -ne 25) {
            throw 'Observed control template comparison lost its actual delta.'
        }
        $before.time.perInstanceCapabilities.templating.observedElapsed.sum = 0
        if ($null -ne @(Compare-ControlAnalysis $before $after)[1].reductionPercent) {
            throw 'Zero control baseline fabricated a percentage.'
        }
        $after.time.perInstanceCapabilities.templating.status = 'not captured/unsupported'
        $rows = @(Compare-ControlAnalysis $before $after)
        if ($null -ne $rows[1].candidate -or $null -ne $rows[1].delta) {
            throw 'Unavailable control attribution was converted to zero or an improvement.'
        }
    }
    $repo = Join-Path $temp 'repo'
    $control = Join-Path $repo 'controls\dev\NavigationView\NavigationView.cpp'
    $framework = Join-Path $repo 'dxaml\xcp\core\Layout.cpp'
    $test = Join-Path $repo 'controls\dev\NavigationView\tests\Protected.cpp'
    foreach ($path in @($control, $framework, $test)) {
        [void](New-Item -ItemType Directory -Force (Split-Path $path -Parent))
        [IO.File]::WriteAllText($path, 'synthetic original')
    }
    git -C $repo init --quiet
    git -C $repo add --all
    git -C $repo -c user.name='Harness Test' -c user.email='test@example.invalid' -c commit.gpgsign=false commit --quiet `
        -m "Synthetic native trial fixture`n`nCo-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda"
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic Git fixture failed.' }
    git -C $repo tag ade-baseline
    [IO.File]::WriteAllText($control, 'synthetic candidate')
    $unicodeRelative = 'controls/test/Assets/SimplePageWithNonÅscií.html'
    $unicodePath = Join-Path $repo $unicodeRelative.Replace('/', '\')
    [void](New-Item -ItemType Directory -Force (Split-Path $unicodePath -Parent))
    [IO.File]::WriteAllText($unicodePath, 'synthetic Unicode source')
    git -C $repo add -- $unicodeRelative
    $unicodeUntracked = 'untracked ü.cpp'
    [IO.File]::WriteAllText((Join-Path $repo $unicodeUntracked), 'synthetic Unicode untracked source')
    $unicodeSample = 'Samples/GalleryÅ'
    $fixtureRevision = git -C $repo rev-parse HEAD
    git -C $repo update-index --add --cacheinfo "160000,$fixtureRevision,$unicodeSample"
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic Unicode Gitlink fixture failed.' }
    $unicodeFiles = @(Get-NativeRepositoryFiles $repo)
    $unicodeHash = Get-SourceHash $repo $unicodeFiles
    $consoleEncoding = [Console]::OutputEncoding
    try {
        [Console]::OutputEncoding = [Text.Encoding]::GetEncoding(437)
        $legacyFiles = @(Get-NativeRepositoryFiles $repo)
        if ($unicodeRelative -cnotin $legacyFiles -or $unicodeUntracked -cnotin $legacyFiles -or
            $unicodeSample -cin $legacyFiles -or (Get-SourceHash $repo $legacyFiles) -cne $unicodeHash) {
            throw 'Legacy console encoding corrupted native source paths or their hash.'
        }
        $samples = @(Get-NativeSampleSubmodules $repo)
        if ($samples.Count -ne 1 -or $samples[0].path -cne $unicodeSample -or
            $samples[0].revision -cne $fixtureRevision -or [Console]::OutputEncoding.CodePage -ne 437) {
            throw 'Git source enumeration corrupted Unicode sample bindings or changed caller encoding.'
        }
        Refuses { Get-RepositoryFiles (Join-Path $temp 'missing-repository') } 'Failed Git enumeration was accepted.'
    } finally { [Console]::OutputEncoding = $consoleEncoding }
    git -C $repo update-index --force-remove -- $unicodeSample $unicodeRelative
    Remove-Item -LiteralPath $unicodePath, (Join-Path $repo $unicodeUntracked)
    $files = @(Get-RepositoryFiles $repo)
    $referenceHash = [Security.Cryptography.IncrementalHash]::CreateHash([Security.Cryptography.HashAlgorithmName]::SHA256)
    try {
        foreach ($relative in ($files | Sort-Object -CaseSensitive)) {
            $referenceHash.AppendData([Text.Encoding]::UTF8.GetBytes("$relative`0$(Get-Sha256 (Join-Path $repo $relative))`n"))
        }
        $expectedHash = [Convert]::ToHexString($referenceHash.GetHashAndReset()).ToLowerInvariant()
        if ((Get-SourceHash $repo $files) -ne $expectedHash) { throw 'Native-sized source hashing changed existing source identity.' }
    } finally { $referenceHash.Dispose() }
    Assert-ScopedCandidate $repo @('controls\dev\NavigationView\NavigationView.cpp') -Level control
    Refuses { Assert-ScopedCandidate $repo @('controls\dev\NavigationView\NavigationView.cpp') } 'Native scope entered the legacy app runner.'
    Refuses { Assert-ScopedCandidate $repo @('controls\dev\NavigationView\NavigationView.cpp') -Level framework } 'A control patch escaped into a framework trial.'
    Refuses { Assert-ScopedCandidate $repo @('controls\dev\NavigationView\tests\Protected.cpp') -Level control } 'Protected native tests were approved for editing.'
    [IO.File]::WriteAllText($framework, 'unapproved second fix')
    Refuses { Assert-ScopedCandidate $repo @('controls\dev\NavigationView\NavigationView.cpp') -Level control } 'An unrelated framework change entered a control trial.'

    $module = Get-Module NativeTrials
    & $module {
        param($directory)
        $context = [pscustomobject]@{ directory = $directory }
        $failed = $false
        try { Assert-NativeTrialApproval $context } catch { $failed = $true }
        if (!$failed) { throw 'Native builds/captures ran without an exact-hash proposal approval.' }
        $run = Join-Path $directory 'ade-runs\synthetic-run'
        [void](New-Item -ItemType Directory -Force $run)
        $proposalPath = Join-Path $run 'proposal.json'
        Write-JsonFile $proposalPath @{ fixture = 'Explicitly synthetic proposal, never a real authorization.' }
        $sha256 = Get-Sha256 $proposalPath
        Write-JsonFile (Join-Path $directory 'ade-run.json') @{runId='synthetic-run';proposalSha256=$sha256}
        Write-JsonFile (Join-Path $run 'approval.json') @{proposalSha256=('0'*64)}
        $failed = $false
        try { Assert-NativeTrialApproval $context } catch { $failed = $true }
        if (!$failed) { throw 'A stale proposal hash authorized native execution.' }
        Write-JsonFile (Join-Path $run 'approval.json') @{proposalSha256=$sha256}
        Assert-NativeTrialApproval $context
    } $temp

    $recommendationPath = Join-Path $temp 'recommendation.json'
    Write-JsonFile $recommendationPath @{ coverage = @{application='App';control='Control';framework='Framework'}
        runtimeSourceParity = 'Unproven discovery binary; source-matched runtime required.'
        recommendations = @(
            @{id='app';level='application';title='App change';verdict='propose';summary='Independent app variant.'},
            @{id='control';level='control';title='Control change';verdict='recommend';summary='Independent native variant.'},
            @{id='framework';level='framework';title='No engine edit';verdict='insufficient-evidence';summary='No invented native fix.'}
        ) }
    $context = @{ directory = $temp; recommendations = @{path=$recommendationPath;sha256=Get-Sha256 $recommendationPath}; trials=@() }
    $reference = Write-RecommendationTrialReport $context
    $report = Read-JsonFile $reference.path
    if ($report.rows.Count -ne 3 -or $report.rows[1].state -ne 'not-prepared' -or
        $report.rows[2].state -ne 'insufficient-evidence') { throw 'Independent native support was conflated with insufficient evidence.' }
    Write-Output 'Native trial checks passed: Unicode paths and Gitlinks under legacy console encoding, repository/layer scope, protected files, approval gate, separate evidence confidence.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}

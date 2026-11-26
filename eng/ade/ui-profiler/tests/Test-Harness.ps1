# Focused offline tests; these never launch an app, call an agent, or spend a capture.
#Requires -Version 7
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
& powershell.exe -NoProfile -NonInteractive -File "$PSScriptRoot\Test-UiSelection.ps1"
if ($LASTEXITCODE -ne 0) { throw 'UIA selection replay tests failed.' }
& "$PSScriptRoot\Test-GuiOwnership.ps1"
$temp = Join-Path ([IO.Path]::GetTempPath()) ('ui-profiler-tests-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $temp | Out-Null
$passed = 0
function Expect([bool]$Condition, [string]$Message) {
    if (!$Condition) { throw $Message }
}
function Expect-Failure([scriptblock]$Action) {
    $failed = $false
    try { & $Action } catch { $failed = $true }
    Expect $failed 'Expected an evidence/budget failure.'
}
function New-Fixture([string]$Name, [double]$P95) {
    $db = Join-Path $temp "$Name.sqlite"
    $actions = Join-Path $temp "$Name.actions.json"
    [IO.File]::WriteAllText($db, 'fixture bytes, not a real recording')
    Write-JsonFile $actions @{
        state = 'complete'; window = @{ dpi = 96; width = 1280; height = 900 }
        steps = @(@{ id = 'expand'; action = 'expand'; state = 'passed'; from = 'Collapsed'; to = 'Expanded' })
    }
    [pscustomobject]@{
        schema = 'ui-profiler/measurement/v1'; state = 'complete'
        scenarioSha256 = 'scenario'; profilerSha256 = 'profiler'; frameworkSha256 = 'framework'
        database = @{ path = $db; sha256 = Get-Sha256 $db }
        actions = @{ path = $actions; sha256 = Get-Sha256 $actions }
        health = @{ lostByEtw = 0; droppedByProfiler = 0; completeFromSeq = $null; ended = $true }
        metrics = @{ scenarioElapsedMs = 100; p95TickMs = $P95; meanTickMs = 4; totalTickMs = 40; maxTickMs = 20; completedTicks = 10 }
    }
}
try {
    Expect ((Get-ScenarioTimeoutSeconds @{ steps = @(@{ settleMilliseconds = 0 }) }) -eq 90) `
        'Short scenarios lost their existing minimum timeout.'
    $longScenario = @{ steps = @(1..32 | ForEach-Object { @{ settleMilliseconds = 2000 } }) }
    Expect ((Get-ScenarioTimeoutSeconds $longScenario) -eq 734) `
        'The full legal plan must include selector, transition and settle time for every step.'
    foreach ($invalid in @($null, -1, 2001, 0.5, '1000', $true)) {
        Expect-Failure { Get-ScenarioTimeoutSeconds @{ steps = @(@{ settleMilliseconds = $invalid }) } }
    }
    Expect-Failure { Get-ScenarioTimeoutSeconds @{ steps = @() } }
    Expect-Failure { Get-ScenarioTimeoutSeconds @{ steps = @(1..33 | ForEach-Object { @{ settleMilliseconds = 0 } }) } }
    $passed++

    $before = New-Fixture 'before' 20
    $after = New-Fixture 'after' 15
    $comparison = Compare-TimingMeasurements $before $after
    $row = $comparison.metrics | Where-Object metric -eq p95TickMs
    Expect ($row.delta -eq -5 -and $row.reductionPercent -eq 25) 'Incorrect observed delta/percentage.'
    $passed++

    $guiBefore = New-Fixture 'gui-before' 20
    $guiAfter = New-Fixture 'gui-after' 15
    foreach ($measurement in @($guiBefore, $guiAfter)) {
        $measurement | Add-Member NoteProperty capture @{
            backend = 'gui'; profiler = @{ payloadSha256 = $before.database.sha256 }
            sourceRecording = @{ sha256 = $measurement.database.sha256 }
        }
    }
    [void](Compare-TimingMeasurements $guiBefore $guiAfter)
    Expect-Failure { Compare-TimingMeasurements $before $guiAfter }
    $guiAfter.capture.profiler.payloadSha256 = Get-Sha256 $PSCommandPath
    Expect-Failure { Compare-TimingMeasurements $guiBefore $guiAfter }
    $guiAfter.capture.sourceRecording.sha256 = 'wrong'
    Expect-Failure { Assert-Measurement $guiAfter }
    $passed++

    $before.metrics.p95TickMs = 0
    $row = (Compare-TimingMeasurements $before $after).metrics | Where-Object metric -eq p95TickMs
    Expect ($null -eq $row.reductionPercent) 'Zero baseline must not produce a percentage.'
    $before.metrics.p95TickMs = 20
    $passed++

    $after.scenarioSha256 = 'other'
    Expect-Failure { Compare-TimingMeasurements $before $after }
    $after.scenarioSha256 = 'scenario'
    $after.health.completeFromSeq = 12
    Expect-Failure { Compare-TimingMeasurements $before $after }
    $after.health.completeFromSeq = $null
    $after.health.lostByEtw = $null
    Expect-Failure { Compare-TimingMeasurements $before $after }
    $after.health.lostByEtw = 0
    $after.metrics.p95TickMs = [double]::NaN
    Expect-Failure { Compare-TimingMeasurements $before $after }
    $after.metrics.p95TickMs = 15
    $passed++

    $audit = Read-JsonFile $after.actions.path
    $audit.steps[0].state = 'failed'
    Write-JsonFile $after.actions.path $audit
    $after.actions.sha256 = Get-Sha256 $after.actions.path
    Expect-Failure { Compare-TimingMeasurements $before $after }
    $audit.steps[0].state = 'passed'
    $audit.steps[0].from = 'Expanded'
    Write-JsonFile $after.actions.path $audit
    $after.actions.sha256 = Get-Sha256 $after.actions.path
    Expect-Failure { Compare-TimingMeasurements $before $after }
    $passed++

    [IO.File]::AppendAllText($before.database.path, 'changed')
    Expect-Failure { Assert-Measurement $before }
    $passed++

    $budget = Join-Path $temp 'capture'
    Reserve-Capture $budget 'source-a'
    Expect-Failure { Reserve-Capture $budget 'source-a' }
    Expect-Failure { Reserve-Capture $budget 'source-b' }
    $passed++

    $stats = Get-TickStatistics @(
        [pscustomobject]@{ TickMilliseconds = $null },
        [pscustomobject]@{ TickMilliseconds = 2 },
        [pscustomobject]@{ TickMilliseconds = 8 })
    Expect ($stats.completedTicks -eq 2 -and $stats.p95TickMs -eq 8 -and $stats.meanTickMs -eq 5) 'Incorrect complete-tick selection.'
    Expect-Failure { Get-TickStatistics @([pscustomobject]@{ TickMilliseconds = $null }) }
    $passed++

    $source = Join-Path $temp 'source'
    New-Item -ItemType Directory $source | Out-Null
    foreach ($file in @('a.cs', 'b.xaml', 'c.cs', 'README.txt')) {
        [IO.File]::WriteAllText((Join-Path $source $file), 'baseline')
    }
    git -C $source init -q
    git -C $source add -- a.cs b.xaml c.cs README.txt
    git -C $source -c user.name='Harness Test' -c user.email='test@example.invalid' commit -qm "Test baseline`n`nCo-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: 895bb146-abbd-4f74-a580-f5970b30689d"
    if ($LASTEXITCODE -ne 0) { throw 'Could not create the isolated source fixture.' }
    git -C $source tag ade-baseline
    Expect-Failure { Assert-MinimalCandidate $source }
    [IO.File]::WriteAllText((Join-Path $source 'a.cs'), 'candidate')
    Assert-MinimalCandidate $source
    [IO.File]::WriteAllText((Join-Path $source 'b.xaml'), 'candidate')
    Assert-MinimalCandidate $source
    [IO.File]::WriteAllText((Join-Path $source 'c.cs'), 'candidate')
    Expect-Failure { Assert-MinimalCandidate $source }
    [IO.File]::WriteAllText((Join-Path $source 'c.cs'), 'baseline')
    [IO.File]::WriteAllText((Join-Path $source 'README.txt'), 'candidate')
    Expect-Failure { Assert-MinimalCandidate $source }
    [IO.File]::WriteAllText((Join-Path $source 'README.txt'), 'baseline')
    [IO.File]::WriteAllText((Join-Path $source 'new.cs'), 'new file')
    Expect-Failure { Assert-MinimalCandidate $source }
    $passed++

    $payload = Join-Path $temp 'build-output'
    $deployment = Join-Path $temp 'deployment'
    New-Item -ItemType Directory (Join-Path $payload 'Controls') | Out-Null
    foreach ($file in @('App.pri', 'App.xbf', 'Controls\Sample.xbf', 'runtime.dll')) {
        [IO.File]::WriteAllText((Join-Path $payload $file), "payload for $file")
    }
    Copy-Deployment $payload $deployment
    foreach ($file in @('App.pri', 'App.xbf', 'Controls\Sample.xbf', 'runtime.dll')) {
        Expect ((Get-Sha256 (Join-Path $payload $file)) -eq (Get-Sha256 (Join-Path $deployment $file))) 'Deployment lost or altered compiled resources.'
    }
    Expect-Failure { Copy-Deployment $payload (Join-Path $payload 'recursive-destination') }
    $passed++

    $script = @'
import json, sys
print(json.dumps({"type":"snapshot","data":{"path":"initial.json"}}), flush=True)
for line in sys.stdin:
    if line.strip() == "snapshot":
        print(json.dumps({"type":"snapshot","data":{"path":"end.json"}}), flush=True)
    elif line.strip() == "stop":
        break
'@
    $fake = New-LoggedProcess (Get-Command python -ErrorAction Stop).Path @('-u', '-c', $script) $temp
    $pump = @{
        process = $fake; pending = $fake.StandardOutput.ReadLineAsync()
        writer = [IO.StreamWriter]::new((Join-Path $temp 'synthetic.ndjson'))
        items = [Collections.Generic.List[object]]::new(); ready = $null; summary = $null
    }
    try {
        [void]$pump.pending.Wait(5000)
        Receive-ProfilerOutput $pump
        Expect ($pump.items.Count -eq 1) 'Synthetic connection did not publish its initial snapshot.'
        Request-ProfilerSnapshot $pump 5
        Expect ($pump.items.Count -eq 2 -and $pump.items[1].data.path -eq 'end.json') 'End snapshot did not use the existing connection.'
        Expect-Failure { Complete-ProfilerOutput $pump }
        $fake.StandardInput.WriteLine('stop')
        $fake.StandardInput.Flush()
        Expect ($fake.WaitForExit(5000)) 'Synthetic connection did not stop.'
        while ($null -ne $pump.pending) {
            Expect ($pump.pending.Wait(5000)) 'Synthetic output did not drain.'
            Receive-ProfilerOutput $pump
        }
        Complete-ProfilerOutput $pump
        $streamPath = Join-Path $temp 'synthetic.ndjson'
        $streamHash = Get-Sha256 $streamPath
        Expect ($streamHash.Length -eq 64) 'Finalized stream could not be hashed.'
        $streamItems = @(Get-Content -LiteralPath $streamPath | ForEach-Object { $_ | ConvertFrom-Json })
        Expect ($streamItems.Count -eq 2 -and $streamItems[1].data.path -eq 'end.json') 'Finalization lost captured output.'
    } finally {
        if (!$fake.HasExited) { Stop-Process -Id $fake.Id }
        if ($null -ne $pump.writer) { $pump.writer.Dispose() }
        $fake.Dispose()
    }
    $passed++
    $knowledge = Join-Path $temp 'knowledge'
    $launcher = Join-Path $knowledge 'tools\WinUIProfiler\scripts\Start-McpServer.ps1'
    New-Item -ItemType Directory -Force (Split-Path $launcher -Parent) | Out-Null
    [IO.File]::WriteAllText($launcher, '# synthetic metadata fixture; never executed')
    $assembly = Join-Path $temp 'WinUIProfiler.Mcp.dll'
    $configuration = New-ProfilerMcpConfiguration $knowledge (Get-Command python -ErrorAction Stop).Path `
        -NativeAssembly $assembly
    $serialized = $configuration | ConvertTo-Json -Depth 8 | ConvertFrom-Json
    Expect ($serialized.mcpServers.'winui-profiler'.args -is [array]) 'Native MCP args must remain a JSON array.'
    Expect ($serialized.mcpServers.'winui-profiler'.args.Count -eq 1 -and
        $serialized.mcpServers.'winui-profiler'.args[0] -eq $assembly) 'Native MCP deployment argument was altered.'
    $passed++
    $independent = Join-Path $temp 'independent-budgets'
    Reserve-Capture (Join-Path $independent 'baseline') 'original-source'
    Reserve-Capture (Join-Path $independent 'trials\first\candidate') 'first-candidate'
    Reserve-Capture (Join-Path $independent 'trials\second\candidate') 'second-candidate'
    Expect-Failure { Reserve-Capture (Join-Path $independent 'baseline') 'original-source' }
    Expect-Failure { Reserve-Capture (Join-Path $independent 'trials\first\candidate') 'first-candidate' }
    Expect-Failure { Reserve-Capture (Join-Path $independent 'trials\second\candidate') 'second-candidate' }
    $passed++

    $selectionError = $null
    try {
        & "$PSScriptRoot\..\scripts\Start-Investigation.ps1" -Phase Run `
            -Directory (Join-Path $temp 'uninitialized-selection') -RecommendationId @('first', 'FIRST')
    } catch { $selectionError = $_.Exception.Message }
    Expect ($selectionError -eq 'Recommendation ids must be unique; no trial has been started.') `
        'Duplicate trial ids were not rejected before reading context or starting a trial.'
    $passed++

    $recommendationsPath = Join-Path $temp 'recommendations.json'
    $entry = @{ id = 'first'; title = 'Synthetic first recommendation'; verdict = 'propose'; summary = 'Synthetic fixture.'
        observations = @('Synthetic evidence, not an actual capture.'); hypotheses = @('Independent variants.')
        risks = @('Do not interpret this fixture as a generated fix.'); files = @(@{ path = 'a.cs'; change = 'Fixture.' }) }
    $second = $entry.Clone(); $second.id = 'second'
    $unsupported = $entry.Clone(); $unsupported.id = 'unsupported'; $unsupported.verdict = 'insufficient-evidence'; $unsupported.files = @()
    Write-JsonFile $recommendationsPath @{ schema = 'ui-profiler/recommendations/v1'; summary = 'Synthetic report contract.'
        recommendations = @($entry, $second, $unsupported); risks = @('No app recording.') }
    $trialDirectory = Join-Path $temp 'trials\first'
    $syntheticContext = Join-Path $trialDirectory 'pack\knowledge\trial-context.json'
    $syntheticProposal = Join-Path $trialDirectory 'ade-run.json'
    New-Item -ItemType Directory -Force (Split-Path $syntheticContext -Parent) | Out-Null
    $shared = @{ path = $recommendationsPath; sha256 = Get-Sha256 $recommendationsPath }
    $reportContext = [pscustomobject]@{ directory = $temp; originalRevision = 'synthetic-revision'
        originalRepository = $source; originalSourceSha256 = 'synthetic-source'; request = $shared; contracts = $shared
        baseline = @{ path = $before.database.path; sha256 = Get-Sha256 $before.database.path }
        recommendations = $shared
        trials = @(); report = $null }
    $trialContext = @{ schema = 'ui-profiler/trial/v1'; strategy = 'independent'; recommendationId = 'first'
        directory = $trialDirectory; investigationDirectory = $temp; originalRepository = $source
        originalRevision = $reportContext.originalRevision; originalSourceSha256 = $reportContext.originalSourceSha256
        request = $shared; contracts = $shared; recommendations = $shared; baseline = $reportContext.baseline }
    Write-JsonFile $syntheticContext $trialContext
    Write-JsonFile $syntheticProposal @{ fixture = $true; status = 'AwaitingApproval'
        runId = 'synthetic-test-only'; proposalSha256 = ('f' * 64) }
    $reportContext.trials = @(@{ id = 'first'; directory = $trialDirectory
            context = @{ path = $syntheticContext; sha256 = Get-Sha256 $syntheticContext }
            proposal = @{ path = $syntheticProposal; sha256 = Get-Sha256 $syntheticProposal }
            runId = 'synthetic-test-only'; proposalSha256 = ('f' * 64) })
    $outcomePath = Join-Path $trialDirectory 'outcome.json'
    $outcome = @{ schema = 'ui-profiler/trial-outcome/v1'; recommendationId = 'first'; state = 'failed'
        baselineSha256 = $reportContext.baseline.sha256; contextSha256 = Get-Sha256 $syntheticContext
        proposalSha256 = ('f' * 64); adeRun = 'synthetic-test-only'; error = 'Explicit synthetic failure'; comparison = $null; counts = $null
        adeChecksPassed = $false; chainVerified = $false; originalRepositoryUnchanged = $true }
    Write-JsonFile $outcomePath $outcome
    $report = Write-RecommendationReport $reportContext
    $reportContext.report = @{ path = $report.path; sha256 = $report.sha256 }
    $verified = Assert-RecommendationReport $reportContext
    Expect ($verified.rows.Count -eq 3 -and $verified.rows[0].state -eq 'failed' -and
        $verified.rows[1].state -eq 'pending' -and $verified.rows[2].state -eq 'insufficient-evidence') `
        'Report omitted failed, pending or unsupported recommendations.'
    [IO.File]::AppendAllText($outcomePath, 'tampered')
    Expect-Failure { Assert-RecommendationReport $reportContext }
    $outcome.state = 'verified-comparison'
    Write-JsonFile $outcomePath $outcome
    Expect-Failure { Write-RecommendationReport $reportContext }
    $passed++

    $trialContext.recommendationId = 'second'
    Write-JsonFile $syntheticContext $trialContext
    $reportContext.trials[0].context.sha256 = Get-Sha256 $syntheticContext
    Expect-Failure { Assert-TrialReference $reportContext $reportContext.trials[0] }
    $trialContext.recommendationId = 'first'
    Write-JsonFile $syntheticContext $trialContext
    $reportContext.trials[0].context.sha256 = Get-Sha256 $syntheticContext
    $reportContext.trials[0].runId = 'different-synthetic-run'
    Expect-Failure { Assert-TrialReference $reportContext $reportContext.trials[0] }
    $reportContext.trials[0].runId = 'synthetic-test-only'
    $reportContext.trials[0].directory = Join-Path $temp 'trials\second'
    Expect-Failure { Assert-TrialReference $reportContext $reportContext.trials[0] }
    $reportContext.trials[0].directory = $trialDirectory
    [void](Assert-TrialReference $reportContext $reportContext.trials[0])
    $passed++

    $baselinePath = Join-Path $temp 'report-baseline.json'
    $candidatePath = Join-Path $trialDirectory 'report-candidate.json'
    Write-JsonFile $baselinePath (New-Fixture 'report-baseline' 20)
    Write-JsonFile $candidatePath (New-Fixture 'report-candidate' 15)
    $reportContext.baseline = @{ path = $baselinePath; sha256 = Get-Sha256 $baselinePath }
    $trialContext.baseline = $reportContext.baseline
    Write-JsonFile $syntheticContext $trialContext
    $reportContext.trials[0].context.sha256 = Get-Sha256 $syntheticContext
    $comparisonPath = Join-Path $trialDirectory 'comparison.json'
    $tableComparison = @{
        schema = 'ui-profiler/comparison/v2'; baseline = $reportContext.baseline
        candidate = @{ path = $candidatePath; sha256 = Get-Sha256 $candidatePath }
        metrics = @(
            @{ family = 'memory'; scope = 'synthetic'; metric = 'peakWorkingSet'; unit = 'MiB'
                baseline = 40; candidate = 30; delta = -10; primary = $true; status = 'available'; change = 'lower' },
            @{ family = 'gpu'; scope = 'synthetic'; metric = 'missingUsage'; unit = 'MiB'
                baseline = $null; candidate = $null; delta = $null; primary = $true; status = 'unavailable'; change = 'unknown' },
            @{ family = 'cpu'; scope = 'synthetic'; metric = 'secondary-detail'; unit = '%'
                baseline = 1; candidate = 2; delta = 1; primary = $false; status = 'available'; change = 'higher' })
    }
    Write-JsonFile $comparisonPath $tableComparison
    $outcome.baselineSha256 = $reportContext.baseline.sha256
    $outcome.contextSha256 = $reportContext.trials[0].context.sha256
    $outcome.state = 'verified-comparison'
    $outcome.adeChecksPassed = $true
    $outcome.chainVerified = $true
    $outcome.comparison = @{ path = $comparisonPath; sha256 = Get-Sha256 $comparisonPath }
    $outcome.counts = @{ improved = 1; regressed = 1; unavailable = 1 }
    Write-JsonFile $outcomePath $outcome
    $report = Write-RecommendationReport $reportContext
    $reportContext.report = @{ path = $report.path; sha256 = $report.sha256 }
    $verified = Assert-RecommendationReport $reportContext
    $markdown = [IO.File]::ReadAllText((Join-Path $temp 'recommendations-report.md'))
    Expect ($verified.rows[0].state -eq 'verified-comparison' -and $verified.rows[0].summaryMetrics.Count -eq 2) `
        'Successful trial did not retain primary metrics.'
    Expect ($markdown.Contains('| 40 | 30 | -10 | available / lower |') -and
        $markdown.Contains('| unavailable | unavailable | unavailable | unavailable / unknown |') -and
        !$markdown.Contains('secondary-detail') -and $markdown.Contains($comparisonPath)) `
        'Aggregate Markdown lost observed deltas, missing-data states or its full-comparison reference.'
    $tableComparison.baseline = $tableComparison.candidate
    Write-JsonFile $comparisonPath $tableComparison
    $outcome.comparison.sha256 = Get-Sha256 $comparisonPath
    Write-JsonFile $outcomePath $outcome
    Expect-Failure { Write-RecommendationReport $reportContext }
    $passed++
    Write-Output "$passed focused harness tests passed; zero captures."
    & "$PSScriptRoot\Test-ProcessInput.ps1"
    & "$PSScriptRoot\Test-Continuation.ps1"
    & "$PSScriptRoot\Test-ExpertSupplement.ps1"
    & "$PSScriptRoot\Test-ThreeLevelReport.ps1"
    & "$PSScriptRoot\Test-NativeRuntime.ps1"
    & "$PSScriptRoot\Test-NativeTrials.ps1"
    & "$PSScriptRoot\Test-RecommendationTrials.ps1"
    & "$PSScriptRoot\Test-ProfiledDraftPr.ps1"
    & python -B "$PSScriptRoot\Test-Analysis.py"
    if ($LASTEXITCODE -ne 0) { throw 'Multidimensional evidence tests failed.' }
    & python -B "$PSScriptRoot\Test-ControlAnalysis.py"
    if ($LASTEXITCODE -ne 0) { throw 'Control-scoped evidence tests failed.' }
    & python -B "$PSScriptRoot\Test-AgentContracts.py"
    if ($LASTEXITCODE -ne 0) { throw 'Agent/tool contract tests failed.' }
    & python -B "$PSScriptRoot\Test-GuiRecording.py"
    if ($LASTEXITCODE -ne 0) { throw 'GUI SQLite evidence tests failed.' }
    & "$PSScriptRoot\Test-Contracts.ps1"
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}

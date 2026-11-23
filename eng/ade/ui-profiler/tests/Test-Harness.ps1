# Focused offline tests; these never launch an app, call an agent, or spend a capture.
#Requires -Version 7
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
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
    $before = New-Fixture 'before' 20
    $after = New-Fixture 'after' 15
    $comparison = Compare-TimingMeasurements $before $after
    $row = $comparison.metrics | Where-Object metric -eq p95TickMs
    Expect ($row.delta -eq -5 -and $row.reductionPercent -eq 25) 'Incorrect observed delta/percentage.'
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
        $fake.StandardInput.WriteLine('stop')
        $fake.StandardInput.Flush()
        Expect ($fake.WaitForExit(5000)) 'Synthetic connection did not stop.'
    } finally {
        if (!$fake.HasExited) { Stop-Process -Id $fake.Id }
        $pump.writer.Dispose()
        $fake.Dispose()
    }
    $passed++
    Write-Output "$passed focused harness tests passed; zero captures."
    & python -B "$PSScriptRoot\Test-Analysis.py"
    if ($LASTEXITCODE -ne 0) { throw 'Multidimensional evidence tests failed.' }
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}

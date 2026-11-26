# Exercise the real ADE approval/verification boundary with labelled inert test adapters, never a model.
#Requires -Version 7
[CmdletBinding()]
param(
    [ValidateSet('request-path-isolation', 'frozen-plan-input-integrity', 'bug-target-must-fail-at-baseline',
        'approval-missing-wrong-and-stale-hash', 'approved-input-drift-is-rejected',
        'protected-file-edits-are-detected-and-rejected', 'failing-target-cannot-report-success',
        'regression-check-failure-cannot-report-success', 'successful-isolated-run-leaves-original-unchanged',
        'approved-file-scope-allows-three-and-rejects-fourth', 'recommendation-trials-remain-independent')]
    [string[]]$ContractName = @()
)
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
$runtime = if ($env:ADE_RUNTIME_DIR) { $env:ADE_RUNTIME_DIR } else { Join-Path $env:LOCALAPPDATA 'ade\runtime\0.1.0' }
$exe = Join-Path $runtime 'ade.exe'
if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'ADE 0.1.0 is required for its real boundary contract tests.' }
$launcher = [IO.Path]::GetFullPath("$PSScriptRoot\..\scripts\ade.ps1")
$pwsh = (Get-Command pwsh -ErrorAction Stop).Source
$temp = (New-Item -ItemType Directory (Join-Path ([IO.Path]::GetTempPath()) ('upc-' + [Guid]::NewGuid().ToString('N')))).FullName
$passed = 0
$saved = @{}
foreach ($name in @('GIT_AUTHOR_NAME', 'GIT_AUTHOR_EMAIL', 'GIT_COMMITTER_NAME', 'GIT_COMMITTER_EMAIL')) {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
    [Environment]::SetEnvironmentVariable($name, $(if ($name.EndsWith('EMAIL')) { 'test@example.invalid' } else { 'Harness Contract Test' }))
}
function Expect([bool]$Condition, [string]$Message) { if (!$Condition) { throw $Message } }
function Refuses([scriptblock]$Action) {
    $refused = $false
    try { & $Action } catch { $refused = $true }
    Expect $refused 'The harness accepted an operation that its contract forbids.'
}
function Contract([string]$Name, [scriptblock]$Body) {
    if ($ContractName.Count -gt 0 -and $Name -notin $ContractName) { return }
    & $Body
    $script:passed++
    Write-Output "PASS $Name"
}
function Platform([string[]]$Arguments) {
    $output = @(& $script:pwsh -NoProfile -NonInteractive -File $script:launcher @Arguments)
    $line = $output | Where-Object { $_ -like 'ADE-RESULT:*' } | Select-Object -Last 1
    if (!$line) { throw "No ADE result: $($output -join [Environment]::NewLine)" }
    $line.Substring(12) | ConvertFrom-Json
}
function Fixture([string]$Name, [string]$Body, [string]$Type = 'bug', [string]$Target = '',
    [string[]]$AllowedPaths = @(), [string]$SourceRepository = '', [string]$RunsDirectory = '') {
    $root = Join-Path $script:temp $Name
    $repo = if ($SourceRepository) { [IO.Path]::GetFullPath($SourceRepository) } else { Join-Path $root 'repo' }
    $pack = Join-Path $root 'pack'
    $runs = if ($RunsDirectory) { [IO.Path]::GetFullPath($RunsDirectory) } else { Join-Path $root 'runs' }
    foreach ($path in @($repo, $runs)) {
        Expect ($path.StartsWith("$script:temp\", [StringComparison]::OrdinalIgnoreCase)) 'Fixtures must stay inside their owned synthetic directory.'
    }
    New-Item -ItemType Directory (Join-Path $pack 'knowledge') | Out-Null
    if ($SourceRepository) {
        Expect (Test-Path -LiteralPath $repo -PathType Container) 'The shared original fixture is missing.'
    } else {
        New-Item -ItemType Directory (Join-Path $repo 'tests') | Out-Null
        [IO.File]::WriteAllText((Join-Path $repo 'App.cs'), 'broken')
        [IO.File]::WriteAllText((Join-Path $repo 'Guard.txt'), 'keep')
        [IO.File]::WriteAllText((Join-Path $repo 'tests\Protected.cs'), 'locked')
        if ($AllowedPaths.Count -gt 0) {
            [IO.File]::WriteAllText((Join-Path $repo 'ViewModel.cs'), 'original-model')
            [IO.File]::WriteAllText((Join-Path $repo 'View.xaml'), 'original-view')
            [IO.File]::WriteAllText((Join-Path $repo 'Other.cs'), 'outside-original')
        }
        git -C $repo init -q
        if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the owned synthetic repository.' }
        git -C $repo add -A
        if ($LASTEXITCODE -ne 0) { throw 'Could not index synthetic baseline files.' }
        git -C $repo -c core.hooksPath=NUL commit -qm "Synthetic contract baseline`n`nCo-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda"
        if ($LASTEXITCODE -ne 0) { throw 'Could not commit the owned synthetic baseline.' }
    }
    [IO.File]::WriteAllText((Join-Path $pack 'knowledge\contract.md'), 'Synthetic harness contract fixture; no model, app build, UI or capture.')
    $adapter = Join-Path $root 'test-adapter.ps1'
    [IO.File]::WriteAllText($adapter, $Body + "`nWrite-Output 'Inert SCRIPT contract-test executor, not an AI agent.'")
    $cfg = Read-JsonFile "$PSScriptRoot\..\pack\ade.team.json"
    $cfg.checks = @(@{
        id = 'guard'; command = "if ((Get-Content -LiteralPath 'Guard.txt' -Raw) -eq 'keep') { exit 0 }; exit 1"
        timeoutSeconds = 15
    })
    $cfg.protectedPaths = @('tests')
    $cfg.limits.maxRounds = 1
    $cfg.limits.roundTimeoutSeconds = 30
    if ($AllowedPaths.Count -gt 0) {
        $tools = Join-Path $pack 'tools'
        New-Item -ItemType Directory $tools | Out-Null
        $module = Join-Path $tools 'ProfilerHarness.psm1'
        Copy-Item -LiteralPath "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Destination $module
        $scope = ($AllowedPaths | ForEach-Object { "'" + $_.Replace("'", "''") + "'" }) -join ', '
        $scopeCheck = @"
`$ErrorActionPreference = 'Stop'
`$changed = @(git diff --name-status ade-baseline)
if (`$LASTEXITCODE -ne 0) { exit 1 }
`$new = @(git ls-files --others --exclude-standard)
if (`$LASTEXITCODE -ne 0) { exit 1 }
if (`$changed.Count -eq 0 -and `$new.Count -eq 0) { exit 0 }
Import-Module '$($module.Replace("'", "''"))' -Force -DisableNameChecking
Assert-ScopedCandidate (Get-Location).Path @($scope)
"@
        $cfg.checks += @{ id = 'approved-file-scope'; command = $scopeCheck; timeoutSeconds = 15 }
    }
    Write-JsonFile (Join-Path $pack 'ade.team.json') $cfg
    if (!$Target) { $Target = "if ((Get-Content -LiteralPath 'App.cs' -Raw) -eq 'fixed') { exit 0 }; exit 1" }
    $arguments = @('propose', '--pack', $pack, '--repo', $repo, '--type', $Type, '--task', 'Synthetic contract check',
        '--check', $Target, '--executor', "script:$adapter", '--no-model-plan', '--runs-dir', $runs)
    if ($AllowedPaths.Count -gt 0) {
        $workstreams = Join-Path $root 'workstreams'
        $ws = Platform @('workstream', 'new', '--pack', $pack, '--repo', $repo, '--type', $Type,
            '--name', $Name, '--goal', 'Inert SCRIPT fixture: change only the explicitly approved source files.',
            '--workstreams-dir', $workstreams)
        Expect $ws.ok 'The synthetic ADE workstream was not created.'
        $maxChangedPaths = $AllowedPaths.Count
        # ADE 0.1.0 has no file-count-limit field; its real check enforces this hash-bound scope.
        [IO.File]::WriteAllText((Join-Path $ws.workstream 'notes.md'), @"
# Inert SCRIPT contract fixture, not an expert or AI recommendation

Approved existing source-file allowlist (maxChangedPaths = $maxChangedPaths):
$(($AllowedPaths | ForEach-Object { "- $_" }) -join "`n")

The scope-derived change budget has no arbitrary two-file cap.
Do not change other files, build or launch an app, invoke a model, or capture profiler/ETW evidence.
"@)
        $arguments += @('--workstream', $Name, '--workstreams-dir', $workstreams)
    }
    $proposal = Platform $arguments
    @{ root = $root; repo = $repo; pack = $pack; runs = $runs; proposal = $proposal }
}
function Approve-Fixture($Case) {
    $result = Platform @('approve', '--run', $Case.proposal.runId, '--hash', $Case.proposal.proposalSha256, '--runs-dir', $Case.runs)
    Expect $result.ok 'The exact synthetic proposal hash was not accepted.'
    $approvalPath = Join-Path $Case.proposal.runDir 'approval.json'
    $approval = Read-JsonFile $approvalPath
    Expect ($approval.proposalSha256 -eq $Case.proposal.proposalSha256 -and
        $approval.proposalSha256 -eq (Get-Sha256 (Join-Path $Case.proposal.runDir 'proposal.json'))) 'ADE retained approval for a different proposal hash.'
    Write-Verbose "Synthetic approval at ${approvalPath}: $(Get-Content -LiteralPath $approvalPath -Raw)"
}
function Run-Fixture($Case) { Platform @('run', '--run', $Case.proposal.runId, '--runs-dir', $Case.runs) }
function Require-FixtureApproval($Case, [string]$RejectedHash = ('0' * 64)) {
    Expect $Case.proposal.ok ("The inert SCRIPT proposal was not approvable: " +
        ($Case.proposal | ConvertTo-Json -Depth 10 -Compress))
    $statePath = Join-Path $Case.proposal.runDir 'state.json'
    $round = (Read-JsonFile $statePath).round
    Expect ($null -ne $round -and $round -eq 0) 'A fresh proposal already consumed a coding round.'
    $result = Run-Fixture $Case
    Expect (!$result.ok -and $result.stopReason -eq 'ApprovalMissing') 'The scoped fixture ran without exact-hash approval.'
    Expect ((Read-JsonFile $statePath).round -eq $round) 'An unapproved attempt consumed a coding round.'
    $result = Platform @('approve', '--run', $Case.proposal.runId, '--hash', $RejectedHash, '--runs-dir', $Case.runs)
    Expect (!$result.ok -and $result.stopReason -eq 'ApprovalStale') 'A different trial or proposal hash authorized this run.'
    Expect ((Read-JsonFile $statePath).round -eq $round) 'Rejected approval consumed a coding round.'
    Approve-Fixture $Case
}
function Expect-CandidatePaths([string]$Repository, [string[]]$Expected) {
    $actual = @(git -C $Repository -c core.quotepath=false diff --name-only ade-baseline)
    Expect ($LASTEXITCODE -eq 0) 'The synthetic candidate lost its ADE baseline.'
    Expect (@(Compare-Object $Expected $actual).Count -eq 0) 'The candidate diff does not match its explicit approved file scope.'
}
$fix = "[IO.File]::WriteAllText((Join-Path `$env:ADE_WORKCOPY 'App.cs'), 'fixed')"
try {
    Contract 'request-path-isolation' {
        Assert-RunDirectory (Join-Path $temp 'output') @((Join-Path $temp 'source'))
        Refuses { Assert-RunDirectory (Join-Path $temp 'source\nested') @((Join-Path $temp 'source')) }
        Refuses { Assert-RunDirectory $temp @((Join-Path $temp 'source')) }
        Refuses { Assert-RunDirectory '\\?\C:\Repo\source\nested' @('C:\Repo\source') }
        Refuses { Assert-RunDirectory 'C:\REPO~1\nested' @('C:\Repo') }
    }
    Contract 'frozen-plan-input-integrity' {
        $path = Join-Path $temp 'scenario.json'
        $script = Join-Path $temp 'scenario.ps1'
        $request = Join-Path $temp 'request.json'
        [IO.File]::WriteAllText($path, '{"steps":["synthetic"]}')
        [IO.File]::WriteAllText($script, '# inert')
        [IO.File]::WriteAllText($request, '{"scenario":"user words"}')
        $context = [pscustomobject]@{
            state = 'planned'; scenarioPath = $path; scenarioSha256 = Get-Sha256 $path
            scenarioScript = $script; scenarioScriptSha256 = Get-Sha256 $script
            request = @{ path = $request; sha256 = Get-Sha256 $request }
        }
        Assert-FrozenScenario $context
        Assert-InvestigationState $context @('planned')
        Refuses { Assert-InvestigationState $context @('measured') }
        [IO.File]::AppendAllText($path, ' ')
        Refuses { Assert-FrozenScenario $context }
        $context.scenarioSha256 = Get-Sha256 $path
        [IO.File]::AppendAllText($request, ' ')
        Refuses { Assert-FrozenScenario $context }
    }
    Contract 'bug-target-must-fail-at-baseline' {
        $case = Fixture 'already-passing' $fix 'bug' 'exit 0'
        Expect (!$case.proposal.ok -and $case.proposal.stopReason -eq 'CheckAlreadyPasses') 'A passing bug target became approvable.'
    }
    Contract 'approval-missing-wrong-and-stale-hash' {
        $case = Fixture 'approval' $fix
        Expect $case.proposal.ok 'A failing synthetic bug target should be approvable.'
        $result = Run-Fixture $case
        Expect (!$result.ok -and $result.stopReason -eq 'ApprovalMissing') 'Unapproved execution was not refused.'
        $result = Platform @('approve', '--run', $case.proposal.runId, '--hash', ('0' * 64), '--runs-dir', $case.runs)
        Expect (!$result.ok -and $result.stopReason -eq 'ApprovalStale') 'A different proposal hash was accepted.'
        Approve-Fixture $case
        [IO.File]::AppendAllText((Join-Path $case.proposal.runDir 'proposal.json'), ' ')
        $result = Run-Fixture $case
        Expect (!$result.ok -and $result.stopReason -eq 'ApprovalStale') 'Modified proposal executed with stale approval.'
        Expect ((Get-Content -LiteralPath (Join-Path $case.repo 'App.cs') -Raw) -eq 'broken') 'Original app was edited.'
    }
    Contract 'approved-input-drift-is-rejected' {
        $case = Fixture 'drift' $fix
        Approve-Fixture $case
        [IO.File]::AppendAllText((Join-Path $case.pack 'knowledge\contract.md'), ' drift')
        $result = Run-Fixture $case
        Expect (!$result.ok -and $result.stopReason -eq 'InputsChanged') 'Changed protected proposal inputs were accepted.'
    }
    Contract 'protected-file-edits-are-detected-and-rejected' {
        $body = $fix + "`n[IO.File]::WriteAllText((Join-Path `$env:ADE_WORKCOPY 'tests\Protected.cs'), 'tampered')"
        $case = Fixture 'protected' $body
        Approve-Fixture $case
        $result = Run-Fixture $case
        Expect (!$result.ok -and $result.stopReason -match 'Protected') 'A protected edit was not reported as a protected-file failure.'
        Expect ((Get-Content -LiteralPath (Join-Path $case.repo 'tests\Protected.cs') -Raw) -eq 'locked') 'Original protected source changed.'
    }
    Contract 'failing-target-cannot-report-success' {
        $case = Fixture 'target-failure' "[IO.File]::WriteAllText((Join-Path `$env:ADE_WORKCOPY 'App.cs'), 'still-broken')"
        Approve-Fixture $case
        $result = Run-Fixture $case
        Expect (!$result.ok -and $result.stopReason -ne 'Passed' -and $result.rounds -eq 1) 'A failed target passed or exceeded its one-round limit.'
        $provenance = Read-JsonFile (Join-Path $case.proposal.runDir 'provenance.json')
        Expect (!$provenance.executor.isModel) 'Synthetic contract adapter was labelled as AI.'
    }
    Contract 'regression-check-failure-cannot-report-success' {
        $body = $fix + "`n[IO.File]::WriteAllText((Join-Path `$env:ADE_WORKCOPY 'Guard.txt'), 'regression')"
        $case = Fixture 'regression' $body
        Approve-Fixture $case
        $result = Run-Fixture $case
        Expect (!$result.ok -and $result.stopReason -ne 'Passed') 'A required baseline-passing check regressed but reported success.'
    }
    Contract 'successful-isolated-run-leaves-original-unchanged' {
        $case = Fixture 'passing' $fix
        $before = Get-Sha256 (Join-Path $case.repo 'App.cs')
        Approve-Fixture $case
        $result = Run-Fixture $case
        Expect ($result.ok -and $result.stopReason -eq 'Passed') 'The valid synthetic fix did not pass its real ADE checks.'
        Expect ((Get-Sha256 (Join-Path $case.repo 'App.cs')) -eq $before) 'The original repository was modified.'
        $provenance = Read-JsonFile (Join-Path $case.proposal.runDir 'provenance.json')
        Expect $provenance.originalRepositoryUnchanged 'ADE did not verify original-source isolation.'
        Expect (!$provenance.executor.isModel) 'Contract adapter must be labelled not-a-model.'
        $integrity = Platform @('verify', '--run', $case.proposal.runId, '--runs-dir', $case.runs)
        Expect $integrity.ok 'The synthetic run event chain did not verify.'
        $again = Run-Fixture $case
        Expect (!$again.ok -and $again.stopReason -eq 'AlreadyCompleted') 'A completed run executed again.'
    }
    Contract 'approved-file-scope-allows-three-and-rejects-fourth' {
        $allowed = @('App.cs', 'ViewModel.cs', 'View.xaml')
        $body = $fix + @"

[IO.File]::WriteAllText((Join-Path `$env:ADE_WORKCOPY 'ViewModel.cs'), 'fixed-model')
[IO.File]::WriteAllText((Join-Path `$env:ADE_WORKCOPY 'View.xaml'), 'fixed-view')
"@
        $target = @"
if ((Get-Content -LiteralPath 'App.cs' -Raw) -eq 'fixed' -and
    (Get-Content -LiteralPath 'ViewModel.cs' -Raw) -eq 'fixed-model' -and
    (Get-Content -LiteralPath 'View.xaml' -Raw) -eq 'fixed-view') { exit 0 }; exit 1
"@
        $case = Fixture 'three-approved-files' $body -Target $target -AllowedPaths $allowed
        $before = Get-SourceHash $case.repo
        Require-FixtureApproval $case
        $result = Run-Fixture $case
        Expect ($result.ok -and $result.stopReason -eq 'Passed') 'An approved three-file C#/XAML fix was rejected.'
        $candidate = (Read-JsonFile (Join-Path $case.proposal.runDir 'proposal.json')).workCopy
        Assert-ScopedCandidate $candidate $allowed
        Expect-CandidatePaths $candidate $allowed
        Expect ((Get-SourceHash $case.repo) -eq $before) 'The three-file fixture changed its original source.'
        $provenance = Read-JsonFile (Join-Path $case.proposal.runDir 'provenance.json')
        Expect (!$provenance.executor.isModel -and $provenance.originalRepositoryUnchanged) 'The inert three-file run lost its executor or isolation provenance.'
        $integrity = Platform @('verify', '--run', $case.proposal.runId, '--runs-dir', $case.runs)
        Expect $integrity.ok 'The approved three-file event chain did not verify.'

        $outside = $body + "`n[IO.File]::WriteAllText((Join-Path `$env:ADE_WORKCOPY 'Other.cs'), 'out-of-scope')"
        $case = Fixture 'fourth-file-outside-scope' $outside -Target $target -AllowedPaths $allowed
        $before = Get-SourceHash $case.repo
        Require-FixtureApproval $case
        $result = Run-Fixture $case
        Expect (!$result.ok -and $result.stopReason -ne 'Passed' -and $result.rounds -eq 1) 'A fourth out-of-scope edit reported success.'
        $candidate = (Read-JsonFile (Join-Path $case.proposal.runDir 'proposal.json')).workCopy
        Expect-CandidatePaths $candidate ($allowed + 'Other.cs')
        Refuses { Assert-ScopedCandidate $candidate $allowed }
        Expect ((Get-SourceHash $case.repo) -eq $before) 'The rejected out-of-scope fixture changed its original source.'
        $provenance = Read-JsonFile (Join-Path $case.proposal.runDir 'provenance.json')
        Expect (!$provenance.executor.isModel) 'The rejected scope fixture was labelled as AI.'
    }
    Contract 'recommendation-trials-remain-independent' {
        $pristine = @'
$ErrorActionPreference = 'Stop'
$expected = @{ 'App.cs' = 'broken'; 'ViewModel.cs' = 'original-model'; 'View.xaml' = 'original-view'; 'Other.cs' = 'outside-original' }
foreach ($name in $expected.Keys) {
    if ((Get-Content -LiteralPath (Join-Path $env:ADE_WORKCOPY $name) -Raw) -cne $expected[$name]) {
        throw 'The recommendation trial did not start from the unchanged original source.'
    }
}
'@
        $firstBody = $pristine + "`n[IO.File]::WriteAllText((Join-Path `$env:ADE_WORKCOPY 'App.cs'), 'trial-one')"
        $first = Fixture 'recommendation-trial-one' $firstBody -Type 'change' -Target 'exit 0' -AllowedPaths @('App.cs')
        $before = Get-SourceHash $first.repo
        $revision = git -C $first.repo rev-parse HEAD
        Expect ($LASTEXITCODE -eq 0) 'The original trial fixture has no source revision.'
        Require-FixtureApproval $first
        $result = Run-Fixture $first
        Expect ($result.ok -and $result.stopReason -eq 'Passed') 'The first inert recommendation trial did not pass.'
        $firstProposal = Read-JsonFile (Join-Path $first.proposal.runDir 'proposal.json')
        Assert-ScopedCandidate $firstProposal.workCopy @('App.cs')
        Expect-CandidatePaths $firstProposal.workCopy @('App.cs')
        Expect ((Get-SourceHash $first.repo) -eq $before) 'The first trial changed the shared original source.'
        $integrity = Platform @('verify', '--run', $first.proposal.runId, '--runs-dir', $first.runs)
        Expect $integrity.ok 'The first independent trial event chain did not verify.'
        $retained = @{}
        foreach ($name in @('proposal.json', 'approval.json', 'diff.patch', 'state.json', 'provenance.json', 'report.md', 'events.jsonl')) {
            $retained[$name] = Get-Sha256 (Join-Path $first.proposal.runDir $name)
        }

        $secondBody = $pristine + "`n[IO.File]::WriteAllText((Join-Path `$env:ADE_WORKCOPY 'View.xaml'), 'trial-two')"
        $second = Fixture 'recommendation-trial-two' $secondBody -Type 'change' -Target 'exit 0' `
            -AllowedPaths @('View.xaml') -SourceRepository $first.repo -RunsDirectory $first.runs
        $secondProposal = Read-JsonFile (Join-Path $second.proposal.runDir 'proposal.json')
        Expect ($first.proposal.runId -ne $second.proposal.runId -and $firstProposal.workCopy -ne $secondProposal.workCopy) 'Two recommendations reused an ADE run or working copy.'
        Expect ($firstProposal.repo -eq $secondProposal.repo -and $secondProposal.repo -eq $first.repo -and
            $firstProposal.originalTreeSha256 -eq $secondProposal.originalTreeSha256) 'Trials did not use the same unchanged original source.'
        Require-FixtureApproval $second $first.proposal.proposalSha256
        $result = Run-Fixture $second
        Expect ($result.ok -and $result.stopReason -eq 'Passed') 'The second inert recommendation trial did not pass from the original baseline.'
        Assert-ScopedCandidate $secondProposal.workCopy @('View.xaml')
        Expect-CandidatePaths $secondProposal.workCopy @('View.xaml')
        Expect ((Get-Content -LiteralPath (Join-Path $secondProposal.workCopy 'App.cs') -Raw) -eq 'broken') 'First-trial edits leaked into the second working copy.'
        Expect ((Get-Content -LiteralPath (Join-Path $firstProposal.workCopy 'App.cs') -Raw) -eq 'trial-one' -and
            (Get-Content -LiteralPath (Join-Path $firstProposal.workCopy 'View.xaml') -Raw) -eq 'original-view') 'The second trial modified the first retained working copy.'
        Expect ((Get-SourceHash $first.repo) -eq $before -and (git -C $first.repo rev-parse HEAD) -eq $revision) 'The shared original source or revision changed across trials.'
        $integrity = Platform @('verify', '--run', $second.proposal.runId, '--runs-dir', $second.runs)
        Expect $integrity.ok 'The second independent trial event chain did not verify.'
        foreach ($name in $retained.Keys) {
            Expect ((Get-Sha256 (Join-Path $first.proposal.runDir $name)) -eq $retained[$name]) "The second trial overwrote the first trial's $name."
        }
        foreach ($case in @($first, $second)) {
            $state = Read-JsonFile (Join-Path $case.proposal.runDir 'state.json')
            Expect ($state.runId -eq $case.proposal.runId -and $state.stopReason -eq 'Passed') 'An independent ADE trial result was not retained under its own run.'
            $provenance = Read-JsonFile (Join-Path $case.proposal.runDir 'provenance.json')
            Expect (!$provenance.executor.isModel -and $provenance.originalRepositoryUnchanged) 'An independent trial lost inert-executor or original-source provenance.'
        }
        $firstDiff = Get-Content -LiteralPath (Join-Path $first.proposal.runDir 'diff.patch') -Raw
        $secondDiff = Get-Content -LiteralPath (Join-Path $second.proposal.runDir 'diff.patch') -Raw
        Expect ($firstDiff.Contains('trial-one') -and !$firstDiff.Contains('trial-two') -and
            $secondDiff.Contains('trial-two') -and !$secondDiff.Contains('trial-one')) 'The independently recorded ADE diffs were mixed or overwritten.'
    }
    Write-Output "$passed real harness/ADE contract groups passed; zero model calls, app launches or captures."
} finally {
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name]) }
    Remove-Item -LiteralPath $temp -Recurse -Force
}

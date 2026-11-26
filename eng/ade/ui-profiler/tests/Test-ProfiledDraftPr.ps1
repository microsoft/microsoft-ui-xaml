#Requires -Version 7
# Explicitly synthetic offline contract tests. No Git repository, commit, push,
# credentials, GitHub operation, native build, model, or measurement is executed.
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
$publisher = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\pack\tools\Publish-ProfiledDraftPr.ps1'))
$errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($publisher, [ref]$null, [ref]$errors)
if ($errors.Count) { throw ($errors.Message -join "`n") }
foreach ($definition in $ast.FindAll({ param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst]
}, $false)) {
    . ([scriptblock]::Create($definition.Extent.Text))
}
$script:processCalls = 0
function Invoke-PublishProcess {
    param($File, $Arguments, $Environment)
    $script:processCalls++
    throw 'Offline tests prohibit all real external processes.'
}
$root = Join-Path $PSScriptRoot ('.profiled-pr-fixture-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory $root)
$script:passed = 0
function Check([bool]$Condition, [string]$Message) { if (!$Condition) { throw $Message } }
function Refuses([scriptblock]$Action, [string]$Expected = '') {
    $message = ''
    try { & $Action } catch { $message = $_.Exception.Message }
    if (!$message -or ($Expected -and $message -notlike "*$Expected*")) { throw "Expected refusal '$Expected'; got '$message'." }
}
function Case([string]$Name, [scriptblock]$Action) {
    & $Action
    $script:passed++
    Write-Output "PASS $Name"
}
function New-Fixture {
    @{
        context = @{schema='ui-profiler/trial/v1';recommendationId='generic-fix'}
        outcome = @{schema='ui-profiler/independent-trial-outcome/v1';state='verified-comparison'
            recommendationId='generic-fix';proposalSha256=('a'*64);chainVerified=$true
            adeChecksPassed=$true;originalRepositoryUnchanged=$true
            validation=@(@{command='synthetic-check --case generic';result='passed (synthetic fixture only)'})}
        run = @{runId='synthetic';proposalSha256=('a'*64)}
        proposal = @{workCopy='synthetic-workcopy'}
        approval = @{proposalSha256=('a'*64)}
        comparison = @{schema='ui-profiler/comparison/v2';metrics=@(
            @{family='ticks';scope='scenario';metric='mean';unit='ms';primary=$true;direction='lower'
                baseline=10;candidate=8;delta=-2;reductionPercent=20
                status='available';baselineStatus='available';candidateStatus='available';change='observed-improvement'},
            @{family='events';scope='scenario';metric='count';unit='count';primary=$true;direction='lower'
                baseline=10;candidate=12;delta=2;reductionPercent=-20
                status='available';baselineStatus='available';candidateStatus='available';change='regression'},
            @{family='gpu';scope='scenario';metric='max';unit='bytes';primary=$true;direction='lower'
                baseline=$null;candidate=$null;delta=$null;reductionPercent=$null
                status='not_comparable';baselineStatus='unavailable';candidateStatus='unavailable';change='unavailable'}
        );capabilities=@{gpu=@{baseline='unavailable';candidate='unavailable';baselineNotes='missing';candidateNotes='missing'}}
            interpretation='Synthetic evidence only.';legacyEvidence=$false}
    }
}
function Eligible($Fixture) {
    Assert-PublishEligibility $Fixture.context $Fixture.outcome $Fixture.run $Fixture.proposal $Fixture.approval $Fixture.comparison
}
try {
    Case 'eligible application/control/framework share the same strict gate' {
        foreach ($schema in @('ui-profiler/trial/v1', 'ui-profiler/native-trial/v1')) {
            $fixture = New-Fixture
            $fixture.context.schema = $schema
            Eligible $fixture
        }
    }
    Case 'non-verified states never authorize publication' {
        foreach ($state in @('failed', 'blocked', 'awaiting-approval', 'Passed', '')) {
            $fixture = New-Fixture
            $fixture.outcome.state = $state
            Refuses { Eligible $fixture } 'verified comparison'
        }
    }
    Case 'each required integrity flag is a literal true boolean' {
        foreach ($flag in @('chainVerified', 'adeChecksPassed', 'originalRepositoryUnchanged')) {
            foreach ($value in @($false, $null, 'True', 1)) {
                $fixture = New-Fixture
                $fixture.outcome[$flag] = $value
                Refuses { Eligible $fixture } 'integrity gates'
            }
        }
    }
    Case 'exact approval and recommendation/proposal bindings reject substitutions' {
        foreach ($mutation in @(
            { param($f) $f.approval.proposalSha256 = 'b'*64 },
            { param($f) $f.approval.Remove('proposalSha256') },
            { param($f) $f.outcome.proposalSha256 = 'b'*64 },
            { param($f) $f.outcome.recommendationId = 'other' },
            { param($f) $f.run.proposalSha256 = 'bad' },
            { param($f) $f.proposal.Remove('workCopy') }
        )) {
            $fixture = New-Fixture
            & $mutation $fixture
            Refuses { Eligible $fixture } 'Exact-hash approval'
        }
    }
    Case 'zero, partial, missing, non-primary, context-only and fabricated improvements are blocked' {
        foreach ($mutation in @(
            { param($r) $r.candidate=10; $r.delta=0; $r.change='unchanged' },
            { param($r) $r.status='partial' },
            { param($r) $r.baselineStatus='partial' },
            { param($r) $r.candidate=$null },
            { param($r) $r.baseline=$null },
            { param($r) $r.primary=$false },
            { param($r) $r.primary='True' },
            { param($r) $r.direction='context' },
            { param($r) $r.candidate=12; $r.delta=2 },
            { param($r) $r.candidate=[double]::NaN },
            { param($r) $r.candidate=-1 },
            { param($r) $r.change='decrease' }
        )) {
            $fixture = New-Fixture
            & $mutation $fixture.comparison.metrics[0]
            Refuses { Eligible $fixture } 'No demonstrated'
        }
    }
    Case 'only MUX multidimensional output and exact validation records are accepted' {
        $fixture = New-Fixture
        $fixture.comparison.schema = 'etw/comparison/v1'
        Refuses { Eligible $fixture } 'Only MUX'
        $fixture = New-Fixture
        $fixture.outcome.validation = @()
        Refuses { Eligible $fixture } 'Outcome.validation'
        $fixture.outcome.validation = @(@{command='synthetic';result=''})
        Refuses { Eligible $fixture } 'observed result'
    }
    Case 'GitHub repository inference is host and credential constrained' {
        foreach ($url in @('https://github.com/owner/repo.git', 'git@github.com:owner/repo.git', 'ssh://git@github.com/owner/repo')) {
            Check ((Get-PublishRepository $url) -ceq 'owner/repo') 'Safe GitHub origin parsed incorrectly.'
        }
        foreach ($url in @('https://evil.invalid/owner/repo', 'https://github.com.evil.invalid/o/r',
            'https://token@github.com/o/r', 'http://github.com/o/r', 'https://github.com/o/r?token=x',
            'git@evil.invalid:owner/repo', 'https://github.com/o/..')) {
            Refuses { Get-PublishRepository $url } 'origin'
        }
    }
    Case 'scope paths reject traversal, aliases and wildcard interpretation' {
        foreach ($path in @('..\escape.cs', 'sub\..\escape.cs', 'C:\escape.cs', 'file.cs:stream', '*.cs', 'x[1].cs', '.\file.cs')) {
            Refuses { Assert-PublishPath $root $path } 'literal'
        }
        Check ((Assert-PublishPath $root 'generic\Control.cs') -eq (Join-Path $root 'generic\Control.cs')) 'Contained path rejected.'
    }
    Case 'long native tracked and untracked paths survive command-scoped Git checks' {
        $longPath = 'dxaml/xcp/' + (('nested-source-segment/' * 16)) + 'ControlX.cpp'
        Check ($longPath.Length -gt 260) 'Long-path fixture is too short.'
        $beforeCount = [Environment]::GetEnvironmentVariable('GIT_CONFIG_COUNT', 'Process')
        Invoke-PublishSourceCheck {
            & (Get-Module ProfilerHarness) {
                param($LongPath)
                $untracked = ''
                function git {
                    $global:LASTEXITCODE = 0
                    $index = [int]$env:GIT_CONFIG_COUNT - 1
                    if ([Environment]::GetEnvironmentVariable("GIT_CONFIG_KEY_$index") -cne 'core.longpaths' -or
                        [Environment]::GetEnvironmentVariable("GIT_CONFIG_VALUE_$index") -cne 'true') {
                        throw 'Shared scope validator lost the command-scoped long-path override.'
                    }
                    if ($args -contains 'diff') { return "M`t$LongPath" }
                    if ($args -contains 'ls-files') { if ($untracked) { $untracked }; return }
                    throw 'Unexpected mocked scope Git command.'
                }
                Assert-ScopedCandidate 'synthetic-no-git-repository' @($LongPath) -Level framework
                $untracked = $LongPath + '.untracked.cpp'
                $refused = $false
                try { Assert-ScopedCandidate 'synthetic-no-git-repository' @($LongPath) -Level framework }
                catch { $refused = $_.Exception.Message -like '*no additions*' }
                if (!$refused) { throw 'Long untracked source disappeared from the scope check.' }
                $untracked = ''
                $refused = $false
                try { Assert-ScopedCandidate 'synthetic-no-git-repository' @('dxaml/xcp/Other.cpp') -Level framework }
                catch { $refused = $_.Exception.Message -like '*outside its approved file scope*' }
                if (!$refused) { throw 'Long modified source disappeared from the scope check.' }
            } $longPath
        }
        Check ([Environment]::GetEnvironmentVariable('GIT_CONFIG_COUNT', 'Process') -ceq $beforeCount) 'Shared-check Git configuration leaked.'
        Refuses { Invoke-PublishSourceCheck { throw 'synthetic scoped failure' } } 'synthetic scoped failure'
        Check ([Environment]::GetEnvironmentVariable('GIT_CONFIG_COUNT', 'Process') -ceq $beforeCount) 'Failed check leaked Git configuration.'
        function Invoke-PublishProcess {
            param($File, $Arguments, $Environment)
            Check ($File -eq 'git' -and $Arguments -contains 'core.longpaths=true') 'Direct Git operation lacks per-command long-path support.'
            @{text=$longPath;bytes=[Text.Encoding]::UTF8.GetBytes('synthetic blob')}
        }
        foreach ($command in @('status', 'checkout', 'add', 'ls-files')) {
            Check ((Get-PublishGit 'synthetic-no-git-repository' @($command, '--', $longPath)) -ceq $longPath) 'Long path changed in Git transport.'
        }
        [void](Get-PublishBlobHash 'synthetic-no-git-repository' 'HEAD' $longPath)
    }
    Case 'template headings/comments/checklists remain ordered, with accurate supplied content' {
        $template = "## Description`n<!-- Keep this comment -->`n[Describe the change]`n## Validation`n- [ ] Existing check`n## Other`nUnfamiliar prose stays."
        $result = Merge-PublishTemplate $template 'Complete measured evidence and exact results.'
        foreach ($text in @('## Description', '<!-- Keep this comment -->', '## Validation', '- [ ] Existing check', '## Other', 'Unfamiliar prose stays.')) {
            Check ($result.Contains($text)) "Template structure lost: $text"
        }
        Check (!$result.Contains('[Describe the change]')) 'Template prompt was not filled.'
        Check ($result.IndexOf('## Description') -lt $result.IndexOf('## Validation')) 'Template headings reordered.'
        Check ($result.EndsWith('Complete measured evidence and exact results.')) 'Measured report missing.'
    }
    $realGit = (Get-Item Function:Get-PublishGit).ScriptBlock
    $realBlob = (Get-Item Function:Get-PublishBlobHash).ScriptBlock
    function Get-PublishGit { param($Repository, $Arguments)
        if ($Arguments[0] -eq 'ls-tree') { return $script:templatePaths -join "`n" }
        return "## Description`n<!-- original template -->"
    }
    function Get-PublishBlobHash { param($Repository, $Revision, $Path) return 'a'*64 }
    Case 'ambiguous alternate templates block; standard root default is respected' {
        $script:templatePaths = @('.github/PULL_REQUEST_TEMPLATE/one.md', '.github/PULL_REQUEST_TEMPLATE/two.md')
        Refuses { Get-PublishTemplate 'synthetic' 'HEAD' } 'Multiple PR templates'
        $script:templatePaths += '.github/pull_request_template.md'
        Check ((Get-PublishTemplate 'synthetic' 'HEAD').path -ceq '.github/pull_request_template.md') 'Simple default was ignored.'
        $script:templatePaths = @('.github/PULL_REQUEST_TEMPLATE/only.md')
        Check ((Get-PublishTemplate 'synthetic' 'HEAD').path -like '*only.md') 'Single alternate template not used.'
    }
    Set-Item Function:Get-PublishGit $realGit
    Set-Item Function:Get-PublishBlobHash $realBlob
    Case 'complete report preserves improvements, regressions, nulls and one-pair caveat equally' {
        $fixture = New-Fixture
        $fixture.comparison = $fixture.comparison | ConvertTo-Json -Depth 20 | ConvertFrom-Json
        $hashes = @{level='framework'}
        foreach ($name in @('context', 'outcome', 'recommendations', 'proposal', 'approval', 'diff', 'comparison', 'manifest', 'baseline', 'candidate')) {
            $hashes[$name] = @{path="local\$name";sha256=('a'*64)}
        }
        $recommendation = @{title='Generic control';summary='A source hypothesis, not a measured conclusion.';hypotheses=@('Possible cause.')}
        $report = New-PublishReport $fixture.context $fixture.outcome $fixture.run $fixture.comparison $recommendation @('Control.cpp') $hashes '1 file changed'
        foreach ($text in @('observed-improvement', 'regression', 'null (unavailable)', '| 20 |', '| -20 |',
            'One observed baseline/candidate run pair', 'not statistical evidence', 'not measured evidence',
            'NOT reviewer-accessible', 'synthetic-check --case generic', 'passed (synthetic fixture only)', 'Control.cpp')) {
            Check ($report.Contains($text)) "Report omitted evidence/limitation: $text"
        }
    }
    $inputPath = Join-Path $root 'input.txt'
    $output = Join-Path $root 'comparison.json'
    $manifestPath = Join-Path $root 'manifest.json'
    [IO.File]::WriteAllText($inputPath, 'synthetic MUX input, never a real recording')
    [IO.File]::WriteAllText($output, '{"fixture":"synthetic"}')
    $manifest = @{schema='ui-profiler/evidence-manifest/v1'
        inputs=@(@{path=$inputPath;sha256=Get-Sha256 $inputPath;origin='recorded'})
        outputs=@(@{path=$output;sha256=Get-Sha256 $output})}
    Write-JsonFile $manifestPath $manifest
    Case 'manifest outputs and every evidence hash are checked offline before external verification' {
        Assert-PublishManifest $manifestPath $output
        Refuses { Assert-PublishManifest $manifestPath $inputPath } 'sealed manifest output'
        [IO.File]::AppendAllText($inputPath, ' tampered')
        Refuses { Assert-PublishManifest $manifestPath $output } 'Evidence or input changed'
        Refuses { Assert-PublishComparison $output @{directory=$root} } 'Evidence or input changed'
        Check ($script:processCalls -eq 0) 'Bad comparison reached an external process.'
    }
    Case 'full entry point rejects missing exact-hash approval without network or publication writes' {
        $fixture = New-Fixture
        $script:TrialContextPath = Join-Path $root 'context.json'
        $script:OutcomePath = Join-Path $root 'outcome.json'
        $fixture.context.directory = $root
        $fixture.context.recommendations = @{path=$inputPath;sha256=Get-Sha256 $inputPath}
        $runDir = Join-Path $root 'ade-runs\synthetic'
        [void](New-Item -ItemType Directory $runDir -Force)
        Write-JsonFile (Join-Path $runDir 'proposal.json') $fixture.proposal
        $fixture.run.proposalSha256 = Get-Sha256 (Join-Path $runDir 'proposal.json')
        $fixture.outcome.proposalSha256 = $fixture.run.proposalSha256
        Write-JsonFile (Join-Path $root 'ade-run.json') $fixture.run
        Write-JsonFile (Join-Path $runDir 'approval.json') @{proposalSha256=('b'*64)}
        Write-JsonFile $output $fixture.comparison
        $fixture.outcome.comparison = @{path=$output;sha256=Get-Sha256 $output}
        Write-JsonFile $script:TrialContextPath $fixture.context
        Write-JsonFile $script:OutcomePath $fixture.outcome
        $script:TrialContextSha256 = Get-Sha256 $script:TrialContextPath
        $beforeFiles = @(Get-ChildItem -LiteralPath $root -Recurse -File | ForEach-Object FullName) -join "`n"
        Refuses { Invoke-ProfiledDraftPublication } 'Exact-hash approval'
        Check ($script:processCalls -eq 0) 'Approval refusal performed network/credential/Git operations.'
        Check (!(Test-Path -LiteralPath (Join-Path $root 'draft-publication'))) 'Approval refusal wrote publication artifacts.'
        Check ($beforeFiles -ceq (@(Get-ChildItem -LiteralPath $root -Recurse -File | ForEach-Object FullName) -join "`n")) 'Gate failure created files.'
    }
    Case 'publisher mutation commands remain confined to a fresh clone and scoped stage' {
        $source = [IO.File]::ReadAllText($publisher)
        Check ($source.Contains("'clone', '--no-local', '--no-hardlinks'")) 'Missing fresh original-history clone.'
        Check ($source.Contains("'Assert-ScopedCandidate'") -or $source.Contains('Assert-ScopedCandidate $clone')) 'Export scope not checked.'
        Check ($source.Contains("@('add', '--') + `$files")) 'Publication does not stage only measured changed files.'
        Check (!$source.Contains("'--force'") -and !$source.Contains("'--force-with-lease'")) 'Force push is prohibited.'
        Check (!$source.Contains("'auth', 'switch'") -and !$source.Contains("'auth', 'setup-git'")) 'Global auth mutation is prohibited.'
        Check ($source.Contains('Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>') -and
            $source.Contains('Copilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda')) 'Commit trailers missing.'
    }
    Case 'unmerged and newer descendant GitHub bases are blocked, and repeated fetch catches moved base' {
        $revision = 'c'*40
        $tip = $revision
        $fetches = 0
        function Get-PublishGit {
            param($Repository, $Arguments, $Environment)
            if ($Arguments -contains 'fetch') { $fetches++; return '' }
            if ($Arguments[0] -eq 'rev-parse') { return $tip }
            throw 'Unexpected Git base verification command.'
        }
        # Deliberately use scriptblock state: all commands are mocked, no branch fetch occurs.
        foreach ($differentTip in @(('b'*40), ('e'*40))) {
            $tip = $differentTip
            Refuses { Assert-PublishMeasuredBase 'synthetic' 'main' $revision @{} } 'not the exact measured originalRevision'
        }
        $tip = $revision
        Assert-PublishMeasuredBase 'synthetic' 'measured-feature' $revision @{}
        $tip = 'f'*40
        Refuses { Assert-PublishMeasuredBase 'synthetic' 'measured-feature' $revision @{} } 'new baseline/comparison'
    }
    Case 'actual triple-dot PR diff rejects unrelated paths and mismatched measured blobs' {
        $workcopy = Join-Path $root 'pr-diff-workcopy'
        [void](New-Item -ItemType Directory $workcopy)
        [IO.File]::WriteAllText((Join-Path $workcopy 'Control.cs'), 'synthetic measured fix')
        $revision = 'c'*40
        $changes = "M`tControl.cs"
        $blobHash = Get-Sha256 (Join-Path $workcopy 'Control.cs')
        function Get-PublishGit {
            param($Repository, $Arguments, $Environment)
            if ($Arguments[0] -in @('rev-parse', 'merge-base')) { return $revision }
            if ($Arguments[0] -eq 'diff') {
                Check ($Arguments -contains "$revision...$('d'*40)") 'Actual PR diff is not a base...head comparison.'
                return $changes
            }
            throw 'Unexpected Git PR diff command.'
        }
        function Get-PublishBlobHash { param($Repository, $Revision, $Path) return $blobHash }
        Assert-PublishPrDiff 'synthetic' $revision ('d'*40) $workcopy @('Control.cs') @('Control.cs')
        $changes = "M`tControl.cs`nM`tUnrelatedHarness.ps1"
        Refuses { Assert-PublishPrDiff 'synthetic' $revision ('d'*40) $workcopy @('Control.cs') @('Control.cs') } 'imports unrelated'
        $changes = "M`tControl.cs"
        $blobHash = '0'*64
        Refuses { Assert-PublishPrDiff 'synthetic' $revision ('d'*40) $workcopy @('Control.cs') @('Control.cs') } 'blob bytes differ'
    }
    Case 'mocked application/control/framework publishing preserves original history, credentials and scoped evidence' {
        function Assert-TrialContext { param($Context) }
        function Assert-NativeTrialContext { param($Context) }
        function Assert-OriginalSource { param($Context) }
        function Assert-Measurement { param($Measurement) }
        function Assert-PublishComparison { param($Path, $Context) }
        function Assert-ScopedCandidate {
            param($RepositoryPath, $AllowedPaths, $Level)
            Check ($AllowedPaths.Count -eq 1 -and $AllowedPaths[0] -eq $script:relative) 'Mock export scope changed.'
            Check ($Level -eq $script:level) 'Mock publication level changed.'
        }
        function Get-SourceHash { param($RepositoryPath) Get-Sha256 (Join-Path $RepositoryPath $script:relative) }
        function Get-PublishTemplate {
            param($Repository, $Revision)
            @{path='.github/pull_request_template.md';text="## Description`n<!-- preserved -->`n## Validation`n- [ ] Existing check";sha256=('a'*64)}
        }
        function Get-PublishBlobHash {
            param($Repository, $Revision, $Path)
            if ($Revision -eq '' -or $Revision -ceq ('d'*40)) { return Get-Sha256 (Join-Path $Repository $Path) }
            Get-Sha256 (Join-Path $script:original $Path)
        }
        function Get-PublishGit {
            param($Repository, $Arguments, $Environment)
            $command = $Arguments[0]
            if ($command -eq 'status') { return '' }
            if ($command -eq 'diff') {
                if ($Arguments -contains '--stat') { return '1 scoped source file changed (synthetic)' }
                if ($Arguments -contains '--name-status') { return "M`t$($script:relative.Replace('\', '/'))" }
                return $script:relative.Replace('\', '/')
            }
            if ($command -eq 'remote' -and $Arguments[1] -eq 'get-url') { return 'git@github.com:synthetic-owner/synthetic-repo.git' }
            if ($command -eq 'check-ref-format') { return '' }
            Check ($Repository -eq $script:clone) 'A mutation or network operation targeted an original source repository.'
            if ($command -eq 'checkout') { $script:branch = $Arguments[3]; return '' }
            if ($command -eq 'rev-parse') {
                if ($Arguments[1] -eq 'HEAD') { return ('d'*40) }
                return ('c'*40)
            }
            if ($command -eq 'merge-base') { return ('c'*40) }
            if ($command -in @('fetch', 'push') -or $Arguments -contains 'push' -or $Arguments -contains 'fetch') {
                Check ($Environment.GH_TOKEN -ceq 'synthetic-child-credential') 'Selected credential not child-scoped.'
                Check ($Arguments -notcontains '--force') 'Unsafe push.'
                if ($Arguments -contains 'fetch') { $script:mockBaseFetches++ }
                if ($Arguments -contains 'push') { Check ($script:mockBaseFetches -ge 2) 'Push occurred without an immediate base recheck.' }
            }
            if ($command -eq 'add') {
                Check (($Arguments -join '|') -ceq "add|--|$($script:relative.Replace('\','/'))") 'Staging exceeded the measured source files.'
            }
            if ($Arguments -contains 'commit') {
                Check ($script:mockBaseFetches -ge 1) 'Commit occurred before the exact fetched-base gate.'
                Check (($Arguments -join "`n").Contains('Copilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda')) 'Commit lacks trailer.'
            }
            $script:mockGitCommands.Add($Arguments -join ' ')
            ''
        }
        function Invoke-PublishProcess {
            param($File, $Arguments, $Environment)
            if ($File -eq 'git' -and $Arguments -contains 'clone') {
                Check ($Arguments -contains 'core.longpaths=true') 'Fresh history clone lacks command-scoped long-path support.'
                Check ($Arguments[-2] -eq $script:original) 'Clone used snapshot history instead of the original Git repository.'
                Check ($Arguments[-1] -eq $script:clone) 'Clone is not publication-owned.'
                $target = Join-Path $script:clone $script:relative
                [void](New-Item -ItemType Directory -Force (Split-Path $target -Parent))
                Copy-Item -LiteralPath (Join-Path $script:original $script:relative) -Destination $target
                return @{text='';bytes=@()}
            }
            Check ($File -eq 'gh') 'Unexpected external command in synthetic publisher.'
            if ($Arguments[0] -eq 'auth') {
                Check (!$Environment.GH_TOKEN -and !$Environment.GITHUB_TOKEN) 'Selected-account lookup used inherited credential override.'
                return @{text='synthetic-child-credential'}
            }
            Check ($Environment.GH_TOKEN -ceq 'synthetic-child-credential') 'GitHub request lacks selected child credential.'
            if ($Arguments[0] -eq 'api') { return @{text='synthetic-user'} }
            if ($Arguments[0] -eq 'repo') {
                return @{text='{"nameWithOwner":"synthetic-owner/synthetic-repo","viewerPermission":"WRITE","defaultBranchRef":{"name":"main"}}'}
            }
            if ($Arguments[0] -eq 'pr' -and $Arguments[1] -eq 'create') {
                Check ($Arguments -contains '--draft') 'PR must be a draft.'
                Check ($script:mockBaseFetches -ge 3) 'PR creation occurred without a fresh fetched-base check.'
                return @{text='https://github.com/synthetic-owner/synthetic-repo/pull/42'}
            }
            if ($Arguments[0] -eq 'pr' -and $Arguments[1] -eq 'view') {
                return @{text=(@{url='https://github.com/synthetic-owner/synthetic-repo/pull/42';isDraft=$true
                    headRefName=$script:branch;headRefOid=('d'*40);baseRefName='main';baseRefOid=('c'*40)} | ConvertTo-Json)}
            }
            throw 'Unexpected mocked gh command.'
        }
        $inheritedToken = $env:GH_TOKEN
        $script:GitHubAccount = 'synthetic-user'
        $script:BaseBranch = ''
        foreach ($script:level in @('application', 'control', 'framework')) {
            $caseRoot = Join-Path $root $script:level
            $script:relative = switch ($script:level) {
                application { 'Control.cs' }
                control { 'controls\dev\ControlX\ControlX.cpp' }
                framework { 'dxaml\xcp\core\ControlX.cpp' }
            }
            $script:original = Join-Path $caseRoot 'original-history'
            $workcopy = Join-Path $caseRoot 'ade-workcopy'
            $baseline = Join-Path $caseRoot 'private-baseline-snapshot'
            foreach ($directory in @($script:original, $workcopy, $baseline)) {
                $path = Join-Path $directory $script:relative
                [void](New-Item -ItemType Directory -Force (Split-Path $path -Parent))
                [IO.File]::WriteAllText($path, $(if ($directory -eq $workcopy) { 'synthetic candidate' } else { 'synthetic baseline' }))
            }
            $trial = Join-Path $caseRoot 'trial'
            $runDir = Join-Path $trial 'ade-runs\synthetic'
            [void](New-Item -ItemType Directory -Force $runDir)
            $fixture = New-Fixture
            $fixture.context.directory = $trial
            $fixture.context.allowedPaths = @($script:relative)
            $fixture.context.originalRepository = $script:original
            $fixture.context.originalRevision = 'c'*40
            $fixture.context.originalSourceSha256 = Get-Sha256 (Join-Path $script:original $script:relative)
            $fixture.context.scenarioSha256 = 'e'*64
            $fixture.context.profilerSha256 = 'f'*64
            $fixture.proposal.workCopy = $workcopy
            $before = @{sourceSha256=$fixture.context.originalSourceSha256}
            $after = @{sourceSha256=Get-Sha256 (Join-Path $workcopy $script:relative)}
            foreach ($measurement in @($before, $after)) {
                $measurement.scenarioSha256 = $fixture.context.scenarioSha256
                $measurement.profilerSha256 = $fixture.context.profilerSha256
            }
            if ($script:level -ne 'application') {
                $fixture.context.schema = 'ui-profiler/native-trial/v1'
                $fixture.context.originalRepository = Join-Path $caseRoot 'original-app'
                [void](New-Item -ItemType Directory $fixture.context.originalRepository)
                $fixture.context.native = @{originalRepository=$script:original;originalRevision=('c'*40)
                    source=$baseline;sourceSha256=$before.sourceSha256;level=$script:level}
                $before.nativeRuntime = @{sourceSha256=$before.sourceSha256;trialDirectory=$trial;level=$script:level;trialId='generic-fix'}
                $after.nativeRuntime = @{sourceSha256=$after.sourceSha256;trialDirectory=$trial;level=$script:level;trialId='generic-fix'}
                $after.sourceSha256 = $before.sourceSha256
            }
            foreach ($side in @('baseline', 'candidate')) {
                $path = Join-Path $trial "$side.json"
                Write-JsonFile $path $(if ($side -eq 'baseline') { $before } else { $after })
                $fixture.comparison[$side] = @{path=$path;sha256=Get-Sha256 $path}
            }
            $fixture.context.baseline = $fixture.comparison.baseline
            $recommendations = Join-Path $trial 'recommendations.json'
            Write-JsonFile $recommendations @{recommendations=@(@{id='generic-fix';title='Generic fix';summary='Unmeasured source hypothesis.'})}
            $fixture.context.recommendations = @{path=$recommendations;sha256=Get-Sha256 $recommendations}
            Write-JsonFile (Join-Path $runDir 'proposal.json') $fixture.proposal
            $fixture.run.proposalSha256 = Get-Sha256 (Join-Path $runDir 'proposal.json')
            $fixture.outcome.proposalSha256 = $fixture.run.proposalSha256
            Write-JsonFile (Join-Path $runDir 'approval.json') @{proposalSha256=$fixture.run.proposalSha256}
            Write-JsonFile (Join-Path $trial 'ade-run.json') $fixture.run
            [IO.File]::WriteAllText((Join-Path $runDir 'diff.patch'), 'synthetic ADE-owned diff')
            Write-JsonFile (Join-Path $trial 'comparison.json') $fixture.comparison
            Write-JsonFile (Join-Path $trial 'manifest.json') @{fixture='synthetic; manifest verifier separately tested'}
            $fixture.outcome.comparison = @{path=(Join-Path $trial 'comparison.json');sha256=Get-Sha256 (Join-Path $trial 'comparison.json')}
            $script:TrialContextPath = Join-Path $trial 'context.json'
            $script:OutcomePath = Join-Path $trial 'outcome.json'
            Write-JsonFile $script:TrialContextPath $fixture.context
            Write-JsonFile $script:OutcomePath $fixture.outcome
            $script:TrialContextSha256 = Get-Sha256 $script:TrialContextPath
            $script:clone = Join-Path $trial 'draft-publication\source'
            $script:mockGitCommands = [Collections.Generic.List[string]]::new()
            $script:mockBaseFetches = 0
            $result = Invoke-ProfiledDraftPublication | ConvertFrom-Json
            Check ($result.state -ceq 'draft-created' -and $result.url -like 'https://github.com/*/pull/42') 'Draft receipt not persisted.'
            Assert-FileHash $result.report.path $result.report.sha256
            Assert-FileHash $result.receipt.path $result.receipt.sha256
            Check ((Read-JsonFile $result.receipt.path).commit -ceq ('d'*40)) 'Receipt has the wrong commit.'
            Check ((Get-Content -LiteralPath (Join-Path $script:original $script:relative) -Raw) -ceq 'synthetic baseline') 'Original source changed.'
            Check ($script:mockGitCommands.Count -gt 3) 'Mock publishing path did not reach clone-only mutations.'
            Check ($env:GH_TOKEN -ceq $inheritedToken) 'Credential leaked into parent environment.'
        }
    }
    Check ($script:processCalls -eq 0) 'An offline test attempted an external process.'
    Write-Output "$script:passed synthetic offline draft-publication checks passed; zero network, commits, pushes or PR creations."
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force
}

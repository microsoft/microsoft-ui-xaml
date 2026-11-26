#Requires -Version 7
<#
.SYNOPSIS
Explicitly publishes one already approved, verified profiler candidate as a draft PR.
.DESCRIPTION
Never runs a model, check, build, or capture. Outcome.validation must contain the exact
trusted ADE check commands and results. All eligibility checks precede credentials,
network access, and publication writes. Failures leave existing trial evidence intact.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$TrialContextPath,
    [Parameter(Mandatory)][ValidatePattern('^[a-fA-F0-9]{64}$')][string]$TrialContextSha256,
    [Parameter(Mandatory)][string]$OutcomePath,
    [string]$GitHubAccount,
    [string]$BaseBranch
)
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\ProfilerHarness.psm1" -DisableNameChecking
Import-Module "$PSScriptRoot\NativeTrials.psm1" -DisableNameChecking

function Invoke-PublishProcess([string]$File, [string[]]$Arguments, [hashtable]$Environment = @{}) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = (Get-Command $File -CommandType Application -ErrorAction Stop).Source
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.CreateNoWindow = $true
    # Do not let inherited Git configuration redirect github.com credentials or
    # commands to another host, index, worktree, filter, or credential helper.
    if ($File -eq 'git') {
        foreach ($name in @('GIT_DIR', 'GIT_WORK_TREE', 'GIT_INDEX_FILE', 'GIT_COMMON_DIR',
            'GIT_CONFIG', 'GIT_CONFIG_PARAMETERS', 'GIT_OBJECT_DIRECTORY', 'GIT_ALTERNATE_OBJECT_DIRECTORIES',
            'GIT_SSH', 'GIT_SSH_COMMAND', 'GIT_PROXY_COMMAND', 'GIT_EXEC_PATH')) { [void]$start.Environment.Remove($name) }
        $start.Environment['GIT_CONFIG_NOSYSTEM'] = '1'
        $start.Environment['GIT_CONFIG_GLOBAL'] = 'NUL'
        $start.Environment['GIT_CONFIG_COUNT'] = '0'
        $start.Environment['GIT_OPTIONAL_LOCKS'] = '0'
        $start.Environment['GIT_TERMINAL_PROMPT'] = '0'
        $start.Environment['GIT_NO_LAZY_FETCH'] = '1'
    }
    foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
    foreach ($name in $Environment.Keys) {
        if ($null -eq $Environment[$name]) { [void]$start.Environment.Remove($name) }
        else { $start.Environment[$name] = $Environment[$name] }
    }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    $buffer = [IO.MemoryStream]::new()
    try {
        [void]$process.Start()
        $stdout = $process.StandardOutput.BaseStream.CopyToAsync($buffer)
        $stderr = $process.StandardError.ReadToEndAsync()
        if (!$process.WaitForExit(300000)) {
            $process.Kill($true)
            throw 'Publication command timed out; inspect retained artifacts before retrying.'
        }
        $stdout.GetAwaiter().GetResult()
        [void]$stderr.GetAwaiter().GetResult()
        # Do not echo command output on errors: credential helpers can include secrets.
        if ($process.ExitCode -ne 0) { throw "Publication command failed ($File, exit $($process.ExitCode))." }
        $bytes = $buffer.ToArray()
        @{ bytes = $bytes; text = [Text.Encoding]::UTF8.GetString($bytes).TrimEnd("`r", "`n") }
    } finally { $buffer.Dispose(); $process.Dispose() }
}

function Get-PublishGit([string]$Repository, [string[]]$Arguments, [hashtable]$Environment = @{}) {
    (Invoke-PublishProcess git (@('-C', $Repository, '-c', 'core.longpaths=true', '-c', 'core.quotepath=false',
        '-c', 'core.hooksPath=NUL', '-c', 'core.fsmonitor=false', '-c', 'core.untrackedCache=false',
        '-c', 'gc.auto=0', '-c', 'maintenance.auto=false') + $Arguments) $Environment).text
}

function Get-PublishBlobHash([string]$Repository, [string]$Revision, [string]$Path) {
    $bytes = (Invoke-PublishProcess git @('-C', $Repository, '-c', 'core.longpaths=true',
        'cat-file', 'blob', "${Revision}:$($Path.Replace('\', '/'))")).bytes
    [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([byte[]]$bytes)).ToLowerInvariant()
}

function Invoke-PublishSourceCheck([scriptblock]$Action) {
    # Shared validators spawn their own Git commands. Append a child-command override
    # only while they run, preserving any existing ADE launcher configuration.
    $countText = [Environment]::GetEnvironmentVariable('GIT_CONFIG_COUNT', 'Process')
    $count = 0
    if ($countText -and (![int]::TryParse($countText, [ref]$count) -or $count -lt 0 -or $count -gt 10000)) {
        throw 'Invalid inherited command-scoped Git configuration count.'
    }
    $values = @{
        GIT_CONFIG_COUNT = [string]($count + 1)
        "GIT_CONFIG_KEY_$count" = 'core.longpaths'
        "GIT_CONFIG_VALUE_$count" = 'true'
    }
    $saved = @{}
    try {
        foreach ($name in $values.Keys) {
            $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
            [Environment]::SetEnvironmentVariable($name, $values[$name], 'Process')
        }
        & $Action
    } finally {
        foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    }
}

function Assert-PublishPath([string]$Root, [string]$Relative) {
    if (!$Relative -or [IO.Path]::IsPathRooted($Relative) -or
        $Relative.Replace('\', '/') -match '(^|/)\.\.?(/|$)|:|[\x00-\x1f*?\[\]]') {
        throw 'Publication paths must be literal, contained source paths.'
    }
    $prefix = [IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    $path = [IO.Path]::GetFullPath((Join-Path $prefix $Relative.Replace('/', '\')))
    if (!$path.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Publication path escapes source.' }
    $ancestor = $path
    while ($ancestor) {
        if ((Test-Path -LiteralPath $ancestor) -and
            ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw 'Publication source/output must not traverse filesystem links.'
        }
        $ancestor = Split-Path $ancestor -Parent
    }
    $path
}

function Get-PublishRepository([string]$Origin) {
    if ($Origin -cmatch '^https://github\.com/([A-Za-z0-9][A-Za-z0-9-]*)/([A-Za-z0-9_.-]+?)(?:\.git)?/?$' -or
        $Origin -cmatch '^git@github\.com:([A-Za-z0-9][A-Za-z0-9-]*)/([A-Za-z0-9_.-]+?)(?:\.git)?$' -or
        $Origin -cmatch '^ssh://git@github\.com/([A-Za-z0-9][A-Za-z0-9-]*)/([A-Za-z0-9_.-]+?)(?:\.git)?/?$') {
        if ($Matches[2] -in @('.', '..')) { throw 'Invalid GitHub origin repository name.' }
        return "$($Matches[1])/$($Matches[2])"
    }
    throw 'Original origin must be a plain github.com HTTPS or git SSH repository (no credentials, query, or alternate host).'
}

function Assert-PublishEligibility($Context, $Outcome, $Run, $Proposal, $Approval, $Comparison) {
    if ($Context.schema -notin @('ui-profiler/native-trial/v1', 'ui-profiler/trial/v1') -or
        $Outcome.schema -cne 'ui-profiler/independent-trial-outcome/v1' -or
        $Outcome.state -cne 'verified-comparison' -or
        $Outcome.chainVerified -isnot [bool] -or !$Outcome.chainVerified -or
        $Outcome.adeChecksPassed -isnot [bool] -or !$Outcome.adeChecksPassed -or
        $Outcome.originalRepositoryUnchanged -isnot [bool] -or !$Outcome.originalRepositoryUnchanged) {
        throw 'Only an independently verified comparison with all three integrity gates may publish.'
    }
    if (!$Context.recommendationId -or $Outcome.recommendationId -cne $Context.recommendationId -or
        $Run.proposalSha256 -cnotmatch '^[a-f0-9]{64}$' -or
        $Outcome.proposalSha256 -cne $Run.proposalSha256 -or $Approval.proposalSha256 -cne $Run.proposalSha256 -or
        !$Proposal.workCopy) { throw 'Exact-hash approval and matching recommendation/proposal identities are required.' }
    if ($Comparison.schema -cne 'ui-profiler/comparison/v2') { throw 'Only MUX profiler multidimensional comparisons are supported.' }
    $improvements = @($Comparison.metrics | Where-Object {
        $_.primary -is [bool] -and $_.primary -and $_.status -ceq 'available' -and $_.baselineStatus -ceq 'available' -and
        $_.candidateStatus -ceq 'available' -and $_.change -ceq 'observed-improvement' -and
        $_.direction -ceq 'lower' -and $null -ne $_.baseline -and $null -ne $_.candidate -and
        [double]::IsFinite([double]$_.baseline) -and [double]::IsFinite([double]$_.candidate) -and
        [double]$_.baseline -gt [double]$_.candidate -and [double]$_.candidate -ge 0 -and
        $null -ne $_.delta -and [double]$_.delta -lt 0
    })
    if (!$improvements.Count) { throw 'No demonstrated available primary improvement; retain evidence locally, do not publish a source fix.' }
    if (!@($Outcome.validation).Count) { throw 'Outcome.validation must supply exact trusted ADE check commands and results.' }
    foreach ($check in $Outcome.validation) {
        if ($check.command -isnot [string] -or [string]::IsNullOrWhiteSpace($check.command) -or
            $check.result -isnot [string] -or [string]::IsNullOrWhiteSpace($check.result)) {
            throw 'Each trusted validation record requires its exact command and observed result.'
        }
    }
}

function Assert-PublishComparison([string]$Path, $Context) {
    Assert-PublishManifest (Join-Path (Split-Path $Path -Parent) 'manifest.json') $Path
    # Revalidate seals AND recompute every comparison row from the retained MUX analyses,
    # without creating reports, __pycache__, or capture outputs.
    $code = @'
import importlib.util, json, pathlib, sys
analyzer, comparison, trial = sys.argv[1:]
spec = importlib.util.spec_from_file_location("publish_analyzer", analyzer)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
p = pathlib.Path(comparison)
seal = module.verify_manifest(p.parent / "manifest.json")
c = module.load_json(p)
if not any(pathlib.Path(x["path"]).resolve() == p.resolve() and x["sha256"] == module.sha256(p) for x in seal["outputs"]):
    raise ValueError("Comparison not sealed as an output")
if not any(x.get("origin") == "analyzer" and x["sha256"] == module.sha256(analyzer) for x in seal["inputs"]):
    raise ValueError("Comparison analyzer differs from frozen implementation")
for side in ("baseline", "candidate"):
    analysis = pathlib.Path(c[side + "Analysis"])
    if not any(pathlib.Path(x["path"]).resolve() == analysis.resolve() for x in seal["inputs"]):
        raise ValueError("Analysis not bound to comparison seal")
    analysis_seal = module.verify_manifest(analysis.parent / "manifest.json")
    if not any(pathlib.Path(x["path"]).resolve() == analysis.resolve() for x in analysis_seal["outputs"]):
        raise ValueError("Analysis not sealed")
before, after = [module.load_json(c[side + "Analysis"]) for side in ("baseline", "candidate")]
if c["baseline"] != before["measurement"] or c["candidate"] != after["measurement"]:
    raise ValueError("Measurement substitution")
if c["metrics"] != module.comparison_rows(before, after):
    raise ValueError("Comparison rows differ from measured analyses")
for side, report in (("baseline", before), ("candidate", after)):
    measured = module.load_json(c[side]["path"])
    if report["inputs"]["sourceSha256"] != measured["sourceSha256"]:
        raise ValueError("Analysis source identity mismatch")
print("Verified MUX comparison seals, provenance, all rows, and pair compatibility.")
'@
    [void](Invoke-PublishProcess python @('-B', '-c', $code, (Join-Path $PSScriptRoot 'Analyze-Profiler.py'), $Path, $Context.directory))
}

function Assert-PublishManifest([string]$Path, [string]$OutputPath, [string[]]$Ancestors = @()) {
    $full = [IO.Path]::GetFullPath($Path)
    if ($full -in $Ancestors) { throw 'Cyclic profiler evidence manifest.' }
    $manifest = Read-JsonFile $Path
    if ($manifest.schema -cne 'ui-profiler/evidence-manifest/v1' -or
        !@($manifest.inputs).Count -or !@($manifest.outputs).Count) { throw 'Incomplete profiler evidence manifest.' }
    foreach ($reference in @($manifest.inputs) + @($manifest.outputs)) {
        if ($reference.sha256 -cnotmatch '^[a-f0-9]{64}$') { throw 'Missing evidence digest.' }
        Assert-FileHash $reference.path $reference.sha256
    }
    if ($OutputPath -and !@($manifest.outputs | Where-Object {
        [IO.Path]::GetFullPath($_.path) -eq [IO.Path]::GetFullPath($OutputPath)
    }).Count) { throw 'The verified comparison must be a sealed manifest output.' }
    foreach ($reference in @($manifest.inputs | Where-Object origin -CEQ 'evidence-manifest')) {
        Assert-PublishManifest $reference.path '' (@($Ancestors) + $full)
    }
}

function Get-PublishTemplate([string]$Repository, [string]$Revision) {
    $paths = (Get-PublishGit $Repository @('ls-tree', '-r', '--name-only', $Revision, '--', '.github')) -split "`n"
    $default = @($paths | Where-Object { $_ -ieq '.github/pull_request_template.md' })
    $alternatives = @($paths | Where-Object { $_ -imatch '^\.github/PULL_REQUEST_TEMPLATE/[^/]+\.md$' })
    $chosen = if ($default.Count -eq 1) { $default[0] } elseif ($alternatives.Count -eq 1) { $alternatives[0] }
    elseif ($default.Count -gt 1 -or $alternatives.Count -gt 1) { throw 'Multiple PR templates require an explicit choice; publication is blocked.' }
    else { return @{ path = $null; text = ''; sha256 = $null } }
    $text = Get-PublishGit $Repository @('cat-file', 'blob', "${Revision}:$chosen")
    @{ path = $chosen; text = $text; sha256 = Get-PublishBlobHash $Repository $Revision $chosen }
}

function ConvertTo-PublishCell($Value) {
    if ($null -eq $Value) { return 'null (unavailable)' }
    ([string]$Value).Replace('|', '\|').Replace("`r", '').Replace("`n", '<br>')
}

function New-PublishReport($Context, $Outcome, $Run, $Comparison, $Recommendation, $Files, $Hashes, [string]$DiffSummary) {
    $lines = [Collections.Generic.List[string]]::new()
    $lines.Add("# Profiled fix: $($Context.recommendationId)")
    $lines.Add('')
    $lines.Add("Level: $($Hashes.level). Recommendation: $($Recommendation.title)")
    $lines.Add('')
    $lines.Add('## Measured evidence (MUX WinUIProfiler only)')
    $lines.Add('One observed baseline/candidate run pair, not statistical evidence or a universal improvement. Regressions, partial results and nulls are retained below. No combined-patch claim; elapsed UI time is not exclusive CPU time.')
    $lines.Add([string]$Comparison.interpretation)
    $lines.Add('')
    $lines.Add('| Family / scope / metric | Primary | Unit | Baseline | Candidate | Absolute delta (candidate - baseline) | Reduction % | Baseline / candidate status | Comparison status / change |')
    $lines.Add('|---|---|---|---:|---:|---:|---:|---|---|')
    foreach ($row in $Comparison.metrics) {
        $cells = @("$($row.family) / $($row.scope) / $($row.metric)", $row.primary, $row.unit,
            $row.baseline, $row.candidate, $row.delta, $row.reductionPercent,
            "$($row.baselineStatus) / $($row.candidateStatus)", "$($row.status) / $($row.change)")
        $lines.Add('| ' + (($cells | ForEach-Object { ConvertTo-PublishCell $_ }) -join ' | ') + ' |')
    }
    $lines.Add('')
    $lines.Add('## Capabilities and missing evidence')
    foreach ($family in $Comparison.capabilities.PSObject.Properties) {
        $lines.Add("- $($family.Name): baseline=$($family.Value.baseline); candidate=$($family.Value.candidate). Baseline notes: $($family.Value.baselineNotes) Candidate notes: $($family.Value.candidateNotes)")
    }
    $lines.Add("Legacy evidence: $($Comparison.legacyEvidence). Missing dimensions are unverified, never zero or inferred improvements.")
    $lines.Add('')
    $lines.Add('## Inferred / source or model reasoning (not measured evidence)')
    $lines.Add([string]$Recommendation.summary)
    foreach ($hypothesis in @($Recommendation.hypotheses)) { $lines.Add("- $hypothesis") }
    $lines.Add('')
    $lines.Add('## Source fix')
    $lines.Add("Exact measured GitHub base revision: $($Hashes.measuredBaseRevision). A newer descendant is not this measured baseline.")
    $lines.Add('Only scoped existing source files are exported from the approved ADE workcopy into a fresh clone of the original Git history. No report is added to product source.')
    foreach ($file in $Files) { $lines.Add("- $file") }
    $lines.Add('```text')
    $lines.Add($DiffSummary)
    $lines.Add('```')
    $lines.Add('')
    $lines.Add('## Exact ADE validation commands and observed results')
    foreach ($check in $Outcome.validation) {
        $lines.Add('```text')
        $lines.Add($check.command)
        $lines.Add('```')
        $lines.Add("Result: $($check.result)")
    }
    $lines.Add('ADE exact-hash proposal approval, chain verification, required checks and unchanged original repositories were verified before publication. Publication itself performs no new capture or code generation.')
    $lines.Add('')
    $lines.Add('## Evidence hashes and local artifacts')
    $lines.Add("ADE run: $($Run.runId). All paths below are local artifacts, NOT reviewer-accessible attachments.")
    foreach ($name in @('context', 'outcome', 'recommendations', 'proposal', 'approval', 'diff', 'comparison', 'manifest', 'baseline', 'candidate')) {
        $lines.Add("- ${name}: $($Hashes.$name.path); SHA-256 $($Hashes.$name.sha256)")
    }
    $lines -join "`n"
}

function Merge-PublishTemplate([string]$Template, [string]$Report) {
    if (!$Template) { return $Report }
    # Preserve all template structure, comments and checklist states/order. Replace only
    # standalone prose prompts; unknown prose is left intact rather than guessed away.
    $filled = [regex]::Replace($Template,
        '(?im)^[ \t]*(?:\[(?:describe|insert|add|provide)[^\r\n]*\]|_(?:describe|insert|add|provide)[^\r\n]*_|(?:TODO|TBD))[ \t]*$',
        'See the verified profiler evidence and exact validation results below.')
    $filled = [regex]::Replace($filled, '(?im)^(#{1,6}[ \t]+(?:description|summary|motivation|solution|changes)[^\r\n]*)$',
        '$1' + "`n`n" + 'This draft contains one approved source fix; its complete scope, observed results and limitations are reported below.')
    $filled = [regex]::Replace($filled, '(?im)^(#{1,6}[ \t]+(?:validation|testing|tests)[^\r\n]*)$',
        '$1' + "`n`n" + 'See Exact ADE validation commands and observed results below; publication does not run additional tests or captures.')
    $filled + "`n`n" + $Report
}

function Assert-PublishMeasuredBase([string]$Repository, [string]$Branch, [string]$Revision, [hashtable]$Environment) {
    $auth = @('-c', 'credential.helper=', '-c', 'credential.https://github.com.helper=!gh auth git-credential')
    try {
        [void](Get-PublishGit $Repository ($auth + @('fetch', '--no-tags', 'origin',
            "refs/heads/${Branch}:refs/remotes/publication/base")) $Environment)
    } catch {
        throw "Cannot verify GitHub base '$Branch' at measured originalRevision ($Revision). The branch may be absent or moved; select an appropriate measured base or prepare a new baseline/comparison. Publication stopped without rebasing or automatically targeting a dependent PR."
    }
    $tip = Get-PublishGit $Repository @('rev-parse', '--verify', 'refs/remotes/publication/base^{commit}')
    if ($tip -cne $Revision) {
        throw "GitHub base '$Branch' is not the exact measured originalRevision ($Revision); it is absent, unmerged, newer, or moved. Select an appropriate explicitly measured base or prepare a new baseline/comparison. No automatic rebase, dependent PR targeting, or stack creation is allowed."
    }
}

function Assert-PublishPrDiff([string]$Repository, [string]$Revision, [string]$Commit,
    [string]$WorkCopy, [string[]]$Files, [string[]]$AllowedPaths) {
    if ((Get-PublishGit $Repository @('rev-parse', "${Commit}^")) -cne $Revision -or
        (Get-PublishGit $Repository @('merge-base', $Revision, $Commit)) -cne $Revision) {
        throw 'Proposed PR must be one optimization commit directly on the exact measured GitHub base.'
    }
    $changes = @((Get-PublishGit $Repository @('diff', '--name-status', '--no-renames',
        "${Revision}...${Commit}", '--')) -split "`n" | Where-Object { $_ })
    $allowed = @($AllowedPaths | ForEach-Object { $_.Replace('\', '/') })
    $expected = @($Files | ForEach-Object { $_.Replace('\', '/') } | Sort-Object -CaseSensitive)
    $actual = @(
        foreach ($change in $changes) {
            if ($change -cnotmatch '^M\t(.+)$' -or $Matches[1] -cnotin $allowed -or $Matches[1] -cnotin $expected) {
                throw 'Actual GitHub base...candidate PR diff imports unrelated, added, deleted, renamed, or unapproved source.'
            }
            $Matches[1]
        }
    )
    if (!$expected.Count -or (($actual | Sort-Object -CaseSensitive) -join "`n") -cne ($expected -join "`n")) {
        throw 'Actual GitHub base...candidate PR diff does not exactly match the measured changed paths.'
    }
    foreach ($relative in $actual) {
        $source = Assert-PublishPath $WorkCopy $relative
        if ((Get-PublishBlobHash $Repository $Commit $relative) -cne (Get-Sha256 $source)) {
            throw 'Actual PR candidate blob bytes differ from the measured ADE source fix.'
        }
    }
}

function Invoke-ProfiledDraftPublication {
    Assert-FileHash $TrialContextPath $TrialContextSha256
    $context = Read-JsonFile $TrialContextPath
    $outcomeHash = Get-Sha256 $OutcomePath
    $outcome = Read-JsonFile $OutcomePath
    $runPath = Join-Path $context.directory 'ade-run.json'
    $run = Read-JsonFile $runPath
    if ($run.runId -cnotmatch '^[A-Za-z0-9][A-Za-z0-9_.-]{0,100}$') { throw 'Invalid ADE run identity.' }
    $runDir = Join-Path $context.directory "ade-runs\$($run.runId)"
    if ($run.runDir -and [IO.Path]::GetFullPath($run.runDir) -ne [IO.Path]::GetFullPath($runDir)) { throw 'ADE run directory mismatch.' }
    $proposalPath = Join-Path $runDir 'proposal.json'
    $approvalPath = Join-Path $runDir 'approval.json'
    Assert-FileHash $proposalPath $run.proposalSha256
    $proposal = Read-JsonFile $proposalPath
    $approval = Read-JsonFile $approvalPath
    Assert-FileHash $context.recommendations.path $context.recommendations.sha256
    Assert-FileHash $outcome.comparison.path $outcome.comparison.sha256
    $comparison = Read-JsonFile $outcome.comparison.path
    Assert-PublishEligibility $context $outcome $run $proposal $approval $comparison
    Invoke-PublishSourceCheck {
        if ($context.schema -eq 'ui-profiler/native-trial/v1') { Assert-NativeTrialContext $context }
        else { Assert-TrialContext $context }
    }
    $native = $context.schema -eq 'ui-profiler/native-trial/v1'
    $level = if ($native) { $context.native.level } else { 'application' }
    $repository = if ($native) { $context.native.originalRepository } else { $context.originalRepository }
    $revision = if ($native) { $context.native.originalRevision } else { $context.originalRevision }
    $baselineSource = if ($native) { $context.native.source } else { $context.originalRepository }
    if ($revision -cnotmatch '^[a-f0-9]{40,64}$') { throw 'Original Git revision is required.' }
    if (Get-PublishGit $repository @('status', '--porcelain', '--untracked-files=all', '--ignore-submodules=all')) {
        throw 'Original history repository contains dirty source; a measured dirty baseline cannot be silently replaced by originalRevision for publication.'
    }
    Invoke-PublishSourceCheck { Assert-ScopedCandidate $proposal.workCopy $context.allowedPaths -Level $level }
    $files = @((Get-PublishGit $proposal.workCopy @('diff', '--name-only', 'ade-baseline', '--')) -split "`n" | Where-Object { $_ })
    foreach ($relative in $context.allowedPaths) {
        $baselineFile = Assert-PublishPath $baselineSource $relative
        [void](Assert-PublishPath $proposal.workCopy $relative)
        if ((Get-PublishBlobHash $repository $revision $relative) -cne (Get-Sha256 $baselineFile) -or
            (Get-PublishBlobHash $proposal.workCopy 'ade-baseline' $relative) -cne (Get-Sha256 $baselineFile)) {
            throw 'Measured source baseline differs from originalRevision; refusing to import pre-existing dirty changes.'
        }
    }
    foreach ($side in @('baseline', 'candidate')) {
        Assert-FileHash $comparison.$side.path $comparison.$side.sha256
        Assert-Measurement (Read-JsonFile $comparison.$side.path)
    }
    $before = Read-JsonFile $comparison.baseline.path
    $after = Read-JsonFile $comparison.candidate.path
    foreach ($name in @('scenarioSha256', 'profilerSha256')) {
        if (!$context.$name -or $before.$name -cne $context.$name -or $after.$name -cne $context.$name) {
            throw "Measured pair differs from frozen trial inputs: $name"
        }
    }
    $expectedBefore = if ($native) { $context.native.sourceSha256 } else { $context.originalSourceSha256 }
    $expectedAfter = Invoke-PublishSourceCheck { Get-SourceHash $proposal.workCopy }
    if ($native) {
        if ($before.nativeRuntime.sourceSha256 -cne $expectedBefore -or $after.nativeRuntime.sourceSha256 -cne $expectedAfter -or
            $before.nativeRuntime.trialDirectory -ne $context.directory -or $after.nativeRuntime.trialDirectory -ne $context.directory -or
            $before.nativeRuntime.level -cne $level -or $after.nativeRuntime.level -cne $level -or
            $before.nativeRuntime.trialId -cne $context.recommendationId -or $after.nativeRuntime.trialId -cne $context.recommendationId -or
            $before.sourceSha256 -cne $context.originalSourceSha256 -or $after.sourceSha256 -cne $context.originalSourceSha256) {
            throw 'Native measured pair is not this approved source candidate with unchanged application.'
        }
    } elseif ($comparison.baseline.sha256 -cne $context.baseline.sha256 -or
        $before.sourceSha256 -cne $expectedBefore -or $after.sourceSha256 -cne $expectedAfter) {
        throw 'Measured app candidate does not match the approved workcopy and baseline.'
    }
    Assert-PublishComparison $outcome.comparison.path $context
    $entries = @((Read-JsonFile $context.recommendations.path).recommendations | Where-Object id -CEQ $context.recommendationId)
    if ($entries.Count -ne 1) { throw 'Ambiguous recommendation.' }
    $template = Get-PublishTemplate $repository $revision
    $repo = Get-PublishRepository (Get-PublishGit $repository @('remote', 'get-url', 'origin'))
    $directory = Join-Path $context.directory 'draft-publication'
    Assert-RunDirectory $directory @($repository, $context.originalRepository, $proposal.workCopy, $baselineSource)
    if (Test-Path -LiteralPath $directory) { throw 'Publication already attempted; inspect retained receipt/branch before any manual retry.' }
    if ($GitHubAccount -and $GitHubAccount -cnotmatch '^[A-Za-z0-9][A-Za-z0-9-]{0,38}$') { throw 'Invalid GitHub account.' }
    if ($BaseBranch) { [void](Get-PublishGit $repository @('check-ref-format', '--branch', $BaseBranch)) }
    $hashes = @{ level = $level; measuredBaseRevision = $revision
        context = @{path=$TrialContextPath;sha256=$TrialContextSha256}; outcome = @{path=$OutcomePath;sha256=$outcomeHash}
        recommendations=$context.recommendations; proposal=@{path=$proposalPath;sha256=$run.proposalSha256}
        approval=@{path=$approvalPath;sha256=Get-Sha256 $approvalPath}
        diff=@{path=(Join-Path $runDir 'diff.patch');sha256=Get-Sha256 (Join-Path $runDir 'diff.patch')}
        comparison=$outcome.comparison; baseline=$comparison.baseline; candidate=$comparison.candidate
        manifest=@{path=(Join-Path (Split-Path $outcome.comparison.path -Parent) 'manifest.json')
            sha256=Get-Sha256 (Join-Path (Split-Path $outcome.comparison.path -Parent) 'manifest.json')} }
    $summary = Get-PublishGit $proposal.workCopy @('diff', '--stat', 'ade-baseline', '--')
    $report = New-PublishReport $context $outcome $run $comparison $entries[0] $files $hashes $summary
    $body = Merge-PublishTemplate $template.text $report
    if ($body.Length -gt 60000) { throw 'Full evidence and template exceed a safe PR body size; retain locally, no truncated evidence or artifact upload.' }
    # Eligibility, source-byte and manifest gates above are strictly read-only.
    $environment = @{ GH_HOST='github.com'; GH_PROMPT_DISABLED='1'; GIT_TERMINAL_PROMPT='0'; GH_REPO=$null
        GH_ENTERPRISE_TOKEN=$null; GITHUB_ENTERPRISE_TOKEN=$null }
    try {
        if ($GitHubAccount) {
            $lookup = $environment.Clone()
            $lookup.GH_TOKEN = $null
            $lookup.GITHUB_TOKEN = $null
            $environment.GH_TOKEN = (Invoke-PublishProcess gh @('auth', 'token', '--hostname', 'github.com', '--user', $GitHubAccount) $lookup).text
            $environment.GITHUB_TOKEN = $null
            if (!$environment.GH_TOKEN) { throw 'Selected GitHub credential is unavailable.' }
            $viewer = (Invoke-PublishProcess gh @('api', 'user', '--jq', '.login') $environment).text
            if ($viewer -ine $GitHubAccount) { throw 'Selected credential resolves to another GitHub account.' }
        }
        $remote = (Invoke-PublishProcess gh @('repo', 'view', $repo, '--json', 'nameWithOwner,viewerPermission,defaultBranchRef') $environment).text | ConvertFrom-Json
        if ($remote.nameWithOwner -ine $repo -or $remote.viewerPermission -notin @('WRITE', 'MAINTAIN', 'ADMIN')) {
            throw 'No write permission to original GitHub repository. Request access or explicitly arrange a user-owned fork; no fork, auth switch, or fabricated PR URL was created.'
        }
        if (!$BaseBranch) { $BaseBranch = $remote.defaultBranchRef.name }
        [void](Get-PublishGit $repository @('check-ref-format', '--branch', $BaseBranch))
        [void](New-Item -ItemType Directory $directory)
        $reportPath = Join-Path $directory 'report.md'
        $bodyPath = Join-Path $directory 'pr-body.md'
        [IO.File]::WriteAllText($reportPath, $report)
        [IO.File]::WriteAllText($bodyPath, $body)
        $clone = Join-Path $directory 'source'
        [void](Invoke-PublishProcess git @('-c', 'core.longpaths=true', '-c', 'core.hooksPath=NUL', 'clone', '--no-local', '--no-hardlinks',
            '--no-checkout', '--', $repository, $clone))
        [void](Get-PublishGit $clone @('config', 'core.autocrlf', 'false'))
        [void](Get-PublishGit $clone @('remote', 'set-url', 'origin', "https://github.com/$repo.git"))
        Assert-PublishMeasuredBase $clone $BaseBranch $revision $environment
        $branch = "profiled/$($context.recommendationId -replace '[^a-zA-Z0-9-]', '-')-$($run.proposalSha256.Substring(0,12))-$([Guid]::NewGuid().ToString('N').Substring(0,8))"
        [void](Get-PublishGit $clone @('checkout', '--quiet', '-b', $branch, $revision))
        [void](Get-PublishGit $clone @('tag', '-f', 'ade-baseline', $revision))
        foreach ($relative in $files) {
            $source = Assert-PublishPath $proposal.workCopy $relative
            $target = Assert-PublishPath $clone $relative
            Copy-Item -LiteralPath $source -Destination $target
            if ((Get-Sha256 $source) -cne (Get-Sha256 $target)) { throw 'Publication export changed source bytes.' }
        }
        Invoke-PublishSourceCheck { Assert-ScopedCandidate $clone $context.allowedPaths -Level $level }
        [void](Get-PublishGit $clone (@('add', '--') + $files))
        $staged = @((Get-PublishGit $clone @('diff', '--cached', '--name-only')) -split "`n" | Where-Object { $_ })
        if (($staged -join "`n") -cne ($files -join "`n")) { throw 'Staged publication files differ from measured fix.' }
        foreach ($relative in $files) {
            if ((Get-PublishBlobHash $clone '' $relative) -cne (Get-Sha256 (Join-Path $proposal.workCopy $relative))) {
                throw 'Git filters or attributes changed the measured candidate bytes.'
            }
        }
        foreach ($name in @('context', 'outcome', 'recommendations', 'proposal', 'approval', 'diff', 'comparison', 'manifest', 'baseline', 'candidate')) {
            Assert-FileHash $hashes.$name.path $hashes.$name.sha256
        }
        Invoke-PublishSourceCheck {
            Assert-ScopedCandidate $proposal.workCopy $context.allowedPaths -Level $level
            if ((Get-SourceHash $proposal.workCopy) -cne $expectedAfter) { throw 'Candidate changed during publication.' }
            Assert-OriginalSource $context
            if ($native) { Assert-NativeTrialContext $context }
        }
        $title = "Profiled fix: $($context.recommendationId)"
        $message = "$title`n`nObserved one baseline/candidate pair; see PR evidence for regressions and limitations.`n`nCo-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda"
        [void](Get-PublishGit $clone @('-c', 'user.name=Copilot', '-c', 'user.email=223556219+Copilot@users.noreply.github.com',
            '-c', 'commit.gpgsign=false', 'commit', '--quiet', '-m', $message))
        $commit = Get-PublishGit $clone @('rev-parse', 'HEAD')
        Assert-PublishPrDiff $clone $revision $commit $proposal.workCopy $files $context.allowedPaths
        $auth = @('-c', 'credential.helper=', '-c', 'credential.https://github.com.helper=!gh auth git-credential')
        $receiptPath = Join-Path $directory 'publication.json'
        $receipt = @{schema='ui-profiler/draft-publication/v1';state='prepared';draft=$true;url=$null
            repository=$repo;branch=$branch;baseBranch=$BaseBranch;measuredBaseRevision=$revision;commit=$commit;recommendationId=$context.recommendationId
            proposalSha256=$run.proposalSha256;comparison=$outcome.comparison;report=@{path=$reportPath;sha256=Get-Sha256 $reportPath}
            body=@{path=$bodyPath;sha256=Get-Sha256 $bodyPath};template=$template;hashes=$hashes}
        Write-JsonFile $receiptPath $receipt
        Assert-PublishMeasuredBase $clone $BaseBranch $revision $environment
        Assert-PublishPrDiff $clone $revision $commit $proposal.workCopy $files $context.allowedPaths
        [void](Get-PublishGit $clone ($auth + @('push', '--porcelain', 'origin', "HEAD:refs/heads/$branch")) $environment)
        $receipt.state = 'pushed'
        Write-JsonFile $receiptPath $receipt
        Assert-PublishMeasuredBase $clone $BaseBranch $revision $environment
        Assert-PublishPrDiff $clone $revision $commit $proposal.workCopy $files $context.allowedPaths
        [void](Invoke-PublishProcess gh @('pr', 'create', '--repo', $repo, '--draft', '--base', $BaseBranch,
            '--head', $branch, '--title', $title, '--body-file', $bodyPath) $environment)
        $pr = (Invoke-PublishProcess gh @('pr', 'view', $branch, '--repo', $repo,
            '--json', 'url,isDraft,headRefName,headRefOid,baseRefName,baseRefOid') $environment).text | ConvertFrom-Json
        if (!$pr.isDraft -or $pr.headRefName -cne $branch -or $pr.headRefOid -cne $commit -or
            $pr.baseRefName -cne $BaseBranch -or $pr.url -cnotmatch "^https://github\.com/$([regex]::Escape($repo))/pull/[0-9]+$") {
            throw 'GitHub did not return the exact expected draft PR; inspect pushed branch, do not retry automatically.'
        }
        $receipt.url = $pr.url
        $receipt.state = 'draft-base-verification-pending'
        Write-JsonFile $receiptPath $receipt
        if ($pr.baseRefOid -cne $revision) { throw 'GitHub draft base moved after the pre-creation check; retained draft is not verified measured evidence. Do not merge or claim exact measured provenance.' }
        Assert-PublishMeasuredBase $clone $BaseBranch $revision $environment
        $receipt.state = 'draft-created'
        $receipt.url = $pr.url
        Write-JsonFile $receiptPath $receipt
        $receipt.receipt = @{path=$receiptPath;sha256=Get-Sha256 $receiptPath}
        $receipt | ConvertTo-Json -Depth 60
    } finally {
        $environment.GH_TOKEN = $null
        $environment.GITHUB_TOKEN = $null
    }
}

Invoke-ProfiledDraftPublication

# Independent native trials never reuse a discovery recording as a source-built runtime baseline.
#Requires -Version 7
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\ProfilerHarness.psm1" -DisableNameChecking

function Assert-NativeTrialContext($Context) {
    if ($Context.schema -ne 'ui-profiler/native-trial/v1' -or $Context.strategy -ne 'independent' -or
        $Context.native.level -notin @('control', 'framework') -or
        $Context.native.authorization.baselineCaptures -ne 1 -or
        $Context.native.authorization.candidateCaptures -ne 1 -or
        $Context.native.authorization.nativeRuntimeChanges -ne $true) {
        throw 'An explicit independent native trial and bounded source-matched capture pair are required.'
    }
    foreach ($reference in @($Context.request, $Context.contracts, $Context.module, $Context.recommendations,
        $Context.baseline, $Context.analysis, $Context.expert, $Context.sourceInvestigation,
        $Context.nativeMcp, $Context.native.implementation.module, $Context.native.implementation.runtime,
        $Context.native.implementation.measure)) {
        Assert-FileHash $reference.path $reference.sha256
    }
    Assert-FrozenScenario $Context
    Assert-OriginalSource $Context
    if ((Get-ProfilerHash $Context.profilerCli) -ne $Context.profilerSha256 -or
        (Get-ProfilerHash $Context.nativeMcp.path) -ne $Context.nativeMcp.deploymentSha256 -or
        (Get-SourceHash $Context.nativeMcp.source $Context.nativeMcp.files) -ne $Context.nativeMcp.sourceSha256) {
        throw 'Frozen profiler or evidence-server implementation changed.'
    }
    $native = $Context.native
    if ([IO.Path]::GetFullPath($native.source) -ne (Join-Path $Context.directory 'native-source')) {
        throw 'The native baseline must be the trial-owned source snapshot.'
    }
    Assert-RunDirectory $Context.directory @($native.originalRepository, $Context.originalRepository, $native.applicationSource)
    $nativeFiles = @(Get-NativeRepositoryFiles $native.originalRepository)
    $sampleSubmodules = @(Get-NativeSampleSubmodules $native.originalRepository)
    if ((ConvertTo-Json @($sampleSubmodules | Select-Object path, revision) -Depth 4 -Compress) -cne
        (ConvertTo-Json @($native.excludedSampleSubmodules | Select-Object path, revision) -Depth 4 -Compress)) {
        throw 'Excluded sample submodule revisions changed.'
    }
    if ((Get-SourceHash $native.originalRepository $nativeFiles) -ne $native.originalSourceSha256 -or
        (git -C $native.originalRepository rev-parse HEAD) -ne $native.originalRevision -or
        (Get-SourceHash $native.source $native.files) -ne $native.sourceSha256 -or
        $native.sourceSha256 -ne $native.originalSourceSha256) {
        throw 'The original native source or independent native baseline snapshot changed.'
    }
    $appFiles = @(Get-RepositoryFiles $Context.originalRepository)
    if ((Get-SourceHash $native.applicationSource $appFiles) -ne $native.applicationSourceSha256 -or
        $native.applicationSourceSha256 -ne $Context.originalSourceSha256) {
        throw 'The application input changed; a native trial cannot also change app usage.'
    }
    $entries = @((Read-JsonFile $Context.recommendations.path).recommendations |
        Where-Object id -CEQ $Context.recommendationId)
    if ($entries.Count -ne 1 -or $entries[0].level -ne $native.level -or $entries[0].verdict -ne 'recommend' -or
        (ConvertTo-Json @($entries[0].files | ForEach-Object path) -Compress) -cne
        (ConvertTo-Json @($Context.allowedPaths) -Compress)) {
        throw 'Native trial target/file scope differs from its frozen expert recommendation.'
    }
}

function Assert-NativeTrialApproval($Context) {
    $proposal = Read-JsonFile (Join-Path $Context.directory 'ade-run.json')
    if (!$proposal.runId -or $proposal.runId -match '[/\\:]' -or $proposal.runId -in @('.', '..')) {
        throw 'Invalid native ADE run identity.'
    }
    $runDirectory = Join-Path (Join-Path $Context.directory 'ade-runs') $proposal.runId
    Assert-FileHash (Join-Path $runDirectory 'proposal.json') $proposal.proposalSha256
    $approval = Read-JsonFile (Join-Path $runDirectory 'approval.json')
    if ($approval.proposalSha256 -cne $proposal.proposalSha256) {
        throw 'Exact-hash native proposal approval is required before builds or captures.'
    }
}

function Invoke-NativeTrialCheck([string]$RepositoryPath, $Context) {
    Assert-NativeTrialContext $Context
    if ((Get-SourceHash $RepositoryPath) -eq $Context.native.sourceSha256) {
        Write-Output 'Native baseline source/inputs verified; builds and both captures wait for exact-hash approval and a changed candidate.'
        return
    }
    Assert-ScopedCandidate $RepositoryPath $Context.allowedPaths -Level $Context.native.level
    Assert-NativeTrialApproval $Context
    Import-Module "$PSScriptRoot\NativeRuntime.psm1" -Force -DisableNameChecking
    $applicationFiles = @(Get-RepositoryFiles $Context.originalRepository)
    $candidateSourceSha256 = Get-SourceHash $RepositoryPath
    foreach ($role in @('baseline', 'candidate')) {
        $measurementPath = Join-Path $Context.directory "$role\measurement.json"
        $runtimeDirectory = Join-Path $Context.directory "$role-runtime"
        $manifestPath = Join-Path $runtimeDirectory 'manifest.json'
        if (Test-Path -LiteralPath $measurementPath) {
            throw "The $role native capture is already spent; no repeated checks or captures."
        }
        $source = if ($role -eq 'baseline') { $Context.native.source } else { $RepositoryPath }
        $runtime = Invoke-NativeBuild -RepositoryPath $source -Level $Context.native.level -Directory $runtimeDirectory
        Assert-FileHash $runtime.path $runtime.sha256
        $manifestPath = $runtime.path
        $manifest = Read-JsonFile $manifestPath
        if ($manifest.source.sha256 -ne (Get-SourceHash $source $Context.native.files)) {
            throw 'Native build manifest does not describe the exact measured source.'
        }
        $appSource = Join-Path $Context.directory "$role-app-source"
        Copy-ApplicationSource $Context.native.applicationSource $appSource $applicationFiles
        [void](Invoke-Measurement $Context $appSource $role $applicationFiles -RuntimeManifestPath $manifestPath)
        Assert-NativeTrialContext $Context
        if ((Get-SourceHash $RepositoryPath) -ne $candidateSourceSha256) {
            throw 'The native build or capture changed candidate source.'
        }
    }
    $comparison = Compare-NativeMeasurements (Join-Path $Context.directory 'baseline\measurement.json') `
        (Join-Path $Context.directory 'candidate\measurement.json') (Join-Path $Context.directory 'multidimensional') $Context.profilerCli
    [void](Export-TrialControlComparison $Context)
    Write-Output ($comparison.counts | ConvertTo-Json -Compress)
}

function Compare-NativeMeasurements([string]$BaselinePath, [string]$CandidatePath, [string]$Directory, [string]$ProfilerCli) {
    $before = Read-JsonFile $BaselinePath
    $after = Read-JsonFile $CandidatePath
    Assert-Measurement $before
    Assert-Measurement $after
    if (!$before.nativeRuntime -or !$after.nativeRuntime -or
        $before.nativeRuntime.trialId -cne $after.nativeRuntime.trialId -or
        $before.nativeRuntime.level -ne $after.nativeRuntime.level -or
        $before.sourceSha256 -ne $after.sourceSha256 -or $before.executableSha256 -ne $after.executableSha256) {
        throw 'A native pair requires the same independent trial and unchanged application.'
    }
    if ((Get-ProfilerHash $ProfilerCli) -ne $before.profilerSha256) { throw 'The recorded profiler payload changed.' }
    if (Test-Path -LiteralPath $Directory) { throw 'Native comparisons must use a fresh output directory.' }
    New-Item -ItemType Directory $Directory | Out-Null
    $baseline = Export-MeasurementAnalysis $BaselinePath (Join-Path $Directory 'baseline') $ProfilerCli
    $candidate = Export-MeasurementAnalysis $CandidatePath (Join-Path $Directory 'candidate') $ProfilerCli
    $python = (Get-Command python -ErrorAction Stop).Path
    [void](Invoke-LoggedProcess $python @("$PSScriptRoot\Analyze-Profiler.py", 'compare', '--baseline', $baseline.path,
        '--candidate', $candidate.path, '--directory', $Directory) $Directory (Join-Path $Directory 'compare.log') 300)
    Read-JsonFile (Join-Path $Directory 'comparison.json')
}

function Invoke-LocalTrialGit([string]$Repository, [string[]]$Arguments) {
    if ($Arguments[0] -notin @('diff', 'cat-file')) { throw 'Local fix reports only permit read-only Git diff/blob queries.' }
    $start = [Diagnostics.ProcessStartInfo]::new((Get-Command git -CommandType Application).Source)
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    foreach ($argument in (@('-C', $Repository, '-c', 'core.longpaths=true', '-c', 'core.quotepath=false',
        '-c', 'core.fsmonitor=false', '-c', 'core.hooksPath=NUL') + $Arguments)) { $start.ArgumentList.Add($argument) }
    $start.Environment['GIT_OPTIONAL_LOCKS'] = '0'
    $start.Environment['GIT_NO_LAZY_FETCH'] = '1'
    $start.Environment['GIT_TERMINAL_PROMPT'] = '0'
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    $buffer = [IO.MemoryStream]::new()
    try {
        [void]$process.Start()
        $stdout = $process.StandardOutput.BaseStream.CopyToAsync($buffer)
        $stderr = $process.StandardError.ReadToEndAsync()
        if (!$process.WaitForExit(60000)) { $process.Kill($true); throw 'Local scoped fix inspection timed out.' }
        $stdout.GetAwaiter().GetResult()
        [void]$stderr.GetAwaiter().GetResult()
        if ($process.ExitCode -ne 0) { throw 'Local scoped fix inspection failed; no patch application is inferred.' }
        $bytes = $buffer.ToArray()
        @{ bytes = $bytes; text = [Text.Encoding]::UTF8.GetString($bytes).TrimEnd("`r", "`n") }
    } finally { $buffer.Dispose(); $process.Dispose() }
}

function Get-LocalTrialFixEvidence($Reference, $Outcome, $Comparison) {
    $fix = @{ schema='ui-profiler/local-fix/v1'; recommendationId=$Reference.id
        trialContextSha256=$Reference.context.sha256; proposalSha256=$Reference.proposalSha256
        disposition = 'unavailable'; actuallyApplied = $null; location = 'ADE workcopy only, never original source'
        scopeVerified = $false; measuredCandidateBound = $false; changedFiles = @(); diff = $null
        adeDiff = $null; proposal = $null; approval = $null; candidateSourceSha256 = $null; error = $null }
    $saved = @{}
    try {
        Assert-FileHash $Reference.context.path $Reference.context.sha256
        Assert-FileHash $Reference.proposal.path $Reference.proposal.sha256
        $trial = Read-JsonFile $Reference.context.path
        $run = Read-JsonFile $Reference.proposal.path
        if ($trial.recommendationId -cne $Reference.id -or $Outcome.recommendationId -cne $Reference.id -or
            $run.runId -cnotmatch '^[A-Za-z0-9][A-Za-z0-9_.-]{0,100}$' -or
            $run.proposalSha256 -cne $Reference.proposalSha256 -or $Outcome.proposalSha256 -cne $Reference.proposalSha256 -or
            [IO.Path]::GetFullPath($trial.directory) -ne [IO.Path]::GetFullPath($Reference.directory)) {
            throw 'Local fix identity differs from its trial, recommendation, or exact proposal.'
        }
        $runDirectory = Join-Path $trial.directory "ade-runs\$($run.runId)"
        if ($run.runDir -and [IO.Path]::GetFullPath($run.runDir) -ne [IO.Path]::GetFullPath($runDirectory)) { throw 'Local ADE run directory mismatch.' }
        $proposalPath = Join-Path $runDirectory 'proposal.json'
        Assert-FileHash $proposalPath $Reference.proposalSha256
        $fix.proposal = @{path=$proposalPath;sha256=$Reference.proposalSha256}
        $adeDiff = Join-Path $runDirectory 'diff.patch'
        if (Test-Path -LiteralPath $adeDiff -PathType Leaf) { $fix.adeDiff = @{path=$adeDiff;sha256=Get-Sha256 $adeDiff} }
        $approvalPath = Join-Path $runDirectory 'approval.json'
        $approval = Read-JsonFile $approvalPath
        if ($approval.proposalSha256 -cne $Reference.proposalSha256) { throw 'No exact-hash approval for this local patch.' }
        $fix.approval = @{path=$approvalPath;sha256=Get-Sha256 $approvalPath}
        $proposal = Read-JsonFile $proposalPath
        if (!$proposal.workCopy) { throw 'ADE proposal does not identify a local workcopy.' }
        $native = $trial.schema -eq 'ui-profiler/native-trial/v1'
        if (!$native -and $trial.schema -ne 'ui-profiler/trial/v1') { throw 'Unsupported local fix trial schema.' }
        $level = if ($native) { $trial.native.level } else { 'application' }
        $directory = Join-Path $trial.directory 'local-fix-evidence'
        $inputs = @($proposal.workCopy, $trial.originalRepository)
        if ($native) { $inputs += $trial.native.originalRepository }
        Assert-RunDirectory $directory $inputs
        # Shared scope/source helpers spawn Git themselves; preserve the launcher's
        # configuration and append only a temporary long-path/read-only override.
        $countText = [Environment]::GetEnvironmentVariable('GIT_CONFIG_COUNT', 'Process')
        $count = 0
        if ($countText -and (![int]::TryParse($countText, [ref]$count) -or $count -lt 0 -or $count -gt 10000)) { throw 'Invalid inherited Git command configuration.' }
        $overrides = @{ GIT_CONFIG_COUNT=[string]($count+1); "GIT_CONFIG_KEY_$count"='core.longpaths'; "GIT_CONFIG_VALUE_$count"='true'
            GIT_OPTIONAL_LOCKS='0'; GIT_NO_LAZY_FETCH='1'; GIT_TERMINAL_PROMPT='0' }
        foreach ($name in $overrides.Keys) {
            $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
            [Environment]::SetEnvironmentVariable($name, $overrides[$name], 'Process')
        }
        Assert-ScopedCandidate $proposal.workCopy $trial.allowedPaths -Level $level
        $sourceHash = Get-SourceHash $proposal.workCopy
        $changes = @((Invoke-LocalTrialGit $proposal.workCopy @('diff', '--name-status', '--no-renames',
            '--no-ext-diff', '--no-textconv', 'ade-baseline', '--')).text -split "`n" | Where-Object { $_ })
        $allowed = @($trial.allowedPaths | ForEach-Object { $_.Replace('\', '/') })
        $files = @(
            foreach ($change in $changes) {
                if ($change -cnotmatch '^M\t(.+)$' -or $Matches[1] -cnotin $allowed) { throw 'Current local patch differs from approved existing-file scope.' }
                $relative = $Matches[1]
                $root = [IO.Path]::GetFullPath($proposal.workCopy).TrimEnd('\') + '\'
                $path = [IO.Path]::GetFullPath((Join-Path $root $relative.Replace('/', '\')))
                if (!$path.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) { throw 'Local patch path escapes the ADE workcopy.' }
                $ancestor = $path
                while ($ancestor) {
                    if ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Local patch traverses a filesystem link.' }
                    $ancestor = Split-Path $ancestor -Parent
                }
                $baselineBytes = (Invoke-LocalTrialGit $proposal.workCopy @('cat-file', 'blob', "ade-baseline:$relative")).bytes
                @{path=$relative;baselineSha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([byte[]]$baselineBytes)).ToLowerInvariant()
                    candidateSha256=Get-Sha256 $path}
            }
        )
        if (!$files.Count) { throw 'No actual changed scoped source files; recommendation prose is not an applied patch.' }
        $diff = Invoke-LocalTrialGit $proposal.workCopy @('diff', '--binary', '--full-index', '--no-ext-diff', '--no-textconv', 'ade-baseline', '--')
        if (!$diff.bytes.Length -or (Get-SourceHash $proposal.workCopy) -cne $sourceHash) { throw 'Candidate changed during local evidence inspection or has no retained diff.' }
        $sha256 = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([byte[]]$diff.bytes)).ToLowerInvariant()
        $path = Join-Path $directory "diff-$sha256.patch"
        if (Test-Path -LiteralPath $path) { Assert-FileHash $path $sha256 }
        else {
            [void](New-Item -ItemType Directory -Force $directory)
            [IO.File]::WriteAllBytes($path, [byte[]]$diff.bytes)
        }
        $fix.diff = @{path=$path;sha256=$sha256}
        $fix.changedFiles = $files
        $fix.scopeVerified = $true
        $fix.actuallyApplied = $true
        $fix.disposition = 'applied-local-unverified'
        $fix.candidateSourceSha256 = $sourceHash
        if ($Comparison -and $Outcome.state -eq 'verified-comparison' -and
            $Outcome.chainVerified -is [bool] -and $Outcome.chainVerified -and
            $Outcome.adeChecksPassed -is [bool] -and $Outcome.adeChecksPassed -and
            $Outcome.originalRepositoryUnchanged -is [bool] -and $Outcome.originalRepositoryUnchanged) {
            Assert-FileHash $Comparison.candidate.path $Comparison.candidate.sha256
            $candidate = Read-JsonFile $Comparison.candidate.path
            $measuredHash = if ($native) { $candidate.nativeRuntime.sourceSha256 } else { $candidate.sourceSha256 }
            $fix.measuredCandidateBound = $sourceHash -ceq $measuredHash
        }
        $fix.disposition = if ($fix.measuredCandidateBound) { 'applied-local-measured' } else { 'applied-local-unverified' }
        if (!$fix.measuredCandidateBound) { $fix.error = 'Local scoped patch exists, but it is not bound to a verified measured candidate; no validated fix or improvement is claimed.' }
    } catch { $fix.error = $_.Exception.Message }
    finally {
        foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    }
    $fix
}

function Get-LocalTrialActionItem($Entry, [string]$State, $Fix, [string]$Performance) {
    $status = 'planned'
    $action = 'Review the proposed existing-file scope and validation, then prepare and explicitly approve an exact-hash trial if warranted.'
    if ($State -eq 'insufficient-evidence') { $status='blocked-no-data'; $action='Collect the specifically missing MUX profiler evidence before proposing a source fix.' }
    elseif ($State -eq 'failed') { $status='failed'; $action='Investigate the retained failure and local patch evidence; do not treat the proposal or partial work as a validated fix.' }
    elseif ($State -eq 'evidence-unavailable' -or $Fix.disposition -eq 'unavailable') {
        $status='blocked-unverified'; $action='Restore or inspect the hash-bound local ADE artifacts; actual patch application or measured provenance could not be verified.'
    } elseif ($Fix.measuredCandidateBound) {
        if ($Performance -eq 'observed-primary-improvement') { $status='locally-verified'; $action='Review all regressions, missing capabilities and the one-pair limitation. Any remote task or draft PR requires a separate explicit opt-in.' }
        else { $status='follow-up-no-improvement'; $action='Investigate the unchanged, regressed or unavailable primary measurements; keep the local patch and evidence, without claiming an optimization success.' }
    } elseif ($Fix.actuallyApplied -eq $true) { $status='needs-validation'; $action='Inspect the actual local patch, then obtain a verified source-bound comparison before claiming a fix.' }
    @{id="local-$($Entry.id)";recommendationId=$Entry.id;level=$Entry.level;title=$Entry.title;status=$status
        action=$action;fixDisposition=$Fix.disposition;performanceDisposition=$Performance
        validation=@($Entry.validation);localOnly=$true;remoteWorkItem=$null;remoteCreationRequiresExplicitOptIn=$true}
}

function Write-RecommendationTrialReport($Context) {
    Assert-FileHash $Context.recommendations.path $Context.recommendations.sha256
    $recommendations = Read-JsonFile $Context.recommendations.path
    $rows = foreach ($entry in $recommendations.recommendations) {
        $trial = @($Context.trials | Where-Object id -CEQ $entry.id)
        if ($trial.Count -gt 1) { throw 'Duplicate independent trial references.' }
        $state = if ($entry.verdict -eq 'insufficient-evidence') { 'insufficient-evidence' } else { 'not-prepared' }
        $outcome = $null
        $comparison = $null
        $evidenceError = $null
        $outcomeReference = $null
        if ($trial.Count) {
            $trial = $trial[0]
            try {
            Assert-FileHash $trial.context.path $trial.context.sha256
            Assert-FileHash $trial.proposal.path $trial.proposal.sha256
            $state = 'awaiting-approval'
            $outcomePath = Join-Path $trial.directory 'outcome.json'
            if (Test-Path -LiteralPath $outcomePath) {
                $outcome = Read-JsonFile $outcomePath
                $outcomeReference = @{path=$outcomePath;sha256=Get-Sha256 $outcomePath}
                if ($outcome.recommendationId -cne $entry.id -or $outcome.proposalSha256 -cne $trial.proposalSha256 -or
                    $outcome.state -notin @('failed', 'verified-comparison')) { throw 'Unbound independent trial outcome.' }
                if ($outcome.state -eq 'verified-comparison') {
                    if ($outcome.chainVerified -isnot [bool] -or !$outcome.chainVerified -or
                        $outcome.adeChecksPassed -isnot [bool] -or !$outcome.adeChecksPassed -or
                        $outcome.originalRepositoryUnchanged -isnot [bool] -or !$outcome.originalRepositoryUnchanged) {
                        throw 'Unverified trial cannot report a comparison.'
                    }
                    Assert-FileHash $outcome.comparison.path $outcome.comparison.sha256
                    $comparison = Read-JsonFile $outcome.comparison.path
                    if ($comparison.schema -ne 'ui-profiler/comparison/v2') { throw 'Unsupported measured comparison; no local measured fix is inferred.' }
                    foreach ($side in @('baseline', 'candidate')) {
                        Assert-FileHash $comparison.$side.path $comparison.$side.sha256
                        Assert-Measurement (Read-JsonFile $comparison.$side.path)
                    }
                }
                $state = $outcome.state
            }
            } catch {
                $evidenceError = $_.Exception.Message
                $state = 'evidence-unavailable'
                $comparison = $null
            }
        }
        $fix = @{disposition='planned';actuallyApplied=$false;measuredCandidateBound=$false;changedFiles=@()
            diff=$null;adeDiff=$null;error=$null;location='No verified local patch yet'}
        if ($state -eq 'insufficient-evidence') { $fix.disposition='not-planned-no-data' }
        if ($evidenceError) {
            $fix.disposition='unavailable'; $fix.actuallyApplied=$null; $fix.error=$evidenceError
        } elseif ($outcome) {
            $fix = Get-LocalTrialFixEvidence $trial $outcome $comparison
            # Run retains this independent of publication. A later report may use that
            # immutable diff/file-hash evidence even if the disposable workcopy is gone.
            if ($fix.disposition -eq 'unavailable' -and $outcome.PSObject.Properties['localFix'] -and $outcome.localFix) {
                try {
                    $retained = $outcome.localFix
                    if ($retained.schema -ne 'ui-profiler/local-fix/v1' -or
                        $retained.recommendationId -cne $entry.id -or $retained.trialContextSha256 -cne $trial.context.sha256 -or
                        $retained.proposalSha256 -cne $trial.proposalSha256 -or
                        $retained.actuallyApplied -isnot [bool] -or !$retained.actuallyApplied -or
                        $retained.scopeVerified -isnot [bool] -or !$retained.scopeVerified -or
                        !@($retained.changedFiles).Count -or !$retained.diff) {
                        throw 'Retained local fix metadata is not bound to this scoped ADE trial.'
                    }
                    $trialContext = Read-JsonFile $trial.context.path
                    $allowed = @($trialContext.allowedPaths | ForEach-Object { $_.Replace('\', '/') })
                    foreach ($file in $retained.changedFiles) {
                        if ($file.path -cnotin $allowed -or $file.baselineSha256 -cnotmatch '^[a-f0-9]{64}$' -or
                            $file.candidateSha256 -cnotmatch '^[a-f0-9]{64}$') { throw 'Retained changed-file evidence escapes approved scope or lacks byte hashes.' }
                    }
                    foreach ($reference in @($retained.diff, $retained.proposal, $retained.approval)) {
                        Assert-FileHash $reference.path $reference.sha256
                    }
                    if ($retained.adeDiff) { Assert-FileHash $retained.adeDiff.path $retained.adeDiff.sha256 }
                    $retained.measuredCandidateBound = $false
                    if ($comparison) {
                        $candidate = Read-JsonFile $comparison.candidate.path
                        $measuredHash = if ($trialContext.schema -eq 'ui-profiler/native-trial/v1') {
                            $candidate.nativeRuntime.sourceSha256
                        } else { $candidate.sourceSha256 }
                        $retained.measuredCandidateBound = $retained.candidateSourceSha256 -ceq $measuredHash
                    }
                    $retained.disposition = if ($retained.measuredCandidateBound) { 'applied-local-measured' } else { 'applied-local-unverified' }
                    if (!$retained.measuredCandidateBound) { $retained.error='Retained scoped local patch is not bound to a verified measured candidate; no validated fix is claimed.' }
                    $fix = $retained
                } catch { $fix.error = "Retained local patch evidence unavailable: $($_.Exception.Message)" }
            }
        }
        $performance = 'not-measured'
        if ($comparison) {
            $improved = @($comparison.metrics | Where-Object {
                $_.primary -eq $true -and $_.status -eq 'available' -and $_.change -eq 'observed-improvement' -and
                $null -ne $_.baseline -and $null -ne $_.candidate -and $null -ne $_.delta -and
                [double]$_.candidate -lt [double]$_.baseline -and [double]$_.delta -lt 0
            })
            $performance = if ($improved.Count) { 'observed-primary-improvement' } else { 'no-demonstrated-primary-improvement' }
        }
        $task = Get-LocalTrialActionItem $entry $state $fix $performance
        @{ id = $entry.id; level = $entry.level; title = $entry.title; state = $state
            summary = $entry.summary; validation = $entry.validation; files = $entry.files; trial = $trial; outcome = $outcome
            outcomeReference=$outcomeReference;fix=$fix;actionItem=$task;performanceDisposition=$performance
            comparisonAvailable=($null -ne $comparison);evidenceError=$evidenceError }
    }
    $report = @{ schema = 'ui-profiler/recommendation-trial-report/v1'; coverage = $recommendations.coverage
        runtimeSourceParity = $recommendations.runtimeSourceParity; rows = @($rows);actionItems=@($rows | ForEach-Object actionItem)
        remoteActions=@{azureWorkItems='not-created';draftPullRequests='explicit-opt-in-only';localReport='independent-of-remote-opt-in'}
        interpretation = 'Independent variants; app trials use discovery baseline, native trials use explicitly approved source-built baseline/candidate pairs. No combined-patch or speedup claim.' }
    if ($recommendations.PSObject.Properties['detailedAnalysis']) { $report.detailedAnalysis = $recommendations.detailedAnalysis }
    $path = Join-Path $Context.directory 'recommendations-report.json'
    Write-JsonFile $path $report
    $markdown = "# Independent recommendation trials`n`n$($report.interpretation)`n`n"
    $markdown += "## Local actionable tasks (no remote work items created)`n`n" +
        "Local reporting and patch evidence do not require publication. Azure tasks are not created; draft PR creation is explicit opt-in only.`n`n" +
        "| Recommendation / level | Task status | Fix disposition | Performance evidence | Next action |`n" +
        "|---|---|---|---|---|`n"
    foreach ($task in $report.actionItems) {
        $markdown += "| $($task.recommendationId) / $($task.level) | $($task.status) | $($task.fixDisposition) | " +
            "$($task.performanceDisposition) | $($task.action.Replace('|', '\|')) |`n"
    }
    $markdown += "`n"
    if ($report.ContainsKey('detailedAnalysis')) {
        foreach ($level in @('application', 'control', 'framework')) {
            $layer = $report.detailedAnalysis.$level
            $markdown += "## $level investigation`n`nQuestion: $($layer.question)`n`n"
            foreach ($section in @('time', 'cpu', 'memory', 'modelAnalysis', 'unverified', 'validation')) {
                $markdown += "### $section`n`n"
                foreach ($item in $layer.$section) { $markdown += "- $item`n" }
            }
        }
    }
    foreach ($row in $rows) {
        $markdown += "## $($row.level): $($row.title)`n`nStatus: $($row.state).`n`n$($row.summary)`n`n"
        $markdown += "### Actual fix disposition and local evidence`n`n" +
            "Disposition: $($row.fix.disposition). Location: $($row.fix.location). " +
            "Bound to measured candidate: $($row.fix.measuredCandidateBound). Performance: $($row.performanceDisposition).`n`n" +
            "Next local action: $($row.actionItem.action)`n`n"
        if ($row.fix.error) { $markdown += "Evidence limitation: $($row.fix.error)`n`n" }
        if ($row.fix.diff) { $markdown += "Retained current scoped diff: $($row.fix.diff.path); SHA-256 $($row.fix.diff.sha256).`n`n" }
        if ($row.fix.adeDiff) { $markdown += "ADE-owned diff (not overwritten or assumed byte-identical to current diff): $($row.fix.adeDiff.path); SHA-256 $($row.fix.adeDiff.sha256).`n`n" }
        foreach ($file in $row.fix.changedFiles) {
            $markdown += "- Actual changed file: $($file.path). Baseline SHA-256 $($file.baselineSha256); candidate SHA-256 $($file.candidateSha256).`n"
        }
        $markdown += "`n"
        if ($row.trial) { $markdown += "ADE run: $($row.trial.runId). Exact proposal SHA-256: $($row.trial.proposalSha256).`n`n" }
        if ($row.outcome) {
            $markdown += if ($row.comparisonAvailable) {
                "Comparison: $($row.outcome.comparison.path). No improvement assumed.`n`n"
            } else { "Failure: $($row.outcome.error)`n`n" }
            if ($row.comparisonAvailable) {
                $measured = Read-JsonFile $row.outcome.comparison.path
                $markdown += "### Observed primary metrics and regressions`n`n"
                $markdown += "| Family / scope / metric | Unit | Baseline | Candidate | Delta | Reduction % | Status / change |`n"
                $markdown += "|---|---|---:|---:|---:|---:|---|`n"
                foreach ($metric in @($measured.metrics | Where-Object { $_.primary -or $_.change -eq 'regression' })) {
                    $values = @($metric.baseline, $metric.candidate, $metric.delta, $metric.reductionPercent) |
                        ForEach-Object { if ($null -eq $_) { 'unavailable' } else { $_ } }
                    $markdown += "| $($metric.family) / $($metric.scope) / $($metric.metric) | $($metric.unit) | " +
                        "$($values[0]) | $($values[1]) | $($values[2]) | $($values[3]) | $($metric.status) / $($metric.change) |`n"
                }
                $markdown += "`nEvery evidence family, unavailable dimension and non-primary row remains in " +
                    "$(Join-Path (Split-Path $row.outcome.comparison.path -Parent) 'comparison.md'). " +
                    "One pair is observational, not statistical significance.`n`n"
            }
            if ($row.comparisonAvailable -and $row.outcome.PSObject.Properties['controlComparison'] -and $row.outcome.controlComparison) {
                try {
                Assert-FileHash $row.outcome.controlComparison.path $row.outcome.controlComparison.sha256
                $control = Read-JsonFile $row.outcome.controlComparison.path
                if ($control.schema -ne 'ui-profiler/control-comparison/v1') { throw 'Unsupported control comparison.' }
                foreach ($side in @('baseline', 'candidate')) {
                    Assert-FileHash $control.$side.path $control.$side.sha256
                }
                $markdown += "Control-scoped before/after evidence: $($row.outcome.controlComparison.path).`n`n"
                $markdown += "| Observed control metric | Unit | Baseline | Candidate | Delta | Reduction % |`n"
                $markdown += "|---|---|---:|---:|---:|---:|`n"
                foreach ($metric in $control.metrics) {
                    $values = @($metric.baseline, $metric.candidate, $metric.delta, $metric.reductionPercent) |
                        ForEach-Object { if ($null -eq $_) { 'unavailable' } else { $_ } }
                    $markdown += "| $($metric.metric) | $($metric.unit) | $($values[0]) | $($values[1]) | $($values[2]) | $($values[3]) |`n"
                }
                $markdown += "`n$($control.interpretation)`n`n"
                } catch { $markdown += "Control-scoped evidence unavailable: $($_.Exception.Message)`n`n" }
            }
            if ($row.outcome.PSObject.Properties['validation']) {
                foreach ($check in $row.outcome.validation) {
                    $markdown += "Validation command: ``$($check.command)``. Result: $($check.result).`n`n"
                }
            }
            if ($row.outcome.PSObject.Properties['draftPr'] -and $row.outcome.draftPr) {
                $markdown += "Draft PR: $($row.outcome.draftPr.url)`n`n"
            }
            if ($row.outcome.PSObject.Properties['publicationError'] -and $row.outcome.publicationError) {
                $markdown += "Draft PR publication blocked: $($row.outcome.publicationError)`n`n"
            }
        }
    }
    [IO.File]::WriteAllText((Join-Path $Context.directory 'recommendations-report.md'), $markdown)
    @{ path = $path; sha256 = Get-Sha256 $path }
}

function Invoke-RecommendationTrials([ValidateSet('Run', 'Verify', 'Report')][string]$Phase,
    [string]$Directory, [string[]]$RecommendationId = @(), [string]$GitHubAccount,
    [switch]$PublishDraftPullRequests) {
    $contextPath = Join-Path $Directory 'context.json'
    $context = Read-JsonFile $contextPath
    if ($context.schema -ne 'ui-profiler/recommendation-trials/v1' -or
        [IO.Path]::GetFullPath($context.directory) -ne [IO.Path]::GetFullPath($Directory)) { throw 'Mismatched trial-set context.' }
    foreach ($reference in @($context.module, $context.sourceInvestigation, $context.contracts, $context.recommendations,
        $context.implementation.harness, $context.implementation.runtime)) {
        Assert-FileHash $reference.path $reference.sha256
    }
    if ((Get-SourceHash $context.packSnapshot.directory $context.packSnapshot.files) -ne $context.packSnapshot.sha256) {
        throw 'The frozen independent-trial tool/knowledge snapshot changed.'
    }
    Assert-OriginalSource $context
    if (@($RecommendationId | Select-Object -Unique).Count -ne $RecommendationId.Count) { throw 'Duplicate trial selection.' }
    if ($Phase -eq 'Run' -and !$RecommendationId.Count) { throw 'Explicitly select approved independent trial ids.' }
    $selected = if ($RecommendationId.Count) {
        foreach ($id in $RecommendationId) {
            $matches = @($context.trials | Where-Object id -CEQ $id)
            if ($matches.Count -ne 1) { throw "No prepared trial: $id" }
            $matches[0]
        }
    } else { @($context.trials) }
    $lock = $null
    $lockPath = Join-Path $Directory 'operation.lock'
    try {
        try { $lock = [IO.File]::Open($lockPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None) }
        catch [IO.IOException] { throw 'Another trial operation is active or interrupted; no automatic reset.' }
        if ($context.report) { Assert-FileHash $context.report.path $context.report.sha256 }
        $context | Add-Member -NotePropertyName report -NotePropertyValue (Write-RecommendationTrialReport $context) -Force
        Write-JsonFile $contextPath $context
        foreach ($reference in $selected) {
            if ($Phase -eq 'Report') { continue }
            $expectedDirectory = Join-Path (Join-Path $Directory 'trials') $reference.id
            if ($reference.id -cnotmatch '^[a-z0-9][a-z0-9-]{0,79}$' -or
                [IO.Path]::GetFullPath($reference.directory) -ne [IO.Path]::GetFullPath($expectedDirectory)) {
                throw 'Trial reference escapes the independent set.'
            }
            Assert-FileHash $reference.context.path $reference.context.sha256
            Assert-FileHash $reference.proposal.path $reference.proposal.sha256
            $trial = Read-JsonFile $reference.context.path
            $proposal = Read-JsonFile $reference.proposal.path
            if ($reference.context.path -ne (Join-Path $expectedDirectory 'pack\knowledge\trial-context.json') -or
                $reference.proposal.path -ne (Join-Path $expectedDirectory 'ade-run.json') -or
                $reference.runId -match '[/\\:]' -or $reference.runId -in @('', '.', '..') -or
                $proposal.status -ne 'AwaitingApproval' -or $proposal.runId -cne $reference.runId -or
                $proposal.proposalSha256 -cne $reference.proposalSha256 -or
                $trial.recommendationId -cne $reference.id -or $trial.directory -ne $reference.directory -or
                $trial.originalRepository -ne $context.originalRepository -or
                $trial.originalSourceSha256 -ne $context.originalSourceSha256 -or
                $trial.recommendations.path -ne $context.recommendations.path -or
                $trial.recommendations.sha256 -ne $context.recommendations.sha256) {
                throw 'Swapped trial context.'
            }
            if ($trial.schema -eq 'ui-profiler/native-trial/v1') { Assert-NativeTrialContext $trial }
            elseif ($trial.schema -eq 'ui-profiler/trial/v1') { Assert-TrialContext $trial }
            else { throw 'Unsupported independent trial.' }
            if ($GitHubAccount) { $trial.githubAccount = $GitHubAccount }
            $runs = Join-Path $reference.directory 'ade-runs'
            if ($Phase -eq 'Verify') {
                [void](Invoke-AdeRuntime @('verify', '--run', $reference.runId, '--runs-dir', $runs) $trial)
                continue
            }
            $outcomePath = Join-Path $reference.directory 'outcome.json'
            if (Test-Path -LiteralPath $outcomePath) { throw 'Trial outcome already exists; no repeated coding/capture.' }
            $approvalPath = Join-Path (Join-Path $runs $reference.runId) 'approval.json'
            Assert-FileHash (Join-Path (Join-Path $runs $reference.runId) 'proposal.json') $reference.proposalSha256
            if (!(Test-Path -LiteralPath $approvalPath) -or
                (Read-JsonFile $approvalPath).proposalSha256 -cne $reference.proposalSha256) {
                throw "Exact-hash approval missing for $($reference.id); coding and capture remain unspent."
            }
            $outcome = @{ schema = 'ui-profiler/independent-trial-outcome/v1'; recommendationId = $reference.id
                proposalSha256 = $reference.proposalSha256; state = 'failed'; error = $null
                adeChecksPassed = $false; chainVerified = $false; originalRepositoryUnchanged = $false
                comparison = $null; controlComparison = $null; validation = @(); draftPr = $null; publicationError = $null }
            try {
                $result = Invoke-AdeRuntime @('run', '--run', $reference.runId, '--runs-dir', $runs) $trial
                if ($result.stopReason -ne 'Passed') { throw 'ADE required checks did not pass.' }
                $outcome.adeChecksPassed = $true
                $config = Read-JsonFile (Join-Path $reference.directory 'pack\ade.team.json')
                $outcome.validation = @($config.checks | ForEach-Object {
                    @{ command = $_.command; result = 'passed (required ADE check; run returned Passed)' }
                })
                [void](Invoke-AdeRuntime @('verify', '--run', $reference.runId, '--runs-dir', $runs) $trial)
                $outcome.chainVerified = $true
                $comparisonPath = Join-Path $reference.directory 'multidimensional\comparison.json'
                if (!(Test-Path -LiteralPath $comparisonPath)) { throw 'No changed and measured candidate; baseline-only checks are not a completed trial.' }
                Assert-OriginalSource $trial
                if ($trial.schema -eq 'ui-profiler/native-trial/v1') { Assert-NativeTrialContext $trial }
                $outcome.originalRepositoryUnchanged = $true
                $outcome.comparison = @{ path = $comparisonPath; sha256 = Get-Sha256 $comparisonPath }
                $controlPath = Join-Path $reference.directory 'control-comparison.json'
                if (Test-Path -LiteralPath $controlPath) {
                    $outcome.controlComparison = @{ path = $controlPath; sha256 = Get-Sha256 $controlPath }
                }
                $outcome.state = 'verified-comparison'
            } catch { $outcome.error = $_.Exception.Message }
            $localComparison = if ($outcome.comparison) { Read-JsonFile $outcome.comparison.path } else { $null }
            $outcome.localFix = Get-LocalTrialFixEvidence $reference $outcome $localComparison
            Write-JsonFile $outcomePath $outcome
            $context | Add-Member -NotePropertyName report -NotePropertyValue (Write-RecommendationTrialReport $context) -Force
            Write-JsonFile $contextPath $context
            if ($PublishDraftPullRequests -and $outcome.state -eq 'verified-comparison') {
                try {
                    $published = & "$PSScriptRoot\Publish-ProfiledDraftPr.ps1" -TrialContextPath $reference.context.path `
                        -TrialContextSha256 $reference.context.sha256 -OutcomePath $outcomePath -GitHubAccount $trial.githubAccount
                    $outcome.draftPr = $published | ConvertFrom-Json
                } catch { $outcome.publicationError = $_.Exception.Message }
                Write-JsonFile $outcomePath $outcome
            }
        }
        $context | Add-Member -NotePropertyName report -NotePropertyValue (Write-RecommendationTrialReport $context) -Force
        Write-JsonFile $contextPath $context
        $report = Read-JsonFile $context.report.path
        $report | ConvertTo-Json -Depth 60
        if (@($report.rows | Where-Object state -eq 'failed').Count -and $Phase -eq 'Run') {
            throw 'Independent trials include failures; retained evidence does not establish success.'
        }
        if ($PublishDraftPullRequests -and @($report.rows | Where-Object {
            $_.outcome -and $_.outcome.publicationError
        }).Count) { throw 'Measured outcomes were retained, but draft PR publication was blocked; inspect the report.' }
    } finally {
        if ($lock) {
            try {
                $context | Add-Member -NotePropertyName report -NotePropertyValue (Write-RecommendationTrialReport $context) -Force
                Write-JsonFile $contextPath $context
            } catch { Write-Warning 'Local report refresh was incomplete; the initial local report and retained ADE artifacts remain available.' }
            finally { $lock.Dispose(); Remove-Item -LiteralPath $lockPath }
        }
    }
}

Export-ModuleMember -Function Assert-NativeTrialContext, Invoke-NativeTrialCheck, Compare-NativeMeasurements,
    Write-RecommendationTrialReport, Invoke-RecommendationTrials

<#
.SYNOPSIS
    Validate a GitHub fork PR by publishing its exact head commit to a validation branch in this
    repository, where the pipeline runs with credentials.

.DESCRIPTION
    Fork PRs receive no pipeline secrets, so WinUI-GitHub-PR (OneBranch) fails at "Install Pipeline
    Tools" with a 401. This script pushes the fork PR's IDENTICAL head commit to
    refs/heads/validation/fork-pr/<n>/..., where the build runs normally. Azure DevOps reports a
    commit status named after the pipeline; because the commit object is shared, that status lands
    on the fork PR and satisfies its required check.

    Two ways to start the build once the branch is pushed:

      CI trigger    The push itself starts the build. Needs no Azure DevOps credential, so this is
                    the path used by .github/workflows/fork-pr-validation.yml.
      -QueueBuild   Queue explicitly via the REST API using the caller's own 'az login'. Convenient
                    by hand; in automation it requires a service principal.

    Use -CheckTrigger to see which is available. With neither, the push is silent — no build, no
    status, and the PR stays blocked — so -Publish fails instead of reporting success.

    SECURITY: publishing runs the fork's code, scripts, MSBuild targets and pipeline YAML in a
    CREDENTIALED pipeline. Only publish a commit after reviewing that exact SHA. -Publish requires
    -IReviewedTheSha; changes to build/pipeline files additionally require
    -IReviewedTheBuildSurface.

.PARAMETER PrNumber     Fork PR number (required).
.PARAMETER Repo         owner/name. Default microsoft/microsoft-ui-xaml.
.PARAMETER Remote       Local git remote pointing at upstream. Default origin.
.PARAMETER Publish      Push the fork's exact head commit to the validation branch.
.PARAMETER Status       Show the current validation result for the fork PR head SHA.
.PARAMETER Cleanup      Delete every validation branch for this fork PR.
.PARAMETER Invalidate   Tear down a stale validation after a new fork commit and prompt for re-run.
.PARAMETER NewSha       New fork head SHA, referenced in the -Invalidate prompt (optional).
.PARAMETER IReviewedTheSha          Required acknowledgement for -Publish.
.PARAMETER IReviewedTheBuildSurface Required acknowledgement when the PR changes build inputs.
.PARAMETER ReuseBranch  Reuse one stable branch per fork PR, force-updated to each new SHA.
.PARAMETER Wait         Poll until the validation result leaves the pending state.
.PARAMETER QueueBuild   After pushing, queue the build via REST (needs az login or a service principal).
.PARAMETER CheckTrigger Report whether the pipeline has a CI trigger covering validation/*.

.EXAMPLE
    # Publish the reviewed head, queue the build, and block until the check resolves.
    .\ValidateForkPRBranch.ps1 -PrNumber 12345 -Publish -IReviewedTheSha -QueueBuild -Wait
.EXAMPLE
    # Publish only; the CI trigger on validation/* starts the build.
    .\ValidateForkPRBranch.ps1 -PrNumber 12345 -Publish -IReviewedTheSha -ReuseBranch
.EXAMPLE
    .\ValidateForkPRBranch.ps1 -PrNumber 12345 -Status -Wait
.EXAMPLE
    .\ValidateForkPRBranch.ps1 -PrNumber 12345 -Cleanup
.EXAMPLE
    # Report which start mechanism is configured.
    .\ValidateForkPRBranch.ps1 -PrNumber 0 -CheckTrigger
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][int]$PrNumber,
    [string]$Repo   = 'microsoft/microsoft-ui-xaml',
    [string]$Remote = 'origin',
    [switch]$Publish,
    [switch]$Status,
    [switch]$Cleanup,
    # Never auto-publishes the new commit: every new SHA needs a fresh review.
    [switch]$Invalidate,
    [string]$NewSha,
    [switch]$IReviewedTheSha,
    [switch]$IReviewedTheBuildSurface,
    # One stable branch per fork PR instead of a fresh SHA-suffixed branch on every push.
    [switch]$ReuseBranch,
    [switch]$Wait,
    [int]$TimeoutMinutes = 90,
    # After -Publish, wait this long for the build to START and fail if it never does; publishing a
    # branch that nothing builds is a silent no-op. Set 0 to skip.
    [int]$WaitForStartMinutes = 8,
    [switch]$CheckTrigger,
    [switch]$QueueBuild,
    # ADO coordinates, used by -CheckTrigger and -QueueBuild.
    [string]$AdoOrgUrl        = 'https://dev.azure.com/microsoft',
    [string]$AdoProject       = 'WinUI',
    [int]$AdoDefinitionId     = 195405
)

$ErrorActionPreference = 'Stop'

# The required check on protected branches. Azure DevOps uses the pipeline's name as the status
# context, so a build of this definition reports under exactly this string.
$Context = 'WinUI-GitHub-PR (OneBranch)'

# All validation refs for a PR live under this directory so per-SHA branches and the stable
# -ReuseBranch head never collide in git's ref store. The trailing slash matters: without it,
# prefix query for PR 1166 would also match 11660, 11661, ...
function Get-BranchPrefix { "validation/fork-pr/$PrNumber/" }

# Paths that influence HOW the build runs rather than what it produces. Azure DevOps compiles the
# pipeline from the candidate commit, so a change here outweighs any amount of product code. These
# are surfaced separately at publish time so they cannot be lost in a large diff.
#
# Directory prefixes: everything beneath them is build orchestration.
$script:BuildSurfaceDirs = @(
    'build/'
    'eng/'
    '.pipelines/'
    '.github/'
    '.azuredevops/'
    '.config/'
)
# Extensions that execute during a build wherever they live: MSBuild .props/.targets can run
# arbitrary tasks, so a nested one is as dangerous as a root one. Only touched files are reported.
$script:BuildSurfaceExts = @('.yml', '.yaml', '.props', '.targets', '.proj')
# Exact root-level filenames that steer restore/build tooling.
$script:BuildSurfaceFiles = @(
    'nuget.config', 'global.json', 'directory.build.props', 'directory.build.targets',
    'directory.packages.props', 'directory.build.rsp', 'dotnet-tools.json'
)

function Test-IsBuildSurface([string]$Path) {
    $p = $Path.Replace('\', '/')
    foreach ($d in $script:BuildSurfaceDirs) { if ($p.ToLowerInvariant().StartsWith($d)) { return $true } }
    foreach ($e in $script:BuildSurfaceExts) { if ($p.ToLowerInvariant().EndsWith($e)) { return $true } }
    $leaf = ($p -split '/')[-1]
    if ($script:BuildSurfaceFiles -contains $leaf.ToLowerInvariant()) { return $true }
    return $false
}

function Get-BuildSurfaceChanges {
    # Uses the PR's own file list, so it is already scoped to the merge base.
    $files = gh api "repos/$Repo/pulls/$PrNumber/files" --paginate --jq '.[].filename' 2>$null
    return @($files | Where-Object { $_ -and (Test-IsBuildSurface $_) })
}

function Assert-BuildSurfaceReviewed {
    $hits = @(Get-BuildSurfaceChanges)
    if (-not $hits) { return }

    Write-Host ""
    Write-Warning "This fork PR changes $($hits.Count) file(s) that control HOW the build runs:"
    foreach ($h in $hits) { Write-Host "    $h" }
    Write-Host ""
    Write-Host "These files are compiled and executed by a CREDENTIALED pipeline. Review them line by"
    Write-Host "line before publishing — a single line here outweighs any amount of product code."
    Write-Host ""

    # Print the patches inline so the decision is made with the change in view, not from a filename.
    # One compact JSON object per line (@json): iterating raw --jq text would split each multi-line
    # patch into separate items.
    $rows = gh api "repos/$Repo/pulls/$PrNumber/files" --paginate --jq '.[] | @json' 2>$null
    foreach ($r in @($rows | Where-Object { $_ })) {
        $o = $null
        try { $o = $r | ConvertFrom-Json } catch { continue }
        if ($hits -notcontains $o.filename) { continue }
        Write-Host "=== $($o.filename) ===" -ForegroundColor Yellow
        if ($o.patch) {
            Write-Host $o.patch
        } else {
            # Binary or oversized — more reason to look, not less.
            Write-Warning "  No inline diff available (binary or oversized). Inspect it manually:"
            Write-Host   "  $($o.blob_url)"
        }
        Write-Host ""
    }

    if (-not $IReviewedTheBuildSurface) {
        throw "Refusing to publish: pass -IReviewedTheBuildSurface to confirm you read the pipeline/build changes listed above."
    }
    Write-Warning "Proceeding: -IReviewedTheBuildSurface was supplied."
}

function Get-PrInfo {
    $json = gh pr view $PrNumber --repo $Repo --json headRefOid,baseRefName,isCrossRepository,state,url
    if ($LASTEXITCODE -ne 0) { throw "gh pr view failed for #$PrNumber" }
    $json | ConvertFrom-Json
}

# Read the validation result from BOTH reporting mechanisms. Which one Azure DevOps uses depends on
# the GitHub service connection: an OAuth grant posts commit statuses, the Azure Pipelines GitHub
# App posts check runs. Reading only one reports "none" whenever the other is in use.
function Get-ValidationState([string]$Sha) {
    $results = @()

    $statusJson = gh api "repos/$Repo/commits/$Sha/status" --jq ".statuses[] | select(.context==`"$Context`")" 2>$null
    foreach ($line in @($statusJson | Where-Object { $_ })) {
        $s = $line | ConvertFrom-Json
        $results += [pscustomobject]@{
            Kind = 'status'; State = $s.state; Description = $s.description
            Url = $s.target_url; Updated = $s.updated_at
        }
    }

    $checkJson = gh api "repos/$Repo/commits/$Sha/check-runs?per_page=100" --jq ".check_runs[] | select(.name==`"$Context`")" 2>$null
    foreach ($line in @($checkJson | Where-Object { $_ })) {
        $c = $line | ConvertFrom-Json
        # Normalise check-run vocabulary onto commit-status vocabulary: an unfinished run is
        # 'pending'; a finished one reports its conclusion.
        $state = if ($c.status -ne 'completed') { 'pending' }
                 elseif ($c.conclusion -in 'success','failure') { $c.conclusion }
                 else { $c.conclusion }
        $results += [pscustomobject]@{
            Kind = 'check_run'; State = $state; Description = $c.output.title
            Url = $c.html_url; Updated = $c.completed_at
        }
    }

    # Newest first, so the caller sees the most recent result when both mechanisms left a record.
    $results | Sort-Object { if ($_.Updated) { [datetime]$_.Updated } else { [datetime]::MinValue } } -Descending
}

function Wait-Validation([string]$Sha) {
    $deadline = (Get-Date).AddMinutes($TimeoutMinutes)
    do {
        $all   = @(Get-ValidationState $Sha)
        $cur   = $all | Select-Object -First 1
        $state = if ($cur) { $cur.State } else { 'none' }
        Write-Host ("[{0:HH:mm:ss}] {1} -> {2}  {3}" -f (Get-Date), $Context, $state, $cur.Description)
        if ($state -in 'success','failure','error','timed_out','cancelled','action_required') { return $cur }
        Start-Sleep -Seconds 30
    } while ((Get-Date) -lt $deadline)
    Write-Warning "Timed out after $TimeoutMinutes min; validation still '$state'."
    return $cur
}

function Get-AdoToken {
    $token = az account get-access-token --resource '499b84ac-1321-427f-aa17-267ca6975798' --query accessToken -o tsv
    if ($LASTEXITCODE -ne 0 -or -not $token) { throw "az account get-access-token failed; run 'az login' first." }
    return $token.Trim()
}

# Queue the pipeline directly against the validation branch. Needs no pipeline change: a queued
# build on a non-fork ref gets full credentials and still reports the status onto the built SHA.
function Invoke-QueueBuild([string]$Sha, [string]$Branch) {
    $body = @{
        definition    = @{ id = $AdoDefinitionId }
        sourceBranch  = "refs/heads/$Branch"
        sourceVersion = $Sha
    } | ConvertTo-Json -Depth 5
    $uri = "$AdoOrgUrl/$AdoProject/_apis/build/builds?api-version=7.1"
    try {
        $build = Invoke-RestMethod -Uri $uri -Method Post -Body $body -ContentType 'application/json' `
                                   -Headers @{ Authorization = "Bearer $(Get-AdoToken)" }
    } catch {
        # A queue-time rejection usually means the candidate commit's YAML failed to compile, which
        # is a real result about the fork's change, not a script failure — surface it verbatim.
        $detail = $_.ErrorDetails.Message
        throw "Queueing build failed: $(if ($detail) { $detail } else { $_.Exception.Message })"
    }
    Write-Host "Queued build $($build.id): $($build._links.web.href)"
    return $build
}

if ($CheckTrigger) {
    $uri = "$AdoOrgUrl/$AdoProject/_apis/build/definitions/$AdoDefinitionId`?api-version=7.1"
    $def = Invoke-RestMethod -Uri $uri -Headers @{ Authorization = "Bearer $(Get-AdoToken)" }
    Write-Host "Definition $AdoDefinitionId '$($def.name)' (revision $($def.revision))"
    $ci = @($def.triggers | Where-Object { $_.triggerType -eq 'continuousIntegration' })
    if (-not $ci) {
        Write-Warning "No continuousIntegration trigger. Pushing a validation branch will NOT build on its own."
        Write-Host  "Either add one in the pipeline UI (Triggers > Continuous integration > Override the YAML"
        Write-Host  "continuous integration trigger from here) with a branch filter of validation/*,"
        Write-Host  "or run -Publish with -QueueBuild, which needs no pipeline change."
        return
    }
    foreach ($t in $ci) {
        $filters = @($t.branchFilters)
        Write-Host "  continuousIntegration branchFilters: $($filters -join ', ')"
        # A UI-defined trigger has no settingsSourceType; one sourced from YAML reports 2. Only the
        # UI-defined form overrides the candidate commit's own `trigger:` block.
        if ($t.PSObject.Properties.Name -contains 'settingsSourceType' -and $t.settingsSourceType -eq 2) {
            Write-Warning "  This trigger comes from YAML, which is evaluated from the pushed branch — i.e. the fork's"
            Write-Warning "  commit. It is missing for PRs branched from an older main, and a fork can delete it."
            Write-Warning "  Define the trigger in the pipeline UI instead."
        }
        $covers = $filters | Where-Object { $_ -match 'validation' -or $_ -eq '+refs/heads/*' -or $_ -eq '*' }
        if ($covers) { Write-Host "  OK: validation/* appears covered." }
        else { Write-Warning "  Filters do not appear to cover validation/*; branch pushes will not build." }
    }
    return
}

$info = Get-PrInfo
$Sha  = $info.headRefOid
$Base = $info.baseRefName

$prefix = Get-BranchPrefix
$Branch = if ($ReuseBranch) { "${prefix}latest" } else { "$prefix$($Sha.Substring(0,8))" }

if ($Status) {
    Write-Host "Fork PR #$PrNumber  head=$Sha  base=$Base  state=$($info.state)"
    if ($Wait) { Wait-Validation $Sha; return }
    $all = @(Get-ValidationState $Sha)
    if ($all) { $all } else { Write-Host "No '$Context' status or check run on $Sha yet." }
    return
}

function Invoke-BranchCleanup {
    # Delete every validation ref for this fork PR: re-validations can leave orphaned per-SHA
    # branches behind.
    $p = Get-BranchPrefix
    $refs = gh api "repos/$Repo/git/matching-refs/heads/$p" --jq '.[].ref' 2>$null
    $refs = @($refs | Where-Object { $_ })
    if (-not $refs) { Write-Host "Nothing to clean for $p."; return 0 }

    # This flow opens no PRs, but warn rather than delete silently if anything else left one on
    # these refs: deleting the branch would close it.
    $openPrs = gh pr list --repo $Repo --search "head:validation/fork-pr/$PrNumber" --state open `
                 --json number,headRefName 2>$null | ConvertFrom-Json
    foreach ($pr in @($openPrs | Where-Object { $_.headRefName -like "$p*" })) {
        Write-Warning "Branch $($pr.headRefName) has open PR #$($pr.number). Deleting the branch will close it."
    }

    foreach ($ref in $refs) {
        $b = $ref -replace '^refs/heads/', ''
        Write-Host "Deleting branch $b"
        git push $Remote --delete "refs/heads/$b" 2>$null
    }
    return $refs.Count
}

if ($Cleanup) { [void](Invoke-BranchCleanup); return }

if ($Invalidate) {
    # Deleting the branch does NOT retract the status on the OLD SHA — statuses are immutable
    # records on the commit they were written to. The merge gate re-blocks because the PR's NEW head
    # has no result of its own; removing the branch stops a stale build reporting late.
    $removed = Invoke-BranchCleanup
    if ($removed -gt 0) {
        $short = if ($NewSha) { $NewSha.Substring(0, [Math]::Min(8, $NewSha.Length)) } else { '' }
        $tmpl = @'
⚠️ A new commit (`__SHA__`) was pushed to this fork PR, so the previous validation no longer applies to the current head. Its validation branch was removed and the **WinUI-GitHub-PR (OneBranch)** check is outstanding again.

A maintainer must review the new code and comment `/validate` (optionally `/validate __SHA__`) to validate this commit. For security, new commits on fork PRs are never validated automatically.
'@
        $body = $tmpl -replace '__SHA__', $short
        Write-Host "Posting stale-validation notice on fork PR #$PrNumber ..."
        $body | gh pr comment $PrNumber --repo $Repo --body-file -
    } else {
        Write-Host "No validation artifacts to invalidate for fork PR #$PrNumber."
    }
    return
}

if ($Publish) {
    if (-not $info.isCrossRepository) { throw "PR #$PrNumber is not a fork; no validation branch needed." }
    if (-not $IReviewedTheSha) {
        throw "Refusing to publish: pass -IReviewedTheSha to confirm you reviewed exact SHA $Sha. " +
              "Publishing runs the fork's code in a credentialed pipeline."
    }

    # Surface build-instruction changes before anything is published: this is what stops a one-line
    # pipeline edit disappearing into a large product diff.
    Assert-BuildSurfaceReviewed

    Write-Host "Fetching pull/$PrNumber/head ..."
    git fetch --no-tags $Remote "pull/$PrNumber/head"
    if ($LASTEXITCODE -ne 0) { throw "git fetch of pull/$PrNumber/head failed (exit $LASTEXITCODE)." }
    $fetched = (git rev-parse FETCH_HEAD).Trim()
    if ($fetched -ne $Sha) { throw "Fetched SHA ($fetched) != PR head SHA ($Sha); aborting." }

    # Re-read the PR head immediately before pushing, so a fork that advanced mid-run can never get
    # an unreviewed commit published.
    $now = (Get-PrInfo).headRefOid
    if ($now -ne $Sha) {
        throw "PR head changed ($Sha -> $now) since start; re-run to validate the new SHA."
    }

    # Push the IDENTICAL commit object — never a cherry-pick, rebase or merge. The shared SHA is the
    # whole mechanism: it is what makes the status appear on the fork PR. The leading '+' force-
    # updates the -ReuseBranch head (still fires the CI trigger); per-SHA branches never move.
    Write-Host "Pushing identical commit to $Branch ..."
    git push $Remote "+$Sha`:refs/heads/$Branch"
    if ($LASTEXITCODE -ne 0) {
        throw "git push to $Branch failed (exit $LASTEXITCODE). A likely cause is a ref name " +
              "conflict with an existing validation branch for this PR; run -Cleanup and retry."
    }

    $upstream = (gh api "repos/$Repo/git/ref/heads/$Branch" --jq '.object.sha').Trim()
    if ($upstream -ne $Sha) { throw "Upstream branch SHA ($upstream) != $Sha; aborting." }

    Write-Host ""
    Write-Host "Published $Sha to $Branch."

    if ($QueueBuild) {
        Write-Host "Queueing build against $Branch ..."
        [void](Invoke-QueueBuild -Sha $Sha -Branch $Branch)
    } else {
        Write-Host "Relying on the CI trigger on validation/* to start the build (-CheckTrigger verifies it)."
    }

    Write-Host ""
    Write-Host "Azure DevOps reports '$Context' against $Sha, which is also this fork PR's head,"
    Write-Host "so the check lands on the PR."
    Write-Host ""

    # Publishing the branch is not the same as starting a build. Without a start mechanism the push
    # succeeds and nothing runs, so a caller treating "published" as "validated" would report
    # success while the required check never arrives. Fail loudly instead.
    if ($WaitForStartMinutes -gt 0) {
        Write-Host "Waiting up to $WaitForStartMinutes min for '$Context' to appear on $Sha ..."
        $deadline = (Get-Date).AddMinutes($WaitForStartMinutes)
        $started  = $null
        while ((Get-Date) -lt $deadline) {
            $started = @(Get-ValidationState $Sha) | Select-Object -First 1
            if ($started) { break }
            Start-Sleep -Seconds 20
        }

        if (-not $started) {
            throw @"
Published $Sha to $Branch, but no '$Context' check appeared within $WaitForStartMinutes minutes.
The branch exists; the build never started. The likely cause is that definition $AdoDefinitionId has
no CI trigger covering 'validation/*'. Diagnose with -CheckTrigger, or re-run with -QueueBuild to
queue explicitly. Failing, because the PR's required check will otherwise never arrive.
"@
        }
        Write-Host "Build started: $($started.State) - $($started.Url)"
    }

    if ($Wait) { Wait-Validation $Sha }
    else { Write-Host "Watch: .\ValidateForkPRBranch.ps1 -PrNumber $PrNumber -Status -Wait" }
    return
}

Write-Host "Nothing to do. Specify -Publish, -Status, -Cleanup, -Invalidate, or -CheckTrigger. (-Publish needs -IReviewedTheSha)"

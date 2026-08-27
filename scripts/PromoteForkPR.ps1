<#
.SYNOPSIS
    Promote a GitHub fork PR's exact head commit into the upstream microsoft-ui-xaml repo so the
    internal WinUI-GitHub-PR (OneBranch) pipeline can validate it (fork PRs otherwise fail at
    "Install Pipeline Tools" with 401 because they get no secrets).

.DESCRIPTION
    Implements the "trusted promotion / shadow PR" flow. Pushes the IDENTICAL commit object
    (never a cherry-pick) to validation/fork-pr/<n>/<shortsha> and opens a draft, do-not-merge
    validation PR. Because both PRs share the head SHA, the resulting check also lands on the
    original fork PR and satisfies the required-check gate.

    Requires maintainer push access to the upstream repo (to push the validation branch).

    SECURITY: promotion causes the fork's code/scripts/YAML to run in a CREDENTIALED pipeline.
    Only promote a commit AFTER reviewing that exact SHA. -Promote requires -IReviewedTheSha.

.PARAMETER PrNumber   Fork PR number (required).
.PARAMETER Repo       owner/name. Default microsoft/microsoft-ui-xaml.
.PARAMETER Remote     Local git remote pointing at upstream. Default origin.
.PARAMETER Promote    Push branch + open the draft shadow PR.
.PARAMETER Status     Show the WinUI-GitHub-PR (OneBranch) status on the fork head SHA.
.PARAMETER Cleanup    Close the shadow PR and delete the validation branch.
.PARAMETER Invalidate Tear down a stale validation after a new fork commit and prompt for re-/validate.
.PARAMETER NewSha     New fork head SHA, referenced in the -Invalidate prompt (optional).
.PARAMETER IReviewedTheSha  Required acknowledgement for -Promote.

.EXAMPLE
    # Promote and block until the required check resolves:
    .\PromoteForkPR.ps1 -PrNumber 11637 -Promote -IReviewedTheSha -Wait
.EXAMPLE
    # Re-validate across pushes with a single shadow PR (force-updated to each new SHA):
    .\PromoteForkPR.ps1 -PrNumber 11637 -Promote -IReviewedTheSha -ReuseBranch
.EXAMPLE
    .\PromoteForkPR.ps1 -PrNumber 11637 -Status -Wait
.EXAMPLE
    # Remove every shadow PR + validation branch for this fork PR:
    .\PromoteForkPR.ps1 -PrNumber 11637 -Cleanup
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][int]$PrNumber,
    [string]$Repo   = 'microsoft/microsoft-ui-xaml',
    [string]$Remote = 'origin',
    [switch]$Promote,
    [switch]$Status,
    [switch]$Cleanup,
    # Tear down a now-stale validation (its shadow PR + branch) after the fork pushed a NEW commit,
    # and prompt maintainers to re-review + /validate. We NEVER auto-promote the new commit, because
    # promotion runs fork code in a credentialed pipeline and every new SHA needs a fresh review.
    [switch]$Invalidate,
    # The fork PR's new head SHA (used only to reference the commit in the -Invalidate prompt).
    [string]$NewSha,
    [switch]$IReviewedTheSha,
    # Reuse ONE stable branch/shadow PR per fork PR (force-updated to each new SHA) instead of
    # creating a fresh SHA-suffixed branch+PR on every push. Reduces PR-list noise and orphans.
    [switch]$ReuseBranch,
    # After -Promote (or standalone), poll the required check until it leaves 'pending'.
    [switch]$Wait,
    [int]$TimeoutMinutes = 90,
    [string]$Label = 'fork-validation',
    # The pipeline requires a team member's comment before building an INTERNAL PR, so the shadow
    # PR does not build on its own (even when a maintainer authors it). By default we post '/azp run'
    # to start it; pass -NoAutoRun to skip (e.g. if you want to comment from a different identity).
    [switch]$NoAutoRun
)

$ErrorActionPreference = 'Stop'
$Context = 'WinUI-GitHub-PR (OneBranch)'

function Get-PrInfo {
    $json = gh pr view $PrNumber --repo $Repo --json headRefOid,baseRefName,isCrossRepository,state,url
    if ($LASTEXITCODE -ne 0) { throw "gh pr view failed for #$PrNumber" }
    $json | ConvertFrom-Json
}

function Wait-Check([string]$Sha) {
    $deadline = (Get-Date).AddMinutes($TimeoutMinutes)
    do {
        $s = gh api "repos/$Repo/commits/$Sha/status" `
            --jq ".statuses[] | select(.context==`"$Context`") | {state,description,target_url}" | ConvertFrom-Json
        $state = if ($s) { $s.state } else { 'none' }
        Write-Host ("[{0:HH:mm:ss}] {1} -> {2}  {3}" -f (Get-Date), $Context, $state, $s.description)
        if ($state -in 'success','failure','error') { return $s }
        Start-Sleep -Seconds 30
    } while ((Get-Date) -lt $deadline)
    Write-Warning "Timed out after $TimeoutMinutes min; check still '$state'."
    return $s
}

$info   = Get-PrInfo
$Sha    = $info.headRefOid
$Base   = $info.baseRefName
# Both naming schemes live UNDER the per-PR directory `validation/fork-pr/<n>/...` so they never
# collide in git's ref store. (Using a bare `validation/fork-pr/<n>` for -ReuseBranch would be a
# directory/file conflict once any per-SHA branch `validation/fork-pr/<n>/<sha>` exists.)
$Branch = if ($ReuseBranch) { "validation/fork-pr/$PrNumber/latest" }
          else              { "validation/fork-pr/$PrNumber/$($Sha.Substring(0,8))" }

if ($Status) {
    Write-Host "Fork PR #$PrNumber  head=$Sha  base=$Base  state=$($info.state)"
    if ($Wait) { Wait-Check $Sha; return }
    $s = gh api "repos/$Repo/commits/$Sha/status" `
        --jq ".statuses[] | select(.context==`"$Context`") | {state,description,target_url}"
    if ($s) { $s } else { Write-Host "No '$Context' status on $Sha yet." }
    return
}

function Invoke-ShadowCleanup {
    # Close ALL shadow PRs for this fork PR and delete EVERY validation ref (all SHAs + latest),
    # not just the current branch — re-validations may leave orphaned per-SHA branches behind.
    # Trailing slash so PR 11665 does not also match 116650, 116651, ... (ref/name prefix collision).
    $prefix = "validation/fork-pr/$PrNumber/"
    $refs = gh api "repos/$Repo/git/matching-refs/heads/$prefix" --jq '.[].ref' 2>$null
    $shadows = gh pr list --repo $Repo --search "head:validation/fork-pr/$PrNumber" --state all `
                 --json number,headRefName | ConvertFrom-Json |
                 Where-Object { $_.headRefName -like "$prefix*" }
    foreach ($pr in $shadows) {
        Write-Host "Closing shadow PR #$($pr.number) ($($pr.headRefName))"
        gh pr close $pr.number --repo $Repo --delete-branch 2>$null | Out-Null
    }
    foreach ($ref in $refs) {
        $b = $ref -replace '^refs/heads/',''
        Write-Host "Deleting branch $b"
        git push $Remote --delete "refs/heads/$b" 2>$null
    }
    $removed = @($refs).Count + @($shadows).Count
    if ($removed -eq 0) { Write-Host "Nothing to clean for $prefix." }
    return $removed
}

if ($Cleanup) { [void](Invoke-ShadowCleanup); return }

if ($Invalidate) {
    # A new commit landed on the fork PR, so the previously validated SHA is no longer the head.
    # Remove its shadow PR + validation branch (this drops the shared-SHA required check back to
    # 'pending', re-blocking merge) and tell maintainers a fresh review + /validate is needed.
    # We deliberately do NOT auto-promote the new commit: that would run un-reviewed fork code in a
    # credentialed pipeline. Every new SHA must be re-reviewed by a maintainer.
    $removed = Invoke-ShadowCleanup
    if ($removed -gt 0) {
        $short = if ($NewSha) { $NewSha.Substring(0, [Math]::Min(8, $NewSha.Length)) } else { '' }
        $tmpl = @'
⚠️ A new commit (`__SHA__`) was pushed to this fork PR, so the previous validation is now stale. Its shadow validation PR and branch were removed and the **WinUI-GitHub-PR (OneBranch)** check is pending again.

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

if ($Promote) {
    if (-not $info.isCrossRepository) { throw "PR #$PrNumber is not a fork; no promotion needed." }
    if (-not $IReviewedTheSha) {
        throw "Refusing to promote: pass -IReviewedTheSha to confirm you reviewed exact SHA $Sha. " +
              "Promotion runs the fork's code in a credentialed pipeline."
    }

    Write-Host "Fetching pull/$PrNumber/head ..."
    git fetch --no-tags $Remote "pull/$PrNumber/head"
    $fetched = (git rev-parse FETCH_HEAD).Trim()
    if ($fetched -ne $Sha) { throw "Fetched SHA ($fetched) != PR head SHA ($Sha); aborting." }

    # TOCTOU guard: re-read the PR head right before pushing; abort if the fork advanced.
    $now = (Get-PrInfo).headRefOid
    if ($now -ne $Sha) {
        throw "PR head changed ($Sha -> $now) since start; re-run to validate the new SHA."
    }

    # -ReuseBranch force-updates the stable branch (moves the single shadow PR to the new SHA);
    # per-SHA branches never move, so a plain push is fine there.
    Write-Host "Pushing identical commit to $Branch ..."
    git push $Remote "+$Sha`:refs/heads/$Branch"
    if ($LASTEXITCODE -ne 0) {
        throw "git push to $Branch failed (exit $LASTEXITCODE). A likely cause is a ref name " +
              "conflict with an existing validation branch for this PR; run -Cleanup and retry."
    }

    $upstream = (gh api "repos/$Repo/git/ref/heads/$Branch" --jq '.object.sha').Trim()
    if ($upstream -ne $Sha) { throw "Upstream branch SHA ($upstream) != $Sha; aborting." }

    # Idempotent: reuse an existing shadow PR for this branch instead of failing on duplicate.
    $existing = gh pr list --repo $Repo --head $Branch --state open --json number,url | ConvertFrom-Json
    if ($existing) {
        $prRef = $existing[0].url
        Write-Host "Reusing shadow PR: $prRef (head now $Sha)"
    } else {
        Write-Host "Opening draft shadow PR (base=$Base) ..."
        $prRef = gh pr create --repo $Repo --base $Base --head $Branch --draft `
            --title "[DO NOT MERGE] Fork-validation shadow for #$PrNumber" `
            --body  "Validation-only shadow of fork PR #$PrNumber (head SHA $Sha). Do not merge. Auto-generated."
        Write-Host "Shadow PR: $prRef"
        # Best-effort label for triage/automation; ignore if the label doesn't exist in the repo.
        if ($Label) { gh pr edit $prRef --repo $Repo --add-label $Label 2>$null | Out-Null }
    }

    # Internal PRs require a team member's comment before building, so start the build explicitly.
    # (Posted as the caller's identity; a maintainer running this satisfies the team-member gate.)
    if (-not $NoAutoRun) {
        Write-Host "Posting '/azp run' on the shadow PR to start the validation build ..."
        gh pr comment $prRef --repo $Repo --body '/azp run' | Out-Null
    }

    if ($Wait) { Wait-Check $Sha }
    else { Write-Host "Watch: .\PromoteForkPR.ps1 -PrNumber $PrNumber -Status -Wait" }
    return
}

Write-Host "Nothing to do. Specify -Promote, -Status, -Cleanup, or -Invalidate. (-Promote needs -IReviewedTheSha)"

<#
.SYNOPSIS
Merges the exact public mirror branch into the internal integration branch.

.DESCRIPTION
The script creates and publishes a no-fast-forward merge commit directly to
the configured target branch. It never rewrites history or force-pushes.

The source and target must already share history. Conflicts and concurrent
target updates fail the run without modifying the target branch.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$RepositoryDirectory,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$SourceBranchName,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetBranchName,

    [Parameter(Mandatory = $false)]
    [ValidateNotNullOrEmpty()]
    [string]$RemoteName
)

$ErrorActionPreference = "Stop"
$global:LASTEXITCODE = 0

function Invoke-GitCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    Write-Host -ForegroundColor Blue "##[command]git -C '$Repository' $($Arguments -join ' ')"
    & git -C $Repository @Arguments
    if ($global:LASTEXITCODE) {
        throw "Command 'git -C $Repository $($Arguments -join ' ')' failed with exit code '$global:LASTEXITCODE'."
    }
}

function Get-GitCommandResult {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    Write-Host -ForegroundColor Blue "##[command]git -C '$Repository' $($Arguments -join ' ')"
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = & git -C $Repository @Arguments 2>&1
        $exitCode = $global:LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    return @{
        ExitCode = $exitCode
        Output = ($output -join "`n").Trim()
    }
}

function Get-GitCommandOutput {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    $result = Get-GitCommandResult -Repository $Repository -Arguments $Arguments
    if ($result.ExitCode) {
        throw "Command 'git -C $Repository $($Arguments -join ' ')' failed with exit code '$($result.ExitCode)'."
    }

    return $result.Output
}

function Assert-ValidBranchName {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string]$BranchName,

        [Parameter(Mandatory = $true)]
        [string]$ParameterName
    )

    if ($BranchName.StartsWith("refs/heads/")) {
        throw "Parameter '$ParameterName' must be a branch name, not the full ref '$BranchName'."
    }

    $result = Get-GitCommandResult -Repository $Repository -Arguments @("check-ref-format", "--branch", $BranchName)
    if ($result.ExitCode) {
        throw "Parameter '$ParameterName' value '$BranchName' is not a valid Git branch name."
    }
}

function Test-GitAncestor {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string]$PossibleAncestor,

        [Parameter(Mandatory = $true)]
        [string]$Commit
    )

    $result = Get-GitCommandResult -Repository $Repository -Arguments @(
        "merge-base",
        "--is-ancestor",
        $PossibleAncestor,
        $Commit
    )

    if ($result.ExitCode -eq 0) {
        return $true
    }

    if ($result.ExitCode -eq 1) {
        return $false
    }

    throw "Unable to determine whether '$PossibleAncestor' is an ancestor of '$Commit'."
}

function Reset-PathToTarget {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string]$TargetCommit,

        [Parameter(Mandatory = $true)]
        [string]$Pathspec
    )

    $existsInTarget = Get-GitCommandResult -Repository $Repository -Arguments @("cat-file", "-e", "${TargetCommit}:$Pathspec")
    if ($existsInTarget.ExitCode -eq 0) {
        Invoke-GitCommand -Repository $Repository -Arguments @("checkout", $TargetCommit, "--", $Pathspec)
    } else {
        Invoke-GitCommand -Repository $Repository -Arguments @("rm", "-r", "--force", "--ignore-unmatch", "--", $Pathspec)
    }
}

$repositoryFullPath = [IO.Path]::GetFullPath([IO.Path]::Combine($pwd, $RepositoryDirectory))
if (-not (Test-Path -LiteralPath $repositoryFullPath -PathType Container)) {
    throw "RepositoryDirectory '$RepositoryDirectory' does not identify an existing directory."
}

$RemoteName = if ($RemoteName) {
    $RemoteName
} else {
    $remotes = @((Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @("remote")) -split "`n" |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($remotes.Count -ne 1) {
        throw "Repository '$repositoryFullPath' has $($remotes.Count) remotes. Supply RemoteName explicitly."
    }
    $remotes[0]
}

Assert-ValidBranchName -Repository $repositoryFullPath -BranchName $SourceBranchName -ParameterName "SourceBranchName"
Assert-ValidBranchName -Repository $repositoryFullPath -BranchName $TargetBranchName -ParameterName "TargetBranchName"
if ($SourceBranchName -eq $TargetBranchName) {
    throw "SourceBranchName and TargetBranchName must identify different branches."
}

$status = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @("status", "--porcelain")
if (-not [string]::IsNullOrWhiteSpace($status)) {
    throw "Repository '$repositoryFullPath' has uncommitted changes and cannot perform integration.`n$status"
}

$sourceRefName = "refs/heads/$SourceBranchName"
$targetRefName = "refs/heads/$TargetBranchName"
$sourceTrackingRef = "refs/remotes/$RemoteName/$SourceBranchName"
$targetTrackingRef = "refs/remotes/$RemoteName/$TargetBranchName"
Invoke-GitCommand -Repository $repositoryFullPath -Arguments @("config", "user.name", "WinUI GitHub integration")
Invoke-GitCommand -Repository $repositoryFullPath -Arguments @("config", "user.email", "winui-github-integration@microsoft.com")

$maximumPublishAttempts = 3
# Safety net for the rare case where the target moves between our fetch and
# our push (e.g. a manual/admin push or another writer). A non-fast-forward
# rejection triggers a re-fetch and rebuild on the new tip. Our own runs are
# already serialized by the stage's runLatest lock, so this seldom fires.
for ($publishAttempt = 1; $publishAttempt -le $maximumPublishAttempts; $publishAttempt++) {
    Write-Host "##[group]Fetching integration branches (attempt $publishAttempt of $maximumPublishAttempts)"
    $fetchArguments = @(
        "fetch",
        "--prune",
        "--no-tags"
    )
    $repositoryIsShallow = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @(
        "rev-parse",
        "--is-shallow-repository"
    )
    if ($repositoryIsShallow -eq "true") {
        $fetchArguments += "--unshallow"
    }
    $fetchArguments += @(
        $RemoteName,
        "+$sourceRefName`:$sourceTrackingRef",
        "+$targetRefName`:$targetTrackingRef"
    )
    Invoke-GitCommand -Repository $repositoryFullPath -Arguments $fetchArguments
    $sourceCommit = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @("rev-parse", "--verify", "$sourceTrackingRef^{commit}")
    $targetCommit = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @("rev-parse", "--verify", "$targetTrackingRef^{commit}")
    $repositoryIsShallow = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @(
        "rev-parse",
        "--is-shallow-repository"
    )
    if ($repositoryIsShallow -eq "true") {
        $shallowFile = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @(
            "rev-parse",
            "--git-path",
            "shallow"
        )
        if (-not [System.IO.Path]::IsPathRooted($shallowFile)) {
            $shallowFile = Join-Path $repositoryFullPath $shallowFile
        }
        foreach ($shallowBoundary in @(Get-Content -LiteralPath $shallowFile)) {
            if ((Test-GitAncestor -Repository $repositoryFullPath -PossibleAncestor $shallowBoundary -Commit $sourceCommit) -or
                (Test-GitAncestor -Repository $repositoryFullPath -PossibleAncestor $shallowBoundary -Commit $targetCommit)) {
                throw "Integration history remains shallow at '$shallowBoundary' after fetching '$SourceBranchName' and '$TargetBranchName'."
            }
        }
    }
    Write-Host "Source '$SourceBranchName': $sourceCommit"
    Write-Host "Target '$TargetBranchName': $targetCommit"
    Write-Host "##[endgroup]"

    if (Test-GitAncestor -Repository $repositoryFullPath -PossibleAncestor $sourceCommit -Commit $targetCommit) {
        Write-Host "Target '$TargetBranchName' already contains source commit '$sourceCommit'. No merge is needed."
        Write-Host "##vso[task.setvariable variable=IntegratedSourceCommit;isOutput=true]$sourceCommit"
        Write-Host "##vso[task.setvariable variable=PublishedTargetCommit;isOutput=true]$targetCommit"
        exit 0
    }

    $restorePathsListText = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @(
        "show",
        "${sourceCommit}:build/PipelineScripts/WinUISourceMirroringRestorePaths.txt"
    )
    $restorePathspecs = @(
        $restorePathsListText -split "`n" |
            ForEach-Object { $_.Trim() } |
            Where-Object { $_ -and (-not $_.StartsWith("#")) } |
            ForEach-Object { $_ -replace "\\", "/" }
    )
    Write-Host "Preserving $($restorePathspecs.Count) divergent path(s) at the target's version during integration."

    Write-Host "##[group]Creating integration merge"
    Invoke-GitCommand -Repository $repositoryFullPath -Arguments @("checkout", "--detach", $targetTrackingRef)

    $mergeBaseResult = Get-GitCommandResult -Repository $repositoryFullPath -Arguments @(
        "merge-base",
        $targetTrackingRef,
        $sourceTrackingRef
    )
    if ($mergeBaseResult.ExitCode -eq 1) {
        throw "Source '$SourceBranchName' and target '$TargetBranchName' have unrelated histories. Complete the one-time ancestry setup outside this recurring integration pipeline."
    } elseif ($mergeBaseResult.ExitCode) {
        throw "Unable to determine the merge base between '$TargetBranchName' and '$SourceBranchName'."
    }

    $mergeResult = Get-GitCommandResult -Repository $repositoryFullPath -Arguments @(
        "merge",
        "--no-ff",
        "--no-commit",
        $sourceTrackingRef
    )
    $mergeHeadResult = Get-GitCommandResult -Repository $repositoryFullPath -Arguments @("rev-parse", "-q", "--verify", "MERGE_HEAD")
    if ($mergeHeadResult.ExitCode) {
        & git -C $repositoryFullPath merge --abort 2>$null
        throw "Unable to start the integration merge of '$SourceBranchName'.`n$($mergeResult.Output)"
    }

    # Keep divergent paths from crossing repositories. WinUISourceMirroringRestorePaths.txt lists
    # paths where GitHub and the internal repo intentionally differ; the outbound mirror preserves
    # GitHub's copies, and inbound we likewise preserve the target's. Reset each listed path to the
    # target's version. This runs even on a clean merge, so source-only paths that merged without a
    # conflict (e.g. specs) are dropped, not just modify/delete conflicts.
    foreach ($restorePathspec in $restorePathspecs) {
        Reset-PathToTarget -Repository $repositoryFullPath -TargetCommit $targetCommit -Pathspec $restorePathspec
    }

    $unmergedPaths = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @("diff", "--name-only", "--diff-filter=U")
    if (-not [string]::IsNullOrWhiteSpace($unmergedPaths)) {
        $conflicts = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @("status", "--short")
        & git -C $repositoryFullPath merge --abort 2>$null
        throw "Unable to integrate '$SourceBranchName' because the merge has conflicts outside the restore-path list.`n$conflicts"
    }

    Invoke-GitCommand -Repository $repositoryFullPath -Arguments @(
        "commit",
        "--no-edit",
        "-m",
        "Merge GitHub main into $TargetBranchName"
    )

    $mergeCommit = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @("rev-parse", "HEAD")
    $mergeParents = (Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @(
        "rev-list",
        "--parents",
        "-n",
        "1",
        $mergeCommit
    )).Split(" ")
    if ($mergeParents.Count -ne 3 -or $mergeParents[1] -ne $targetCommit -or $mergeParents[2] -ne $sourceCommit) {
        throw "Merge '$mergeCommit' must have exactly target '$targetCommit' as its first parent and source '$sourceCommit' as its second parent."
    }

    Write-Host "Created integration merge '$mergeCommit'."
    Write-Host "##[endgroup]"

    Write-Host "##[group]Publishing integration merge"
    $pushResult = Get-GitCommandResult -Repository $repositoryFullPath -Arguments @(
        "push",
        "--porcelain",
        $RemoteName,
        "$mergeCommit`:$targetRefName"
    )
    if ($pushResult.ExitCode) {
        $isConcurrentUpdate = $pushResult.Output -match "(?m)^!\s+.*\[(remote )?rejected\]\s+.*\(.*(fetch first|non-fast-forward|stale info|incorrect old value provided|TF401028|has already been updated by another client).*\)\s*$"
        if (-not $isConcurrentUpdate) {
            throw "Unable to publish merge '$mergeCommit'.`n$($pushResult.Output)"
        }

        $currentTargetResult = Get-GitCommandResult -Repository $repositoryFullPath -Arguments @(
            "ls-remote",
            "--exit-code",
            $RemoteName,
            $targetRefName
        )
        if ($currentTargetResult.ExitCode) {
            throw "Unable to publish merge '$mergeCommit' and unable to query target branch '$TargetBranchName'.`n$($pushResult.Output)"
        }

        $currentTargetCommit = ($currentTargetResult.Output -split "\s+")[0]
        if ($currentTargetCommit -eq $targetCommit) {
            throw "Unable to publish merge '$mergeCommit' while target branch '$TargetBranchName' remained at '$targetCommit'.`n$($pushResult.Output)"
        }
        if ($publishAttempt -eq $maximumPublishAttempts) {
            throw "Target branch '$TargetBranchName' advanced from '$targetCommit' to '$currentTargetCommit' while publishing, and all $maximumPublishAttempts attempts were exhausted.`n$($pushResult.Output)"
        }

        Write-Warning "Target branch '$TargetBranchName' advanced from '$targetCommit' to '$currentTargetCommit' while publishing. Rebuilding the merge from current branch tips."
        Write-Host "##[endgroup]"
        continue
    }

    Invoke-GitCommand -Repository $repositoryFullPath -Arguments @(
        "fetch",
        "--no-tags",
        $RemoteName,
        "+$targetRefName`:$targetTrackingRef"
    )
    $publishedTargetCommit = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @(
        "rev-parse",
        "--verify",
        "$targetTrackingRef^{commit}"
    )
    if (-not (Test-GitAncestor -Repository $repositoryFullPath -PossibleAncestor $mergeCommit -Commit $publishedTargetCommit)) {
        throw "Target branch '$TargetBranchName' at '$publishedTargetCommit' does not contain published merge '$mergeCommit'."
    }

    Write-Host "Published target branch '$TargetBranchName' at '$publishedTargetCommit'."
    Write-Host "##vso[task.setvariable variable=IntegratedSourceCommit;isOutput=true]$sourceCommit"
    Write-Host "##vso[task.setvariable variable=PublishedTargetCommit;isOutput=true]$publishedTargetCommit"
    Write-Host "##[endgroup]"
    exit 0
}

<#
.SYNOPSIS
Merges the exact public mirror branch into the internal integration branch.

.DESCRIPTION
The script creates and publishes a no-fast-forward merge commit directly to
the configured target branch. It never rewrites history or force-pushes.

The first integration establishes the relationship between unrelated histories
with an "ours" merge, preserving the target tree while recording the current
public mirror commit as integrated. Subsequent integrations use ordinary
no-fast-forward merges. Conflicts and concurrent target updates fail the run
without modifying the target branch.
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
    $output = & git -C $Repository @Arguments
    $exitCode = $global:LASTEXITCODE

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

Write-Host "##[group]Fetching integration branches"
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
Write-Host "Source '$SourceBranchName': $sourceCommit"
Write-Host "Target '$TargetBranchName': $targetCommit"
Write-Host "##[endgroup]"

if (Test-GitAncestor -Repository $repositoryFullPath -PossibleAncestor $sourceCommit -Commit $targetCommit) {
    Write-Host "Target '$TargetBranchName' already contains source commit '$sourceCommit'. No merge is needed."
    Write-Host "##vso[task.setvariable variable=IntegratedSourceCommit;isOutput=true]$sourceCommit"
    Write-Host "##vso[task.setvariable variable=PublishedTargetCommit;isOutput=true]$targetCommit"
    exit 0
}

Write-Host "##[group]Creating integration merge"
Invoke-GitCommand -Repository $repositoryFullPath -Arguments @("checkout", "--detach", $targetTrackingRef)
Invoke-GitCommand -Repository $repositoryFullPath -Arguments @("config", "user.name", "WinUI GitHub integration")
Invoke-GitCommand -Repository $repositoryFullPath -Arguments @("config", "user.email", "winui-github-integration@microsoft.com")

$mergeBaseResult = Get-GitCommandResult -Repository $repositoryFullPath -Arguments @(
    "merge-base",
    $targetTrackingRef,
    $sourceTrackingRef
)
$isBootstrap = $false
$mergeArguments = @("merge", "--no-ff", "--no-edit")
if ($mergeBaseResult.ExitCode -eq 1) {
    $isBootstrap = $true
    Write-Host "No merge base exists. Creating the one-time ancestry bootstrap while preserving the target tree."
    $mergeArguments += @(
        "--allow-unrelated-histories",
        "--strategy=ours"
    )
} elseif ($mergeBaseResult.ExitCode) {
    throw "Unable to determine the merge base between '$TargetBranchName' and '$SourceBranchName'."
}

$mergeMessage = if ($isBootstrap) {
    "Bootstrap GitHub main ancestry into $TargetBranchName"
} else {
    "Merge GitHub main into $TargetBranchName"
}
$mergeArguments += @(
    "-m",
    $mergeMessage,
    $sourceTrackingRef
)

$mergeResult = Get-GitCommandResult -Repository $repositoryFullPath -Arguments $mergeArguments
if ($mergeResult.ExitCode) {
    $conflicts = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @("status", "--short")
    & git -C $repositoryFullPath merge --abort 2>$null
    throw "Unable to integrate '$SourceBranchName' because the merge has conflicts.`n$conflicts"
}

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

if ($isBootstrap) {
    $targetTree = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @("rev-parse", "$targetCommit^{tree}")
    $mergeTree = Get-GitCommandOutput -Repository $repositoryFullPath -Arguments @("rev-parse", "$mergeCommit^{tree}")
    if ($mergeTree -ne $targetTree) {
        throw "Bootstrap merge '$mergeCommit' changed the target tree. Expected '$targetTree', found '$mergeTree'."
    }
    Write-Host "Verified that the ancestry bootstrap preserves the target tree '$targetTree'."
}
Write-Host "Created integration merge '$mergeCommit'."
Write-Host "##[endgroup]"

Write-Host "##[group]Publishing integration merge"
Invoke-GitCommand -Repository $repositoryFullPath -Arguments @(
    "push",
    $RemoteName,
    "$mergeCommit`:$targetRefName"
)

$publishedTarget = Get-GitCommandResult -Repository $repositoryFullPath -Arguments @(
    "ls-remote",
    "--exit-code",
    $RemoteName,
    $targetRefName
)
if ($publishedTarget.ExitCode) {
    throw "Unable to verify target branch '$TargetBranchName' after publication."
}

$publishedTargetCommit = ($publishedTarget.Output -split "\s+")[0]
if ($publishedTargetCommit -ne $mergeCommit) {
    throw "Target branch '$TargetBranchName' is at '$publishedTargetCommit', expected merge '$mergeCommit'."
}

Write-Host "Published target branch '$TargetBranchName' at '$publishedTargetCommit'."
Write-Host "##vso[task.setvariable variable=IntegratedSourceCommit;isOutput=true]$sourceCommit"
Write-Host "##vso[task.setvariable variable=PublishedTargetCommit;isOutput=true]$publishedTargetCommit"
Write-Host "##[endgroup]"

exit 0

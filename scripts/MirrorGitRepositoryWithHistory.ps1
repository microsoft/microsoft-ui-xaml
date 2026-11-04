<#
.SYNOPSIS
Mirrors one Git branch to another repository while preserving commit history.

.DESCRIPTION
This script is intended for Azure Pipelines scenarios where a public GitHub
branch is the source of truth and an Azure Repos branch is an exact mirror.
It performs a fast-forward-only push from the source branch to the target
branch when the target branch already exists. If the target branch does not
exist, the push creates it at the source commit. It never force-pushes,
rewrites commits, or attempts to preserve target-only files.

Target-only files are not compatible with an exact history-preserving mirror:
the target branch must be treated as a pure mirror of the source branch.

.PARAMETER SourceRepositoryDirectory
Path to the local clone of the source repository.

.PARAMETER SourceBranchName
Name of the branch in the source repository to mirror.

.PARAMETER ExpectedSourceCommit
Optional full commit hash that the source branch must contain and the mirror
must publish. Use this to bind a pipeline run to its triggering commit.

.PARAMETER TargetRepositoryDirectory
Path to the local clone of the target repository.

.PARAMETER TargetBranchName
Name of the branch in the target repository to update.

.PARAMETER SourceRemoteName
Name of the source repository remote. Defaults to the only configured remote.

.PARAMETER TargetRemoteName
Name of the target repository remote. Defaults to the only configured remote.

.PARAMETER SourceRemoteUrlPattern
Optional regular expression that the source remote URL must match.

.PARAMETER TargetRemoteUrlPattern
Optional regular expression that the target remote URL must match.

#>

param(
    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$SourceRepositoryDirectory,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$SourceBranchName,

    [parameter(Mandatory=$false)]
    [ValidatePattern("^[0-9a-fA-F]{40}$")]
    [string]$ExpectedSourceCommit,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetRepositoryDirectory,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetBranchName,

    [parameter(Mandatory=$false)]
    [ValidateNotNullOrEmpty()]
    [string]$SourceRemoteName,

    [parameter(Mandatory=$false)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetRemoteName,

    [parameter(Mandatory=$false)]
    [ValidateNotNullOrEmpty()]
    [string]$SourceRemoteUrlPattern,

    [parameter(Mandatory=$false)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetRemoteUrlPattern
)

$ErrorActionPreference = "Stop"
$global:LASTEXITCODE = 0

function Invoke-GitCommand {
    param(
        [parameter(Mandatory=$true)]
        [string]$RepositoryDirectory,

        [parameter(Mandatory=$true)]
        [string[]]$Arguments
    )

    Write-Host -ForegroundColor Blue "##[command]git -C '$RepositoryDirectory' $($Arguments -join ' ')"
    & git -C $RepositoryDirectory @Arguments
    $exitCode = $global:LASTEXITCODE

    if ($exitCode) {
        throw "Command 'git -C $RepositoryDirectory $($Arguments -join ' ')' terminated with error code '$exitCode'"
    }
}

function Get-GitCommandOutput {
    param(
        [parameter(Mandatory=$true)]
        [string]$RepositoryDirectory,

        [parameter(Mandatory=$true)]
        [string[]]$Arguments
    )

    Write-Host -ForegroundColor Blue "##[command]git -C '$RepositoryDirectory' $($Arguments -join ' ')"
    $output = & git -C $RepositoryDirectory @Arguments
    $exitCode = $global:LASTEXITCODE

    if ($exitCode) {
        throw "Command 'git -C $RepositoryDirectory $($Arguments -join ' ')' terminated with error code '$exitCode'"
    }

    return ($output -join "`n").Trim()
}

function Get-GitCommandOutputWithExitCode {
    param(
        [parameter(Mandatory=$true)]
        [string]$RepositoryDirectory,

        [parameter(Mandatory=$true)]
        [string[]]$Arguments
    )

    Write-Host -ForegroundColor Blue "##[command]git -C '$RepositoryDirectory' $($Arguments -join ' ')"
    $output = & git -C $RepositoryDirectory @Arguments
    $exitCode = $global:LASTEXITCODE

    return @{
        ExitCode = $exitCode
        Output = ($output -join "`n").Trim()
    }
}

function Resolve-GitRemoteName {
    param(
        [parameter(Mandatory=$true)]
        [string]$RepositoryDirectory,

        [parameter(Mandatory=$false)]
        [string]$RemoteNameParameter,

        [parameter(Mandatory=$true)]
        [string]$ParameterName
    )

    if ($RemoteNameParameter) {
        return $RemoteNameParameter
    }

    $remotes = @((Get-GitCommandOutput $RepositoryDirectory @("remote")) -split "`n" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($remotes.Count -ne 1) {
        throw "Parameter '$ParameterName' was not supplied and repository '$RepositoryDirectory' has $($remotes.Count) remotes. Supply '$ParameterName' explicitly."
    }

    return $remotes[0]
}

function Assert-CleanWorktree {
    param(
        [parameter(Mandatory=$true)]
        [string]$RepositoryDirectory
    )

    $status = Get-GitCommandOutput $RepositoryDirectory @("status", "--porcelain")
    if (-not [string]::IsNullOrWhiteSpace($status)) {
        throw "Repository '$RepositoryDirectory' has uncommitted changes and cannot be used for mirroring.`n$status"
    }
}

function Assert-RemoteUrlMatches {
    param(
        [parameter(Mandatory=$true)]
        [string]$RepositoryDirectory,

        [parameter(Mandatory=$true)]
        [string]$RemoteName,

        [parameter(Mandatory=$false)]
        [string]$RemoteUrlPattern,

        [parameter(Mandatory=$true)]
        [string]$RemoteDescription
    )

    $remoteUrl = Get-GitCommandOutput $RepositoryDirectory @("remote", "get-url", $RemoteName)
    Write-Host "##[debug]$RemoteDescription remote URL: $remoteUrl"

    if ($RemoteUrlPattern -and $remoteUrl -notmatch $RemoteUrlPattern) {
        throw "$RemoteDescription remote '$RemoteName' URL '$remoteUrl' does not match expected pattern '$RemoteUrlPattern'."
    }
}

function Assert-ValidBranchName {
    param(
        [parameter(Mandatory=$true)]
        [string]$RepositoryDirectory,

        [parameter(Mandatory=$true)]
        [string]$BranchName,

        [parameter(Mandatory=$true)]
        [string]$ParameterName
    )

    if ($BranchName.StartsWith("refs/heads/")) {
        throw "Parameter '$ParameterName' should be a branch name like 'winui3/main', not a full ref like '$BranchName'."
    }

    Write-Host -ForegroundColor Blue "##[command]git -C '$RepositoryDirectory' check-ref-format --branch '$BranchName'"
    & git -C $RepositoryDirectory check-ref-format --branch $BranchName | Out-Null
    $exitCode = $global:LASTEXITCODE

    if ($exitCode) {
        throw "Parameter '$ParameterName' value '$BranchName' is not a valid Git branch name."
    }
}

$SourceRepositoryDirectoryFullPath = [IO.Path]::Combine($pwd, $SourceRepositoryDirectory)
if (-not (Test-Path $SourceRepositoryDirectoryFullPath)) {
    throw "Parameter 'SourceRepositoryDirectory' does not point to a valid path"
}

$TargetRepositoryDirectoryFullPath = [IO.Path]::Combine($pwd, $TargetRepositoryDirectory)
if (-not (Test-Path $TargetRepositoryDirectoryFullPath)) {
    throw "Parameter 'TargetRepositoryDirectory' does not point to a valid path"
}

$SourceRemoteName = Resolve-GitRemoteName $SourceRepositoryDirectoryFullPath $SourceRemoteName "SourceRemoteName"
$TargetRemoteName = Resolve-GitRemoteName $TargetRepositoryDirectoryFullPath $TargetRemoteName "TargetRemoteName"

Write-Host "##[group]Validating repositories"
Assert-ValidBranchName $SourceRepositoryDirectoryFullPath $SourceBranchName "SourceBranchName"
Assert-ValidBranchName $TargetRepositoryDirectoryFullPath $TargetBranchName "TargetBranchName"
Assert-CleanWorktree $SourceRepositoryDirectoryFullPath
Assert-CleanWorktree $TargetRepositoryDirectoryFullPath
Assert-RemoteUrlMatches $SourceRepositoryDirectoryFullPath $SourceRemoteName $SourceRemoteUrlPattern "Source"
Assert-RemoteUrlMatches $TargetRepositoryDirectoryFullPath $TargetRemoteName $TargetRemoteUrlPattern "Target"
Write-Host "##[endgroup]"

Write-Host "##[group]Fetching source and target branches"
$sourceFetchArguments = @("fetch", "--prune", "--no-tags")
$sourceRepositoryIsShallow = Get-GitCommandOutput $SourceRepositoryDirectoryFullPath @("rev-parse", "--is-shallow-repository")
if ($sourceRepositoryIsShallow -eq "true") {
    $sourceFetchArguments += "--unshallow"
}
$sourceFetchArguments += @($SourceRemoteName, "+refs/heads/$SourceBranchName`:refs/remotes/$SourceRemoteName/$SourceBranchName")
Invoke-GitCommand $SourceRepositoryDirectoryFullPath $sourceFetchArguments

$sourceTrackingRef = "refs/remotes/$SourceRemoteName/$SourceBranchName"
$sourceBranchCommit = Get-GitCommandOutput $SourceRepositoryDirectoryFullPath @("rev-parse", "--verify", "$sourceTrackingRef^{commit}")
Write-Host "##[debug]Source $SourceRemoteName/$SourceBranchName branch commit: $sourceBranchCommit"

if ($ExpectedSourceCommit) {
    $sourceCommit = Get-GitCommandOutput $SourceRepositoryDirectoryFullPath @("rev-parse", "--verify", "$ExpectedSourceCommit^{commit}")

    Write-Host -ForegroundColor Blue "##[command]git -C '$SourceRepositoryDirectoryFullPath' merge-base --is-ancestor $sourceCommit $sourceBranchCommit"
    & git -C $SourceRepositoryDirectoryFullPath merge-base --is-ancestor $sourceCommit $sourceBranchCommit
    $exitCode = $global:LASTEXITCODE

    if ($exitCode -eq 1) {
        throw "Expected source commit '$sourceCommit' is not contained in source branch '$SourceBranchName' at '$sourceBranchCommit'."
    } elseif ($exitCode) {
        throw "Source branch containment validation failed with error code '$exitCode'."
    }
} else {
    $sourceCommit = $sourceBranchCommit
}

Write-Host "##[debug]Source $SourceRemoteName/$SourceBranchName commit: $sourceCommit"
Write-Host "##[endgroup]"

Write-Host "##[group]Importing source commit into target repository"
$sourceBranchRefInTargetRepository = "refs/remotes/sourceRepository/$SourceBranchName"
Invoke-GitCommand $TargetRepositoryDirectoryFullPath @("fetch", "--no-tags", $SourceRepositoryDirectoryFullPath, "+$sourceTrackingRef`:$sourceBranchRefInTargetRepository")
$sourceCommitInTargetRepository = Get-GitCommandOutput $TargetRepositoryDirectoryFullPath @("rev-parse", "--verify", "$sourceCommit^{commit}")

if ($sourceCommitInTargetRepository -ne $sourceCommit) {
    throw "Source commit imported into target repository as '$sourceCommitInTargetRepository', expected '$sourceCommit'."
}

$sourceRefInTargetRepository = "refs/remotes/sourceRepository/mirrorSource"
Invoke-GitCommand $TargetRepositoryDirectoryFullPath @("update-ref", $sourceRefInTargetRepository, $sourceCommitInTargetRepository)
Write-Host "##[endgroup]"

Write-Host "##[group]Validating fast-forward mirror"
$targetBranchRef = "refs/heads/$TargetBranchName"
$targetBranchSearch = Get-GitCommandOutputWithExitCode $TargetRepositoryDirectoryFullPath @("ls-remote", "--exit-code", $TargetRemoteName, $targetBranchRef)
$targetBranchExists = $targetBranchSearch.ExitCode -eq 0

if ($targetBranchExists) {
    Invoke-GitCommand $TargetRepositoryDirectoryFullPath @("fetch", "--prune", $TargetRemoteName, "+$targetBranchRef`:refs/remotes/$TargetRemoteName/$TargetBranchName")
    $targetTrackingRef = "refs/remotes/$TargetRemoteName/$TargetBranchName"
    $targetCommit = Get-GitCommandOutput $TargetRepositoryDirectoryFullPath @("rev-parse", "--verify", "$targetTrackingRef^{commit}")
    Write-Host "##[debug]Target $TargetRemoteName/$TargetBranchName commit: $targetCommit"

    if ($targetCommit -eq $sourceCommitInTargetRepository) {
        Write-Host "Target branch '$TargetBranchName' ($targetCommit) is already at source '$SourceBranchName' ($sourceCommitInTargetRepository). Nothing to mirror."
        Write-Host "##[endgroup]"
        exit 0
    }

    Write-Host -ForegroundColor Blue "##[command]git -C '$TargetRepositoryDirectoryFullPath' merge-base --is-ancestor $targetCommit $sourceCommitInTargetRepository"
    & git -C $TargetRepositoryDirectoryFullPath merge-base --is-ancestor $targetCommit $sourceCommitInTargetRepository
    $exitCode = $global:LASTEXITCODE

    if ($exitCode -eq 1) {
        throw "Refusing to mirror because target '$TargetBranchName' ($targetCommit) is not an ancestor of source '$SourceBranchName' ($sourceCommitInTargetRepository). This would require rewriting target history."
    } elseif ($exitCode) {
        throw "Fast-forward validation failed with error code '$exitCode'."
    }

    Write-Host "Target branch '$TargetBranchName' can fast-forward from $targetCommit to $sourceCommitInTargetRepository."
} elseif ($targetBranchSearch.ExitCode -eq 2) {
    Write-Host "Target branch '$TargetBranchName' does not exist on '$TargetRemoteName'. It will be created at $sourceCommitInTargetRepository."
} else {
    throw "Unable to query target branch '$TargetBranchName'. 'git ls-remote' failed with error code '$($targetBranchSearch.ExitCode)'."
}
Write-Host "##[endgroup]"

Write-Host "##[group]Publishing mirror"
Invoke-GitCommand $TargetRepositoryDirectoryFullPath @("push", $TargetRemoteName, "$sourceRefInTargetRepository`:refs/heads/$TargetBranchName")

$publishedTargetBranch = Get-GitCommandOutputWithExitCode $TargetRepositoryDirectoryFullPath @("ls-remote", "--exit-code", $TargetRemoteName, $targetBranchRef)
if ($publishedTargetBranch.ExitCode) {
    throw "Unable to verify published target branch '$TargetBranchName'. 'git ls-remote' failed with error code '$($publishedTargetBranch.ExitCode)'."
}

$publishedTargetCommit = ($publishedTargetBranch.Output -split "\s+")[0]
if ($publishedTargetCommit -ne $sourceCommitInTargetRepository) {
    throw "Published target branch '$TargetBranchName' is at '$publishedTargetCommit', expected '$sourceCommitInTargetRepository'."
}

Write-Host "Published target branch '$TargetBranchName' at expected commit $publishedTargetCommit."
Write-Host "##[endgroup]"

exit 0

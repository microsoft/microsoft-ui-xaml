# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

<#
.SYNOPSIS
Script to be used within the context of an Azure Devops Pipeline to mirror a snapshot
of a Git repo into a different Git repo.

.DESCRIPTION

.PARAMETER SelfRepositoryDirectory
Specifies the path to the directory containing the repository that triggered this script. 
This path can be relative.

.PARAMETER SourceRepositoryDirectory
Specifies the path to the directory containing the repository to be used as the mirroring source. 
This path can be relative.

.PARAMETER SourceRepositoryCommittish
Specifies the committish that the 'source' repository should be synced to before beginning the mirror
operation.

.PARAMETER TargetRepositoryDirectory
Specifies the path to the directory containing the repository to be used as the mirroring target. 
This path can be relative.

.PARAMETER TargetRepositoryBranchName
Specifies the branch in the 'target' repository whose HEAD will be used as the initial state of the target.

.PARAMETER MergeBackToDefaultBranch
(Optional) Specifies whether the target repository branch should subsequently be merged back to the default
branch. Defaults to 'automatic'.
Valid values:
- 'automatic' - allow the script to make the decision
- 'always' - merge back to default branch
- 'never' - do not merge back to default branch

.PARAMETER TargetRepositorySubdirectory
(Optional) Specifies a subdirectory of the 'target' repository that the source code should be mirrored into
instead of the repository root.

.PARAMETER TargetRepositoryTagName
(Optional) Specifies the tag to associate with the commit in the 'target' repository.

.PARAMETER ExclusionPathspecsFile
(Optional) Specifies the path to a file containing Git pathspecs that specify which files 
should be excluded from the target repository. This path may be either absolute or 
relative to either the 'self' or 'source' repository's root; the `UseExclusionsFileFromSelfRepo`
parameter controls which root is used if a relative path is provided.

.PARAMETER RestorePathspecsFile
(Optional) Specifies the path to a file listing target-repo paths whose target-branch
contents should be restored (via `git checkout --`) after the mirror's robocopy /mir step.
Use this to carve out target-only paths (e.g. `NuGet.config`, OSS-only docs/specs) that
would otherwise be purged by /mir. One path per line; `#` comments and blank lines are
ignored. Paths are interpreted relative to the target repo root; the `$TargetRepositorySubdirectory`
parameter, when provided, is automatically prepended to each entry so the same file works
for both root-layout and `src/`-subdirectory mirrors. This path may be either absolute or
relative to the 'self' or 'source' repository's root, governed by the same
`UseExclusionsFileFromSelfRepo` parameter as the exclusions file.

.PARAMETER UseExclusionsFileFromSelfRepo
(Optional) Specifies whether the exclusions file should be sourced from the 'self' or the 'source'
repository.

.PARAMETER ComputeCommitDescription
(Optional) Specifies whether the commit description should automatically compute the commit
description by using the first line from all commits since the last committish.

#>

param(
    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$SelfRepositoryDirectory,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$SourceRepositoryDirectory,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$SourceRepositoryCommittish,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetRepositoryDirectory,

    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetRepositoryBranchName,

    [parameter(Mandatory=$false)]
    [ValidateSet("automatic", "always", "never")]
    [string]$MergeBackToDefaultBranch = "automatic",

    [parameter(Mandatory=$false)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetRepositorySubdirectory,

    [parameter(Mandatory=$false)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetRepositoryTagName,

    [parameter(Mandatory=$false)]
    [string]$ExclusionPathspecsFile,

    [parameter(Mandatory=$false)]
    [string]$RestorePathspecsFile,

    [parameter(Mandatory=$false)]
    [switch]$UseExclusionsFileFromSelfRepo,

    [parameter(Mandatory=$false)]
    [switch]$ComputeCommitDescription
)

function Run-LoggedCommand {
    param(
        [parameter(Mandatory=$true)]
        [string]$command
    )

    Write-Host -ForegroundColor Blue "##[command]$command $arguments"
    Invoke-Expression $command

    if ($command.StartsWith("robocopy ")) {
        if ($LASTEXITCODE -gt 7) {
            # Robocopy doesn't follow exit code conventions; any code from 0-7 indicates success
            # while those greater than 7 indicate failure.
            Write-Error "##[error]Command '$command' terminated with error code '$LASTEXITCODE'"
            throw
        }
    }
    elseif ($LASTEXITCODE) {
        Write-Error "##[error]Command '$command' terminated with error code '$LASTEXITCODE'"
        throw
    }
}

$ErrorActionPreference = "Stop"

$TempDirectory = if ($env:AGENT_TEMPDIRECTORY) { $env:AGENT_TEMPDIRECTORY } else { $env:TEMP }
$LogDirectory = if ($env:OB_OUTPUTDIRECTORY) { $env:OB_OUTPUTDIRECTORY } else { $TempDirectory }

# Ensure we start with a clean state in case this property was left from some
# previous script run.
$LASTEXITCODE = 0

# Build and validate full paths from the parameters. 
# Note: Path.Combine returns the second argument if it is an absolute path
$SelfRepositoryDirectoryFullPath = [IO.Path]::Combine($pwd, $SelfRepositoryDirectory)
if (-not (Test-Path $SelfRepositoryDirectoryFullPath)) { 
    throw "Parameter 'SelfRepositoryDirectory' does not point to a valid path"
}
$SourceRepositoryDirectoryFullPath = [IO.Path]::Combine($pwd, $SourceRepositoryDirectory)
if (-not (Test-Path $SourceRepositoryDirectoryFullPath)) { 
    throw "Parameter 'SourceRepositoryDirectory' does not point to a valid path"
}
$TargetRepositoryDirectoryFullPath = [IO.Path]::Combine($pwd, $TargetRepositoryDirectory)
if (-not (Test-Path $TargetRepositoryDirectoryFullPath)) { 
    throw "Parameter 'TargetRepositoryDirectory' does not point to a valid path"
}
if ($TargetRepositorySubdirectory) {
    $ActualMirrorTarget = "$TargetRepositoryDirectory\$TargetRepositorySubdirectory"
} else {
    $ActualMirrorTarget = $TargetRepositoryDirectory
}

if ($ExclusionPathspecsFile) {
    if ($UseExclusionsFileFromSelfRepo) {
        $ExclusionPathspecsFileFullPath = [IO.Path]::Combine($SelfRepositoryDirectoryFullPath, $ExclusionPathspecsFile)
    } else {
        $ExclusionPathspecsFileFullPath = [IO.Path]::Combine($SourceRepositoryDirectoryFullPath, $ExclusionPathspecsFile)
    }

    if (-not (Test-Path $ExclusionPathspecsFileFullPath)) { 
        throw "Parameter 'ExclusionPathspecsFile' does not point to a valid path"
    }
}

# Initialized to an empty array so the foreach below doesn't iterate once over $null.
$pathsToRestoreFromTarget = @()
if ($RestorePathspecsFile) {
    if ($UseExclusionsFileFromSelfRepo) {
        $RestorePathspecsFileFullPath = [IO.Path]::Combine($SelfRepositoryDirectoryFullPath, $RestorePathspecsFile)
    } else {
        $RestorePathspecsFileFullPath = [IO.Path]::Combine($SourceRepositoryDirectoryFullPath, $RestorePathspecsFile)
    }

    if (-not (Test-Path $RestorePathspecsFileFullPath)) {
        throw "Parameter 'RestorePathspecsFile' does not point to a valid path"
    }

    # Read the file NOW, before the exclusions step below runs `git rm` against
    # the source clone. If the file lives under a directory matched by the
    # exclusions list (e.g. `:/build/`), `git rm` would remove it from disk
    # before the post-mirror restore step gets a chance to consume it.
    $pathsToRestoreFromTarget = @(Get-Content $RestorePathspecsFileFullPath |
        Where-Object { $_ -notmatch "^#" -and -not [string]::IsNullOrWhiteSpace($_) } |
        ForEach-Object { $_.Trim() })
}

Run-LoggedCommand("md -Force $LogDirectory")

#########################################################################################
# Print repositories' information
Push-Location $SourceRepositoryDirectory
$sourceRemotes = Run-LoggedCommand("git remote -v show")
if ($SourceRepositoryCommittish -eq "HEAD") {
    # Replace HEAD with the id of the latest commit. This ensures the commit description
    # contains a useful committish rather than just "HEAD".
    $SourceRepositoryCommittish = Run-LoggedCommand("git log -n 1 --pretty=format:'%H'")
}
Write-Host "##[group]Source repository information"
Write-Host "##[debug]Committish:"
Write-Host "##[debug]$SourceRepositoryCommittish"
Write-Host "##[debug]Remote:"
Write-Host "##[debug]$sourceRemotes"
Write-Host "##[endgroup]"
Pop-Location

Push-Location $TargetRepositoryDirectory
$targetRemotes = Run-LoggedCommand("git remote -v show")
$targetRemoteName = Run-LoggedCommand("git remote")
Run-LoggedCommand("git remote set-head '$targetRemoteName' --auto")
$targetDefaultBranchWithRemote = Run-LoggedCommand("git rev-parse --abbrev-ref '$targetRemoteName/HEAD'")
$targetDefaultBranch = $targetDefaultBranchWithRemote -replace "$targetRemoteName\/(.*)",'$1'
Write-Host "##[group]Target repository information"
Write-Host "##[debug]Branch:"
Write-Host "##[debug]$TargetRepositoryBranchName"
Write-Host "##[debug]Remote:"
Write-Host "##[debug]$targetRemotes"
Write-Host "##[debug]Default branch:"
Write-Host "##[debug]$targetDefaultBranch"
Write-Host "##[endgroup]"
Pop-Location

#########################################################################################
Write-Host "##[group]Cleaning source repository clone '$SourceRepositoryDirectory'..."

Push-Location $SourceRepositoryDirectory
Run-LoggedCommand("git switch --detach '$SourceRepositoryCommittish'")
Run-LoggedCommand("git submodule deinit -f --all")
Run-LoggedCommand("git clean -xdf")
Pop-Location

Write-Host "##[endgroup]Finished cleaning source repository clone."

#########################################################################################
Write-Host "##[group]Preparing target repository clone '$TargetRepositoryDirectory'"
Push-Location $TargetRepositoryDirectory

Run-LoggedCommand("git fetch -a")
$targetBranchExists = (Run-LoggedCommand("git branch --list -r '$targetRemoteName/$TargetRepositoryBranchName' --format='%(refname:short)'")) -eq "$targetRemoteName/$TargetRepositoryBranchName"

if ($targetBranchExists) {
    Write-Host "##[debug]Target branch '$TargetRepositoryBranchName' exists."
    Write-Host "##[debug]Checking out target branch"

    Run-LoggedCommand("git submodule deinit -f --all")
    Run-LoggedCommand("git clean -xdf")
    Run-LoggedCommand("git checkout '$TargetRepositoryBranchName'")
} else {
    Write-Host "##[debug]Target branch '$TargetRepositoryBranchName' does not exist."
    Write-Host "##[debug]Creating target branch"

    Run-LoggedCommand("git submodule deinit -f --all")
    Run-LoggedCommand("git clean -xdf")
    Run-LoggedCommand("git checkout -b '$TargetRepositoryBranchName' '$targetDefaultBranchWithRemote'")
}

Pop-Location
Write-Host "##[endgroup]Finished preparing target repository clone."

#########################################################################################
# Potentially compute a more detailed commit description, using this process:
#   1. Look at the log in the target repo/branch to find the last committish used
#   2. Look through the source repo back to that commit to find the PR titles
#   3. Use those combined titles in the new commit description
#
# Future: Publish multiple times if there are multiple commits? The above at least
#         doesn't lose any titles but doesn't ensure they are all separate. Mirroring
#         multiple commits at once should be rare and not worth this extra complexity.

$commitDescription = ""
$syncContentFromCommittishText = "Syncing content from committish"

if ($ComputeCommitDescription -and $targetBranchExists) {
    Write-Host "##[group]Computing commit description"
    Write-Host "##[section]Searching target repo for previous committish to compute commit description from"

    Push-Location $TargetRepositoryDirectory
    $lastCommitWithCommittish = Run-LoggedCommand("git log --grep='$syncContentFromCommittishText' -n 1")
    Pop-Location
    $previousCommittish = ""
    if ($lastCommitWithCommittish) {
        # Search the multiple lines in the commit description for the right line
        foreach ($_ in $lastCommitWithCommittish) {
            if ($_ -match "$syncContentFromCommittishText (\w+)") {
                $previousCommittish = $matches[1]
                Write-Host "##[debug]Previous committish: $previousCommittish"
                Break
            }
        }
    }

    if ($previousCommittish -ne "") {
        Write-Host "##[section]Scanning for PR titles to add to commit description"
        Push-Location $SourceRepositoryDirectory
        $commitList = Run-LoggedCommand("git log --pretty='format:%C(auto)%H !(%an)! %s' $previousCommittish..$SourceRepositoryCommittish")
        Pop-Location
        $commitList | ForEach-Object {
            Write-Host "##[debug]Scanning commit: $_"
            if ($_ -match "!\((?<Author>[^)]*)\)! (?<Title>.*$)") {
                $prAuthor = $matches['Author']
                $prTitle = $matches['Title']
                if ($prAuthor -eq "ProductConstructionServiceProd") {
                    Write-Host "##[debug]  Ignoring automated PR."
                } else {
                    Write-Host "##[debug]  Adding to description: $prTitle"
                    $commitDescription += "$prTitle`n"
                }
            }
        }
        if ($commitDescription.Length -gt 0) {
            $commitDescription += "`n"
        }
        Write-Host "Commit description: $commitDescription"
    } else {
        Write-Host "##vso[task.logissue type=warning]Unable to find last commit with committish hash."
    }

    Write-Host "##[endgroup]"
} elseif ($ComputeCommitDescription -and -not $targetBranchExists) {
    Write-Host "##[debug]Skipping commit description computation for new target branch."
}

#########################################################################################
Write-Host "##[group]Mirroring source to target."

if ($ExclusionPathspecsFileFullPath) {
    Write-Host "##[section]Processing exclusion pathspecs."
    
    # Preprocess the pathspecs file to remove comments and blank lines
    # before handing it off to `git rm`
    (Get-Content $ExclusionPathspecsFileFullPath | Where { $_ -notmatch "^#" -and -not [string]::IsNullOrWhiteSpace($_) }) -Join "`r`n" | Set-Content -NoNewline -Path $TempDirectory\processedExclusionPathspecsFile.txt

    Push-Location $SourceRepositoryDirectory
    Run-LoggedCommand("git rm -rf --pathspec-from-file='$TempDirectory\processedExclusionPathspecsFile.txt'")
    Pop-Location
}

# Compute robocopy /xd directory exclusions. Always exclude .git. When mirroring
# into the repo root (no `src/` subdirectory), additionally exclude .github so
# that the target's GitHub Actions workflows, CODEOWNERS, issue/PR templates,
# and policies are not purged by /mir. Side-effect: source's own .github/
# content does not flow into the mirror under the root layout.
$robocopyXdArgs = ".git"
if (-not $TargetRepositorySubdirectory) {
    $robocopyXdArgs += " .github"
}

# Restore target paths from the target's index after the mirror.
# The list was populated earlier when -RestorePathspecsFile was resolved
# (see the parameter-validation block near the top of the script). The file's
# contents must be buffered there so they survive the exclusion step's
# `git rm` on the source clone, which can otherwise delete the restore-paths
# config from disk before we consume it.
#
# Paths in the file are relative to the target repo root; the loop below
# prefixes each with $TargetRepositorySubdirectory when one is set, so
# the same file works for both the legacy `src/`-layout flow and the
# root-layout flow.

Run-LoggedCommand("robocopy '$SourceRepositoryDirectory' '$ActualMirrorTarget' /mt /mir /xd $robocopyXdArgs /xf .gitattributes .gitignore .gitmodules /log:'$LogDirectory\MirrorGitRepositoryWithoutHistory_Robocopy_$env:BUILD_BUILDID.txt'")

# Run an unlogged command, to ignore error if the file doesn't exist.
# Note: "git checkout --" does not produce any output when it succeeds,
# so nothing shows in the log.
Push-Location $TargetRepositoryDirectory
foreach ($p in $pathsToRestoreFromTarget) {
    $effective = if ($TargetRepositorySubdirectory) { "$TargetRepositorySubdirectory\$p" } else { $p }
    $restoreCmd = "git checkout -- '$effective'"
    Write-Host -ForegroundColor Blue "##[command]$restoreCmd"
    Invoke-Expression $restoreCmd
}
Pop-Location

Write-Host "##[endgroup]Finished mirroring target to source."

#########################################################################################
# Log what files have changed
Write-Host "##[group]Git status -- show changed/added/deleted files"

Push-Location $TargetRepositoryDirectory
Run-LoggedCommand("git status")
Pop-Location

Write-Host "##[endgroup]Finished git status"

#########################################################################################
# And ship it!
Write-Host "##[group]Publishing the synced mirror."

Push-Location $TargetRepositoryDirectory

# If the tag already exists pointing at a commit that references this run's source
# committish, reuse it instead of creating a duplicate.
$needsNewCommit = $true
if ($TargetRepositoryTagName) {
    Run-LoggedCommand("git fetch --tags '$targetRemoteName'")

    $tagExists = (Run-LoggedCommand("git tag --list '$TargetRepositoryTagName'")).Trim() -eq $TargetRepositoryTagName
    if ($tagExists) {
        $taggedCommit = (Run-LoggedCommand("git rev-list -n 1 '$TargetRepositoryTagName'")).Trim()
        $taggedCommitMessage = (Run-LoggedCommand("git log -1 --format='%B' '$taggedCommit'")) -join "`n"
        $expectedSyncMarker = "$syncContentFromCommittishText $SourceRepositoryCommittish"

        if ($taggedCommitMessage -match [regex]::Escape($expectedSyncMarker)) {
            Write-Host "##[section]Tag '$TargetRepositoryTagName' already publishes $SourceRepositoryCommittish at $taggedCommit. Reusing it; the push below will fast-forward the branch."
            Run-LoggedCommand("git reset --hard '$taggedCommit'")
            $needsNewCommit = $false
        } else {
            throw "Refusing to overwrite tag '$TargetRepositoryTagName': it points at $taggedCommit, which doesn't match source committish $SourceRepositoryCommittish. Delete the tag manually to override."
        }
    }
}

if ($needsNewCommit) {
    # When mirroring into the repo root (no `src/` subdirectory), use `git add -f`
    # so target-side `.gitignore` patterns don't silently drop source files that
    # happen to live under directories the target ignores (e.g. NuGet convention
    # `**/packages/*`, Visual Studio `[Dd]ebug/` / `[Rr]elease/`). Under the
    # legacy `src/`-layout flow the same paths arrive as modifications to already
    # tracked files and gitignore is bypassed anyway, so the unforced form is
    # retained there to keep behaviour byte-identical with the pre-existing pipeline.
    $gitAddCmd = if ($TargetRepositorySubdirectory) { "git add --all" } else { "git add --all -f" }
    Run-LoggedCommand($gitAddCmd)
    Run-LoggedCommand("git config user.name 'reunion-maestro-bot'")
    Run-LoggedCommand("git config user.email 'reunion-maestro-bot@microsoft.com'")
    # Add in the new committish string
    $newCommittishDescription = "$syncContentFromCommittishText $SourceRepositoryCommittish"
    $commitDescription = "$commitDescription$newCommittishDescription`n"
    # Write the message to a temp file and use `git commit -F` instead of `-m '...'`.
    # Passing the message inline is fragile: Run-LoggedCommand uses Invoke-Expression,
    # and Windows PowerShell mangles embedded double quotes when handing arguments to
    # git.exe. A PR title like: Fix broken "developer guide" link would get split, so
    # git treated the trailing words as pathspecs and the commit failed with code 1.
    # Reading from a file passes the message through verbatim (quotes, newlines, etc.).
    $commitMessageFile = [IO.Path]::Combine($TempDirectory, "mirror-commit-message-$([guid]::NewGuid().ToString('N')).txt")
    [IO.File]::WriteAllText($commitMessageFile, $commitDescription, (New-Object System.Text.UTF8Encoding $false))
    # Escape single quotes so an apostrophe in the path (e.g. C:\Users\O'Connor\...)
    # can't break the single-quoted argument that Invoke-Expression parses.
    $commitMessageFileEscaped = $commitMessageFile -replace "'", "''"
    try {
        Run-LoggedCommand("git commit -q -F '$commitMessageFileEscaped'")
    } finally {
        Remove-Item -Force $commitMessageFile -ErrorAction SilentlyContinue
    }
    if ($TargetRepositoryTagName) {
        Run-LoggedCommand("git tag '$TargetRepositoryTagName'")
    }
}
# --atomic so a rejected branch ref doesn't let the tag land on its own.
Run-LoggedCommand("git push --atomic --tags -u '$targetRemoteName' HEAD")

Pop-Location

Write-Host "##[endgroup]Finished publishing the synced mirror."

#########################################################################################
Write-Host "##[group]Merge back to default branch if necessary"

Push-Location $TargetRepositoryDirectory

if ($MergeBackToDefaultBranch -eq "always") {
    Write-Host "##[debug]Merge back strategy: always"
    $shouldMergeBackToDefaultBranch = $true
} elseif ($MergeBackToDefaultBranch -eq "never") {
    Write-Host "##[debug]Merge back strategy: never"
    $shouldMergeBackToDefaultBranch = $false
} else {
    Write-Host "##[debug]Merge back strategy: automatic"
    # The automatic merge back strategy requires the target branch to be "winui3/main", since
    # we now regularly mirror the main branch rather than just the latest stable release branch.

    if ($TargetRepositoryBranchName -eq "winui3/main") {
        $shouldMergeBackToDefaultBranch = $true
    } else {
        Write-Host "##[debug]Source drop corresponds to a branch other than main. Skipping merge back."
        $shouldMergeBackToDefaultBranch = $false
    }
}

if ($shouldMergeBackToDefaultBranch) {
    Write-Host "##[warning]Merge to main is no longer supported due to branch policy to require PRs into main."
    Write-Host "##[warning]   Create a pull request to merge into main if desired."
#    # Note: Because this is a simple merge, this puts files in the same path as they are
#    # in the target branch. The means all files are under $TargetRepositorySubdirectory
#    # (which defaults to "src"), just like they are in the target branch.
#    Write-Host "##[debug]Merging back to default branch '$targetDefaultBranch'"
#    Run-LoggedCommand("git checkout '$targetDefaultBranch'")
#    Run-LoggedCommand("git merge '$TargetRepositoryBranchName'")
#    Run-LoggedCommand("git push")
}

Pop-Location

Write-Host "##[endgroup]"

exit 0

Param(
    [string]$MUXControlsRepoRoot
)

$canCreateNewBranch = $MUXControlsRepoRoot.Length -gt 0
$MUXControlsBackPortBranch = "user/uxpc/OSRepoBackPort-" + (Get-Date -Format yyyy-MM-dd-HHmmss)

# To ensure that no external changes have occurred, we'll store a hash of every file under dxaml\controls,
# and check to make sure that every hash we had previously is still the same right now.
# If at least one isn't, then we know that something has changed in the enlistment that we need to back-port
# to the dep.controls repository.
Import-Module .\HashingHelpers.psm1 -DisableNameChecking

# If the expected file doesn't exist, then we'll create it later and initialize it with the current state of the enlistment.
if (-not (Test-Path .expectedEnlistmentFileHashes))
{
    exit 0
}

Load-FileHashes .expectedEnlistmentFileHashes

[System.Collections.Generic.Dictionary[string, string]]$externalChangesByType = @{}
[System.Collections.Generic.List[string]]$filesAccountedFor = @()

Write-Host "Removing any files left over from building..."

& git clean -dfx . -e buildchk.* -e buildfre.*

Write-Host "Ensuring that no external changes to dxaml\controls have occurred since the last known checkin..."

# First, we'll check to see if any files have been changed or added.
Get-FilesToTrackChangesIn | ForEach-Object {
    $filePath = $_.FullName
    $fileSubPath = Get-FileSubPathForDictionary $filePath

    if (Get-FileHasChanged $filePath)
    {
        if ($global:fileHashDictionary.Keys.Contains($fileSubPath))
        {
            $change = "changed"
        }
        else
        {
            $change = "added"
        }

        $externalChangesByType.Add($fileSubPath, $change)
    }

    $filesAccountedFor.Add($fileSubPath)
}

# Next, we'll check to see if any files have been deleted.
$global:fileHashDictionary.Keys | ForEach-Object {
    if (-not $filesAccountedFor.Contains($_))
    {
        $externalChangesByType.Add($_, "removed")
    }
}

if ($externalChangesByType.Count -gt 0)
{
    if ($canCreateNewBranch)
    {
        $lastSyncedToCommit = (Get-Content .lastSyncedToCommit -Raw)
        $changeString = ""
    
        # In order to make back-porting external changes easier, we'll sync the MUXControls repo
        # to the last-synced-to commit, pull over all of the external changes, and create a pull request
        # that can be looked at and either approved if everything is OK, or used to see what changes
        # need to be made if not (e.g., if code-gen'd files were changed, we'd need to update code-gen
        # instead of just pulling over the files.)
        Write-Host "External changes detected! Creating a new dep.controls branch `"$MUXControlsBackPortBranch`" to hold the back-ported changes..."

        & pushd $MUXControlsRepoRoot
        & git clean -df .
        & git reset .
        & git checkout -- .
        & git checkout $MUXControlsTargetBranch
        & git pull
        & git checkout -b $MUXControlsBackPortBranch $lastSyncedToCommit
    
        Write-Host "Pulling over the external changes..."
        Write-Host

        $externalChangesByType.Keys | ForEach-Object {
            $changeType = $externalChangesByType[$_]

            if ($changeType -like "changed" -or $changeType -like "added")
            {
                Write-Host "    Copied $_."
                Copy-Item "$PSScriptRoot$_" "$MUXControlsRepoRoot$_"
            }
            elseif ($changeType -like "removed")
            {
                Write-Host "    Deleted $_."
                Remove-Item "$MUXControlsRepoRoot$_"
            }

            $changeString = "$changeString    $_ ($changeType)$([System.Environment]::NewLine)"
        }

        Write-Host

        $changeDescription = "Back-porting external changes from the OS repo."
    
        Write-Host "Committing changes..."
        & git add .
        $changeDescription | & git commit --file - 2>&1> $null

        Write-Host "Pushing changes..."
        & git push --set-upstream origin $MUXControlsBackPortBranch
        & popd

        $changeString = "$changeString$([System.Environment]::NewLine)Create a back-port pull request here: https://github.com/Microsoft/microsoft-ui-xaml/compare/master...$($MUXControlsBackPortBranch)?expand=1"
    }
    else
    {
        $externalChangesByType.Keys | ForEach-Object {
            $changeType = $externalChangesByType[$_]
            $changeString = "$changeString    $_ ($changeType)$([System.Environment]::NewLine)"
        }
    }

    Write-Error "External changes detected! Please back-port changes to the following files to the dep.controls repository:$([System.Environment]::NewLine)$([System.Environment]::NewLine)$changeString"
    exit 1
}
else
{
    Write-Host "Complete - no external changes detected."
}
# To ensure that no external changes have occurred, we'll store a hash of every file under dxaml\controls,
# and check to make sure that every hash we have right now is still the same in the future.
Import-Module .\HashingHelpers.psm1 -DisableNameChecking

Write-Host "Removing any files left over from building..."

& git clean -dfx -e buildchk.* -e buildfre.*

Write-Host "Committing the current state of dxaml\controls to disk..."

Get-FilesToTrackChangesIn | ForEach-Object {
    Save-FileHash $_.FullName
}

Commit-FileHashes .expectedEnlistmentFileHashes
Write-Host "Complete!"
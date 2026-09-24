# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

# 
# UpdateMasterFiles.ps1:
#
#   After running tests with associated masters files, the current masters and any new snapshots that don't match the masters
#   will be placed in the directory %USERPROFILE%\Pictures\XamlTAEFOutput.  Point this script at that directory (or one on your
#   dev box that you copied it into), and it will find the associated masters and update them to have the new snapshots instead.
#   Alternatively, you can download the "helixTestOutput" artifact from a lab build, extract it to a folder, and point this at
#   that folder in order to have this script update masters from a lab build.
#

Param(
    [Parameter(Mandatory = $true)]
    [string]$NewMastersDirectory,
    [switch]$release,
    [switch]$v
)

$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))

# First we'll update the MUX masters, where the new snapshots have the filename pattern of *.out.*.
$muxMasterFilesDirectory = "$projectRoot\dxaml\test\resources\masters"
$muxMasterReleaseFilesDirectory = Join-Path $muxMasterFilesDirectory "release"
$newMuxMasters = Get-ChildItem -Path "$NewMastersDirectory" -Filter *.out.* -File -Recurse

foreach ($newMaster in $newMuxMasters)
{
    # We'll find the corresponding current master file, if one exists.  Sometimes it doesn't, in the case of the
    # "DO_NOT_SUBMIT" high contrast output files.  In that case, we'll just ignore this file.
    $oldMaster = Get-ChildItem -Path $newMaster.Directory.FullName -Filter $newMaster.Name.Replace(".out.", ".master*.") -File

    if (-not $oldMaster)
    {
        continue
    }

    # Next, we'll make sure that a corresponding master file with the same name exists in the master files directory.
    $oldMasterPath = Join-Path $muxMasterFilesDirectory $oldMaster.Name
    $oldMasterReleasePath = Join-Path $muxMasterReleaseFilesDirectory $oldMaster.Name
    $target = $null

    # If in release mode, check the release dir first and then fall back to normal path
    if ($release -and [System.IO.File]::Exists($oldMasterReleasePath))
    {
        if ($v) { Write-Host "Found masters/release file, using it." }
        $target = $oldMasterReleasePath

    }
    elseif ([System.IO.File]::Exists($oldMasterPath))
    {
        $target = $oldMasterPath
    }

    # Just to help the user see what's going on, avoid the copy if the files are identical
    if ((Get-FileHash $newMaster.FullName).Hash -eq (Get-FileHash $target).Hash)
    {
        if ($v) { Write-Host "Skipping copy because files are identical: $($newMaster.FullName) == $target" }
        $target = $null
    }

    if ($target)
    {
        # Now that we know that an original master file exists, let's copy over the snapshot as the new master.
        Write-Host "Copying $($newMaster.FullName) to $target..."
        Copy-Item $newMaster.FullName $target
    }

    if ($release -and [System.IO.File]::Exists($oldMasterReleasePath) -and (Get-FileHash $oldMasterReleasePath).Hash -eq (Get-FileHash $oldMasterPath).Hash)
    {
        Write-Host "WARNING: $oldMasterReleasePath can be removed because it is identical to $oldMasterPath."
    }
}

# Next we'll update the MUXC masters, where the old snapshots have the filename pattern of *.xml.orig.
$muxcMasterFilesDirectory = "$projectRoot\controls\test\MUXControlsTestApp\verification"
$oldMuxcMasters = Get-ChildItem -Path "$NewMastersDirectory" -Filter *.xml.orig -File -Recurse

foreach ($oldMaster in $oldMuxcMasters)
{
    $newMaster = Get-ChildItem -Path $oldMaster.Directory.FullName -Filter $oldMaster.Name.Replace(".xml.orig", "*.xml") -File

    if (-not $newMaster)
    {
        continue
    }

    # Next, we'll make sure that a corresponding master file with the same name exists in the master files directory.
    $oldMasterPath = Join-Path $muxcMasterFilesDirectory ($newMaster.Name -ireplace "-[0-9]+", "")

    if (-not [System.IO.File]::Exists($oldMasterPath))
    {
        continue
    }

    # Now that we know that an original master file exists, let's copy over the snapshot as the new master.
    Write-Host "Copying $($newMaster.FullName) to $oldMasterPath..."
    Copy-Item $newMaster.FullName $oldMasterPath
}

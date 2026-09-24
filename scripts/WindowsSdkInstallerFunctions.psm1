# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

# The Debugging Tools for Windows are available only as part of the Windows 10 SDK

function Download-WindowsSdkIso(
    [parameter(mandatory)][ValidateNotNullOrEmpty()] [string] $windowsBuildNumber,
    [parameter(mandatory)][ValidateNotNullOrEmpty()] [string] $outFile)
{
    if ($windowsBuildNumber -notmatch "^\d{5,}$")
    {
        throw "'$windowsBuildNumber' doesn't look like a Windows build number"
    }

    # Static(ish) link for Windows SDK
    # Note: there is a delay from Windows SDK announcements to availability via the static link
    $uri = "https://software-download.microsoft.com/download/sg/Windows_InsiderPreview_SDK_en-us_$($windowsBuildNumber)_1.iso";

    $oldProgressPreference = $ProgressPreference
    $ProgressPreference = 'SilentlyContinue'
    try
    {
        Invoke-WebRequest -Uri $uri -OutFile $outFile
    }
    finally
    {
        $ProgressPreference = $oldProgressPreference
    }
}

function Get-ISODriveLetter(
    [parameter(mandatory)][ValidateNotNullOrEmpty()] [string] $isoPath)
{
    $diskImage = Get-DiskImage -ImagePath $isoPath
    if ($diskImage)
    {
        $volume = Get-Volume -DiskImage $diskImage

        if ($volume)
        {
            $driveLetter = $volume.DriveLetter
            if ($driveLetter)
            {
                return $driveLetter
            }
        }
    }

    return $null
}

function Mount-ISO(
    [parameter(mandatory)][ValidateNotNullOrEmpty()] [string] $isoPath)
{
    if (!(Get-DiskImage -ImagePath $isoPath).Attached)
    {
        Mount-DiskImage -ImagePath $isoPath -StorageType ISO | Out-Null
    }

    $isoDrive = Get-ISODriveLetter $isoPath
    Write-Verbose "$isoPath mounted to ${isoDrive}:"
}

function Dismount-ISO(
    [parameter(mandatory)][ValidateNotNullOrEmpty()] [string] $isoPath)
{
    if ((Get-DiskImage -ImagePath $isoPath).Attached)
    {
        Dismount-DiskImage -ImagePath $isoPath | Out-Null
        Write-Verbose "$isoPath dismounted"
    }
}

function Ensure-WindowsSDKInstaller(
    [parameter(mandatory)][ValidateNotNullOrEmpty()] [string] $windowsBuildNumber)
{
    Write-Host -NoNewLine "Preparing Windows 10 SDK ($windowsBuildNumber) installer..."

    try
    {
        # Download
        $isoPath = Join-Path "$([System.IO.Path]::GetTempPath())" "$((new-guid).Guid).iso"
        Download-WindowsSdkIso -WindowsBuildNumber $windowsBuildNumber $isoPath

        # Mount
        Mount-ISO($isoPath)

        # Return path to installer exe
        $isoDrive = Get-ISODriveLetter $isoPath

        if ($isoDrive)
        {
            Write-Host -ForegroundColor Green "OK"
            return Join-Path "${isoDrive}:" "WinSDKSetup.exe"
        }
        else
        {
            throw "Unknown error when attempting to prepare Windows 10 SDK installer"
        }
    }
    catch
    {
        Write-Host -ForegroundColor Red "FAILED"
        Write-Host -ForegroundColor Red "Error: $PSItem"
        throw
    }
}

function Cleanup-WindowsSDKInstaller(
    [parameter(mandatory)][ValidateNotNullOrEmpty()] [string] $installerPath)
{
    Write-Host -NoNewline "Cleaning up Windows 10 SDK installer..."

    try
    {
        $driveLetter = (Get-Item $installerPath).PSDrive.Name
        if ($driveLetter)
        {
            $isoPath = Get-Volume -DriveLetter $driveLetter | % { (Get-DiskImage -DevicePath $($_.Path -Replace "\\$")).ImagePath }
            if ($isoPath)
            {
                Dismount-ISO($isoPath)
                Remove-Item $isoPath
            }
        }
        Write-Host -ForegroundColor Green "OK"
    }
    catch
    {
        Write-Host -ForegroundColor Red "FAILED"
        Write-Host -ForegroundColor Red "Error: $PSItem"
        throw
    }
}

Export-ModuleMember -Function Ensure-WindowsSDKInstaller, Cleanup-WindowsSDKInstaller
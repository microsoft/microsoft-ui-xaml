# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
[CmdLetBinding()]
param(
    [string]$LocalizedFilesLocation="$PSScriptRoot\..\..\BuildOutput\Temp\LocOutput",
    [string]$PackageVersion,
    [string]$AccessToken = $env:SYSTEM_ACCESSTOKEN
)

# This script applies any changes to the localization files in the repo, 
# commits them to a new branch, pushes the branch, and opens a Pull Request.
# The function 'Copy-LocalizedFiles' copies the new .resw files returned by the localization service back into the source tree.
# The function 'Update-WinUILocalizationResourcesNuget' compares the hashes of the current and new .mui files and 
# updates the required packages.config package version if it detects changes.
# The function 'Publish-LocalizationChanges' creates a new branch, pushes it, and opens a PR.

function Copy-LocalizedFiles
{
    Param(
        [Parameter(Mandatory=$true)]
        [String] $LocalizedFilesLocation
    )

    Write-Host "Copying localized resw files back into the source tree" -ForegroundColor Green

    $repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
    $controlsPath = Join-Path $repoRoot "controls\dev\"

    # Retrieve all the english resource files in the repo
    $englishResourceFiles = Get-ChildItem -Path $controlsPath -Include "Resources.resw" -Recurse | Where-Object {$_.Directory -Match "en-us"}

    # Use ColorPicker as an example in order to extract the list of available languages.
    # Due to how the localization service currently returns files,
    # all the languages are currently under the path ..\Strings\en-us\.. (very confusing).
    $languages = get-childitem "$LocalizedFilesLocation\controls\dev\ColorPicker\Strings\en-us" | Where-Object { $_.Name -match "-" } | % { $_.Name }

    foreach ($language in $languages)
    {
        Write-Verbose "Current language: $language"
        foreach ($file in $englishResourceFiles)
        {
            $destFilePath = $file.FullName -ireplace "en-us",$language

            # Extract control name (directory name under ..\dev\)
            $endPath = $file.FullName.substring($controlsPath.Length)
            $controlName = ($endPath -split '\\')[0]

            $fileName = Split-Path -Leaf $destFilePath

            $destFileLocation = Split-Path -Parent $destFilePath

            $sourceLocation = "$LocalizedFilesLocation\controls\dev\$controlName\Strings\en-us\$language\$fileName"
            Write-Verbose "Dest: $destFileLocation Source: $sourceLocation"

            if (-not (Test-Path $sourceLocation)) 
            { 
                Write-Host "##vso[task.logissue type=error;]File does not exist: $sourceLocation"
                Exit 1;
            }
            else
            {
                if (-not (Test-Path $destFileLocation)) { mkdir $destFileLocation }
                Copy-Item $sourceLocation $destFileLocation
            }
        }
    }
    Write-Host 

    Write-Host "Done copying localized files back into the source tree." -ForegroundColor Green
}

function Update-WinUILocalizationResourcesNuget
{
    Param(
        [Parameter(Mandatory=$true)]
        [String] $LocalizedFilesLocation,
        [Parameter(Mandatory=$true)]
        [String] $PackageVersion
    )

    Write-Host "Checking MUI DLLs."

    $repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

    # Get current version of the localization resources NuGet package
    $packagesConfigFilePath = Join-Path $repoRoot "packages.config"
    $xmldoc = [System.Xml.XmlDocument](Get-Content $packagesConfigFilePath)
    $packageElement = $xmlDoc.SelectSingleNode('//package[@id="Microsoft.Internal.WinUILocalizationResources"]')
    $versionNumber = $packageElement.GetAttribute('version')
    
    Write-Host "Found Version Number: $versionNumber"

    if($versionNumber)
    {
        $currentMUILocation = Join-Path $repoRoot "packages\Microsoft.Internal.WinUILocalizationResources.$versionNumber\LocalizedResources\BuildOutput\Temp\localizationResources"

        $newMUILocation = "$LocalizedFilesLocation\BuildOutput\Temp\localizationResources\"

        Write-Verbose "Current Location: $currentMUILocation"
        Write-Verbose "New Location: $newMUILocation"

        # Extract the list of available languages
        $languages = get-childitem "$newMUILocation" | Where-Object { $_.Name -match "-" } | % { $_.Name }

        # Compare each MUI dll in order to detect if there has been a change
        $updatePackageVersion = $false
        foreach ($language in $languages)
        {
            Write-Verbose "Current language: $language"

            $currentMUXLocation = "$currentMUILocation\$language\Microsoft.ui.xaml.dll.mui"
            $currentMUXPhoneLocation = "$currentMUILocation\$language\Microsoft.UI.Xaml.Phone.dll.mui"

            # Check if MUI files do not exist for locale
            if ((-not (Test-Path $currentMUXLocation)) -or (-not (Test-Path $currentMUXPhoneLocation))) 
            { 
                Write-Host "MUI files do not exist." -ForegroundColor Yellow
                $updatePackageVersion = $true
                break
            }

            $newMUXLocation = "$newMUILocation\$language\Microsoft.ui.xaml.dll.mui"
            $newMUXPhoneLocation = "$newMUILocation\$language\Microsoft.UI.Xaml.Phone.dll.mui"

            $hashNewMUX = Get-FileHash -Algorithm MD5 -Path $newMUXLocation
            $hashNewMUXPhone = Get-FileHash -Algorithm MD5 -Path $newMUXPhoneLocation

            $hashCurrentMUX = Get-FileHash -Algorithm MD5 -Path $currentMUXLocation
            $hashCurrentMUXPhone = Get-FileHash -Algorithm MD5 -Path $currentMUXPhoneLocation

            # Compare the two hashes
            if (($hashNewMUX.Hash -ne $hashCurrentMUX.Hash) -or ($hashNewMUXPhone.Hash -ne $hashCurrentMUXPhone.Hash)) 
            {
                Write-Host "Hashes are not the same." -ForegroundColor Yellow
                Write-Host "New Microsoft.ui.xaml.dll.mui location: $newMUXLocation" -ForegroundColor Yellow
                Write-Host "Current Microsoft.ui.xaml.dll.mui location: $currentMUXLocation" -ForegroundColor Yellow
                Write-Host "New Microsoft.UI.Xaml.Phone.dll.mui location: $newMUXPhoneLocation" -ForegroundColor Yellow
                Write-Host "Current Microsoft.UI.Xaml.Phone.dll.mui location: $currentMUXPhoneLocation" -ForegroundColor Yellow
                $updatePackageVersion = $true
                break
            }
        }

        # If we have detected a change in the mui dll, we want to update the package version
        if ($updatePackageVersion)
        {
            $packageElement.SetAttribute('version', $PackageVersion)

            $xmlDoc.Save($packagesConfigFilePath)

            Write-Host "Updated packages.config to version: $PackageVersion" -ForegroundColor Yellow
        }
        else
        {
            Write-Host "MUI DLLs are upto date." -ForegroundColor Green
        }
    }
    else
    {
        Write-Host "##vso[task.logissue type=error;]Microsoft.Internal.WinUILocalizationResources version number not found in packages.config."
        Exit 1;
    }

    Write-Host "Finished checking MUI DLLs."
}

function Publish-LocalizationChanges
{
    Write-Host "Pushing changes if required."

    $repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

    $changes = & git ls-files -m
    Write-Host "Edited files:"
    Write-Host "$changes"
    if ($changes)
    {
        Write-Host "##vso[task.logissue type=warning;]Localization updates needed"
        foreach ($change in $changes)
        {
            Write-Host "$change"
        }

        $currentDateTime = Get-Date
        $dateTimeString = $currentDateTime.ToString("yyyyMMdd")
        $branchName = "LocalizationUpdate" + $dateTimeString

        & git config --global user.email "DoNotEmailThis@dev.null.microsoft.com"
        & git config --global user.name "reunion-maestro-bot"

        Write-Host "Creating new branch: $branchName"
        & git checkout -b "$branchName"

        Write-Host "Commiting changes."
        & git add "controls/*"
        & git add "packages.config"
        & git commit -m "Update Localized Resource Files"

        Write-Host "Pushing branch."
        $header = "AUTHORIZATION: bearer $AccessToken"
        & git -c http.extraheader="$header" push --set-upstream origin "$branchName"

        # Check the exit status
        if ($?) 
        {
            Write-Host "Git push succeeded."
        } 
        else 
        {
            Write-Host "Git push failed!" -ForegroundColor Red
            Exit 1
        }

        Write-Host "Creating PR."
        $createPRScript = Join-Path $repoRoot "scripts\createPullRequest.ps1"
        & $createPRScript `
            -AzureDevOpsPat $AccessToken `
            -branch $branchName `
            -title "Localization Update $dateTimeString" `
            -description "This is an automated pull request to update localization assets. `
            This PR is opened by the script build\PipelineScripts\ApplyLocalizationUpdates.ps1. `
            For more info please refer to docs\localization-process.md."
    }
    else
    {
        Write-Host "No changes required." -ForegroundColor Green
    }
}

if (-not (Test-Path "$LocalizedFilesLocation"))
{
    Write-Host "Could not find LocalizationDrop folder. Either specify using -LocalizedFilesLocation or place in the BuildOutput folder." -ForegroundColor Red
    Exit 1
}

Copy-LocalizedFiles -LocalizedFilesLocation $LocalizedFilesLocation

if ($PackageVersion)
{
    Update-WinUILocalizationResourcesNuget -LocalizedFilesLocation $LocalizedFilesLocation -PackageVersion $PackageVersion
}
else
{
    Write-Host "Package version for new Microsoft.Internal.WinUILocalizationResources was not provided!" -ForegroundColor Red
    Exit 1
}

Publish-LocalizationChanges

Write-Host "Done!" -ForegroundColor Green
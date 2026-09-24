# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

# This script is used to repackage an existing AppX package containing WinUI 3 binaries, replacing the package binaries
# with the ones locally built from your branch. This can be used to deploy local binaries to an app in order to debug issues or
# test changes that you've made. The script will generate an .msix file, which then can be used to install a modified version
# of the AppX package, after you've uninstalled the existing AppX package.
#
# Note that the repackaged package is signed using the locally generated build\WinUITest.cer - you'll need to install
# that certificate to "Trusted People" on a machine that you're trying to install the repackaged package.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PackageFullName,

    [ValidateSet("x86", "x64", "arm64")]
    [string]$Platform = "$env:BUILDPLATFORM",

    [ValidateSet("chk", "fre")]
    [string]$Configuration = "$env:_BuildType",

    [switch]$Clean,

    [switch]$Force
)

$packageTempPath = Join-Path $env:TEMP $PackageFullName

# We'll copy the package contents to a temp staging path from which we'll then repackage things.
if ($Clean -and (Test-Path $packageTempPath))
{
    Remove-Item $packageTempPath -Recurse -Force
}

if (-not (Test-Path $packageTempPath))
{
    $installedAppxPackage = Get-AppxPackage | Where-Object -Property PackageFullName -eq $PackageFullName | Select-Object -First 1

    if (-not $installedAppxPackage)
    {
        Write-Error "Could not find installed AppX package with the package full name `"$PackageFullName`"."
        exit 1
    }

    $installedAppxPackageLocation = $installedAppxPackage.InstallLocation
    $muxPath = Get-ChildItem $installedAppxPackageLocation -Filter Microsoft.UI.Xaml.dll | Select-Object -First 1

    # We'll first check whether this package contains an instance of WinUI 3.
    if (-not $muxPath)
    {
        # If there isn't one, then this might be an AppX package that references the framework package.
        # If so, we'll report as much via a helpful error message.
        $frameworkPackageDependency = $installedAppxPackage.Dependencies | Where-Object { $_.PackageFullName.Contains("Microsoft.WindowsAppRuntime") } | Select-Object -First 1

        if ($frameworkPackageDependency)
        {
            Write-Error @"
The package $PackageFullName does not contain an instance of WinUI 3 binaries. However, it does contain a dependency on the following WindowsAppSDK framework package:

    $($frameworkPackageDependency.PackageFullName)

If you want to replace WinUI 3 binaries that are used by this package, you can repackage the above framework package and install that as a replacement instead.
"@
        }
        else
        {
            Write-Error "The package $PackageFullName does not contain an instance of WinUI 3 binaries, and no WindowsAppSDK framework package dependencies were detected, either."
        }

        exit 1
    }

    # Later versions of WinUI 3 store the WinUI 3 commit ID in the Microsoft.UI.Xaml.dll version string.
    # If this is one that does, we'll check that commit ID and raise an alert if the current repo
    # does not contain that commit ID in its past. If this is one that doesn't, we'll raise an alert
    # about that fact instead.
    $repoCommitId = [System.Text.RegularExpressions.Regex]::Match($muxPath.VersionInfo.FileVersion, "[0-9a-z]{40}").Value

    if (-not $repoCommitId -and -not $Force)
    {
        Write-Warning @"
The version of WinUI 3 in this package is old enough that it does not contain information about what WinUI 3 commit ID it was packaged from. As a result, binaries built from this branch cannot be guaranteed to be compatible with this package.

Hit enter to continue packaging, or Ctrl+C to abort.

Use the -Force parameter to suppress this warning.
"@

        Read-Host
    }

    # We'll use the presence of an asterisk in the "git branch" output as a proxy for the question of whether or not
    # the current branch is one that contains the commit ID in question.
    $currentBranchContainsCommit = (& git branch --contains $repoCommitId | Out-String).Contains("*")

    if (-not $currentBranchContainsCommit -and -not $Force)
    {
        Write-Warning @"
The version of WinUI 3 in this package contains a WinUI 3 commit not present in the current branch. As a result, binaries built from this branch cannot be guaranteed to be compatible with this package.

To ensure compatibility, rebase to the following commit ID:

    $repoCommitId

Hit enter to continue packaging, or Ctrl+C to abort.

Use the -Force parameter to suppress this warning.
"@

        Read-Host
    }
    
    Write-Host "Staging path $packageTempPath does not exist. Copying package contents from $installedAppxPackageLocation to $packageTempPath..."
    New-Item -ItemType Directory $packageTempPath | Out-Null
    & robocopy $installedAppxPackageLocation $packageTempPath * /S

    # We need to have the publisher information in the manifest match the test-signing cert we'll use to sign the repackaged package,
    # so we'll edit the manifest accordingly.
    $appxManifestPath = Join-Path $packageTempPath "AppxManifest.xml"

    Write-Host
    Write-Host "Updating publisher information in $appxManifestPath..."
    
    $appxManifestXml = [xml](Get-Content -Raw $appxManifestPath)
    $appxManifestXml.Package.Identity.Publisher = "CN=WinUITest"
    $appxManifestXml.Package.Properties.PublisherDisplayName = "WinUITest"
    $appxManifestXml.Save($appxManifestPath)
}

if ($Platform -eq "x64")
{
    $Arch = "amd64"
}
else
{
    $Arch = $Platform
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$binPath = "$repoRoot\BuildOutput\bin\$Arch$Configuration"

$muxDirectory = [System.IO.Path]::GetDirectoryName((Get-ChildItem $packageTempPath -Filter Microsoft.UI.Xaml.dll | Select-Object -First 1).FullName)

Write-Host
Write-Host "Copying the latest built binaries from $binPath to $muxDirectory..."

& robocopy "$binPath\Product" "$muxDirectory" *.dll
& robocopy "$binPath\Product" "$muxDirectory" *.winmd
& robocopy "$binPath\Product\en-US" "$muxDirectory\en-US" *.dll.mui
& robocopy "$binPath\test\private" "$muxDirectory" Microsoft.WinUI.dll

$msixPath = Join-Path $env:TEMP "$PackageFullName.msix"
$sdkPath = Join-Path $env:WindowsSdkVerBinPath "x64"
$makeappxPath = Join-Path $sdkPath "makeappx.exe"
$signtoolPath = Join-Path $sdkPath "signtool.exe"

Write-Host
Write-Host "Repackaging AppX package to $msixPath..."
& "$makeappxPath" pack /p "$msixPath" /d "$packageTempPath" /o

$testPfxPath = "$repoRoot\build\WinUITest.pfx"

Write-Host
Write-Host "Signing $msixPath using $testPfxPath..."
& "$signtoolPath" sign /fd SHA256 /a /f "$testPfxPath" "$msixPath"

Write-Host @"

Repackaged at

    $msixPath
    
Uninstall the existing AppX package and then install from that path to install a version of the package with your locally built WinUI 3 binaries.
"@
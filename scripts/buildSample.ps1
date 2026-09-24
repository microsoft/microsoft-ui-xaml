# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

<#
.SYNOPSIS

This scripts builds a sample app, such as WinUIGallery, using either 
local build output, or a WinUI component package or published WindowsAppSDK package.

.DESCRIPTION 

Use the -experimental flag if building for an Experimental branch
(sets the finalRelease build flag to false)

For example:

buildSample WinUIGallery 1.1.1
buildSample WinUIGallery 1.1.1 -unpackaged -experimental -selfcontained -dbg

#> 


param(
    [Parameter(Mandatory=$true)] $sampleName,
    [Parameter(Mandatory=$true)] $windowsAppSdkVersion = $null,
    [switch] $dbg = $false,
    [switch] $unpackaged = $false,
    [switch] $selfcontained = $false,
    [switch] $experimental = $false)


if($env:repoRoot -eq $null)
{
    write-error "Must be in an init'd repo"
    exit
}

$arch = $env:BUILDPLATFORM

$configuration = if($dbg) { "Debug" } else { "Release" }

nuget install Microsoft.WindowsAppSDK -Version $windowsAppSdkVersion

# Build the solution with the `WindowsAppSdkPackageVersion` property set.
# The reason for having to specify multiple package versions is due to how vcxproj handles nuget packages
# (you must explictly refer to each package; vcxproj does not pull in package dependencies automatically).
if($experimental ) 
{
    $muxFinalRelease = "/p:MUXFinalRelease=false"
} 
else 
{
    $muxFinalRelease = "/p:MUXFinalRelease=true"
}

if($selfcontained)
{
    $containedFlag = "/p:WindowsAppSDKSelfContained=true"
}

if($unpackaged)
{
    $packageType = "/p:WindowsPackageType=None"
}
else
{
    $packageType = "/p:WindowsPackageType=MSIX"
    $publish = "/t:Publish"
}

if($sampleName -eq "WinUIGallery")
{
    $sampleDir= "WinUIGallery"
    $publishProfile = "win-$arch.pubxml"
    if($configuration -eq "Release")
    {
        # PublishAot must be specified at restore, as it conditionally generates nuget.g.* imports
        $publish = "/p:PublishAot=true $publish"
    }
    if($unpackaged)
    {
        $configuration = "$configuration-unpackaged"
    }
}
else
{
    $sampleDir = $sampleName
    $publishProfile = "win10-$arch.pubxml"
}

$baseCommand = "msbuild.exe /p:platform=$arch /p:configuration=$configuration /restore /p:WindowsAppSdkPackageVersion=$windowsAppSdkVersion /p:UseStandalone=true /p:PublishProfile=$publishProfile $publish $muxFinalRelease $containedFlag"

function RunBuild
{
    Param(
        [string] $SolutionName
    )

    $file="$env:repoRoot\Samples\$sampleDir\$SolutionName.sln"

    if(-not (Test-Path -Path $file -PathType Leaf))
    {
        $file="$env:repoRoot\Samples\$sampleDir\$SolutionName.slnx"
    }

    if(Test-Path -Path $file -PathType Leaf)
    {
        $command = "$baseCommand $packageType $file /bl:Samples-$SolutionName-$arch-$configuration.binlog"

        write-output ""
        write-output $command
        invoke-expression $command

        if($LASTEXITCODE -ne 0)
        {
            write-output "$file build failed with exit code: $LASTEXITCODE"
            exit $LASTEXITCODE
        }
    }
}

# Build the sample exactly as requested
RunBuild -SolutionName $sampleName

# For packaged, build the WAP version if it exists
if(!$unpackaged)
{
    $packageType = ""
    RunBuild -SolutionName "$sampleName.DesktopWAP"
}

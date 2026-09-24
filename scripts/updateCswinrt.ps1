# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

#
# Update the files in the Xaml repo to reference a version of CsWinRT
#

Param(
    [Parameter(Position=0)] 
    [string]$version = ""
)

if($version -eq "" -or $version -eq "/?")
{
    write-host "Usage:   UpdateCswinrt [version]"
    write-host "Example: UpdateCswinrt 1.3.3"
    exit 1
}

# Get the root of the repo.
$scriptFullPath =  (split-path -parent $MyInvocation.MyCommand.Definition) 
$rootPath = (split-path -parent $scriptFullPath)

Function CheckFile($filename)
{
    if(-not (Test-Path $filename))
    {
        write-host "File not found: $filename"
        exit 1
    }
}

# update CsWinRT Version in ~\packages.config
$filename = "$rootPath\packages.config"
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename)
$($xmldoc.packages.package | ? {$_.id.EndsWith("CsWinRT")}).version = $version
$xmldoc.Save($filename)
write-host "Updated $filename"

$filename = "$rootPath\perf\scenarios\build\PackageVersions.props"
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename)
$xmldoc.Project.PropertyGroup.CsWinRTVersion = $version
$xmldoc.Save($filename)
write-host "Updated $filename"   

# update CsWinRT Version in ~\eng\versions.props
$filename = "$rootPath\eng\versions.props"
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename)
$xmldoc.Project.PropertyGroup.MicrosoftCsWinRTPackageVersion = $version
$xmldoc.Save($filename)
write-host "Updated $filename"

# update CsWinRT version in WinUIGallery's standalone.props 
$filename = "$rootPath\Samples\WinUIGallery\standalone.props"
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename)
$xmldoc.Project.PropertyGroup.MicrosoftCsWinRTPackageVersion = $version
$xmldoc.Save($filename)
write-host "Updated $filename"
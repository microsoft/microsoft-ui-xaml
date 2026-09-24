# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

#
# Update the repo to reference a specific version of the IXP package.
#

Param(
    [Parameter(Position=0)] 
    [string]$version = "",
    [string]$onlyArch = ""
)

if($version -eq "" -or $version -eq "/?")
{
    write-host "Usage:   UpdateIxp [version]"
    write-host "Example: UpdateIxp 0.1.1-CI-22390.1000.210524-1000.0"
    exit 1
}

# Get the root of the repo.
$scriptFileName = $MyInvocation.MyCommand.Name
$scriptFullPath =  (split-path -parent $MyInvocation.MyCommand.Definition) 
$rootPath = (split-path -parent $scriptFullPath)

Function OpenXml($filename)
{
    if(-not (Test-Path $filename))
    {
        write-host "File not found: $filename"
        exit 1
    }

    # Load the file with whitespace preserved or we'll get git commit warnings about converting LF to CRLF
    $xmldoc = new-object System.Xml.XmlDocument
    $xmldoc.PreserveWhitespace = $true
    $xmldoc.Load($filename)
    return $xmldoc
}


#update IXP package version in eng\Version.Details.Xml
$filename = "$rootPath\eng\Version.Details.Xml"
$xmldoc = OpenXml $filename

$($xmldoc.Dependencies.ProductDependencies.Dependency | Where-Object {$_.Name.EndsWith("Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage")}).version = $version
$xmldoc.Save($filename)
write-host "Updated $filename"

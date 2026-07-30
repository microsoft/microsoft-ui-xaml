# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

#
# Update the files in the WinUI repo to reference 
# new versions of CoreWebView2 SDK and WebView2 Runtime
#

Param(
    [Parameter(Position=0)] 
    [string]$sdkVersion = "",
    [Parameter(Position=1)]
    [string]$browserVersion = ""
)

if($sdkVersion -eq "/?" -or $browserVersion -eq "" -or $browserVersion -eq "")
{
    write-host "Usage:   UpdateWebView2 [sdkVersion] [browserVersion]"
    write-host "Example: UpdateWebView2 1.0.721-prerelease 88.0.705.0"
    exit 1
}

# Get the root of the repo.
$scriptFileName = $MyInvocation.MyCommand.Name
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

#
# Update SDK Version
#

$filename = "$rootPath\eng\versions.props"
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename)
foreach ($propGroup in $xmldoc.Project.PropertyGroup)
{
    if ($propGroup.WebView2Version)
    {
        $propGroup.WebView2Version = $sdkVersion
    }
}
$xmldoc.Save($filename)
write-host "Updated $filename"

$filename = "$rootPath\controls\dev\dll\packages.config"
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename)
$($xmldoc.packages.package | ? {$_.id.EndsWith("WebView2")}).version = $sdkVersion
$xmldoc.Save($filename)
write-host "Updated $filename"

$filename = "$rootPath\controls\test\TestAppCX\packages.config"
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename)
$($xmldoc.packages.package | ? {$_.id.EndsWith("WebView2")}).version = $sdkVersion
$xmldoc.Save($filename)
write-host "Updated $filename"

#
# Update Browser Version
#

$filename = "$rootPath\packages.config"
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename)
$($xmldoc.packages.package | ? {$_.id.EndsWith("DCPP.Dependencies.Edge")}).version = $browserVersion
$xmldoc.Save($filename)
write-host "Updated $filename"

$filename = "$rootPath\dxaml\test\infra\taefhostappmanaged\TaefHostAppManaged.csproj"
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename)

$($xmldoc.Project.Target.ItemGroup[1].BinplaceItem | ? {$_.Include.Contains("DCPP.Dependencies.Edge")}).Include = "`$(NugetPackageDirectory)\Microsoft.UI.DCPP.Dependencies.Edge.$browserVersion\edge\mini_installer\`$(Platform)\mini_installer.exe"
$xmldoc.Save($filename)
write-host "Updated $filename"

$nuspecPath = "$rootPath\dxaml\test\external\Microsoft.UI.DCPP.Dependencies.Edge.nuspec"
$filename = $nuspecPath
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename -Encoding UTF8)
$xmldoc.package.metadata.version = $browserVersion
$xmldoc.package.files.file.src = "..\test\edge\$browserVersion\**"
$xmldoc.Save($filename)
write-host "Updated $filename"
write-host ""

#
# Create directories for mini_installers
#

$scriptDirectory = $script:MyInvocation.MyCommand.Path | Split-Path -Parent
$testPath = Resolve-Path -Path (Join-Path "$scriptDirectory\.." "dxaml\test")
$edgePath = "$testPath\edge"
if (!(Test-Path $edgePath)) { mkdir $edgePath | out-null }
$browserPath = "$edgePath\$browserVersion"
if (!(Test-Path $browserPath)) { mkdir $browserPath | out-null }
$x86Path = "$browserPath\x86"
$x64Path = "$browserPath\x64"
if (!(Test-Path $x86Path)) { mkdir $x86Path | out-null }
write-host "Created $x86Path"
if (!(Test-Path $x64Path)) { mkdir $x64Path | out-null }
write-host "Created $x64Path"
write-host ""

#
# Next steps
#

write-host "Next steps:"
write-host "1. Copy installers into paths above"
write-host "2. Run `"nuget pack $nuspecPath -OutputDirectory $rootPath\packages`""
write-host "3. Run `"nuget push $rootPath\packages\Microsoft.UI.DCPP.Dependencies.Edge.$browserVersion.nupkg -Source WinUI.Dependencies -apikey <api_key>`""
write-host "4. Run `"nuget restore build\packages.webview2.config`" to ensure the package gets pulled down correctly"

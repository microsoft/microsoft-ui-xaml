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
    write-host "Example: UpdateWebView2 1.0.3537.50 152.0.4191.66"
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

$nuspecPath = "$rootPath\dxaml\test\external\Microsoft.UI.DCPP.Dependencies.Edge.nuspec"
$filename = $nuspecPath
CheckFile $filename
$xmldoc = [System.Xml.XmlDocument](Get-Content $filename -Encoding UTF8)
$xmldoc.package.metadata.version = $browserVersion
$xmldoc.package.files.file.src = "..\..\test\edge\$browserVersion\x64\MicrosoftEdgeWebView2RuntimeInstallerX64.exe"
$xmldoc.Save($filename)
write-host "Updated $filename"
write-host ""

#
# Create the temporary directory for the x64 Evergreen Standalone Installer
#

$scriptDirectory = $script:MyInvocation.MyCommand.Path | Split-Path -Parent
$testPath = Resolve-Path -Path (Join-Path "$scriptDirectory\.." "dxaml\test")
$edgePath = "$testPath\edge"
if (!(Test-Path $edgePath)) { mkdir $edgePath | out-null }
$browserPath = "$edgePath\$browserVersion"
if (!(Test-Path $browserPath)) { mkdir $browserPath | out-null }
$x64Path = "$browserPath\x64"
if (!(Test-Path $x64Path)) { mkdir $x64Path | out-null }
write-host "Created $x64Path"
write-host ""

#
# Next steps
#

$installerPath = "$x64Path\MicrosoftEdgeWebView2RuntimeInstallerX64.exe"
write-host "Next steps:"
write-host "1. Download the official x64 Evergreen Standalone Installer to $installerPath"
write-host "2. Verify its Microsoft signature, SHA-256, and size as documented in controls\dev\WebView2\WebView2-update.md"
write-host "3. Run `"nuget pack $nuspecPath -OutputDirectory $rootPath\packages`""
write-host "4. Set `$packagePath to the exact .nupkg path printed by nuget pack; NuGet may normalize a trailing .0 from the filename"
write-host "5. Obtain the internal and shine-oss feed URLs from Key Vault as documented in controls\dev\WebView2\WebView2-update.md"
write-host "6. After approval, push the same package to both feeds using those Key Vault values"

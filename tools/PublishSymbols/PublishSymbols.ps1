Push-Location $PSScriptRoot

[xml]$customProps = (Get-Content ..\..\custom.props)
$versionMajor = $customProps.GetElementsByTagName("VersionMajor").'#text'
$versionMinor = $customProps.GetElementsByTagName("VersionMinor").'#text'

if ((!$versionMajor) -or (!$versionMinor))
{
    Write-Error "Expected VersionMajor and VersionMinor tags to be in custom.props file"
    Exit 1
}

$buildVersion = $versionMajor + "." + $versionMinor + "." + $env:BUILD_BUILDNUMBER

Write-Host "Build = $buildVersion"

$buildId=$env:BUILD_BUILDNUMBER + "_" + $env:BUILDCONFIGURATION
$directory=$env:BUILD_BINARIESDIRECTORY + "\" + $env:BUILDCONFIGURATION + "\" + $env:BUILDPLATFORM + "\Microsoft.UI.Xaml"

Write-Host "buildId = $buildId"
Write-Host "directory = $directory"

Copy-Item pdb_index_template.ini pdb_index.ini
Add-Content pdb_index.ini "Build=$buildVersion"

\\symbols\Tools\createrequest.cmd -i .\pdb_index.ini -d .\requests -c -a -b $buildId -e Release -g $directory

if ($lastexitcode -ne 0)
{
    Exit $lastexitcode;
}

Pop-Location
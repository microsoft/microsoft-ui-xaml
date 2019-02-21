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

$buildId="$($env:BUILD_BUILDNUMBER)_$($env:BUILDCONFIGURATION)_$($env:BUILDPLATFORM)"
$localDirectory=$env:BUILD_BINARIESDIRECTORY + "\" + $env:BUILDCONFIGURATION + "\" + $env:BUILDPLATFORM + "\Microsoft.UI.Xaml"
$directory = "$env:XES_DFSDROP\$env:XES_RELATIVEOUTPUTROOT\Microsoft.UI.Xaml"

Write-Host "Local path: '$localDirectory'"
Write-Host "Build share: '$directory'"

Copy-Item -Recurse "$localDirectory" "$directory"

Write-Host "buildId = $buildId"

Copy-Item pdb_index_template.ini pdb_index.ini
Add-Content pdb_index.ini "Build=$buildVersion"

\\symbols\Tools\createrequest.cmd -i .\pdb_index.ini -d .\requests -c -a -b $buildId -e Release -g $directory

if ($lastexitcode -ne 0)
{
    Exit $lastexitcode;
}

Pop-Location
[CmdLetBinding()]
Param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$localDirectory
)

Push-Location $PSScriptRoot

[xml]$customProps = (Get-Content ..\..\version.props)
$versionMajor = $customProps.GetElementsByTagName("MUXVersionMajor").'#text'
$versionMinor = $customProps.GetElementsByTagName("MUXVersionMinor").'#text'
$versionPatch = $customProps.GetElementsByTagName("MUXVersionPatch").'#text'

if ((!$versionMajor) -or (!$versionMinor) -or (!$versionPatch))
{
    Write-Error "Expected MUXVersionMajor, MUXVersionMinor, and MUXVersionPatch tags to be in version.props file"
    Exit 1
}

$buildVersion = "$versionMajor.$versionMinor.$versionPatch.$env:BUILD_BUILDNUMBER"

Write-Host "Build = $buildVersion"

$buildId="$($env:BUILD_BUILDNUMBER)"
$directory = "$env:XES_DFSDROP"

Write-Host "Local path: '$localDirectory'"
Write-Host "Build share: '$directory'"

Copy-Item -Recurse -Verbose "$localDirectory" "$directory"

Write-Host "buildId = $buildId"

Copy-Item pdb_index_template.ini pdb_index.ini
Add-Content pdb_index.ini "Build=$buildVersion"

\\symbols\Tools\createrequest.cmd -i .\pdb_index.ini -d .\requests -c -a -b $buildId -e Release -g $directory -r

if ($lastexitcode -ne 0)
{
    Exit $lastexitcode;
}

Pop-Location
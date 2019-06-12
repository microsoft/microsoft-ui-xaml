<#
.SYNOPSIS
Use this script to create a VPack that can be pushed to the OS.
The input to this script should be a non-prerelease .nupkg containing the MUX Framework Packages.
This script does not push the vpack, but gives the required command to do so.
#>
[CmdLetBinding()]
Param(
    [string]$NugetPackageInputPath,
    [string]$VPackDirectoryOutputPath
    )

$nupkgFileNameWithoutExtension = (Get-Item $NugetPackageInputPath).Basename

if(!$nupkgFileNameWithoutExtension.StartsWith("Microsoft.UI.Xaml.","CurrentCultureIgnoreCase"))
{
    Write-Error "Expected input nupkg file to start with 'Microsoft.UI.Xaml'"
    exit 1
}

$ver = $nupkgFileNameWithoutExtension -replace "Microsoft.UI.Xaml."
$verParts = $ver.Split(".")
if($verParts.Count -ne 3)
{
    Write-Error "Expected input nupkg file to have 3-place version number"
    exit 1
}
$verMajor = $verParts[0]
$verMinor = $verParts[1]
$verPatch = $verParts[2]
Write-Verbose "Version = '$verMajor.$verMinor.$verPatch'"

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::OpenRead($NugetPackageInputPath)

function New-TemporaryDirectory {
    $parent = [System.IO.Path]::GetTempPath()
    $name = [System.IO.Path]::GetRandomFileName()
    New-Item -ItemType Directory -Path (Join-Path $parent $name)
}

$tempDir = New-TemporaryDirectory
$nugetUnpacked = $tempDir.FullName
Write-Verbose "Temp Nuget directory: $nugetUnpacked"

[System.IO.Compression.ZipFileExtensions]::ExtractToDirectory($archive, $nugetUnpacked)

$outputDirName = "Microsoft.UI.Xaml"
$outputPath = Join-Path $VPackDirectoryOutputPath $outputDirName

if(Test-Path $outputPath)
{
    Write-Error "The path '$outputPath' already exists. Please delete the old '$outputDirName' directory before running this script."
    exit 1
}

New-Item -ItemType Directory -Force -Path $outputPath

$flavors = @("x86", "x64", "arm", "arm64")
foreach ($flavor in $flavors)
{
    $sourcePathDir = Join-Path $nugetUnpacked "tools\AppX\$flavor\Release\"

    $search = "Microsoft.UI.Xaml.*.appx"
    $found = Get-ChildItem $sourcePathDir -Filter $search
    if ($found.Length -eq 0)
    {
        Write-Error "Could not find '$search' in '$sourcePathDir'"
        Exit 1
    }

    $fileName = $found[0].Name
    $sourcePathFull = $found[0].FullName
    
    $destPathDir = Join-Path $outputPath $flavor
    $destPathFull = Join-Path $destPathDir $fileName

    Write-Verbose "Create directory '$destPathDir'"
    New-Item -ItemType Directory -Force -Path $destPathDir

    Write-Verbose "Copy item from '$sourcePathFull' to '$destPathFull' "
    Copy-Item $sourcePathFull $destPathFull
}

Write-Verbose "Removing temp dir '$nugetUnpacked'"
Remove-Item -Force -Recurse $nugetUnpacked

Write-Host "Push this vpack with the following command:"
Write-Host "VPack.exe push /Name:Microsoft.UI.Xaml /SourceDirectory:$outputPath /VersionIncrementType:None /Major:$verMajor /Minor:$verMinor /Patch:$verPatch"
Write-Host "Then update %SDXROOT%/build/Config/OSDependencies.Manifest in the OS repo to consume this new version."
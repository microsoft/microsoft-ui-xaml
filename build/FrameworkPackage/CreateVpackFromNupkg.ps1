<#
.SYNOPSIS
Use this script to create a VPack that can be pushed to the OS.
The input to this script can be either a prerelease or a non-prerelease .nupkg.
This script does not push the vpack, but gives the required command to do so.
#>
[CmdLetBinding()]
Param(
    [Parameter(Mandatory=$true)]
    [string]$NugetPackageInputPath,

    [Parameter(Mandatory=$true)]
    [string]$VPackDirectoryOutputPath,

    [Parameter(Mandatory=$true)]
    [string]$PublicsDir # e.g. "d:\os\public\x86chk.nocil"
    )

$vpackName  = "Microsoft.UI.Xaml"
$outputDirName = $vpackName
$outputPath = Join-Path $VPackDirectoryOutputPath $outputDirName

if(!(Test-Path $PublicsDir))
{
    Write-Error "The path '$PublicsDir' does not exist."
    exit 1
}

if(Test-Path $outputPath)
{
    Write-Error "The path '$outputPath' already exists. Please delete the old '$outputDirName' directory before running this script."
    exit 1
}

if(!(Get-Command mdmerge -ErrorAction Ignore))
{
    Write-Error "Cannot find mdmerge. Make sure to run from a Developer Command Prompt."
    exit 1
}

$nupkgFileNameWithoutExtension = (Get-Item $NugetPackageInputPath).Basename

if(!$nupkgFileNameWithoutExtension.StartsWith("Microsoft.UI.Xaml.","OrdinalIgnoreCase"))
{
    Write-Error "Expected input nupkg file to start with 'Microsoft.UI.Xaml.'"
    exit 1
}

$isPrerelease = $nupkgFileNameWithoutExtension.EndsWith("-prerelease","OrdinalIgnoreCase")
$nupkgFileNameWithoutExtension = $nupkgFileNameWithoutExtension -replace "-prerelease"

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
$verPrereleaseSuffix = if ($isPrerelease) {"-prerelease"} else {""}
Write-Verbose "Version = '$verMajor.$verMinor.$verPatch$verPrereleaseSuffix'"

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


New-Item -ItemType Directory -Force -Path $outputPath | Out-Null

$osBuildMetadataDir = Join-Path $PublicsDir "onecoreuap\internal\buildmetadata"
$outputMetaDataDir = Join-Path $outputPath "metadata"
$inputWinmdDir = Join-Path $nugetUnpacked "lib\uap10.0\"

# We need to re-merge Microsoft.UI.Xaml.winmd against the razzle metadata instead of against the metadata from the public sdk:
$mdMergeArgs = "-v -metadata_dir $osBuildMetadataDir -o $outputMetaDataDir -i $inputWinmdDir -partial -n:3 -createPublicMetadata -transformExperimental:transform"
Write-Host "mdmerge $mdMergeArgs"
Invoke-Expression "mdmerge $mdMergeArgs"
if($LASTEXITCODE)
{
    Write-Error "mdmerge exited with error ($LASTEXITCODE)"
    exit
}

$infoTextFilePath = Join-Path $outputPath "info.txt"
Out-File -FilePath $infoTextFilePath -InputObject "Vpack created from nuget package: $nupkgFileNameWithoutExtension"

if($isPrerelease)
{
    $flavors = @("x86", "x64", "arm", "arm64")
    foreach ($flavor in $flavors)
    {
        $flavorPath = Join-Path $outputPath $flavor
        New-Item -ItemType Directory -Force -Path $flavorPath

        $runtimeDir = Join-Path $nugetUnpacked "runtimes\win10-$flavor\native\"

        Copy-Item "$runtimeDir\*" -Destination $flavorPath -Recurse
        Copy-Item "$outputMetaDataDir\*.winmd" -Destination $flavorPath
    }
}
Else
{
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
        New-Item -ItemType Directory -Force -Path $destPathDir | Out-Null

        Write-Verbose "Copy item from '$sourcePathFull' to '$destPathFull' "
        Copy-Item $sourcePathFull $destPathFull
    }
}

Write-Verbose "Removing temp dir '$nugetUnpacked'"
Remove-Item -Force -Recurse $nugetUnpacked

$verPrereleaseSwitch = If ($isPrerelease) {"/Prerelease:prerelease"} Else {""}

Write-Host "Created the vpack in this directory:"
Write-Host "    $outputPath" 
Write-Host "" 
Write-Host "Push this vpack with the following command:"
Write-Host "    vpack push /Name:Microsoft.UI.Xaml /SourceDirectory:$outputPath /VersionIncrementType:None /Major:$verMajor /Minor:$verMinor /Patch:$verPatch $verPrereleaseSwitch"
Write-Host "Then update %SDXROOT%\build\onecoreuap\internal\config\OSDependencies.Manifest in the OS repo to consume this new version."
[CmdLetBinding()]
Param(
    [Parameter(Mandatory=$true)]
    [string]$NugetPackageInputPath,

    [Parameter(Mandatory=$true)]
    [string]$VPackDirectoryOutputPath
    )

$vpackName  = "WinUIInternalSDK"
$outputDirName = $vpackName
$outputPath = Join-Path $VPackDirectoryOutputPath $outputDirName

if(Test-Path $outputPath)
{
    Write-Error "The path '$outputPath' already exists. Please delete the old '$outputDirName' directory before running this script."
    exit 1
}

$nupkgFileNameWithoutExtension = (Get-Item $NugetPackageInputPath).Basename

if(!$nupkgFileNameWithoutExtension.StartsWith("Microsoft.UI.Xaml.","OrdinalIgnoreCase"))
{
    Write-Error "Expected input nupkg file to start with 'Microsoft.UI.Xaml.'"
    exit 1
}

if(!$nupkgFileNameWithoutExtension.EndsWith("-prerelease","OrdinalIgnoreCase"))
{
    Write-Error "Expected input nupkg file to end with '-prerelease'"
    exit 1
}

$ver = $nupkgFileNameWithoutExtension -replace "Microsoft.UI.Xaml."
$ver = $ver -replace "-prerelease"

$verParts = $ver.Split(".")
if($verParts.Count -ne 3)
{
    Write-Error "Expected input nupkg file to have 3-place version number"
    exit 1
}

$verMajor = $verParts[0]
$verMinor = $verParts[1]
$verPatch = $verParts[2]
$verPrerelease = "-prerelease"
Write-Host "Version = '$verMajor.$verMinor.$verPatch$verPrerelease'"

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

$publicsDir = "d:\os2\public\x86chk.nocil"
$osBuildMetadataDir = Join-Path $publicsDir "onecoreuap\internal\buildmetadata"



$outputMetaDataDir = Join-Path $outputPath "metadata"

$inputWinmdDir = Join-Path $nugetUnpacked "lib\uap10.0\"

$mdMergeArgs = "-v -metadata_dir $osBuildMetadataDir -o $outputMetaDataDir -i $inputWinmdDir -partial -n:3 -createPublicMetadata -transformExperimental:transform"

Invoke-Expression "mdmerge $mdMergeArgs"



$flavors = @("x86", "x64", "arm", "arm64")
foreach ($flavor in $flavors)
{
    $flavorPath = Join-Path $outputPath $flavor
    New-Item -ItemType Directory -Force -Path $flavorPath

    $runtimeDir = Join-Path $nugetUnpacked "runtimes\win10-$flavor\native\"

    Copy-Item "$runtimeDir\*" -Destination $flavorPath -Recurse
    Copy-Item "$outputMetaDataDir\*.winmd" -Destination $flavorPath
}


Write-Verbose "Removing temp dir '$nugetUnpacked'"
Remove-Item -Force -Recurse $nugetUnpacked


Write-Host "Created the vpack in this directory:"
Write-Host "    $outputPath" 
Write-Host "" 
Write-Host "Push this vpack with the following command:"
Write-Host "    VPack.exe push /Name:$vpackName /SourceDirectory:$outputPath /VersionIncrementType:None /Major:$verMajor /Minor:$verMinor /Patch:$verPatch /Prerelease:prerelease"
Write-Host "Then update %SDXROOT%/build/Config/OSDependencies.Manifest in the OS repo to consume this new version."
param
(
    [Parameter(Mandatory=$true)]
    [string] $releaseFolder,

    [Parameter(Mandatory=$true)]
    [string] $publicsRoot,

    [Parameter(Mandatory=$true)]
    [string] $publishDir
)

Trap
{
    Write-Error $_.Exception.Message;
    Exit 1
};

$repoRoot = $script:MyInvocation.MyCommand.Path | Split-Path -Parent | Split-Path -Parent

function GetVersionFromManifest([string] $manifestPath)
{
    return (Get-Content $manifestPath) | Select-String 'Version="(.+?)"' -caseSensitive | Foreach-Object {$_.Matches} | Foreach-Object {$_.Groups[1].Value} | Select-Object -First 1
}

function UnpackAppx([string] $sourceFolder, [string] $targetFolder)
{
    $appxPath = "$sourceFolder\FrameworkPackage"

    $appx = Get-ChildItem "$appxPath" -Filter "Microsoft.UI.Xaml*.appx"

    $count = ($appx | Measure-Object).Count

    if ($count -ne 1) 
    {
        Write-Host "There is 0 or 1+ .appx in $appxPath"
        exit 1
    }

    $path = $appx.FullName 

    Write-Host "Running: makeappx.exe unpack /p $path /d $targetFolder /o /l"
    makeappx.exe unpack /p "$path" /d "$targetFolder" /o /l | Out-Null
}

function Get-ScriptDirectory {
    Split-Path -parent $PSCommandPath
}

function Get-WebView2PackageVersion {
    $packagesConfig = Join-Path $repoRoot "dev\dll\packages.config"
    [xml]$packages = Get-Content $packagesConfig
    $webView2Version = $packages.SelectSingleNode("//packages/package[@id=`"Microsoft.Web.WebView2`"]").version

    return $webView2Version
}

function RemergeWinMd([string] $winmdSourceFolder, [string] $winmdTargetFolder, [string] $winmdReferencesDir)
{
    Write-Host "re-merge Microsoft.UI.Xaml.winmd"

    New-Item -Path "$winmdTargetFolder" -ItemType Directory | Out-Null

    # We need to re-merge Microsoft.UI.Xaml.winmd against the OS internal metadata instead of against the metadata from the public sdk:
    $mdMergeArgs = "-v -metadata_dir ""$winmdReferencesDir"" -o ""$winmdTargetFolder"" -i ""$winmdSourceFolder"" -partial -n:3 -createPublicMetadata -transformExperimental:transform"
    Write-Host "mdmerge $mdMergeArgs"
    Invoke-Expression "mdmerge $mdMergeArgs" | Out-Null
    if($LASTEXITCODE)
    {
        Write-Error "mdmerge exited with error ($LASTEXITCODE)"
        exit
    }
}

if (-not (Test-Path "$releaseFolder"))
{
    Write-Error "Not found folder $releaseFolder"
    Exit 1
} 

if (-not (Test-Path "$publicsRoot"))
{
    Write-Error "Not found folder $publicsRoot"
    Exit 1
} 

if(!(Get-Command mdmerge -ErrorAction Ignore))
{
    Write-Error "Cannot find mdmerge. Make sure to run from a Developer Command Prompt."
    exit 1
}
$winuiVpackFolder = "$publishDir\WinUIVpack"
$cbsFolder = "$publishDir\CBS"
$cbswinmdFolder = "$cbsFolder\winmd"
$packagesDir = Join-Path $repoRoot "packages"
$winmdReferencesDir = Join-Path $repoRoot "winmdreferences"

if (Test-Path $cbsFolder)
{
    Write-Host "Deleting $cbsFolder"
    Remove-Item -Path $cbsFolder -Force -Recurse| Out-Null
}

if (Test-Path $winuiVpackFolder)
{
    Write-Host "Deleting $winuiVpackFolder"
    Remove-Item -Path $winuiVpackFolder -Force -Recurse| Out-Null
}

if (Test-Path $winmdReferencesDir)
{
    Write-Host "Deleting $winmdReferencesDir"
    Remove-Item -Path $winmdReferencesDir -Force -Recurse| Out-Null
}

New-Item -Path "$winuiVpackFolder" -ItemType Directory | Out-Null
New-Item -Path "$cbsFolder" -ItemType Directory | Out-Null
New-Item -Path "$winmdReferencesDir" -ItemType Directory | Out-Null

Write-Host "Copy OS publics to $winmdReferencesDir"
$osBuildMetadataDir = Join-Path $publicsRoot "onecoreuap\internal\buildmetadata"
Copy-Item "$osBuildMetadataDir\*.winmd" "$winmdReferencesDir" -Force 

Write-Host "Copy WebView2 winmd to $winmdReferencesDir"
$webView2Version = Get-WebView2PackageVersion
$webView2WinMdPath = Join-Path $packagesDir "Microsoft.Web.WebView2.$($webView2Version)\lib"
Copy-Item "$webView2WinMdPath\*.winmd" "$winmdReferencesDir" -Force 

$buildFlavours = @("X64", "X86", "ARM", "ARM64")

foreach ($flavour in $buildFlavours)
{
    $sourceFolder = "$releaseFolder\$flavour"
    if (-not (Test-Path $sourceFolder))
    {
        Write-Error "Not found folder $sourceFolder"
        Exit 1
    }  
}

# Create WinUI Vpack:
foreach ($flavour in $buildFlavours) 
{
    $sourceFolder = "$releaseFolder\$flavour\FrameworkPackage"
    $winmdSourceFolder = "$releaseFolder\$flavour\Microsoft.UI.Xaml\sdk"
    $targetFolder = "$winuiVpackFolder\$flavour"
    $winmdTargetFolder = "$winuiVpackFolder\winmd"

    New-Item -Path "$targetFolder" -ItemType Directory | Out-Null

    $search = "Microsoft.UI.Xaml.*.appx"
    $found = Get-ChildItem $sourceFolder -Filter $search
    if ($found.Length -eq 0)
    {
        Write-Error "Could not find '$search' in '$sourceFolder'"
        Exit 1
    }

    $fileName = $found[0].Name
    $sourcePathFull = $found[0].FullName
    
    $destPathFull = Join-Path $targetFolder $fileName

    Write-Verbose "Copy item from '$sourcePathFull' to '$destPathFull' "
    Copy-Item $sourcePathFull $destPathFull

    if ($flavour -ieq "X64")
    {
        RemergeWinMd $winmdSourceFolder $winmdTargetFolder $winmdReferencesDir
    }
}

# Create CBS Vpacks:
foreach ($flavour in $buildFlavours) 
{
    $sourceFolder = "$releaseFolder\$flavour"
    $targetFolder = "$cbsFolder\$flavour"

    New-Item -Path "$cbsFolder\$flavour" -ItemType Directory | Out-Null
 
    UnpackAppx "$sourceFolder" "$targetFolder"
    
    Write-Host "Preparing AppxManifest, .pdb and resources.pri for $flavour"    
    Copy-Item "$sourceFolder\Symbols\Microsoft.UI.Xaml.pdb" "$targetFolder" -Force | Out-Null
    Copy-Item "$sourceFolder\FrameworkPackage\CBSAppxManifest.xml" "$targetFolder\AppxManifest.xml" -Force | Out-Null
    Copy-Item "$sourceFolder\FrameworkPackage\CBSresources.pri" "$targetFolder\resources.pri" -Force | Out-Null

    if ($flavour -ieq "X64")
    {
        RemergeWinMd $targetFolder $cbswinmdFolder $winmdReferencesDir
    }
}

$manifestVersion = GetVersionFromManifest "$targetFolder\AppxManifest.xml"
$version = $manifestVersion.split(".")
$major = $version[0]
$minor = $version[1]
$patch = $version[2]

$vpackversion = "$($major).$($minor).$($patch)"
Write-Host "vpackversion=$vpackversion"

$readmeFilePath = Join-Path $cbsFolder "readme.txt"
$contentString = "This is the contents of version '$($vpackversion)' of the MicrosoftUIXamlInbox VPacks"
Set-Content -Path $readmeFilePath -Value $contentString 

# This sets a Pipeline variable that the Job will use to push the created vpacks with the specified version.
Write-Host "##vso[task.setvariable variable=vpackversion;isOutput=true;]$vpackversion"
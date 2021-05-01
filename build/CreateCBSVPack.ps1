param
(
    [Parameter(Mandatory=$true)]
    [string] $releaseFolder,

    [Parameter(Mandatory=$true)]
    [string] $publicsRoot
)

Trap
{
    Write-Error $_.Exception.Message;
    Exit 1
};

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

$cbsFolder = "$releaseFolder\CBS"
$winmdFolder = "$cbsFolder\winmd"

if (Test-Path $cbsFolder)
{
    Write-Host "Deleting $cbsFolder"
    Remove-Item -Path $cbsFolder -Force -Recurse| Out-Null
}

New-Item -Path "$cbsFolder" -ItemType Directory | Out-Null
New-Item -Path "$winmdFolder" -ItemType Directory | Out-Null

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
        Write-Host "re-merge Microsoft.UI.Xaml.winmd"

        $osBuildMetadataDir = Join-Path $publicsRoot "onecoreuap\internal\buildmetadata"

        # We need to re-merge Microsoft.UI.Xaml.winmd against the OS internal metadata instead of against the metadata from the public sdk:
        $mdMergeArgs = "-v -metadata_dir ""$osBuildMetadataDir"" -o ""$winmdFolder"" -i ""$targetFolder"" -partial -n:3 -createPublicMetadata -transformExperimental:transform"
        Write-Host "mdmerge $mdMergeArgs"
        Invoke-Expression "mdmerge $mdMergeArgs" | Out-Null
        if($LASTEXITCODE)
        {
            Write-Error "mdmerge exited with error ($LASTEXITCODE)"
            exit
        }
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
Write-Host "##vso[task.setvariable variable=vpackversion]$vpackversion"
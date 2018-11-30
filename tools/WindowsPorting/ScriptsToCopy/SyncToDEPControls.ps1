[CmdletBinding()]
Param(
    [Parameter(Mandatory = $true)]
    [string]$gitRepositoryPath
)

# First we'll make sure that we're running in a razzle context, and store the path to the
# repository if we are.
if ($($env:SDXROOT).Length -eq 0)
{
    Write-Host "This script must be run from a razzle environment."
    exit 1
}

$windowsPath = $PSScriptRoot

$env:ERRORLEVEL = "0"
$LASTEXITCODE = 0

function Get-WasError
{
    return ($env:ERRORLEVEL -ne $null -and $env:ERRORLEVEL.Length -gt 0 -and $env:ERRORLEVEL -ne "0") -or ($LASTEXITCODE -ne 0)
}

function Report-FailedStep
{
    Param(
        [Parameter(Mandatory = $true)] 
        [string]$stepName
    )
    
    Write-Error "$stepName FAILED."
    exit 1
}

# We'll check to make sure that all of the directories specified exist.
if (!(Test-Path $gitRepositoryPath))
{
    "Could not find the path '$gitRepositoryPath'."
    exit 1
}

function Copy-Directory
{
    Param(
        [Parameter(Mandatory = $true)] 
        [string]$SourcePath,
 
        [string]$Exclude)

    $directoryName = $(Split-Path $SourcePath -Leaf)
    $destinationPath = "$windowsPath\$directoryName"
  
    Write-Host "Copying $SourcePath\ to \$directoryName\..."

    if (Test-Path "$destinationPath")
    {
        Get-ChildItem "$destinationPath" -File -Recurse -Exclude "sources.dep","autogen.*","Auto-OnecoreUapWindows.*" | ForEach-Object { Remove-Item $_.FullName }
    }

    New-Item "$destinationPath" -ItemType Directory 2>&1> $null
    Copy-Item "$SourcePath" "$windowsPath" -Force -Recurse -Exclude "$Exclude"

    if (Get-WasError)
    {
        return $false
    }
    
    return $true
}

if (-not (Copy-Directory -SourcePath "$gitRepositoryPath\build")) { Report-FailedStep "Copy" }
if (-not (Copy-Directory -SourcePath "$gitRepositoryPath\dev")) { Report-FailedStep "Copy" }
if (-not (Copy-Directory -SourcePath "$gitRepositoryPath\idl")) { Report-FailedStep "Copy" }
if (-not (Copy-Directory -SourcePath "$gitRepositoryPath\manifest")) { Report-FailedStep "Copy" }
if (-not (Copy-Directory -SourcePath "$gitRepositoryPath\test" -Exclude "Nuget.config")) { Report-FailedStep "Copy" }
if (-not (Copy-Directory -SourcePath "$gitRepositoryPath\tools")) { Report-FailedStep "Copy" }
if (-not (Copy-Directory -SourcePath "$gitRepositoryPath\winrt")) { Report-FailedStep "Copy" }

# There are a few files that we need to exclude from the OS repository for different reasons:
#
#   Build.cmd - This file conflicts with the OS repo-wide build.cmd.
#   .gitignore/.gitattributes - The OS repo build is GVFS, which supports different .gitattributes from normal Git,
#                               and builds with razzle, which leaves different build artifacts behind than VS.
#
Get-ChildItem "$gitRepositoryPath\*" -File -Exclude "Build.cmd",".git*" | Where-Object { -not $_.PSIsContainer } | ForEach-Object { Copy-Item $_.FullName "$windowsPath\" }

if (Get-WasError)
{
    Report-FailedStep "Copy"
}

Write-Host
Write-Host "Running WUXC code gen..."
Write-Host
Write-Host "$windowsPath\RunWUXCCodeGen.cmd"
& $windowsPath\RunWUXCCodeGen.cmd

if (Get-WasError)
{
    Report-FailedStep "Code gen"
}

Write-Host "Adding file changes to Git..."
Write-Host

& git add *

if (Get-WasError)
{
    Report-FailedStep "Adding"
}

Write-Host "Sync complete!"
Write-Host
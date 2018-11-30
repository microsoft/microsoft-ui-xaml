<#
.SYNOPSIS
This script is used to extract the appx files from a nuget package, rename them to include the platform
architecture, and then put them all in the same directory. This facilitates submitting the packages to
the Store.
#>
[CmdLetBinding()]
Param(
    [Parameter(Position=0,Mandatory=$true)]
    [string]$nugetPackage,
    [Parameter(Position=1,Mandatory=$true)]
    [string]$outputDirectory)
    
    
Add-Type -AssemblyName System.IO.Compression.FileSystem
$appxArchiveEntries = [System.IO.Compression.ZipFile]::OpenRead($nugetPackage).Entries | Where-Object { $_.FullName.EndsWith(".appx") }

[Environment]::CurrentDirectory = $PSScriptRoot
$outputPath = [System.IO.Path]::GetFullPath($outputDirectory)
Write-Verbose "Output path = $outputPath"

if (-not (Test-Path $outputPath))
{
    $ignore = New-Item -ItemType Directory $outputPath
    Write-Verbose $ignore
}

foreach ($appxEntry in $appxArchiveEntries)
{
    $pathParts = $appxEntry.FullName.Split("/")
    $platform = $pathParts[2]
    $fileName = $pathParts[$pathParts.Length - 1]
    $outputFile = $fileName.Replace(".appx", ".$platform.appx")
    $outputFilePath = Join-Path $outputPath $outputFile
    Write-Verbose "Extracting $($appxEntry.FullName) to $outputPath"
    [System.IO.Compression.ZipFileExtensions]::ExtractToFile($appxEntry, $outputFilePath)
}
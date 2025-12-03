Param(
    [Parameter(Mandatory=$true)] [string] $outDir,
    [Parameter(Mandatory=$true)] [string] $downloadFeed,
    [Parameter(Mandatory=$true)] [string] $packageName,
    [Parameter(Mandatory=$true)] [string] $targetFileName,
    [string]$Verbosity="quiet"
)

$ErrorActionPreference = "Stop"

$downloadDest = Join-Path $outDir $targetFileName

$tempFolder = "$env:temp\Package-$(Get-Date -format 'yyyy-MM-dd_hh-mm-ss')"
New-Item $tempFolder -ItemType Directory | Out-Null

& "$outDir\nuget.exe" install $packageName -source $downloadFeed -OutputDirectory `"$tempFolder`" -Prerelease -NonInteractive -Verbosity $Verbosity

$authHelper = Get-ChildItem -Filter $targetFileName -Path $tempFolder -Recurse

if(-not $authHelper)
{
    Write-Warning "Failed to fetch updated $targetFileName from $downloadFeed"
    if (!(Test-Path $downloadDest)) {
        throw "$packageName was not found at $downloadFeed"
    } else {
        Write-Warning "$targetFileName may be out of date"
    }
}
else
{
   Copy-Item -Path $authHelper.FullName -Destination $outDir
}

Remove-Item $tempFolder -Force -Recurse
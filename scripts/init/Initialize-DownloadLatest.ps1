Param(
    [Parameter(Mandatory=$true)] [string] $outDir,
    [Parameter(Mandatory=$true)] [string] $downloadUrl,
    [Parameter(Mandatory=$true)] [string] $downloadName,    
    [Parameter(Mandatory=$false)] [boolean] $unzip = $false
)

$ErrorActionPreference = "Stop"

Write-Host -NoNewLine "Ensuring that $downloadName is up to date..."

# Grab the VSS.NuGet toolset from VSO
Add-Type -AssemblyName System.IO
Add-Type -AssemblyName System.IO.Compression.FileSystem

$etagFile = Join-Path $outDir "$downloadName.ETag"
$downloadPath = Join-Path $outDir "$downloadName.download"
$downloadDest = Join-Path $outDir $downloadName
$downloadDestTemp = Join-Path $outDir "$downloadName.tmp"
$headers = @{}


# If the destination folder doesn't exist, delete the ETag file if it exists
if (!(Test-Path -PathType Container $downloadDest) -and (Test-Path -PathType Container $etagFile)) {
    Remove-Item -Force $etagFile
}

if (Test-Path $etagFile)
{
    $headers.Add("If-None-Match", [System.IO.File]::ReadAllText($etagFile))
}

# Invoke-WebRequest is orders of magnitude slower when the progress indicator is being displayed. So temporarily disable it.
$ProgressPreferenceOld = $ProgressPreference
$ProgressPreference = "SilentlyContinue"
try 
{
    $response = Invoke-WebRequest -Headers $headers -Uri $downloadUrl -PassThru -OutFile $downloadPath -UseBasicParsing
}
catch [System.Net.WebException]
{
    $response = $_.Exception.Response
}
finally
{
    $ProgressPreference = $ProgressPreferenceOld    
}

if ($response.StatusCode -eq 200)
{
    Unblock-File $downloadPath
    [System.IO.File]::WriteAllText($etagFile, $response.Headers["ETag"])

    if($unzip)
    {
        # Extract to a temp folder
        if (Test-Path -PathType Container $downloadDestTemp)
        {
            [System.IO.Directory]::Delete($downloadDestTemp, $true)
        }

        [System.IO.Compression.ZipFile]::ExtractToDirectory($downloadPath, $downloadDestTemp)
        Remove-Item $downloadPath
    }
    else
    {
        $downloadDestTemp = $downloadPath;
    }

    # Delete and rename to final dest
    if (Test-Path -PathType Container $downloadDest)
    {
        [System.IO.Directory]::Delete($downloadDest, $true)
    }

    Move-Item -Force $downloadDestTemp $downloadDest
    Write-Host "Updated $downloadName"
}
elseif ($response.StatusCode -eq 304)
{
    Write-Host -ForegroundColor Green "Done."
}
else
{
	Write-Host
    Write-Warning "Failed to fetch updated NuGet tools from $downloadUrl"
    if (!(Test-Path $downloadDest)) {
        throw "$downloadName was not found at $downloadDest"
    } else {
        Write-Warning "$downloadName may be out of date"
    }
}

return $downloadDest
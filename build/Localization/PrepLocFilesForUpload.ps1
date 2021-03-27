[CmdLetBinding()]
param(
    [string]$DestinationFilePath
)

Write-Host "Copying resource files to staging area to prep for upload." -ForegroundColor Green
[xml]$locConfigXml = Get-Content Settings\LocConfig.xml

foreach ($file in $locConfigXml.Modules.Module.File)
{
    $destFileLocation = "$DestinationFilePath\$($file.location)"
    $sourceLocation = "../../$($file.path)"
    Write-Verbose "Dest: $destFileLocation Source: $sourceLocation"

    if (-not (Test-Path $destFileLocation)) { mkdir $destFileLocation }
    $output = Copy-Item $sourceLocation $destFileLocation
}

Write-Host 

Write-Host "Done!" -ForegroundColor Green
[CmdLetBinding()]
param(
    [string]$DestinationFilePath
)

Write-Host "Copying resource files to staging area to prep for upload." -ForegroundColor Green

# Retrieve all the english resource files in the repo
$englishResourceFiles = Get-ChildItem -Path "$PSScriptRoot\..\..\dev" -Include "Resources.resw" -Recurse | Where-Object {$_.Directory -Match "en-us"}

foreach ($file in $englishResourceFiles)
{
    $controlName = $file.FullName | Split-Path | Split-Path | Split-Path | Split-Path -Leaf

    $destFileLocation = "$DestinationFilePath\$controlName"
    $sourceLocation = $file.FullName
    Write-Verbose "Dest: $destFileLocation Source: $sourceLocation"

    if (-not (Test-Path $destFileLocation)) { mkdir $destFileLocation }
    $output = Copy-Item $sourceLocation $destFileLocation
}

Write-Host 

Write-Host "Done!" -ForegroundColor Green

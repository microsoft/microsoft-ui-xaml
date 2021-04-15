[CmdLetBinding()]
param(
    [string]$DestinationFilePath
)

Write-Host "Copying resource files to staging area to prep for upload." -ForegroundColor Green

$controlsPath = Convert-Path "$PSScriptRoot\..\..\dev\"

# Retrieve all the english resource files in the repo
$englishResourceFiles = Get-ChildItem -Path $controlsPath -Include "Resources.resw" -Recurse | Where-Object {$_.Directory -Match "en-us"}

foreach ($file in $englishResourceFiles)
{
    # Extract control name (directory name under ..\dev\)
    $endPath = $file.FullName.substring($controlsPath.Length)
    $controlName = ($endPath -split '\\')[0]

    $destFileLocation = "$DestinationFilePath\$controlName"
    $sourceLocation = $file.FullName
    Write-Verbose "Dest: $destFileLocation Source: $sourceLocation"

    if (-not (Test-Path $destFileLocation)) { mkdir $destFileLocation }
    $output = Copy-Item $sourceLocation $destFileLocation
}

Write-Host 

Write-Host "Done!" -ForegroundColor Green

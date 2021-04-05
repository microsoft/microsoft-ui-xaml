[CmdLetBinding()]
param(
    [string]$LocalizedFilesLocation="$PSScriptRoot\..\..\BuildOutput\LocalizationDrop"
)

if (-not (Test-Path "$LocalizedFilesLocation"))
{
    Write-Host "Could not find LocalizationDrop folder. Either specify using -LocalizedFilesLocation or place in the BuildOutput folder." -ForegroundColor Red
    Exit 1
}

Write-Host "Copying localized files back into the source tree" -ForegroundColor Green

$controlsPath = Convert-Path "$PSScriptRoot\..\..\dev\"

# Retrieve all the english resource files in the repo
$englishResourceFiles = Get-ChildItem -Path $controlsPath -Include "Resources.resw" -Recurse | Where-Object {$_.Directory -Match "en-us"}

# Use ColorPicker as an example in order to extract the list of available languages
$languages = get-childitem "$LocalizedFilesLocation\ColorPicker" | Where-Object { $_.Name -match "-" } | % { $_.Name }

foreach ($language in $languages)
{
    Write-Verbose "Current language: $language"
    foreach ($file in $englishResourceFiles)
    {
        $destFilePath = $file.FullName -ireplace "en-us",$language

        # Extract control name (directory name under ..\dev\)
        $endPath = $file.FullName.substring($controlsPath.Length)
        $controlName = ($endPath -split '\\')[0]

        $fileName = Split-Path -Leaf $destFilePath

        $destFileLocation = Split-Path -Parent $destFilePath

        $sourceLocation = "$LocalizedFilesLocation\$controlName\$language\$fileName"
        Write-Verbose "Dest: $destFileLocation Source: $sourceLocation"

        if (-not (Test-Path $destFileLocation)) { mkdir $destFileLocation }
        $output = Copy-Item $sourceLocation $destFileLocation
    }
}
Write-Host 

Write-Host "Done!" -ForegroundColor Green
[CmdLetBinding()]
param(
    [string]$LocalizedFilesLocation="$PSScriptRoot\..\..\BuildOutput\LocalizationDrop"
)

if (-not (Test-Path "$LocalizedFilesLocation\LocalizationDrop"))
{
    Write-Host "Could not find LocalizationDrop folder. Either specify using -LocalizedFilesLocation or place in the BuildOutput folder." -ForegroundColor Red
    Exit 1
}

Write-Host "Copying localized files back into the source tree" -ForegroundColor Green
[xml]$locConfigXml = Get-Content "$PSScriptRoot\Settings\LocConfig.xml"

# Use ColorPicker as an example in order to extract the list of available languages
$languages = get-childitem "$LocalizedFilesLocation\LocalizationDrop\ColorPicker" | Where-Object { $_.Name -match "-" } | % { $_.Name }

foreach ($language in $languages)
{
    Write-Verbose "Current language: $language"
    foreach ($file in $locConfigXml.Modules.Module.File)
    {
        $destFilePath = $file.path -ireplace "en-us",$language
        $destFilePath = "$PSScriptRoot\..\..\$destFilePath"

        $fileName = Split-Path -Leaf $destFilePath

        $destFileLocation = Split-Path -Parent $destFilePath

        $sourceLocation = "$LocalizedFilesLocation\LocalizationDrop\$($file.location)\$language\$fileName"
        Write-Verbose "Dest: $destFileLocation Source: $sourceLocation"

        if (-not (Test-Path $destFileLocation)) { mkdir $destFileLocation }
        $output = Copy-Item $sourceLocation $destFileLocation
    }
}
Write-Host 

Write-Host "Done!" -ForegroundColor Green
<#
.SYNOPSIS
Script that takes an Azure Maps token and injects it into MapControl test page.
#>

param (
    [parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$MapsToken
)

$repoRoot = Split-Path -Parent $PSScriptRoot

# Read the content of the test page
$file_path = "$repoRoot\controls\dev\MapControl\TestUI\MapControlApiKey.cs"
$content = Get-Content -Path $file_path

# Replace the tag "<AzureMapsToken>" with the provided token
$new_content = $content -replace "<AzureMapsToken>", $MapsToken

# Write the updated content back to the file
Set-Content -Path $file_path -Value $new_content
Write-Host "Azure Maps token injected successfully."
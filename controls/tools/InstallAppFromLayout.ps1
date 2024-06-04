$appxManifestPath = Join-Path $PSScriptRoot "AppxManifest.xml"

$appName = ([xml][System.IO.File]::ReadAllText($appxManifestPath)).Package.Identity.Name
$existingInstallation = Get-AppxPackage $appName

if ($existingInstallation)
{
    Write-Host "Uninstalling existing test app..."
    Remove-AppxPackage $existingInstallation.PackageFullName
}

Write-Host "Installing test app..."
Add-AppxPackage -Register $appxManifestPath
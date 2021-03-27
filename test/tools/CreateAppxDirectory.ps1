$vsInstallDir = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -Latest -requires Microsoft.Component.MSBuild -property InstallationPath
$msBuildLocation = Join-Path $vsInstallDir "MSBuild\Current\Bin\MSBuild.exe"

$createAppxDirectoryProjectPath = Join-Path $PSScriptRoot "CreateAppxDirectory.msbuildproj"
$appxManifestPath = Join-Path $PSScriptRoot "AppX\AppxManifest.xml"

Write-Host "Creating test app layout..."
& $msBuildLocation $createAppxDirectoryProjectPath | Out-Null

Write-Host "Registering test app..."
Add-AppxPackage -Register $appxManifestPath | Out-Null
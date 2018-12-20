@echo off

if exist "%SystemDrive%\Program Files\WindowsApps\MUXControlsTestApp_1.0.0.0_neutral__8wekyb3d8bbwe\MUXControlsTestApp.exe" (
    echo MUXControlsTestApp.appx already installed. Uninstalling...
    powershell Remove-AppxPackage MUXControlsTestApp_1.0.0.0_neutral__8wekyb3d8bbwe
)

echo Installing MUXControlsTestApp.appx...
powershell Add-AppxPackage %~dp0\Test\MUXControlsTestApp.appx -DependencyPath %~dp0\Test\TaefFramework.appx

@echo off

if exist "%SystemDrive%\Program Files\WindowsApps\MUXControlsTestApp_1.0.0.0_neutral__6f07fta6qpts2\MUXControlsTestApp.exe" (
    echo MUXControlsTestApp.appx already installed. Uninstalling...
    powershell Remove-AppxPackage MUXControlsTestApp_1.0.0.0_neutral__6f07fta6qpts2
)

echo Installing MUXControlsTestApp.appx...
powershell Add-AppxPackage %~dp0\Test\MUXControlsTestApp.appx -DependencyPath %~dp0\Test\TaefFramework.appx

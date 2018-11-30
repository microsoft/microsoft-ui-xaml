@echo off

if exist "%SystemDrive%\Program Files\WindowsApps\407b1cc5-f51e-4bfa-b5d2-04afa83fe380_1.0.0.0_neutral__8wekyb3d8bbwe\MUXControlsTestApp.exe" (
    echo MUXControlsTestApp.appx already installed. Uninstalling...
    powershell Remove-AppxPackage 407b1cc5-f51e-4bfa-b5d2-04afa83fe380_1.0.0.0_neutral__8wekyb3d8bbwe
)

echo Installing MUXControlsTestApp.appx...
powershell Add-AppxPackage %~dp0\Test\MUXControlsTestApp.appx -DependencyPath %~dp0\Test\TaefFramework.appx

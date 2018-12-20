@echo off

if exist "%SystemDrive%\Data\PROGRAMS\WINDOWSAPPS\MUXControlsTestApp_1.0.0.0_neutral__8wekyb3d8bbwe\MUXControlsTestApp.exe" (
    echo MUXControlsTestApp.appx already installed. Uninstalling...
    th {MUXControlsTestApp} -u
    echo.
)

echo Installing MUXControlsTestApp.appx...
th %~dp0\Test\MUXControlsTestApp.appx -sload
echo.
@echo off

if exist "%SystemDrive%\Data\PROGRAMS\WINDOWSAPPS\407b1cc5-f51e-4bfa-b5d2-04afa83fe380_1.0.0.0_neutral__8wekyb3d8bbwe\MUXControlsTestApp.exe" (
    echo MUXControlsTestApp.appx already installed. Uninstalling...
    th {407B1CC5-F51E-4BFA-B5D2-04AFA83FE380} -u
    echo.
)

echo Installing MUXControlsTestApp.appx...
th %~dp0\Test\MUXControlsTestApp.appx -sload
echo.
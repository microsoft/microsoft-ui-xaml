@echo off

if "%WinUIVersion%"=="" set WinUIVersion=3.0.0-dev

echo Building with WinUIVersion=%WinUIVersion%

taskkill /f /im msbuild.exe >nul 2>&1
taskkill /f /im vbcscompiler.exe >nul 2>&1
rmdir /s /q packages >nul 2>&1
nuget locals all -clear
msbuild -nologo -t:Restore MeasureMUX-set.sln -p:PublishReadyToRun=true -p:Configuration=%Configuration% -p:Platform=%buildPlatform%
nuget restore -configfile nuget.config packages.config
nuget install Microsoft.WindowsAppSDK.WinUI -Version %WinUIVersion%

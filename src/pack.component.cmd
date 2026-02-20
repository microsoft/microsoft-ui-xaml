@echo off
if not "%winui_echo%" == "" @echo on
setlocal enabledelayedexpansion enableextensions

if "%BuildPlatform%"=="" goto :usage
if "%Configuration%"=="" goto :usage

set _root=%~dp0
set _versionOption=-VersionOverride "3.0.0-dev"

:parseArgs
if "%1"=="/?" (
    goto :usage
) else if "%1"=="/version" (
    set _versionOption=-VersionOverride %2
    shift
) else if "%1"=="" (
    goto :main
) else (
    echo Unrecognized option: %1
    goto :usage
)
shift
goto :parseArgs

:main
if EXIST "%RepoRoot%\pack.cmd" (
    call %_root%\tools\PowershellWrapper.cmd %_root%\build\nuspecs\build-nupkg.ps1 -Nuspec Microsoft.WindowsAppSDK.WinUI.nuspec %_versionOption% -PackageRoot %_root%BuildOutput\packaging\%Configuration%
) else (
    call %_root%\tools\PowershellWrapper.cmd %_root%\build\nuspecs\build-nupkg.ps1 -Nuspec Microsoft.WindowsAppSDK.WinUI.nuspec %_versionOption% -PackageRoot %_root%BuildOutput\packaging\%Configuration% -UseDependencyOverrides
)
if ERRORLEVEL 1 (
    echo ---
    echo ERROR: build-nupkg.ps1 FAILED with %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
exit /b 0

:usage
echo Builds the Microsoft.WindowsAppSDK.WinUI component nuget package into the PackageStore directory.
echo This only packs whatever architectures have been built and are in the packaging directory.
echo.
echo Requires Configuration environment variable to be set, e.g. via init.cmd
echo.
echo Usage:
echo     pack.component.cmd [options]
echo.
echo    Options:
echo        /version [ver]  Override nuget package version (default is 3.0.0-dev)
echo.
exit /b /1

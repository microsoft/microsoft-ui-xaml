@echo off
if not "%winui_echo%" == "" @echo on
pushd %~dp0
setlocal enabledelayedexpansion enableextensions

if "%1"=="/?" goto :usage
if "%1"=="-?" goto :usage

set _targetMUXControls=
set _targetTestRunner=
set _targetProject=
set _targetProjectPath=
set _targetProduct=

:parseArgs
if "%1"=="/project" (
    set _targetProject=1
    set _targetProjectPath=%~2
) else if "%1"=="testrunner" (
    set _targetTestRunner=1
) else if "%1"=="controls" (
    set _targetMUXControls=1
) else if "%1"=="product" (
    set _targetProduct=1
) else if "%1"=="" (
    set _targetMUXControls=1
) else (
    echo Unrecognized option: %1
    goto :usage
)

:main
if "%EnvironmentInitialized%" == "" (
    echo Please run %RepoRoot%\init.cmd to ensure environment is properly initialized
    exit /b 1
)

if "%_targetMUXControls%" == "1" (
    call :buildMockPackage
    call :buildSolution %reporoot%\controls\MUXControls.sln /m
) else if "%_targetTestRunner%" == "1" (
    call :buildMockPackage
    call :buildSolution %reporoot%\controls\test\MUXControls.Test\MUXControls.Test.csproj /m
) else if "%_targetProject%" == "1" (
    call :buildSolution %reporoot%\controls\%_targetProjectPath% /m
) else if "%_targetProduct%" == "1" (
    call :buildSolution %reporoot%\controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj /m
)
goto:eof


:buildMockPackage
call %RepoRoot%\pack.cmd 
if ERRORLEVEL 1 exit /b %ERRORLEVEL%
goto :eof


:buildSolution
set _solution=%1
shift
set _args=%*

for %%i in (%_solution%) do set _title=%%~ni

set _options=/restore /bl:%_title%.%_BuildArch%%_BuildType%.binlog %_args% /nr:false

set _command=call msbuild !_options!

%_command%

goto :eof

:usage
echo Usage:
echo     build.cmd [targets]
echo.
echo    Available targets:
echo        controls            Builds all of MUXControls.sln (includes the MUXC DLL and all controls test runners and applications)
echo        testrunner          Builds the TAEF test runner
echo        product             Builds only MUXC and none of the tests
echo        /project [project]  Builds the project at the given relative path
echo.

exit /b /1

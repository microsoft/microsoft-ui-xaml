@echo off
setlocal enabledelayedexpansion
rem skip signing when using msb to build
set SkipSigning=true
set _subfolder=
rem Save script dir before arg parsing since 'shift' changes %~dp0
set _scriptDir=%~dp0

set _msb_quiet=
set _msb_args=
set _msb_initFlavor=

:parseArgs
if "%~1"=="" goto :parseDone
if /i "%~1"=="/skiploggingbuild" (
    shift
    goto :parseArgs
)
if /i "%~1"=="/q" (
    set _msb_quiet=1
    shift
    goto :parseArgs
)
if /i "%~1"=="/i" (
    set _msb_initFlavor=%~2
    shift
    shift
    goto :parseArgs
)
set _msb_args=!_msb_args! %1
shift
goto :parseArgs

:parseDone

if not "%_msb_initFlavor%" == "" (
    if "%_msb_quiet%"=="1" (
        call "%_scriptDir%..\init.cmd" %_msb_initFlavor% /envcheck /notitle >nul
    ) else (
        call "%_scriptDir%..\init.cmd" %_msb_initFlavor% /envcheck /notitle
    )
    if ERRORLEVEL 1 (
        echo ERROR: init.cmd %_msb_initFlavor% /envcheck failed
        exit /b 1
    )
) else if "%EnvironmentInitialized%" == "" (
    echo ERROR: Build environment not initialized. Run init.cmd or use /i ^<flavor^>.
    exit /b 1
)

rem The logging DLL doesn't currently work with multiproc msbuild, skip it for now
goto :buildIt

set LoggingDllPath=%~dp0\..\build\logging
msbuild.exe %LoggingDllPath% /verbosity:quiet /nologo
if "%errorlevel%" NEQ "0" goto :Error
if not exist %LoggingDllPath%\logging.dll goto :Error
set LoggingOpts=/l:%LoggingDllPath%\logging.dll;%LOGGING_OPTS%
goto :buildIt


:Error
echo Something went wrong trying to build the logging assembly -- exiting.
exit /b 1

:buildIt
if "%_msb_quiet%"=="1" (
    MSBuild.exe %_msb_args% %LoggingOpts% /m /nologo /bl /v:q /clp:ErrorsOnly
) else (
    MSBuild.exe %_msb_args% %LoggingOpts% /m /nologo /bl /v:m /clp:Summary /consoleLoggerParameters:ForceNoAlign
)
exit /b %ERRORLEVEL%
@echo off
setlocal enabledelayedexpansion
rem skip signing when using msb to build
set SkipSigning=true
set _subfolder=

set args=%*

if /i "%1" EQU "/skiploggingbuild" (set args=!args:~18! && goto :buildIt)

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
MSBuild.exe  %args% %LoggingOpts% /m /nologo /bl /v:m /clp:Summary /consoleLoggerParameters:ForceNoAlign
exit /b %ERRORLEVEL%
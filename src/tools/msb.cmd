@echo off
setlocal enabledelayedexpansion
rem skip signing when using msb to build
set SkipSigning=true
set _subfolder=

set _verbosity=m
set args=%*

if /i "%1" EQU "/q" (set _verbosity=q && set args=!args:~3! && shift)
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
rem Derive a meaningful binlog name from the first project-ish arg
rem (sln/slnx/vcxproj/csproj/vbproj/proj). Fall back to cwd basename
rem so a no-arg "msb" still produces something locatable.
set _title=
for %%a in (%args%) do (
    if not defined _title (
        set "_ext=%%~xa"
        if /I "!_ext!"==".sln"     set "_title=%%~na"
        if /I "!_ext!"==".slnx"    set "_title=%%~na"
        if /I "!_ext!"==".vcxproj" set "_title=%%~na"
        if /I "!_ext!"==".csproj"  set "_title=%%~na"
        if /I "!_ext!"==".vbproj"  set "_title=%%~na"
        if /I "!_ext!"==".proj"    set "_title=%%~na"
    )
)
if not defined _title for %%i in ("%CD%") do set "_title=%%~ni"
set "_binlog=%RepoRoot%\BuildOutput\%_title%.%_BuildArch%%_BuildType%.binlog"

MSBuild.exe  %args% %LoggingOpts% /m /nologo /bl:!_binlog! /v:%_verbosity% /clp:Summary /consoleLoggerParameters:ForceNoAlign
exit /b %ERRORLEVEL%
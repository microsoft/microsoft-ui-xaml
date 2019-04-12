@echo OFF
pushd %~dp0

setlocal ENABLEDELAYEDEXPANSION

if "%DevEnvDir%" == "" (
    echo DevEnvDir environment variable not set. Running DevCmd.cmd to get a developer command prompt...
    call %~dp0\DevCmd.cmd /PreserveContext
)

set TFS_SourcesDirectory=%~dp0
set XES_DFSDROP=%~dp0BuildOutput
set BUILD_BINARIESDIRECTORY=%~dp0BuildOutput
set VERSIONBUILDNUMBER=local
set VERSIONBUILDREVISION=10001

set BUILDALL=
set BUILDLEANMUXFORTHESTOREAPP=
set MUXFINAL=
set PROJECTPATH=

if "%1" == "" goto :usage

if "%1" == "all" (
    shift
    goto :ALL
)

if "%2" == "" goto :usage

REM Loose arguments

set BUILDPLATFORM=%1
if "%1" NEQ "x86" if "%1" NEQ "x64" if "%1" NEQ "arm" if "%1" NEQ "arm64" (
    echo ERROR: Invalid platform "%1"
    echo.
    goto :usage
)
shift
set BUILDCONFIGURATION=%1
if "%1" NEQ "debug" if "%1" NEQ "release" (
    echo ERROR: Invalid configuration "%1"
    echo.
    goto :usage
)
shift

REM echo BUILDPLATFORM=%BUILDPLATFORM% BUILDCONFIGURATION=%BUILDCONFIGURATION%
goto :MoreArguments

:ALL
REM echo Building all
set BUILDALL=1
goto :MoreArguments

:MoreArguments
if "%1" == "BuildLeanMuxForTheStoreApp" (
    REM echo LeanMux
    set BUILDLEANMUXFORTHESTOREAPP=1
    shift
    goto :MoreArguments
)
if "%1" == "/leanmux" (
    REM echo LeanMux
    set BUILDLEANMUXFORTHESTOREAPP=1
    shift
    goto :MoreArguments
)
if "%1" == "/muxfinal" (
    REM echo MUXFinal
    set MUXFINAL=1
    shift
    goto :MoreArguments
)
if "%1" == "/UseInsiderSDK" (
    REM echo UseInsiderSDK
    set USEINSIDERSDK=1
    shift
    goto :MoreArguments
)
if "%1" == "/UseInternalSDK" (
    REM echo UseInternalSDK
    set USEINTERNALSDK=1
    shift
    goto :MoreArguments
)
if "%1" == "/EmitTelemetryEvents" (
    REM echo EmitTelemetryEvents
    set EMITTELEMETRYEVENTS=1
    shift
    goto :MoreArguments
)
if "%1" == "/project" (
    set PROJECTPATH=%~2
    shift
    shift
    goto :MoreArguments
)
if "%1" NEQ "" (
    echo ERROR: Unknown argument "%1"
    goto :usage
)
goto :DoBuild

:DoBuild

REM
REM     NUGET Restore
REM
if "%PROJECTPATH%" NEQ "" (
	call .\Tools\NugetWrapper.cmd restore -MSBuildPath "%VSINSTALLDIR%\MSBuild\15.0\Bin" -NonInteractive %PROJECTPATH%
) else (
	call .\Tools\NugetWrapper.cmd restore -MSBuildPath "%VSINSTALLDIR%\MSBuild\15.0\Bin" -NonInteractive .\MUXControls.sln 
)

REM
REM     Build Solution
REM
set PreferredToolArchitecture=x64

set EXTRAMSBUILDPARAMS=
if "%BUILDLEANMUXFORTHESTOREAPP%" == "1" ( set EXTRAMSBUILDPARAMS=/p:BuildLeanMuxForTheStoreApp=true )
if "%MUXFINAL%" == "1" ( set EXTRAMSBUILDPARAMS=/p:MUXFinalRelease=true )
if "%USEINSIDERSDK%" == "1" ( set EXTRAMSBUILDPARAMS=/p:UseInsiderSDK=true )
if "%USEINTERNALSDK%" == "1" ( set EXTRAMSBUILDPARAMS=/p:UseInternalSDK=true )
if "%EMITTELEMETRYEVENTS%" == "1" ( set EXTRAMSBUILDPARAMS=/p:EmitTelemetryEvents=true )

REM Need an explicit full path to MSBuild.exe or it will fall back to 14.0 for some reason
set MSBUILDPATH=%VSINSTALLDIR%\MSBuild\15.0\Bin\MSBuild.exe

if "%PROJECTPATH%" NEQ "" (
    set MSBuildCommand="%MSBUILDPATH%" %PROJECTPATH% /p:platform="%BUILDPLATFORM%" /p:configuration="%BUILDCONFIGURATION%" /p:VisualStudioVersion="15.0" /flp:Verbosity=Diagnostic /fl /bl %EXTRAMSBUILDPARAMS% /verbosity:Minimal
    echo !MSBuildCommand!
    !MSBuildCommand!
) else (
    if "%BUILDALL%" == "" (
        set XES_OUTDIR=%BUILD_BINARIESDIRECTORY%\%BUILDCONFIGURATION%\%BUILDPLATFORM%\

        "%MSBUILDPATH%" .\MUXControls.sln /p:platform="%BUILDPLATFORM%" /p:configuration="%BUILDCONFIGURATION%" /p:VisualStudioVersion="15.0" /flp:Verbosity=Diagnostic /fl /bl %EXTRAMSBUILDPARAMS% /verbosity:Minimal

        if "%ERRORLEVEL%" == "0" call .\tools\MakeAppxHelper.cmd %BUILDPLATFORM% %BUILDCONFIGURATION%
    ) else (
        "%MSBUILDPATH%" .\build\BuildAll.proj /maxcpucount:12 /p:VisualStudioVersion="15.0" /flp:Verbosity=Diagnostic /fl /bl /verbosity:Minimal

        if "%ERRORLEVEL%" == "0" (
            call .\tools\MakeAppxHelper.cmd x86 release
            call .\tools\MakeAppxHelper.cmd x86 debug
            call .\tools\MakeAppxHelper.cmd x64 release
            call .\tools\MakeAppxHelper.cmd x64 debug
            call .\tools\MakeAppxHelper.cmd arm release
            call .\tools\MakeAppxHelper.cmd arm debug
            call .\tools\MakeAppxHelper.cmd arm64 release
            call .\tools\MakeAppxHelper.cmd arm64 debug
        )

        REM
        REM     PostBuild
        REM
        call .\tools\PostBuild.cmd

        REM
        REM     PkgGen
        REM
        REM call .\tools\pkggen.cmd
    )
)

goto :end

:usage
echo Usage:
echo    Specific platform ^& config:
echo        build.cmd ^<x86^|x64^|arm^|arm64^> ^<debug^|release^> [options]
echo.
echo    All platforms ^& configs:
echo        build.cmd all [options]
echo.
echo    Options:
echo        /leanmux - build lean mux for the store
echo        /muxfinal - build "final" bits which have the winmd stripped of experimental types
echo        /UseInsiderSDK - build using insider SDK
echo        /UseInternalSDK - build using internal SDK
echo        /EmitTelemetryEvents - build with telemetry events turned on
echo        /project ^<path^> - builds a specific project
echo.

:end


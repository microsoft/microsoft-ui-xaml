@echo off
if not "%winui_echo%" == "" @echo on
setlocal enabledelayedexpansion enableextensions

if "%1"=="/?" goto :usage

rem  This command file expects to be run from the same directory that contains it
pushd %~dp0

set _targetProduct=
set _targetProdTest=
set _targetSamples=
set _targetMux=
set _targetTest=
set _clean=
set _restore=
set _graph=
set _cache=
:: The /m flag to msbuild.exe controls how many instances of msbuild.exe are spawned.
:: The default of 4 was found to be a reasonable number to ensure good build times
:: without hitting out of memory issues.
set _procCount=/m:4
set _fake=
set _muxfinal=
set _version=3.0.0-dev
set _lowpriority=%XAMLBUILD_LOWPRIORITY%
set _verbosity=/verbosity:minimal
set _analyze=

:parseArgs
if "%1"=="/c" (
    set _clean=1
) else if "%1"=="prodtest" (
    set _targetProdTest=1
) else if "%1"=="product" (
    set _targetProduct=1
) else if "%1"=="samples" (
    set _targetSamples=1
) else if "%1"=="mux" (
    set _targetMux=1
) else if "%1"=="test" (
    set _targetTest=1
) else if "%1"=="all" (
    set _targetProdTest=1
    set _targetSamples=1
) else if "%1"=="/restore" (
    set _restore=1
) else if "%1" == "/graph" (
    set _graph=1
) else if "%1" == "/cache" (
    set _graph=1
    set _cache=1
) else if "%1"=="/fake" (
    set _fake=1
) else if "%1"=="/muxfinal" (
    set _muxfinal=1
) else if "%1" == "/lowpri" (
    set _lowpriority=1
) else if "%1" == "/normalpri" (
    set _lowpriority=0
) else if "%1" == "/verbose" (
    :: Normal is still pretty far from full verbosity but it can have more useful details than minimal and is not 
    :: nearly as verbose as detailed or diagnostic.
    set _verbosity=/verbosity:normal
) else if "%1"=="/b" (
    set _procCount=/m:2
) else if "%1"=="/analyze" (
    set _analyze=1
) else if "%1"=="/m" (
    set _procCount=/m
) else if "%1"=="/version" (
    set _version=%2
    shift
) else if "%1"=="" (
    goto:main
) else (
    echo Unrecognized option: %1
    goto :usage
)
shift
goto:parseArgs
:main

set _versionOption=/p:WinUIVersion=%_version%

set BUILDCMDSTARTTIME=%time%

rem If no targets specified, default to prodtest
if "%_targetProduct%%_targetProdTest%%_targetSamples%%_targetMux%%_targetTest%" == "" (
    set _targetProdTest=1
)

if "%_targetProdTest%" == "1" if "%_targetProduct%" == "1" (
    echo Target ProdTest includes Product, no need to use both.
    goto :eof
)

if "%_targetMux%" == "1" if "%_targetProduct%" == "1" (
    echo Target Product includes mux, no need to use both.
    goto :eof
)

if "%_targetMux%" == "1" if "%_targetProdTest%" == "1" (
    echo Target ProdTest includes mux, no need to use both.
    goto :eof
)

if "%_targetTest%" == "1" if "%_targetProdTest%" == "1" (
    echo Target ProdTest includes test, no need to use both.
    goto :eof
)

if "%_clean%"=="1" (
    call :callScript clean.cmd /all
    set _restore=1
)


if "%EnvironmentInitialized%" == "" (
    echo Please run init.cmd to ensure environment is properly initialized
    exit /b 1
)

rem When we build the XAML compiler as part of the same build that consumes the XAML compiler,
rem we can hit build breaks caused by MSBuild instances trying to produce DLLs that it's
rem already loaded.  To avoid that, we'll build the parts of the XAML compiler we need to build
rem before we build the solutions that consume them.
rem
rem First, determine if we have necessary files to build the compiler or if we should download
rem and use the compiler from a public WinUI package.
if EXIST "%reporoot%\src\XamlCompiler\BuildTasks\Microsoft\Lmr\XamlTypeUniverse.cs" (
    rem Build the compiler
    call :buildSolution %reporoot%\XamlCompilerPrerequisites.sln
    if ERRORLEVEL 1 goto:showDurationAndExit
) else (
    rem Download a recent compatible public WinUI package and copy the necessary compiler
    rem files into the right location.
    call :buildSolution %reporoot%\XamlCompilerPublic.csproj
    if ERRORLEVEL 1 goto:showDurationAndExit
    rem Ensure GenXbf.dll is available
    call :buildSolution %reporoot%\eng\BuildGenXbfForMSBuild\BuildGenXbfForMSBuild.csproj
    if ERRORLEVEL 1 goto:showDurationAndExit
)

if "%_targetMux%" == "1" (
    call :buildSolution %reporoot%\dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj
) else if "%_targetProduct%" == "1" (
   call :buildSolution %reporoot%\Microsoft.UI.Xaml-Product.sln
   if ERRORLEVEL 1 goto:showDurationAndExit
   call :buildSolution %reporoot%\controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj
   call :buildMockPackage
) else if "%_targetProdTest%" == "1" (
   rem If we have all files, build the full solution. Otherwise, build the one limited to OSS-
   rem available projects.
   if EXIST "%reporoot%\src\XamlCompiler\BuildTasks\Microsoft\Lmr\XamlTypeUniverse.cs" (
      call :buildSolution %reporoot%\dxaml\Microsoft.UI.Xaml.sln
      if ERRORLEVEL 1 goto:showDurationAndExit
      call :buildMockPackage
      call :buildSolution %reporoot%\controls\MUXControls.sln /restore
   ) else (
      rem Build the smaller solution
      call :buildSolution %reporoot%\dxaml\Microsoft.UI.Xaml.OSS.sln
      if ERRORLEVEL 1 goto:showDurationAndExit
      rem Can't yet build the test projects in MUXControls.sln
      rem No samples yet in OSS
      set _targetSamples=0
   )
) else if "%_targetTest%" == "1" (
   call :buildMockPackage
   call :buildSolution %reporoot%\controls\MUXControls.sln /restore
)
if ERRORLEVEL 1 goto:showDurationAndExit

if "%_targetSamples%" == "1" (
    rem If not fake and not building prodtest (so already built the mock), do so
    if "%_fake%%_targetProdTest%"=="" call :buildMockPackage

    rem Note that buildsamples.cmd does it's own check for "fake", so we just call it directly.
    call :callScript buildsamples.cmd %_versionOption%

    if "%_fake%"=="1" (
        rem buildsamples.cmd has its own /fake support, so we call it here to show the user what it will do.
        call buildsamples.cmd /fake %_versionOption%
    )
    if ERRORLEVEL 1 goto :showDurationAndExit
)
echo ---
echo BUILD SUCCEEDED.

git diff --exit-code "controls/dev/dll/XamlMetadataProviderGenerated.h" > nul
if ERRORLEVEL 1 (
    set _muxcIXMPChanged=1
)
if "%_muxcIXMPChanged%"=="1" (
    echo ---
    echo:
    echo Generated file 'controls/dev/dll/XamlMetadataProviderGenerated.h' has changed.
    echo If this is intended then use the following command to include it in your commit:
    echo     git add controls/dev/dll/XamlMetadataProviderGenerated.h
    echo: 
)



goto:showDurationAndExit

:buildSolution
set _solution=%1
shift
set _args=%*

for %%i in (%_solution%) do set _title=%%~ni

set _binlog=%_title%.%_BuildArch%%_BuildType%.binlog
set _options=/bl:!_binlog! !_verbosity! /clp:Summary,ForceNoAlign /ds:false !_procCount! %_args% %_versionOption% /nr:false

if "%_restore%"=="1" (
    echo Adding restore option...
    set _options=/restore /p:DisableWarnForInvalidRestoreProjects=true !_options!
)

if "%_graph%"=="1" (
    echo Adding graph option...
    set _options=!_options! /graph
)

if "%_cache%"=="1" (
    echo Enabling project cache
    set _options=!_options! /reportfileaccesses /p:MSBuildCacheEnabled=true /p:MSBuildCacheLogDirectory=%reporoot%\MSBuildCacheLogs\%_title%.%_BuildArch%%_BuildType%
)

if "%_lowpriority%"=="1" (
    set _options=!_options! /lowPriority
)

rem Define Configuration and Platform as global properties instead of just env vars to ensure consistent behavior with VS.
if defined Configuration set _options=!_options! /p:Configuration=%Configuration%
if defined Platform set _options=!_options! /p:Platform=%Platform%

if "%_muxfinal%"=="1" (
    echo Setting MUXFinalRelease...
    set _options=!_options! /p:MUXFinalRelease=true
)

if "%_analyze%"=="1" (
    echo Turning on Code Analysis...
    set _options=!_options! /p:ExperimentalAnalysis=true
)

set _command=call msbuild !_options!

if "%_fake%"=="1" (
    echo COMMAND: %_command%
    goto :eof
)

rem Clear the PSModulePath environment variable. This avoids issues when the caller is a version of
rem powershell that does not match the version of powershell that msbuild invokes. (Ex: pwsh.exe vs
rem powershell.exe)
rem This is safe because cmd.exe is its own process and setting the environment variable here will
rem not affect the parent process.
if NOT "%PSModulePath%" == "" (
    set PSModulePath=
)

%_command%

if ERRORLEVEL 1  (
    echo ---
    echo ERROR: buildSolution for !_solution! FAILED.  Binlog is here: !_binlog!
)

goto:eof

:callScript

if "%_fake%"=="1" (
    echo COMMAND: %*
    goto :eof
)

call %*

if ERRORLEVEL 1  (
    echo ---
    echo ERROR: callScript FAILED.
)
goto :eof


:buildMockPackage
if "%_fake%"=="1" (
    echo COMMAND: call %RepoRoot%\pack.cmd /version %_version%
    goto :eof
)
call %RepoRoot%\pack.cmd /version %_version%
if ERRORLEVEL 1 goto :showDurationAndExit
goto :eof


:showDurationAndExit
set BUILDCMDENDTIME=%time%
:: Note: The '1's in this line are to convert a value like "08" to "108", since numbers which
::       begin with '0' are interpreted as octal, which makes "08" and "09" invalid. Adding the
::       '1's effectively adds 100 to both sides of the subtraction, avoiding this issue.
::       Hours has a leading space instead of 0, so the '1's trick isn't used on that one.
set /a BUILDDURATION_HRS= %BUILDCMDENDTIME:~0,2%- %BUILDCMDSTARTTIME:~0,2%
set /a BUILDDURATION_MIN=1%BUILDCMDENDTIME:~3,2%-1%BUILDCMDSTARTTIME:~3,2%
set /a BUILDDURATION_SEC=1%BUILDCMDENDTIME:~6,2%-1%BUILDCMDSTARTTIME:~6,2%
set /a BUILDDURATION_HSC=1%BUILDCMDENDTIME:~9,2%-1%BUILDCMDSTARTTIME:~9,2%
if %BUILDDURATION_HSC% lss 0 (
    set /a BUILDDURATION_HSC=!BUILDDURATION_HSC!+100
    set /a BUILDDURATION_SEC=!BUILDDURATION_SEC!-1
)
if %BUILDDURATION_SEC% lss 0 (
    set /a BUILDDURATION_SEC=!BUILDDURATION_SEC!+60
    set /a BUILDDURATION_MIN=!BUILDDURATION_MIN!-1
)
if %BUILDDURATION_MIN% lss 0 (
    set /a BUILDDURATION_MIN=!BUILDDURATION_MIN!+60
    set /a BUILDDURATION_HRS=!BUILDDURATION_HRS!-1
)
if %BUILDDURATION_HRS% lss 0 (
    set /a BUILDDURATION_HRS=!BUILDDURATION_HRS!+24
)
:: Add a '0' at the start to ensure at least two digits. The output will then just
:: show the last two digits for each.
set BUILDDURATION_HRS=0%BUILDDURATION_HRS%
set BUILDDURATION_MIN=0%BUILDDURATION_MIN%
set BUILDDURATION_SEC=0%BUILDDURATION_SEC%
set BUILDDURATION_HSC=0%BUILDDURATION_HSC%
echo ---
echo Start time: %BUILDCMDSTARTTIME%. End time: %BUILDCMDENDTIME%
echo    Elapsed: %BUILDDURATION_HRS:~-2%:%BUILDDURATION_MIN:~-2%:%BUILDDURATION_SEC:~-2%.%BUILDDURATION_HSC:~-2%
endlocal
goto :eof

:usage
echo Usage:
echo     build.cmd [targets] [options]
echo.
echo    Available targets:
echo        prodtest ^(default^)  Builds product code and tests ^(no samples^)
echo        product             Builds product code only ^(no tests or samples, subset of prodtest^)
echo        mux                 Builds Microsoft.UI.Xaml.dll ^(subset of product^)
echo        test                Builds tests only ^(subset of prodtest^)
echo        samples             Builds sample apps
echo        all                 Builds the world
echo.
echo    Options:
echo        /c              Deletes bin, obj, temp, and packaging directories before building. 
echo                        Kills all existing instances of msbuild.exe, so should not be run alongside another build
echo        /restore        Add the Nuget restore option
echo        /graph          (experimental: requires VS 17.7+) Perform graph-based MSBuild scheduling. See: https://github.com/dotnet/msbuild/blob/main/documentation/specs/static-graph.md
echo        /cache          (experimental: requires VS 17.8+) Perform a build with MSBuild project caching. Implies /graph. See: https://github.com/dotnet/msbuild/blob/main/documentation/specs/project-cache.md
echo        /b              Background mode (spawn fewer instances of msbuild.exe; 2 instead of 4).
echo        /m              Passes the /m flag to msbuild. This will spawn as many msbuild.exe instances as you have cores on your machine.
echo                        This could result in faster builds, but may result in failures due to out of memory. Do not combine with /b.
echo        /muxfinal       Set MUXFinalRelease to true to simulate a release build
echo        /fake           Don't actually do the build--just tell me what you're going to build.
echo        /lowpri         Launch MSBuild using below normal priority (default if environment variable XAMLBUILD_LOWPRIORITY = 1)
echo        /normalpri      Launch MSBuild using normal priority (default if environment variable XAMLBUILD_LOWPRIORITY ^^!= 1)
echo        /version [ver]  Override WinUIVersion property (default is %_version%)
rem echo        /local          Builds without using IncrediBuild.
echo.
echo    There is also a build command in the controls directory, with options specific to building controls
echo.

exit /b 1

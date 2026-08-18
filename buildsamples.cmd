@rem Copyright (c) Microsoft Corporation.
@rem Licensed under the MIT License.
@echo off
if not "%winui_echo%" == "" @echo on
setlocal enabledelayedexpansion enableextensions

set _samplesFake=
if "%1"=="/fake" (
    set _samplesFake=1
    shift
)
set _scriptArgs=%*

REM This script uses the VS16.8 build tools installed with the repository, or if running in 16.8 command prompt
REM will just use that

REM We can't use the /m switch for C++ Desktop apps because the .wapproj
REM will cause MSBuild to be invoked on the referenced project with an additional
REM property (GenerateAppxPackageOnBuild) compared to the top-level MSBuild invocation
REM of the same project triggered by the .sln.metaproj. Because of the different sets of
REM properties, the second MSBuild process won't defer to the first (they don't think they're
REM identical) and we can end up trying to build the same project twice at the same time.
REM This is the case for our C++ Desktop apps (but not C# Desktop) because our C++ Desktop
REM project system is derived from the C++ UWP project system; there's logic in the WAP build
REM task (Microsoft.Build.DesktopBridge.SetPublishProperties) that will add that additional
REM property if the project has a TargetPlatformIdentifier=UAP.

REM This app is a single-project MSIX app, so it only emits its .msix when published.
call :buildSamplesSolution %reporoot%\Samples\WinUICsDesktopSampleApp\WinUICsDesktopSampleApp.sln /m /t:Publish
if ERRORLEVEL 1 goto:eof
call :buildSamplesSolution %reporoot%\Samples\WinUICppDesktopSampleApp\WinUICppDesktopSampleApp.sln
if ERRORLEVEL 1 goto:eof
call :buildSamplesSolution %reporoot%\Samples\WinUICppIsland2SampleApp\WinUICppIsland2SampleApp.sln
if ERRORLEVEL 1 goto:eof
call :buildSamplesSolution %reporoot%\Samples\WinUICppIslandsSampleApp\WinUICppIslandsSampleApp.sln
if ERRORLEVEL 1 goto:eof
call :buildSamplesSolution %reporoot%\Samples\DisableXamlGeneratedMain\DisableXamlGeneratedMain.sln
if ERRORLEVEL 1 goto:eof
REM The C# projects in the solution above are single-project MSIX apps and so only emit their MSIX 
REM when published
call :buildSamplesSolution %reporoot%\Samples\DisableXamlGeneratedMain\Cs\DisableXamlGeneratedMainCs.csproj /t:Publish /p:PublishProfile=win-%BUILDPLATFORM%.pubxml
if ERRORLEVEL 1 goto:eof
call :buildSamplesSolution %reporoot%\Samples\DisableXamlGeneratedMain\CsNoCtor\DisableXamlGeneratedMainNoCtorCs.csproj /t:Publish /p:PublishProfile=win-%BUILDPLATFORM%.pubxml
if ERRORLEVEL 1 goto:eof
if "%Configuration%"=="Release" (
    REM PublishAot must be specified at restore, as it conditionally generates nuget.g.* imports
    set _publishAot=/p:PublishAot=true
)
call :buildSamplesSolution %reporoot%\Samples\WinUIGallery\WinUIGallery.slnx /m /p:PublishProfile=win-%BUILDPLATFORM%.pubxml /t:Publish %_publishAot% 
if ERRORLEVEL 1 goto:eof
call :buildSamplesSolution %reporoot%\Samples\WinUIDesktop\NativeDX12DesktopSample.sln
if ERRORLEVEL 1 goto:eof
call :buildSamplesSolution %reporoot%\Samples\ChartApp\ChartApp.sln
if ERRORLEVEL 1 goto:eof
call :buildSamplesSolution %reporoot%\Samples\ChartApp\ChartAppCsUnpackaged\ChartAppCsUnpackaged.csproj /t:Publish /p:PublishProfile=win-%BUILDPLATFORM%.pubxml
if ERRORLEVEL 1 goto:eof
REM The C# packaged Chart app is a single-project MSIX app, so it only emits its .msix when published.
call :buildSamplesSolution %reporoot%\Samples\ChartApp\ChartAppCsPackaged\ChartAppCsPackaged.csproj /t:Publish /p:PublishProfile=win-%BUILDPLATFORM%.pubxml
if ERRORLEVEL 1 goto:eof

exit /b 0

:buildSamplesSolution
set _solution=%1
set _titleExtension=%2

if "%_titleExtension%" neq "Universal" (
    set _titleExtension=
)

for %%i in (%_solution%) do set _title=%%~ni

if "%_titleExtension%" neq "" (
    set _title=%_title%.%_titleExtension%
    for /f "tokens=2,* delims= " %%a in ("%*") do set _args=%%b
) else (
    for /f "tokens=1,* delims= " %%a in ("%*") do set _args=%%b
)

set _binlog=%reporoot%\BuildOutput\%_title%.%_BuildArch%%_BuildType%.binlog
set _options=/restore /bl:!_binlog! %_scriptArgs% %_args% 
set _command=call msbuild !_solution! !_options!

if "%_samplesFake%"=="1" (
    echo +   COMMAND: !_command!
    goto :eof
)

echo %_command%
%_command%

if ERRORLEVEL 1  (
    echo ---
    echo ERROR: buildSolution for !_solution! FAILED.  Binlog is here: !_binlog!
)

goto:eof

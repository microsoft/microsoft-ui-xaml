@echo off
rem Scripts cannot set enviroment variables because of an older handler in the agent that is not compatible with some pipeline containers
rem Hence to set enviroment variables, use SetEnviromentVariable

pushd %~dp0

PATH %PATH%;%~dp0\tools
set PrereleaseArg=

setlocal enableextensions enabledelayedexpansion
set _ARGS=

rem We are targeting VS 2019 (version 16.x) or later
set VsVersion=16.0

if exist %temp%\WinUI.PreserveContext.marker del %temp%\WinUI.PreserveContext.marker
:ParseArgs
if "%1" EQU "" (
    goto :DoneParsing
) else if /i "%1" EQU "/PreserveContext" (
    echo. > %temp%\WinUI.PreserveContext.marker
) else if /i "%1" EQU "/Prerelease" (
    set PrereleaseArg=-prerelease
) else (
    set _ARGS=%_ARGS% %1=%2
    shift
)
shift
goto ParseArgs

:DoneParsing
set vswhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %vswhere% (echo VSWhere.exe not found. Install MSBuild first from OneTimeSetup.cmd && exit /b 1)

rem Try MSBuild first
set MSBuildInstallpath=
for /f "tokens=*" %%a in ('%vswhere% -products Microsoft.VisualStudio.Product.BuildTools -property InstallationPath %PrereleaseArg% -latest -version %VsVersion%') do set MSBuildInstallPath=%%a

if "%MSBuildInstallPath%" EQU "" (
    rem We didn't find MSBuild, try a full VSexit
    for /f "tokens=*" %%a in ('%vswhere% -requires Microsoft.Component.MSBuild -property InstallationPath %PrereleaseArg% -latest -version %VsVersion%') do set MSBuildInstallPath=%%a
    )

if "%MSBuildInstallPath%" EQU "" (
    echo Could not find an MSBuild install, exiting
    if defined AGENT_NAME (
        call:PrintVsWhere
    )
    exit /b 2
)

rem In the pipeline, vswhere cannot locate VS build tools installed as part of the pipeline run without a restart.  If the .buildtools directory exists,
rem implying the pipeline installed VS build tools, use them instead.

if exist %~dp0.buildtools (
  echo Using MSBuild from .buildtools directory...
  set MSBuildInstallPath=%~dp0.buildtools
) else (
  echo .buildtools directory not found, using msbuild from vswhere...
)

endlocal & (
    echo "Initializing VS command prompt from %MSBuildInstallPath%\Common7\Tools\VsDevCmd.bat ..."
    call "%MSBuildInstallPath%\Common7\Tools\VsDevCmd.bat" /no_logo %_ARGS%
)

rem These variables are set in VsDevCmd.bat but we need to set them again with SetEnviromentVariable to work in pipeline containers
call:SetEnviromentVariable VCToolsInstallDir "%VCToolsInstallDir%"
call:SetEnviromentVariable VCToolsRedistDir "%VCToolsRedistDir%"
call:SetEnviromentVariable ExtensionSdkDir "%ExtensionSdkDir%"

if not exist %temp%\WinUI.PreserveContext.marker (
    echo Starting cmd /k
    cmd /k "cd /d %RepoRoot%"
)

if exist %temp%\WinUI.PreserveContext.marker (del %temp%\WinUI.PreserveContext.marker)

goto:eof

:PrintVsWhere
echo on
%vswhere% -products Microsoft.VisualStudio.Product.BuildTools -property InstallationPath %PrereleaseArg%
%vswhere% -requires Microsoft.Component.MSBuild -property InstallationPath %PrereleaseArg%
@echo off
exit /b

rem This function is used to set enviroment variables in Azure Pipelines
rem It will call task.setVariable on top of the regular "set" command if /pipeline is passed in.
:SetEnviromentVariable
set _varName=%~1
set _varValue=%~2
set %_varName%=%_varValue%
if not "%Pipeline%"=="true" (
    @REM I don't know why but if I put the echo line in here, it modifies the value, just batch things
    exit /b 0
)
echo ##vso[task.setVariable variable=%_varName%]%_varValue%
exit /b 0

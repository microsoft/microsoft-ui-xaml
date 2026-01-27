@echo off
rem Scripts cannot set enviroment variables because of an older handler in the agent that is not compatible with some pipeline containers
rem Hence to set enviroment variables, use SetEnviromentVariable

if not "%winui_echo%" == "" @echo on

set RepoRoot=%~dp0
rem Remove the trailing backslash
set RepoRoot=%RepoRoot:~0,-1%

set EnvOnly=
set Verbose=
set NoTitle=
set Pipeline=

rem In case we run init.cmd multiple times, we don't want to keep our additions to PATH around.
rem We'll save the original value of PATH and restore it on future calls to init.cmd.
rem Given that PATH can get super long, we'll split this into multiple statements since having both "set"
rem statements on the same line can cause cmd to complain the command is too large.
if "%_OriginalPathBeforeInit%" neq "" goto :OriginalPathSet
set _OriginalPathBeforeInit=%PATH%
goto :DoneSettingPath
:OriginalPathSet
set PATH=%_OriginalPathBeforeInit%
:DoneSettingPath

set x86=
set amd64=
set ARM64=
set ARM64EC=
set fre=
set chk=
set _BuildArch=
set _BuildType=
set _DotNetMoniker=net8.0
set _archIsSet=

:parseArgs
if /i "%1"=="" (
    goto:doneParsingArgs
) else if /i "%1"=="/envonly" (
    set EnvOnly=true
) else if /i "%1"=="/pipeline" (
    set EnvOnly=true
    set Pipeline=true
) else if /i "%1"=="/verbose" (
    set Verbose=-Verbosity normal
) else if /i "%1"=="/notitle" (
    set NoTitle=1
) else if /i "%1"=="x86chk" (
    set x86=1
    set chk=1
    set _archIsSet=1
) else if /i "%1"=="x86fre" (
    set x86=1
    set fre=1
    set _archIsSet=1
) else if /i "%1"=="x64fre" (
    set amd64=1
    set fre=1
    set _archIsSet=1
) else if /i "%1"=="x64chk" (
    set amd64=1
    set chk=1
    set _archIsSet=1
) else if /i "%1"=="amd64fre" (
    set amd64=1
    set fre=1
    set _archIsSet=1
) else if /i "%1"=="amd64chk" (
    set amd64=1
    set chk=1
    set _archIsSet=1
) else if /i "%1"=="arm64fre" (
    set ARM64=1
    set fre=1
    set _archIsSet=1
) else if /i "%1"=="arm64chk" (
    set ARM64=1
    set chk=1
    set _archIsSet=1
) else if /i "%1"=="arm64ecfre" (
    set ARM64EC=1
    set fre=1
    set _archIsSet=1
) else if /i "%1"=="arm64ecchk" (
    set ARM64EC=1
    set chk=1
    set _archIsSet=1
) else if /i "%1"=="net6" (
    set _DotNetMoniker=net6.0
) else if /i "%1"=="net7" (
    set _DotNetMoniker=net7.0
) else if /i "%1"=="net8" (
    set _DotNetMoniker=net8.0
)  else (
    echo Syntax:    %0 ^<arch^>^<flavor^> [^<options^>] [^<toolset^>]
    echo.
    echo            ^<arch^> :          x86 ^| ^(x64^|amd64^) ^| ARM64
    echo            ^<flavor^> :        chk ^| fre
    echo            ^<options^> :       /verbose, /envonly, /notitle
    exit /b 1
)

rem To set enviroment variables, use SetEnviromentVariable
rem See top of file for reasoning
call:SetEnviromentVariable _DotNetMoniker %_DotNetMoniker%
call:SetEnviromentVariable RepoRoot "%RepoRoot%"

shift
goto:parseArgs

:doneParsingArgs
if "%_archIsSet%"=="" (
    set amd64=1
    set chk=1
)
set _archIsSet=

if "%x86%"=="1" (
    call:SetEnviromentVariable _BuildArch x86
    call:SetEnviromentVariable Platform Win32
)
if "%amd64%"=="1" (
    call:SetEnviromentVariable _BuildArch amd64
    call:SetEnviromentVariable Platform x64
)
if "%ARM64%"=="1" (
    call:SetEnviromentVariable _BuildArch ARM64
    call:SetEnviromentVariable Platform ARM64
)
if "%ARM64EC%"=="1" (
    call:SetEnviromentVariable _BuildArch ARM64EC
    call:SetEnviromentVariable Platform ARM64EC
)
if not "%Pipeline%"=="true" (
    call:SetEnviromentVariable BUILDPLATFORM %Platform%

    if "%x86%"=="1" (
        call:SetEnviromentVariable BUILDPLATFORM x86
    )
)


if "%fre%"=="1" (
    call:SetEnviromentVariable _BuildType fre
    call:SetEnviromentVariable Configuration Release
)
if "%chk%"=="1" (
    call:SetEnviromentVariable _BuildType chk
    call:SetEnviromentVariable Configuration Debug
)

if "%DevEnvDir%" == "" (
    echo DevEnvDir environment variable not set. Running DevCmd.cmd to get a developer command prompt...
    if "%ARM64EC%"=="1" (
        call %RepoRoot%\DevCmd.cmd /PreserveContext /prerelease -arch=amd64 -host_arch=amd64
    ) else (
        call %RepoRoot%\DevCmd.cmd /PreserveContext /prerelease -arch=%_BuildArch% -host_arch=amd64
    )
    if errorlevel 1 (echo Could not set up a developer command prompt && exit /b %ERRORLEVEL%)
)

if "%VisualStudioVersion%" == "16.0" (echo Visual Studio 2019 is not supported. && exit /b /1)

set PATH=%RepoRoot%\.buildtools\MSBuild\Current\Bin\amd64;%RepoRoot%\.tools;%RepoRoot%\.tools\VSS.NuGet.AuthHelper;%RepoRoot%\tools;%RepoRoot%\dxaml\scripts;%PATH%

rem If we have init'd from a VS developer command prompt, we should use its tooling instead of the VS build tools installed with the repo
call:AddPathIfExists "%VSINSTALLDIR%\MSBuild\Current\Bin\amd64"

call:SetEnviromentVariable BuildArtifactsDir %RepoRoot%\BuildOutput

call:SetEnviromentVariable BinRoot %BuildArtifactsDir%\Bin

call:SetEnviromentVariable BuildOutputRoot %BuildArtifactsDir%\Obj

call:SetEnviromentVariable TEMP %BuildArtifactsDir%\Temp\%_BuildArch%%_BuildType%

call:SetEnviromentVariable TMP %TEMP%

if not exist %TEMP% mkdir %TEMP%

call:SetEnviromentVariable DOTNET_ROOT %RepoRoot%\.dotnet

call:SetEnviromentVariable DOTNET_ROOT_x86 %RepoRoot%\.dotnet\x86

call:SetEnviromentVariable DOTNET_INSTALL_DIR %RepoRoot%\.dotnet

call:SetEnviromentVariable DOTNET_MULTILEVEL_LOOKUP 0

set PATH=%DOTNET_ROOT%;%DOTNET_ROOT_x86%;%PATH%

call %RepoRoot%\scripts\init\SetupDotNetFiles.cmd %RepoRoot%

powershell -ExecutionPolicy Bypass -NoProfile %RepoRoot%\scripts\GenerateTestPfx.ps1 %RepoRoot%\build\WinUITest.pfx

if "%EnvOnly%"=="" (
    rem For pipeline builds, submodules are checked out with authentication elsewhere
    rem For dev builds, ensure that submodules are populated with latest commits
    git submodule update --init --recursive
    powershell -ExecutionPolicy Bypass -NoProfile -File %RepoRoot%\scripts\init\Initialize-Restore.ps1 -RepoRoot %RepoRoot% %Verbose%
)

if "%ARM64EC%"=="1" (
    if exist %RepoRoot%\scripts\MockArm64ECFolder.ps1 (
        powershell -ExecutionPolicy Bypass -NoProfile -File %RepoRoot%\scripts\MockArm64ECFolder.ps1
    )
)

xcopy /d /y %RepoRoot%\scripts\winui.natvis "%USERPROFILE%\My Documents\Visual Studio 2022\Visualizers\" >nul 2>&1

set EnvironmentInitialized=1

if "%NoTitle%"=="" (
   title DCPP %RepoRoot% - %_BuildArch%%_BuildType%
)
doskey /macrofile=%RepoRoot%\scripts\aliases

goto:eof


:AddPathIfExists
rem Remove quotes we used above
set _ToAdd=%1
if not exist %_ToAdd%  (
    goto:PathNotFound %1
)
set PATH=%_ToAdd:~1,-1%;%PATH%
set _ToAdd=
exit /b 0

:PathNotFound
set _A=%1
echo Could not find path: %_A:~1,-1%
set _A=
exit /b 4

rem This function is used to set enviroment variables in Azure Pipelines
rem It will call task.setVariable on top of the regular "set" command if /pipeline is passed in.
rem See comment at top of file for reasoning
:SetEnviromentVariable
set _varName=%~1
set _varValue=%~2
set %_varName%=%_varValue%
if not "%Pipeline%"=="true" (
    exit /b 0
)
echo ##vso[task.setVariable variable=%_varName%]%_varValue%
exit /b 0

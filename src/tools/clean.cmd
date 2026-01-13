@echo off
setlocal enableextensions
if "%reporoot%" EQU "" (echo %%RepoRoot%% not defined - run init.cmd first && exit /b 1)
set subfolder=\%_BuildArch%%_BuildType%

set controlsSubfolder=\%Configuration%\

if "%Platform%" == "Win32" (
    set controlsSubfolder=%controlsSubfolder%x86
) else (
    set controlsSubfolder=%controlsSubfolder%%Platform%
)

rem These processes can be left running after a build, so we need to kill them if so,
rem since they can have handles open on files in the folders we're about to clean.
call :callScript taskkill /im msbuild.exe /f 2> nul
call :callScript taskkill /im VBCSCompiler.exe /f 2> nul

:parseArgs
if "%1"=="/all" (
    set subfolder=
    set controlsSubfolder=
) else if "%1"=="/packages" (
    set deletePackages=1
) else if "%1"=="/?" (
    echo %0 [/all] [/packages]
    echo Deletes build output for the current architecture and flavor ^(%_BuildArch%%_BuildType%^)
    echo.
    echo /all         deletes output for all architectures.
    echo /packages    deletes nuget packages cache.
    exit /b 0
) else if "%1"=="" (
    goto:main
) else (
    echo Unrecognized option: %1
    exit /b 2
)
shift
goto:parseArgs

:main
rem MSBuild.exe can hold onto some things we'll be trying to delete, so let's kill that process first.
taskkill /f /im MSBuild.exe > nul 2>&1

rem Also, VBCSCompiler.
taskkill /f /im VBCSCompiler.exe > nul 2>&1

rem Some things fail if we don't have a temp folder, so we'll put back any that were present before cleaning
rem to make sure that everything's still in good working order.
set LocalTempDir=%LOCALAPPDATA%\Temp
if not exist %LocalTempDir% md %LocalTempDir%

set buildoutput=BuildOutput

if exist %reporoot%\%buildoutput%\Temp (
    dir /a:d /b %reporoot%\%buildoutput%\Temp > %LocalTempDir%\tempSubFolders.txt
)

set objdir=%buildoutput%\obj%subfolder%
set bindir=%buildoutput%\bin%subfolder%
set tempdir=%buildoutput%\temp%subfolder%
set packagingdir=%buildoutput%\packaging%subfolder%
set mockdir=%buildoutput%\WindowsAppSDK
set testpayloaddir=TestPayload%subfolder%

echo Deleting %bindir%...
if exist %reporoot%\%bindir% (rd /s /q %reporoot%\%bindir%)

echo Deleting %objdir%...
if exist %reporoot%\%objdir% (rd /s /q %reporoot%\%objdir%)

echo Deleting %tempdir%...
if exist %reporoot%\%tempdir% (rd /s /q %reporoot%\%tempdir%)

echo Deleting %packagingdir%...
if exist %reporoot%\%packagingdir% (rd /s /q %reporoot%\%packagingdir%)

echo Deleting %mockdir%...
if exist %reporoot%\%mockdir% (rd /s /q %reporoot%\%mockdir%)

echo Deleting %testpayloaddir%...
if exist %reporoot%\%testpayloaddir% (rd /s /q %reporoot%\%testpayloaddir%)

if exist %LocalTempDir%\tempSubFolders.txt (
    for /f %%A in (%LocalTempDir%\tempSubFolders.txt) do (
        echo Recreating %buildoutput%\Temp\%%A...
        md %reporoot%\%buildoutput%\Temp\%%A
    )
    
    del %LocalTempDir%\tempSubFolders.txt
)

if "%deletePackages%"=="1" (
    if exist %reporoot%\packages (
        echo Deleting packages...
        rd /s /q %reporoot%\packages
    )
    
    if exist %reporoot%\src\packages (
        echo Deleting src\packages...
        rd /s /q %reporoot%\src\packages
    )
    
    if exist %reporoot%\src\XamlCompiler\packages (
        echo Deleting src\XamlCompiler\packages...
        rd /s /q %reporoot%\src\XamlCompiler\packages
    )
    
    echo Deleting %reporoot%\PackageStore...
    git clean -df %reporoot%\PackageStore
    echo Deleted NuGet packages - make sure to re-run init.cmd to restore packages.
)
endlocal
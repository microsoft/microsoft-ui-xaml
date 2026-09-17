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
rem Processes may belong to other repos or be idle compiler servers. Never kill them.
powershell.exe -NoProfile -Command "$ErrorActionPreference = 'Stop'; $processes = Get-CimInstance Win32_Process -Filter \"Name='MSBuild.exe' OR Name='VBCSCompiler.exe' OR Name='cl.exe' OR Name='link.exe'\"; if ($processes) { Write-Host 'WARNING: Build processes or compiler servers are running. They may be idle or belong to another repo.'; $processes | ForEach-Object { Write-Host ('  {0} (PID {1})' -f $_.Name, $_.ProcessId) }; Write-Host 'Continuing without stopping them. Do not clean this repo while a build in it is running.' }"
if errorlevel 1 (
    echo ERROR: Could not check for running build processes. Cleanup was not started.
    exit /b 1
)

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

for %%D in ("%bindir%" "%objdir%" "%tempdir%" "%packagingdir%" "%mockdir%" "%testpayloaddir%") do (
    call :deleteDirectory "%reporoot%\%%~D"
    if errorlevel 1 exit /b 1
)

if exist %LocalTempDir%\tempSubFolders.txt (
    for /f %%A in (%LocalTempDir%\tempSubFolders.txt) do (
        echo Recreating %buildoutput%\Temp\%%A...
        if not exist "%reporoot%\%buildoutput%\Temp\%%A" md "%reporoot%\%buildoutput%\Temp\%%A"
        if not exist "%reporoot%\%buildoutput%\Temp\%%A" (
            echo ERROR: Could not recreate temporary directory %%A.
            exit /b 1
        )
    )
    
    del %LocalTempDir%\tempSubFolders.txt
)

if "%deletePackages%"=="1" (
    for %%D in ("packages" "src\packages" "src\XamlCompiler\packages") do (
        call :deleteDirectory "%reporoot%\%%~D"
        if errorlevel 1 exit /b 1
    )
    
    echo Deleting %reporoot%\PackageStore...
    git clean -df %reporoot%\PackageStore
    if errorlevel 1 exit /b 1
    echo Deleted NuGet packages - make sure to re-run init.cmd to restore packages.
)
endlocal & exit /b 0

:deleteDirectory
echo Deleting %~1...
if exist "%~1" rd /s /q "%~1"
if exist "%~1" (
    echo ERROR: Could not delete "%~1". Check for locked files or a build using this directory.
    exit /b 1
)
exit /b 0
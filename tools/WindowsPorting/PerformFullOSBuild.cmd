@echo off
setlocal enableDelayedExpansion

set PATH=C:\Program Files\GitTelemetry;C:\Program Files\Git\cmd;C:\Program Files\GVFS;%PATH%
set ACCESSTOKEN=%1
set BUILDONLY=%2
set DEPControlsRepoRoot=%BUILD_SOURCESDIRECTORY%
set OSRepoRoot=C:\os
set OSRepoUri=https://microsoft.visualstudio.com/os/_git/os
set OfficialBranch=official/rs_onecore_dep_uxp
set PortingBranch=user/uxpc/DEPControlsPortingBranch
set StagingBranch=user/uxpc/DEPControlsPortStagingBranch

set AUTO_DEP=1
set SKIP_SPKG=1

rem The tracer is currently hosed in new branches due to the ongoing ES outages.
rem Until it's working again, we need to turn off the tracer or else
rem all builds will fail due to tracer failures.
set TRACER_ENABLED=0

rem This define can sometimes have multiple lines in it, which makes analyzer.exe unhappy.
rem We never use its value, so we can just delete its contents.
set BUILD_SOURCEVERSIONMESSAGE=

echo.
echo ------------------------
echo   PRE-PORT ENVIRONMENT
echo ------------------------
echo.

pushd \

echo Environment variables:
echo.

set

echo.
echo System git configuration:
echo.

call git config --system -l

echo.
echo Global git configuration:
echo.

call git config --global -l

popd

echo.
echo -------------------------------
echo   STEP 1: SET UP WINDOWS REPO
echo -------------------------------

set StartTime=%time%

echo.
echo Setting Git name and email address...
echo.

pushd \

rem A blank value for %BUILD_REQUESTEDFOREMAIL% implies that this was an automated port.
rem Otherwise, it was scheduled manually, and we should note who did so.

if "%BUILD_REQUESTEDFOREMAIL%" == "" (
    echo user.name -^> "UXP Controls Automated Porting System"
    call git config --global user.name "UXP Controls Automated Porting System"
    echo user.email -^> "uxpc@microsoft.com"
    call git config --global user.email "uxpc@microsoft.com"
) else (
    echo user.name -^> "%BUILD_REQUESTEDFOR%"
    call git config --global user.name "%BUILD_REQUESTEDFOR%"
    echo user.email -^> "%BUILD_REQUESTEDFOREMAIL%"
    call git config --global user.email "%BUILD_REQUESTEDFOREMAIL%"
)

popd

echo.
echo Registering access tokens...
echo.

cmdkey /generic:LegacyGeneric:target=git:https://microsoft.visualstudio.com /user:"Personal Access Token" /password:%ACCESSTOKEN%
cmdkey /generic:LegacyGeneric:target=vpack:https://microsoft.vsblob.visualstudio.com /user:"PAT" /password:%ACCESSTOKEN%
cmdkey /generic:LegacyGeneric:target=vpack:https://microsoft.artifacts.visualstudio.com /user:"PAT" /password:%ACCESSTOKEN%

echo.
echo Access tokens registered:
echo.

cmdkey /list

pushd

if not exist "%ProgramFiles%\GVFS\GVFS.exe" (
    echo ##vso[task.logissue type=error;] GVFS needs to be installed before we can sync to Windows.  Please install GVFS.
    set ERRORLEVEL=1
    goto Cleanup
)

if not exist "%OSRepoRoot%" mkdir %OSRepoRoot%
cd %OSRepoRoot%

rem
rem SYNTAX NOTE:
rem
rem     "||" within the context of a batch file means, "Execute the right side only if the left side has failed."
rem     Since "exit" from within a function call will merely exit that function, not the batch file entirely,
rem     we need to add "|| goto Cleanup" in order to make sure we actually for reals skip everything if there's an error
rem     and jump straight to the :Cleanup label.
rem

if not exist "%OSRepoRoot%\src" (
    rem For some reason, "rm * -rf" takes *forever* to complete.  As such, we need to manually remove all of the directories,
    rem and then delete the files left over.

    echo.
    echo Detected no OS repo at %OSRepoRoot%.  Deleting current contents and then cloning %OSRepoUri%...
    echo.

    for /f "delims=" %%I in ('dir /B /AD') do ( rd /s /q %%I )
    rm * -f
    call :CheckErrorLevel "Deleting current contents" || goto Cleanup

    gvfs clone %OSRepoUri% %OSRepoRoot%
) else (
    echo.
    echo Found OS repo at %OSRepoRoot%\src.  Mounting...
    echo.

    gvfs mount
)

set RepoIsMounted=false

echo.

:MountWaitLoopBegin

if "%RepoIsMounted%" == "false" (
    echo Waiting for repo to be mounted...
    for /f "usebackq delims=" %%I in (`gvfs status ^| findstr "Mount status: Ready"`) do ( set RepoIsMounted=true )
    goto :MountWaitLoopBegin
)

echo.
echo Repo mounted successfully.

popd

echo.
echo Setting up Windows repo succeeded!
echo.
echo Started: %StartTime%
echo Ended:   %time%
echo.
echo -----------------------------
echo   STEP 2: SYNC WINDOWS REPO
echo -----------------------------

set StartTime=%time%

pushd %OSRepoRoot%\src

echo.
echo Starting VPack daemons...
echo.

start %OSRepoRoot%\Tools\VPack\Microsoft.Engineering.FileVirtualization.Daemon.exe
start %OSRepoRoot%\Tools\VPack\Microsoft.Engineering.MonitoringAgent.RunAgentDaemon.exe

echo.
echo Starting Razzle...
echo.

call %OSRepoRoot%\src\tools\razzle.cmd x86fre no_oacr
call :CheckErrorLevel "Razzle start" || goto Cleanup
call doublecheck /awd /rc

set WindowsDir=%SDXROOT%\onecoreuap\windows\dxaml

echo.
echo Setting up porting branch...
echo.

call git fetch
call git checkout origin/%PortingBranch%

popd

echo.
echo Syncing Windows repo succeeded!
echo.
echo Started: %StartTime%
echo Ended:   %time%
echo.
echo ------------------------------
echo   STEP 3: PERFORM FULL BUILD
echo ------------------------------

set StartTime=%time%

echo.

pushd %WindowsDir%

echo.
echo Building the synced changes...
echo.

call buildxaml.cmd /superclean
call :ReportBuildFailures
call :CheckErrorLevel "Building branch" || goto Cleanup

popd

echo.
echo Building changes succeeded!
echo.
echo Started: %StartTime%
echo Ended:   %time%
echo.

:Cleanup
exit /b %ERRORLEVEL%
goto:eof

:ReportBuildFailures
rem If there were any build failures, let's output those as VSTS errors
rem so they're easier to see in the build definition output.
rem Otherwise, people will need to go to the child build definition
rem to find it.
if exist buildfre.err (
    for /F "delims=" %%I in (buildfre.err) do ( echo ##vso[task.logissue type=error;] %%I )
)

if exist idl\buildfre.err (
    for /F "delims=" %%I in (idl\buildfre.err) do ( echo ##vso[task.logissue type=error;] %%I )
)

if exist test\buildfre.err (
    for /F "delims=" %%I in (test\buildfre.err) do ( echo ##vso[task.logissue type=error;] %%I )
)

if exist ..\xcp\dxaml\themes\buildfre.err (
    for /F "delims=" %%I in (..\xcp\dxaml\themes\buildfre.err) do ( echo ##vso[task.logissue type=error;] %%I )
)

if exist ..\xcp\dxaml\dllsrv\winrt\res\buildfre.err (
    for /F "delims=" %%I in (..\xcp\dxaml\dllsrv\winrt\res\buildfre.err) do ( echo ##vso[task.logissue type=error;] %%I )
)

if exist ..\xcp\tools\xcptypesautogen\runcodegen\buildfre.err (
    for /F "delims=" %%I in (..\xcp\tools\xcptypesautogen\runcodegen\buildfre.err) do ( echo ##vso[task.logissue type=error;] %%I )
)
goto:eof

:CheckErrorLevel
if ERRORLEVEL 1 (
    echo ##vso[task.logissue type=error;] %~1 FAILED.  Skipping to cleanup.
    goto :Cleanup
)
goto:eof

:PrepBranch
echo.
echo Ensuring that %1 is available to be checked out...
echo.

set ShouldFavoriteBranch=true
for /F "usebackq delims=" %%I in (`git branch ^| findstr %1`) do ( set ShouldFavoriteBranch=false )

if "%ShouldFavoriteBranch%" == "true" (
    call favbranch /BranchName:%1
    call git fetch --prune
)

rem If we're only building, we don't care about this - we won't be committing anything anyway,
rem and these commands take a little while to complete.
if "%BUILDONLY%" neq "--buildonly" (
    echo.
    echo Current branch list:
    echo.

    call git branch --all
    call :CheckErrorLevel "Retrieving branch list" || goto Cleanup
)

echo.
echo Deleting local version of %1, if one exists...
echo.

call git branch -D %1 --force > nul 2>&1

echo.
echo Checking out %1...
echo.

if "%2" == "--perform-setup" (
    call git checkout %1 --force || call git checkout -b %1
    call :CheckErrorLevel "Create" || goto Cleanup
    call git push --set-upstream origin %1
    call :CheckErrorLevel "Create" || goto Cleanup
) else (
    call git checkout %1 --force
    call :CheckErrorLevel "Checkout" || goto Cleanup
)

call git rebase --abort > nul 2>&1

echo.
echo Pulling latest from %1...
echo.

call git pull --force
call :CheckErrorLevel "Pull" || goto Cleanup
goto:eof

:MakeDirIfNotExist
if not exist "%1" (
    echo Creating directory %1...
    mkdir "%1"
) else (
    echo Directory %1 already exists. Skipping creation.
)
goto:eof
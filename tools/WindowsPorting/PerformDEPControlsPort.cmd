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
    del /q *
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

rem If a build is canceled in the middle of a Git operation, then the lock file can be left over, and then causes the 
rem next build to fail. So make sure we clean it out if it exists.
if exist %OSRepoRoot%\src\.git\index.lock del %OSRepoRoot%\src\.git\index.lock

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

rem If we're planning to check in, then we need to check out a branch that tracks a remote branch.
rem If we're just planning to build, though, then we can just check out a detached commit.
if "%BUILDONLY%" neq "--buildonly" (
    echo.
    echo Setting up official branch...
    echo.

    call :PrepBranch %OfficialBranch% || goto Cleanup
    call :CheckErrorLevel "Setup" || goto Cleanup

    echo.
    echo Setting up staging branch...
    echo.

    call :PrepBranch %StagingBranch% --perform-setup || goto Cleanup
    call :CheckErrorLevel "Setup" || goto Cleanup

    echo.
    echo Setting up porting branch...
    echo.

    call :PrepBranch %PortingBranch% --perform-setup || goto Cleanup
    call :CheckErrorLevel "Setup" || goto Cleanup
) else (
    echo.
    echo Setting up porting branch...
    echo.
    
    call :FavBranchIfNeeded %OfficialBranch%
    call git fetch
    call git checkout origin/%PortingBranch%
)

rem If we're only building, we don't care about any of this - we won't be committing anything anyway,
rem and these commands take a little while to complete.
if "%BUILDONLY%" neq "--buildonly" (
    echo.
    echo Last three checkins to the porting branch:
    echo.

    call git log -n 3
    call :CheckErrorLevel "Retrieving logs" || goto Cleanup

    echo.
    echo Current branch state:
    echo.

    call git status
    call :CheckErrorLevel "Retrieving Git branch status" || goto Cleanup
)

popd

echo.
echo Syncing Windows repo succeeded!
echo.
echo Started: %StartTime%
echo Ended:   %time%
echo.
echo ---------------------------------
echo   STEP 3: SYNC DEPCONTROLS REPO
echo ---------------------------------

set StartTime=%time%

echo.
echo Re-starting Razzle...
echo.

call %OSRepoRoot%\src\tools\razzle.cmd x86fre no_oacr
call :CheckErrorLevel "Razzle start" || goto Cleanup

set WindowsDir=%SDXROOT%\onecoreuap\windows\dxaml\controls

if "%BUILDONLY%" neq "--buildonly" (
    echo.
    echo Ensuring that there are DEPControls commits to port...
    echo.

    for /f "delims=" %%I in (%WindowsDir%\.lastSyncedToCommit) do (
        set LastSyncedToCommitId=%%I
    )

    echo Last synced-to commit ID: "!LastSyncedToCommitId!"
    echo New synced-to commit ID:  "%BUILD_SOURCEVERSION%"

    if "!LastSyncedToCommitId!" == "%BUILD_SOURCEVERSION%" (
        echo.
        echo No new DEPControls commits to port. Exiting.
        goto Cleanup
    )
)

echo.
echo Copying scripts to Windows...
echo.

xcopy %DEPControlsRepoRoot%\tools\WindowsPorting\ScriptsToCopy %WindowsDir% /Y

pushd %WindowsDir%

if "%BUILDONLY%" neq "--buildonly" (
    for /F "usebackq" %%A in (`dir /B %DEPControlsRepoRoot%\tools\WindowsPorting\ScriptsToCopy`) do ( call git add %%A )
    call EnsureNoExternalChanges.cmd %DEPControlsRepoRoot% %ACCESSTOKEN%
    call :CheckErrorLevel "Ensuring no external changes" || goto Cleanup
)

echo.
echo Processing VPack manifest...
echo.

call processvpackmanifest
call :CheckErrorLevel "Processing" || goto Cleanup

echo.
echo Removing any previously built object files...
echo.

rmdir /S /Q %OBJECT_ROOT%\onecoreuap\windows\dxaml\controls

echo.
echo Syncing DEPControls repo to Windows...
echo.

call SyncToDEPControls.cmd %DEPControlsRepoRoot%
call :ReportBuildFailures
call :CheckErrorLevel "Sync" || goto Cleanup

echo.
echo Retrieving list of DEPControls changes...
echo.

call git status

echo.
echo Ensuring that DEPControls changes exist...
echo.

set AreUnportedChanges=false

for /F "tokens=2 delims= " %%I in ('call git status -s') do (
    if "%%I" neq ".cachedFileHashes" (
        if "%%I" neq ".expectedEnlistmentFileHashes" (
            if "%%I" neq ".lastSyncedToCommit" (
                echo %%I
                set AreUnportedChanges=true
            )
        )
    )
)

if "!AreUnportedChanges!" == "false" (
    echo No DEPControls changes. Exiting.
    goto Cleanup
)

echo.
echo Copying built files to the drop folder...
echo.

call :MakeDirIfNotExist %XES_DFSDROP%\obj

set CommandToExecute=robocopy %OBJECT_ROOT%\onecoreuap\windows\dxaml\controls %XES_DFSDROP%\obj /S /R:50 /NP /XX /XD winrt /XF *.dll /XF *.obj /XF *.mui* /XF *.exp /XF *.lce /XF *.lib /XF *.map /XF *.pch /XF *.pdb
echo %CommandToExecute%
%CommandToExecute%

popd

echo.
echo Syncing DEPControls repo succeeded!
echo.
echo Started: %StartTime%
echo Ended:   %time%
echo.
echo -------------------------
echo   STEP 4: BUILD CHANGES
echo -------------------------

set StartTime=%time%

echo.

pushd %WindowsDir%

echo.
echo Building the synced changes...
echo.

call buildcontrols.cmd /nowinmd
call :ReportBuildFailures
call :CheckErrorLevel "Building changes" || goto Cleanup

if "%BUILDONLY%" neq "--buildonly" (
    call SaveCurrentEnlistmentState.cmd
    call :CheckErrorLevel "Saving current enlistment state" || goto Cleanup
    
    echo.
    echo Staging any files that have been added...
    echo.

    call git add .
    call :CheckErrorLevel "Adding files" || goto Cleanup
)

popd

echo.
echo Building changes succeeded!
echo.
echo Started: %StartTime%
echo Ended:   %time%
echo.

if "%BUILDONLY%" == "--buildonly" (
    goto Cleanup
)

echo --------------------------
echo   STEP 5: COMMIT CHANGES
echo --------------------------

set StartTime=%time%

echo.

pushd %WindowsDir%

echo.
echo Committing the synced changes...
echo.

call CommitSync.cmd %BUILD_SOURCEVERSION% %ACCESSTOKEN% master
call :CheckErrorLevel "Committing changes" || goto Cleanup

popd

echo.
echo Committing changes succeeded!
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

:FavBranchIfNeeded
set ShouldFavoriteBranch=true
for /F "usebackq delims=" %%I in (`git branch ^| findstr %1`) do ( set ShouldFavoriteBranch=false )

if "%ShouldFavoriteBranch%" == "true" (
    call favbranch /BranchName:%1
    call git fetch --prune
)
goto:eof

:PrepBranch
echo.
echo Ensuring that %1 is available to be checked out...
echo.

call :FavBranchIfNeeded %1

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

call git fetch
call git merge FETCH_HEAD
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
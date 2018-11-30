@echo off
setlocal enableDelayedExpansion

set PATH=C:\Program Files\GitTelemetry;C:\Program Files\Git\cmd;C:\Program Files\GVFS;%PATH%
set OSRepoRoot=C:\os

echo.
echo -----------
echo   CLEANUP
echo -----------
echo.
echo Starting Razzle...
echo.

call %OSRepoRoot%\src\tools\razzle.cmd x86fre no_oacr

rem To help with debugging build breaks, we'll copy all of the useful files we built to the drop folder
rem so they can be analyzed after the fact.
echo.
echo Copying built files to the drop folder...
echo.

call :MakeDirIfNotExist %XES_DFSDROP%\os_src

call :CopyIfExists %BASEDIR%\onecoreuap\windows\dxaml\controls\dev\dll\CppWinRTFilterTypes.txt %XES_DFSDROP%\os_src\
call :CopyIfExists %BASEDIR%\onecoreuap\windows\dxaml\controls\dev\dll\XamlTypeInfo.g.h %XES_DFSDROP%\os_src\
call :CopyIfExists %BASEDIR%\onecoreuap\windows\dxaml\controls\dev\dll\XamlTypeInfo.g.rc %XES_DFSDROP%\os_src\
call :CopyIfExists %BASEDIR%\onecoreuap\windows\dxaml\controls\manifest\Microsoft-Windows-UI-Xaml-DEPControls.man %XES_DFSDROP%\os_src\
call :CopyIfExists %BASEDIR%\onecoreuap\windows\dxaml\xcp\dxaml\dllsrv\winrt\native\themeresources.rc %XES_DFSDROP%\os_src\
call :CopyIfExists %BASEDIR%\onecoreuap\windows\dxaml\xcp\dxaml\themes\generic.xaml %XES_DFSDROP%\os_src\
call :CopyIfExists %BASEDIR%\onecoreuap\windows\dxaml\xcp\tools\XCPTypesAutoGen\Modules\DEPControls\DEPControls.cs %XES_DFSDROP%\os_src\

call :MakeDirIfNotExist %XES_DFSDROP%\public

call :CopyIfExists %PUBLIC_ROOT%\internal\onecoreuap\sdkmetadata\publicmetadata\windows.foundation.universalapicontract.winmd %XES_DFSDROP%\public\
call :CopyIfExists %PUBLIC_ROOT%\internal\onecoreuap\internal\interimmetadata\depcontrols.winmd %XES_DFSDROP%\public\
call :CopyIfExists %PUBLIC_ROOT%\internal\onecoreuap\internal\buildmetadata\windows.winmd %XES_DFSDROP%\public\
call :CopyIfExists %PUBLIC_ROOT%\internal\onecoreuap\internal\buildmetadata\internal\windows.ui.winmd %XES_DFSDROP%\public\
call :CopyIfExists %PUBLIC_ROOT%\internal\onecoreuapwindows\inc\depcontrols.xaml %XES_DFSDROP%\public\

call :MakeDirIfNotExist %XES_DFSDROP%\obj

set CommandToExecute=robocopy %OBJECT_ROOT%\onecoreuap\windows\dxaml\controls %XES_DFSDROP%\obj /S /R:50 /NP /XX /XD winrt /XF *.dll /XF *.obj /XF *.mui* /XF *.exp /XF *.lce /XF *.lib /XF *.map /XF *.pch /XF *.pdb
echo %CommandToExecute%
%CommandToExecute%

rem We don't want to copy the *entire* C++/WinRT folder, because that contains a whole lot of files that we don't care about.
rem We'll copy everything except for the Platform and Component\winrt folders, from which we'll only copy files starting with "Windows.UI.Xaml.*".
set CommandToExecute=robocopy %OBJECT_ROOT%\onecoreuap\windows\dxaml\controls\winrt %XES_DFSDROP%\obj\winrt /S /R:50 /NP /XX /XD %OBJECT_ROOT%\onecoreuap\windows\dxaml\controls\winrt\%_BuildAlt%\Platform /XD %OBJECT_ROOT%\onecoreuap\windows\dxaml\controls\winrt\%_BuildAlt%\Component\winrt
echo %CommandToExecute%
%CommandToExecute%

set CommandToExecute=robocopy %OBJECT_ROOT%\onecoreuap\windows\dxaml\controls\winrt\%_BuildAlt%\Platform %XES_DFSDROP%\obj\winrt\%_BuildAlt%\Platform Windows.UI.Xaml.* /S /R:50 /NP /XX
echo %CommandToExecute%
%CommandToExecute%

set CommandToExecute=robocopy %OBJECT_ROOT%\onecoreuap\windows\dxaml\controls\winrt\%_BuildAlt%\Component\winrt %XES_DFSDROP%\obj\winrt\%_BuildAlt%\Component\winrt Windows.UI.Xaml.* /S /R:50 /NP /XX
echo %CommandToExecute%
%CommandToExecute%

echo.
echo Deleting saved credentials...
echo.

cmdkey /delete:LegacyGeneric:target=git:https://microsoft.visualstudio.com
cmdkey /delete:LegacyGeneric:target=vpack:https://microsoft.vsblob.visualstudio.com
cmdkey /delete:LegacyGeneric:target=vpack:https://microsoft.artifacts.visualstudio.com

echo.
echo Reverting any changes that weren't committed...
echo.

if exist "%BASEDIR%" (
    pushd %BASEDIR%
    call git reset .
    call git checkout -- .
    popd
)

echo.
echo Removing any leftover files...
echo.

rem "git clean" will hydrate files as it searches, so we want to make sure that
rem we only clean the directories that we actually care about, in order to avoid
rem repo over-hydration.
if exist "%BASEDIR%\onecoreuap\windows\dxaml" (
    pushd %BASEDIR%\onecoreuap\windows\dxaml
    call git clean -df
    popd
)

rem We also need to delete the files that we published to publics,
rem in order to avoid the situation where one build can publish something
rem that conflicts with something published by another build.
call :DeleteIfExists %PUBLIC_ROOT%\internal\onecoreuap\internal\interimmetadata\depcontrols.winmd
call :DeleteIfExists %PUBLIC_ROOT%\internal\onecoreuapwindows\inc\depcontrols.xaml

echo.
echo Ensuring that all processes we started are cleaned up...
echo.

tasklist
taskkill /IM Microsoft.Engineering.FileVirtualization.Daemon.exe /F
taskkill /IM Microsoft.Engineering.MonitoringAgent.RunAgentDaemon.exe /F

echo.
echo Cleanup complete!
echo.
goto:eof

:MakeDirIfNotExist
if not exist "%1" (
    echo Creating directory %1...
    mkdir "%1"
) else (
    echo Directory %1 already exists. Skipping creation.
)
goto:eof

:CopyIfExists
if exist "%1" (
    echo Copying %1 to %2...
    copy /Y "%1" "%2"
) else (
    echo File %1 does not exist. Skipping copy.
)
goto:eof

:DeleteIfExists
if exist "%1" (
    echo Deleting %1...
    del "%1"
) else (
    echo File %1 does not exist. Skipping delete.
)
goto:eof
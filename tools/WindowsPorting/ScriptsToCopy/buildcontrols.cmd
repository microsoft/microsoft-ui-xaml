@echo off
setlocal enableextensions enabledelayedexpansion

rem We'll set these to be empty so we don't rely on their values in our project -
rem the nightly build machines don't have these set, so they can't use them.
set O=
set TARGET_DIRECTORY=

rem Any sources.dep updates are ones that we want to happen, anyway, so we'll set
rem AUTO_DEP in order to cause them to be automatic, rather than having them pointlessly
rem break the build and require you to do it again.
set AUTO_DEP=1

rem Similarly, we don't care about building SPKGs, so let's turn building them off as well
rem unless the caller specifically requests them.
set SKIP_SPKG=1

set nowinmd=
set dependents=
set dependentsonly=
set incremental=
set superclean=
set customcommand=
set starttime=%time%

set DIRSTOBUILD=%SDXROOT%\onecoreuap\windows\dxaml\controls
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecore\windows\appcompat\db
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecoreuap\windows\appcompat\db
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecoreuap\windows\dxaml\xcp\components\allocation
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecoreuap\windows\dxaml\xcp\components\base
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecoreuap\windows\dxaml\xcp\components\criticalsection
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecoreuap\windows\dxaml\xcp\components\dependencylocator
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecoreuap\windows\dxaml\xcp\components\math
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecoreuap\windows\dxaml\xcp\components\pch
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecoreuap\windows\dxaml\xcp\components\quirks
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecoreuap\windows\dxaml\xcp\components\runtimeenabledfeatures
set DIRSTOBUILD=%DIRSTOBUILD%;%SDXROOT%\onecoreuap\windows\dxaml\xcp\components\terminateProcessOnOOM

set BUILDPARAMS=/skiptestcode
set TESTBUILDPARAMS=

set DOMINODIRSTOBUILD=

:: Parse args
:loop

IF NOT "%1"=="" (
    if "%1"=="/nowinmd" (
        set nowinmd=1
    ) else if "%1"=="/dependents" (
        set dependents=1
    ) else if "%1"=="/dependentsonly" (
        set dependents=1
        set dependentsonly=1
    ) else if "%1"=="/inc" (
        set incremental=1
    ) else if "%1"=="/spkg" (
        set SKIP_SPKG=
    ) else if "%1"=="/superclean" (
        set superclean=1
    ) else if "%1"=="/command" (
	SHIFT
	set customcommand=1
	goto :loopend
    ) else (
        echo ERROR: Unrecognized argument: "%1"
        goto :usage
    )
    SHIFT
    GOTO :loop
)
:loopend

if NOT "%nowinmd%" == "1" (
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\MergedComponents\winmetadata\sdkmetadata
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\MergedComponents\winmetadata\systemmetadata
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\onecoreuap\merged\winmetadata\ContractMetadata
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\onecoreuap\merged\winmetadata\InternalIdlFiles
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\onecoreuap\merged\winmetadata\InternalMetadata
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\onecoreuap\merged\winmetadata\SdkIdlFiles
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\onecoreuap\merged\winmetadata\SdkMetadata
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\onecoreuap\merged\winmetadata\SystemMetadata
)

if "%dependents%" == "1" (
    if "%dependentsonly%" == "1" (
        set DIRSTOBUILD=
        set DOMINODIRSTOBUILD=
    )

    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\onecore\windows\AccessibleTech
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\onecoreuap\shell\lock
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\onecoreuap\printscan\Print
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\pcshell\shell\PeopleBar
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\pcshell\shell\Windows.UI.Shell
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\shellcommon\inetcore\spartan
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\shellcommon\shell\AdaptiveCards
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\shellcommon\shell\deviceux
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\shellcommon\shell\SystemSettings
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\shellcommon\shell\welcome
    set DIRSTOBUILD=!DIRSTOBUILD!;%SDXROOT%\shellcommon\shell\Windows.UI.Shell

    set DOMINODIRSTOBUILD=%SDXROOT%\analog\apex\ShellExperiences\ApplyFirstBootSettings

    rem The above directories depend on a bunch of other stuff, so we need to make sure
    rem that we build everything that they depend on, as well.
    set BUILDPARAMS=!BUILDPARAMS! /parent
)

if NOT "%incremental%" == "1" (
    set BUILDPARAMS=!BUILDPARAMS! /c
    set TESTBUILDPARAMS=!TESTBUILDPARAMS! /c
)

:commandloopstart
if "!customcommand!" == "1" (
    :: Can't use '%*' to extract the rest of the command line; it doesn't respect shift
    set BUILDPARAMS=!BUILDPARAMS! %1
    set TESTBUILDPARAMS=!TESTBUILDPARAMS! %1
    SHIFT
    if "%1" NEQ "" goto commandloopstart
)

if "%superclean%" == "1" (
    echo.
    echo Deleting object_root/public_root/_nttree...
    echo.

    if exist %OBJECT_ROOT% rmdir /q /s %OBJECT_ROOT%
    if exist %OBJECT_ROOT% (echo OBJECT_ROOT still exists! & goto :fail)

    if exist %PUBLIC_ROOT% rmdir /q /s %PUBLIC_ROOT%
    if exist %PUBLIC_ROOT% (echo PUBLIC_ROOT still exists! & goto :fail)

    if exist %_NTTREE% rmdir /q /s %_NTTREE%
    if exist %_NTTREE% (echo __NTTREE still exists! & goto :fail)

    call git clean -df
)

if "%dependents%" == "1" (
    if not "!DOMINODIRSTOBUILD!" == "" (
        echo.
        echo Building Domino-based dependent build directories prior to the main build...
        echo.

        call db build /directories:!DOMINODIRSTOBUILD!
        if ERRORLEVEL 1 goto :fail
    )
)

set COMMAND=build %BUILDPARAMS% /dir "%DIRSTOBUILD%"

echo.
echo %COMMAND%
echo.

call %COMMAND%
if ERRORLEVEL 1 goto :fail

echo.
echo Building test code...
echo.

pushd test
call build %TESTBUILDPARAMS%
popd
if ERRORLEVEL 1 goto :fail

echo.
echo Building resources...
echo.
call ..\xcp\buildResources.cmd
if ERRORLEVEL 1 goto :fail

echo.
echo Signing built binaries...
echo.
call ntsign %_NTTREE%\windows.ui.xaml.controls.dll
call ntsign %_NTTREE%\windows.ui.xaml.resources*.dll

echo.
echo buildcontrols.cmd %* succeeded!
echo.
echo Started: %starttime%
echo Ended:   %time%
goto end

:usage
echo Usage:
echo     buildcontrols.cmd [options]
echo.
echo        /inc            Build incrementally (builds with -c by default)
echo        /nowinmd        Skips MergedComponents\winmetadata.  Skip if you've
echo                        not updated the API surface and are synced to LKG.
echo        /superclean     Deletes obj/bin/public and scorches.  Will delete
echo                        loose files that aren't opened for add!
echo        /dependents     Builds all directories that depend on what this directory builds.
echo        /spkg           Builds SPKGs in addition to everything else.
echo.
echo        /command        Pass in additional build commands
echo.
goto :end

:fail
echo.
echo ERROR: buildcontrols.cmd %* failed.
echo.

set ERRORLEVEL=1

:end
exit /b %ERRORLEVEL%

endlocal
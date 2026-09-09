@echo off

REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License. See LICENSE in the project root for license information.

REM Accepts the current XAML compiler codegen as the new baseline under TestMasters.
REM
REM Usage, from src\XamlCompiler, in a build window (init.cmd):
REM
REM   copynewmasters.cmd            Refresh every target. Refuses to do anything if any target has
REM                                 no codegen, so a build failure cannot quietly empty a baseline.
REM   copynewmasters.cmd /partial   Refresh only the targets that have codegen, leaving the masters
REM                                 of the rest untouched, and list what was skipped.
REM
REM Which codegen directory maps to which master directory is recorded in
REM Tests\UnitTests\CodegenTargets.txt, which CodegenTests.cs reads as well, so the copy and the
REM diff cannot disagree about where codegen is. Build the regression projects first: this script
REM only copies what the build produced.

SETLOCAL EnableDelayedExpansion

REM Work from the directory this script lives in, so it can be run from anywhere. SETLOCAL restores
REM the caller's directory on exit.
cd /d "%~dp0"

set _targets=Tests\UnitTests\CodegenTargets.txt
set _codegenRoot=%BuildOutputRoot%\%_BuildArch%%_BuildType%
set _partial=0
if /I "%~1"=="/partial" set _partial=1

if not exist "%_targets%" (
    echo ERROR: Cannot find %_targets%. Run this script from src\XamlCompiler.
    goto :failed
)

if "%BuildOutputRoot%"=="" (
    echo ERROR: BuildOutputRoot is not set. Run this script from a build window ^(init.cmd^).
    goto :failed
)

REM Pass 1 - check every target before touching TestMasters, so that a failure here cannot leave
REM the masters in the half-updated state the old version of this script warned about.
set _missing=0
for /f "usebackq eol=# tokens=1,2 delims=|" %%a in ("%_targets%") do CALL :checkProject "%%a" "%%b"

if %_missing% GTR 0 (
    if %_partial%==0 (
        echo.
        echo ERROR: %_missing% target^(s^) have no codegen. Build them, or re-run with /partial to
        echo        refresh the rest and leave their masters alone. Nothing has been changed.
        goto :failed
    )
    echo.
    echo WARNING: skipping %_missing% target^(s^) with no codegen ^(/partial^).
)

call csc tools\fixmasters\fixmasters.cs /out:%temp%\fixmasters.exe
if NOT %ERRORLEVEL%==0 goto :failed

REM Pass 2 - refresh each target in place. Each master directory is emptied immediately before it is
REM repopulated, so a stale generated file cannot survive a rename, and the masters of any skipped
REM target are left as they were.
for /f "usebackq eol=# tokens=1,2 delims=|" %%a in ("%_targets%") do (
    CALL :copyProject "%%a" "%%b"
    if ERRORLEVEL 1 goto :failed
)

call %temp%\fixmasters.exe
if NOT %ERRORLEVEL%==0 goto :failed

echo.
echo Done.
goto :EOF

:failed
echo.
@echo ERROR: copynewmasters did not complete.
EXIT /B 1

:checkProject
dir /s /b "%_codegenRoot%\%~2\*.g.*" >nul 2>&1
if ERRORLEVEL 1 (
    echo MISSING: no codegen in "%_codegenRoot%\%~2" for "%~1"
    set /a _missing+=1
)
EXIT /B 0

:copyProject
dir /s /b "%_codegenRoot%\%~2\*.g.*" >nul 2>&1
if ERRORLEVEL 1 EXIT /B 0

echo ## Updating %~1 from %~2
if exist "TestMasters\%~1" (
    rmdir /s /q "TestMasters\%~1"
    if exist "TestMasters\%~1" (
        echo ERROR: Could not remove "TestMasters\%~1".
        EXIT /B 1
    )
)

robocopy "%_codegenRoot%\%~2" "TestMasters\%~1" *.g.* /XF *.g.obj /XF *.nuget.g.* /XF *.backup /s /r:0 /z /ndl
if ERRORLEVEL 2 (
    echo ERROR: Could not refresh "TestMasters\%~1".
    EXIT /B 1
)
EXIT /B 0

@echo off

REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License. See LICENSE in the project root for license information.

REM Accepts the current XAML compiler codegen as the new baseline under TestMasters.
REM
REM Usage, from any directory in a build window (init.cmd):
REM
REM   copynewmasters.cmd            Refresh every target. Refuses to do anything if any target has
REM                                 no codegen, so a build failure cannot quietly empty a baseline.
REM
REM Before running this script for a flavor, build XamlCompiler.sln and XamlCompilerTests.sln in
REM the same initialized window. Run once for chk and once for fre.
REM
REM Which codegen directory maps to which master directory is recorded in
REM Tests\UnitTests\CodegenTargets.txt, which CodegenTests.cs reads as well, so the copy and the
REM diff cannot disagree about where codegen is. Build the regression projects first: this script
REM only copies what the build produced.

SETLOCAL EnableDelayedExpansion

REM Work from the directory this script lives in. PUSHD and POPD preserve the caller's directory.
pushd "%~dp0"
if ERRORLEVEL 1 (
    echo ERROR: Cannot access the directory containing copynewmasters.cmd.
    EXIT /B 1
)

if not "%~1"=="" (
    echo ERROR: Unknown argument "%~1". copynewmasters.cmd does not accept arguments.
    goto :failed
)

set _targets=Tests\UnitTests\CodegenTargets.txt
set _codegenRoot=%BuildOutputRoot%\%_BuildArch%%_BuildType%
set "_mastersRoot="
if /I "%_BuildType%"=="chk" set "_mastersRoot=TestMasters\RegressionProjects\chk"
if /I "%_BuildType%"=="fre" set "_mastersRoot=TestMasters\RegressionProjects\fre"

if not exist "%_targets%" (
    echo ERROR: Cannot find %_targets% next to copynewmasters.cmd.
    goto :failed
)

if "%BuildOutputRoot%"=="" (
    echo ERROR: BuildOutputRoot is not set. Run this script from a build window ^(init.cmd^).
    goto :failed
)

if not defined _mastersRoot (
    echo ERROR: _BuildType must be chk or fre. Run this script from a build window ^(init.cmd^).
    goto :failed
)

REM Pass 1 - verify every target before touching TestMasters, making an incomplete build an
REM all-or-nothing failure.
set _missing=0
for /f "usebackq eol=# tokens=1,2 delims=|" %%a in ("%_targets%") do CALL :checkProject "%%a" "%%b"

if %_missing% GTR 0 (
    echo.
    echo ERROR: %_missing% target^(s^) have no codegen. Build every target and re-run.
    echo        Nothing has been changed.
    goto :failed
)

call csc tools\fixmasters\fixmasters.cs /out:%temp%\fixmasters.exe
if NOT %ERRORLEVEL%==0 goto :failed

REM Pass 2 - refresh each target in place. Each master directory is emptied immediately before it is
REM repopulated, so a stale generated file cannot survive a rename.
for /f "usebackq eol=# tokens=1,2 delims=|" %%a in ("%_targets%") do (
    CALL :copyProject "%%a" "%%b"
    if ERRORLEVEL 1 goto :failed
)

call %temp%\fixmasters.exe
if NOT %ERRORLEVEL%==0 goto :failed

echo.
echo Done.
popd
EXIT /B 0

:failed
echo.
@echo ERROR: copynewmasters did not complete.
popd
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
if ERRORLEVEL 1 (
    echo ERROR: Codegen disappeared from "%_codegenRoot%\%~2" for "%~1".
    EXIT /B 1
)

echo ## Updating %~1 from %~2
if exist "%_mastersRoot%\%~1" (
    rmdir /s /q "%_mastersRoot%\%~1"
    if exist "%_mastersRoot%\%~1" (
        echo ERROR: Could not remove "%_mastersRoot%\%~1".
        EXIT /B 1
    )
)

robocopy "%_codegenRoot%\%~2" "%_mastersRoot%\%~1" *.g.* /XF *.g.obj /XF *.nuget.g.* /XF *.backup /s /r:0 /z /ndl
if ERRORLEVEL 2 (
    echo ERROR: Could not refresh "%_mastersRoot%\%~1".
    EXIT /B 1
)
EXIT /B 0

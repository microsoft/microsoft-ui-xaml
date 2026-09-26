@echo off

REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License. See LICENSE in the project root for license information.

REM Accepts the current XAML compiler codegen as the new baseline under TestMasters.
REM
REM Usage, from any directory in a build window (init.cmd):
REM
REM   copynewmasters.cmd            Refresh every target for the current flavor. Refuses to update
REM                                 any masters if a target has no codegen.
REM
REM Before running this script for a flavor, build XamlCompiler.sln and XamlCompilerTests.sln in
REM the same initialized window. Run once for chk and once for fre. The other flavor's masters
REM are preserved until it is refreshed, and identical files are moved into common automatically.
REM
REM Which codegen directory maps to which master directory is recorded in
REM Tests\UnitTests\CodegenTargets.txt, which CodegenTests.cs reads as well, so the copy and the
REM diff cannot disagree about where codegen is. Build the regression projects first: this script
REM only copies what the build produced.

SETLOCAL

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

if not exist "%_targets%" (
    echo ERROR: Cannot find %_targets% next to copynewmasters.cmd.
    goto :failed
)

if "%BuildOutputRoot%"=="" (
    echo ERROR: BuildOutputRoot is not set. Run this script from a build window ^(init.cmd^).
    goto :failed
)

if /I not "%_BuildType%"=="chk" if /I not "%_BuildType%"=="fre" (
    echo ERROR: _BuildType must be chk or fre. Run this script from a build window ^(init.cmd^).
    goto :failed
)

REM The helper verifies all generated targets before replacing masters for the current flavor.
set "_updater=%temp%\fixmasters.exe"
call csc /nologo tools\fixmasters\fixmasters.cs /out:"%_updater%"
if not "%ERRORLEVEL%"=="0" goto :failed

"%_updater%" "TestMasters\RegressionProjects" "%_BuildType%" "%_codegenRoot%" "%_targets%"
if not "%ERRORLEVEL%"=="0" goto :failed

del /q "%_updater%"
echo.
echo Done.
popd
EXIT /B 0

:failed
if defined _updater if exist "%_updater%" del /q "%_updater%"
echo.
@echo ERROR: copynewmasters did not complete.
popd
EXIT /B 1

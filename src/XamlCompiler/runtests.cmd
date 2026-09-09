@echo off
setlocal EnableExtensions

rem Copyright (c) Microsoft Corporation.
rem Licensed under the MIT License. See LICENSE in the project root for license information.

if not defined EnvironmentInitialized goto :not_initialized
if not defined RepoRoot goto :not_initialized
if not defined BuildOutputRoot goto :not_initialized
if not defined _BuildArch goto :not_initialized
if not defined _BuildType goto :not_initialized

set "Flavor=%_BuildArch%%_BuildType%"
set "TestDir=%BuildOutputRoot%\%Flavor%\src\XamlCompiler\Tests\UnitTests\XamlCompilerUnitTests"
set "RunSettings=%RepoRoot%\src\XamlCompiler\Tests\UnitTests\test.runsettings"
set "PayloadMarker=%TestDir%\XamlCompilerUnitTests.payload.complete"

if not exist "%TestDir%\UnitTests.dll" (
    echo [ERROR] Missing test assembly: %TestDir%\UnitTests.dll
    goto :build_required
)
if not exist "%PayloadMarker%" (
    echo [ERROR] XamlCompiler unit-test payload is incomplete: %PayloadMarker%
    goto :build_required
)
if not exist "%RunSettings%" goto :missing_runsettings

call :find_vstest
if errorlevel 1 exit /b 1
if not defined VsTest goto :vstest_not_found

echo [INFO] Running XamlCompiler tests for %Flavor%...
pushd "%TestDir%" >nul
"%VsTest%" UnitTests.dll /Settings:"%RunSettings%" %*
set "Result=%ERRORLEVEL%"
popd
exit /b %Result%

:find_vstest
set "VsTest="

if not defined VSTEST_CONSOLE goto :find_vstest_on_path
set "VsTest=%VSTEST_CONSOLE:"=%"
if exist "%VsTest%" exit /b 0
echo [ERROR] VSTEST_CONSOLE does not point to an existing file:
echo         %VsTest%
exit /b 1

:find_vstest_on_path
for %%I in (vstest.console.exe) do if not "%%~$PATH:I"=="" set "VsTest=%%~$PATH:I"
if defined VsTest exit /b 0

if defined DevEnvDir if exist "%DevEnvDir%\Extensions\TestPlatform\vstest.console.exe" set "VsTest=%DevEnvDir%\Extensions\TestPlatform\vstest.console.exe"
if defined VsTest exit /b 0

if defined VSINSTALLDIR if exist "%VSINSTALLDIR%\Common7\IDE\Extensions\TestPlatform\vstest.console.exe" set "VsTest=%VSINSTALLDIR%\Common7\IDE\Extensions\TestPlatform\vstest.console.exe"
if defined VsTest exit /b 0

set "VsWhere="
for %%I in (vswhere.exe) do if not "%%~$PATH:I"=="" set "VsWhere=%%~$PATH:I"
if not defined VsWhere if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" set "VsWhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not defined VsWhere exit /b 0

setlocal EnableDelayedExpansion
for /f "usebackq delims=" %%I in (`""!VsWhere!" -products * -latest -find Common7\IDE\Extensions\TestPlatform\vstest.console.exe"`) do if not defined VsTest set "VsTest=%%I"
for %%I in ("!VsTest!") do endlocal & set "VsTest=%%~I"
exit /b 0

:not_initialized
echo [ERROR] The WinUI build environment is not initialized correctly.
echo         Run init.cmd ^<flavor^> from the repo root in this command prompt, then retry.
exit /b 1

:build_required
echo.
echo [ERROR] XamlCompiler test outputs for %Flavor% are incomplete.
echo         Build "%RepoRoot%\src\XamlCompiler\XamlCompiler.sln" in this initialized
echo         command prompt, then retry.
exit /b 1

:missing_runsettings
echo [ERROR] Test settings were not found at:
echo         %RunSettings%
exit /b 1

:vstest_not_found
echo [ERROR] vstest.console.exe was not found in the initialized Visual Studio environment.
echo         Rerun init.cmd %Flavor% and ensure the Visual Studio Test Platform is installed.
exit /b 1

@echo off
setlocal EnableExtensions

rem Copyright (c) Microsoft Corporation.
rem Licensed under the MIT License. See LICENSE in the project root for license information.

rem Runs the XAML compiler regression tests end to end, with the Native AOT compiler and the CompileXaml task.
rem See src\XamlCompiler\Tests\RunNativeCompilerTests.ps1 for the options, e.g. -DesignTime, -Samples and
rem -Differential -ReferencePackage <package layout>.

if not defined EnvironmentInitialized goto :not_initialized
if not defined RepoRoot goto :not_initialized

powershell -NoProfile -ExecutionPolicy Bypass -File "%RepoRoot%\src\XamlCompiler\Tests\RunNativeCompilerTests.ps1" %*
exit /b %ERRORLEVEL%

:not_initialized
echo [ERROR] The WinUI build environment is not initialized correctly.
echo         Run init.cmd ^<flavor^> from the repo root in this command prompt, then retry.
exit /b 1

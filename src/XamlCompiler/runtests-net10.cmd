@echo off
setlocal EnableExtensions

rem Copyright (c) Microsoft Corporation.
rem Licensed under the MIT License. See LICENSE in the project root for license information.

rem Runs the XAML compiler unit tests on .NET (CoreCLR) against the net10.0 build of the compiler, which runs the same code as
rem the Native AOT executable compiler. Same usage as runtests.cmd (all arguments are forwarded to vstest.console.exe); see
rem src\XamlCompiler\Tests\UnitTests\README.md.

set "XamlCompilerUnitTestsTargetFramework=net10.0"
call "%~dp0runtests.cmd" %*
exit /b %ERRORLEVEL%

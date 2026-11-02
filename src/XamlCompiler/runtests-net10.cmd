@echo off
setlocal EnableExtensions

rem Copyright (c) Microsoft Corporation.
rem Licensed under the MIT License. See LICENSE in the project root for license information.

rem Obsolete alias of runtests.cmd, kept for existing scripts: the unit tests only run on .NET (CoreCLR) now, against the IL build
rem of the compiler executable. See src\XamlCompiler\Tests\UnitTests\README.md.

call "%~dp0runtests.cmd" %*
exit /b %ERRORLEVEL%

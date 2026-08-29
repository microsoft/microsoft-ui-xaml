@echo Off

REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License. See LICENSE in the project root for license information.

set SRCDIR=\\authoring\team\user\felixa\XAML Tools\PERFPROJECTS
set DSTDIR=%APPDATA%\PerfProjects
@echo Copying from %SRCDIR% to %DSTDIR%
robocopy /s "%SRCDIR%" "%DSTDIR%"

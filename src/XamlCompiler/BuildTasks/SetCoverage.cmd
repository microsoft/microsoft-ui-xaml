@echo off

REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License. See LICENSE in the project root for license information.

if "%_echo%" == "" @echo off
setlocal

set PF=%ProgramFiles%
rem if "%ProgramFiles(x86)%" == "" set PF=%ProgramFiles(x86)%

set SB=%_NTBINROOT%\SuiteBin
if NOT "%_NTBINROOT%" == "" goto instr 

if NOT "%1%" == "" goto setbinpath 
echo please provide the path to the "src" folder of the enlistment
echo For example:  SetCoverage  K:\dd\W8T_Refactor\binaries\x86chk\SuiteBin
goto eof

:setbinpath
set SB=%1%

:instr
set INSTRCMD="%PF%\Microsoft Visual Studio 14.0\Team Tools\Performance Tools\vsinstr.exe"
%INSTRCMD% /coverage %SB%\Microsoft.Windows.UI.Xaml.Build.Tasks.dll
%INSTRCMD% /coverage %SB%\Microsoft.WindowsPhoneAppx.Build.Tasks.dll

:eof

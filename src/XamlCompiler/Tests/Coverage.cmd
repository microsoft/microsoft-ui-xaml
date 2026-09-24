:: Copyright (c) Microsoft Corporation.
:: Licensed under the MIT License.
setlocal

@if NOT "%_NTROOT%" == "" goto Usage
@if "%VSINSTALLDIR%" == "" goto Usage

set PF=%ProgramFiles%
if NOT "%ProgramFiles(x86)%" == "" set PF=%ProgramFiles(x86)%
goto CollectCoverage

:Usage
@echo Please run this command from a "Visual Studio x86 Native Tools Command Prompt"
@echo cd /D %CD%
@echo Coverage.cmd
@exit /B 1

:CollectCoverage
@set outputFile=%1
@if "%1" == "" set outputFile=UnitTest.coverage

set VSPERFMON=VSPerfMon.exe
set VSPERFCMD=VSPerfCmd.exe

start "Collecting Coverage Data" "%VSPERFMON%" /coverage /output:%outputFile%

echo Waiting
"%VSPERFCMD%" /waitstart
"%VSPERFCMD%" /GlobalOn

echo Testing
mstest /testcontainer:UnitTests\bin\Debug\UnitTests.dll

"%VSPERFCMD%" /GlobalOff
echo Shutting down
"%VSPERFCMD%" /shutdown

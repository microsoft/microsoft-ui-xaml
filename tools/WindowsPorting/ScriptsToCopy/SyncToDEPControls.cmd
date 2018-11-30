@echo off
setlocal enableDelayedExpansion

if "%1"=="" goto :Usage

set SyncCommand=%SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File %~dp0\SyncToDEPControls.ps1 -gitRepositoryPath %1

echo Running "%SyncCommand%"
%SyncCommand%
goto :End

:Usage

echo Usage: SyncToDEPControls.cmd ^<path to Git repository^>
echo Example: SyncToDEPControls.cmd C:\Users\^<alias^>\Source\Repos\dep.controls
echo.

:End

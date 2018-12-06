@echo off
setlocal enableDelayedExpansion

if "%1"=="" goto :Usage
if "%2"=="" goto :Usage

set CommitCommand=%SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File %~dp0\CommitSync.ps1 -SyncedToCommitId %1 -PersonalAccessToken %2

echo Running "%CommitCommand%"
%CommitCommand%

goto :End

:Usage

echo.
echo Usage: CommitSync.cmd ^<commit to sync to^> ^<access token^>
echo.

:End

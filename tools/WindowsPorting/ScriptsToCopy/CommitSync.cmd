@echo off
setlocal enableDelayedExpansion

if "%1"=="" goto :Usage
if "%2"=="" goto :Usage

if not "%3"=="" (
    set SyncCommand=!SyncCommand! -DEPControlsBranch %3
)

if not "%4"=="" (
    set SyncCommand=!SyncCommand! -PortingBranch %4
)

set CommitCommand=%SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File %~dp0\CommitSync.ps1 -SyncedToCommitId %1 -PersonalAccessToken %2

echo Running "%CommitCommand%"
%CommitCommand%

goto :End

:Usage

echo.
echo Usage: CommitSync.cmd ^<commit to sync to^> ^<access token^> [^<source branch^>] [^<porting branch^>]
echo.

:End

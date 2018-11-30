@echo off
setlocal enableDelayedExpansion

set ExtraCommands=

if not "%1"=="" (
    set ExtraCommands=!ExtraCommands! -DEPControlsRepoRoot %1
)

if not "%2"=="" (
    set ExtraCommands=!ExtraCommands! -PersonalAccessToken %2
)

if not "%3"=="" (
    set ExtraCommands=!ExtraCommands! -DEPControlsTargetBranch %3
)

if not "%4"=="" (
    set ExtraCommands=!ExtraCommands! -DEPControlsBackPortBranch %4
)

set EnsureCommand=%SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File %~dp0\EnsureNoExternalChanges.ps1 !ExtraCommands!

echo Running "%EnsureCommand%"
%EnsureCommand%
@echo off
setlocal enableDelayedExpansion

set ExtraCommands=

if not "%1"=="" (
    set ExtraCommands=!ExtraCommands! -MUXControlsRepoRoot %1
)

set EnsureCommand=%SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File %~dp0\EnsureNoExternalChanges.ps1 !ExtraCommands!

echo Running "%EnsureCommand%"
%EnsureCommand%
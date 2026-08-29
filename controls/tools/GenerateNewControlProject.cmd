@echo off
setlocal enableDelayedExpansion

@%SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -ExecutionPolicy Bypass -File %~dp0\GenerateNewControlProject.ps1 %*
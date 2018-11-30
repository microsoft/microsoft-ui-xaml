@echo off
setlocal enableDelayedExpansion

@%SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File %~dp0\RunWUXCCodeGen.ps1
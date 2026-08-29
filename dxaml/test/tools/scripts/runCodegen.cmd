@echo off
if exist %SystemRoot%\SysWOW64\WindowsPowerShell\v1.0\powershell.exe (
    %SystemRoot%\SysWOW64\WindowsPowerShell\v1.0\powershell.exe -NoProfile -ExecutionPolicy Unrestricted -File %~dp0\runCodegen.ps1 %*
) else (
    %SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -ExecutionPolicy Unrestricted -File %~dp0\runCodegen.ps1 %*
)
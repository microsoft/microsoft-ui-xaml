@echo off
setlocal

%WINDIR%\Syswow64\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy RemoteSigned -NoLogo -NoExit -Command if(test-path $profile){.$profile}; . %~dp0\PSProfile.ps1
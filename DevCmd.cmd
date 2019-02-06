@echo OFF

pushd %~dp0

set PATH=%PATH%;%~dp0\tools

call %~dp0\tools\addaliases.cmd

powershell -Command "&{ 'set _VSINSTALLDIR15=' + (Get-ItemProperty 'HKLM:\software\wow6432node\Microsoft\VisualStudio\SxS\vs7' -Name '15.0').'15.0' }" > %TEMP%\vsinstalldir.bat
call %TEMP%\vsinstalldir.bat

IF EXIST "%_VSINSTALLDIR15%" (
    call "%_VSINSTALLDIR15%\Common7\Tools\VsDevCmd.bat"
) ELSE (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\Preview\Enterprise\Common7\Tools\VsDevCmd.bat"
)

pushd %~dp0

if '%1%' neq '/PreserveContext' (
    cmd /k
)
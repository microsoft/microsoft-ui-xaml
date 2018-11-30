@echo OFF

pushd %~dp0

set PATH=%PATH%;%~dp0\tools

call %~dp0\tools\addaliases.cmd

IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat"
) ELSE (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\Preview\Enterprise\Common7\Tools\VsDevCmd.bat"
)

pushd %~dp0

if '%1%' neq '/PreserveContext' (
    cmd /k
)
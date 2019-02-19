@echo OFF

pushd %~dp0

set PATH=%PATH%;%~dp0\tools

call %~dp0\tools\addaliases.cmd

"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -Latest -requires Microsoft.Component.MSBuild -property InstallationPath > %TEMP%\vsinstalldir.txt

set /p _VSINSTALLDIR15=<%TEMP%\vsinstalldir.txt

call "%_VSINSTALLDIR15%\Common7\Tools\VsDevCmd.bat"

pushd %~dp0

if '%1%' neq '/PreserveContext' (
    cmd /k
)
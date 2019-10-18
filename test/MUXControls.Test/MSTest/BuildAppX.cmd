@echo off
setlocal

"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -Latest -requires Microsoft.Component.MSBuild -property InstallationPath > %TEMP%\vsinstalldir.txt
set /p _VsInstallDir=<%TEMP%\vsinstalldir.txt
call "%_VsInstallDir%\Common7\Tools\VsDevCmd.bat"

set Command=MSBuild.exe %1 /p:platform="%2" /p:configuration="%3" /p:VisualStudioVersion="%VisualStudioVersion%" /flp:Verbosity=Diagnostic /fl /verbosity:Minimal

echo.
echo Calling "%Command%"
echo.

%Command%
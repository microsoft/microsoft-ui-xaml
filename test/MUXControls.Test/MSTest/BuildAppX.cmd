@echo off

if "%DevEnvDir%" == "" (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise" (
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"
    ) else (
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat"
    )
)

set Command=MSBuild.exe %1 /p:platform="%2" /p:configuration="%3" /p:VisualStudioVersion="%VisualStudioVersion%" /flp:Verbosity=Diagnostic /fl /verbosity:Minimal

echo.
echo Calling "%Command%"
echo.

%Command%
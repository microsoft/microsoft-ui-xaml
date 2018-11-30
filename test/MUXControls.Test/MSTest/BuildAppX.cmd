@echo off

if "%DevEnvDir%" == "" (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise" (
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat"
    ) else (
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\Preview\Enterprise\Common7\Tools\VsDevCmd.bat"
    )
)

set Command="%VSINSTALLDIR%\MSBuild\15.0\Bin\MSBuild.exe" %1 /p:platform="%2" /p:configuration="%3" /p:VisualStudioVersion="15.0" /flp:Verbosity=Diagnostic /fl /verbosity:Minimal

echo.
echo Calling "%Command%"
echo.

%Command%
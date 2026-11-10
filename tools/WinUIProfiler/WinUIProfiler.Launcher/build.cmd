@echo off
setlocal
set "VSROOT=%~1"
set "DETOURSROOT=%~2"
set "SOURCEROOT=%~3"
set "OUTPUT=%~4"
set "ARCH=%~5"
set "VCTARGET=amd64"
set "LIBARCH=X64"
if /i "%ARCH%"=="arm64" (
    set "VCTARGET=amd64_arm64"
    set "LIBARCH=ARM64"
)
if /i "%ARCH%"=="x86" (
    set "VCTARGET=amd64_x86"
    set "LIBARCH=X86"
)
if /i "%~6"=="arm64" (
    set "VCTARGET=arm64"
    if /i "%ARCH%"=="x64" set "VCTARGET=arm64_x64"
    if /i "%ARCH%"=="x86" set "VCTARGET=arm64_x86"
)
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%VSROOT%\VC\Auxiliary\Build\vcvarsall.bat" %VCTARGET%
if errorlevel 1 exit /b 1

rem Some SDK installations have headers/libraries but no Universal CRT registry discovery.
if not defined UCRTVersion (
    if not exist "%WindowsSdkDir%Include\%WindowsSDKVersion%ucrt\ctype.h" (
        echo The selected Windows SDK is missing its Universal CRT headers.
        exit /b 1
    )
    if not exist "%WindowsSdkDir%Lib\%WindowsSDKVersion%ucrt\%ARCH%\ucrt.lib" (
        echo The selected Windows SDK is missing its %ARCH% Universal CRT libraries.
        exit /b 1
    )
    set "INCLUDE=%WindowsSdkDir%Include\%WindowsSDKVersion%ucrt;%INCLUDE%"
    set "LIB=%WindowsSdkDir%Lib\%WindowsSDKVersion%ucrt\%ARCH%;%LIB%"
)
set "DETOURS_TARGET_PROCESSOR=%ARCH%"
pushd "%DETOURSROOT%\src"
nmake /nologo
if errorlevel 1 exit /b 1
popd
pushd "%OUTPUT%"
cl /nologo /std:c++20 /W4 /WX /EHsc /MT /O2 /DUNICODE /D_UNICODE /I"%DETOURSROOT%\include" "%SOURCEROOT%\Redirect.cpp" /LD /link /DEF:"%SOURCEROOT%\Redirect.def" /OUT:ProfilerRedirect.dll "%DETOURSROOT%\lib.%LIBARCH%\detours.lib"
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /MT /O2 /DUNICODE /D_UNICODE /I"%DETOURSROOT%\include" "%SOURCEROOT%\Launcher.cpp" /link /SUBSYSTEM:WINDOWS /OUT:ProfilerLauncher.exe "%DETOURSROOT%\lib.%LIBARCH%\detours.lib" ole32.lib shell32.lib uuid.lib
exit /b %ERRORLEVEL%

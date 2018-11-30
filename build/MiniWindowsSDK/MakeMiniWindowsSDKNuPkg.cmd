@IF NOT DEFINED ECHOON ECHO OFF

SETLOCAL
set ERRORLEVEL=

PUSHD "%~dp0"

IF "%NUGETCMD%" == "" (

    WHERE /Q nuget >NUL
    IF %ERRORLEVEL% NEQ 0 ( 
        ECHO nuget not found.
        ECHO.
        ECHO Run "%~pd0download-nuget.cmd" to download the latest version, or update PATH as appropriate.
        GOTO END
    )

    set NUGETCMD=nuget.exe
)

SET VERSION=%1
SET PUBLIC_ROOT=%2


if DEFINED ECHOON echo NUGETCMD is %NUGETCMD%

csc MiniWindowsSDK_Dummy.cs /target:library

call %NUGETCMD% pack MiniWindowsSDK.nuspec %NUGET_ARGS% -version %VERSION% -properties PUBLIC_ROOT=%PUBLIC_ROOT%
IF %ERRORLEVEL% NEQ 0 GOTO END

:END

POPD
EXIT /B %ERRORLEVEL%
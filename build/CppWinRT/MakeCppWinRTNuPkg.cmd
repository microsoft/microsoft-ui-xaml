@IF NOT DEFINED ECHOON ECHO OFF

SETLOCAL ENABLEDELAYEDEXPANSION
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

if "%1" == "" goto :Usage

if "%2" == "" (
    SET CPPWINRTPATH=%1\x86\release\compiler.zip
    for %%a in ("%1") do set VERSION=%%~nxa
    echo using VERSION=!VERSION!
) else (
    SET VERSION=%1
    SET CPPWINRTPATH=%2\x86\release\compiler.zip
)

if DEFINED ECHOON echo NUGETCMD is %NUGETCMD%

REM \\redmond\osg\Threshold\Tools\CORE\DEP\DART\CppForWinRT\$VERSION$

if exist temp rmdir /s /q temp

powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('%CPPWINRTPATH%', 'temp'); }"

echo call %NUGETCMD% pack CppWinRT_SDK.nuspec %NUGET_ARGS% -version %VERSION% -properties CppWinRTPath="%CD%\temp"
call %NUGETCMD% pack CppWinRT_SDK.nuspec %NUGET_ARGS% -version %VERSION% -properties CppWinRTPath="%CD%\temp"
IF %ERRORLEVEL% NEQ 0 GOTO END

rmdir /s /q temp

:END

POPD
EXIT /B %ERRORLEVEL%

:Usage
@echo.
@echo MakeCppWinRTNuPkg.cmd [^<version^>] ^<path to CppWinRTBuild^>
@echo e.g. MakeCppWinRTNuPkg.cmd 1.0.170226.1 \\redmond\osg\Threshold\Tools\CORE\DEP\DART\CppForWinRT\1.0.170226.1
@echo -or- to use the version from the file share:
@echo e.g. MakeCppWinRTNuPkg.cmd \\redmond\osg\Threshold\Tools\CORE\DEP\DART\CppForWinRT\1.0.170226.1
@echo.
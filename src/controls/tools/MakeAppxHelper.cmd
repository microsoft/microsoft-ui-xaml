@echo OFF
SETLOCAL

echo -- MakeAppxHelper.cmd %* --

pushd %~dp0

set PATH=%TFS_SourcesDirectory%\tools;%PATH%;%ProgramFiles(x86)%\Windows Kits\10\bin\x86
set ExitCode=0
set ERRORLEVEL=

if "%1" NEQ "" (
    set TFS_PLATFORM=%1
)

if "%2" NEQ "" (
    set TFS_BUILDCONFIGURATION=%2
)

if "%TFS_PLATFORM%" EQU "" (
    echo Expecting TFS_PLATFORM to be set
    exit /b 1
)

if "%TFS_BUILDCONFIGURATION%" EQU "" (
    echo Expecting TFS_BUILDCONFIGURATION to be set
    exit /b 1
)

set BasePackageName=Microsoft.UI.Xaml

echo BUILDOUTPUT_OVERRIDE = %BUILDOUTPUT_OVERRIDE%
if "%BUILDOUTPUT_OVERRIDE%" == "" (
    set InputDirectory=%CD%\..\BuildOutput\%TFS_BUILDCONFIGURATION%\%TFS_PLATFORM%\Microsoft.UI.Xaml
    set OutputDirectory=%CD%\..\BuildOutput\%TFS_BUILDCONFIGURATION%\%TFS_PLATFORM%\FrameworkPackage
) else (
    set InputDirectory=%BUILDOUTPUT_OVERRIDE%\Microsoft.UI.Xaml
    set OutputDirectory=%BUILDOUTPUT_OVERRIDE%\FrameworkPackage
)

call ..\build\FrameworkPackage\MakeFrameworkPackage.cmd -InputDirectory '%InputDirectory%' ^
-OutputDirectory '%OutputDirectory%' -BasePackageName '%BasePackageName%' ^
-Platform %TFS_PLATFORM% -Configuration %TFS_BUILDCONFIGURATION% ^
 %3 %4 %5 %6 %7 %8 %9

if %ERRORLEVEL% NEQ 0 (
    @echo ##vso[task.logissue type=error;] MakeFrameworkPackage failed with exit code %ERRORLEVEL%
    goto END
)

:END
EXIT /B %ERRORLEVEL%

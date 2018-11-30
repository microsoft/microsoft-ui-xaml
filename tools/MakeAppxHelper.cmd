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

if "%XES_OUTDIR%" == "" (
	set WinMDInputs=%CD%\..\BuildOutput\%TFS_BUILDCONFIGURATION%\%TFS_PLATFORM%\Microsoft.UI.Xaml\Microsoft.UI.Xaml.winmd
	set OutputDirectory=%CD%\..\BuildOutput\%TFS_BUILDCONFIGURATION%\%TFS_PLATFORM%\FrameworkPackage
	set TestAppManifest=%CD%\..\BuildOutput\%TFS_BUILDCONFIGURATION%\%TFS_PLATFORM%\MUXControlsTestApp\AppxManifest.xml
) else (
	set WinMDInputs=%XES_OUTDIR%\Microsoft.UI.Xaml\Microsoft.UI.Xaml.winmd
	set OutputDirectory=%XES_OUTDIR%\FrameworkPackage
	set TestAppManifest=%XES_OUTDIR%\MUXControlsTestApp\AppxManifest.xml
)

call ..\build\FrameworkPackage\MakeFrameworkPackage.cmd -Inputs '%WinMDInputs%' ^
-OutputDirectory '%OutputDirectory%' -BasePackageName '%BasePackageName%' -PackageNameSuffix 3.0 ^
-Platform %TFS_PLATFORM% -Configuration %TFS_BUILDCONFIGURATION% ^
-TestAppManifest %TestAppManifest% %3 %4 %5 %6 %7 %8 %9

if %ERRORLEVEL% NEQ 0 (
	@echo ##vso[task.logissue type=error;] MakeFrameworkPackage failed with exit code %ERRORLEVEL%
	goto END
)

:END
EXIT /B %ERRORLEVEL%

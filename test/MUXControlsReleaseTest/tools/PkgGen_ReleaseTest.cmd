@if NOT DEFINED TF_BUILD echo off

SETLOCAL ENABLEDELAYEDEXPANSION

pushd %~dp0

if NOT DEFINED TFS_SourcesDirectory (
	pushd ..\..\..\
	set TFS_SourcesDirectory=!CD!
	popd
	echo Running locally, using TFS_SourcesDirectory=!TFS_SourcesDirectory!
)

set RELEASE_TEST_DIRECTORY=%TFS_SourcesDirectory%\test\MUXControlsReleaseTest
set NUGETROOT=%RELEASE_TEST_DIRECTORY%\packages

call nuget restore -PackagesDirectory %NUGETROOT% -Source "https://microsoft.pkgs.visualstudio.com/_packaging/WindowsES-External/nuget/v3/index.json" packages.pkggen.config

set SPKGGEN_PATH=%NUGETROOT%\microsoft.testinfrastructure.universaltest.1.0.20170906.2\tools\SpkgGen

set PATH=%RELEASE_TEST_DIRECTORY%\tools;%PATH%;%SPKGGEN_PATH%
set SIGN_WITH_TIMESTAMP=0
set ExitCode=0
set ERRORLEVEL=

if NOT DEFINED BUILDCONFIGURATION (
	if "%1" == "" goto :Usage
	set BUILDPLATFORM=%1
	set BUILDCONFIGURATION=%2
)
if NOT DEFINED BUILDPLATFORM goto :Usage
if NOT DEFINED RELATIVEOUTPUTROOT set RELATIVEOUTPUTROOT=%BUILDCONFIGURATION%\%BUILDPLATFORM%

echo BUILDPLATFORM=%BUILDPLATFORM% BUILDCONFIGURATION=%BUILDCONFIGURATION%

REM Skip PkgGen for arm64 as we don't have spkg support (or need) for it yet
if /I "%BUILDPLATFORM%" EQU "arm64" goto :END

if DEFINED TF_BUILD (
echo ++++++++++++++++ ENVIRONMENT ++++++++++++++++++++++++
set
echo ++++++++++++++++ ----------- ++++++++++++++++++++++++
)

REM Set this after MakeAppxHelper because we only want to redirect the pkggen output.
if NOT DEFINED XES_OUTDIR set XES_OUTDIR=%RELEASE_TEST_DIRECTORY%\BuildOutput\%BUILDCONFIGURATION%\%BUILDPLATFORM%

REM PkgGen.exe must be run from a directory above the current directory
cd %~dp0\..
 
REM Create the MitaLite SPKG file

set NUGETROOT=%USERPROFILE%\.nuget\packages
set APPSTESTVERSION=1.0.181203002
set NETCOREAPPVERSION=2.1.0
set TAEFVERSION=10.57.200817002-TaefAdapterWithNetCo

set VARIABLES=_RELEASEDIR=%XES_OUTDIR%
set VARIABLES=%VARIABLES%;_BINPLACEDIR=%SystemDrive%\data\test\bin
set VARIABLES=%VARIABLES%;APPSTESTROOT=%NUGETROOT%\microsoft.windows.apps.test\%APPSTESTVERSION%\lib\netcoreapp2.1
set VARIABLES=%VARIABLES%;NETCOREAPPROOT=%NUGETROOT%\runtime.win-%BUILDPLATFORM%.microsoft.netcore.app\%NETCOREAPPVERSION%\runtimes\win-%BUILDPLATFORM%
set VARIABLES=%VARIABLES%;TAEFROOT=%NUGETROOT%\taef.redist.wlk\%TAEFVERSION%\build\Binaries\%BUILDPLATFORM%
set VARIABLES=%VARIABLES%;RELATIVEOUTPUTROOT=%RELATIVEOUTPUTROOT%


set PKGPARAMS=/variables:"%VARIABLES%" /output:"%XES_OUTDIR%\prebuilt" /config:"%SPKGGEN_PATH%\pkggen.cfg.xml"

echo pkggen.exe "%RELEASE_TEST_DIRECTORY%\build\SpkgDefs\MUXControls.ReleaseTest.pkg.xml" %PKGPARAMS%

pkggen.exe "%RELEASE_TEST_DIRECTORY%\build\SpkgDefs\MUXControls.ReleaseTest.pkg.xml" %PKGPARAMS%
if %ERRORLEVEL% NEQ 0 (
    @echo ##vso[task.logissue type=error;] Creating SPKG failed with exit code %ERRORLEVEL%
    goto END
)

REM Generate package definition dynamically

call GenerateAppxDependenciesXML_ReleaseTest -sourceFile %XES_OUTDIR%\NugetPackageTestApp\AppPackages\NugetPackageTestApp_Test\NugetPackageTestApp.dependencies.txt -outputFile %XES_OUTDIR%\MUXControlsReleaseTest.dependencies.pkg.xml -BUILDPLATFORM %BUILDPLATFORM%

echo pkggen.exe %XES_OUTDIR%\MUXControlsReleaseTest.dependencies.pkg.xml %PKGPARAMS%
pkggen.exe %XES_OUTDIR%\MUXControlsReleaseTest.dependencies.pkg.xml %PKGPARAMS%
if %ERRORLEVEL% NEQ 0 (
    @echo ##vso[task.logissue type=error;] Creating SPKG failed with exit code %ERRORLEVEL%
    goto END
)

REM Manually gather up all our TAEF/MitaLite dependencies so that we run with ones that are in sync with our Nuget packages

echo pkggen.exe "%TFS_SourcesDirectory%\build\SpkgDefs\MUXControls.TestDependencies.%BUILDPLATFORM%.pkg.xml" %PKGPARAMS%
pkggen.exe "%TFS_SourcesDirectory%\build\SpkgDefs\MUXControls.TestDependencies.%BUILDPLATFORM%.pkg.xml" %PKGPARAMS%
if %ERRORLEVEL% NEQ 0 (
    @echo ##vso[task.logissue type=error;] Creating SPKG failed with exit code %ERRORLEVEL%
    goto END
)

:END
EXIT /B %ERRORLEVEL%

:Usage
echo Must set environment variables:
echo    BUILDPLATFORM = x86 ^| x64 ^| arm
echo    BUILDCONFIGURATION = debug ^| release

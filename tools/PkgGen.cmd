@if NOT DEFINED TF_BUILD echo off

SETLOCAL ENABLEDELAYEDEXPANSION

pushd %~dp0

if NOT DEFINED TFS_SourcesDirectory (
	pushd ..
	set TFS_SourcesDirectory=!CD!
	popd
	echo Running locally, using TFS_SourcesDirectory=!TFS_SourcesDirectory!
)

set NUGETROOT=%TFS_SourcesDirectory%\packages

call nuget restore -PackagesDirectory %NUGETROOT% -Source "https://microsoft.pkgs.visualstudio.com/_packaging/WindowsES-External/nuget/v3/index.json" packages.pkggen.config

set SPKGGEN_PATH=%NUGETROOT%\microsoft.testinfrastructure.universaltest.1.0.20170906.2\tools\SpkgGen

set PATH=%TFS_SourcesDirectory%\tools;%PATH%;%SPKGGEN_PATH%
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

if DEFINED TF_BUILD (
echo ++++++++++++++++ ENVIRONMENT ++++++++++++++++++++++++
set
echo ++++++++++++++++ ----------- ++++++++++++++++++++++++
)

call MakeAppxHelper.cmd %BUILDPLATFORM% %BUILDCONFIGURATION% -subversion %VERSIONBUILDREVISION:~-3%
if %ERRORLEVEL% NEQ 0 (
    @echo ##vso[task.logissue type=error;] Make AppxHelper failed with exit code %ERRORLEVEL%
    goto END
)

REM Skip PkgGen for arm64 as we don't have spkg support (or need) for it yet
if /I "%BUILDPLATFORM%" EQU "arm64" goto :END

REM Also skip for arm since it is not needed.
if /I "%BUILDPLATFORM%" EQU "arm" goto :END


REM Set this after MakeAppxHelper because we only want to redirect the pkggen output.
if NOT DEFINED XES_OUTDIR set XES_OUTDIR=%TFS_SourcesDirectory%\BuildOutput\%BUILDCONFIGURATION%\%BUILDPLATFORM%

REM PkgGen.exe must be run from a directory above the current directory
cd %~dp0\..
 
REM Create the test SPKG files

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

echo pkggen.exe "%TFS_SourcesDirectory%\build\SpkgDefs\MUXControls.Test.pkg.xml" %PKGPARAMS%

pkggen.exe "%TFS_SourcesDirectory%\build\SpkgDefs\MUXControls.Test.pkg.xml" %PKGPARAMS%
if %ERRORLEVEL% NEQ 0 (
    @echo ##vso[task.logissue type=error;] Creating SPKG failed with exit code %ERRORLEVEL%
    goto END
)

REM Package dependencies change per build, generate the package definition dynamically

call GenerateAppxDependenciesXML -sourceFile %XES_OUTDIR%\MUXControlsTestApp.TAEF\AppPackages\MUXControlsTestApp_Test\MUXControlsTestApp.dependencies.txt -outputFile %XES_OUTDIR%\MUXControls.dependencies.pkg.xml -BUILDPLATFORM %BUILDPLATFORM%

echo pkggen.exe %XES_OUTDIR%\MUXControls.dependencies.pkg.xml %PKGPARAMS%
pkggen.exe %XES_OUTDIR%\MUXControls.dependencies.pkg.xml %PKGPARAMS%
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

REM If we're not building for code coverage, then we'll generate an empty package so that deploying a package with the name we expect doesn't fail.
REM This will enable us to have a single Atlas test definition for both coverage and non-coverage runs.

if not exist %XES_OUTDIR%\prebuilt\Microsoft.MUXControls.CodeCoverageDependencies.cab (
    echo pkggen.exe "%TFS_SourcesDirectory%\build\SpkgDefs\MUXControls.CodeCoverageDependencies.Empty.pkg.xml" %PKGPARAMS%
    pkggen.exe "%TFS_SourcesDirectory%\build\SpkgDefs\MUXControls.CodeCoverageDependencies.Empty.pkg.xml" %PKGPARAMS%
    if %ERRORLEVEL% NEQ 0 (
        @echo ##vso[task.logissue type=error;] Creating SPKG failed with exit code %ERRORLEVEL%
        goto END
    )
)

:END
EXIT /B %ERRORLEVEL%

:Usage
echo Must set environment variables:
echo    BUILDPLATFORM = x86 ^| x64 ^| arm
echo    BUILDCONFIGURATION = debug ^| release

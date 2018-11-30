pushd %~dp0

SETLOCAL

set PATH=%TFS_SourcesDirectory%\tools;%PATH%;%ProgramFiles(x86)%\Windows Kits\10\Tools\bin\i386;%ProgramFiles(x86)%\Windows Kits\10\bin\x86
set SIGN_WITH_TIMESTAMP=0
set ExitCode=0
set ERRORLEVEL=

REM PkgGen.exe must be run from the root directory.
cd %~dp0..\..
 
set BINPLACEDIR=%SystemDrive%\data\test\bin
set NUGETROOT=%USERPROFILE%\.nuget\packages
set MAGELLANVERSION=5.4.170227001-pkges

set VARIABLES=_RELEASEDIR=%XES_OUTDIR%
set VARIABLES=%VARIABLES%;_BINPLACEDIR=%BINPLACEDIR%
set VARIABLES=%VARIABLES%;NUGETROOT=%USERPROFILE%\.nuget\packages
set VARIABLES=%VARIABLES%;MAGELLANVERSION=%MAGELLANVERSION%
set VARIABLES=%VARIABLES%;MAGELLANX86BINPLACEDIR=%BINPLACEDIR%\Magellan\tools\x86
set VARIABLES=%VARIABLES%;MAGELLANX64BINPLACEDIR=%BINPLACEDIR%\Magellan\tools\x64
set VARIABLES=%VARIABLES%;TOOLSPATH=%~dp0..

set PKGPARAMS=/variables:"%VARIABLES%" /output:"%XES_OUTDIR%\prebuilt" /config:"C:\Program Files (x86)\Windows Kits\10\Tools\bin\i386\pkggen.cfg.xml"

REM Manually package the version of Magellan that we used to instrument and helper scripts and files - we'll need to deploy it to the test machine.
REM The test machine doesn't have access to the build ID, but needs it to publish coverage data to a database, so we'll deploy it as part of the package.
echo %BUILD_BUILDID%_%BUILDPLATFORM% > %~dp0..\BuildID.txt

echo pkggen.exe "%TFS_SourcesDirectory%\build\SpkgDefs\MUXControls.CodeCoverageDependencies.pkg.xml" %PKGPARAMS%
pkggen.exe "%TFS_SourcesDirectory%\build\SpkgDefs\MUXControls.CodeCoverageDependencies.pkg.xml" %PKGPARAMS%
if %ERRORLEVEL% NEQ 0 (
    @echo ##vso[task.logissue type=error;] Creating SPKG failed with exit code %ERRORLEVEL%
    goto END
)

call %~dp0..\PkgGen.cmd

:END
EXIT /B %ERRORLEVEL%
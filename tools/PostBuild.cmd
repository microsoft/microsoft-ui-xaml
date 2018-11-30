ECHO ON
SETLOCAL enabledelayedexpansion
set ERRORLEVEL=0

echo Converting XES_DFSDROP to FQDN path...
echo XES_DFSDROP is currently %XES_DFSDROP%

for /F "tokens=1 delims=\" %%A in ('echo %XES_DFSDROP%') do (set XES_DFSDROP_DN=%%A)
call set XES_DFSDROP_FQDN=%%XES_DFSDROP:!XES_DFSDROP_DN!=!XES_DFSDROP_DN!.redmond.corp.microsoft.com%%
echo ##vso[task.setvariable variable=XES_DFSDROP;]%XES_DFSDROP_FQDN%

pushd %~dp0

set CHUNKCOUNT=0
if EXIST %XES_DFSDROP%\release\x86 (
    set /A CHUNKCOUNT=%CHUNKCOUNT%+1
    robocopy %XES_DFSDROP%\release\x86\prebuilt %XES_DFSDROP%\spkg\release\x86\prebuilt
)
if EXIST %XES_DFSDROP%\release\x64 (
    set /A CHUNKCOUNT=%CHUNKCOUNT%+1
    robocopy %XES_DFSDROP%\release\x64\prebuilt %XES_DFSDROP%\spkg\release\amd64\prebuilt
)
if EXIST %XES_DFSDROP%\release\arm (
    set /A CHUNKCOUNT=%CHUNKCOUNT%+1
    robocopy %XES_DFSDROP%\release\arm\prebuilt %XES_DFSDROP%\spkg\release\arm\prebuilt
)
if EXIST %XES_DFSDROP%\release\arm64 (
    set /A CHUNKCOUNT=%CHUNKCOUNT%+1
	REM we don't create arm64 spkgs (yet), so don't copy them.
)
if EXIST %XES_DFSDROP%\debug\x86 (
    set /A CHUNKCOUNT=%CHUNKCOUNT%+1
    robocopy %XES_DFSDROP%\debug\x86\prebuilt %XES_DFSDROP%\spkg\debug\x86\prebuilt
)
if EXIST %XES_DFSDROP%\debug\x64 (
    set /A CHUNKCOUNT=%CHUNKCOUNT%+1
    robocopy %XES_DFSDROP%\debug\x64\prebuilt %XES_DFSDROP%\spkg\debug\amd64\prebuilt
)
if EXIST %XES_DFSDROP%\debug\arm (
    set /A CHUNKCOUNT=%CHUNKCOUNT%+1
    robocopy %XES_DFSDROP%\debug\arm\prebuilt %XES_DFSDROP%\spkg\debug\arm\prebuilt
)
if EXIST %XES_DFSDROP%\debug\arm64 (
    set /A CHUNKCOUNT=%CHUNKCOUNT%+1
	REM we don't create arm64 spkgs (yet), so don't copy them.
)

echo Number of chunks available: %CHUNKCOUNT%

echo ++++++++++++++++ ENVIRONMENT ++++++++++++++++++++++++
set
echo ++++++++++++++++ ----------- ++++++++++++++++++++++++
echo ++++++++++++++++ OUTPUT DIRECTORY ++++++++++++++++++++++++
dir /s /b %XES_OUTDIR%
echo ++++++++++++++++ ----------- ++++++++++++++++++++++++

pushd %~dp0\..\build\NuSpecs

set NUGETCMD=\\edge-svcs\nuget\v3.5\nuget.exe
set BUILDOUTPUT=%XES_DFSDROP%
set OUTPUTDIR=%BUILD_BINARIESDIRECTORY%\PostBuild
set ECHOON=1

echo Calling build-nupkg for "prerelease" version.
echo build-nupkg.cmd -BuildOutput %BUILDOUTPUT% -OutputDir %OUTPUTDIR% -subversion %VERSIONBUILDREVISION:~-3% -prereleaseversion prerelease
call build-nupkg.cmd -BuildOutput %BUILDOUTPUT% -OutputDir %OUTPUTDIR% -subversion %VERSIONBUILDREVISION:~-3% -prereleaseversion prerelease
IF %ERRORLEVEL% NEQ 0 GOTO END

if /I "%MUXFINALRELEASE%" == "true" (
    echo Calling build-nupkg for "release" version.
    echo build-nupkg.cmd -BuildOutput %BUILDOUTPUT% -OutputDir %OUTPUTDIR%\Final -subversion %VERSIONBUILDREVISION:~-3%
    call build-nupkg.cmd -BuildOutput %BUILDOUTPUT% -OutputDir %OUTPUTDIR%\Final -subversion %VERSIONBUILDREVISION:~-3%
    IF !ERRORLEVEL! NEQ 0 GOTO END
)

popd

robocopy /S %BUILD_BINARIESDIRECTORY%\PostBuild %XES_DFSDROP%\PostBuildUnsigned

REM robocopy returns 0 for no files copied, 1 for files copied. (https://support.microsoft.com/en-us/kb/954404)
IF %ERRORLEVEL%==1 set ERRORLEVEL=0

:END
exit /B %ERRORLEVEL%
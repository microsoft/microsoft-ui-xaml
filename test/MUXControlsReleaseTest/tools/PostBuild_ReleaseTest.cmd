ECHO ON
SETLOCAL enabledelayedexpansion
set ERRORLEVEL=0

echo Converting XES_DFSDROP to FQDN path...
echo XES_DFSDROP is currently %XES_DFSDROP%

for /F "tokens=1 delims=\" %%A in ('echo %XES_DFSDROP%') do (set XES_DFSDROP_DN=%%A)
call set XES_DFSDROP_FQDN=%%XES_DFSDROP:!XES_DFSDROP_DN!=!XES_DFSDROP_DN!.redmond.corp.microsoft.com%%
echo ##vso[task.setvariable variable=XES_DFSDROP;]%XES_DFSDROP_FQDN%

pushd %~dp0

if EXIST %XES_DFSDROP%\release\x86 (
    robocopy %XES_DFSDROP%\release\x86\prebuilt %XES_DFSDROP%\spkg\release\x86\prebuilt
)
if EXIST %XES_DFSDROP%\release\x64 (
    robocopy %XES_DFSDROP%\release\x64\prebuilt %XES_DFSDROP%\spkg\release\amd64\prebuilt
)
if EXIST %XES_DFSDROP%\release\arm (
    robocopy %XES_DFSDROP%\release\arm\prebuilt %XES_DFSDROP%\spkg\release\arm\prebuilt
)
if EXIST %XES_DFSDROP%\release\arm64 (
	REM we don't create arm64 spkgs (yet), so don't copy them.
)
if EXIST %XES_DFSDROP%\debug\x86 (
    robocopy %XES_DFSDROP%\debug\x86\prebuilt %XES_DFSDROP%\spkg\debug\x86\prebuilt
)
if EXIST %XES_DFSDROP%\debug\x64 (
    robocopy %XES_DFSDROP%\debug\x64\prebuilt %XES_DFSDROP%\spkg\debug\amd64\prebuilt
)
if EXIST %XES_DFSDROP%\debug\arm (
    robocopy %XES_DFSDROP%\debug\arm\prebuilt %XES_DFSDROP%\spkg\debug\arm\prebuilt
)
if EXIST %XES_DFSDROP%\debug\arm64 (
	REM we don't create arm64 spkgs (yet), so don't copy them.
)

REM robocopy returns 0 for no files copied, 1 for files copied. (https://support.microsoft.com/en-us/kb/954404)
IF %ERRORLEVEL%==1 set ERRORLEVEL=0

:END
exit /B %ERRORLEVEL%
ECHO ON
SETLOCAL
set ERRORLEVEL=0

pushd %~dp0

if EXIST %XES_DFSDROP%\release\x86 (
    robocopy %XES_DFSDROP%\release\x86\prebuilt %XES_DFSDROP%\spkg\release\x86\prebuilt
)
if EXIST %XES_DFSDROP%\release\x64 (
    robocopy %XES_DFSDROP%\release\x64\prebuilt %XES_DFSDROP%\spkg\release\amd64\prebuilt
)

REM robocopy returns 0 for no files copied, 1 for files copied. (https://support.microsoft.com/en-us/kb/954404)
IF %ERRORLEVEL%==1 set ERRORLEVEL=0

:END
exit /B %ERRORLEVEL%
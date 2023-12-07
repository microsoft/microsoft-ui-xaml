ECHO ON
SETLOCAL
set ERRORLEVEL=

set RETRYCOUNT=0

:TryAgain
echo robocopy %BUILD_BINARIESDIRECTORY% %XES_DFSDROP% /E /R:50 /NP /XX
robocopy %BUILD_BINARIESDIRECTORY% %XES_DFSDROP% /E /R:50 /NP /XX

REM robocopy returns 0 for no files copied, 1 for files copied and random other values up to 8 for ok. (https://support.microsoft.com/en-us/kb/954404)
IF %ERRORLEVEL% LEQ 8 (
    set ERRORLEVEL=0
) ELSE IF %RETRYCOUNT% LEQ 2 (
    set /a RETRYCOUNT=RETRYCOUNT+1
    @echo ##vso[task.logissue type=warning;] Robocopy returned error, trying again %BUILD_BINARIESDIRECTORY% %XES_DFSDROP%
    goto :TryAgain
) ELSE (
    @echo ##vso[task.logissue type=error;] Robocopy failed %BUILD_BINARIESDIRECTORY% %XES_DFSDROP%
)

:END
exit /B %ERRORLEVEL%
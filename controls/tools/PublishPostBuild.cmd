ECHO ON
SETLOCAL
set ERRORLEVEL=

robocopy /s %BUILD_BINARIESDIRECTORY%\signed %XES_DFSDROP%\PostBuild

REM robocopy returns 0 for no files copied, 1 for files copied. (https://support.microsoft.com/en-us/kb/954404)
IF %ERRORLEVEL%==1 set ERRORLEVEL=0

exit /B %ERRORLEVEL%
@ECHO OFF
SETLOCAL
set ERRORLEVEL=

echo robocopy /E %~dp0..\..\packages\InternalWindowsSDK-part1.1.0.4\windowssdk-21361-part1 %~dp0..\..\packages\InternalWindowsSDK
call robocopy /E %~dp0..\..\packages\InternalWindowsSDK-part1.1.0.4\windowssdk-21361-part1 %~dp0..\..\packages\InternalWindowsSDK

echo robocopy /E %~dp0..\..\packages\InternalWindowsSDK-part2.1.0.4\windowssdk-21361-part2 %~dp0..\..\packages\InternalWindowsSDK
call robocopy /E %~dp0..\..\packages\InternalWindowsSDK-part2.1.0.4\windowssdk-21361-part2 %~dp0..\..\packages\InternalWindowsSDK

REM robocopy returns:
REM - 0x0 for no files copied,
REM - 0x1 for files copied, (https://support.microsoft.com/en-us/kb/954404)
REM - 0�2 for Some Extra files or directories were detected. Some housekeeping may be needed.
IF %ERRORLEVEL%==1 set ERRORLEVEL=0
IF %ERRORLEVEL%==2 set ERRORLEVEL=0
IF %ERRORLEVEL%==3 set ERRORLEVEL=0

exit /B %ERRORLEVEL%
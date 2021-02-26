@ECHO OFF
SETLOCAL
set ERRORLEVEL=

echo executing %~dp0..\..\packages\InternalWindowsSDK.1.0.2\winsdksetup.exe /quiet
call %~dp0..\..\packages\InternalWindowsSDK.1.0.2\winsdksetup.exe /quiet

:END
exit /B %ERRORLEVEL%
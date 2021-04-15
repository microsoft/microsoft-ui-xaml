@ECHO OFF
SETLOCAL
set ERRORLEVEL=

echo executing %~dp0..\..\packages\InternalWindowsSDK\winsdksetup.exe /quiet
call %~dp0..\..\packages\InternalWindowsSDK\winsdksetup.exe /quiet

dir "c:\program files (x86)\windows kits\10\References"

:END
exit /B %ERRORLEVEL%
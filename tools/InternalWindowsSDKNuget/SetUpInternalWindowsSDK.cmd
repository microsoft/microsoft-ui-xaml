@ECHO OFF
SETLOCAL
set ERRORLEVEL=

echo executing %~dp0..\..\packages\InternalWindowsSDK.1.0.7\winsdksetup.exe /features OptionId.UWPCpp OptionId.DesktopCPPx64 OptionId.DesktopCPPx86 OptionId.DesktopCPPARM64 OptionId.DesktopCPPARM /quiet
call %~dp0..\..\packages\InternalWindowsSDK.1.0.7\winsdksetup.exe /features OptionId.UWPCpp OptionId.DesktopCPPx64 OptionId.DesktopCPPx86 OptionId.DesktopCPPARM64 OptionId.DesktopCPPARM /quiet

:END
exit /B %ERRORLEVEL%
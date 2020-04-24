@echo off
REM %~dp0 is position of script file


if exist %~dp0"..\..\BuildOutput\Debug\x86\Microsoft.UI.Xaml" (
	call %~dp0..\..\tools\MakeAppxHelper.cmd x86 debug %*
)
if exist %~dp0"..\..\BuildOutput\Release\x86\Microsoft.UI.Xaml" (
	call %~dp0..\..\tools\MakeAppxHelper.cmd x86 release%*
)
if exist %~dp0"..\..\BuildOutput\Debug\x64\Microsoft.UI.Xaml" (
  call %~dp0..\..\tools\MakeAppxHelper.cmd x64 debug %*
)
if exist %~dp0"..\..\BuildOutput\Release\x64\Microsoft.UI.Xaml" (
  call %~dp0..\..\tools\MakeAppxHelper.cmd x64 release %*
)
if exist %~dp0"..\..\BuildOutput\Debug\arm\Microsoft.UI.Xaml" (
  call %~dp0..\..\tools\MakeAppxHelper.cmd arm debug %*
)
if exist %~dp0"..\..\BuildOutput\Release\arm\Microsoft.UI.Xaml" (
  call %~dp0..\..\tools\MakeAppxHelper.cmd arm release %*
)
if exist %~dp0"..\..\BuildOutput\Debug\arm64\Microsoft.UI.Xaml" (
  call %~dp0..\..\tools\MakeAppxHelper.cmd arm64 debug %*
)
if exist %~dp0"..\..\BuildOutput\Release\arm64\Microsoft.UI.Xaml" (
  call %~dp0..\..\tools\MakeAppxHelper.cmd arm64 release %*
)
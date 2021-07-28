@echo off

echo %~dp0..\..\NugetWrapper.cmd pack Microsoft.Internal.WinUI.WindowsPublicsWinmd.nuspec -properties PublicsDir="%PUBLIC_ROOT%%" -NoPackageAnalysis
call %~dp0..\..\NugetWrapper.cmd pack Microsoft.Internal.WinUI.WindowsPublicsWinmd.nuspec -properties PublicsDir="%PUBLIC_ROOT%%" -NoPackageAnalysis

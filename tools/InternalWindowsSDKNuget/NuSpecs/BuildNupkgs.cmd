@echo off

echo %~dp0..\..\NugetWrapper.cmd pack InternalWindowsSDK-part1.nuspec -properties PROGRAMFILES="%ProgramFiles(x86)%"
call %~dp0..\..\NugetWrapper.cmd pack InternalWindowsSDK-part1.nuspec -properties PROGRAMFILES="%ProgramFiles(x86)%"

echo %~dp0..\..\NugetWrapper.cmd pack InternalWindowsSDK-part2.nuspec -properties PROGRAMFILES="%ProgramFiles(x86)%"
call %~dp0..\..\NugetWrapper.cmd pack InternalWindowsSDK-part2.nuspec -properties PROGRAMFILES="%ProgramFiles(x86)%"
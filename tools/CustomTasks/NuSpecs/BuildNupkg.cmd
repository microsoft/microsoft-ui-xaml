@echo off

call %~dp0..\..\NugetWrapper.cmd pack MUXCustomBuildTasks.nuspec -properties PROGRAMFILES="%ProgramFiles(x86)%"
@echo off

call %~dp0..\..\NugetWrapper.cmd pack MUXPGODatabase.nuspec -properties PROGRAMFILES="%ProgramFiles(x86)%"
@echo off

call %~dp0..\..\NugetWrapper.cmd pack MUXCustomBuildTasks.nuspec -properties BuildOutput=%~dp0..\..\..\BuildOutput\Release\AnyCPU\CustomTasks -OutputDirectory %~dp0
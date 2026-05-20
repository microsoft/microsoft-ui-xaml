@echo off
setlocal enabledelayedexpansion

rem Derive a meaningful binlog name from the first project-ish arg
rem (sln/slnx/vcxproj/csproj/vbproj/proj). Fall back to cwd basename.
set _title=
for %%a in (%*) do (
    if not defined _title (
        set "_ext=%%~xa"
        if /I "!_ext!"==".sln"     set "_title=%%~na"
        if /I "!_ext!"==".slnx"    set "_title=%%~na"
        if /I "!_ext!"==".vcxproj" set "_title=%%~na"
        if /I "!_ext!"==".csproj"  set "_title=%%~na"
        if /I "!_ext!"==".vbproj"  set "_title=%%~na"
        if /I "!_ext!"==".proj"    set "_title=%%~na"
    )
)
if not defined _title for %%i in ("%CD%") do set "_title=%%~ni"
set "_binlog=%RepoRoot%\BuildOutput\%_title%.%_BuildArch%%_BuildType%.binlog"

msbuild /p:CLToolPath="C:\Program Files\LLVM\bin" /p:CLToolExe=clang-cl.exe /p:ExperimentalClang=true /bl:!_binlog! /fl /verbosity:minimal /m %*
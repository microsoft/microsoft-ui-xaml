@echo off
rem This script returns the latest commit to a given folder
setlocal
if "%1"=="" (
    set _dir=%cd%
) else (
    set _dir=%1
)

for /f "tokens=2" %%i in ('git log -1 %_dir%') do (echo %%i && goto:gotIt)
:gotIt

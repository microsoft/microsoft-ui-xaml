@echo off
REM -- Prepare the Command Processor --
SETLOCAL ENABLEEXTENSIONS
SETLOCAL DISABLEDELAYEDEXPANSION

::BatchSubstitude - parses a File line by line and replaces a substring"
::syntax: BatchSubstitude.bat OldStr NewStr File
::          OldStr  [in] - string to be replaced
::          NewFile [in] - file to replace with
::          File    [in] - file to be parsed
::          NewStr  [in] - string to replace with
:$changed 20100115
:$source http://www.dostips.com
if "%~1"=="" findstr "^::" "%~f0"&GOTO:EOF
for /f "tokens=1,* delims=]" %%A in ('"type %3|find /n /v """') do (
    set "line=%%B"
    if [%%B]==[%1] type %2
    if defined line (
        call set "line=echo.%%line:%~1=%~4%%"
        for /f "delims=" %%X in ('"echo."%%line%%""') do %%~X
    ) ELSE echo.
)

@echo off
rem usage: RunHelixWorkItem [testbinaries] (/taefQuery [taefQuery] /taefParameters [taefParameters])

: Due to the fact that the value for taefQuery is a complex expression including special symbols,
: we cannot directly pass it to Powershell. Instead we pass the arguments via a file which 
: the script will parse.
echo %* > args.txt
copy /y %HELIX_CORRELATION_PAYLOAD%\RunHelixWorkItem.ps1 . >nul
powershell -NonInteractive -ExecutionPolicy Bypass .\RunHelixWorkItem.ps1 -ArgsFile .\args.txt
set "runExitCode=%ERRORLEVEL%"
if exist args.txt del /q args.txt
exit /b %runExitCode%
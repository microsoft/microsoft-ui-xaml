@echo off
set packagestate=%3
echo Package state :  %packagestate%
echo %* > args.txt
copy /y %HELIX_CORRELATION_PAYLOAD%\RunPerfWorkItem.ps1 .
powershell -NonInteractive -ExecutionPolicy Bypass .\RunPerfWorkItem.ps1 -PackageState %packagestate% -ArgsFile .\args.txt
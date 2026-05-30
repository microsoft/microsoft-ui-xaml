@echo off
echo Time: %TIME%
powershell -NonInteractive -ExecutionPolicy Bypass %~dp0\CreateTestPayload.ps1 %*
set result=%ERRORLEVEL%
echo Time: %TIME%
exit /b %result%

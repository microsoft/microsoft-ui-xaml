@echo off
echo Time: %TIME%
powershell -NonInteractive -ExecutionPolicy Bypass %~dp0\CreateTestPayload.ps1 %*
echo Time: %TIME%

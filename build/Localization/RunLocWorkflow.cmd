@echo off

pushd %~dp0
powershell -ExecutionPolicy Unrestricted -NoLogo -NoProfile -Command "&{ %~dpn0.ps1 %*; exit $lastexitcode  }"

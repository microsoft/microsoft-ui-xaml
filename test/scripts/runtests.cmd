@powershell.exe -NonInteractive -NoProfile -ExecutionPolicy Bypass -File "%~dp0runtests.ps1" %*
@exit /b %ERRORLEVEL%

@echo OFF

%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Unrestricted -NoLogo -NoProfile -Command "&{ %*; exit $lastexitcode }" || set ERRORLEVEL=1
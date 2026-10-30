@echo OFF

rem Let Windows PowerShell rebuild its module path instead of inheriting PowerShell 7 locations.
rem Keep the reset local so the caller's environment is unchanged.
setlocal
set "PSModulePath="

%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Unrestricted -NoLogo -NoProfile -Command "&{ %*; exit $lastexitcode }" || set ERRORLEVEL=1
endlocal & exit /b %ERRORLEVEL%
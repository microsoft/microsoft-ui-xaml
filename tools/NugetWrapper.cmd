@echo OFF
setlocal

set VisualStudioVersion=15.0

\\edge-svcs\nuget\v4.6.2\NuGet.exe %*

exit /B %ERRORLEVEL%
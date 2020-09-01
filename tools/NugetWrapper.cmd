@echo OFF
setlocal

if "%VisualStudioVersion%" == "" set VisualStudioVersion=15.0

if defined NUGETEXETOOLPATH goto :AzurePipelines

if not exist %TEMP%\nuget.4.9.2.exe (
    echo Nuget.exe not found in the temp dir, downloading.
    powershell -Command "& { Invoke-WebRequest https://dist.nuget.org/win-x86-commandline/v4.9.2/nuget.exe -outfile $env:TEMP\nuget.4.9.2.exe }"
)

%TEMP%\nuget.4.9.2.exe %*

exit /B %ERRORLEVEL%



:AzurePipelines
echo NUGETEXETOOLPATH = %NUGETEXETOOLPATH%

%NUGETEXETOOLPATH% %*
exit /B %ERRORLEVEL%

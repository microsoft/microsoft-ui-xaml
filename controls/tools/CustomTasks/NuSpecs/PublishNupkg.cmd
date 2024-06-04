@ECHO OFF
SETLOCAL
set ERRORLEVEL=

for %%A IN (%~dp0MUXCustomBuildTasks.??.??.*.nupkg) do (
    echo Candidate nuget package: %%A
    echo nuget push %%A -ConfigFile %~dp0..\..\..\..\nuget.config -Source WinUI.Dependencies -apikey AzureDevOps
    nuget push %%A -ConfigFile %~dp0..\..\..\..\nuget.config -Source WinUI.Dependencies -apikey AzureDevOps
)

:END
exit /B %ERRORLEVEL%

@ECHO OFF
SETLOCAL
set ERRORLEVEL=

for %%A IN (%~dp0MUXCustomBuildTasks.??.??.*.nupkg) do (
    echo Candidate nuget package: %%A
    echo %~dp0..\..\NugetWrapper.cmd push %%A -Source https://pkgs.dev.azure.com/ms/microsoft-ui-xaml/_packaging/MUX-Dependencies/nuget/v3/index.json -apikey VSTS
    %~dp0..\..\NugetWrapper.cmd push %%A -Source https://pkgs.dev.azure.com/ms/microsoft-ui-xaml/_packaging/MUX-Dependencies/nuget/v3/index.json -apikey VSTS
)

:END
exit /B %ERRORLEVEL%
@ECHO OFF
SETLOCAL
set ERRORLEVEL=

for %%A IN (%~dp0MUXCustomBuildTasks.??.??.*.nupkg) do (
    echo Candidate nuget package: %%A
    echo %~dp0..\..\NugetWrapper.cmd push %%A -Source https://microsoft.pkgs.visualstudio.com/DefaultCollection/_packaging/DEPControls/nuget/v3/index.json -apikey VSTS
    %~dp0..\..\NugetWrapper.cmd push %%A -Source https://microsoft.pkgs.visualstudio.com/DefaultCollection/_packaging/DEPControls/nuget/v3/index.json -apikey VSTS
)

:END
exit /B %ERRORLEVEL%
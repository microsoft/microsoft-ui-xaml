@ECHO OFF
SETLOCAL
set ERRORLEVEL=

for %%A IN (%~dp0MUXPGODatabase.??.??.*.nupkg) do (
    echo Candidate nuget package: %%A
    echo %~dp0..\..\NugetWrapper.cmd push %%A -Source https://microsoft.pkgs.visualstudio.com/DefaultCollection/_packaging/DEPControls/nuget/v3/index.json -apikey VSTS
    rem you need to get an authorized version of nuget.exe from https://dev.azure.com/microsoft/WinUI/_packaging?_a=feed&feed=DEPControls
     %~dp0..\..\NugetWrapper.cmd push %%A -Source https://microsoft.pkgs.visualstudio.com/DefaultCollection/_packaging/DEPControls/nuget/v3/index.json -apikey VSTS
)

:END
exit /B %ERRORLEVEL%
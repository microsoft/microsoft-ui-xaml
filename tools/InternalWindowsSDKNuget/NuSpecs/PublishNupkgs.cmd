@ECHO OFF
SETLOCAL
set ERRORLEVEL=

for %%A IN (%~dp0InternalWindowsSDK-part1.??.??.*.nupkg) do (
    echo Candidate nuget package: %%A
    echo %~dp0..\..\NugetWrapper.cmd push %%A -Source https://pkgs.dev.azure.com/microsoft/WinUI/_packaging/WinUIInternalWindowsSDK/nuget/v3/index.json -apikey VSTS
    call %~dp0..\..\NugetWrapper.cmd push %%A -Source https://pkgs.dev.azure.com/microsoft/WinUI/_packaging/WinUIInternalWindowsSDK/nuget/v3/index.json -apikey VSTS
)

for %%A IN (%~dp0InternalWindowsSDK-part2.??.??.*.nupkg) do (
    echo Candidate nuget package: %%A
    echo %~dp0..\..\NugetWrapper.cmd push %%A -Source https://pkgs.dev.azure.com/microsoft/WinUI/_packaging/WinUIInternalWindowsSDK/nuget/v3/index.json -apikey VSTS
    call %~dp0..\..\NugetWrapper.cmd push %%A -Source https://pkgs.dev.azure.com/microsoft/WinUI/_packaging/WinUIInternalWindowsSDK/nuget/v3/index.json -apikey VSTS
)

:END
exit /B %ERRORLEVEL%
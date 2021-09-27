@ECHO OFF
SETLOCAL
set ERRORLEVEL=

for %%A IN (%~dp0Microsoft.Internal.WinUI.WindowsPublicsWinmd.??.??.*.nupkg) do (
    echo Candidate nuget package: %%A
    echo %~dp0..\..\NugetWrapper.cmd push %%A -Source https://pkgs.dev.azure.com/microsoft/WinUI/_packaging/WinUIInternalWindowsSDK/nuget/v3/index.json -apikey VSTS
    call %~dp0..\..\NugetWrapper.cmd push %%A -Source https://pkgs.dev.azure.com/microsoft/WinUI/_packaging/WinUIInternalWindowsSDK/nuget/v3/index.json -apikey VSTS
)

:END
exit /B %ERRORLEVEL%
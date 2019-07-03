@ECHO OFF
SETLOCAL
set ERRORLEVEL=

for %%A IN (%~dp0MUXPGODatabase.??.??.*.nupkg) do (
    echo Candidate nuget package: %%A
    echo %~dp0..\..\NugetWrapper.cmd push %%A -Source https://pkgs.dev.azure.com/ms/_packaging/MUX-CI/nuget/v3/index.json -apikey VSTS
rem you need to get an authorized version of nuget.exe from https://dev.azure.com/ms/microsoft-ui-xaml/_packaging?_a=package&feed=MUX-CI&package=Microsoft.UI.Xaml&protocolType=NuGet&version=2.2.190702001-prerelease
    "C:\Users\ranjeshj\Desktop\CredentialProviderBundle\NuGet.exe" push %%A -Source https://pkgs.dev.azure.com/ms/_packaging/MUX-CI/nuget/v3/index.json -apikey VSTS
)

:END
exit /B %ERRORLEVEL%
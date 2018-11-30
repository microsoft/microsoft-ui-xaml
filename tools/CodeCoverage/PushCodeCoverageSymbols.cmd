@echo off
setlocal

%~dp0\..\PowershellWrapper.cmd %~dp0\PushCodeCoverageSymbols.ps1 "-MagellanInstallPath """%USERPROFILE%\.nuget\packages\microsoft.internal.magellan\5.4.170227001-pkges""" -SqlConnectionString """Server=tcp:pkgesmegdep.westus2.cloudapp.azure.com,1433;Database=%BUILD_BUILDID%_%BUILDPLATFORM%;User ID=MagellanUser;Password=PkgesCCData1!;Trusted_Connection=False;Connection Timeout=30;""" -CoverageSymPath %XES_OUTDIR%\Microsoft.UI.Xaml\instr"
exit /B %ERRORLEVEL%
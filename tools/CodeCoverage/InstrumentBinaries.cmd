@echo off

%~dp0\..\PowershellWrapper.cmd %~dp0\InstrumentBinaries.ps1 -BuildPlatform %BUILDPLATFORM% -BuildConfiguration %BUILDCONFIGURATION%
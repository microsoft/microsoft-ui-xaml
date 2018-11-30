@echo off

set ERRORLEVEL=

REM echo ----------------------------- SDK DIR DUMP -----------------------------
REM dir /s /b "C:\Windows\Microsoft.NET\Framework"
REM echo ----------------------------- END DIR DUMP -----------------------------

REM if not defined NETFXSDKDir (call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\Common7\Tools\vsvars32.bat")

pushd %~dp0
powershell -ExecutionPolicy Unrestricted -NoLogo -NoProfile -Command "&{ %~dpn0.ps1 %*; exit $lastexitcode  }"

if %ERRORLEVEL% NEQ 0 (
	@echo ##vso[task.logissue type=error;] MakeFrameworkPackage failed with exit code %ERRORLEVEL%
	goto END
)

:END
EXIT /B %ERRORLEVEL%
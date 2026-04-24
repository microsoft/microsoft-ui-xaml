@ECHO OFF
:: Copyright (c) Microsoft Corporation.
:: Licensed under the MIT License.

SETLOCAL

if "%1"=="/?" goto :usage
if "%1"=="-?" goto :usage

set DESTINATION=%SystemDrive%\data\test\bin\AppAnalysis

if "%1" neq "" (
  set DESTINATION=%1
)

IF NOT EXIST %DESTINATION% (
  mkdir %DESTINATION%
)
IF NOT EXIST %DESTINATION%\en-us\ (
  mkdir %DESTINATION%\en-us\
)

if %ERRORLEVEL%==0 goto copy

echo.
echo ERROR: Please check that deploy location %DESTINATION% is valid.
echo. 

exit /b /1

:copy
echo Copying AppAnalysis and dependencies to %DESTINATION%
xcopy /y /q  %0\..\microsoft.diagnostics.appanalysis.dll %DESTINATION%
if NOT %ERRORLEVEL%==0 goto error
xcopy /y /q  %0\..\appanalysistool.exe %DESTINATION%
if NOT %ERRORLEVEL%==0 goto error
xcopy /y /q  %0\..\AppAnalysisReportStylesheet.xslt %DESTINATION%
if NOT %ERRORLEVEL%==0 goto error
xcopy /y /q  %0\..\microsoft.diagnostics.appanalysis.dll.mui %DESTINATION%\en-us\
if NOT %ERRORLEVEL%==0 goto error

REM Most users won't actually need the manifest since the events are already on the machine.
REM This is really just for decoding events on a machine that doesn't have the events. 
if EXIST %0\..\..\NTTEST\WINDOWSTEST\dxaml\xcp\Microsoft-Windows-XAML-ETW.man (
   xcopy /y /q  %0\..\..\NTTEST\WINDOWSTEST\dxaml\xcp\Microsoft-Windows-XAML-ETW.man %DESTINATION%
) else (
   echo Event manifest not found. Post-mortem processing of an ETL file may not work.
)

REM only try to copy this dll if it exists, we don't require it to function.
if EXIST %0\..\xperf_coresys\KernelTraceControl.dll (
   xcopy /y /q  %0\..\xperf_coresys\KernelTraceControl.dll %DESTINATION% 
)

goto done

:error
echo.
echo Failed to deploy. Check that binaries exist on the build share
echo. 

exit /b /1

:usage
echo.
echo Deploys AppAnalysisTool.exe and other necessary binaries from the build share to %SystemDrive%\data\test\bin\AppAnalysis.
echo Should be run directly from the build share to copy the binaries from.
echo.
echo Usage:
echo     deployAppAnalysis.cmd [customDeployLocation]
echo.
echo        customDeployLocation:   Optional. Overrides the default location of where the tool is deployed to.
echo.
echo     Examples: ^<build-output^>\bin\appanalysis\deployAppAnalysis.cmd
echo               ^<build-output^>\bin\appanalysis\deployAppAnalysis.cmd c:\temp\

exit /b /1

:done

echo Successfully deployed AppAnalysis to %DESTINATION%! See AppAnalysisTool.exe -? for usage.

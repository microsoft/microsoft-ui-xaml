SETLOCAL EnableDelayedExpansion

robocopy %HELIX_CORRELATION_PAYLOAD% . /s /NP

reg add HKLM\Software\Policies\Microsoft\Windows\Appx /v AllowAllTrustedApps /t REG_DWORD /d 1 /f

cd scripts
powershell -ExecutionPolicy Bypass .\InstallTestAppDependencies.ps1
cd ..

te MUXControls.Test.dll MUXControlsTestApp.appx IXMPTestApp.appx /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError %* 
set taeferrorlevel=%errorlevel%

%HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result te.wtl -result_name te.wtl

FOR %%I in (WexLogFileOutput\*.jpg) DO (
    echo Uploading %%I to "%HELIX_RESULTS_CONTAINER_URI%/%%~nI%%~xI%HELIX_RESULTS_CONTAINER_RSAS%"
    %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result %%I -result_name %%~nI%%~xI 
)

if "%taeferrorlevel%"=="0" (goto uploadlogs)


REM Upload appx logs
cd scripts
TDRDump.exe > tdrdump.txt
echo Uploading tdrdump.txt to "%HELIX_RESULTS_CONTAINER_URI%/tdrdump.txt%HELIX_RESULTS_CONTAINER_RSAS%"
%HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result tdrdump.txt -result_name tdrdump.txt
cd ..

cd scripts
powershell -ExecutionPolicy Bypass .\runappxdiag.ps1 start
cd ..

REM Run a dummy test to try appx install
te MUXControls.Test.dll MUXControlsTestApp.appx IXMPTestApp.appx /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /name:*ColorPickerTests.CanSelectColorFromSpectrumLTR

cd scripts
powershell -ExecutionPolicy Bypass .\runappxdiag.ps1 stop
cd ..

FOR %%I in (scripts\*.zip) DO (
    echo Uploading %%I to "%HELIX_RESULTS_CONTAINER_URI%/%%~nI%%~xI%HELIX_RESULTS_CONTAINER_RSAS%"
    %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result %%I -result_name %%~nI%%~xI 
)


:uploadlogs
cd scripts
powershell -ExecutionPolicy Bypass .\ConvertWttLogToXUnit.ps1 ..\te.wtl ..\testResults.xml %testnameprefix%
cd ..

type testResults.xml
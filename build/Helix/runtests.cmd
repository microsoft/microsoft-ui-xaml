set

robocopy %HELIX_CORRELATION_PAYLOAD% . /s /NP

te MUXControls.Test.dll MUXControlsTestApp.appx IXMPTestApp.appx /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError %* 

dir /b /s

%HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result te.wtl -result_name te.wtl

FOR %%I in (WexLogFileOutput\*.jpg) DO (
    echo Uploading %%I to %HELIX_RESULTS_CONTAINER_URI%/%%I%HELIX_RESULTS_CONTAINER_RSAS%
    %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result %%I -result_name %%~nI%%~xI 
)

set linkToUploadedWtlLog = %HELIX_RESULTS_CONTAINER_URI%te.wtl%HELIX_RESULTS_CONTAINER_RSAS%

cd scripts
powershell -ExecutionPolicy Bypass .\ConvertWttLogToXUnit.ps1 ..\te.wtl ..\testResults.xml %testnameprefix% %linkToUploadedWtlLog% %HELIX_RESULTS_CONTAINER_URI% %HELIX_RESULTS_CONTAINER_RSAS%
cd ..

type testResults.xml
robocopy %HELIX_CORRELATION_PAYLOAD% . /s

te MUXControls.Test.dll MUXControlsTestApp.appx IXMPTestApp.appx /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 %*

%HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result te.wtl -result_name kmahone-te.wtl

cd scripts
powershell -ExecutionPolicy Bypass .\ConvertWttLogToXUnit.ps1 ..\te.wtl ..\testResults.xml %testnameprefix%
cd ..

type testResults.xml
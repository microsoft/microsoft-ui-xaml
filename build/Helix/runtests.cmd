robocopy %HELIX_CORRELATION_PAYLOAD% . /s
te MUXControls.Test.dll MUXControlsTestApp.appx /enablewttlogging /unicodeOutput:false /testtimeout:0:05 %*
type te.wtl
cd scripts
powershell .\ConvertWttLogToXUnit.ps1 ..\te.wtl ..\testResults.xml
cd ..
type testResults.xml
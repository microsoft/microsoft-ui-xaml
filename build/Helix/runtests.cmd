echo hello from runtests.cmd
dir /b /s
set
robocopy %HELIX_CORRELATION_PAYLOAD% . /s
dir /b /s
te MUXControls.Test.dll MUXControlsTestApp.appx /enablewttlogging /unicodeOutput:false /testtimeout:0:05 %*
dir *.wtl
type te.wtl
mkdir foo
cd foo
powershell ..\ConvertWttLogToXUnit.ps1 ..\te.wtl ..\testResults.xml
cd ..
type testResults.xml
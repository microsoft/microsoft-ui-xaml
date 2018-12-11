echo hello from runtests.cmd
dir /b /s
set
robocopy %HELIX_CORRELATION_PAYLOAD% . /s
dir /b /s
te MUXControls.Test.dll MUXControlsTestApp.appx /unicodeOutput:false /testtimeout:0:05 %*
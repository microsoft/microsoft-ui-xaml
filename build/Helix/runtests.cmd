echo hello from runtests.cmd
dir /b /s
set
robocopy %HELIX_CORRELATION_PAYLOAD% . /s
dir /b /s
te MUXControls.Test.dll /list /unicodeOutput:false
te MUXControls.Test.dll /unicodeOutput:false /testtimeout:0:01 %*
echo hello from runtests.cmd
dir /b /s
set
te MUXControls.Test.dll /list
te MUXControls.Test.dll /name:*ColorPickerTests.CanSelectPreviousColor
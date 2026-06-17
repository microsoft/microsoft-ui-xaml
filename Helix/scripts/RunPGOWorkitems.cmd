@REM IXP Modules will fail the training test. TODO: Fix IXP module PGO training failures.
@REM set IXP_MODULES=dwmcorei.dll!dcompi.dll!marshal.dll!Microsoft.DirectManipulation.dll!Microsoft.InputStateManager.dll!Microsoft.UI.Input.dll
set IXP_MODULES=
@REM set HELIX_CORRELATION_PAYLOAD=.

set MODULES=Microsoft.UI.Xaml.dll!Microsoft.UI.Xaml.Controls.dll!WinUIEdit.dll
call %HELIX_CORRELATION_PAYLOAD%\RunPerfWorkItemHelper.cmd packaged XamlPGO.HelloWorld.Cs %MODULES% 40000

set MODULES=Microsoft.UI.Xaml.dll!Microsoft.UI.Xaml.Controls.dll!Microsoft.UI.Xaml.Phone.dll!%IXP_MODULES%
call %HELIX_CORRELATION_PAYLOAD%\RunPerfWorkItemHelper.cmd packaged XamlPGO.CommonControls1 %MODULES% 15000

set MODULES=Microsoft.UI.Xaml.dll!Microsoft.UI.Xaml.Controls.dll
call %HELIX_CORRELATION_PAYLOAD%\RunPerfWorkItemHelper.cmd unpackaged SimpleIslandApp %MODULES% 50000

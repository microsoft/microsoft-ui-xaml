REM %~dp0 is position of script file
call %~dp0..\..\tools\MakeAppxHelper.cmd x86 release %*
call %~dp0..\..\tools\MakeAppxHelper.cmd x86 debug %*
call %~dp0..\..\tools\MakeAppxHelper.cmd x64 release %*
call %~dp0..\..\tools\MakeAppxHelper.cmd x64 debug %*
call %~dp0..\..\tools\MakeAppxHelper.cmd arm release %*
call %~dp0..\..\tools\MakeAppxHelper.cmd arm debug %*
REM Exit with 0 since some script may fail but we don't want to block pipeline
REM If one of the scripts failed unexpectedly, later steps in pipeline will fail 
EXIT 0
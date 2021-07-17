pushd %~dp0
echo "Calling DevCmd.cmd"
call ..\DevCmd.cmd /PreserveContext
pushd %~dp0
echo "Calling CreateCBSVpack.ps1"
powershell -ExecutionPolicy Unrestricted -NonInteractive -Command %~dpn0.ps1 %*
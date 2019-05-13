setlocal ENABLEDELAYEDEXPANSION

robocopy %HELIX_CORRELATION_PAYLOAD% . /s /NP

reg add HKLM\Software\Policies\Microsoft\Windows\Appx /v AllowAllTrustedApps /t REG_DWORD /d 1 /f

powershell -ExecutionPolicy Bypass scripts\InstallTestAppDependencies.ps1

set testBinaryCandidates=MUXControls.Test.dll MUXControlsTestApp.appx IXMPTestApp.appx MUXControls.ReleaseTest.dll NugetPackageTestApp.appx NugetPackageTestAppCX.appx
set testBinaries=
for %%B in (%testBinaryCandidates%) do (
    if exist %%B (
        set "testBinaries=!testBinaries! %%B"
    )
)

te %testBinaries% /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError %*

%HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result te.wtl -result_name te.wtl

FOR %%I in (WexLogFileOutput\*.jpg) DO (
    echo Uploading %%I to "%HELIX_RESULTS_CONTAINER_URI%/%%I%HELIX_RESULTS_CONTAINER_RSAS%"
    %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result %%I -result_name %%~nI%%~xI 
)

move te.wtl te_old.wtl

for /F "tokens=* usebackq" %%A IN (`powershell -ExecutionPolicy Bypass scripts\OutputFailedTests.ps1 .\te.wtl`) DO (
  set FailingTestQuery=%%A
)

te %testBinaries% /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError /testmode:Loop /LoopTest:10 /select:"%FailingTestQuery%"

%HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result te.wtl -result_name te_rerun.wtl

FOR %%I in (WexLogFileOutput\*.jpg) DO (
    echo Uploading %%I to "%HELIX_RESULTS_CONTAINER_URI%/%%I%HELIX_RESULTS_CONTAINER_RSAS%"
    %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result %%I -result_name %%~nI%%~xI_rerun
)

echo.
echo te_old.wtl:
echo.

type te_old.wtl

echo.
echo te.wtl:
echo.

type te.wtl

powershell -ExecutionPolicy Bypass scripts\ConvertWttLogToXUnit.ps1 .\te.wtl .\testResults.xml %testnameprefix%

type testResults.xml
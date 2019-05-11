setlocal ENABLEDELAYEDEXPANSION

rem cd scripts
for /F "tokens=* usebackq" %%A IN (`powershell -ExecutionPolicy Bypass .\OutputFailedTests.ps1 C:\Temp\te.wtl`) DO (
  set FailingTestQuery=%%A
)

echo %FailingTestQuery%
rem cd ..
goto:eof

robocopy %HELIX_CORRELATION_PAYLOAD% . /s /NP

reg add HKLM\Software\Policies\Microsoft\Windows\Appx /v AllowAllTrustedApps /t REG_DWORD /d 1 /f

cd scripts
powershell -ExecutionPolicy Bypass .\InstallTestAppDependencies.ps1
cd ..

set testBinaryCandidates=MUXControls.Test.dll MUXControlsTestApp.appx IXMPTestApp.appx MUXControls.ReleaseTest.dll NugetPackageTestApp.appx NugetPackageTestAppCX.appx
set testBinaries=
for %%B in (%testBinaryCandidates%) do (
    if exist %%B (
        set "testBinaries=!testBinaries! %%B"
    )
)

te %testBinaries% /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError %*

cd scripts
for /F "tokens=* usebackq" %%A IN (`powershell -ExecutionPolicy Bypass .\OutputFailedTests.ps1 .\te.wtl`) DO (
  set FailingTestQuery=%%A
)
cd ..

te %testBinaries% /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError /testmode:Loop /LoopTest:10 /select:"%FailingTestQuery%"

%HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result te.wtl -result_name te.wtl

FOR %%I in (WexLogFileOutput\*.jpg) DO (
    echo Uploading %%I to "%HELIX_RESULTS_CONTAINER_URI%/%%I%HELIX_RESULTS_CONTAINER_RSAS%"
    %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result %%I -result_name %%~nI%%~xI 
)

move te.wtl te_old.wtl

type te_old.wtl
type te.wtl

cd scripts
powershell -ExecutionPolicy Bypass .\ConvertWttLogToXUnit.ps1 ..\te.wtl ..\testResults.xml %testnameprefix%
cd ..

type testResults.xml
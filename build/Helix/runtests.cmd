setlocal ENABLEDELAYEDEXPANSION

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

move te.wtl te_original.wtl
%HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result te_original.wtl -result_name te_original.wtl

FOR %%I in (WexLogFileOutput\*.jpg) DO (
    echo Uploading %%I to "%HELIX_RESULTS_CONTAINER_URI%/%%I%HELIX_RESULTS_CONTAINER_RSAS%"
    %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result %%I -result_name %%~nI%%~xI 
)

move te.wtl te_original.wtl

cd scripts
set FailedTestQuery=
for /F "tokens=* usebackq" %%A IN (`powershell -ExecutionPolicy Bypass .\OutputFailedTestQuery.ps1 ..\te_original.wtl`) DO (
  set FailedTestQuery=%%A
)
cd ..

rem The first time, we'll just re-run failed tests once.  In many cases, tests fail very rarely, such that
rem a single re-run will be sufficient to detect many unreliable tests.
if "%FailedTestQuery%" neq "" (
    te %testBinaries% /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError /select:"%FailedTestQuery%"

    move te.wtl te_rerun.wtl
    %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result te_rerun.wtl -result_name te_rerun.wtl

    FOR %%I in (WexLogFileOutput\*.jpg) DO (
        echo Uploading %%I to "%HELIX_RESULTS_CONTAINER_URI%/%%I%HELIX_RESULTS_CONTAINER_RSAS%"
        %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result %%I -result_name %%~nI_rerun%%~xI
    )
)

rem If there are still failing tests remaining, we'll run them eight more times, so they'll have been run a total of ten times.
rem If any tests fail all ten times, we can be pretty confident that these are actual test failures rather than unreliable tests.
if exist te_rerun.wtl (
    cd scripts
    set FailedTestQuery=
    for /F "tokens=* usebackq" %%A IN (`powershell -ExecutionPolicy Bypass .\OutputFailedTestQuery.ps1 ..\te_rerun.wtl`) DO (
      set FailedTestQuery=%%A
    )
    cd ..

    if "%FailedTestQuery%" neq "" (
        te %testBinaries% /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError /testmode:Loop /LoopTest:8 /select:"%FailedTestQuery%"
        
        move te.wtl te_rerun_multiple.wtl
        %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result te_rerun_multiple.wtl -result_name te_rerun_multiple.wtl

        FOR %%I in (WexLogFileOutput\*.jpg) DO (
            echo Uploading %%I to "%HELIX_RESULTS_CONTAINER_URI%/%%I%HELIX_RESULTS_CONTAINER_RSAS%"
            %HELIX_PYTHONPATH% %HELIX_SCRIPT_ROOT%\upload_result.py -result %%I -result_name %%~nI_rerun_multiple%%~xI
        )
    )
)

cd scripts
powershell -ExecutionPolicy Bypass .\ConvertWttLogToXUnit.ps1 ..\te_original.wtl ..\te_rerun.wtl ..\te_rerun_multiple.wtl ..\testResults.xml %testnameprefix%
cd ..

type testResults.xml

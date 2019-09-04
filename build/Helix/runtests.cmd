setlocal ENABLEDELAYEDEXPANSION

robocopy %HELIX_CORRELATION_PAYLOAD% . /s /NP > NUL

reg add HKLM\Software\Policies\Microsoft\Windows\Appx /v AllowAllTrustedApps /t REG_DWORD /d 1 /f

%systemroot%\sysnative\cmd.exe /c reg add "HKLM\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\MUXControlsTestApp.exe" /v DumpFolder /t REG_EXPAND_SZ /d %HELIX_DUMP_FOLDER% /f
%systemroot%\sysnative\cmd.exe /c reg add "HKLM\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\MUXControlsTestApp.exe" /v DumpType /t REG_DWORD /d 2 /f
%systemroot%\sysnative\cmd.exe /c reg add "HKLM\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\MUXControlsTestApp.exe" /v DumpCount /t REG_DWORD /d 10 /f

%systemroot%\sysnative\cmd.exe /c reg export "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" %HELIX_WORKITEM_UPLOAD_ROOT%\wer.reg /y

set

:: kill dhandler, which is a tool designed to handle unexpected windows appearing. But since our tests are 
:: expected to show UI we don't want it running.
taskkill -f -im dhandler.exe

powershell -ExecutionPolicy Bypass .\EnsureMachineState.ps1
powershell -ExecutionPolicy Bypass .\InstallTestAppDependencies.ps1

set testBinaryCandidates=MUXControls.Test.dll MUXControlsTestApp.appx IXMPTestApp.appx MUXControls.ReleaseTest.dll
set testBinaries=
for %%B in (%testBinaryCandidates%) do (
    if exist %%B (
        set "testBinaries=!testBinaries! %%B"
    )
)

te %testBinaries% /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError %*

move te.wtl te_original.wtl

copy /y te_original.wtl %HELIX_WORKITEM_UPLOAD_ROOT%
copy /y WexLogFileOutput\*.jpg %HELIX_WORKITEM_UPLOAD_ROOT%
copy /y *.pgc %HELIX_WORKITEM_UPLOAD_ROOT%

set FailedTestQuery=
for /F "tokens=* usebackq" %%I IN (`powershell -ExecutionPolicy Bypass .\OutputFailedTestQuery.ps1 te_original.wtl`) DO (
  set FailedTestQuery=%%I
)

rem The first time, we'll just re-run failed tests once.  In many cases, tests fail very rarely, such that
rem a single re-run will be sufficient to detect many unreliable tests.
if "%FailedTestQuery%" == "" goto :SkipReruns

te %testBinaries% /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError /select:"%FailedTestQuery%"

move te.wtl te_rerun.wtl

copy /y te_rerun.wtl %HELIX_WORKITEM_UPLOAD_ROOT%
copy /y WexLogFileOutput\*.jpg %HELIX_WORKITEM_UPLOAD_ROOT%

rem If there are still failing tests remaining, we'll run them eight more times, so they'll have been run a total of ten times.
rem If any tests fail all ten times, we can be pretty confident that these are actual test failures rather than unreliable tests.
if not exist te_rerun.wtl goto :SkipReruns

set FailedTestQuery=
for /F "tokens=* usebackq" %%I IN (`powershell -ExecutionPolicy Bypass .\OutputFailedTestQuery.ps1 te_rerun.wtl`) DO (
  set FailedTestQuery=%%I
)

if "%FailedTestQuery%" == "" goto :SkipReruns

te %testBinaries% /enablewttlogging /unicodeOutput:false /sessionTimeout:0:15 /testtimeout:0:10 /screenCaptureOnError /testmode:Loop /LoopTest:8 /select:"%FailedTestQuery%"

move te.wtl te_rerun_multiple.wtl

copy /y te_rerun_multiple.wtl %HELIX_WORKITEM_UPLOAD_ROOT%
copy /y WexLogFileOutput\*.jpg %HELIX_WORKITEM_UPLOAD_ROOT%

:SkipReruns

powershell -ExecutionPolicy Bypass .\OutputSubResultsJsonFiles.ps1 te_original.wtl te_rerun.wtl te_rerun_multiple.wtl %testnameprefix%
powershell -ExecutionPolicy Bypass .\ConvertWttLogToXUnit.ps1 te_original.wtl te_rerun.wtl te_rerun_multiple.wtl testResults.xml %testnameprefix%

copy /y *_subresults.json %HELIX_WORKITEM_UPLOAD_ROOT%

type testResults.xml

powershell -ExecutionPolicy Bypass Get-Process

dir /b /s c:\cores

@echo off
setlocal enableDelayedExpansion

set RepoRoot=%~dp0\..

echo Generating MUXControls.Test.proj...
call GenerateTestProjFile.cmd %RepoRoot%\BuildOutput\Debug\AnyCPU\MUXControls.Test.TAEF\MUXControls.Test.dll C:\Temp\MUXControls.Test.proj DevTestSuite

echo Generating MUXControlsTestApp.proj...
call GenerateTestProjFile.cmd %RepoRoot%\BuildOutput\Debug\x86\MUXControlsTestApp.TAEF\AppPackages\MUXControlsTestApp_Test\MUXControlsTestApp.appx C:\Temp\MUXControlsTestApp.proj DevTestSuite

echo Generating IXMPTestApp.proj...
call GenerateTestProjFile.cmd %RepoRoot%\BuildOutput\Debug\x86\IXMPTestApp.TAEF\AppPackages\IXMPTestApp_Test\IXMPTestApp.appx C:\Temp\IXMPTestApp.proj DevTestSuite

echo Generating MUXControls.ReleaseTest.proj...
call GenerateTestProjFile.cmd %RepoRoot%\BuildOutput\Debug\AnyCPU\MUXControls.ReleaseTest.TAEF\MUXControls.ReleaseTest.dll C:\Temp\MUXControls.ReleaseTest.proj NugetTestSuite
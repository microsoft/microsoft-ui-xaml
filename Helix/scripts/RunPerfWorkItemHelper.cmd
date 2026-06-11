if "%~1" == "packaged" (
  call %HELIX_CORRELATION_PAYLOAD%\RunPerfWorkItem.cmd Test\perf\legacy-tools\Microsoft.UI.Xaml.Tests.AppPerf.dll /unicodeOutput:false /p:packaged /p:AppToRun=%2_6f07fta6qpts2!App /p:SweepModules=%3 /p:SweepOutput=%2.pgc /p:SweepOutputDir=%HELIX_CORRELATION_PAYLOAD%\pgc /p:AppActiveTime=%4 /p:AppArgs=%5
) else (
  call %HELIX_CORRELATION_PAYLOAD%\RunPerfWorkItem.cmd Test\perf\legacy-tools\Microsoft.UI.Xaml.Tests.AppPerf.dll /unicodeOutput:false /p:unpackaged /p:AppToRun=%2.exe /p:SweepModules=%3 /p:SweepOutput=%2.pgc /p:SweepOutputDir=%HELIX_CORRELATION_PAYLOAD%\pgc /p:AppActiveTime=%4 /p:AppArgs=%5 /p:RunAsCommandLine=true /p:BinaryDirPath=Test\perf\apps\%2\
)
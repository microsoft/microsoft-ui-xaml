@echo off
REM This script is supposed to run in the Package ES build after tests have complete

pushd %~dp0

@echo off
if not defined XES_SERIALPOSTBUILDREADY (      
    echo Not last build.  Outputting a dummy code coverage report that will later be overwritten with the real one...

    if not exist %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageSummary_x86 ( mkdir %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageSummary_x86 )

    REM We categorically do not want to upload a code coverage report directory yet since that can't get overwritten, so let's
    REM remove the directory if it does exist to ensure we don't.
    if exist %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageReport_x86 ( rmdir %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageReport_x86 /S /Q )

    echo ^<?xml version="1.0" encoding="utf-8"?^> > %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageSummary_x86\coveragesummary.xml
    echo ^<report name="JaCoCo"^> >> %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageSummary_x86\coveragesummary.xml
    echo   ^<group name="Microsoft"^> >> %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageSummary_x86\coveragesummary.xml
    echo     ^<package name="microsoft.ui.xaml.dll"^> >> %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageSummary_x86\coveragesummary.xml
    echo     ^</package^> >> %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageSummary_x86\coveragesummary.xml
    echo   ^</group^> >> %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageSummary_x86\coveragesummary.xml
    echo ^</report^> >> %BUILD_ARTIFACTSTAGINGDIRECTORY%\CodeCoverageSummary_x86\coveragesummary.xml

    exit 0
)

call %~dp0\CreateCodeCoverageReport.cmd
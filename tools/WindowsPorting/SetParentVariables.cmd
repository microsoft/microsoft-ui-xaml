@echo off
rem
rem USAGE NOTE:
rem
rem     This script allows someone to kick off the build definitions DEPControls_OS_Port and DEPControls_build_OS
rem     without needing for it to be a child build of DEPControls_master.
rem     DEPControls_OS_Port and DEPControls_build_OS have variables defined that are the same name
rem     as the variables passed into it by DEPControls_master, so if you set those variables
rem     when queuing a build from DEPControls_OS_Port, those variables will be used instead.
rem

:LoopStart
set ARG1=%1
set ARG2=%2

if "%ARG1:~0,1%" == "-" (
    if not "%ARG2:~0,1%" == "-" (
        if "%ARG1%" == "-sourceBranch" (
            echo Source branch set to %ARG2%
            set PARENT_SOURCEBRANCH=%ARG2%
        )

        if "%ARG1%" == "-commitId" (
            echo Commit ID to sync to set to %ARG2%
            set PARENT_COMMITTOSYNCTO=%ARG2%
        )

        shift
        shift
    ) else (
        shift
    )
) else (
    goto :End
)

goto :LoopStart

:End
exit /b %ERRORLEVEL%
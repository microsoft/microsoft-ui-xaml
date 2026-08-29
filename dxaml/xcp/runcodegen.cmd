@echo off

REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License. See LICENSE in the project root for license information.

if "%RepoRoot%" == "" call %~dp0\..\..\init.cmd

SETLOCAL

REM Use "/noabi" command line argument to not prefix with ABI.
SET PREFIXABI=%1
SET DEV_RUNNING_CODE_GEN=true
SET RUNCODEGEN_PATH=%RepoRoot%\dxaml\xcp\tools\XCPTypesAutoGen\RunCodeGen
SET XBFINDEXES_PATH=%RepoRoot%\dxaml\xcp\tools\XCPTypesAutoGen\XamlGen
SET UPDATE_FILES_CMD_PATH=%BuildOutputRoot%\%_BuildArch%%_BuildType%\dxaml\Codegen\updatecheckedinfiles.cmd

@echo.
@echo.
@echo ****************************************************************************
@echo *                                                                          *
@echo *  This script automates running code-gen.                                 *
@echo *                                                                          *
@echo ****************************************************************************
@echo.

@echo *** Reverting local updates to XBF Stable Indexes.
@echo.
git checkout HEAD %XBFINDEXES_PATH%\*.g.csv

pushd %RUNCODEGEN_PATH%
REM We do a rebuild here just in case there's something wrong with incremental codegen builds.
REM The user has manually requested a CodeGen update, let's make sure it's a clean one.
call msbuild /bl /t:rebuild
popd

@echo.
@echo.

if exist "%UPDATE_FILES_CMD_PATH%" goto :updatefiles
if errorlevel 1 goto :error

@echo ****************************************************************************
@echo *                                                                          *
@echo * Code-gen succeeded, but no updates were necessary. Is that expected?     *
@echo *                                                                          *
@echo ****************************************************************************
@echo.
goto :eof

:updatefiles
@echo ****************************************************************************
@echo *                                                                          *
@echo * Code-gen succeeded, updating generated files...                          *
@echo *                                                                          *
@echo ****************************************************************************
@echo.
call %UPDATE_FILES_CMD_PATH%
@echo.
goto :eof

:error
@echo *** Code-gen FAILED ***
@echo.
@echo Build details can be found at:
@echo %RUNCODEGEN_PATH%\msbuild.binlog
@echo.
goto :eof

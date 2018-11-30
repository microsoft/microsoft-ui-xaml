setlocal

@if not defined XES_NUGETPACKVERSIONNOBETA (
	echo ERROR: Expecting XES_NUGETPACKVERSIONNOBETA to be set
	exit /B 1
)

call %~dp0\..\build\localization\RunLocWorkflow.cmd Daily-%XES_NUGETPACKVERSIONNOBETA%

REM Undo any changes that were made since we are just doing handoff and the process will automatically hand back.
pushd %~dp0\..
git reset -- *
git checkout -- *
git reset --hard
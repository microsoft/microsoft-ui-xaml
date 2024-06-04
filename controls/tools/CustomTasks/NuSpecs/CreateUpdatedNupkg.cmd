@echo off

pushd %~dp0

echo Deleting existing MUXCustomBuildTasks*.nupkg files...
del MUXCustomBuildTasks*.nupkg

call IncrementVersionNumber.cmd

msbuild /m %RepoRoot%\controls\CustomTasks.sln /restore /p:Configuration=Release /p:Platform="Any CPU" /t:Rebuild

call BuildNupkg.cmd
call PublishNupkg.cmd
call UpdateReferences.cmd

popd
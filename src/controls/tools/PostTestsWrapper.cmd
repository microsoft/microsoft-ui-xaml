REM This script is supposed to run in the Package ES build after tests have complete

pushd %~dp0

call PostBuildOnly.cmd %~dp0\PublishNupkg.cmd
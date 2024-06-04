@echo OFF
pushd %~dp0\..

doskey ..=pushd ..
doskey ...=pushd ..\..
doskey ....=pushd ..\..\..
doskey .....=pushd ..\..\..\..
doskey ......=pushd ..\..\..\..\..
doskey src=pushd %CD%

popd

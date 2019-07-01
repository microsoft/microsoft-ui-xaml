@echo off

pushd x86
pgomgr.exe /merge *.pgc microsoft.ui.xaml.pgd 
popd

pushd x64
pgomgr.exe /merge *.pgc microsoft.ui.xaml.pgd 
popd


pushd arm
pgomgr.exe /merge *.pgc microsoft.ui.xaml.pgd 
popd


pushd arm64
pgomgr.exe /merge *.pgc microsoft.ui.xaml.pgd 
popd

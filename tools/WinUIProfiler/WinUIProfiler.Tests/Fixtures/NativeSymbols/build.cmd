@echo off
rem Copyright (c) Microsoft Corporation. All rights reserved.
rem Licensed under the MIT License. See LICENSE in the project root for license information.

rem Builds NativeSymbols.dll and NativeSymbols.pdb from NativeSymbols.c, for x64, from a Visual Studio x64 developer
rem command prompt. The PDB has the paths of the directory it was built in, so the build runs in a new directory at the
rem root of the system drive, not here, and neither file names the enlistment or the user, as the tests check. A new
rem directory also has no old PDB for the linker to update, with a new age. /Brepro makes the build reproducible;
rem /PDBALTPATH keeps the build directory out of the DLL.
setlocal
where cl.exe >nul 2>nul || (echo Run build.cmd from a Visual Studio x64 developer command prompt. & exit /b 1)
set "build=%SystemDrive%\NativeSymbolsBuild"
if exist "%build%" (echo %build% exists, left by an earlier build: delete it and build again. & exit /b 1)
md "%build%" || exit /b 1
copy "%~dp0NativeSymbols.c" "%build%" >nul || goto fail
cd /d "%build%"
cl /nologo /c /Z7 /O1 /GS- /Brepro NativeSymbols.c || goto fail
link /nologo /DLL /NOENTRY /NODEFAULTLIB /DEBUG:FULL /INCREMENTAL:NO /OPT:REF /OPT:NOICF /Brepro /PDBALTPATH:%%_PDB%% ^
    /OUT:NativeSymbols.dll /PDB:NativeSymbols.pdb NativeSymbols.obj || goto fail
copy /y NativeSymbols.dll "%~dp0" >nul || goto fail
copy /y NativeSymbols.pdb "%~dp0" >nul || goto fail
cd /d "%~dp0"
rd /s /q "%build%"
exit /b 0

:fail
cd /d "%~dp0"
rd /s /q "%build%"
exit /b 1

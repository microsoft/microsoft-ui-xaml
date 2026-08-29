@echo Off

REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License. See LICENSE in the project root for license information.

if "%_NTROOT%" == "" goto collectPerf
@echo Cannot collect perf from razzle window, copy this into a cmd window
@echo cd /d "%CD%"
@echo collectperfnumbers.cmd
goto :EOF

:collectPerf

for /F "tokens=1,2,3* delims=/ " %%i in ("%date%") do set nicedate=%%k/%%j/%%l
for /F "tokens=1,2,3* delims=:. " %%i in ("%time%") do set nicetime=%%i:%%j:%%k

for /F "tokens=1,2,3* delims=/ " %%i in ("%date%") do set nicedateFN=%%l-%%j-%%k
for /F "tokens=1,2,3* delims=:. " %%i in ("%time%") do set nicetimeFN=%%i%%j%%k
set niceFN=%nicedateFN%-%nicetimeFN%

set PROJROOT=%APPDATA%\PerfProjects
set MSBUILDDIR=%WINDIR%\Microsoft.NET\Framework\v4.0.30319
set LOGDIR=\\authoring\team\user\felixa\XAML Tools\PERFPROJECTS-RESULTS
mkdir %LOGDIR%

@echo Building all projects in %PROJROOT%
for /F %%i in ('dir "%PROJROOT%" /ad /b /o') DO @call :buildProj %%i

@echo Touch and rebuild projects
for /F %%i in ('dir "%PROJROOT%" /ad /b /o') DO @call :rebuildProj %%i

goto :EOF

:buildProj
@echo Building %1
for /F %%i in ('dir "%PROJROOT%\%1\*.sln" /b') DO set SLNFILE=%%i
"%MSBUILDDIR%\msbuild" /t:rebuild /v:diag "%PROJROOT%\%1\%SLNFILE%" > "%PROJROOT%\%1\build.log"

REM @echo Finding timing information
findstr /ip /c:" ms " "%PROJROOT%\%1\build.log" > "%PROJROOT%\%1\ms.log"

REM @echo Finding XAML compiler interesting info

del /q "%PROJROOT%\%1\ms.csv"
for /F %%i in (info.txt) do @call :findLine "%PROJROOT%\%1" %%i

rem make one collective csv
copy columns.csv + "%PROJROOT%\%1\*-ms.csv.tmp" "%PROJROOT%\%1\perf.csv"
mkdir "%LOGDIR%\%USERNAME%"
copy "%PROJROOT%\%1\perf.csv" "%LOGDIR%\%USERNAME%\%1-perf.csv"
goto :EOF

REM [path to project]
:rebuildProj
@echo Building %1
for /F %%i in ('dir "%PROJROOT%\%1\*.sln" /b') DO set SLNFILE=%%i
for /F %%i in ('dir /s "%PROJROOT%\%1\app.xaml" /b') DO set APPXAML==%%i
echo. >> %%i

"%MSBUILDDIR%\msbuild" /v:diag "%PROJROOT%\%1\%SLNFILE%" > "%PROJROOT%\%1\build.log"

REM @echo Finding timing information
findstr /ip /c:" ms " "%PROJROOT%\%1\build.log" > "%PROJROOT%\%1\ms-incr.log"

REM @echo Finding XAML compiler interesting info

del /q "%PROJROOT%\%1\ms-incr.csv"
for /F %%i in (info.txt) do @call :findLineIncr "%PROJROOT%\%1" %%i

rem make one collective csv
copy columns.csv + "%PROJROOT%\%1\*ms-incr.csv.tmp" "%PROJROOT%\%1\perf-incr.csv"
mkdir "%LOGDIR%\%USERNAME%"
copy "%PROJROOT%\%1\perf-incr.csv" "%LOGDIR%\%USERNAME%\%1-perf-incr.csv"
goto :EOF



REM parses the ms.log finding all matching lines [folder to search] [line to find]
:findLine
for /F "tokens=1,2,3,4 delims= " %%i in ('findstr /p /c:"%2" "%1\ms.log"') do call :matchLine %%k %%i %2 %1
goto :EOF

REM [thing to matche] [time] [partial match] [folder searched]
:matchLine
if "%1" == "%3" echo %nicedate% %nicetime%,%1,%2 >> "%4\%niceFN%-ms.csv.tmp"




REM parses the ms.log finding all matching lines [folder to search] [line to find]
:findLineIncr
for /F "tokens=1,2,3,4 delims= " %%i in ('findstr /p /c:"%2" "%1\ms-incr.log"') do call :matchLineIncr %%k %%i %2 %1
goto :EOF

REM [thing to matche] [time] [partial match] [folder searched]
:matchLineIncr
if "%1" == "%3" echo %nicedate% %nicetime%,%1,%2 >> "%4\%niceFN%-ms-incr.csv.tmp"

@echo off
REM Run a build while tracking the total commit of the system.
REM Works best to pipe the output of this script to a file, and then look at that file
REM to see how memory use changes during the build.
REM Like:
REM     buildWithSystemMemoryMonitor > run1.txt
REM
REM The systemmemory.ps1 script will run until we write the below %temp%\systemmemorystop file.

start /b powershell %reporoot%\scripts\systemmemory.ps1 -runForever

call %reporoot%\build /c /verbose

echo done > %temp%\systemmemorystop


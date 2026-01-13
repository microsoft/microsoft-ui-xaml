rem Kill existing msbuild processes to minimize the probability
rem that a reboot will be required for the LongPathsEnabled regkey
rem value to be honored.
taskkill /im msbuild.exe /f 2> nul

rem Set the regkey. This will fail if this script is not run elevated.
reg add "HKLM\SYSTEM\CurrentControlSet\Control\FileSystem" /v LongPathsEnabled /t REG_DWORD /d 1 /f
@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM ============================================================================
REM  runtests.cmd  -  XamlCompiler unit-test runner
REM  Path: C:\repo1\microsoft-ui-xaml-lift\src\XamlCompiler\runtests.cmd
REM
REM  What this script does, top to bottom:
REM    1. Parses optional flags  (/config, /platform, /flavor, /rid, /vstest)
REM       and falls back to sensible defaults for anything you do not pass.
REM    2. Locates vstest.console.exe; if missing, tries init.cmd; if still
REM       missing, asks you to enter a path.
REM    3. Builds the three LibManaged*.dll/winmd files standalone if they
REM       are missing from BuildOutput (applies a temp csproj patch, then
REM       reverts it - same recipe as test_chnages.txt).
REM    4. Copies every required dependency into UnitTestingBin\. Prints a
REM       [WARN] for anything it cannot find but keeps going.
REM    5. Runs vstest.console.exe on UnitTests.dll.
REM
REM  Usage examples:
REM    runtests.cmd
REM    runtests.cmd /config:amd64fre
REM    runtests.cmd /platform:x64 /flavor:chk
REM    runtests.cmd /rid:"win;win-x64"
REM    runtests.cmd /vstest:"C:\Program Files\...\vstest.console.exe"
REM    runtests.cmd /TestCaseFilter:"FullyQualifiedName~Basic01"
REM    runtests.cmd /?
REM ============================================================================


REM --- Default values -------------------------------------------------------
REM REPO is the absolute path to the repo root: 2 levels above this script.
set "REPO=%~dp0..\.."
for %%I in ("%REPO%") do set "REPO=%%~fI"

set "CONFIG="
set "PLATFORM=x64"
set "FLAVOR=chk"
set "RID=win;win-x64;win-x86;win-arm64;win10-x86;win10-x64;win10-arm64"
set "VSTEST="
set "EXTRA="

REM Environment-variable defaults (CLI flags below will still override)
if defined XAML_TESTS_CONFIG set "CONFIG=%XAML_TESTS_CONFIG%"
if defined VSTEST_CONSOLE    set "VSTEST=%VSTEST_CONSOLE%"


REM --- Parse CLI flags ------------------------------------------------------
:parse
if "%~1"=="" goto :parse_done
if /I "%~1"=="/?"    goto :show_help
if /I "%~1"=="/help" goto :show_help
if /I "%~1"=="-h"    goto :show_help

set "ARG=%~1"
if /I "!ARG:~0,8!"=="/config:"    (set "CONFIG=!ARG:~8!"    & shift & goto :parse)
if /I "!ARG:~0,10!"=="/platform:" (set "PLATFORM=!ARG:~10!" & shift & goto :parse)
if /I "!ARG:~0,8!"=="/flavor:"    (set "FLAVOR=!ARG:~8!"    & shift & goto :parse)
if /I "!ARG:~0,5!"=="/rid:"       (set "RID=!ARG:~5!"       & shift & goto :parse)
if /I "!ARG:~0,8!"=="/vstest:"    (set "VSTEST=!ARG:~8!"    & shift & goto :parse)

REM Anything else is forwarded to vstest at the end of the script
set "EXTRA=!EXTRA! %1"
shift
goto :parse
:parse_done

REM Map MSBuild PLATFORM name (x64/Win32/ARM64) to BuildOutput folder
REM prefix (amd64/x86/arm64). The repo uses different names in different
REM places: MSBuild wants Platform=x64, but BuildOutput\obj\amd64chk\... .
set "ARCH=%PLATFORM%"
if /I "%PLATFORM%"=="x64"   set "ARCH=amd64"
if /I "%PLATFORM%"=="Win32" set "ARCH=x86"
if /I "%PLATFORM%"=="ARM64" set "ARCH=arm64"

REM If /config was not given explicitly, build it from <ARCH><FLAVOR>
if "%CONFIG%"=="" set "CONFIG=%ARCH%%FLAVOR%"


REM --- Derived paths --------------------------------------------------------
set "OBJ=%REPO%\BuildOutput\obj\%CONFIG%\src\XamlCompiler\Tests\UnitTests"
set "PROD=%REPO%\BuildOutput\bin\%CONFIG%\Product"
set "EXE=%REPO%\BuildOutput\obj\%CONFIG%\src\XamlCompiler\Exe\Microsoft.UI.Xaml.Markup.Compiler.Executable"
set "DST=%REPO%\src\XamlCompiler\Tests\UnitTests\UnitTestingBin"
set "MSB=%REPO%\.buildtools\MSBuild\Current\Bin\amd64\MSBuild.exe"

echo [INFO] Repo:     %REPO%
echo [INFO] Config:   %CONFIG%   (Platform=%PLATFORM%, Flavor=%FLAVOR%)
echo [INFO] Staging:  %DST%
echo.

REM Safety net: if a previous run was Ctrl-C'd mid-build and left a patched
REM csproj behind, revert it BEFORE doing anything else. Otherwise that
REM stale patch would break the dependency check below (and any later
REM build of XamlCompilerUnitTests.csproj would fail with NU1201).
call :Revert LibManagedDllSatellite
call :Revert LibManagedDll
call :Revert LibManagedWinmd


REM --- STEP 1: Locate vstest.console.exe ------------------------------------
call :FindVsTest
if "%VSTEST%"=="" (
    echo [ERROR] No vstest.console.exe available. Cannot run tests.
    echo         Provide one with /vstest:^"^<path^>^" or set VSTEST_CONSOLE.
    exit /b 2
)


REM --- STEP 2: Ensure LibManaged*.dll/winmd exist (build them if missing) ---
call :EnsureLibManaged


REM --- STEP 3: Stage every required dependency into UnitTestingBin\ ---------
if not exist "%DST%" mkdir "%DST%"

REM Bulk-copy the test project output (UnitTests.dll, proxies, Microsoft.UI.*, GenXbf, TestMasters\)
call :StageDir "%OBJ%\XamlCompilerUnitTests" "%DST%"

REM Copy the three LibManaged* outputs (rename .winmdobj -> .dll for the winmd project)
call :StageOne "%OBJ%\LibManagedDll\LibManagedDll\LibManagedDll.dll"                            "%DST%\LibManagedDll.dll"
call :StageOne "%OBJ%\LibManagedDllSatellite\LibManagedDllSatellite\LibManagedDllSatellite.dll" "%DST%\LibManagedDllSatellite.dll"
call :StageOne "%OBJ%\LibManagedWinmd\LibManagedWinmd\LibManagedWinmd.winmdobj"                 "%DST%\LibManagedWinmd.dll"
call :StageOne "%OBJ%\LibManagedWinmd\LibManagedWinmd\LibManagedWinmd.winmd"                    "%DST%\LibManagedWinmd.winmd"

REM Copy the XamlCompiler.exe folder (XamlCompiler.exe + System.* DLLs)
call :StageDir "%EXE%" "%DST%"

REM Copy the runsettings source file
call :StageOne "%REPO%\src\XamlCompiler\Tests\UnitTests\test.runsettings" "%DST%\test.runsettings"


REM --- STEP 4: Run the tests ------------------------------------------------
if not exist "%DST%\UnitTests.dll" (
    echo [ERROR] UnitTests.dll not found in staging folder. Cannot run tests.
    echo         Build src\XamlCompiler\Tests\UnitTests\XamlCompilerUnitTests.csproj first.
    exit /b 3
)
echo.
echo [INFO] Running vstest.console.exe...
echo.
pushd "%DST%"
"%VSTEST%" UnitTests.dll /Settings:test.runsettings%EXTRA%
set "RC=%ERRORLEVEL%"
popd
exit /b %RC%


REM ============================================================================
REM  SUBROUTINES
REM ============================================================================

REM ---- :StageOne <source-file> <dest-file> --------------------------------
REM Copies source to dest, but only if dest is missing. Warns if source
REM does not exist either.
:StageOne
if exist "%~2" goto :eof
if exist "%~1" (
    copy /Y "%~1" "%~2" >nul
    if errorlevel 1 echo [WARN] Could not copy "%~1"
    goto :eof
)
echo [WARN] Missing dependency: %~nx2
echo                expected at: %~1
goto :eof


REM ---- :StageDir <source-dir> <dest-dir> ----------------------------------
REM Copies every file from source-dir into dest-dir, recursively. Warns
REM if source-dir does not exist (but still allows the script to continue).
:StageDir
if not exist "%~1" (
    echo [WARN] Source folder missing: %~1
    goto :eof
)
xcopy /Y /Q /E /I "%~1\*" "%~2\" >nul
if errorlevel 1 echo [WARN] xcopy reported errors copying from "%~1"
goto :eof


REM ---- :FindVsTest --------------------------------------------------------
REM Tries to locate vstest.console.exe in this priority order:
REM   1) the /vstest:<path> already set above (or VSTEST_CONSOLE env var)
REM   2) the in-repo .buildtools copy (installed by init.cmd)
REM   3) vswhere -latest  (any installed VS)
REM   4) well-known VS 2022 install paths
REM   5) attempt init.cmd to install .buildtools, then re-check (2)
REM   6) prompt the user to type a path
:FindVsTest
if not "%VSTEST%"=="" (
    if exist "%VSTEST%" goto :have_vstest
    echo [WARN] /vstest path does not exist: %VSTEST%
    set "VSTEST="
)

set "CAND=%REPO%\.buildtools\Common7\IDE\Extensions\TestPlatform\vstest.console.exe"
if exist "!CAND!" (set "VSTEST=!CAND!" & goto :have_vstest)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "!VSWHERE!" (
    for /f "usebackq tokens=*" %%I in (`""!VSWHERE!" -latest -property installationPath"`) do (
        set "TRY=%%I\Common7\IDE\Extensions\TestPlatform\vstest.console.exe"
        if exist "!TRY!" (set "VSTEST=!TRY!" & goto :have_vstest)
    )
)

REM Well-known VS 2022 install paths
set "PF=%ProgramFiles%"
for %%E in (Enterprise Professional Community BuildTools) do (
    set "TRY=!PF!\Microsoft Visual Studio\2022\%%E\Common7\IDE\Extensions\TestPlatform\vstest.console.exe"
    if exist "!TRY!" (set "VSTEST=!TRY!" & goto :have_vstest)
)

REM Not found anywhere. Try installing .buildtools via init.cmd.
echo [WARN] vstest.console.exe not found.
echo [INFO] Attempting to install .buildtools via init.cmd (this can take a while)...
if exist "%REPO%\init.cmd" (
    pushd "%REPO%"
    call init.cmd %CONFIG%
    popd
    if exist "!CAND!" (set "VSTEST=!CAND!" & goto :have_vstest)
) else (
    echo [WARN] %REPO%\init.cmd not found - cannot auto-install.
)

REM Last resort: ask the user
echo.
echo [WARN] vstest.console.exe is still not available.
set /p "VSTEST=Enter full path to vstest.console.exe (or press Enter to abort): "
if "%VSTEST%"=="" goto :eof
if not exist "!VSTEST!" (
    echo [ERROR] File not found: !VSTEST!
    set "VSTEST="
)

:have_vstest
if not "%VSTEST%"=="" echo [INFO] vstest:   %VSTEST%
goto :eof


REM ---- :EnsureLibManaged --------------------------------------------------
REM Checks if all four LibManaged outputs exist under BuildOutput\obj. If
REM any are missing, builds them via :PatchAndBuild (which also reverts
REM the csproj back to its original UAP form afterwards).
:EnsureLibManaged
set "MISSING=0"
if not exist "%OBJ%\LibManagedDll\LibManagedDll\LibManagedDll.dll"                            set "MISSING=1"
if not exist "%OBJ%\LibManagedDllSatellite\LibManagedDllSatellite\LibManagedDllSatellite.dll" set "MISSING=1"
if not exist "%OBJ%\LibManagedWinmd\LibManagedWinmd\LibManagedWinmd.winmd"                    set "MISSING=1"
if not exist "%OBJ%\LibManagedWinmd\LibManagedWinmd\LibManagedWinmd.winmdobj"                 set "MISSING=1"

if "%MISSING%"=="0" (
    echo [INFO] LibManaged*.dll/winmd already built. Skipping rebuild.
    goto :eof
)

echo [WARN] One or more LibManaged*.dll/winmd files are missing in BuildOutput.
echo [INFO] Building them standalone (patch csproj -^> build -^> revert)...

if not exist "!MSB!" (
    echo [ERROR] MSBuild not found at: !MSB!
    echo         Run init.cmd first to install .buildtools.
    goto :eof
)

REM Build order matters: Satellite first; LibManagedDll depends on it.
REM We PATCH all three first, then BUILD all three, then REVERT all three.
REM (Reverting a project between builds would cause NuGet to re-evaluate the
REM  un-patched ProjectReference during the next build -> ForwardedType lost.)
call :Patch  LibManagedDllSatellite
call :Patch  LibManagedDll
call :Patch  LibManagedWinmd

call :Build  LibManagedDllSatellite
call :Build  LibManagedDll
call :Build  LibManagedWinmd

call :Revert LibManagedDllSatellite
call :Revert LibManagedDll
call :Revert LibManagedWinmd
goto :eof


REM ---- :Patch <projectName> ----------------------------------------------
REM Backs up <name>.csproj to <name>.csproj.runtests-bak and inserts the
REM 3-line recipe (TargetPlatformVersion + TargetPlatformMinVersion +
REM <Import .../Microsoft.Windows.UI.Xaml.CSharp.targets />).
:Patch
set "CSPROJ=%REPO%\src\XamlCompiler\Tests\UnitTests\%~1\%~1.csproj"
set "BAK=%CSPROJ%.runtests-bak"
echo [INFO]   - patch %~1
copy /Y "%CSPROJ%" "%BAK%" >nul
powershell -NoProfile -Command ^
    "$f='%CSPROJ%';" ^
    "$l=Get-Content $f;" ^
    "$o=@(); foreach($x in $l){ $o+=$x; if($x -match '<TargetPlatformIdentifier>UAP</TargetPlatformIdentifier>'){ $o+='    <TargetPlatformVersion>10.0.22621.0</TargetPlatformVersion>'; $o+='    <TargetPlatformMinVersion>10.0.17763.0</TargetPlatformMinVersion>' } };" ^
    "for($j=$o.Count-1;$j-ge 0;$j--){ if($o[$j] -match '^\s*</Project>\s*$'){ $imp='  <Import Project=\"$(MSBuildExtensionsPath)\Microsoft\WindowsXaml\v$(VisualStudioVersion)\Microsoft.Windows.UI.Xaml.CSharp.targets\" />'; $o=$o[0..($j-1)] + $imp + $o[$j..($o.Count-1)]; break } };" ^
    "Set-Content -Path $f -Value $o -Encoding UTF8"
goto :eof


REM ---- :Build <projectName> ----------------------------------------------
REM Writes a tiny response file and invokes msbuild on the csproj.
:Build
set "CSPROJ=%REPO%\src\XamlCompiler\Tests\UnitTests\%~1\%~1.csproj"
set "RSP=%TEMP%\runtests_%~1.rsp"
echo [INFO]   - build %~1
> "%RSP%" echo %CSPROJ%
>>"%RSP%" echo /restore
>>"%RSP%" echo /t:Build
>>"%RSP%" echo /nologo
>>"%RSP%" echo /v:m
>>"%RSP%" echo /m:2
>>"%RSP%" echo /p:Configuration=Debug
>>"%RSP%" echo /p:Platform=%PLATFORM%
>>"%RSP%" echo /p:VisualStudioVersion=17.0
>>"%RSP%" echo /p:PublicMUXDir=%PROD%\
>>"%RSP%" echo /p:ProjectRoot=%REPO%\
>>"%RSP%" echo /p:RuntimeIdentifiers="%RID%"
>>"%RSP%" echo /p:DisableWarnForInvalidRestoreProjects=true
pushd "%REPO%"
"%MSB%" @"%RSP%"
set "BUILD_RC=!ERRORLEVEL!"
popd
del "%RSP%"
if not "%BUILD_RC%"=="0" echo [WARN]     Build of %~1 failed (exit code %BUILD_RC%)
goto :eof


REM ---- :Revert <projectName> ---------------------------------------------
REM Copies the .runtests-bak back over the csproj and deletes the backup.
REM This is MANDATORY: leaving a patched csproj on disk will break a later
REM `msbuild XamlCompilerUnitTests.csproj /restore` with NU1201.
:Revert
set "CSPROJ=%REPO%\src\XamlCompiler\Tests\UnitTests\%~1\%~1.csproj"
set "BAK=%CSPROJ%.runtests-bak"
if exist "!BAK!" (
    copy /Y "!BAK!" "!CSPROJ!" >nul
    del "!BAK!"
)
goto :eof


REM ---- :show_help ---------------------------------------------------------
:show_help
echo.
echo runtests.cmd  -  XamlCompiler unit-test runner
echo.
echo Usage:  runtests.cmd [options]  [...vstest args]
echo.
echo Options:
echo   /config:^<name^>      Build flavor folder under BuildOutput\obj
echo                         (default: amd64chk)
echo   /platform:^<plat^>    Platform part only (default: x64)
echo                         Combined with /flavor when /config not given
echo   /flavor:^<chk^|fre^>   Configuration part only (default: chk)
echo   /rid:^<list^>         Semicolon-separated RuntimeIdentifiers used when
echo                         building LibManaged* projects
echo                         (default: win;win-x64;win-x86;win-arm64;
echo                                   win10-x86;win10-x64;win10-arm64)
echo   /vstest:^<path^>      Path to vstest.console.exe (default: auto-detect)
echo.  /? or /help           Show this help
echo   ^<other^>             Any other arg is forwarded to vstest.console.exe
echo                         (e.g. /TestCaseFilter:..., /Logger:trx, /Blame)
echo.
echo Environment-variable overrides:
echo   XAML_TESTS_CONFIG     same effect as /config:
echo   VSTEST_CONSOLE        same effect as /vstest:
echo.
exit /b 0

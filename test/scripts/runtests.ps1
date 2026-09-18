# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

param (
  [ValidateSet("Auto", "WPF", "UAP", "Win32Explicit", "None", "")][string]$HostingMode = "Auto",
  [Parameter(Position=0)][string]$TestQuery,
  [Parameter(Position=1, Mandatory = $false, ValueFromRemainingArguments = $true)][string[]]$ExtraArgs, # Glom the rest of the args
  [switch]$WaitForDebugger,
  [switch]$WpfMode,
  [switch]$Win32Explicit,  
  [switch]$TestsWithMasterFilesOnly,
  [switch]$RunIgnoredTests,
  [switch]$RunTestsInALoop,
  [switch]$TerminateOnFirstFailure,
  [switch]$Stat,
  [switch]$forceHostingMode,
  [string]$fromFile,
  [switch]$SkipPackageUninstall
)

if (!$TestQuery)
{
    Write-Host "
    Runtests script : Tool for simplifying running WinUI tests in TAEF.

    runtests <TestQuery> [Optional arguments]

    Optional Arguments : 
      -WaitForDebugger: for attaching debugger
      -HostingMode:<mode> : filter tests by their declared mode: WPF, UAP, Win32Explicit, or None.
                            Default 'Auto' runs all matching tests, each in its declared hosting mode.
                            'None' selects tests without hosting metadata, such as unit tests and app tests.
      -TestsWithMasterFilesOnly: run tests with Master files only. 
      -RunIgnoredTests: run tests even if they have been marked as Ignore='True'
      -RunTestsInALoop: run tests 9 times in a loop.
      -TerminateOnFirstFailure: run tests until the first failure is encountered.
      -Stat: display statistics about the test suite from your query rather than run them.
      -FromFile:<file> : select test names from the given file, rather than the main testQuery param (which is ignored)
      -SkipPackageUninstall: skip uninstalling previous versions of sample apps. This is useful if using dll redirection since uninstalling the app deletes the entire app folder.

    This script passes through unrecognized arguments to TAEF.  See `"te.exe /!`" for TAEF parameters.
    TAEF passes through -p arguments to the Xaml tests and infrastructure, as `"-p:ParamName=Value`".

    Useful (optional) pass-through arguments:
      -p:GoSlow tells test code (WaitForIdle) to inject a long pause.
      -list tells TAEF to list the tests rather than execute them.
      -p:WaitForAppDebugger tells MUXC tests and sample app tests to wait for a debugger to attach after starting.

    Examples : 

        Run matching tests in their declared hosting modes:

            runtests `"testname`"

        Run only matching tests that declare WPF hosting:

            runtests `"testname`" -HostingMode:WPF

        See statistics about the entire test suite:

            runtests * -Stat

        Run tests listed in the file RadioButtonFailures.txt:

            runtests * -fromFile:RadioButtonFailures.txt
    "
    return
}

if ($forceHostingMode -or ($ExtraArgs | Where-Object { $_ -match '^[-/]p:HostingMode(?:=|$)' }))
{
    throw "Tests declare their own hosting mode. Remove -forceHostingMode or /p:HostingMode; use -HostingMode only to filter tests."
}

$testDllList = "Test\Microsoft.UI.Xaml.Tests.*.dll Test\MUXControls.Test.dll Test\UnpackagedApps\MUXControlsTestApp\MUXControlsTestApp.dll Test\IXMPTestApp.appx" 

function get-tests {
    param (
        $query,
        $argsEx
    )
    $testlist = @()    
    #Write-Host .\te.exe "/list" $testDllList.Split(" ") "/select:`"$query`""  $argsEx
    $result = (.\te.exe "/list" $testDllList.Split(" ") "/select:`"$query`""  $argsEx)

    $result |% {
        if ($_.StartsWith(" "*16)) {
            $testname = $($_.SubString(16))
            $testlist += $testname
        }
    }

    # Need to use Write-Output -NoEnumerate to return as an array if we just have one element.
    Write-Output -NoEnumerate $testlist
}

$TestDir = $PSScriptRoot

Push-Location $TestDir
Set-Location $TestDir

if (!$SkipPackageUninstall)
{
    # Note - Make sure that this list is kept in sync with controls\test\testinfra\MUXTestInfra\TestAppInstallHelper.cs
    Write-Host "Attempting to uninstall previous versions of sample apps";
    Get-AppxPackage "WinUICppDesktopSampleApp" | Remove-AppxPackage -ErrorAction SilentlyContinue;
    Get-AppxPackage "WinUICsDesktopSampleApp" | Remove-AppxPackage -ErrorAction SilentlyContinue;
    Get-AppxPackage "Microsoft.WinUI3ControlsGallery.Debug" | Remove-AppxPackage -ErrorAction SilentlyContinue;
    Get-AppxPackage "Microsoft.WinUI3ControlsGallery" | Remove-AppxPackage -ErrorAction SilentlyContinue;
    Get-AppxPackage "DisableXamlGeneratedMainCs" | Remove-AppxPackage -ErrorAction SilentlyContinue;
    Get-AppxPackage "DisableXamlGeneratedMainNoCtorCs" | Remove-AppxPackage -ErrorAction SilentlyContinue;
    Get-AppxPackage "DisableXamlGeneratedMainCpp" | Remove-AppxPackage -ErrorAction SilentlyContinue;
    Get-AppxPackage "DisableXamlGeneratedMainNoCtorCpp" | Remove-AppxPackage -ErrorAction SilentlyContinue;
}

[string[]]$argsEx = $null

if ($fromFile)
{
    $testNames = @(Get-Content -LiteralPath $fromFile -ErrorAction Stop |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        ForEach-Object { ($_.Trim() -split '\s+')[0] })
    if ($testNames.Count -eq 0)
    {
        throw "The test list '$fromFile' contains no test names."
    }
    $queryArgs = "(@Name=''"
    $testNames |% {
        # We take just the first word from each line to make it easy for folks to copy straight from the TAEF
        # output, so that lines could look like this:
        #   Microsoft::UI::Xaml::Tests::MyTest [Failed]
        $test = $_
        $queryArgs += " OR @Name='*$test'"
    }
    $queryArgs += ")"
}
else
{
    $queryArgs = "@Name='*$TestQuery'"
}

$queryArgs += " and (@TestPass:IncludeOnlyOn='*Desktop*' OR (NOT(@TestPass:IncludeOnlyOn=*) AND NOT(@TestPass:ExcludeOn='*Desktop*')))"

if ($WaitForDebugger)
{
    $argsEx += "/p:WaitForDebugger"
}

if ($WpfMode -and $Win32Explicit)
{
    throw "It's illegal to specify both -WpfMode and -Win32Explicit, you must run one or the other."
}

if ($WpfMode) {
    Write-Host "Setting HostingMode to WPF.  You can also use -HostingMode:WPF for this." -ForegroundColor Yellow
    $HostingMode = "WPF"
}

if ($Win32Explicit) {
    Write-Host "Setting HostingMode to Win32Explicit.  You can also use -HostingMode:Win32Explicit for this." -ForegroundColor Yellow
    $HostingMode = "Win32Explicit"
}

if($RunIgnoredTests)
{
    $argsEx += "/runignoredtests"
}

if ($TestsWithMasterFilesOnly)
{
    $queryArgs += " and @HasAssociatedMasterFile='True'"
}

if ($Stat.IsPresent) {
    Write-Host "Statistics:"

    $test_types = @("WPF", "UAP", "Win32Explicit")
    $total = 0

    $test_types |% {        
        $tests = (get-tests("(@Hosting:Mode='$_')"))
        echo "Tests with hosting mode $_ : $($tests.Length)"
        $total += $tests.Length
    }
    $tests_no_hostingmode = (get-tests("not(@Hosting:Mode='*')"))
    Write-Host "Tests with no hosting mode: $($tests_no_hostingmode.Length)"
    $total += $tests_no_hostingmode.Length

    $unrecognized_hosting_mode = (get-tests("(@Hosting:Mode='*' and not (@Hosting:Mode='WPF') and not (@Hosting:Mode='UAP') and not (@Hosting:Mode='Win32Explicit'))"))
    Write-Host "Tests with other hosting mode: $($unrecognized_hosting_mode.Length)"
    $total += $unrecognized_hosting_mode.Length
    Write-host "-----"
    Write-Host "Total of those tests: $total"
    Write-Host ""


    $all_tests = (get-tests("@Name=*"))
    Write-Host "All tests (should match above total): $($all_tests.Length)"

    Write-Host ""
    Write-Host "Checkers for older hosting mode paradigm:"
    Write-Host "Tests with hosting mode *WPF*: $((get-tests("(@Hosting:Mode='*WPF*')")).Length)"
    Write-Host "Tests with hosting mode *UAP*: $((get-tests("(@Hosting:Mode='*UAP*')")).Length)"
    Write-Host "Tests with hosting mode *WPF* and *UAP*: $((get-tests("(@Hosting:Mode='*WPF*')and(@Hosting:Mode='*UAP*')")).Length)"


    Write-Host ""

    $ignored_tests = (get-tests "(@Ignore='True')" "/runIgnoredTests")
    Write-Host "Tests with Ignore=true (Not run in lab): $($ignored_tests.Length)"

    Write-Host "Tests with method-level isolation (slows down run): $((get-tests "(@IsolationLevel='Method')").Length)"

    Pop-Location
    exit 0
}

# A mode selects tests; it never overrides their class-declared host.
switch ($HostingMode)
{
    'WPF'           { $queryArgs += " and (@Hosting:Mode='WPF')" }
    'UAP'           { $queryArgs += " and (@Hosting:Mode='UAP')" }
    'Win32Explicit' { $queryArgs += " and (@Hosting:Mode='Win32Explicit')" }
    'None'          { $queryArgs += " and not (@Hosting:Mode='*')" }
    ''              { $queryArgs += " and not (@Hosting:Mode='*')" }
}

if($RunTestsInALoop)
{
    $argsEx += ("/testmode:loop", "/looptest:3", "/loop:3")
}

if($TerminateOnFirstFailure)
{
    $argsEx += "/terminateOnFirstFailure"
}

Write-Host $argsEx
Write-Host $ExtraArgs

Write-Host ".\te.exe $testDllList /p:SkipConsoleWindowMinimize /select:`"$queryArgs`" $argsEx $ExtraArgs"
.\te.exe $testDllList.Split(" ") "/p:SkipConsoleWindowMinimize" "/select:`"$queryArgs`"" $argsEx $ExtraArgs
$testExitCode = $LASTEXITCODE

Pop-Location
exit $testExitCode

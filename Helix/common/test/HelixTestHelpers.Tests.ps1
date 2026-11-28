# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if (-not ('HelixTestHelpers.WttLogRecovery' -as [type]))
{
    Add-Type -Language CSharp -ReferencedAssemblies System.Xml,System.Xml.Linq,System.Runtime.Serialization,System.Runtime.Serialization.Json (Get-Content (Join-Path $PSScriptRoot "HelixTestHelpers.cs") -Raw)
}

function New-WttStartTest
{
    param([String]$Test, [Int64]$TimeStamp = 1000)

    @"
<StartTest Title="$Test" TUID="" >
<Data>
<WexTraceInfo ThreadId="1" ProcessId="2" TimeStamp="$TimeStamp" LogSessionId="1" SessionTraceCount="1"/>
</Data>
</StartTest>

"@
}

function New-WttTest
{
    param([String]$Test, [String]$Result = "Pass", [Int64]$TimeStamp = 1000)

    (New-WttStartTest -Test $Test -TimeStamp $TimeStamp) + @"
<EndTest Title="$Test" TUID="" Result="$Result" Repro="" >
<Data>
<WexTraceInfo ThreadId="1" ProcessId="2" TimeStamp="$($TimeStamp + 500)" LogSessionId="1" SessionTraceCount="2"/>
</Data>
</EndTest>

"@
}

function Write-WttLog
{
    # Writes a log shaped like te.exe's te.wtl. -Truncated cuts it off mid-element, the way te.exe leaves it when it
    # exits abnormally.
    param([String]$Path, [String]$Body, [Switch]$Truncated)

    $log = "<?xml version=`"1.0`" encoding=`"UTF-16`" ?>`r`n<WTT-Logger>`r`n<RTI ID=`"1`" Frequency=`"10000000`" />`r`n$Body"
    if ($Truncated)
    {
        $log += "<Msg `r`n`tUserText=`"Resetting Host`" >`r`n<Data>`r`n<WexTraceIn"
    }
    else
    {
        $log += "</WTT-Logger>`r`n"
    }

    [IO.File]::WriteAllText($Path, $log, [Text.Encoding]::Unicode)
}

function Get-TestOutcomes
{
    param([String]$Path)

    $testPass = [HelixTestHelpers.TestPass]::ParseTestWttFile($Path, $false, $false)
    ($testPass.TestResults | ForEach-Object { "$($_.Name)=$(if ($_.Passed) { 'Pass' } else { 'Fail' })" }) -join ","
}

Describe "WttLogRecovery.RecoverIncompleteLog" {
    BeforeEach {
        $script:wttPath = Join-Path $TestDrive "te.wtl"
        $script:consoleLogPath = Join-Path $TestDrive "te_console.log"
        $script:incompleteLogPath = Join-Path $TestDrive "te_original.incomplete.wtl"
        Remove-Item -LiteralPath $script:wttPath, $script:consoleLogPath, $script:incompleteLogPath -ErrorAction SilentlyContinue
    }

    function Write-ConsoleLog
    {
        param([String[]]$Lines)

        Set-Content -LiteralPath $script:consoleLogPath -Value $Lines -Encoding Unicode
    }

    function Invoke-Recovery
    {
        [HelixTestHelpers.WttLogRecovery]::RecoverIncompleteLog($script:wttPath, $script:consoleLogPath, $script:incompleteLogPath)
    }

    It "leaves a complete log alone" {
        Write-WttLog -Path $script:wttPath -Body (New-WttTest -Test "Tests::A")
        $originalHash = (Get-FileHash -LiteralPath $script:wttPath).Hash

        $result = Invoke-Recovery

        "$($result.State)" | Should Be "Complete"
        (Get-FileHash -LiteralPath $script:wttPath).Hash | Should Be $originalHash
        Test-Path -LiteralPath $script:incompleteLogPath | Should Be $false
    }

    It "adds the results te.exe printed but never wrote to te.wtl" {
        Write-WttLog -Path $script:wttPath -Truncated -Body ((New-WttTest -Test "Tests::A") + (New-WttTest -Test "Tests::B"))
        $originalHash = (Get-FileHash -LiteralPath $script:wttPath).Hash
        Write-ConsoleLog -Lines @(
            "StartGroup: Tests::A", "EndGroup: Tests::A [Passed]",
            "StartGroup: Tests::B", "EndGroup: Tests::B [Passed]",
            "StartGroup: Tests::C", "Verify: AreEqual(1, 1)", "EndGroup: Tests::C [Passed]",
            "StartGroup: Tests::D", "Error: Verify: AreEqual(1, 2)", "EndGroup: Tests::D [Failed]")

        $result = Invoke-Recovery

        "$($result.State)" | Should Be "Recovered"
        $result.ReadError | Should Match "Unexpected end of file"
        $result.TestsFromLog | Should Be 2
        $result.TestsFromConsole | Should Be 2
        $result.InFlightTests.Count | Should Be 0
        (Get-FileHash -LiteralPath $script:incompleteLogPath).Hash | Should Be $originalHash
        Get-TestOutcomes -Path $script:wttPath | Should Be "Tests::A=Pass,Tests::B=Pass,Tests::C=Pass,Tests::D=Fail"
        [HelixTestHelpers.FailedTestDetector]::GetFailedTestQuery($script:wttPath) | Should Be "(@Name='Tests::D')"
        $testPass = [HelixTestHelpers.TestPass]::ParseTestWttFile($script:wttPath, $false, $false)
        ($testPass.TestResults | Where-Object { $_.Name -eq "Tests::D" }).Details | Should Match "console output"
    }

    It "produces a log that converts to xUnit results" {
        Write-WttLog -Path $script:wttPath -Truncated -Body (New-WttTest -Test "Tests::A")
        Write-ConsoleLog -Lines @("EndGroup: Tests::A [Passed]", "EndGroup: Tests::B [Failed]", "EndGroup: Tests::C [Skipped]")
        Invoke-Recovery | Out-Null
        $xunitPath = Join-Path $TestDrive "testResults.xml"

        $parser = New-Object HelixTestHelpers.TestResultParser -ArgumentList "Prefix"
        $parser.ConvertWttLogToXUnitLog($script:wttPath, (Join-Path $TestDrive "te_rerun.wtl"), (Join-Path $TestDrive "te_rerun_multiple.wtl"), (Join-Path $TestDrive "te_rerun_more.wtl"), $xunitPath, 5, $true)

        [xml]$xunit = Get-Content -LiteralPath $xunitPath -Raw
        $xunit.assemblies.assembly.total | Should Be "3"
        $xunit.assemblies.assembly.passed | Should Be "1"
        $xunit.assemblies.assembly.failed | Should Be "1"
        $xunit.assemblies.assembly.skipped | Should Be "1"
    }

    It "reports the test that was running when te.exe exited as failed" {
        Write-WttLog -Path $script:wttPath -Truncated -Body ((New-WttTest -Test "Tests::A") + (New-WttStartTest -Test "Tests::B"))
        Write-ConsoleLog -Lines @("StartGroup: Tests::A", "EndGroup: Tests::A [Passed]", "StartGroup: Tests::B")

        $result = Invoke-Recovery

        "$($result.State)" | Should Be "Recovered"
        $result.TestsFromLog | Should Be 1
        $result.TestsFromConsole | Should Be 0
        ($result.InFlightTests -join ",") | Should Be "Tests::B"
        Get-TestOutcomes -Path $script:wttPath | Should Be "Tests::A=Pass,Tests::B=Fail"
        $testPass = [HelixTestHelpers.TestPass]::ParseTestWttFile($script:wttPath, $false, $false)
        ($testPass.TestResults | Where-Object { $_.Name -eq "Tests::B" }).Details | Should Match "exited while this test was running"
    }

    It "takes the result of a test whose end never reached te.wtl from the console" {
        Write-WttLog -Path $script:wttPath -Truncated -Body (New-WttStartTest -Test "Tests::A")
        Write-ConsoleLog -Lines @("StartGroup: Tests::A", "EndGroup: Tests::A [Passed]")

        $result = Invoke-Recovery

        $result.TestsFromConsole | Should Be 1
        $result.InFlightTests.Count | Should Be 0
        Get-TestOutcomes -Path $script:wttPath | Should Be "Tests::A=Pass"
    }

    It "reports a test that started on the console but never reached te.wtl as failed" {
        Write-WttLog -Path $script:wttPath -Truncated -Body (New-WttTest -Test "Tests::A")
        Write-ConsoleLog -Lines @("StartGroup: Tests::A", "EndGroup: Tests::A [Passed]", "StartGroup: Tests::C")

        $result = Invoke-Recovery

        ($result.InFlightTests -join ",") | Should Be "Tests::C"
        Get-TestOutcomes -Path $script:wttPath | Should Be "Tests::A=Pass,Tests::C=Fail"
    }

    It "trusts te.wtl when te.exe's last console lines were lost" {
        Write-WttLog -Path $script:wttPath -Truncated -Body (New-WttTest -Test "Tests::A")
        Write-ConsoleLog -Lines @("StartGroup: Tests::A")

        $result = Invoke-Recovery

        "$($result.State)" | Should Be "Recovered"
        $result.InFlightTests.Count | Should Be 0
        Get-TestOutcomes -Path $script:wttPath | Should Be "Tests::A=Pass"
    }

    It "counts repeated runs of a test, as in loop reruns" {
        Write-WttLog -Path $script:wttPath -Truncated -Body ((New-WttTest -Test "Tests::A" -Result "Fail") + (New-WttTest -Test "Tests::A"))
        Write-ConsoleLog -Lines @("EndGroup: Tests::A [Failed]", "EndGroup: Tests::A [Passed]", "EndGroup: Tests::A [Passed]")

        $result = Invoke-Recovery

        $result.TestsFromLog | Should Be 2
        $result.TestsFromConsole | Should Be 1
        Get-TestOutcomes -Path $script:wttPath | Should Be "Tests::A=Fail,Tests::A=Pass,Tests::A=Pass"
    }

    It "recovers from the console when te.exe didn't create te.wtl" {
        Write-ConsoleLog -Lines @("EndGroup: Tests::A [Passed]", "EndGroup: Tests::B [Failed]")

        $result = Invoke-Recovery

        "$($result.State)" | Should Be "Recovered"
        $result.LogFileExisted | Should Be $false
        Test-Path -LiteralPath $script:incompleteLogPath | Should Be $false
        Get-TestOutcomes -Path $script:wttPath | Should Be "Tests::A=Pass,Tests::B=Fail"
    }

    It "moves an unrecoverable log aside so it isn't read as results" {
        Write-WttLog -Path $script:wttPath -Truncated -Body ""

        $result = Invoke-Recovery

        "$($result.State)" | Should Be "Unrecoverable"
        Test-Path -LiteralPath $script:wttPath | Should Be $false
        Test-Path -LiteralPath $script:incompleteLogPath | Should Be $true
    }
}

Describe "WttLogRecovery.CheckSelectedTestsRan" {
    BeforeEach {
        $script:wttPath = Join-Path $TestDrive "te.wtl"
        $script:consoleLogPath = Join-Path $TestDrive "te_console.log"
        $script:incompleteLogPath = Join-Path $TestDrive "te_original.incomplete.wtl"
        Remove-Item -LiteralPath $script:wttPath, $script:consoleLogPath, $script:incompleteLogPath -ErrorAction SilentlyContinue
    }

    # Recovers a truncated log the way RunHelixWorkItem.ps1 does, then checks it against the tests te.exe /list selects.
    function Invoke-Check
    {
        param([String]$Body, [String[]]$ConsoleLines, [String[]]$SelectedTests)

        Write-WttLog -Path $script:wttPath -Truncated -Body $Body
        Set-Content -LiteralPath $script:consoleLogPath -Value $ConsoleLines -Encoding Unicode
        $recovery = [HelixTestHelpers.WttLogRecovery]::RecoverIncompleteLog($script:wttPath, $script:consoleLogPath, $script:incompleteLogPath)
        "$($recovery.State)" | Should Be "Recovered"

        [HelixTestHelpers.WttLogRecovery]::CheckSelectedTestsRan($script:wttPath, [string[]]$SelectedTests, [string[]]@($recovery.InFlightTests))
    }

    It "passes a run that te.exe exited after its last test" {
        $check = Invoke-Check `
            -Body ((New-WttTest -Test "Tests::A") + (New-WttTest -Test "Tests::B")) `
            -ConsoleLines @("EndGroup: Tests::A [Passed]", "EndGroup: Tests::B [Passed]", "EndGroup: Tests::C [Passed]") `
            -SelectedTests @("Tests::A", "Tests::B", "Tests::C")

        $check.AllSelectedTestsRan | Should Be $true
        $check.TestsWithoutResults.Count | Should Be 0
        $check.DataDrivenTestsCutShort.Count | Should Be 0
    }

    It "finds the selected tests te.exe never started when it exited between tests" {
        $check = Invoke-Check `
            -Body (New-WttTest -Test "Tests::A") `
            -ConsoleLines @("StartGroup: Tests::A", "EndGroup: Tests::A [Passed]") `
            -SelectedTests @("Tests::A", "Tests::B", "Tests::C")

        $check.AllSelectedTestsRan | Should Be $false
        ($check.TestsWithoutResults -join ",") | Should Be "Tests::B,Tests::C"
    }

    It "counts the test te.exe was running as run, but not the tests after it" {
        $check = Invoke-Check `
            -Body ((New-WttTest -Test "Tests::A") + (New-WttStartTest -Test "Tests::B")) `
            -ConsoleLines @("StartGroup: Tests::A", "EndGroup: Tests::A [Passed]", "StartGroup: Tests::B") `
            -SelectedTests @("Tests::A", "Tests::B", "Tests::C")

        $check.AllSelectedTestsRan | Should Be $false
        ($check.TestsWithoutResults -join ",") | Should Be "Tests::C"
    }

    It "matches the variations of a data-driven test to the one name /list gives it" {
        $check = Invoke-Check `
            -Body ((New-WttTest -Test "Tests::A#metadataSet0") + (New-WttTest -Test "Tests::A#metadataSet1") + (New-WttTest -Test "Tests::B")) `
            -ConsoleLines @("EndGroup: Tests::A#metadataSet0 [Passed]", "EndGroup: Tests::A#metadataSet1 [Passed]", "EndGroup: Tests::B [Passed]") `
            -SelectedTests @("Tests::A", "Tests::B")

        $check.AllSelectedTestsRan | Should Be $true
    }

    It "doesn't pass a data-driven test whose later variations te.exe may not have run" {
        $check = Invoke-Check `
            -Body ((New-WttTest -Test "Tests::A") + (New-WttTest -Test "Tests::B#0") + (New-WttTest -Test "Tests::B#1")) `
            -ConsoleLines @("EndGroup: Tests::A [Passed]", "EndGroup: Tests::B#0 [Passed]", "EndGroup: Tests::B#1 [Passed]") `
            -SelectedTests @("Tests::A", "Tests::B")

        $check.AllSelectedTestsRan | Should Be $false
        $check.TestsWithoutResults.Count | Should Be 0
        ($check.DataDrivenTestsCutShort -join ",") | Should Be "Tests::B"
    }

    It "finds a data-driven test te.exe was running even when a group it started ends the log" {
        # Neither start reached te.wtl, so recovery adds both as failed, ending the log with the group, whose name
        # looks like a variation but isn't a selected test.
        $check = Invoke-Check `
            -Body (New-WttTest -Test "Tests::A") `
            -ConsoleLines @("EndGroup: Tests::A [Passed]", "StartGroup: Tests::B#0", "StartGroup: Phase#2") `
            -SelectedTests @("Tests::A", "Tests::B")

        $check.AllSelectedTestsRan | Should Be $false
        $check.TestsWithoutResults.Count | Should Be 0
        ($check.DataDrivenTestsCutShort -join ",") | Should Be "Tests::B"
    }
}

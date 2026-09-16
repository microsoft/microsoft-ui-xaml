# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Unit tests for run-tests-on-vm.ps1's argument forwarding and result handling.
# Loads the runner's helpers and supplies synthetic arguments and TAEF output.
# Does not connect to a VM, deploy a payload, or run WinUI tests.
# No test framework is required.
# Run manually from the repo root:
# powershell.exe -NoProfile -File tools\tests\run-tests-on-vm.UnitTests.ps1
$ErrorActionPreference = "Stop"
$runnerPath = Join-Path $PSScriptRoot "..\run-tests-on-vm.ps1"
$parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($runnerPath, [ref]$null, [ref]$parseErrors)
if ($parseErrors.Count -ne 0) { throw ($parseErrors -join "`n") }

foreach ($functionName in @("Read-NormalizedLog", "Get-TaefExitCode", "Invoke-TestsOnVM")) {
    $definitions = @($ast.FindAll({
        param($node)
        $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $functionName
    }, $true))
    if ($definitions.Count -ne 1) { throw "Expected exactly one $functionName definition." }
    . ([scriptblock]::Create($definitions[0].Extent.Text))
}

$script:caseCount = 0
function Assert-ExitCode {
    param(
        [string]$Name,
        [string[]]$LogLines,
        [int]$Expected,
        [int]$ProcessExitCode = 0,
        [bool]$IsDiscoveryRun = $false
    )

    $actual = Get-TaefExitCode -ProcessExitCode $ProcessExitCode -LogLines $LogLines `
        -IsDiscoveryRun $IsDiscoveryRun 6>$null
    if ($actual -ne $Expected) { throw "${Name}: expected exit code $Expected, got $actual." }
    $script:caseCount++
}

$passed = "Summary: Total=1, Passed=1, Failed=0, Blocked=0, Not Run=0, Skipped=0"
$leakError = "Error: memory.cpp[353]: *** Leaked memory totals: 5 blocks, 1656 bytes. ***"
$cleanupError = "Error: TAEF: Cleanup fixture 'CheckBoxIntegrationTests::TestCleanup' failed."

Assert-ExitCode -Name "Clean test" -LogLines $passed -Expected 0
Assert-ExitCode -Name "Native process failure is preserved" -LogLines $passed -ProcessExitCode 42 -Expected 42
Assert-ExitCode -Name "Leak outside passing test body" -LogLines @($leakError, $passed) -Expected 1
Assert-ExitCode -Name "Cleanup outside passing test body" -LogLines @($cleanupError, $passed) -Expected 1
Assert-ExitCode -Name "Indented error summary" -LogLines @("    $cleanupError", $passed) -Expected 1
Assert-ExitCode -Name "Outside-test error summary" -LogLines @("Summary of Errors Outside of Tests:", $passed) -Expected 1
Assert-ExitCode -Name "Error after result summary" -LogLines @($passed, $cleanupError) -Expected 1
Assert-ExitCode -Name "Warning is not failure" -LogLines @("Warning: an ignored leak", $passed) -Expected 0
Assert-ExitCode -Name "Error text inside ordinary output" -LogLines @("Verifying the text 'Error: example'", $passed) -Expected 0
Assert-ExitCode -Name "Failed count" -LogLines "Summary: Total=1, Passed=0, Failed=1, Blocked=0, Not Run=0, Skipped=0" -Expected 1
Assert-ExitCode -Name "Blocked count" -LogLines "Summary: Total=1, Passed=0, Failed=0, Blocked=1, Not Run=0, Skipped=0" -Expected 1
Assert-ExitCode -Name "Unexecuted count" -LogLines "Summary: Total=1, Passed=0, Failed=0, Blocked=0, Not Run=1, Skipped=0" -Expected 1
Assert-ExitCode -Name "Skipped test remains non-failing" -LogLines "Summary: Total=1, Passed=0, Failed=0, Blocked=0, Not Run=0, Skipped=1" -Expected 0
Assert-ExitCode -Name "No selected tests" -LogLines "Summary: Total=0, Passed=0, Failed=0, Blocked=0, Not Run=0, Skipped=0" -Expected 1
Assert-ExitCode -Name "Missing summary" -LogLines "Test process started" -Expected 1
Assert-ExitCode -Name "Incomplete summary" -LogLines "Summary: Total=1, Passed=1" -Expected 1
Assert-ExitCode -Name "Missing log" -LogLines @() -Expected 1
Assert-ExitCode -Name "Only intermediate loop summary" -LogLines "Loop $passed" -Expected 1
Assert-ExitCode -Name "Complete loop run" -LogLines @("Loop $passed", $passed) -Expected 0
Assert-ExitCode -Name "Discovery needs no result summary" -LogLines "CheckBoxIntegrationTests::CanInstantiate" -IsDiscoveryRun $true -Expected 0
Assert-ExitCode -Name "Discovery errors still fail" -LogLines $cleanupError -IsDiscoveryRun $true -Expected 1

& {
    # Stub remoting in this scope, but use the runner's actual remote parameter declaration.
    function Invoke-Command {
        param($Session, [scriptblock]$ScriptBlock, [object[]]$ArgumentList)

        $probe = [scriptblock]::Create($ScriptBlock.Ast.ParamBlock.Extent.Text + @'

[pscustomobject]@{ Directory = $dir; TestArgs = $testArgs; User = $taskUser; Discovery = $isDiscoveryRun }
'@)
        $serializedArgs = [System.Management.Automation.PSSerializer]::Deserialize(
            [System.Management.Automation.PSSerializer]::Serialize($ArgumentList))
        $received = & $probe @serializedArgs
        if ($received.Directory -ne "C:\payload with spaces" -or $received.User -ne "test-user" -or
            $received.Discovery -ne $case.Discovery) {
            throw "$($case.Name): remote parameter positions changed."
        }
        if ($received.TestArgs.Count -ne $case.Args.Count) {
            throw "$($case.Name): expected $($case.Args.Count) separate test arguments, got $($received.TestArgs.Count)."
        }
        for ($i = 0; $i -lt $case.Args.Count; $i++) {
            if ($received.TestArgs[$i] -cne $case.Args[$i]) {
                throw "$($case.Name): test argument $i changed during remoting."
            }
        }
        return 42
    }

    foreach ($case in @(
        @{ Name = "Single test argument"; Args = @("Pilot*"); Discovery = $false },
        @{ Name = "Host options and argument with spaces"; Args = @("Pilot*", "-HostingMode", "WPF", "-SkipPackageUninstall", "/p:Value=two words"); Discovery = $false },
        @{ Name = "Discovery argument forwarding"; Args = @("Pilot*", "-HostingMode", "WPF", "-SkipPackageUninstall", "/listProperties"); Discovery = $true }
    )) {
        $actual = Invoke-TestsOnVM -Session "unit-test-session" -RemoteTestDir "C:\payload with spaces" `
            -TestArgs $case.Args -TaskUser "test-user" 6>$null
        if ($actual -ne 42) { throw "$($case.Name): remote exit code was not preserved." }
        $script:caseCount++
    }
}

$testDirectory = Join-Path ([System.IO.Path]::GetTempPath()) ("WinUI-VmResultTests-" + [guid]::NewGuid())
$logPath = Join-Path $testDirectory "testrun-output.log"
New-Item -ItemType Directory -Path $testDirectory | Out-Null
try {
    $failedLog = "$leakError`r`n$cleanupError`r`n$passed`r`n"
    $fixtures = @(
        @{ Name = "UTF-8"; Bytes = [System.Text.Encoding]::UTF8.GetBytes($failedLog); Expected = 1 },
        @{ Name = "UTF-8 BOM"; Bytes = [System.Text.Encoding]::UTF8.GetPreamble() + [System.Text.Encoding]::UTF8.GetBytes($failedLog); Expected = 1 },
        @{ Name = "UTF-16LE"; Bytes = [System.Text.Encoding]::Unicode.GetBytes($failedLog); Expected = 1 },
        @{ Name = "UTF-16LE BOM at first error"; Bytes = [System.Text.Encoding]::Unicode.GetPreamble() + [System.Text.Encoding]::Unicode.GetBytes("$leakError`r`n$passed"); Expected = 1 },
        @{ Name = "Mixed runner/TAEF encoding"; Bytes = [System.Text.Encoding]::UTF8.GetBytes("Hosting mode is 'WPF'.`r`n") + [System.Text.Encoding]::Unicode.GetBytes($failedLog); Expected = 1 },
        @{ Name = "Clean mixed encoding"; Bytes = [System.Text.Encoding]::UTF8.GetBytes("Hosting mode is 'WPF'.`r`n") + [System.Text.Encoding]::Unicode.GetBytes($passed); Expected = 0 },
        @{ Name = "Empty log"; Bytes = [byte[]]@(); Expected = 1 },
        @{ Name = "UTF-8 BOM only"; Bytes = [System.Text.Encoding]::UTF8.GetPreamble(); Expected = 1 },
        @{ Name = "UTF-16LE BOM only"; Bytes = [System.Text.Encoding]::Unicode.GetPreamble(); Expected = 1 }
    )
    foreach ($fixture in $fixtures) {
        [System.IO.File]::WriteAllBytes($logPath, [byte[]]$fixture.Bytes)
        Assert-ExitCode -Name $fixture.Name -LogLines @(Read-NormalizedLog $logPath) -Expected $fixture.Expected
    }
    Remove-Item -LiteralPath $logPath
    Assert-ExitCode -Name "Missing log file" -LogLines @(Read-NormalizedLog $logPath) -Expected 1
} finally {
    if (Test-Path -LiteralPath $logPath) { Remove-Item -LiteralPath $logPath }
    Remove-Item -LiteralPath $testDirectory
}

Write-Output "Passed $script:caseCount VM-wrapper checks."

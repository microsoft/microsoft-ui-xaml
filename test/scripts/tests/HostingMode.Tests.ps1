# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Run with Windows PowerShell: powershell -NoProfile -File .\test\scripts\tests\HostingMode.Tests.ps1
$ErrorActionPreference = 'Stop'

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

$repoRoot = (Resolve-Path "$PSScriptRoot\..\..\..").Path
$fixture = Join-Path ([IO.Path]::GetTempPath()) ("WinUI-HostingMode-" + [Guid]::NewGuid())
New-Item -ItemType Directory -Path $fixture | Out-Null
$savedArgsFile = $env:WINUI_HOSTING_TEST_ARGS_FILE
$savedExitCode = $env:WINUI_HOSTING_TEST_EXIT_CODE
try {
    Copy-Item "$repoRoot\test\scripts\runtests.ps1" $fixture
    Add-Type -OutputAssembly "$fixture\te.exe" -OutputType ConsoleApplication -TypeDefinition @'
using System;
using System.IO;
public static class TestRunner
{
    public static int Main(string[] args)
    {
        File.WriteAllLines(Environment.GetEnvironmentVariable("WINUI_HOSTING_TEST_ARGS_FILE"), args);
        return int.Parse(Environment.GetEnvironmentVariable("WINUI_HOSTING_TEST_EXIT_CODE"));
    }
}
'@

    $env:WINUI_HOSTING_TEST_ARGS_FILE = "$fixture\arguments.txt"
    $env:WINUI_HOSTING_TEST_EXIT_CODE = '0'
    $cases = @(
        @{ Arguments = @(); Filter = $null },
        @{ Arguments = @('-HostingMode', 'Auto'); Filter = $null },
        @{ Arguments = @('-HostingMode', 'WPF'); Filter = "@Hosting:Mode='WPF'" },
        @{ Arguments = @('-HostingMode', 'UAP'); Filter = "@Hosting:Mode='UAP'" },
        @{ Arguments = @('-HostingMode', 'Win32Explicit'); Filter = "@Hosting:Mode='Win32Explicit'" },
        @{ Arguments = @('-HostingMode', 'None'); Filter = "not (@Hosting:Mode='*')" },
        @{ Arguments = @('-WpfMode'); Filter = "@Hosting:Mode='WPF'" },
        @{ Arguments = @('-Win32Explicit'); Filter = "@Hosting:Mode='Win32Explicit'" }
    )
    foreach ($case in $cases) {
        $arguments = $case.Arguments
        & powershell.exe -NoProfile -File "$fixture\runtests.ps1" 'HostingMode*' -SkipPackageUninstall @arguments | Out-Null
        Assert-True ($LASTEXITCODE -eq 0) "Runner failed for $arguments."
        $actual = [IO.File]::ReadAllLines($env:WINUI_HOSTING_TEST_ARGS_FILE)
        Assert-True (@($actual | Where-Object { $_ -match '^/p:HostingMode' }).Count -eq 0) 'Runner injected a hosting override.'
        Assert-True (@($actual | Where-Object { $_ -eq '/list' }).Count -eq 0) 'Runner probed hosting modes before execution.'
        $query = @($actual | Where-Object { $_ -like '/select:*' })
        Assert-True ($query.Count -eq 1) 'Expected one TAEF selection.'
        if ($case.Filter) {
            Assert-True ($query[0].Contains($case.Filter)) "Missing filter $($case.Filter)."
        } else {
            Assert-True (-not $query[0].Contains('Hosting:Mode')) 'Automatic selection excluded hosting modes.'
        }
    }

    $env:WINUI_HOSTING_TEST_EXIT_CODE = '7'
    & powershell.exe -NoProfile -File "$fixture\runtests.ps1" 'HostingMode*' -SkipPackageUninstall | Out-Null
    Assert-True ($LASTEXITCODE -eq 7) 'Runner did not propagate the TAEF failure exit code.'
    $env:WINUI_HOSTING_TEST_EXIT_CODE = '0'

    @(' FirstTests::WantedTest ', '', " `t ", "SecondTests::WantedTest`t[Failed]", '') |
        Set-Content "$fixture\test-names.txt"
    & powershell.exe -NoProfile -File "$fixture\runtests.ps1" '*' -SkipPackageUninstall -fromFile "$fixture\test-names.txt" | Out-Null
    Assert-True ($LASTEXITCODE -eq 0) 'Runner rejected a list containing valid names and blank lines.'
    $query = [IO.File]::ReadAllLines($env:WINUI_HOSTING_TEST_ARGS_FILE) | Where-Object { $_ -like '/select:*' }
    Assert-True ($query.Contains("@Name='*FirstTests::WantedTest'") -and $query.Contains("@Name='*SecondTests::WantedTest'")) 'Test list lost valid names.'
    Assert-True (-not $query.Contains("@Name='*'")) 'Blank lines broadened the test selection.'
    foreach ($contents in @('', " `r`n`t`r`n")) {
        Set-Content "$fixture\test-names.txt" $contents
        Remove-Item $env:WINUI_HOSTING_TEST_ARGS_FILE -ErrorAction SilentlyContinue
        try {
            $ErrorActionPreference = 'Continue'
            & powershell.exe -NoProfile -File "$fixture\runtests.ps1" '*' -SkipPackageUninstall -fromFile "$fixture\test-names.txt" *> "$fixture\empty-list.log"
        } finally {
            $ErrorActionPreference = 'Stop'
        }
        Assert-True ($LASTEXITCODE -ne 0) 'Runner accepted an empty test list.'
        Assert-True (-not (Test-Path $env:WINUI_HOSTING_TEST_ARGS_FILE)) 'Runner launched TAEF for an empty test list.'
    }

    foreach ($argument in @('-forceHostingMode', '/p:HostingMode=WPF')) {
        if (Test-Path $env:WINUI_HOSTING_TEST_ARGS_FILE) {
            Remove-Item $env:WINUI_HOSTING_TEST_ARGS_FILE
        }
        try {
            $ErrorActionPreference = 'Continue'
            & powershell.exe -NoProfile -File "$fixture\runtests.ps1" 'HostingMode*' -SkipPackageUninstall $argument *> "$fixture\rejected.log"
        } finally {
            $ErrorActionPreference = 'Stop'
        }
        Assert-True ($LASTEXITCODE -ne 0) "Runner accepted $argument."
        Assert-True (-not (Test-Path $env:WINUI_HOSTING_TEST_ARGS_FILE)) "Runner launched TAEF with $argument."
    }

    Copy-Item "$repoRoot\Helix\GenerateHelixWorkItems.ps1" "$fixture\GenerateHelixWorkItems.ps1"
    New-Item -ItemType Directory -Path "$fixture\common\pipeline" | Out-Null
    @'
param(
    $TestFilePattern, $TestBinaryDirectoryPath, $OutputProjFile, $TestExecutionMultiplier,
    $RunIgnoredTests, $WorkItemPrefix, $TaefBaseQuery, $TestTimeout, $TaefExtraParameters,
    $TestNamePrefix, $TaefExePath
)
$PSBoundParameters | ConvertTo-Json | Set-Content $OutputProjFile
'@ | Set-Content "$fixture\common\pipeline\GenerateHelixWorkItems.ps1"
    foreach ($mode in @('WPF', 'UAP', 'Win32Explicit')) {
        & "$fixture\GenerateHelixWorkItems.ps1" -TestFilePattern '*.dll' -TestBinaryPath "$fixture\Test" `
            -OutputProjFile "$fixture\workitems.json" -JobTestSuiteName DevTestSuite -HostingMode $mode
        $workItems = Get-Content "$fixture\workitems.json" -Raw | ConvertFrom-Json
        Assert-True (-not $workItems.TaefExtraParameters) 'Helix passed a hosting runtime parameter.'
        Assert-True ($workItems.TaefBaseQuery.Contains("@Hosting:Mode='$mode'")) "Helix lost the $mode selection."
        $prefix = if ($mode -eq 'UAP') { '' } else { $mode }
        Assert-True ($workItems.WorkItemPrefix -eq $prefix -and $workItems.TestNamePrefix -eq $prefix) "Helix changed the $mode test labels."
    }

    $tokens = $null
    $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile(
        "$repoRoot\tools\run-tests-on-vm.ps1", [ref]$tokens, [ref]$errors)
    Assert-True ($errors.Count -eq 0) 'VM runner has PowerShell syntax errors.'
    foreach ($functionName in @('Get-WtlSummary', 'Get-TestRunResult')) {
        $function = $ast.Find({
            param($node)
            $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $functionName
        }, $true)
        . ([scriptblock]::Create($function.Extent.Text))
    }

    $wtl = "$fixture\te.wtl"
    $summary = Get-WtlSummary $wtl
    Assert-True ($summary.Total -eq 0) 'Missing results were reported as tests.'
    @'
<WTT-Logger>
<StartTest Title="Pass" /><EndTest Title="Pass" Result="Pass" />
<StartTest Title="Skip" /><EndTest Title="Skip" Result="Skipped" />
<StartTest Title="Fail" /><EndTest Title="Fail" Result="Fail" />
<StartTest Title="Block" /><EndTest Title="Block" Result="Blocked" />
<PFRollup Total="4" Passed="1" Failed="1" Blocked="1" Skipped="1" />
</WTT-Logger>
'@ | Set-Content $wtl -Encoding Unicode
    $summary = Get-WtlSummary $wtl
    Assert-True ($summary.Complete -and $summary.Total -eq 4 -and $summary.Passed -eq 1 -and $summary.Failed -eq 2 -and $summary.Skipped -eq 1) 'Incorrect WTL result counts.'

    $passRecords = '<StartTest Title="Test" /><EndTest Title="Test" Result="Pass" />'
    $rollup = '<PFRollup Total="1" Passed="1" Failed="0" Blocked="0" Skipped="0" />'
    $passingWtl = "<WTT-Logger>$passRecords$rollup</WTT-Logger>"
    $passingConsole = 'Summary: Total=1, Passed=1, Failed=0, Blocked=0, Not Run=0, Skipped=0'
    $failedConsole = 'Summary: Total=1, Passed=0, Failed=1, Blocked=0, Not Run=0, Skipped=0'
    $verdictCases = @(
        @{ Name = 'Complete pass'; Wtl = $passingWtl; Console = $passingConsole; Passed = $true },
        @{ Name = 'Complete WTL without console'; Wtl = $passingWtl; Console = ''; Passed = $true },
        @{ Name = 'Console failure overrides passing WTL'; Wtl = $passingWtl; Console = $failedConsole; Passed = $false },
        @{ Name = 'Cleanup error after passing body'; Wtl = "<WTT-Logger>$passRecords<Error UserText='Cleanup failed' />$rollup</WTT-Logger>"; Console = $passingConsole; Passed = $false },
        @{ Name = 'Failed cleanup group'; Wtl = "<WTT-Logger>$passRecords<Msg><Data><EndGroup Result='Failed' /></Data></Msg>$rollup</WTT-Logger>"; Console = $passingConsole; Passed = $false },
        @{ Name = 'Incomplete console summary'; Wtl = $passingWtl; Console = 'Summary: Total=1, Passed=0, Failed=1'; Passed = $false },
        @{ Name = 'Unmatched start'; Wtl = "<WTT-Logger>$passRecords<StartTest Title='Next' />$rollup</WTT-Logger>"; Console = $passingConsole; Passed = $false },
        @{ Name = 'Mismatched test identity'; Wtl = $passingWtl.Replace('<EndTest Title="Test"', '<EndTest Title="Other"'); Console = $passingConsole; Passed = $false },
        @{ Name = 'Missing final rollup'; Wtl = "<WTT-Logger>$passRecords</WTT-Logger>"; Console = $passingConsole; Passed = $false },
        @{ Name = 'Truncated XML'; Wtl = "<WTT-Logger>$passRecords"; Console = $passingConsole; Passed = $false },
        @{ Name = 'WTL failure overrides console pass'; Wtl = $passingWtl.Replace('Result="Pass"', 'Result="Fail"'); Console = $passingConsole; Passed = $false },
        @{ Name = 'Console-only pass'; Wtl = $null; Console = $passingConsole; Passed = $true },
        @{ Name = 'No results'; Wtl = $null; Console = ''; Passed = $false },
        @{ Name = 'No executed tests'; Wtl = $null; Console = 'Summary: Total=0, Passed=0, Failed=0, Blocked=0, Not Run=0, Skipped=0'; Passed = $false },
        @{ Name = 'Blocked test'; Wtl = $null; Console = 'Summary: Total=2, Passed=1, Failed=0, Blocked=1, Not Run=0, Skipped=0'; Passed = $false },
        @{ Name = 'Unexecuted test'; Wtl = $null; Console = 'Summary: Total=2, Passed=1, Failed=0, Blocked=0, Not Run=1, Skipped=0'; Passed = $false }
    )
    foreach ($case in $verdictCases) {
        if ($null -eq $case.Wtl) {
            Remove-Item $wtl -ErrorAction SilentlyContinue
        } else {
            Set-Content $wtl $case.Wtl -Encoding Unicode
        }
        $result = Get-TestRunResult (Get-WtlSummary $wtl -WarningAction SilentlyContinue) @($case.Console) 0
        Assert-True (($result.ExitCode -eq 0) -eq $case.Passed) "Incorrect verdict: $($case.Name)."
    }
    Set-Content $wtl $passingWtl -Encoding Unicode
    $result = Get-TestRunResult (Get-WtlSummary $wtl) @($passingConsole) 7
    Assert-True ($result.ExitCode -eq 7) 'Result reconciliation discarded a failing process exit code.'
    Write-Host 'Hosting-mode runner tests passed.'
} finally {
    $env:WINUI_HOSTING_TEST_ARGS_FILE = $savedArgsFile
    $env:WINUI_HOSTING_TEST_EXIT_CODE = $savedExitCode
    Remove-Item -LiteralPath $fixture -Recurse -Force
}

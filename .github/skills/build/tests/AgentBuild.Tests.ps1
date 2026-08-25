<#
.SYNOPSIS
    Tests for .github/skills/build/Invoke-AgentBuild.ps1.

.DESCRIPTION
    These tests do not build the repository. They run the wrapper against a stub
    repository whose init and build scripts are replaced with controllable fakes, so the
    whole suite finishes in seconds instead of the hour a real build takes.

    Each test covers a failure that is silent in practice: a build that reports success
    after failing, a crash that produces no diagnostics, or setup that half-completes.

.EXAMPLE
    powershell -NoProfile -File .\.github\skills\build\tests\AgentBuild.Tests.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
$wrapper = Join-Path $repoRoot '.github\skills\build\Invoke-AgentBuild.ps1'

$script:Passed = 0
$script:Failed = 0
$script:Failures = @()

function Test-Case {
    param([string]$Name, [scriptblock]$Body)

    try {
        & $Body
        Write-Host "[PASS] $Name" -ForegroundColor Green
        $script:Passed++
    }
    catch {
        Write-Host "[FAIL] $Name - $($_.Exception.Message)" -ForegroundColor Red
        $script:Failed++
        $script:Failures += "$Name : $($_.Exception.Message)"
    }
}

function Assert-Equal {
    param($Expected, $Actual, [string]$Message)
    if ($Expected -ne $Actual) {
        throw "$Message Expected '$Expected', got '$Actual'."
    }
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function New-StubRepo {
    <#
        Creates a throwaway repository root containing stubs for the scripts the wrapper
        calls. Behaviour is driven by AGENTBUILD_TEST_* environment variables so each
        test can choose what the build does.
    #>
    param([switch]$WithSpaceInPath, [switch]$Initialized)

    $suffix = [guid]::NewGuid().Guid.Substring(0, 8)
    $leaf = if ($WithSpaceInPath) { "agent build $suffix" } else { "agentbuild$suffix" }
    $root = Join-Path ([IO.Path]::GetTempPath()) $leaf
    New-Item -ItemType Directory -Path $root -Force | Out-Null

    Set-Content -LiteralPath (Join-Path $root 'Build.cmd') -Value @'
@echo off
rem Stub. The wrapper reaches build.cmd through initrun.ps1, which is also a stub.
exit /b 0
'@ -Encoding ASCII

    Set-Content -LiteralPath (Join-Path $root 'init.cmd') -Value @'
@echo off
echo Initializing stub environment for %1
echo ran > "%~dp0init-was-run.txt"
if "%AGENTBUILD_TEST_INIT_FAIL%"=="1" (
    echo ERROR: init.cmd %1 /envcheck failed
    exit /b 1
)
if not exist "%~dp0packages" mkdir "%~dp0packages"
if not exist "%~dp0.tools" mkdir "%~dp0.tools"
exit /b 0
'@ -Encoding ASCII

    Set-Content -LiteralPath (Join-Path $root 'initrun.ps1') -Value @'
param(
    [string]$Flavor = "amd64chk",
    [Parameter(Position = 0, ValueFromRemainingArguments = $true)]
    [string[]]$Command
)

# Record how the wrapper invoked us, so tests can assert on argument forwarding.
"$Flavor|$($Command -join ' ')" | Set-Content -LiteralPath $env:AGENTBUILD_TEST_ARGSFILE

if ($env:AGENTBUILD_TEST_OUTPUT) {
    foreach ($line in ($env:AGENTBUILD_TEST_OUTPUT -split "`n")) { Write-Output $line }
}

if ($env:AGENTBUILD_TEST_BINLOG -eq "1") {
    $outputDir = Join-Path $PSScriptRoot "BuildOutput"
    if (-not (Test-Path -LiteralPath $outputDir)) {
        New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    }
    Set-Content -LiteralPath (Join-Path $outputDir "MUXControls.amd64chk.binlog") -Value "stub"
}

$code = 0
if ($env:AGENTBUILD_TEST_EXIT) { $code = [int]$env:AGENTBUILD_TEST_EXIT }
exit $code
'@ -Encoding ASCII

    if ($Initialized) {
        New-Item -ItemType Directory -Path (Join-Path $root 'packages') -Force | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $root '.tools') -Force | Out-Null
    }

    $env:AGENTBUILD_TEST_ARGSFILE = Join-Path $root 'invocation.txt'
    return $root
}

function Reset-StubBehavior {
    $env:AGENTBUILD_TEST_OUTPUT = $null
    $env:AGENTBUILD_TEST_EXIT = $null
    $env:AGENTBUILD_TEST_BINLOG = '1'
    $env:AGENTBUILD_TEST_INIT_FAIL = $null
}

function Invoke-Wrapper {
    param([string]$Root, [hashtable]$Arguments = @{})

    # *>&1 captures Write-Host as well; the wrapper reports its result through the host.
    $output = & $wrapper -RepoRoot $Root @Arguments *>&1 | Out-String
    return [pscustomobject]@{
        ExitCode = $LASTEXITCODE
        Output   = $output
    }
}

function Get-RecordedInvocation {
    param([string]$Root)
    $file = Join-Path $Root 'invocation.txt'
    if (-not (Test-Path -LiteralPath $file)) { return $null }
    return (Get-Content -LiteralPath $file -Raw).Trim()
}

function Remove-StubRepo {
    param([string]$Root)
    Remove-Item -LiteralPath $Root -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Testing $wrapper`n"

# --- Exit code correctness ---------------------------------------------------------------

Test-Case 'A successful build exits 0 and reports the binary log' {
    Reset-StubBehavior
    $root = New-StubRepo -Initialized
    try {
        $result = Invoke-Wrapper -Root $root
        Assert-Equal 0 $result.ExitCode 'Successful build did not exit 0.'
        Assert-True ($result.Output -match 'BUILD SUCCEEDED') 'Success was not reported.'
        Assert-True ($result.Output -match 'MUXControls\.amd64chk\.binlog') 'Binary log path was not reported on success.'
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'A failed build is reported as a failure even when it exits 0' {
    # The bug this wrapper exists for: build.cmd loses the failing exit code.
    Reset-StubBehavior
    $env:AGENTBUILD_TEST_OUTPUT = 'ERROR: buildSolution for MUXControls.sln FAILED.  Binlog is here: BuildOutput\x.binlog'
    $env:AGENTBUILD_TEST_EXIT = '0'
    $root = New-StubRepo -Initialized
    try {
        $result = Invoke-Wrapper -Root $root
        Assert-Equal 1 $result.ExitCode 'A failed build was reported as success.'
        Assert-True ($result.Output -match 'BUILD FAILED') 'Failure was not reported.'
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'A compiler diagnostic is treated as a failure even when the build exits 0' {
    Reset-StubBehavior
    $env:AGENTBUILD_TEST_OUTPUT = 'foo.cpp(12): error C3859: Failed to create virtual memory for PCH'
    $env:AGENTBUILD_TEST_EXIT = '0'
    $root = New-StubRepo -Initialized
    try {
        $result = Invoke-Wrapper -Root $root
        Assert-Equal 1 $result.ExitCode 'A compiler error was reported as success.'
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'A non-zero build exit code is reported as a failure' {
    Reset-StubBehavior
    $env:AGENTBUILD_TEST_EXIT = '7'
    $root = New-StubRepo -Initialized
    try {
        $result = Invoke-Wrapper -Root $root
        Assert-Equal 1 $result.ExitCode 'A non-zero exit code was not reported as failure.'
        Assert-True ($result.Output -match 'exit code 7') 'The underlying exit code was not surfaced.'
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'A crash that produces no binary log is reported as a failure' {
    # An MSBuild access violation returns a negative code, which build.cmd's own
    # "if ERRORLEVEL 1" check does not catch, so nothing is printed.
    Reset-StubBehavior
    $env:AGENTBUILD_TEST_BINLOG = '0'
    $env:AGENTBUILD_TEST_EXIT = '0'
    $root = New-StubRepo -Initialized
    try {
        $result = Invoke-Wrapper -Root $root
        Assert-Equal 1 $result.ExitCode 'A crash with no binary log was reported as success.'
        Assert-True ($result.Output -match 'crashed') 'The crash was not explained.'
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'A /fake build succeeds without producing a binary log' {
    Reset-StubBehavior
    $env:AGENTBUILD_TEST_BINLOG = '0'
    $root = New-StubRepo -Initialized
    try {
        $result = Invoke-Wrapper -Root $root -Arguments @{ BuildArguments = @('/fake') }
        Assert-Equal 0 $result.ExitCode 'A dry run was reported as a failure.'
    }
    finally { Remove-StubRepo $root }
}

# --- Initialization ------------------------------------------------------------------------

Test-Case 'First-time initialization runs automatically before the build' {
    Reset-StubBehavior
    $root = New-StubRepo   # deliberately not initialized
    try {
        $result = Invoke-Wrapper -Root $root
        Assert-True (Test-Path -LiteralPath (Join-Path $root 'init-was-run.txt')) 'Initialization did not run.'
        Assert-Equal 0 $result.ExitCode 'Build after automatic initialization did not succeed.'
        Assert-True ($null -ne (Get-RecordedInvocation -Root $root)) 'The build did not run after initialization.'
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'Initialization is skipped when the repository is already initialized' {
    Reset-StubBehavior
    $root = New-StubRepo -Initialized
    try {
        Invoke-Wrapper -Root $root | Out-Null
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $root 'init-was-run.txt'))) 'Initialization ran unnecessarily.'
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'A failed initialization stops before the build runs' {
    Reset-StubBehavior
    $env:AGENTBUILD_TEST_INIT_FAIL = '1'
    $root = New-StubRepo
    try {
        $result = Invoke-Wrapper -Root $root
        Assert-Equal 1 $result.ExitCode 'A failed initialization did not fail the command.'
        Assert-True ($null -eq (Get-RecordedInvocation -Root $root)) 'The build ran despite a failed initialization.'
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'SkipInitialize fails fast on an uninitialized repository' {
    Reset-StubBehavior
    $root = New-StubRepo
    try {
        $result = Invoke-Wrapper -Root $root -Arguments @{ SkipInitialize = $true }
        Assert-Equal 1 $result.ExitCode 'SkipInitialize did not fail on an uninitialized repository.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $root 'init-was-run.txt'))) 'SkipInitialize still ran initialization.'
    }
    finally { Remove-StubRepo $root }
}

# --- Argument handling ---------------------------------------------------------------------

Test-Case 'The build runs from a repository path containing a space' {
    Reset-StubBehavior
    $root = New-StubRepo -WithSpaceInPath
    try {
        Assert-True ($root -match ' ') 'The test repository path has no space in it.'
        $result = Invoke-Wrapper -Root $root
        Assert-Equal 0 $result.ExitCode 'A path containing a space failed the build.'
        Assert-True (Test-Path -LiteralPath (Join-Path $root 'init-was-run.txt')) 'Initialization did not run from a spaced path.'
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'Target and flavor are forwarded to the build' {
    Reset-StubBehavior
    $root = New-StubRepo -Initialized
    try {
        Invoke-Wrapper -Root $root -Arguments @{ Target = 'mux'; Flavor = 'arm64fre' } | Out-Null
        $invocation = Get-RecordedInvocation -Root $root
        Assert-True ($invocation -match '^arm64fre\|') "Flavor was not forwarded. Got '$invocation'."
        Assert-True ($invocation -match '\bmux\b') "Target was not forwarded. Got '$invocation'."
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'The default target is not passed as an argument' {
    Reset-StubBehavior
    $root = New-StubRepo -Initialized
    try {
        Invoke-Wrapper -Root $root | Out-Null
        $invocation = Get-RecordedInvocation -Root $root
        Assert-True ($invocation -notmatch 'prodtest') "The default target was passed explicitly. Got '$invocation'."
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'Builds are quiet by default and detailed on request' {
    Reset-StubBehavior
    $root = New-StubRepo -Initialized
    try {
        Invoke-Wrapper -Root $root | Out-Null
        $quiet = Get-RecordedInvocation -Root $root
        Assert-True ($quiet -match '/q') "Quiet mode was not requested by default. Got '$quiet'."

        Invoke-Wrapper -Root $root -Arguments @{ Detailed = $true } | Out-Null
        $detailed = Get-RecordedInvocation -Root $root
        Assert-True ($detailed -notmatch '/q') "-Detailed still requested quiet mode. Got '$detailed'."
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'Extra build arguments are forwarded' {
    Reset-StubBehavior
    $root = New-StubRepo -Initialized
    try {
        Invoke-Wrapper -Root $root -Arguments @{ BuildArguments = @('/c', '/b') } | Out-Null
        $invocation = Get-RecordedInvocation -Root $root
        Assert-True ($invocation -match '/c') "'/c' was not forwarded. Got '$invocation'."
        Assert-True ($invocation -match '/b') "'/b' was not forwarded. Got '$invocation'."
    }
    finally { Remove-StubRepo $root }
}

Test-Case 'A directory that is not a repository is reported clearly' {
    Reset-StubBehavior
    $root = Join-Path ([IO.Path]::GetTempPath()) "notarepo$([guid]::NewGuid().Guid.Substring(0,8))"
    New-Item -ItemType Directory -Path $root -Force | Out-Null
    try {
        $result = Invoke-Wrapper -Root $root
        Assert-Equal 1 $result.ExitCode 'A missing build script did not fail the command.'
        Assert-True ($result.Output -match 'not found') 'The missing script was not named.'
    }
    finally { Remove-StubRepo $root }
}

# --- Public guidance -------------------------------------------------------------------------

Test-Case 'AGENTS.md documents the wrapper and the unreliable exit code' {
    $agents = Join-Path $repoRoot 'AGENTS.md'
    Assert-True (Test-Path -LiteralPath $agents) 'AGENTS.md is missing from the repository root.'
    $content = Get-Content -LiteralPath $agents -Raw
    Assert-True ($content -match 'Invoke-AgentBuild\.ps1') 'AGENTS.md does not mention the build wrapper.'
    Assert-True ($content -match 'exit with code') 'AGENTS.md does not warn about the unreliable exit code.'
}

Test-Case 'Copilot instructions point at AGENTS.md and the build skill' {
    $instructions = Join-Path $repoRoot '.github\copilot-instructions.md'
    Assert-True (Test-Path -LiteralPath $instructions) 'copilot-instructions.md is missing.'
    $content = Get-Content -LiteralPath $instructions -Raw
    Assert-True ($content -match 'AGENTS\.md') 'copilot-instructions.md does not reference AGENTS.md.'
    Assert-True ($content -match 'skills/build/SKILL\.md') 'copilot-instructions.md does not reference the build skill.'
}

Test-Case 'Public guidance contains no internal feed or credential instructions' {
    $files = @(
        (Join-Path $repoRoot 'AGENTS.md'),
        (Join-Path $repoRoot '.github\copilot-instructions.md'),
        (Join-Path $repoRoot '.github\skills\build\SKILL.md')
    )
    foreach ($file in $files) {
        $content = Get-Content -LiteralPath $file -Raw
        foreach ($term in @('pkgs.visualstudio.com', 'personal access token', 'install-bt')) {
            if ($content -match [regex]::Escape($term)) {
                throw "$(Split-Path $file -Leaf) references internal-only '$term'."
            }
        }
    }
}

Test-Case 'Guidance does not reference script options that were never added' {
    # -EnsureInitialized belonged to an earlier approach that modified initrun.ps1.
    # Those script changes are no longer made, so the option must not be documented.
    $files = @(
        (Join-Path $repoRoot 'AGENTS.md'),
        (Join-Path $repoRoot '.github\copilot-instructions.md'),
        (Join-Path $repoRoot '.github\skills\build\SKILL.md')
    )
    foreach ($file in $files) {
        $content = Get-Content -LiteralPath $file -Raw
        if ($content -match 'EnsureInitialized') {
            throw "$(Split-Path $file -Leaf) documents -EnsureInitialized, which does not exist."
        }
    }
}

Test-Case 'The wrapper never writes to the repository build scripts' {
    $wrapperContent = Get-Content -LiteralPath $wrapper -Raw
    foreach ($guard in @('Set-Content', 'Out-File', 'Add-Content')) {
        if ($wrapperContent -match "$guard[^\r\n]*(Build\.cmd|init\.cmd|initrun\.ps1)") {
            throw "The wrapper writes to a repository build script using $guard."
        }
    }
}

# --- Summary -----------------------------------------------------------------------------------

Write-Host ''
Write-Host "Passed: $script:Passed" -ForegroundColor Green
if ($script:Failed -gt 0) {
    Write-Host "Failed: $script:Failed" -ForegroundColor Red
    Write-Host ''
    foreach ($failure in $script:Failures) { Write-Host "  $failure" -ForegroundColor Red }
    exit 1
}

Write-Host 'All tests passed.' -ForegroundColor Green
exit 0

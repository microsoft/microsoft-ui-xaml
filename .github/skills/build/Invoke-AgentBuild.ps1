<#
.SYNOPSIS
    Build the WinUI repository unattended, with a trustworthy exit code.

.DESCRIPTION
    Wraps the repository's existing build entry points so an agent can run a build with a
    single command and rely on the result. It does not modify Build.cmd, init.cmd,
    initrun.ps1 or scripts\init\*; it only calls them.

    It adds three things those scripts do not provide to an unattended caller:

      1. First-time initialization. A full init is run automatically when the repository
         has not been initialized yet, using the same signal init.cmd /envcheck uses
         (the packages and .tools directories).

      2. A trustworthy exit code. Build.cmd can return 0 for a failed build, because the
         failing code is not preserved on the way out. This script ignores that exit code
         and derives the result from the failure markers Build.cmd prints and from whether
         a binary log was produced.

      3. The binary log location, reported on success as well as on failure, so there is
         always a diagnostic artifact to inspect.

.PARAMETER Target
    Build target passed to Build.cmd: prodtest (default), product, mux, test, samples.

.PARAMETER Flavor
    Build flavor. Default: amd64chk.
    One of: amd64chk, amd64fre, x86chk, x86fre, arm64chk, arm64fre.

.PARAMETER BuildArguments
    Additional flags forwarded verbatim to Build.cmd, for example /c or /b.

.PARAMETER Detailed
    Show full build output. By default the build runs quietly (/q, errors only).

.PARAMETER SkipInitialize
    Do not run a full initialization even when the repository looks uninitialized.

.PARAMETER RepoRoot
    Repository root. Defaults to the root inferred from this script's location.

.EXAMPLE
    .\.github\skills\build\Invoke-AgentBuild.ps1
    Full default build, initializing first if required.

.EXAMPLE
    .\.github\skills\build\Invoke-AgentBuild.ps1 -Target mux
    Build Microsoft.UI.Xaml.dll only.

.OUTPUTS
    Exit code 0 only when the build genuinely succeeded; non-zero otherwise.
#>
[CmdletBinding()]
param(
    [ValidateSet('prodtest', 'product', 'mux', 'test', 'samples')]
    [string]$Target = 'prodtest',

    [ValidateSet('amd64chk', 'amd64fre', 'x86chk', 'x86fre', 'arm64chk', 'arm64fre')]
    [Alias('i')]
    [string]$Flavor = 'amd64chk',

    [string[]]$BuildArguments = @(),

    [switch]$Detailed,

    [switch]$SkipInitialize,

    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
}
$RepoRoot = $RepoRoot.TrimEnd('\')

# Build.cmd prints these when a step fails. They are the only reliable failure signal,
# because the process exit code does not survive the end of the script.
$script:FailureMarkers = @(
    'ERROR: buildSolution for ',
    'ERROR: callScript FAILED',
    'ERROR: init.cmd ',
    'Please run init.cmd or use /i',
    'Could not set up a developer command prompt',
    'Could not find path: '
)

# Compiler, linker and MSBuild diagnostics, e.g. "error MSB4217:" or "error C3859:".
$script:ErrorPattern = '\berror\s+(MSB|C|LNK|CS|CVT|RC|AL)\d+\b'

# A build can fail for missing packages or tools even though the initialization check
# passed, because that check only confirms the two directories exist and not that their
# contents are complete or current. These patterns identify that case so the caller is
# told to initialize again rather than treating it as a code error.
$script:NeedsInitPatterns = @(
    'references NuGet package\(s\) that are missing',
    'The missing file is packages\\',
    'Unable to find package',
    '\berror\s+NU\d{4}\b',
    '\bMSB3644\b',
    "Could not find .*\\\.tools\\",
    'is not recognized as an internal or external command'
)

function Test-NeedsInit {
    param([string[]]$Lines)

    foreach ($line in $Lines) {
        foreach ($pattern in $script:NeedsInitPatterns) {
            if ($line -match $pattern) { return $true }
        }
    }
    return $false
}

function Get-BinaryLog {
    param([string]$Root, [datetime]$Since)

    $outputDir = Join-Path $Root 'BuildOutput'
    if (-not (Test-Path -LiteralPath $outputDir)) { return @() }

    return @(Get-ChildItem -LiteralPath $outputDir -Filter '*.binlog' -ErrorAction SilentlyContinue |
        Where-Object { $_.LastWriteTime -ge $Since } |
        Sort-Object LastWriteTime -Descending)
}

function Test-Initialized {
    param([string]$Root)

    # The same two directories init.cmd /envcheck requires before it will set up an
    # environment without restoring. Keep this in step with init.cmd.
    return (Test-Path -LiteralPath (Join-Path $Root 'packages')) -and
           (Test-Path -LiteralPath (Join-Path $Root '.tools'))
}

function Invoke-Captured {
    <#
        Runs a command, streams its output to the host, and returns the captured lines.
        Output is needed in full because the build result is derived from it.
    #>
    param(
        [string]$FilePath,
        [string[]]$ArgumentList
    )

    $lines = New-Object System.Collections.Generic.List[string]
    & $FilePath @ArgumentList 2>&1 | ForEach-Object {
        $line = [string]$_
        $lines.Add($line)
        Write-Host $line
    }
    return , $lines.ToArray()
}

function Get-WindowsPowerShell {
    $candidate = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
    if (Test-Path -LiteralPath $candidate) { return $candidate }

    foreach ($name in 'powershell.exe', 'pwsh.exe') {
        $found = Get-Command $name -ErrorAction SilentlyContinue
        if ($found) { return $found.Source }
    }
    throw 'No PowerShell host found to run the build.'
}

function Test-FailureInOutput {
    param([string[]]$Lines)

    foreach ($line in $Lines) {
        foreach ($marker in $script:FailureMarkers) {
            if ($line -like "*$marker*") { return $true }
        }
        if ($line -match $script:ErrorPattern) { return $true }
    }
    return $false
}

$initCmd = Join-Path $RepoRoot 'init.cmd'
$initRun = Join-Path $RepoRoot 'initrun.ps1'
$buildCmd = Join-Path $RepoRoot 'Build.cmd'

foreach ($required in @($initCmd, $initRun, $buildCmd)) {
    if (-not (Test-Path -LiteralPath $required)) {
        Write-Host "ERROR: Expected build script not found: $required" -ForegroundColor Red
        Write-Host "       Is '$RepoRoot' a WinUI repository root?" -ForegroundColor Red
        exit 1
    }
}

$powershell = Get-WindowsPowerShell
$startedAt = Get-Date

# --- Step 1: initialize once, if needed -------------------------------------------------

if (-not (Test-Initialized -Root $RepoRoot)) {
    if ($SkipInitialize) {
        Write-Host 'ERROR: The repository is not initialized and -SkipInitialize was specified.' -ForegroundColor Red
        Write-Host "       Run: $initCmd $Flavor" -ForegroundColor Red
        exit 1
    }

    Write-Host "Initializing the build environment for $Flavor. The first run downloads tools and restores packages."

    $initOutput = @(Invoke-Captured -FilePath $env:ComSpec -ArgumentList @('/c', "`"$initCmd`" $Flavor"))
    $initExit = $LASTEXITCODE

    # init.cmd reports a bad flavor or a missing path through its exit code, but a failed
    # restore can still leave the repository unusable, so check the outcome as well.
    if ($initExit -ne 0 -or (Test-FailureInOutput -Lines $initOutput) -or -not (Test-Initialized -Root $RepoRoot)) {
        Write-Host '---' -ForegroundColor Red
        Write-Host "ERROR: Initialization failed for $Flavor (exit code $initExit)." -ForegroundColor Red
        Write-Host '       Not starting the build: a build on an uninitialized repository fails for unrelated reasons.' -ForegroundColor Red
        exit 1
    }

    Write-Host "Initialized environment for $Flavor."
}

# --- Step 2: build ----------------------------------------------------------------------

$buildArgs = @()
if (-not $Detailed) { $buildArgs += '/q' }
if ($Target -ne 'prodtest') { $buildArgs += $Target }
$buildArgs += $BuildArguments

$initRunArgs = @(
    '-NoProfile', '-ExecutionPolicy', 'Bypass',
    '-File', $initRun,
    '-Flavor', $Flavor,
    'build.cmd'
) + $buildArgs

Write-Host "Building $Target ($Flavor). A full build takes over an hour on a clean repository."

$buildOutput = @(Invoke-Captured -FilePath $powershell -ArgumentList $initRunArgs)
$reportedExit = $LASTEXITCODE

# --- Step 3: decide the real result ------------------------------------------------------

# Wrap in @() so a single result is still an array once returned from the function.
$binlogs = @(Get-BinaryLog -Root $RepoRoot -Since $startedAt)
$isFake = $buildArgs -contains '/fake'

$failed = $false
$reason = $null

# A restore problem is reported without an MSBuild error code, so it has to be checked
# separately or a build that exits 0 after failing to find its packages looks successful.
$needsInit = Test-NeedsInit -Lines $buildOutput

if (Test-FailureInOutput -Lines $buildOutput) {
    $failed = $true
    $reason = 'the build reported errors'
}
elseif ($needsInit) {
    $failed = $true
    $reason = 'the build could not find restored packages or tools'
}
elseif ($reportedExit -ne 0) {
    $failed = $true
    $reason = "the build returned exit code $reportedExit"
}
elseif (-not $isFake -and $binlogs.Count -eq 0) {
    # No errors and no log. MSBuild crashes (for example 0xC0000005) are not caught by
    # Build.cmd's own error checks, so an absent log is the only remaining signal.
    $failed = $true
    $reason = 'no binary log was produced, which usually means the build crashed before it started'
}

Write-Host '---'
if ($binlogs.Count -gt 0) {
    Write-Host 'Binary logs:'
    foreach ($log in $binlogs) { Write-Host "  $($log.FullName)" }
}
elseif (-not $isFake) {
    Write-Host "Binary logs: none found under $(Join-Path $RepoRoot 'BuildOutput')"
}

if ($failed) {
    Write-Host "BUILD FAILED: $reason." -ForegroundColor Red

    if ($needsInit) {
        Write-Host ''
        Write-Host 'This looks like missing packages or tools rather than a code error.' -ForegroundColor Yellow
        Write-Host 'The initialization check only confirms that packages\ and .tools\ exist, so a' -ForegroundColor Yellow
        Write-Host 'partial or out-of-date restore passes it. Initialize again before changing code:' -ForegroundColor Yellow
        Write-Host "    $initCmd $Flavor" -ForegroundColor Yellow
    }

    exit 1
}

Write-Host "BUILD SUCCEEDED: $Target ($Flavor)." -ForegroundColor Green
exit 0

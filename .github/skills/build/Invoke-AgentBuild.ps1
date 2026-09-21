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

.PARAMETER SkipMachineSetup
    Do not install Visual Studio or its missing components, and build with the machine as
    it is. The build may fail for missing components.

.PARAMETER SetupMachineOnly
    Perform the machine setup and exit without building. Used to run the setup step
    elevated; can also be run directly from an elevated prompt.

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

    [switch]$SkipMachineSetup,

    [switch]$SetupMachineOnly,

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

# Initialization installs Visual Studio build tools and enables long path support, and
# both need elevation. Neither failure stops init: the installer's 5007 ("could not make
# changes") is counted as a success by scripts\MSBuildFunctions.psm1, so a run that
# installed nothing reports that it worked. The build then fails much later, for missing
# components, in a way that reads as broken source code.
$script:SetupIncompletePatterns = @(
    'could not update build tools',
    'Error enabling long path support'
)

# A build can fail for missing packages or tools even though the initialization check
# passed, because that check only confirms the two directories exist and not that their
# contents are complete or current. These patterns identify that case so the caller is
# told to initialize again rather than treating it as a code error.
#
# Every pattern here has to name a restore or tool output specifically. The build scripts
# probe for optional tools as a normal part of choosing a toolchain, and a failed probe
# prints ordinary shell errors such as "is not recognized as an internal or external
# command". Matching those failed a complete build that produced every binary log and
# reported no diagnostic.
$script:NeedsInitPatterns = @(
    'references NuGet package\(s\) that are missing',
    'The missing file is packages\\',
    'Unable to find package',
    '\berror\s+NU\d{4}\b',
    '\bMSB3644\b',
    "Could not find .*\\\.tools\\"
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

function Repair-GitConfigEnvironment {
    <#
        Git reads configuration from GIT_CONFIG_COUNT together with the
        GIT_CONFIG_KEY_<n> / GIT_CONFIG_VALUE_<n> pairs. Some agent hosts inject a pair
        whose value is empty. An empty variable is dropped when the environment crosses
        into the build's cmd.exe, so git sees a key it has no value for, rejects its own
        configuration, and exits 128. The build calls git, so every project that does
        fails for a reason that has nothing to do with this repository.

        Only a provably incomplete set is cleared, because a key declared without a value
        is never valid, and only in this process.
    #>
    $countText = [Environment]::GetEnvironmentVariable('GIT_CONFIG_COUNT')
    $count = 0
    if (-not [int]::TryParse($countText, [ref]$count) -or $count -le 0) { return }

    $incomplete = @()
    for ($i = 0; $i -lt $count; $i++) {
        $key = [Environment]::GetEnvironmentVariable("GIT_CONFIG_KEY_$i")
        $value = [Environment]::GetEnvironmentVariable("GIT_CONFIG_VALUE_$i")
        if (-not [string]::IsNullOrEmpty($key) -and [string]::IsNullOrEmpty($value)) {
            $incomplete += $key
        }
    }
    if ($incomplete.Count -eq 0) { return }

    Get-ChildItem Env: |
        Where-Object { $_.Name -like 'GIT_CONFIG*' } |
        Remove-Item -ErrorAction SilentlyContinue

    Write-Host ("Cleared the inherited GIT_CONFIG_* variables for this build: " +
        ($incomplete -join ', ') + " declared no value, which makes git exit 128.")
}

function Test-Elevated {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-LongPathsEnabled {
    # The same registry value scripts\init\Initialize-CheckLongPathSupport.ps1 reads.
    try {
        $key = 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem'
        return (Get-ItemProperty -LiteralPath $key -Name 'LongPathsEnabled' -ErrorAction Stop).LongPathsEnabled -eq 1
    }
    catch { return $false }
}

function Get-VisualStudioPath {
    <#
        The installation path of a Visual Studio that can build this repository, or $null.
        vswhere ships with the Visual Studio Installer. VS 17 and earlier install it under
        Program Files (x86); VS 18 installs under Program Files, so look in both.
    #>
    foreach ($programFiles in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
        if (-not $programFiles) { continue }
        $vswhere = Join-Path $programFiles 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (-not (Test-Path -LiteralPath $vswhere)) { continue }

        $found = @(& $vswhere -products '*' -latest -requires 'Microsoft.Component.MSBuild' -property 'installationPath' 2>$null)
        if ($found.Count -gt 0 -and $found[0]) { return $found[0] }
    }
    return $null
}

function Get-VisualStudioInstaller {
    foreach ($programFiles in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
        if (-not $programFiles) { continue }
        $installer = Join-Path $programFiles 'Microsoft Visual Studio\Installer\vs_installer.exe'
        if (Test-Path -LiteralPath $installer) { return $installer }
    }
    return $null
}

function Get-RunningVisualStudioInstaller {
    <#
        The Visual Studio Installer processes already running, as "name (PID n)" strings.
        The installer is a singleton. A second instance started while one is open exits 0
        without installing anything, so an unattended setup that ignores this reports
        success and the build then fails for components nobody installed.
    #>
    $filter = "Name='setup.exe' or Name='vs_installer.exe' or Name='vs_installershell.exe'"
    try { $processes = @(Get-CimInstance Win32_Process -Filter $filter -ErrorAction Stop) }
    catch { return @() }

    return @($processes |
        Where-Object { $_.ExecutablePath -and $_.ExecutablePath -like '*Microsoft Visual Studio*Installer*' } |
        ForEach-Object { "$($_.Name) (PID $($_.ProcessId))" })
}

function Get-MissingVisualStudioComponent {
    <#
        The components listed in the repository's .vsconfig that the installation does not
        have. init only ever probes for ATL/ARM64 and repairs against .vsconfig_buildtools,
        so an installation missing anything else is found only here.
    #>
    param([string]$Root, [string]$InstallPath)

    $configPath = Join-Path $Root '.vsconfig'
    if (-not (Test-Path -LiteralPath $configPath)) { return @() }

    $vswhere = $null
    foreach ($programFiles in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
        if (-not $programFiles) { continue }
        $candidate = Join-Path $programFiles 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (Test-Path -LiteralPath $candidate) { $vswhere = $candidate; break }
    }
    if (-not $vswhere) { return @() }

    try { $components = @((Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json).components) }
    catch { return @() }

    $missing = @()
    foreach ($component in $components) {
        if (-not $component) { continue }
        $has = @(& $vswhere -path $InstallPath -products '*' -requires $component -property 'installationPath' 2>$null)
        if ($has.Count -eq 0 -or -not $has[0]) { $missing += $component }
    }
    return $missing
}

function Get-MachineSetupWork {
    <#
        What setup this machine still needs before it can build, as a list of descriptions.
        Empty means the machine is ready.
    #>
    param([string]$Root)

    $work = @()
    if (-not (Test-LongPathsEnabled)) { $work += 'enable long path support' }

    $vs = Get-VisualStudioPath
    if (-not $vs) {
        $work += 'install Visual Studio with the components in .vsconfig'
    }
    else {
        $missing = @(Get-MissingVisualStudioComponent -Root $Root -InstallPath $vs)
        if ($missing.Count -gt 0) {
            $work += "add $($missing.Count) missing Visual Studio component(s): $($missing -join ', ')"
        }
    }
    return $work
}

function Install-MachineSetup {
    <#
        Performs the machine setup itself. Must run elevated: the Visual Studio Installer
        refuses to change an installation without it, and reports 5007 rather than failing,
        and the long path setting lives under HKLM.
    #>
    param([string]$Root)

    if (-not (Test-LongPathsEnabled)) {
        Write-Host 'Enabling long path support.'
        $key = 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem'
        New-ItemProperty -LiteralPath $key -Name 'LongPathsEnabled' -Value 1 -PropertyType DWord -Force | Out-Null
    }

    $config = Join-Path $Root '.vsconfig'
    if (-not (Test-Path -LiteralPath $config)) {
        Write-Host 'No .vsconfig in the repository root; leaving the Visual Studio installation alone.'
        return
    }

    $vs = Get-VisualStudioPath
    if ($vs) {
        $missing = @(Get-MissingVisualStudioComponent -Root $Root -InstallPath $vs)
        if ($missing.Count -eq 0) { return }
    }

    # Checked before anything is started, so the singleton lock is reported as the cause
    # rather than showing up later as an install that claimed to work and did nothing.
    $running = @(Get-RunningVisualStudioInstaller)
    if ($running.Count -gt 0) {
        throw ("A Visual Studio Installer is already running: $($running -join ', '). " +
            'It holds a lock that makes an unattended install exit successfully without ' +
            'installing anything. Close it, then run this again.')
    }

    if ($vs) {
        $installer = Get-VisualStudioInstaller
        if (-not $installer) { throw 'Visual Studio is installed but vs_installer.exe was not found.' }

        Write-Host "Adding $($missing.Count) missing component(s) to $vs. This can take several minutes."
        $arguments = @('modify', '--installPath', $vs, '--config', $config, '--quiet', '--norestart', '--force', '--wait')
    }
    else {
        # The same bootstrapper scripts\MSBuildFunctions.psm1 uses, with the repository's
        # own component list rather than the build tools subset.
        $bootstrapper = Join-Path $env:TEMP 'vs_community.exe'
        Write-Host 'No Visual Studio found. Downloading the installer.'
        Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vs_community.exe' -OutFile $bootstrapper

        Write-Host 'Installing Visual Studio with the components in .vsconfig. This takes a while.'
        $installer = $bootstrapper
        $arguments = @('--config', $config, '--quiet', '--norestart', '--wait')
    }

    $process = Start-Process -FilePath $installer -ArgumentList $arguments -Wait -PassThru
    $code = $process.ExitCode

    # 3010 asks for a reboot we do not need. 5007 means the installer changed nothing,
    # which scripts\MSBuildFunctions.psm1 treats as success and must not be treated as one.
    if ($code -eq 5007) { throw 'The Visual Studio Installer could not make changes (5007). Setup was not applied.' }
    if ($code -ne 0 -and $code -ne 3010) { throw "The Visual Studio Installer failed with exit code $code." }

    # Exit code 0 is not proof the work happened: an instance that loses the singleton race
    # exits 0 having installed nothing. Confirm against the machine before reporting success,
    # so the failure is named here rather than surfacing later as a missing compiler.
    $remaining = @(Get-MachineSetupWork -Root $Root)
    if ($remaining.Count -gt 0) {
        $message = "The Visual Studio Installer reported success but the machine still needs: $($remaining -join '; ')."
        if ($code -eq 3010) {
            $message += ' The installer asked for a reboot. Restart the machine, then run this again.'
        }
        else {
            $stillRunning = @(Get-RunningVisualStudioInstaller)
            if ($stillRunning.Count -gt 0) {
                $message += " A Visual Studio Installer is still running: $($stillRunning -join ', ')." +
                    ' Close it, then run this again.'
            }
        }
        throw $message
    }

    Write-Host 'Visual Studio setup complete.'
}

function Initialize-Machine {
    <#
        Brings the machine up to what the build needs, installing Visual Studio and its
        missing components when they are absent, so a clean machine needs no manual setup.

        The installer requires administrator rights, which cannot be granted from inside an
        unelevated process, so this relaunches itself elevated for the setup step only. On
        an interactive desktop that is one consent prompt; where no consent can be given the
        setup is reported as needed rather than failing later for missing components.
    #>
    param([string]$Root)

    # Only meaningful for a repository whose init does machine setup. Keyed off the setup
    # scripts themselves so the wrapper behaves the same way with or without a test hook.
    $setupScripts = @(
        (Join-Path $Root 'scripts\init\Initialize-InstallMSBuild.ps1'),
        (Join-Path $Root 'scripts\init\Initialize-CheckLongPathSupport.ps1')
    )
    if (-not ($setupScripts | Where-Object { Test-Path -LiteralPath $_ })) { return }

    $work = @(Get-MachineSetupWork -Root $Root)
    if ($work.Count -eq 0) {
        Write-Host 'Machine setup: Visual Studio and long path support are already in place.'
        return
    }

    Write-Host "Machine setup required: $($work -join '; ')."

    if ($SkipMachineSetup) {
        Write-Host 'Skipping machine setup as requested.' -ForegroundColor Yellow
    }
    elseif (Test-Elevated) {
        Install-MachineSetup -Root $Root
    }
    else {
        Write-Host 'Requesting administrator rights to perform it. Approve the prompt if one appears.'
        $arguments = @(
            '-NoProfile', '-ExecutionPolicy', 'Bypass',
            '-File', $PSCommandPath,
            '-SetupMachineOnly',
            '-RepoRoot', $Root
        )
        try {
            $elevatedRun = Start-Process -FilePath (Get-WindowsPowerShell) -ArgumentList $arguments -Verb RunAs -Wait -PassThru
            $elevatedCode = $elevatedRun.ExitCode
        }
        catch {
            $elevatedCode = -1
        }

        if ($elevatedCode -ne 0) {
            Write-Host '---' -ForegroundColor Red
            Write-Host 'ERROR: The machine setup could not be performed.' -ForegroundColor Red
            Write-Host "       Still required: $($work -join '; ')." -ForegroundColor Red
            Write-Host '       Elevation was refused or unavailable. Run this script once from an' -ForegroundColor Red
            Write-Host '       elevated PowerShell, or run it with -SkipMachineSetup to build anyway.' -ForegroundColor Red
            Write-Host '       Not starting the build: it would fail later for missing components,' -ForegroundColor Red
            Write-Host '       with errors that read as broken source code.' -ForegroundColor Red
            exit 1
        }
    }

    $remaining = @(Get-MachineSetupWork -Root $Root)
    if ($remaining.Count -gt 0 -and -not $SkipMachineSetup) {
        Write-Host "WARNING: Setup ran but the machine still needs: $($remaining -join '; ')." -ForegroundColor Yellow
    }
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

    # Tools in the build chain write progress to stderr even when they succeed, and
    # 'Stop' turns a redirected stderr line into a terminating error. This assignment is
    # function scoped, so the caller's preference is unaffected.
    $ErrorActionPreference = 'Continue'

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

function Test-SetupIncomplete {
    param([string[]]$Lines)

    foreach ($line in $Lines) {
        foreach ($pattern in $script:SetupIncompletePatterns) {
            if ($line -match $pattern) { return $true }
        }
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

# The build shells out to git. Clear an inherited git configuration that cannot work
# before anything runs, so init and build are not failed by the host's environment.
Repair-GitConfigEnvironment

# Running elevated purely to set the machine up: do that and stop.
if ($SetupMachineOnly) {
    try {
        Install-MachineSetup -Root $RepoRoot
        exit 0
    }
    catch {
        Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
}

# Install Visual Studio and any components .vsconfig asks for that are missing, so a
# machine with none of them can build without anyone setting it up by hand.
Initialize-Machine -Root $RepoRoot

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

    # The build tools installer reports 5007 when it could not make changes, and that is
    # counted as a success, so this is the only signal that setup did nothing.
    if (Test-SetupIncomplete -Lines $initOutput) {
        Write-Host '---' -ForegroundColor Red
        Write-Host 'ERROR: Initialization could not complete the machine setup it attempted.' -ForegroundColor Red
        Write-Host '       Build tools or long path support were left unchanged because the' -ForegroundColor Red
        Write-Host '       installer was not allowed to make changes. Rerun init.cmd from an' -ForegroundColor Red
        Write-Host '       elevated PowerShell, then build again.' -ForegroundColor Red
        Write-Host '       Not starting the build: it would fail later for missing components.' -ForegroundColor Red
        exit 1
    }

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

<#
.SYNOPSIS
    Run a command in an initialized WinUI build environment.

.DESCRIPTION
    Calls init.ps1 with the specified flavor (default: amd64chk), then executes the
    given command with all environment variables and PATH entries set. This eliminates
    the need to persist a shell session after running init.

.PARAMETER Flavor
    Build flavor. Default: amd64chk.
    Examples: amd64chk, amd64fre, x86chk, x86fre, arm64chk, arm64fre

.PARAMETER EnsureInitialized
    Run the full one-time initialization automatically when the lightweight
    environment check reports that required tools or packages are missing.

.PARAMETER Command
    The command and arguments to run after initialization.

.EXAMPLE
    .\initrun.ps1 -EnsureInitialized build.cmd /q
    .\initrun.ps1 -EnsureInitialized build.cmd mux /q
    .\initrun.ps1 msb "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj" /q
    .\initrun.ps1 -EnsureInitialized -Flavor arm64fre build.cmd mux /q
    .\initrun.ps1 run-tests-vm.ps1 *CommandBar*
#>
param(
    [Alias("i")]
    [string]$Flavor = "amd64chk",

    [switch]$EnsureInitialized,

    [Parameter(Position = 0, ValueFromRemainingArguments = $true)]
    [string[]]$Command
)

$ErrorActionPreference = "Stop"
$scriptDir = $PSScriptRoot

if (-not $Command -or $Command.Count -eq 0) {
    Get-Help $MyInvocation.MyCommand.Path -Detailed
    exit 1
}

# Run init with /envcheck to set up environment variables and PATH.
# This skips NuGet restore when the one-time setup has already completed.
$initFailure = $null
try {
    & (Join-Path $scriptDir "init.ps1") $Flavor /envcheck /notitle
    if ($LASTEXITCODE -ne 0) {
        throw "init.ps1 exited with code $LASTEXITCODE"
    }
} catch {
    $initFailure = $_
}

if ($initFailure) {
    if (-not $EnsureInitialized) {
        Write-Host "ERROR: init.ps1 $Flavor /envcheck failed: $initFailure" -ForegroundColor Red
        Write-Host "Run a full init first: .\init.ps1 $Flavor" -ForegroundColor Red
        exit 1
    }

    Write-Host "Build environment is not initialized. Running one-time setup for $Flavor..."
    try {
        & (Join-Path $scriptDir "init.ps1") $Flavor /notitle
        if ($LASTEXITCODE -ne 0) {
            throw "init.ps1 exited with code $LASTEXITCODE"
        }
    } catch {
        Write-Host "ERROR: Automatic initialization for $Flavor failed: $_" -ForegroundColor Red
        exit 1
    }
}

# Execute the command
$cmd = $Command[0]
$cmdArgs = @()
if ($Command.Count -gt 1) {
    $cmdArgs = $Command[1..($Command.Count - 1)]
}

# Resolve repo-relative commands before PATH so initrun works from any directory.
$repoCommand = if ([IO.Path]::IsPathRooted($cmd)) { $cmd } else { Join-Path $scriptDir $cmd }
if (Test-Path $repoCommand) {
    $cmd = (Resolve-Path $repoCommand).Path
} else {
    $resolved = Get-Command $cmd -ErrorAction SilentlyContinue
    if (-not $resolved) {
        Write-Host "ERROR: Command not found: $cmd" -ForegroundColor Red
        exit 1
    }
    $cmd = $resolved.Source
}

# The call operator preserves argument boundaries without reparsing user input.
$global:LASTEXITCODE = 0
& $cmd @cmdArgs
exit $LASTEXITCODE

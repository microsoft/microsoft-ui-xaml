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

.PARAMETER Command
    The command and arguments to run after initialization.

.EXAMPLE
    .\initrun.ps1 build.cmd mux /q
    .\initrun.ps1 msb "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj" /q
    .\initrun.ps1 -Flavor arm64fre build.cmd mux /q
    .\initrun.ps1 run-tests-vm.ps1 *CommandBar*
#>
param(
    [Alias("i")]
    [string]$Flavor = "amd64chk",

    [Parameter(Position = 0, ValueFromRemainingArguments = $true)]
    [string[]]$Command
)

$ErrorActionPreference = "Stop"
$scriptDir = $PSScriptRoot

if (-not $Command -or $Command.Count -eq 0) {
    Get-Help $MyInvocation.MyCommand.Path -Detailed
    exit 1
}

# Run init with /envcheck to set up environment variables and PATH
# This skips NuGet restore — a full init must have been run previously.
try {
    & (Join-Path $scriptDir "init.ps1") $Flavor /envcheck /notitle
} catch {
    Write-Host "ERROR: init.ps1 $Flavor /envcheck failed: $_" -ForegroundColor Red
    Write-Host "Run a full init first: .\init.ps1 $Flavor" -ForegroundColor Red
    exit 1
}

# Execute the command
$cmd = $Command[0]
$cmdArgs = @()
if ($Command.Count -gt 1) {
    $cmdArgs = $Command[1..($Command.Count - 1)]
}

# Resolve the command — check PATH and current directory
$resolved = Get-Command $cmd -ErrorAction SilentlyContinue
if ($resolved) {
    $cmd = $resolved.Source
} elseif (Test-Path (Join-Path $scriptDir $cmd)) {
    $cmd = Join-Path $scriptDir $cmd
}

# Use Invoke-Expression to preserve named parameters like -IPAddress
$escapedArgs = $cmdArgs | ForEach-Object {
    if ($_ -match '^-') { $_ }           # pass switches/named params as-is
    elseif ($_ -match '\s') { "'$_'" }   # quote args with spaces
    else { $_ }
}
$fullCmd = "& '$cmd' $($escapedArgs -join ' ')"
Invoke-Expression $fullCmd
exit $LASTEXITCODE

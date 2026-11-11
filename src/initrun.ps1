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
    .\initrun.ps1 build.cmd /q mux
    .\initrun.ps1 msb /q "dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj"
    .\initrun.ps1 -Flavor arm64fre build.cmd /q mux
    .\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName MyVM *CommandBar*
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
Remove-Item Env:\EnvironmentInitialized -ErrorAction SilentlyContinue
try {
    & (Join-Path $scriptDir "init.ps1") $Flavor /envcheck /notitle
} catch {
    throw "init.ps1 $Flavor /envcheck failed: $_"
}
if ($env:EnvironmentInitialized -ne "1") {
    throw "init.ps1 $Flavor /envcheck did not complete. Run a full init first: .\init.ps1 $Flavor"
}

# Execute the command
$cmd = $Command[0]
$cmdArgs = @()
if ($Command.Count -gt 1) {
    $cmdArgs = $Command[1..($Command.Count - 1)]
}

# Resolve relative commands from this checkout before searching PATH or the caller's directory.
$repoCommand = if (-not [System.IO.Path]::IsPathRooted($cmd)) { Join-Path $scriptDir $cmd }
if ($repoCommand -and (Test-Path -LiteralPath $repoCommand -PathType Leaf)) {
    $cmd = (Resolve-Path -LiteralPath $repoCommand).ProviderPath
} else {
    $resolved = Get-Command $cmd -ErrorAction SilentlyContinue
    if (-not $resolved) { throw "Command not found: $cmd" }
    $cmd = $resolved.Source
}

$global:LASTEXITCODE = 0
if ([System.IO.Path]::GetExtension($cmd) -ieq ".ps1") {
    $powerShell = if ($PSVersionTable.PSEdition -eq "Core") { "pwsh.exe" } else { "powershell.exe" }
    & (Join-Path $PSHOME $powerShell) -NoProfile -File $cmd @cmdArgs
} else {
    & $cmd @cmdArgs
}
$commandSucceeded = $?
if (-not $commandSucceeded -and $LASTEXITCODE -eq 0) { exit 1 }
exit $LASTEXITCODE

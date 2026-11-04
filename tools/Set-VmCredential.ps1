<#
.SYNOPSIS
    Validate a Hyper-V test VM and cache its credential for the VM test tools, WITHOUT the password
    ever passing through a chat or agent session.

.DESCRIPTION
    Shared by tools\run-tests-on-vm.ps1 users and the repo skills that run tests on a VM (for
    example .github\skills\code-coverage-calculation).
    * Fails unless the Hyper-V VM exists and is Running.
    * Prompts for the password in a SEPARATE console window (username pre-filled), the same way
      tools\run-tests-on-vm.ps1 does, and writes the DPAPI-encrypted cache
      %USERPROFILE%\.winui-test\vmcred-<VM>.xml - the cache that run-tests-on-vm.ps1 reads.
    * Validates the login over PowerShell Direct and that the user has an Active desktop session
      (UI tests need one).
    * A newly entered credential replaces the existing cache only after ALL of these checks pass;
      otherwise the previous cache is left untouched.

.EXAMPLE
    .\tools\Set-VmCredential.ps1 -VMName MyTestVM -UserName TestUser
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$VMName,
    # Plain account name exactly as qwinsta shows it on the VM (no ".\" or "VM\" prefix): the
    # active-session check matches the session row by this name.
    [Parameter(Mandatory)][string]$UserName,
    # Re-prompt even if a cached credential for this VM/user exists.
    [switch]$Force
)
$ErrorActionPreference = 'Stop'

if ($UserName -match '\\') { throw "Use the plain account name without a domain/machine prefix (got '$UserName')." }
$vm = Get-VM -Name $VMName -ErrorAction SilentlyContinue
if (-not $vm) { throw "Hyper-V VM '$VMName' not found (or this account is not in Hyper-V Administrators)." }
if ($vm.State -ne 'Running') { throw "VM '$VMName' is $($vm.State). Start it and log on to its desktop first." }

$credDir = Join-Path $env:USERPROFILE '.winui-test'
New-Item -ItemType Directory -Force $credDir | Out-Null
$credFile = Join-Path $credDir "vmcred-$($VMName -replace '[^a-zA-Z0-9]','_').xml"
$newFile = "$credFile.new"
Remove-Item $newFile -ErrorAction SilentlyContinue

$cached = if (Test-Path $credFile) { Import-Clixml $credFile } else { $null }
if ($Force -or -not $cached -or $cached.UserName -ne $UserName) {
    $helper = Join-Path $credDir 'prompt-coverage-cred.ps1'
    @'
param([string]$VMName, [string]$UserName, [string]$CredFile)
Write-Host "Password for '$UserName' on VM '$VMName'" -ForegroundColor Cyan
$p = Read-Host 'Password' -AsSecureString
[pscredential]::new($UserName, $p) | Export-Clixml $CredFile
'@ | Set-Content $helper -Encoding UTF8
    Write-Host "A password prompt opened in a new window..." -ForegroundColor Cyan
    Start-Process powershell -Wait -ArgumentList '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $helper,
        '-VMName', $VMName, '-UserName', $UserName, '-CredFile', $newFile
    Remove-Item $helper -ErrorAction SilentlyContinue
    if (-not (Test-Path $newFile)) { throw 'No password was entered; cannot continue.' }
    $cred = Import-Clixml $newFile
} else {
    $cred = $cached
}

try {
    $s = New-PSSession -VMName $VMName -Credential $cred
} catch {
    Remove-Item $newFile -ErrorAction SilentlyContinue
    throw "Could not log on to '$VMName' as '$UserName': $($_.Exception.Message)"
}
try {
    $row = Invoke-Command -Session $s -ScriptBlock {
        param($u) (qwinsta 2>$null) | Where-Object { $_ -match "\s$([regex]::Escape($u))\s" } | Select-Object -First 1
    } -ArgumentList $UserName
} finally { Remove-PSSession $s }

if ("$row" -notmatch '\sActive') {
    # Keep the previous cache: a valid password for an account that is not logged on to the
    # desktop must not replace a working credential.
    Remove-Item $newFile -ErrorAction SilentlyContinue
    throw "'$UserName' has no Active desktop session on '$VMName' (session: '$("$row".Trim())'). Log on to the VM desktop as that user, then retry."
}

if (Test-Path $newFile) { Move-Item $newFile $credFile -Force }
[pscustomobject]@{
    VMName = $VMName; UserName = $UserName; CredentialCache = $credFile
    LogonOk = $true; DesktopSession = "$row".Trim(); DesktopActive = $true
}

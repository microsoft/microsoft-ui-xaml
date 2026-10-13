<#
.SYNOPSIS
    Gate for the coverage skill: validate the VM and cache its credential WITHOUT the password ever
    passing through the agent/chat.

.DESCRIPTION
    * Fails unless the Hyper-V VM exists and is Running.
    * Prompts for the password in a SEPARATE console window (username pre-filled), the same way
      tools\run-tests-on-vm.ps1 does, and writes the DPAPI-encrypted cache
      %USERPROFILE%\.winui-test\vmcred-<VM>.xml - the cache that run-tests-on-vm.ps1 and the
      Step3/Step4 scripts already read. (Step3 does not forward -Credential to run-tests-on-vm.ps1,
      so the cache must exist; passing -Credential alone is not enough.)
    * Validates the login over PowerShell Direct and that the user has an Active desktop session.
    * Only replaces an existing cache after the new credential has been validated.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$VMName,
    # Plain account name exactly as qwinsta shows it on the VM (no ".\" or "VM\" prefix):
    # Step3 matches the session row with the credential's UserName.
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

if (Test-Path $newFile) { Move-Item $newFile $credFile -Force }
$active = "$row" -match '\sActive'
[pscustomobject]@{
    VMName = $VMName; UserName = $UserName; CredentialCache = $credFile
    LogonOk = $true; DesktopSession = "$row".Trim(); DesktopActive = $active
}
if (-not $active) { throw "'$UserName' has no Active desktop session on '$VMName'. Log on to the VM desktop, then retry." }

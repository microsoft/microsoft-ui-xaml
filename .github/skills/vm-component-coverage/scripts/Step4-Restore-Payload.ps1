<#
.SYNOPSIS
    Step 4 - Restore the original (uninstrumented) module in the local payload and on the VM.

.DESCRIPTION
    Uses <WorkDir>\instrument-state.json written by Step 2:
      * copies every backed-up original back over its payload copy (hash-verified),
      * deletes the static_covrun*.dll files that Step 2 added,
      * optionally (-VMName) does the same inside the VM's deployed payload
        (C:\TestPayload\<HOSTNAME>-<repo-folder>, the path tools\run-tests-on-vm.ps1 uses) and
        throws if any VM copy does not match its original afterwards,
      * removes instrument-state.json only after everything was restored (on failure it is kept,
        so this script can simply be run again; backups are kept for audit).
    The deploy manifest of tools\run-tests-on-vm.ps1 is size+mtime based, so the next normal run
    re-deploys the restored files anyway; the VM step just makes the VM clean immediately.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RepoRoot,
    [Parameter(Mandatory)][string]$WorkDir,
    [string]$VMName,
    [PSCredential]$Credential
)
$ErrorActionPreference = 'Stop'
function Info($m) { Write-Host "[restore] $m" }

$statePath = Join-Path $WorkDir 'instrument-state.json'
if (-not (Test-Path $statePath)) { Info 'nothing to restore (no instrument-state.json)'; return }
$state = Get-Content $statePath -Raw | ConvertFrom-Json
$payload = $state.payloadDir

foreach ($c in $state.copies) {
    $dest = Join-Path $payload $c.relativePath
    Copy-Item $c.backup $dest -Force
    if ((Get-FileHash $dest).Hash -ne $c.originalHash) { throw "restore hash mismatch: $dest" }
    Info "restored $($c.relativePath)"
}
foreach ($r in $state.runtimesAdded) {
    $p = Join-Path $payload $r
    if (Test-Path $p) { Remove-Item $p -Force; Info "removed $r" }
}

if ($VMName) {
    if (-not $Credential) {
        $Credential = Import-Clixml (Join-Path $env:USERPROFILE ".winui-test\vmcred-$($VMName -replace '[^a-zA-Z0-9]','_').xml")
    }
    $remoteDir = "C:\TestPayload\$env:COMPUTERNAME-$(Split-Path $RepoRoot -Leaf)"
    $s = New-PSSession -VMName $VMName -Credential $Credential
    try {
        $vmErrors = @()
        Invoke-Command -Session $s -ScriptBlock { New-Item -ItemType Directory -Force C:\WinUICoverage | Out-Null }
        # Copies can come from different originals (e.g. an app folder with an older staged DLL),
        # so restore each distinct original to exactly the copies it was backed up from.
        foreach ($g in ($state.copies | Group-Object originalHash)) {
            $tmp = "C:\WinUICoverage\restore-$($g.Name.Substring(0,12))-$($state.module)"
            Copy-Item $g.Group[0].backup $tmp -ToSession $s -Force
            Invoke-Command -Session $s -ScriptBlock {
                param($dir, $tmp, [string[]]$rels, $hash)
                if (-not (Test-Path $dir)) { return "remote payload $dir not found; skipped" }
                foreach ($r in $rels) { $d = Join-Path $dir $r; if (Test-Path (Split-Path $d)) { Copy-Item $tmp $d -Force } }
                Remove-Item $tmp -Force
                $bad = @($rels | Where-Object { (Test-Path (Join-Path $dir $_)) -and (Get-FileHash (Join-Path $dir $_)).Hash -ne $hash })
                if ($bad) { "WARNING: hash mismatch on VM for $($bad -join ', ')" } else { "VM restored $($rels.Count) copy(ies) of original $($hash.Substring(0,8))" }
            } -ArgumentList $remoteDir, $tmp, @($g.Group.relativePath), $g.Name | ForEach-Object {
                Info $_
                if ("$_" -like 'WARNING:*') { $vmErrors += "$_" }
            }
        }
        Invoke-Command -Session $s -ScriptBlock {
            param($dir, [string[]]$rts) foreach ($r in $rts) { Remove-Item (Join-Path $dir $r) -Force -EA SilentlyContinue }
        } -ArgumentList $remoteDir, @($state.runtimesAdded)
        if ($vmErrors) {
            throw "VM restore incomplete ($($vmErrors -join '; ')). instrument-state.json was kept; run Step4-Restore-Payload.ps1 again."
        }
    } finally { Remove-PSSession $s }
}
Remove-Item $statePath -Force
Info 'done'

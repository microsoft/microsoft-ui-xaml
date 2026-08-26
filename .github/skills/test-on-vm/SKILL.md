---
name: test-on-vm
description: EXPERIMENTAL. Run locally built WinUI interaction tests on a Hyper-V VM through PowerShell Direct. Use when asked to run WinUI tests on a local VM.
---

# Run WinUI Tests on a Hyper-V VM

Use `tools\run-tests-on-vm.ps1`. It refreshes the local test payload, deploys
it to the VM, performs required machine setup, and runs tests on the VM's
interactive desktop.

This skill and script are experimental and may not be stable.

## Before You Run

Build the repository first. Follow the **build** skill:

```powershell
.\initrun.ps1 .\build.cmd /q
```

Also verify:

- The VM is running with an unlocked desktop and a logged-in user.
- The caller is a local admin or a member of **Hyper-V Administrators**.
- The command uses `initial_wait` of at least **180 seconds**.

For missing Hyper-V permissions, ask the user for confirmation before making
this permanent host change. If they approve, run it once from an admin prompt,
then have them log out and back in:

```powershell
Add-LocalGroupMember -Group 'Hyper-V Administrators' -Member (whoami)
```

The first run prompts for VM credentials in a separate window and caches them
encrypted under `~\.winui-test`. Use `-ResetCredential` to replace them.

## Run Tests

```powershell
# Run a specific test
.\tools\run-tests-on-vm.ps1 -VMName <vm> <testname>

# Wildcard match
.\tools\run-tests-on-vm.ps1 -VMName <vm> *CommandBar*

# Force full copy instead of incremental
.\tools\run-tests-on-vm.ps1 -VMName <vm> -FullCopy <testname>

# Stop a stuck test run
.\tools\run-tests-on-vm.ps1 -VMName <vm> -Stop
```

## Payload Rules

By default, the script runs `test\CreateTestPayload.ps1` to refresh
`TestPayload\<flavor>` from the current build outputs, then deploys changed
files incrementally.

- `-FullCopy` still refreshes the local payload, then copies all of it to the VM.
- **Use `-SkipPayload` rarely.** It deploys the existing local payload without
  refreshing it. Use it only when that payload is known to be current or when
  intentionally testing stale binaries. Never use it merely to save time.

## Useful Options

| Option | Purpose |
|--------|---------|
| `-Platform` | `x86`, `x64`, or `arm64`; inferred when omitted |
| `-Configuration` | `chk` or `fre`; inferred when omitted |
| `-FullCopy` | Copy the full refreshed payload instead of only changes |
| `-SkipPayload` | Rare: deploy the existing payload without refreshing it |
| `-SkipPrerun` | Skip `testmachine-prerun.cmd` |
| `-ForcePrerun` | Run `testmachine-prerun.cmd` again |
| `-Credential` | Supply a `PSCredential` instead of using the prompt/cache |
| `-ResetCredential` | Clear the cached credential for this VM |
| `-Stop` | Stop the active test and clean up |

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `Could not launch app`, foreground-window errors, or an empty UIA tree | VM desktop is locked or not visible | Keep the VM desktop unlocked and active |
| `Failed to connect to VM` | VM not running or wrong credentials | Start the VM; use `-ResetCredential` to re-enter credentials |
| Permission error | Need Hyper-V Administrators membership | Run the one-time group command above |
| Incomplete payload or prerun failure | Stale payload or partial deployment | Omit `-SkipPayload`; retry with `-FullCopy` if needed |

## Crash Dumps

The first prerun configures full crash dumps under `C:\dumps` on the VM, with
up to three dumps per process. Pull one back through PowerShell Direct:

```powershell
$vmName = "<vm>"
$credentialKey = $vmName -replace '[^a-zA-Z0-9]', '_'
$cred = Import-Clixml (Join-Path $env:USERPROFILE ".winui-test\vmcred-$credentialKey.xml")
$destination = Join-Path $env:TEMP "WinUIDumps"
New-Item -ItemType Directory -Path $destination -Force | Out-Null

$session = New-PSSession -VMName $vmName -Credential $cred
try {
    Invoke-Command -Session $session { Get-ChildItem C:\dumps\*.dmp }
    Copy-Item -FromSession $session -Path "C:\dumps\<dump>.dmp" -Destination $destination
}
finally {
    Remove-PSSession $session
}
```

For a quick local analysis:

```powershell
cdb -z "$env:TEMP\WinUIDumps\<dump>.dmp" -c "!analyze -v; ~*k; q"
```

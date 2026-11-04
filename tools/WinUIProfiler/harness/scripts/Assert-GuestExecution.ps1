$ErrorActionPreference = "Stop"

$hostId = [Guid]::Empty
$guestId = [Guid]::Empty
if (-not [Guid]::TryParse($env:PROFILER_HARNESS_HOST_MACHINE_GUID, [ref]$hostId) -or
    -not [Guid]::TryParse($env:PROFILER_HARNESS_GUEST_MACHINE_GUID, [ref]$guestId) -or
    $hostId -eq [Guid]::Empty -or $guestId -eq [Guid]::Empty -or $hostId -eq $guestId) {
    throw "Guest-only operation refused: approved host and guest identity are required."
}
$machineId = [Guid](Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography" -Name MachineGuid -ErrorAction Stop).MachineGuid
if ($machineId -ne $guestId -or $machineId -eq $hostId -or
    -not (Test-Path "HKLM:\SOFTWARE\Microsoft\Virtual Machine\Guest\Parameters")) {
    throw "Guest-only operation refused: this machine is not the approved Hyper-V guest. No host installation or package changes are allowed."
}

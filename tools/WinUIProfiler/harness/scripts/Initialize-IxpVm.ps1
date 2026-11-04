[CmdletBinding()]
param(
    [switch]$Provision
)

$ErrorActionPreference = "Stop"

$prerequisites = @(
    "Run manually in an elevated PowerShell 7 session on the chosen virtualization host.",
    "Hyper-V must already be available and its management service running.",
    "Select an existing virtual switch; this script does not configure host networking.",
    "Reserve disk space for the VM, image downloads, and checkpoints, and leave RAM for the host.",
    "IXPTools must already be installed. Importing it can download/build host dependencies.",
    "Review IXP defaults, authorized image access or matching offline cache, and the Windows branch.",
    "Choose a new VM name and an unused local storage directory; existing VMs are never replaced.",
    "Approve creation of a stopped Desktop amd64 VM with no unattended setup or TShell installation.",
    "Verify the new VM and create a clean checkpoint before its first boot.",
    "Later, manually complete guest setup, verify app/runtime compatibility, and sign in inside the guest."
)

Write-Host "IXP VM prerequisites (review these before proceeding):"
for ($index = 0; $index -lt $prerequisites.Count; $index++) {
    Write-Host ("{0}. {1}" -f ($index + 1), $prerequisites[$index])
}
Write-Host ""
Write-Host "This script never invokes the harness, builds apps, or deploys apps, agents, or the profiler."
Write-Host "VM provisioning still changes the host: IXP initialization, downloads, VM files, and checkpoints."
if (-not $Provision) {
    Write-Host "Checklist only. Rerun manually with -Provision to enter the guided provisioning process."
    return
}

function Confirm-Step {
    param([Parameter(Mandatory)][string]$Message)

    if ((Read-Host "$Message Type yes or y to continue") -notin @("yes", "y")) {
        throw "Provisioning stopped by the user. No automatic cleanup or rollback is attempted."
    }
}

function Read-Setting {
    param(
        [Parameter(Mandatory)][string]$Prompt,
        [Parameter(Mandatory)][string]$Default
    )

    $value = Read-Host "$Prompt [$Default]"
    if ([string]::IsNullOrWhiteSpace($value)) {
        return $Default
    }
    return $value.Trim()
}

Confirm-Step "1. Have you reviewed the entire prerequisite list and chosen this host?"
if (-not $IsWindows -or $PSVersionTable.PSVersion.Major -lt 7) {
    throw "Windows and PowerShell 7 or newer are required."
}
$principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "Open an elevated PowerShell 7 session manually. This script will not elevate itself."
}

Write-Host "2. Checking existing Hyper-V support."
if (-not (Get-Module -ListAvailable Hyper-V)) {
    throw "Hyper-V management tools are unavailable. Configure virtualization separately; no features will be enabled here."
}
Import-Module Hyper-V -ErrorAction Stop
if ((Get-Service -Name vmms -ErrorAction Stop).Status -ne "Running") {
    throw "The Hyper-V management service is not running. Resolve this separately."
}

Write-Host "3. Select an existing virtual switch."
$switches = @(Get-VMSwitch -ErrorAction Stop)
if ($switches.Count -eq 0) {
    throw "No virtual switch exists. Configure one separately before provisioning."
}
$switches | Select-Object Name, SwitchType | Format-Table -AutoSize | Out-Host
$defaultSwitch = if ($switches.Name -contains "Default Switch") { "Default Switch" } else { $switches[0].Name }
$switchName = Read-Setting "Virtual switch name" $defaultSwitch
if ($switchName -notin $switches.Name) {
    throw "Select an existing virtual switch. '$switchName' was not found."
}

Write-Host "4. Configure VM resources and unused storage."
$name = Read-Setting "New VM name" ("WinUIProfilerHarness-" + (Get-Date -Format "yyyyMMdd-HHmmss"))
if ($name -notmatch '^[A-Za-z0-9][A-Za-z0-9_-]{0,63}$') {
    throw "VM name must be 1-64 letters, digits, hyphens, or underscores, starting with a letter or digit."
}
$storage = Read-Setting "New VM storage directory" "C:\VMs\$name"
if ($storage -notmatch '^[A-Za-z]:\\') {
    throw "Choose an absolute directory on a local drive, not a relative path or network share."
}
$storage = [IO.Path]::GetFullPath($storage).TrimEnd('\')
if ($storage -eq [IO.Path]::GetPathRoot($storage).TrimEnd('\') -or
    (Test-Path -LiteralPath $storage)) {
    throw "VM storage must be a new, non-root directory. Refusing '$storage'."
}
$memory = 0
$processors = 0
if (-not [int]::TryParse((Read-Setting "VM RAM in GB" "8"), [ref]$memory) -or $memory -lt 4) {
    throw "Specify at least 4 GB of VM RAM."
}
if (-not [int]::TryParse((Read-Setting "VM processor count" "4"), [ref]$processors) -or $processors -lt 1) {
    throw "Specify a positive VM processor count."
}
$os = Get-CimInstance Win32_OperatingSystem -ErrorAction Stop
$computer = Get-CimInstance Win32_ComputerSystem -ErrorAction Stop
if ($processors -gt $computer.NumberOfLogicalProcessors) {
    throw "The requested processor count exceeds the host's logical processor count."
}
$freeMemoryGb = [double]$os.FreePhysicalMemory / 1MB
if ($memory + 4 -gt $freeMemoryGb) {
    throw "Not enough free RAM to reserve $memory GB for the VM and leave 4 GB free for the host."
}
$drive = [IO.DriveInfo]::new([IO.Path]::GetPathRoot($storage))
if (-not $drive.IsReady -or $drive.AvailableFreeSpace -lt 160GB) {
    throw "At least 160 GB free is required on the VM storage drive for the 128 GB disk and checkpoint headroom."
}
Confirm-Step "Are these resources sufficient, including additional image/cache space on its configured drive?"

Write-Host "5. IXPTools bootstrap requires separate host-side consent."
if (-not (Get-Module -ListAvailable IXPTools)) {
    throw "IXPTools is not installed. Install it through your authorized process separately."
}
Confirm-Step "Allow IXPTools import, including any host dependency downloads/builds it performs?"
Import-Module IXPTools -ErrorAction Stop

Write-Host "6. Review IXP defaults and media access."
Write-Host "If this host is not configured, choose setup and review every IXP prompt."
Write-Host "Leave optional tools, agent installs, application installs, and custom scripts disabled."
$defaults = Read-Setting "IXP defaults: setup or already-configured" "setup"
switch ($defaults) {
    "setup" {
        Confirm-Step "Run IXPTools' interactive defaults setup? This writes host-side IXP configuration."
        New-TestMachine -SetDefaults
    }
    "already-configured" { }
    default { throw "Choose setup or already-configured." }
}
$branch = Read-Setting "Windows image branch (verify application compatibility separately)" "ge_current"
if ($branch -notmatch '^[A-Za-z0-9_.-]+$') {
    throw "Invalid Windows image branch."
}
$media = Read-Setting "Media access: online or cached-offline-fallback" "online"
if ($media -notin @("online", "cached-offline-fallback")) {
    throw "Choose online or cached-offline-fallback."
}
Confirm-Step "Have you verified authorized image access or matching cached media? Offline fallback can still attempt online access."

Write-Host "7. Refusing existing VM/device names and storage directories."
if (@(Get-VM -ErrorAction Stop | Where-Object Name -EQ $name).Count -gt 0 -or
    @(Get-IxpDevice -ErrorAction Stop | Where-Object Name -EQ $name).Count -gt 0 -or
    (Test-Path -LiteralPath $storage)) {
    throw "A VM, IXP device, or storage directory already exists. Choose a new name/path; nothing will be replaced."
}

$parameters = @{
    Name = $name
    Branch = $branch
    Latest = $true
    Flavor = "Desktop"
    Arch = "amd64"
    SavePath = $storage
    VirtualSwitchName = $switchName
    VmMemInGb = $memory
    VmNumProcessors = $processors
    VmDiskSizeInGb = 128
    NoUI = $true
    NoUnattend = $true
    NoTShell = $true
    DoNotStart = $true
    KdSetupMode = "Disable"
    NoNestedVirtualization = $true
    DisableVpnDuringDropDownload = $false
    AllowOffline = $media -eq "cached-offline-fallback"
    EnableCDKeys = $false
    Tool = @()
    WinGetApps = @()
    VelocityKeysToEnable = @()
    VelocityKeysToDisable = @()
    EnableOptionalFeature = @()
    DriverVerifierFlag = 0
}
Write-Host "8. Proposed creation parameters:"
$parameters | ConvertTo-Json -Depth 4 | Out-Host
Write-Host "Review IXPTools' own final summary too. Abort if it proposes deleting a device or unwanted changes."
if ((Read-Host "Type CREATE $name to create this stopped VM") -cne "CREATE $name") {
    throw "VM creation was not approved."
}
# Keep IXPTools' own creation confirmation; never force replacement of an existing device.
New-TestMachine @parameters

Write-Host "9. Verifying the new VM and creating its pre-boot checkpoint."
$vm = Get-VM -Name $name -ErrorAction Stop
if ($vm.State -ne "Off") {
    throw "The new VM is unexpectedly '$($vm.State)'. Do not deploy anything; inspect it manually."
}
$checkpointName = "ProfilerHarness-Clean-PreBoot"
Checkpoint-VM -VM $vm -SnapshotName $checkpointName -ErrorAction Stop
Get-VMSnapshot -VM $vm -Name $checkpointName -ErrorAction Stop | Out-Null

Write-Host "10. VM created and checkpoint verified. It has not been booted or made harness-ready."
Write-Host "Manually boot it, complete Windows setup, and verify the guest's identity and runtime compatibility."
Write-Host "Take a second checkpoint after guest setup, before installing agents, apps, or profiler dependencies."
[pscustomobject]@{
    name = $name
    vmId = [string]$vm.Id
    branch = $branch
    storage = $storage
    switchName = $switchName
    checkpoint = $checkpointName
    state = [string]$vm.State
    harnessReady = $false
}

param(
    [Parameter(Mandatory = $true)][int]$ProfilerProcessId,
    [Parameter(Mandatory = $true)][int]$TargetProcessId,
    [switch]$Stopped
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
if (!('LiveMetricMemoryProbe' -as [type])) {
    Add-Type @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
public static class LiveMetricMemoryProbe {
    [StructLayout(LayoutKind.Sequential)]
    public struct Counters {
        public uint cb, faults;
        public ulong peak, working, peakPaged, paged, peakNonPaged, nonPaged,
            pagefile, peakPagefile, commit, privateWorking, sharedCommit;
    }
    [DllImport("kernel32.dll", SetLastError=true)] static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", SetLastError=true)] static extern bool K32GetProcessMemoryInfo(IntPtr process, ref Counters counters, uint bytes);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
    public static Counters Read(int pid) {
        var handle = OpenProcess(0x1000, false, pid);
        if (handle == IntPtr.Zero) throw new Win32Exception(Marshal.GetLastWin32Error());
        try {
            var value = new Counters { cb = (uint)Marshal.SizeOf<Counters>() };
            if (!K32GetProcessMemoryInfo(handle, ref value, value.cb))
                throw new Win32Exception(Marshal.GetLastWin32Error());
            return value;
        } finally { CloseHandle(handle); }
    }
}
'@
}
$window = [System.Windows.Automation.AutomationElement]::RootElement.FindFirst(
    [System.Windows.Automation.TreeScope]::Children,
    [System.Windows.Automation.AndCondition]::new(
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $ProfilerProcessId),
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::NameProperty, 'WinUI Profiler')))
if (!$window) { throw 'Profiler main window is missing.' }
function Read-Graph([string]$name) {
    $until = [DateTime]::UtcNow.AddSeconds(5)
    do {
        $element = $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::NameProperty, $name))
        if ($element) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $until)
    if (!$element) { throw "Missing running graph: $name" }
    return [pscustomobject]@{ Element = $element; Data = ($element.Current.ItemStatus | ConvertFrom-Json) }
}
$names = @('Live CPU graph', 'Live GPU graph', 'Live working set graph',
    'Live private working set graph', 'Live private commit graph')
$deadline = [DateTime]::UtcNow.AddSeconds(20)
do {
    $graphs = @($names | ForEach-Object { Read-Graph $_ })
    if (@($graphs | Where-Object { $_.Data.points -lt 3 }).Count -eq 0) { break }
    Start-Sleep -Milliseconds 250
} while ([DateTime]::UtcNow -lt $deadline)
foreach ($graph in $graphs) {
    $data = $graph.Data
    if ($data.processId -ne $TargetProcessId -or $data.points -lt 3 -or $data.points -gt 121 -or
        $data.watermark -le 0 -or !$data.sessionId) { throw "Invalid graph identity/bounds: $($graph.Element.Current.ItemStatus)" }
    if ($data.measured -eq 0 -and ($null -ne $data.value -or $data.vertices -ne 0)) {
        throw 'Unavailable graph contains a numeric value or drawn zero.'
    }
    if ($data.measured -gt 0 -and $data.vertices -lt 1) { throw 'Measured graph has no rendered vertices.' }
    if ($null -ne $data.value) {
        $rendered = ([double]$data.value).ToString('N1', [Globalization.CultureInfo]::CurrentCulture) + ' ' + $data.unit
        $text = $graph.Element.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::NameProperty, $rendered))
        if (!$text) { throw "Current graph value is not rendered: $rendered" }
    }
    if ($data.metric -like 'Process*') {
        if ($data.unit -ne 'MiB' -or $data.rawValue -le 0 -or
            $data.source -ne 'NtQuerySystemInformation/SystemProcessInformation') {
            throw 'Memory graph does not show the actual process snapshot field and units.'
        }
        if ([Math]::Abs($data.value - $data.rawValue / 1048576.0) -gt 0.00001) { throw 'Memory MiB conversion is wrong.' }
    }
    Write-Output "LIVE_GRAPH_PASS $($graph.Element.Current.Name) $($graph.Element.Current.ItemStatus)"
}
if (!$Stopped) {
    $native = [LiveMetricMemoryProbe]::Read($TargetProcessId)
    $independent = @{ ProcessWorkingSet = $native.working; ProcessPrivateWorkingSet = $native.privateWorking;
        ProcessPrivateCommit = $native.commit }
    foreach ($graph in $graphs | Where-Object { $_.Data.metric -like 'Process*' }) {
        $expected = [double]$independent[$graph.Data.metric]
        $difference = [Math]::Abs($graph.Data.rawValue - $expected)
        if ($difference -gt [Math]::Max(1048576, $expected * 0.03)) {
            throw "Independent memory read differs beyond asynchronous-window tolerance: $($graph.Data.metric) graph=$($graph.Data.rawValue) K32=$expected"
        }
        Write-Output "LIVE_MEMORY_API_COMPARISON_PASS metric=$($graph.Data.metric) graphBytes=$($graph.Data.rawValue) K32Bytes=$expected"
    }
}
$before = @($graphs | ForEach-Object { $_.Data.latestQpc })
function Select-Workspace([string]$name) {
    $tab = $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.AndCondition]::new(
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::NameProperty, $name),
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
                [System.Windows.Automation.ControlType]::TabItem)))
    if (!$tab) { throw "Missing workspace tab: $name" }
    $tab.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
}
if (!$Stopped) {
    Select-Workspace 'Saved session'
    $deadline = [DateTime]::UtcNow.AddSeconds(5)
    do {
        $pathField = $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::NameProperty, 'Saved session database path'))
        if ($pathField -and !$pathField.Current.IsOffscreen) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    if (!$pathField -or $pathField.Current.IsOffscreen) { throw 'Saved workspace is clipped by the live panel.' }
    $connect = $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::NameProperty, 'Live connection status'))
    if ($connect -and !$connect.Current.IsOffscreen) { throw 'Live panel remained visible over the saved workspace.' }
}
Start-Sleep -Seconds 3
if (!$Stopped) { Select-Workspace 'Live metrics' }
$after = @($names | ForEach-Object { (Read-Graph $_).Data.latestQpc })
for ($i = 0; $i -lt $before.Count; $i++) {
    if ($Stopped -and $after[$i] -ne $before[$i]) { throw 'Graph kept acquiring after disconnect.' }
    if (!$Stopped -and $after[$i] -le $before[$i]) { throw 'Connected running graph did not advance.' }
}
if (!$Stopped -and (Read-Graph $names[0]).Data.sessionId -ne $graphs[0].Data.sessionId) {
    throw 'Switching workspaces reset the connected metric session.'
}
Write-Output "LIVE_GRAPH_LIFECYCLE_PASS stopped=$Stopped"

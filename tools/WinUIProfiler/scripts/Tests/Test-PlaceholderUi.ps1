[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Target,
    [ValidateSet('Unpackaged', 'Packaged', 'Attach')][string]$Mode = 'Unpackaged',
    [string]$TargetArguments = '',
    [int]$ProfilerProcessId = 0,
    [switch]$ExerciseErrors,
    [switch]$CloseWhileConnected,
    [switch]$Reconnect,
    [ValidateRange(5, 300)][int]$ObserveSeconds = 15
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class PlaceholderMemoryCheck {
    [StructLayout(LayoutKind.Sequential)] public struct Counters {
        public uint Size, PageFaultCount;
        public UIntPtr PeakWorkingSet, WorkingSet, QuotaPeakPaged, QuotaPaged;
        public UIntPtr QuotaPeakNonPaged, QuotaNonPaged, Pagefile, PeakPagefile;
        public UIntPtr PrivateCommit, PrivateWorkingSet, SharedCommit;
    }
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern bool K32GetProcessMemoryInfo(IntPtr process, ref Counters counters, uint size);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern bool GetExitCodeProcess(Microsoft.Win32.SafeHandles.SafeProcessHandle process, out uint code);
}
'@
$root = Split-Path (Split-Path $PSScriptRoot)
$existing = @(Get-Process | Select-Object -ExpandProperty Id)
$ownedProfiler = $ProfilerProcessId -eq 0
if ($ownedProfiler) {
    $output = (Get-Content (Join-Path $root 'WinUIProfiler.App\obj\ProfilerAppOutput.x64.Release.txt') -Raw).Trim()
    $profiler = Start-Process (Join-Path $output 'WinUIProfiler.App.exe') -WorkingDirectory $output -PassThru
}
else { $profiler = Get-Process -Id $ProfilerProcessId }
$profilerHandle = $profiler.SafeHandle
$targetPid = 0
$window = $null
function Wait-Until([string]$description, [scriptblock]$probe, [int]$seconds = 30) {
    $until = [DateTime]::UtcNow.AddSeconds($seconds)
    do {
        $value = & $probe
        if ($value) { return $value }
        $profiler.Refresh()
        if ($profiler.HasExited) { throw "Profiler exited $($profiler.ExitCode): $description" }
        Start-Sleep -Milliseconds 200
    } while ([DateTime]::UtcNow -lt $until)
    throw "Timed out: $description"
}
function Find([string]$name) {
    $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::NameProperty, $name))
}
function Ready([string]$name) { Wait-Until $name { Find $name } }
function Click([string]$name) {
    (Ready $name).GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
}
function Status { (Ready 'Live connection status').Current.ItemStatus }
function Graph([string]$name) {
    $text = (Ready $name).Current.ItemStatus
    if ($text) { return $text | ConvertFrom-Json }
}
function Select-Mode([string]$modeName) {
    (Ready 'Launch or attach mode').GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
    (Ready $modeName).GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
}
function Set-Target([string]$value) {
    (Ready 'Executable path, application AUMID, or process ID').GetCurrentPattern(
        [System.Windows.Automation.ValuePattern]::Pattern).SetValue($value)
}
try {
    $window = Wait-Until 'profiler window' {
        [System.Windows.Automation.AutomationElement]::RootElement.FindFirst(
            [System.Windows.Automation.TreeScope]::Children,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $profiler.Id))
    }
    $null = Wait-Until 'empty graphs before hello' {
        $graph = Graph 'CPU and GPU usage graph'
        $graph -and $graph.totalSamples -eq 0
    }
    if ($ExerciseErrors) {
        Set-Target $Target
        Click 'Launch and connect'
        $null = Wait-Until 'explicit untrusted-binary rejection' { (Status) -eq 'Failed' } 10
        if ((Graph 'CPU and GPU usage graph').totalSamples -ne 0) { throw 'Untrusted target produced samples.' }
        $trust = (Ready 'Trust local injection binaries').GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern)
        if ($trust.Current.ToggleState -ne [System.Windows.Automation.ToggleState]::On) { $trust.Toggle() }
        Set-Target (Join-Path $root 'does-not-exist.exe')
        Click 'Launch and connect'
        $null = Wait-Until 'failed launch' { (Status) -eq 'Failed' } 10
        Start-Sleep -Seconds 2
        if ((Status) -ne 'Failed') { throw 'Late connection progress overwrote the failed state.' }
        Write-Output 'TRUST_AND_FAILED_LAUNCH_PASS'
        $sleeping = Start-Process powershell.exe -ArgumentList '-NoProfile', '-NonInteractive', '-Command', 'Start-Sleep -Seconds 120' -WindowStyle Hidden -PassThru
        try {
            Select-Mode 'Attach to process ID'
            Set-Target ([string]$sleeping.Id)
            Click 'Launch and connect'
            $null = Wait-Until 'non-XAML target readiness wait' { (Status) -eq 'Waiting for XAML' } 10
            Click 'Disconnect'
            $null = Wait-Until 'cancelled connection' { (Status) -eq 'Disconnected' } 10
            Start-Sleep -Seconds 2
            $sleeping.Refresh()
            if ((Status) -ne 'Disconnected' -or $sleeping.HasExited -or (Graph 'CPU and GPU usage graph').totalSamples -ne 0) {
                throw 'Cancellation changed target lifetime, admitted samples, or accepted stale progress.'
            }
            Write-Output 'CANCEL_PRESERVES_TARGET_AND_EMPTY_HISTORY_PASS'
        }
        finally { if (!$sleeping.HasExited) { Stop-Process -Id $sleeping.Id } }
    }
    Select-Mode $(switch ($Mode) { 'Packaged' { 'Packaged application' } 'Attach' { 'Attach to process ID' } default { 'Unpackaged executable' } })
    Set-Target $Target
    if ($Mode -ne 'Attach') {
        (Ready 'Launch arguments').GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($TargetArguments)
    }
    $trust = (Ready 'Trust local injection binaries').GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern)
    if ($trust.Current.ToggleState -ne [System.Windows.Automation.ToggleState]::On) { $trust.Toggle() }
    Click 'Launch and connect'
    $null = Wait-Until 'authenticated native TAP hello' {
        $state = Status
        if ($state -eq 'Failed') {
            $names = $window.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition) | ForEach-Object { $_.Current.Name }
            throw "Connection failed: $($names -join ' | ')"
        }
        $state -eq 'Connected'
    } 60
    $identity = (Ready 'Connected target identity').Current.ItemStatus | ConvertFrom-Json
    $targetPid = [int]$identity.processId
    if (!$targetPid -or !$identity.lastHelloUtc -or !$identity.creationTimeFileTime) { throw 'Missing authenticated target identity.' }
    $targetProcess = Get-Process -Id $targetPid
    if ($targetProcess.StartTime.ToFileTimeUtc() -ne $identity.creationTimeFileTime) { throw 'Target creation identity mismatch.' }
    $before = Graph 'CPU and GPU usage graph'
    Write-Output "TAP_HELLO_PASS mode=$Mode profiler=$($profiler.Id) target=$targetPid runtime=$($identity.runtimePath)"
    Start-Sleep -Seconds $ObserveSeconds
    if ((Status) -ne 'Connected') { throw 'Connection did not remain healthy during sampling.' }
    $utilization = Graph 'CPU and GPU usage graph'
    $memory = Graph 'Process memory usage graph'
    if ($utilization.totalSamples -le $before.totalSamples -or $utilization.series.Count -ne 2 -or $memory.series.Count -ne 3) {
        throw 'Expected two combined graphs with 2 utilization and 3 memory series.'
    }
    if ($utilization.unit -ne 'Percent' -or $memory.unit -ne 'MiB') { throw 'Incorrect graph units.' }
    foreach ($series in @($utilization.series) + @($memory.series)) {
        if ($series.points -lt 2 -or $series.points -gt 121) { throw "Invalid bounded history: $($series | ConvertTo-Json -Compress)" }
        if ($series.lastQpc - $series.firstQpc -gt 120 * [Diagnostics.Stopwatch]::Frequency) { throw 'History exceeds two minutes.' }
        if (!$series.source) { throw 'Metric source is absent.' }
        if ($null -eq $series.value -and !$series.reason) { throw 'Missing measurement lacks an explicit reason.' }
    }
    if (($utilization.series.metric -join ',') -ne 'Cpu,Gpu' -or
        ($memory.series.metric -join ',') -ne 'WorkingSet,PrivateWorkingSet,PrivateCommit') { throw 'Incorrect metric grouping.' }
    $counters = [PlaceholderMemoryCheck+Counters]::new()
    $counters.Size = [Runtime.InteropServices.Marshal]::SizeOf($counters)
    if (![PlaceholderMemoryCheck]::K32GetProcessMemoryInfo($targetProcess.Handle, [ref]$counters, $counters.Size)) {
        throw "Independent process memory query failed: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())"
    }
    $expected = @{
        WorkingSet = $counters.WorkingSet.ToUInt64() / 1MB
        PrivateWorkingSet = $counters.PrivateWorkingSet.ToUInt64() / 1MB
        PrivateCommit = $counters.PrivateCommit.ToUInt64() / 1MB
    }
    foreach ($series in $memory.series) {
        if ($null -eq $series.value) { throw "Memory metric unavailable: $($series.metric): $($series.reason)" }
        if ([Math]::Abs($series.value - $expected[$series.metric]) -gt [Math]::Max(16, $expected[$series.metric] * 0.15)) {
            throw "Memory graph differs from independent K32 query: $($series.metric) $($series.value) vs $($expected[$series.metric]) MiB"
        }
    }
    Write-Output "TWO_COMBINED_GRAPHS_PASS samples=$($utilization.totalSamples) utilization=$($utilization | ConvertTo-Json -Depth 5 -Compress) memory=$($memory | ConvertTo-Json -Depth 5 -Compress)"
    Write-Output "INDEPENDENT_MEMORY_COMPARISON_PASS $($expected | ConvertTo-Json -Compress)"
    if ($CloseWhileConnected) {
        $null = $profiler.CloseMainWindow()
        if (!$profiler.WaitForExit(30000)) { throw 'Active connection failed to drain during window close.' }
        $targetProcess.Refresh()
        if ($targetProcess.HasExited) { throw 'Active profiler shutdown terminated the target.' }
        Write-Output 'ACTIVE_CLOSE_DRAINS_AND_PRESERVES_TARGET_PASS'
        return
    }
    Click 'Disconnect'
    $null = Wait-Until 'disconnected owner' { (Status) -eq 'Disconnected' }
    $stopped = Graph 'CPU and GPU usage graph'
    Start-Sleep -Seconds 2
    $retained = Graph 'CPU and GPU usage graph'
    $targetProcess.Refresh()
    if ($targetProcess.HasExited) { throw 'Disconnect terminated the target.' }
    if ($retained.totalSamples -ne $stopped.totalSamples -or $retained.totalSamples -lt 2) { throw 'Stopped history changed or disappeared.' }
    Write-Output "DISCONNECT_RETAINS_HISTORY_AND_TARGET_PASS target=$targetPid samples=$($retained.totalSamples)"
    if ($Reconnect) {
        Select-Mode 'Attach to process ID'
        Set-Target ([string]$targetPid)
        Click 'Launch and connect'
        $null = Wait-Until 'same-process reconnect' { (Status) -eq 'Connected' } 60
        $reconnected = (Ready 'Connected target identity').Current.ItemStatus | ConvertFrom-Json
        if ($reconnected.processId -ne $targetPid -or $reconnected.sessionId -eq $identity.sessionId) {
            throw 'Reconnect reused the old metric scope or selected another target.'
        }
        $fresh = Graph 'CPU and GPU usage graph'
        if ($fresh.totalSamples -ge $retained.totalSamples) { throw 'Reconnect did not reset metric history.' }
        Start-Sleep -Seconds 3
        if ((Graph 'CPU and GPU usage graph').totalSamples -le $fresh.totalSamples) { throw 'Reconnected metrics are not progressing.' }
        Write-Output 'SAME_PID_RECONNECT_RESETS_SCOPE_AND_PROGRESSES_PASS'
    }
}
finally {
    try {
        $profiler.Refresh()
        if (!$profiler.HasExited) {
            $null = $profiler.CloseMainWindow()
            if (!$profiler.WaitForExit(30000)) { throw "Owned profiler $($profiler.Id) failed to drain and exit." }
        }
        $exitCode = [uint32]0
        if (![PlaceholderMemoryCheck]::GetExitCodeProcess($profilerHandle, [ref]$exitCode)) {
            throw "Cannot read the owned profiler exit code: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())"
        }
        if ($exitCode -ne 0) { throw "Owned profiler exited $exitCode." }
        Write-Output 'PROFILER_CLEAN_EXIT_PASS'
    }
    finally {
        if ($targetPid -and $Mode -ne 'Attach' -and $existing -notcontains $targetPid) {
            $ownedTarget = Get-Process -Id $targetPid -ErrorAction SilentlyContinue
            if ($ownedTarget) { $null = $ownedTarget.CloseMainWindow(); $null = $ownedTarget.WaitForExit(10000) }
        }
    }
}

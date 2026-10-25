param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$ApplicationAumid,
    [switch]$VerifyHeartbeat,
    [switch]$VerifySnapshots,
    [switch]$VerifyViewer,
    [switch]$VerifyMetrics,
    [switch]$LeaveConnected
)
$ErrorActionPreference = 'Stop'
if ($VerifyViewer) { $VerifySnapshots = $true }
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class InstalledTargetWindow {
    [DllImport("user32.dll")] public static extern bool SetWindowPos(
        IntPtr window, IntPtr after, int x, int y, int width, int height, uint flags);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr window);
    [DllImport("user32.dll")] public static extern bool ShowWindowAsync(IntPtr window, int command);
}
'@
$profiler = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path $Executable) -PassThru
$keep = $false
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(25)
    do {
        Start-Sleep -Milliseconds 100
        $profiler.Refresh()
        if ($profiler.HasExited) { throw "Profiler exited $($profiler.ExitCode)" }
    } while ($profiler.MainWindowHandle -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
    if ($profiler.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Profiler has no ready window.' }
    $window = [System.Windows.Automation.AutomationElement]::FromHandle($profiler.MainWindowHandle)
    function Find-Control([string]$name) {
        $deadline = [DateTime]::UtcNow.AddSeconds(10)
        do {
            $control = $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.PropertyCondition]::new(
                    [System.Windows.Automation.AutomationElement]::NameProperty, $name))
            if (!$control) { Start-Sleep -Milliseconds 100 }
        } while (!$control -and [DateTime]::UtcNow -lt $deadline)
        if (!$control) { throw "Missing control: $name" }
        return $control
    }
    function Invoke-Control([string]$name) {
        (Find-Control $name).GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    }
    function Select-Mode([string]$name) {
        $mode = Find-Control 'Launch or attach mode'
        $mode.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
        (Find-Control $name).GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
        $mode.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Collapse()
    }
    function Exercise-TargetLayout([int]$width, [int]$height) {
        $target.Refresh()
        $handle = $target.MainWindowHandle
        if ($handle -eq [IntPtr]::Zero) { throw 'Installed target has no main window.' }
        if ([InstalledTargetWindow]::IsIconic($handle)) {
            if (![InstalledTargetWindow]::ShowWindowAsync($handle, 9)) {
                throw 'Could not restore the minimized installed target before exercising XAML.'
            }
            $deadline = [DateTime]::UtcNow.AddSeconds(5)
            do { Start-Sleep -Milliseconds 100 } while (
                [InstalledTargetWindow]::IsIconic($handle) -and [DateTime]::UtcNow -lt $deadline)
            if ([InstalledTargetWindow]::IsIconic($handle)) { throw 'Installed target remained minimized.' }
        }
        if (![InstalledTargetWindow]::SetWindowPos($handle, [IntPtr]::Zero, 0, 0, $width, $height, 0x16)) {
            throw 'Could not drive actual XAML layout in the installed target.'
        }
    }
    function Wait-Connected {
        $deadline = [DateTime]::UtcNow.AddSeconds(55)
        do {
            Start-Sleep -Milliseconds 100
            $state = (Find-Control 'Live connection status').Current.ItemStatus
            if ($state -eq 'Failed') {
                $errors = $window.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                    [System.Windows.Automation.Condition]::TrueCondition) | ForEach-Object { $_.Current.Name }
                throw "Installed target connection failed: $($errors -join ' | ')"
            }
        } while ($state -ne 'Connected' -and [DateTime]::UtcNow -lt $deadline)
        if ($state -ne 'Connected') { throw "Connection did not complete: $state" }
    }
    $snapshots = [System.Collections.Generic.List[object]]::new()
    function Test-TreeSnapshot {
        Invoke-Control 'Take tree snapshot'
        $deadline = [DateTime]::UtcNow.AddSeconds(55)
        do {
            Start-Sleep -Milliseconds 100
            $status = (Find-Control 'Tree snapshot status').Current.ItemStatus
            if ($status -like 'Snapshot failed*') {
                $errors = $window.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                    [System.Windows.Automation.Condition]::TrueCondition) | ForEach-Object { $_.Current.Name }
                throw "Native TAP snapshot failed: $($errors -join ' | ')"
            }
        } while ($status -notlike 'Saved *' -and [DateTime]::UtcNow -lt $deadline)
        if ($status -notlike 'Saved *') { throw "Snapshot did not complete: $status" }
        $path = (Find-Control 'Tree snapshot file').Current.ItemStatus
        $document = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
        if ($document.schemaVersion -ne 1 -or $document.processId -ne $target.Id -or
            $document.creationTimeFileTime -ne $target.StartTime.ToFileTimeUtc() -or
            $document.roots.Count -lt 1 -or $document.xamlNodes.Count -lt 1 -or
            $document.compositionNodes.Count -lt 1 -or $document.correlations.Count -lt 1) {
            throw 'Snapshot lacks actual process-scoped XAML/composition tree evidence.'
        }
        $xaml = @{}
        foreach ($node in $document.xamlNodes) { $xaml[[uint64]$node.id] = $node }
        $visuals = @{}
        foreach ($node in $document.compositionNodes) { $visuals[[uint64]$node.id] = $node }
        foreach ($link in $document.correlations) {
            $element = $xaml[[uint64]$link.xamlNodeId]
            $visual = $visuals[[uint64]$link.visualNodeId]
            if (!$element -or !$visual -or $element.rootId -ne $visual.rootId -or
                $element.objectAddress -notmatch '^0x[0-9a-fA-F]+$' -or
                $visual.visualAddress -notmatch '^0x[0-9a-fA-F]+$') {
                throw 'Snapshot has an uncorrelated object/visual endpoint.'
            }
        }
        if ($snapshots | Where-Object { $_.CaptureId -eq $document.captureId }) { throw 'Repeated snapshot reused its capture identity.' }
        $snapshots.Add([pscustomobject]@{ Path = $path; CaptureId = $document.captureId;
            Hash = (Get-FileHash -LiteralPath $path).Hash })
        Write-Output "INSTALLED_TARGET_TAP_SNAPSHOT_PASS roots=$($document.roots.Count) xaml=$($document.xamlNodes.Count) visuals=$($document.compositionNodes.Count) correlations=$($document.correlations.Count) path=$path"
    }
    Select-Mode 'Launch packaged'
    (Find-Control 'Executable path, application AUMID, or process ID').GetCurrentPattern(
        [System.Windows.Automation.ValuePattern]::Pattern).SetValue($ApplicationAumid)
    $trust = (Find-Control 'Trust local injection binaries').GetCurrentPattern(
        [System.Windows.Automation.TogglePattern]::Pattern)
    if ($trust.Current.ToggleState -ne [System.Windows.Automation.ToggleState]::On) { $trust.Toggle() }
    Invoke-Control 'Launch and connect'
    Wait-Connected
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        Start-Sleep -Milliseconds 100
        $statistics = (Find-Control 'Live event statistics').Current.ItemStatus
    } while ($statistics -notmatch '^PID (\d+) ' -and [DateTime]::UtcNow -lt $deadline)
    if ($statistics -notmatch '^PID (\d+) ') { throw 'Connected target identity is missing.' }
    $target = Get-Process -Id ([int]$Matches[1])
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do { Start-Sleep -Milliseconds 100; $target.Refresh() } while (
        $target.MainWindowHandle -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
    if ($target.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Installed target has no visible main window.' }
    $hello = $window.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition) | Where-Object {
            $_.Current.Name -like "hello received from target PID $($target.Id);*"
        }
    if (!$hello) { throw 'The native target TAP did not return its correlated hello.' }
    foreach ($size in @(1180, 1220)) {
        Exercise-TargetLayout $size 780
        Start-Sleep -Milliseconds 200
    }
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        Start-Sleep -Milliseconds 100
        $statistics = (Find-Control 'Live event statistics').Current.ItemStatus
    } while ($statistics -notmatch '\| XAML events [1-9]' -and [DateTime]::UtcNow -lt $deadline)
    if ($statistics -notmatch '\| XAML events [1-9]') { throw 'No actual target XAML ETW event reached the recording.' }
    if ($VerifySnapshots) {
        Test-TreeSnapshot
        Test-TreeSnapshot
        if ($VerifyViewer) {
            & (Join-Path $PSScriptRoot 'Test-SnapshotViewerUi.ps1') -ProfilerProcessId $profiler.Id `
                -ProfilerWindowHandle $window.Current.NativeWindowHandle -SnapshotPaths @($snapshots.Path) -VerifyLoadFailure
        }
        Start-Sleep -Seconds 6
        if ((Find-Control 'Live connection status').Current.ItemStatus -ne 'Connected') {
            throw 'TAP hello heartbeat failed after repeated snapshots.'
        }
        $before = [long]([regex]::Match((Find-Control 'Live event statistics').Current.ItemStatus, 'XAML events (\d+)').Groups[1].Value)
        Exercise-TargetLayout 1200 760
        $deadline = [DateTime]::UtcNow.AddSeconds(10)
        do {
            Start-Sleep -Milliseconds 100
            $after = [long]([regex]::Match((Find-Control 'Live event statistics').Current.ItemStatus, 'XAML events (\d+)').Groups[1].Value)
        } while ($after -le $before -and [DateTime]::UtcNow -lt $deadline)
        if ($after -le $before) { throw 'Actual XAML ETW stopped after tree capture.' }
    }
    if ($VerifyMetrics) {
        & (Join-Path $PSScriptRoot 'Test-LiveMetricsUi.ps1') -ProfilerProcessId $profiler.Id -TargetProcessId $target.Id
    }
    if ($VerifyHeartbeat) {
        $deadline = [DateTime]::UtcNow.AddSeconds(32)
        do {
            Start-Sleep -Milliseconds 200
            if ((Find-Control 'Live connection status').Current.ItemStatus -ne 'Connected') {
                throw 'Installed target did not remain connected beyond the native idle deadline.'
            }
        } while ([DateTime]::UtcNow -lt $deadline)
        Write-Output 'INSTALLED_TARGET_HEARTBEAT_32SEC_PASS'
    }
    $database = (Find-Control 'Live session database').Current.ItemStatus
    $target.Refresh()
    foreach ($module in $target.Modules | Where-Object {
        $_.ModuleName -in @('Microsoft.UI.Xaml.dll', 'WinUIProfiler.Tap.dll')
    }) {
        Write-Output "INSTALLED_TARGET_MODULE pid=$($target.Id) path=$($module.FileName) sha256=$((Get-FileHash -LiteralPath $module.FileName).Hash)"
    }
    Invoke-Control 'Disconnect'
    $deadline = [DateTime]::UtcNow.AddSeconds(25)
    do { Start-Sleep -Milliseconds 100 } while (
        (Find-Control 'Live connection status').Current.ItemStatus -ne 'Disconnected' -and [DateTime]::UtcNow -lt $deadline)
    if ((Find-Control 'Live connection status').Current.ItemStatus -ne 'Disconnected') { throw 'Disconnect did not drain cleanly.' }
    if ($VerifyMetrics) {
        & (Join-Path $PSScriptRoot 'Test-LiveMetricsUi.ps1') -ProfilerProcessId $profiler.Id -TargetProcessId $target.Id -Stopped
    }
    if ($VerifyViewer) {
        & (Join-Path $PSScriptRoot 'Test-SnapshotViewerUi.ps1') -ProfilerProcessId $profiler.Id `
            -ProfilerWindowHandle $window.Current.NativeWindowHandle -SnapshotPaths @($snapshots.Path) -LeaveOpen:(!$LeaveConnected)
    }
    $cli = Join-Path (Split-Path (Split-Path $PSScriptRoot)) 'WinUIProfiler.Cli\bin\Release\net10.0\WinUIProfiler.Cli.dll'
    $json = dotnet $cli session --db $database
    if ($LASTEXITCODE -ne 0) { throw 'Read-only CLI could not reopen the actual recording.' }
    $saved = $json | ConvertFrom-Json
    if ($saved.result.value.status -ne 'Stopped' -or $saved.result.context.watermark.ingestOrdinal -le 0) {
        throw "Installed target recording has no durable terminal watermark: $json"
    }
    $target.Refresh()
    if ($target.HasExited) { throw 'Profiler disconnect terminated the installed target.' }
    foreach ($snapshot in $snapshots) {
        if ((Get-FileHash -LiteralPath $snapshot.Path).Hash -ne $snapshot.Hash) {
            throw 'Snapshot was lost or changed after disconnect.'
        }
        $null = Get-Content -LiteralPath $snapshot.Path -Raw | ConvertFrom-Json
    }
    Write-Output "INSTALLED_PACKAGE_TAP_HELLO_ETW_PASS profiler=$($profiler.Id) target=$($target.Id) durable=$($saved.result.context.watermark.ingestOrdinal) database=$database"
    if ($LeaveConnected) {
        Select-Mode 'Attach to process ID'
        (Find-Control 'Executable path, application AUMID, or process ID').GetCurrentPattern(
            [System.Windows.Automation.ValuePattern]::Pattern).SetValue([string]$target.Id)
        Invoke-Control 'Launch and connect'
        Wait-Connected
        Exercise-TargetLayout 1180 780
        $deadline = [DateTime]::UtcNow.AddSeconds(10)
        do {
            Start-Sleep -Milliseconds 100
            $statistics = (Find-Control 'Live event statistics').Current.ItemStatus
        } while ($statistics -notmatch '\| XAML events [1-9]' -and [DateTime]::UtcNow -lt $deadline)
        if ($statistics -notmatch "^PID $($target.Id) \| ETW events [1-9].*\| XAML events [1-9]") {
            throw "Reconnected target identity or actual XAML ETW receipt is missing: $statistics"
        }
        if ($VerifySnapshots) { Test-TreeSnapshot }
        if ($VerifyMetrics) {
            & (Join-Path $PSScriptRoot 'Test-LiveMetricsUi.ps1') -ProfilerProcessId $profiler.Id -TargetProcessId $target.Id
        }
        if ($VerifyViewer) {
            & (Join-Path $PSScriptRoot 'Test-SnapshotViewerUi.ps1') -ProfilerProcessId $profiler.Id `
                -ProfilerWindowHandle $window.Current.NativeWindowHandle -SnapshotPaths @($snapshots.Path) -LeaveOpen
        }
        $keep = $true
        Write-Output "LEFT_CONNECTED profiler=$($profiler.Id) target=$($target.Id) $statistics"
    }
}
finally {
    if (!$keep) {
        $profiler.Refresh()
        if (!$profiler.HasExited) {
            if ($window) {
                $window.GetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern).Close()
            }
            else { $null = $profiler.CloseMainWindow() }
            if (!$profiler.WaitForExit(30000)) { Stop-Process -Id $profiler.Id }
            if ($VerifyViewer -and $profiler.ExitCode -ne 0) { throw "Profiler/viewer shutdown failed: $($profiler.ExitCode)" }
        }
    }
    $profiler.Dispose()
}

# Execute external UI Automation actions against one owned process; never instrument the app.
#Requires -Version 5.1
param(
    [Parameter(Mandatory)][int]$ProcessId,
    [Parameter(Mandatory)][long]$CreationTimeFileTime,
    [Parameter(Mandatory)][string]$ScenarioPath,
    [Parameter(Mandatory)][string]$OutputPath
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes, System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class ScenarioWindow {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hwnd, IntPtr after, int x, int y, int w, int h, uint flags);
    [DllImport("user32.dll")] public static extern uint GetDpiForWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint pid);
}
'@
$scenario = Get-Content -LiteralPath $ScenarioPath -Raw | ConvertFrom-Json
$target = Get-Process -Id $ProcessId
if ($target.StartTime.ToUniversalTime().ToFileTimeUtc() -ne $CreationTimeFileTime) { throw 'Target PID was reused.' }
$audit = [ordered]@{
    state = 'running'; qpcFrequency = [Diagnostics.Stopwatch]::Frequency
    startedQpc = 0L; endedQpc = 0L; window = $null; steps = @()
}
function Save-Audit {
    [IO.File]::WriteAllText($OutputPath, ($audit | ConvertTo-Json -Depth 20), [Text.UTF8Encoding]::new($false))
}
function Find-TargetElement($Selector) {
    $conditions = [Collections.Generic.List[System.Windows.Automation.Condition]]::new()
    $conditions.Add([System.Windows.Automation.PropertyCondition]::new(
        [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $ProcessId))
    foreach ($pair in @(@('name', 'NameProperty'), @('automationId', 'AutomationIdProperty'), @('className', 'ClassNameProperty'))) {
        $property = $Selector.PSObject.Properties[$pair[0]]
        if ($null -ne $property) {
            $conditions.Add([System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::($pair[1]), [string]$property.Value))
        }
    }
    if ($null -ne $Selector.PSObject.Properties['controlType']) {
        $conditions.Add([System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
            [System.Windows.Automation.ControlType]::($Selector.controlType)))
    }
    $conditions.Add([System.Windows.Automation.PropertyCondition]::new(
        [System.Windows.Automation.AutomationElement]::IsOffscreenProperty, $false))
    $condition = [System.Windows.Automation.AndCondition]::new($conditions.ToArray())
    $wait = [Diagnostics.Stopwatch]::StartNew()
    while ($wait.Elapsed.TotalSeconds -lt 15) {
        $matches = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, $condition)
        if ($matches.Count -gt 0) { return $matches[0] }
        Start-Sleep -Milliseconds 50
    }
    throw "UI element missing: $($Selector | ConvertTo-Json -Compress)"
}
function Send-TargetKeys($Element, [string]$Keys) {
    if ([ScenarioWindow]::GetForegroundWindow() -ne $handle) { throw 'The target lost foreground focus; no keys were sent.' }
    $Element.SetFocus()
    if ([System.Windows.Automation.AutomationElement]::FocusedElement.Current.ProcessId -ne $ProcessId) {
        throw 'Keyboard focus is not in the target process.'
    }
    [System.Windows.Forms.SendKeys]::SendWait($Keys)
}
try {
    $wait = [Diagnostics.Stopwatch]::StartNew()
    while ($target.MainWindowHandle -eq [IntPtr]::Zero -and $wait.Elapsed.TotalSeconds -lt 15) {
        $target.Refresh()
        if ($target.HasExited) { throw 'Target exited before its window appeared.' }
        Start-Sleep -Milliseconds 50
    }
    $handle = $target.MainWindowHandle
    if ($handle -eq [IntPtr]::Zero) { throw 'Target has no main window.' }
    $root = [System.Windows.Automation.AutomationElement]::FromHandle($handle)
    $windowPattern = $root.GetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern)
    $windowPattern.SetWindowVisualState([System.Windows.Automation.WindowVisualState]::Normal)
    if (![ScenarioWindow]::SetWindowPos($handle, [IntPtr]::Zero, 40, 40, $scenario.window.width, $scenario.window.height, 4)) {
        throw 'Could not set the target window size.'
    }
    [void][ScenarioWindow]::SetForegroundWindow($handle)
    Start-Sleep -Milliseconds 300
    [uint32]$owner = 0
    $threadId = [ScenarioWindow]::GetWindowThreadProcessId($handle, [ref]$owner)
    if ($owner -ne $ProcessId) { throw 'Window belongs to a different process.' }
    $bounds = $root.Current.BoundingRectangle
    $audit.window = @{
        width = [int]$bounds.Width; height = [int]$bounds.Height
        dpi = [ScenarioWindow]::GetDpiForWindow($handle); threadId = $threadId
    }
    $audit.startedQpc = [Diagnostics.Stopwatch]::GetTimestamp()
    foreach ($step in $scenario.steps) {
        $entry = [ordered]@{
            id = $step.id; action = $step.action; state = 'running'
            from = $null; to = $null
            startedQpc = [Diagnostics.Stopwatch]::GetTimestamp(); endedQpc = 0L
        }
        $audit.steps += $entry
        Save-Audit
        $element = Find-TargetElement $step.selector
        switch ($step.action) {
            'type-text' {
                Send-TargetKeys $element '^a'
                $escaped = [regex]::Replace($step.text, '[+^%~(){}\[\]]', '{$0}')
                Send-TargetKeys $element $escaped
                $value = $element.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
                if ($value.Current.Value -ne $step.text) { throw 'Search text was not entered.' }
            }
            'keys' { Send-TargetKeys $element $step.keys }
            'invoke' { $element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke() }
            'assert' { }
            { $_ -in 'expand', 'collapse' } {
                $pattern = $element.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
                $entry.from = [string]$pattern.Current.ExpandCollapseState
                $expected = if ($step.action -eq 'expand') { 'Expanded' } else { 'Collapsed' }
                if ($entry.from -eq $expected) { throw 'Requested expansion transition is a no-op.' }
                if ($step.action -eq 'expand') { $pattern.Expand() } else { $pattern.Collapse() }
                $wait.Restart()
                while ([string]$pattern.Current.ExpandCollapseState -ne $expected -and $wait.Elapsed.TotalSeconds -lt 5) {
                    Start-Sleep -Milliseconds 20
                }
                $entry.to = [string]$pattern.Current.ExpandCollapseState
                if ($entry.to -ne $expected) { throw 'Tree expansion state did not change.' }
            }
            default { throw "Unsupported external UI action: $($step.action)" }
        }
        Start-Sleep -Milliseconds $step.settleMilliseconds
        $entry.endedQpc = [Diagnostics.Stopwatch]::GetTimestamp()
        $entry.state = 'passed'
        Save-Audit
    }
    $audit.endedQpc = [Diagnostics.Stopwatch]::GetTimestamp()
    $audit.state = 'complete'
    Save-Audit
} catch {
    $audit.state = 'failed'
    Save-Audit
    throw
} finally { $target.Dispose() }

[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ApplicationId,
    [Parameter(Mandatory)][ValidateRange(1, 2147483647)][int]$TargetProcessId
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
$root = Split-Path (Split-Path $PSScriptRoot)
$output = (Get-Content (Join-Path $root 'WinUIProfiler.App\obj\ProfilerAppOutput.x64.Release.txt') -Raw).Trim()
$profiler = Start-Process (Join-Path $output 'WinUIProfiler.App.exe') -WorkingDirectory $output -PassThru
$null = $profiler.Handle
$window = $null
function Wait-Until([string]$description, [scriptblock]$probe) {
    $until = [DateTime]::UtcNow.AddSeconds(30)
    do {
        $value = & $probe
        if ($value) { return $value }
        if ($profiler.HasExited) { throw "Owned profiler exited: $description" }
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
function Candidate([string]$value) {
    $list = Find 'Available targets'
    if (!$list) { return }
    foreach ($item in $list.FindAll([System.Windows.Automation.TreeScope]::Children,
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
            [System.Windows.Automation.ControlType]::ListItem))) {
        if ($item.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::NameProperty, $value))) { return $item }
    }
}
try {
    $window = Wait-Until 'owned profiler window' {
        [System.Windows.Automation.AutomationElement]::RootElement.FindFirst(
            [System.Windows.Automation.TreeScope]::Children,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $profiler.Id))
    }
    foreach ($selection in @(@('Packaged application', $ApplicationId), @('Attach to process ID', [string]$TargetProcessId))) {
        (Ready 'Launch or attach mode').GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
        (Ready $selection[0]).GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
        Click 'Browse for target'
        (Ready 'Search name, AUMID, or PID').GetCurrentPattern(
            [System.Windows.Automation.ValuePattern]::Pattern).SetValue($selection[1])
        $item = Wait-Until 'matching discovered target' { Candidate $selection[1] }
        $item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
        $null = Wait-Until 'enabled selection' { (Ready 'Select').Current.IsEnabled }
        Click 'Refresh list'
        $null = Wait-Until 'refreshed candidate' { Candidate $selection[1] }
        if ((Ready 'Select').Current.IsEnabled) { throw 'Refresh retained a stale selected candidate.' }
        (Candidate $selection[1]).GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
        $null = Wait-Until 'refreshed selection enabled' { (Ready 'Select').Current.IsEnabled }
        Click 'Select'
        $null = Wait-Until 'selected target copied to main page' {
            $input = Find 'Executable path, application AUMID, or process ID'
            $input -and $input.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value -eq $selection[1]
        }
        Click 'Browse for target'
        Click 'Cancel'
        $null = Wait-Until 'cancel preserves target input' {
            $input = Find 'Executable path, application AUMID, or process ID'
            $input -and $input.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value -eq $selection[1] -and
                !(Find 'Available targets')
        }
        Write-Output "PICKER_SEARCH_REFRESH_SELECTION_CANCEL_PASS mode=$($selection[0])"
    }
}
finally {
    if (!$profiler.HasExited) {
        $null = $profiler.CloseMainWindow()
        if (!$profiler.WaitForExit(30000)) { throw 'Owned picker profiler failed to close.' }
    }
    if ($profiler.ExitCode -ne 0) { throw "Owned picker profiler exited $($profiler.ExitCode)." }
}

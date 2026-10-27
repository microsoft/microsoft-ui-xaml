param(
    [Parameter(Mandatory = $true)][string]$Executable
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$Executable = (Resolve-Path -LiteralPath $Executable).Path
$process = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path $Executable) -PassThru

function Wait-Element($root, $property, [string]$value) {
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do {
        $element = $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.PropertyCondition]::new($property, $value))
        if ($element -and !$element.Current.IsOffscreen) { return $element }
        if ($process.HasExited) { throw "Profiler exited early: $($process.ExitCode)" }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Missing visible UI element: $value"
}

function Get-Viewer {
    $windows = [System.Windows.Automation.AutomationElement]::RootElement.FindAll(
        [System.Windows.Automation.TreeScope]::Children,
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $process.Id))
    return @($windows | Where-Object { $_.Current.Name -eq 'Snapshot viewer' })
}

try {
    $null = $process.Handle
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    do {
        $process.Refresh()
        if ($process.HasExited) { throw "Profiler failed to launch: $($process.ExitCode)" }
        if ($process.MainWindowHandle -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($process.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Profiler did not create a window.' }

    $main = [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
    $name = [System.Windows.Automation.AutomationElement]::NameProperty
    $id = [System.Windows.Automation.AutomationElement]::AutomationIdProperty
    foreach ($text in @('No process connected.', 'CPU', 'GPU', 'Working set', 'Private working set')) {
        $null = Wait-Element $main $name $text
    }
    $scroll = (Wait-Element $main $id 'ProcessMetricsScroll').GetCurrentPattern(
        [System.Windows.Automation.ScrollPattern]::Pattern)
    if ($scroll.Current.VerticallyScrollable) { $scroll.SetScrollPercent(-1, 100) }
    $null = Wait-Element $main $name 'Private commit'
    if ($scroll.Current.VerticallyScrollable) { $scroll.SetScrollPercent(-1, 0) }
    foreach ($runtime in @('coreclr.dll', 'Microsoft.UI.Xaml.dll')) {
        $module = $process.Modules | Where-Object { $_.ModuleName -eq $runtime }
        if (!$module -or $module.FileName -ine (Join-Path (Split-Path $Executable) $runtime)) {
            throw "Profiler did not load its deployed runtime: $runtime"
        }
    }

    $savedTab = Wait-Element $main $id 'SavedSessionTab'
    $savedTab.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
    $null = Wait-Element $main $name 'No session loaded.'
    if ((Wait-Element $main $id 'OpenSession').Current.IsEnabled) {
        throw 'Session loading must be disabled in the app shell.'
    }
    $metricsTab = Wait-Element $main $id 'ProcessMetricsTab'
    $metricsTab.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
    $null = Wait-Element $main $name 'No process connected.'

    $button = Wait-Element $main $id 'SnapshotViewer'
    $invoke = $button.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
    $invoke.Invoke()
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do {
        $viewers = @(Get-Viewer)
        if ($viewers.Count -eq 1) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($viewers.Count -ne 1) { throw 'Expected one snapshot viewer.' }
    $viewer = $viewers[0]
    $null = Wait-Element $viewer $name 'No snapshot loaded. Snapshot capture and loading are not available in this build.'
    foreach ($treeId in @('SnapshotXamlTree', 'SnapshotCompositionTree')) {
        if ((Wait-Element $viewer $id $treeId).Current.IsEnabled) {
            throw "Snapshot tree must remain inactive: $treeId"
        }
    }
    $invoke.Invoke()
    Start-Sleep -Milliseconds 200
    if (@(Get-Viewer).Count -ne 1) { throw 'Opening the viewer twice created duplicate windows.' }

    $viewer.GetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern).Close()
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    while (@(Get-Viewer).Count -ne 0 -and [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 100
    }
    if (@(Get-Viewer).Count -ne 0) { throw 'Snapshot viewer did not close.' }
    $invoke.Invoke()
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    while (@(Get-Viewer).Count -ne 1 -and [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 100
    }
    if (@(Get-Viewer).Count -ne 1) { throw 'Snapshot viewer did not reopen.' }

    $main.GetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern).Close()
    if (!$process.WaitForExit(10000)) { throw 'Closing the main window left the profiler or viewer running.' }
    if ($process.ExitCode -ne 0) { throw "Profiler exited with code $($process.ExitCode)." }
    Write-Output 'APP_SHELL_UI_PASS: local runtimes, graph panels, session navigation, inactive data actions, viewer reuse/reopen, clean shutdown.'
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id }
    $process.Dispose()
}

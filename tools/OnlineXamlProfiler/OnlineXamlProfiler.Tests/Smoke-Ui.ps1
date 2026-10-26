param(
    [Parameter(Mandatory)][string]$ProfilerPath,
    [Parameter(Mandatory, ParameterSetName='Attach')][uint32]$TargetProcessId,
    [Parameter(Mandatory, ParameterSetName='Launch')][string]$TargetExecutable,
    [switch]$EnableGpuTracing,
    [string]$ScreenshotPath
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName UIAutomationClient
$scope = [System.Windows.Automation.TreeScope]::Descendants
$nameProperty = [System.Windows.Automation.AutomationElement]::NameProperty
$typeProperty = [System.Windows.Automation.AutomationElement]::ControlTypeProperty
$profiler = Start-Process -FilePath $ProfilerPath -WorkingDirectory (Split-Path $ProfilerPath) -PassThru

function Wait-For([scriptblock]$Condition, [string]$Description) {
    $deadline = (Get-Date).AddSeconds(35)
    do {
        $profiler.Refresh()
        if ($profiler.HasExited) { throw "Profiler exited while $Description." }
        if (& $Condition) { return }
        Start-Sleep -Milliseconds 100
    } while ((Get-Date) -lt $deadline)
    $details = if ($root) {
        @($root.FindAll($scope, [System.Windows.Automation.PropertyCondition]::new(
            $typeProperty, [System.Windows.Automation.ControlType]::Text)) |
            ForEach-Object { $_.Current.Name }) -join ' | '
    } else { 'No profiler UI root.' }
    $bounds = if ($root) {
        @('SnapshotPanelsScroll', 'XamlTree', 'CompTree', 'VisualTree') | ForEach-Object {
            $element = Find-Id $_
            "$_ bounds=$($element.Current.BoundingRectangle)"
        }
    }
    throw "Timed out while $Description. Viewports: $($bounds -join '; '). Profiler UI: $details"
}

function Find-Name([string]$Name) {
    $root.FindFirst($scope, [System.Windows.Automation.PropertyCondition]::new($nameProperty, $Name))
}

function Find-Id([string]$Id) {
    $root.FindFirst($scope, [System.Windows.Automation.PropertyCondition]::new(
        [System.Windows.Automation.AutomationElement]::AutomationIdProperty, $Id))
}

function Invoke-Button([string]$Name) {
    $button = Find-Name $Name
    if (!$button -or !$button.Current.IsEnabled) { throw "Button '$Name' is unavailable." }
    $button.SetFocus()
    ([System.Windows.Automation.InvokePattern]$button.GetCurrentPattern(
        [System.Windows.Automation.InvokePattern]::Pattern)).Invoke()
}

function Set-Text([string]$Name, [string]$Value) {
    $control = Find-Name $Name
    $control.SetFocus()
    $valuePattern = [System.Windows.Automation.ValuePattern]$control.GetCurrentPattern(
        [System.Windows.Automation.ValuePattern]::Pattern)
    $valuePattern.SetValue($Value)
    Wait-For { $valuePattern.Current.Value -eq $Value } "setting $Name"
}

function Selected-Snapshot {
    $combo = Find-Name 'Captured snapshots'
    $selection = [System.Windows.Automation.SelectionPattern]$combo.GetCurrentPattern(
        [System.Windows.Automation.SelectionPattern]::Pattern)
    @($selection.Current.GetSelection() | ForEach-Object { $_.Current.Name }) -join '; '
}

function Memory-Reading([string]$Name) {
    $label = Find-Name $Name
    if (!$label) { return $null }
    $row = [System.Windows.Automation.TreeWalker]::ControlViewWalker.GetParent($label)
    @($row.FindAll($scope, [System.Windows.Automation.PropertyCondition]::new(
        $typeProperty, [System.Windows.Automation.ControlType]::Text)) |
        ForEach-Object { $_.Current.Name }) -join ' | '
}

function Show-Memory {
    $panel = Find-Id 'SnapshotMemory'
    ([System.Windows.Automation.ExpandCollapsePattern]$panel.GetCurrentPattern(
        [System.Windows.Automation.ExpandCollapsePattern]::Pattern)).Expand()
    $viewport = Find-Id 'SnapshotPanelsScroll'
    $scroll = [System.Windows.Automation.ScrollPattern]$viewport.GetCurrentPattern(
        [System.Windows.Automation.ScrollPattern]::Pattern)
    Wait-For { $scroll.Current.VerticallyScrollable } 'expanding the snapshot memory panel'
    $scroll.SetScrollPercent([System.Windows.Automation.ScrollPattern]::NoScroll, 100)
    Wait-For { (Memory-Reading 'Measured core object allocations') -match '[\d,]+ bytes' } 'showing frozen core allocations'
}

function Tree-Group([string]$Id) {
    $root.FindFirst($scope, [System.Windows.Automation.PropertyCondition]::new(
        [System.Windows.Automation.AutomationElement]::AutomationIdProperty, $Id))
}

function Tree-Items([string]$Id) {
    $group = Tree-Group $Id
    $group.FindAll($scope, [System.Windows.Automation.PropertyCondition]::new(
        $typeProperty, [System.Windows.Automation.ControlType]::TreeItem))
}

function Captured-NodeCount {
    $status = @($root.FindAll($scope, [System.Windows.Automation.PropertyCondition]::new(
        $typeProperty, [System.Windows.Automation.ControlType]::Text)) |
        Where-Object { $_.Current.Name -match '^Captured nodes: \d+ \|' })
    if ($status.Count -eq 1) {
        [int][regex]::Match($status[0].Current.Name, '^Captured nodes: (\d+)').Groups[1].Value
    }
}

function Expand-AllTrees {
    Wait-For { (Find-Name 'Expand all trees').Current.IsEnabled } 'enabling tree expansion'
    Invoke-Button 'Expand all trees'
    Wait-For { (Find-Name 'Expand all trees').Current.IsEnabled } 'finishing tree expansion'
    Wait-For { (Captured-NodeCount) -gt 0 } 'showing the captured node count'
    $expected = Captured-NodeCount
    $total = 0
    foreach ($id in @('XamlTree', 'CompTree', 'VisualTree')) {
        $seen = [System.Collections.Generic.HashSet[string]]::new()
        $scrollElement = (Tree-Group $id).FindFirst([System.Windows.Automation.TreeScope]::Subtree,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::IsScrollPatternAvailableProperty, $true))
        $scroll = if ($scrollElement) {
            [System.Windows.Automation.ScrollPattern]$scrollElement.GetCurrentPattern(
                [System.Windows.Automation.ScrollPattern]::Pattern)
        } else { $null }
        if ($scroll -and $scroll.Current.VerticallyScrollable) {
            $scroll.SetScrollPercent([System.Windows.Automation.ScrollPattern]::NoScroll, 0)
            Wait-For { $scroll.Current.VerticalScrollPercent -eq 0 } "scrolling $id to its first node"
        }
        do {
            Wait-For {
                $items = Tree-Items $id
                if ($items.Count -lt 2) { return $false }
                foreach ($item in $items) {
                    $pattern = [System.Windows.Automation.ExpandCollapsePattern]$item.GetCurrentPattern(
                        [System.Windows.Automation.ExpandCollapsePattern]::Pattern)
                    if ($pattern.Current.ExpandCollapseState -notin @(
                        [System.Windows.Automation.ExpandCollapseState]::Expanded,
                        [System.Windows.Automation.ExpandCollapseState]::LeafNode)) {
                        return $false
                    }
                }
                return $true
            } "expanding every displayed descendant in $id"
            $realized = @{ Previous = $null; Names = @() }
            Wait-For {
                $current = @(Tree-Items $id | ForEach-Object { $_.Current.Name })
                $key = $current -join "`n"
                $stable = $key -eq $realized.Previous
                $realized.Previous = $key
                $realized.Names = $current
                $stable
            } "stabilizing recycled item labels in $id"
            $names = $realized.Names
            foreach ($name in $names) { [void]$seen.Add($name) }
            if (!$scroll -or !$scroll.Current.VerticallyScrollable -or $scroll.Current.VerticalScrollPercent -ge 100) {
                break
            }
            $previous = $scroll.Current.VerticalScrollPercent
            $scroll.Scroll([System.Windows.Automation.ScrollAmount]::NoAmount,
                [System.Windows.Automation.ScrollAmount]::LargeIncrement)
            Wait-For {
                $scroll.Current.VerticalScrollPercent -gt $previous
            } "realizing the next page of $id"
        } while ($true)
        $total += $seen.Count
        Write-Output "$id expanded nodes: $($seen.Count)"
    }
    if ($total -ne $expected) { throw "Expanded $total nodes, but the snapshot contains $expected." }
}

try {
    Wait-For { $profiler.MainWindowHandle -ne 0 } 'opening the profiler'
    $root = [System.Windows.Automation.AutomationElement]::FromHandle($profiler.MainWindowHandle)
    Wait-For { $null -ne (Find-Name 'Expand all trees') } 'loading profiler controls'
    if ((Find-Name 'Expand all trees').Current.IsEnabled) {
        throw 'Expand all trees must be disabled before a snapshot exists.'
    }
    $gpuCheckbox = Find-Name 'Optional Windows GPU trace (may require elevation; enable before launch/attach)'
    if (!$gpuCheckbox) { throw 'The opt-in Windows GPU tracing control is missing.' }
    $gpuToggle = [System.Windows.Automation.TogglePattern]$gpuCheckbox.GetCurrentPattern(
        [System.Windows.Automation.TogglePattern]::Pattern)
    if ($gpuToggle.Current.ToggleState -ne [System.Windows.Automation.ToggleState]::Off) {
        throw 'Windows GPU tracing must be off by default.'
    }
    if ($EnableGpuTracing) {
        $gpuToggle.Toggle()
        Wait-For { $gpuToggle.Current.ToggleState -eq [System.Windows.Automation.ToggleState]::On } 'enabling Windows GPU tracing'
    }
    if ($PSCmdlet.ParameterSetName -eq 'Launch') {
        Set-Text 'Executable path' $TargetExecutable
        Invoke-Button 'Launch executable'
    } else {
        Set-Text 'Running process ID' $TargetProcessId.ToString()
        Invoke-Button 'Attach'
    }
    Wait-For { (Find-Name 'Take snapshot').Current.IsEnabled } 'connecting to the target'
    if ($gpuCheckbox.Current.IsEnabled) { throw 'Tracing configuration must not change while connected.' }
    Wait-For {
        (Find-Id 'StatisticsStatus').Current.Name -like 'Live process statistics | PID *' -and
        (Find-Id 'RamStatistics').Current.Name -match '\d+[\.,]\d+ MiB'
    } 'sampling process memory without a tree snapshot'
    $firstSample = (Find-Id 'StatisticsStatus').Current.Name
    Wait-For { (Find-Id 'StatisticsStatus').Current.Name -ne $firstSample } 'refreshing live statistics'
    foreach ($id in @('CommitStatistics', 'GpuMemoryStatistics', 'GpuEngineStatistics')) {
        if (!(Find-Id $id)) { throw "Statistics card '$id' is missing." }
        Write-Output "$id : $((Find-Id $id).Current.Name)"
    }
    Write-Output "Statistics diagnostic: $((Find-Id 'StatisticsDiagnostic').Current.Name)"
    if ($PSCmdlet.ParameterSetName -eq 'Launch') {
        $pidBox = Find-Name 'Running process ID'
        $launchedPid = ([System.Windows.Automation.ValuePattern]$pidBox.GetCurrentPattern(
            [System.Windows.Automation.ValuePattern]::Pattern)).Current.Value
        Write-Output "Launched target PID: $launchedPid"
    }
    $snapshotAStatistics = $null
    $snapshotAMemory = $null
    foreach ($number in 1..2) {
        Invoke-Button 'Take snapshot'
        Wait-For { (Selected-Snapshot) -like "#$number |*" } "publishing snapshot $number"
        Wait-For {
            (Find-Id 'SnapshotStatistics').Current.Name -like "#$number |*" -and
            (Find-Id 'SnapshotStatistics').Current.Name -match 'ms from capture midpoint'
        } "retaining process statistics for snapshot $number"
        if ($number -eq 1) { $snapshotAStatistics = (Find-Id 'SnapshotStatistics').Current.Name }
        Show-Memory
        if ($number -eq 1) { $snapshotAMemory = Memory-Reading 'Measured core object allocations' }
    }

    foreach ($id in @('XamlTree', 'CompTree', 'VisualTree')) {
        if ((Tree-Group $id).Current.BoundingRectangle.IsEmpty) {
            throw "$id has no visible viewport with the snapshot memory panel open."
        }
        $items = Tree-Items $id
        if ($items.Count -eq 0) { throw "$id has no displayed roots." }
        $expand = [System.Windows.Automation.ExpandCollapsePattern]$items[0].GetCurrentPattern(
            [System.Windows.Automation.ExpandCollapsePattern]::Pattern)
        if ($expand.Current.ExpandCollapseState -eq [System.Windows.Automation.ExpandCollapseState]::LeafNode) {
            throw "$id is not displaying the captured hierarchy."
        }
        $expand.Expand()
    }
    Wait-For { (Tree-Items 'CompTree').Count -gt 1 } 'expanding the HWCompNode tree'
    $compItem = (Tree-Items 'CompTree')[1]
    ([System.Windows.Automation.SelectionItemPattern]$compItem.GetCurrentPattern(
        [System.Windows.Automation.SelectionItemPattern]::Pattern)).Select()
    Invoke-Button 'Show IVisual'
    Wait-For {
        @(Tree-Items 'VisualTree' | Where-Object {
            ([System.Windows.Automation.SelectionItemPattern]$_.GetCurrentPattern(
                [System.Windows.Automation.SelectionItemPattern]::Pattern)).Current.IsSelected
        }).Count -eq 1
    } 'following the HWCompNode to its composition visual'
    Expand-AllTrees

    $combo = Find-Name 'Captured snapshots'
    ([System.Windows.Automation.ExpandCollapsePattern]$combo.GetCurrentPattern(
        [System.Windows.Automation.ExpandCollapsePattern]::Pattern)).Expand()
    $first = @($root.FindAll($scope, [System.Windows.Automation.PropertyCondition]::new(
        $typeProperty, [System.Windows.Automation.ControlType]::ListItem)) |
        Where-Object { $_.Current.Name -like '#1 |*' })[0]
    if (!$first) { throw 'Snapshot A was not retained.' }
    ([System.Windows.Automation.SelectionItemPattern]$first.GetCurrentPattern(
        [System.Windows.Automation.SelectionItemPattern]::Pattern)).Select()
    Wait-For { (Selected-Snapshot) -like '#1 |*' } 'returning to snapshot A'
    if ((Find-Id 'SnapshotStatistics').Current.Name -ne $snapshotAStatistics) {
        throw 'Snapshot A process statistics changed after capture.'
    }
    Show-Memory
    Wait-For { (Memory-Reading 'Measured core object allocations') -eq $snapshotAMemory } 'restoring snapshot A memory readings'
    Expand-AllTrees
    if ($ScreenshotPath) {
        Add-Type -AssemblyName System.Drawing
        (Find-Name 'Take snapshot').SetFocus()
        $rect = $root.Current.BoundingRectangle
        $bitmap = [System.Drawing.Bitmap]::new([int]$rect.Width, [int]$rect.Height)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.CopyFromScreen([int]$rect.Left, [int]$rect.Top, 0, 0, $bitmap.Size)
            $bitmap.Save($ScreenshotPath, [System.Drawing.Imaging.ImageFormat]::Png)
        } finally { $graphics.Dispose(); $bitmap.Dispose() }
    }
    Invoke-Button 'Disconnect'
    Wait-For { (Find-Name 'Attach').Current.IsEnabled } 'disconnecting'
    Wait-For { (Find-Id 'StatisticsStatus').Current.Name -like 'Process statistics stopped.*' } 'stopping process sampling'
    $stoppedRam = (Find-Id 'RamStatistics').Current.Name
    Start-Sleep -Milliseconds 1500
    if ((Find-Id 'RamStatistics').Current.Name -ne $stoppedRam) { throw 'Process sampling continued after disconnect.' }
    if ((Find-Id 'SnapshotStatistics').Current.Name -ne $snapshotAStatistics) {
        throw 'Disconnect changed the retained snapshot statistics.'
    }
    if ((Memory-Reading 'Measured core object allocations') -ne $snapshotAMemory) {
        throw 'Disconnect changed the retained snapshot memory.'
    }
    if (!$gpuCheckbox.Current.IsEnabled) { throw 'Tracing configuration remained disabled after disconnect.' }
    Invoke-Button 'Attach'
    Wait-For {
        (Find-Id 'StatisticsStatus').Current.Name -like 'Live process statistics | PID *' -and
        (Find-Id 'RamStatistics').Current.Name -match '\d+[\.,]\d+ MiB'
    } 'restarting process sampling after reattach'
    if ((Find-Id 'SnapshotStatistics').Current.Name -ne $snapshotAStatistics) {
        throw 'Reattaching changed the retained snapshot statistics.'
    }
    Invoke-Button 'Disconnect'
    Write-Output 'Passed: launch/attach, opt-in tracing control, live statistics, frozen snapshot memory, expand all trees, cross-tree selection, retained history, stopped sampling on disconnect, and reattach.'
}
finally {
    $profiler.Refresh()
    if (!$profiler.HasExited) {
        $null = $profiler.CloseMainWindow()
        if (!$profiler.WaitForExit(5000)) { Stop-Process -Id $profiler.Id }
        elseif ($profiler.ExitCode -ne 0) { throw "Profiler exited with code $($profiler.ExitCode)." }
    }
}

param(
    [Parameter(Mandatory)][string]$ProfilerPath,
    [Parameter(Mandatory, ParameterSetName='Attach')][uint32]$TargetProcessId,
    [Parameter(Mandatory, ParameterSetName='Launch')][string]$TargetExecutable
)
$ErrorActionPreference = 'Stop'
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
    throw "Timed out while $Description."
}

function Find-Name([string]$Name) {
    $root.FindFirst($scope, [System.Windows.Automation.PropertyCondition]::new($nameProperty, $Name))
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
    ([System.Windows.Automation.ValuePattern]$control.GetCurrentPattern(
        [System.Windows.Automation.ValuePattern]::Pattern)).SetValue($Value)
}

function Selected-Snapshot {
    $combo = Find-Name 'Captured snapshots'
    $selection = [System.Windows.Automation.SelectionPattern]$combo.GetCurrentPattern(
        [System.Windows.Automation.SelectionPattern]::Pattern)
    @($selection.Current.GetSelection() | ForEach-Object { $_.Current.Name }) -join '; '
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
            $names = @(Tree-Items $id | ForEach-Object { $_.Current.Name })
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
    if ($PSCmdlet.ParameterSetName -eq 'Launch') {
        Set-Text 'Executable path' $TargetExecutable
        Invoke-Button 'Launch executable'
    } else {
        Set-Text 'Running process ID' $TargetProcessId.ToString()
        Invoke-Button 'Attach'
    }
    Wait-For { (Find-Name 'Take snapshot').Current.IsEnabled } 'connecting to the target'
    if ($PSCmdlet.ParameterSetName -eq 'Launch') {
        $pidBox = Find-Name 'Running process ID'
        $launchedPid = ([System.Windows.Automation.ValuePattern]$pidBox.GetCurrentPattern(
            [System.Windows.Automation.ValuePattern]::Pattern)).Current.Value
        Write-Output "Launched target PID: $launchedPid"
    }
    foreach ($number in 1..2) {
        Invoke-Button 'Take snapshot'
        Wait-For { (Selected-Snapshot) -like "#$number |*" } "publishing snapshot $number"
    }

    foreach ($id in @('XamlTree', 'CompTree', 'VisualTree')) {
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
    Expand-AllTrees
    Invoke-Button 'Disconnect'
    Wait-For { (Find-Name 'Attach').Current.IsEnabled } 'disconnecting'
    Write-Output 'Passed: launch/attach, snapshots A and B, expand all three trees including descendants, cross-tree selection, retained history, and disconnect.'
}
finally {
    $profiler.Refresh()
    if (!$profiler.HasExited) {
        $null = $profiler.CloseMainWindow()
        if (!$profiler.WaitForExit(5000)) { Stop-Process -Id $profiler.Id }
    }
}

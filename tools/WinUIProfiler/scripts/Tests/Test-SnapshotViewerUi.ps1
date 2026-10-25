param(
    [Parameter(Mandatory = $true)][int]$ProfilerProcessId,
    [Parameter(Mandatory = $true)][long]$ProfilerWindowHandle,
    [Parameter(Mandatory = $true)][string[]]$SnapshotPaths,
    [switch]$VerifyLoadFailure,
    [switch]$LeaveOpen
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$main = [System.Windows.Automation.AutomationElement]::FromHandle([IntPtr]$ProfilerWindowHandle)
if ($main.Current.ProcessId -ne $ProfilerProcessId) { throw 'Profiler window identity changed.' }

function Find-Element($root, [string]$name) {
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        $element = $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::NameProperty, $name))
        if (!$element) { Start-Sleep -Milliseconds 100 }
    } while (!$element -and [DateTime]::UtcNow -lt $deadline)
    if (!$element) { throw "Missing snapshot UI element: $name" }
    return $element
}
function Get-Viewer([string]$captureId) {
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do {
        $viewers = @([System.Windows.Automation.AutomationElement]::RootElement.FindAll(
            [System.Windows.Automation.TreeScope]::Children,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $ProfilerProcessId)) |
            Where-Object { $_.Current.Name -eq "Snapshot viewer - $captureId" })
        if (!$viewers.Count) { Start-Sleep -Milliseconds 100 }
    } while (!$viewers.Count -and [DateTime]::UtcNow -lt $deadline)
    if ($viewers.Count -ne 1) {
        $details = $main.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) | ForEach-Object { $_.Current.Name }
        throw "Expected one viewer for $captureId; found $($viewers.Count). Main UI: $($details -join ' | ')"
    }
    return $viewers[0]
}
function Open-Snapshot([string]$captureId) {
    $history = Find-Element $main 'Snapshot history'
    $item = $null
    foreach ($percent in @(0, 50, 100)) {
        $scroll = $null
        if ($history.TryGetCurrentPattern([System.Windows.Automation.ScrollPattern]::Pattern, [ref]$scroll) -and
            $scroll.Current.VerticallyScrollable) {
            $scroll.SetScrollPercent(-1, $percent)
            Start-Sleep -Milliseconds 100
        }
        $item = $history.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
                [System.Windows.Automation.ControlType]::ListItem)) |
            Where-Object { $_.Current.Name -like "*$captureId*" } | Select-Object -First 1
        if ($item) { break }
    }
    if (!$item) { throw "Snapshot history does not expose capture $captureId." }
    $item.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    return Get-Viewer $captureId
}
function Get-Depth($nodes) {
    $index = @{}
    foreach ($node in $nodes) { $index[[string]$node.id] = $node }
    $maximum = 0
    foreach ($node in $nodes) {
        $depth = 1
        while ($null -ne $node.parentId) { $node = $index[[string]$node.parentId]; $depth++ }
        $maximum = [Math]::Max($maximum, $depth)
    }
    return $maximum
}
function Test-Metric($viewer, [string]$name, [int]$expected) {
    $control = Find-Element $viewer $name
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        if ($control.Current.ItemStatus -eq [string]$expected) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($control.Current.ItemStatus -ne [string]$expected) {
        throw "$name expected $expected, got $($control.Current.ItemStatus)."
    }
    $text = $control.GetCurrentPattern([System.Windows.Automation.TextPattern]::Pattern).DocumentRange.GetText(-1).Trim()
    if ($text -ne [string]$expected) { throw "$name rendered '$text', not $expected." }
}
function Get-NodeLabel($node) {
    $label = "#$($node.id) $(($node.type -split '\.')[-1])"
    if ($node.name) { $label += " ($($node.name))" }
    return $label
}
function Find-TreeItem($tree, [string]$label) {
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        $item = $tree.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
                [System.Windows.Automation.ControlType]::TreeItem)) |
            Where-Object { $_.Current.Name -eq $label } | Select-Object -First 1
        if (!$item) { Start-Sleep -Milliseconds 100 }
    } while (!$item -and [DateTime]::UtcNow -lt $deadline)
    if (!$item) { throw "Missing rendered TreeView item: $label" }
    return $item
}
function Select-Node($tree, $node, $allNodes, $roots) {
    $index = @{}
    foreach ($candidate in $allNodes) { $index[[string]$candidate.id] = $candidate }
    $path = [System.Collections.Generic.Stack[object]]::new()
    $current = $node
    $path.Push($current)
    while ($null -ne $current.parentId) { $current = $index[[string]$current.parentId]; $path.Push($current) }
    $root = $roots | Where-Object { $_.id -eq $node.rootId } | Select-Object -First 1
    $group = Find-TreeItem $tree "XamlRoot #$($root.id) | thread $($root.threadId)"
    $expand = $group.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
    if ($expand.Current.ExpandCollapseState -eq [System.Windows.Automation.ExpandCollapseState]::Collapsed) { $expand.Expand() }
    while ($path.Count) {
        $current = $path.Pop()
        $item = Find-TreeItem $tree (Get-NodeLabel $current)
        if ($path.Count) {
            $expand = $item.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
            if ($expand.Current.ExpandCollapseState -eq [System.Windows.Automation.ExpandCollapseState]::Collapsed) { $expand.Expand() }
        }
    }
    $item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
    return $item
}
function Wait-Mapping($viewer, [string]$expected) {
    $status = Find-Element $viewer 'Snapshot mapping status'
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        if ($status.Current.ItemStatus -eq $expected) { return }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Mapping expected '$expected', got '$($status.Current.ItemStatus)'."
}
function Test-RevealedLabel($item, [string]$label) {
    $text = Find-Element $item $label
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        if (!$text.Current.IsOffscreen) { return }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Selected mapping label was not scrolled into view: $label"
}
function Test-Mappings($viewer, $document) {
    $xamlTree = Find-Element $viewer 'Snapshot XAML tree'
    $visualTree = Find-Element $viewer 'Snapshot composition tree'
    $unmapped = $document.compositionNodes | Where-Object {
        $id = $_.id
        $null -eq $_.parentId -and !($document.correlations | Where-Object { $_.visualNodeId -eq $id })
    } | Select-Object -First 1
    if (!$unmapped) { throw 'This UI fixture requires an unmapped composition root.' }
    $null = Select-Node $visualTree $unmapped $document.compositionNodes $document.roots
    Wait-Mapping $viewer "xaml=;visual=$($unmapped.id)"
    $selectedXaml = $xamlTree.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.SelectionItemPattern]::IsSelectedProperty, $true))
    if ($selectedXaml.Count) { throw 'Unmapped composition node left a guessed XAML selection.' }

    $link = $document.correlations | Select-Object -First 1
    $xaml = $document.xamlNodes | Where-Object { $_.id -eq $link.xamlNodeId } | Select-Object -First 1
    $null = Select-Node $xamlTree $xaml $document.xamlNodes $document.roots
    $visualIds = @($document.correlations | Where-Object { $_.xamlNodeId -eq $xaml.id } |
        Select-Object -ExpandProperty visualNodeId | Sort-Object -Unique)
    Wait-Mapping $viewer "xaml=$($xaml.id);visual=$($visualIds -join ',')"
    $primary = $document.compositionNodes | Where-Object { $_.id -eq $visualIds[0] } | Select-Object -First 1
    $visualItem = Find-TreeItem $visualTree (Get-NodeLabel $primary)
    $selection = $visualItem.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern)
    if (!$selection.Current.IsSelected) { throw 'XAML selection did not auto-select its composition counterpart.' }
    Test-RevealedLabel $visualItem (Get-NodeLabel $primary)
    $chooser = Find-Element $viewer 'Mapped snapshot nodes'
    if ($chooser.Current.ItemStatus -ne "target=Composition;nodes=$($visualIds -join ',')") {
        throw "Programmatic counterpart selection reversed the mapping direction: $($chooser.Current.ItemStatus)"
    }
    $expand = $chooser.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
    $expand.Expand()
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        $choice = $viewer.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
                [System.Windows.Automation.ControlType]::ListItem)) |
            Where-Object { $_.Current.Name -eq (Get-NodeLabel $primary) } | Select-Object -First 1
        if (!$choice) { Start-Sleep -Milliseconds 100 }
    } while (!$choice -and [DateTime]::UtcNow -lt $deadline)
    if (!$choice) {
        $names = $viewer.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) | ForEach-Object { "$($_.Current.ControlType.ProgrammaticName):$($_.Current.Name)" }
        throw "Mapped-node chooser does not display $(Get-NodeLabel $primary). UI: $($names -join ' | ')"
    }
    $choice.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
    $expand.Collapse()
    $selection.RemoveFromSelection()
    $selection.Select()
    $xamlIds = @($document.correlations | Where-Object { $_.visualNodeId -eq $primary.id } |
        Select-Object -ExpandProperty xamlNodeId | Sort-Object -Unique)
    Wait-Mapping $viewer "xaml=$($xamlIds -join ',');visual=$($primary.id)"
    if ($chooser.Current.ItemStatus -ne "target=Xaml;nodes=$($xamlIds -join ',')") {
        throw "Reverse selection did not update the chooser direction: $($chooser.Current.ItemStatus)"
    }
    $primaryXaml = $document.xamlNodes | Where-Object { $_.id -eq $xamlIds[0] } | Select-Object -First 1
    $xamlItem = Find-TreeItem $xamlTree (Get-NodeLabel $primaryXaml)
    if (!$xamlItem.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Current.IsSelected) {
        throw 'Composition selection did not auto-select its XAML counterpart.'
    }
    Test-RevealedLabel $xamlItem (Get-NodeLabel $primaryXaml)
}

$history = Find-Element $main 'Snapshot history'
if ([int]$history.Current.ItemStatus -ne $SnapshotPaths.Count -or $SnapshotPaths.Count -lt 2) {
    throw "Expected $($SnapshotPaths.Count) history entries (at least two), got $($history.Current.ItemStatus)."
}
foreach ($path in $SnapshotPaths) {
    $document = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $viewer = Open-Snapshot $document.captureId
    $identity = Find-Element $viewer 'Snapshot capture identity'
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do { Start-Sleep -Milliseconds 100 } while (
        $identity.Current.ItemStatus -ne $document.captureId -and [DateTime]::UtcNow -lt $deadline)
    if ($identity.Current.ItemStatus -ne $document.captureId) { throw 'Viewer did not finish loading its own capture.' }
    Test-Metric $viewer 'Snapshot XAML node count' $document.xamlNodes.Count
    Test-Metric $viewer 'Snapshot composition visual count' $document.compositionNodes.Count
    Test-Metric $viewer 'Snapshot XAML tree depth' (Get-Depth $document.xamlNodes)
    Test-Metric $viewer 'Snapshot composition tree depth' (Get-Depth $document.compositionNodes)
    Test-Metric $viewer 'Snapshot root count' $document.roots.Count
    Test-Metric $viewer 'Snapshot mapping count' $document.correlations.Count
    $histogram = @($document.compositionNodes | Group-Object type |
        ForEach-Object { "$($_.Name)=$($_.Count)" } | Sort-Object)
    $displayed = @((Find-Element $viewer 'Composition visual type counts').Current.ItemStatus -split "`n" | Sort-Object)
    if (@(Compare-Object $histogram $displayed).Count) { throw 'Displayed visual-type histogram differs from the saved tree.' }
    Test-Mappings $viewer $document
    $handle = $viewer.Current.NativeWindowHandle
    $reused = Open-Snapshot $document.captureId
    if ($reused.Current.NativeWindowHandle -ne $handle) { throw 'Opening the same capture created a duplicate viewer.' }
    $viewer.GetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern).Close()
    Write-Output "SNAPSHOT_VIEWER_UI_PASS capture=$($document.captureId) xaml=$($document.xamlNodes.Count) visuals=$($document.compositionNodes.Count) xamlDepth=$(Get-Depth $document.xamlNodes) visualDepth=$(Get-Depth $document.compositionNodes) types=$($histogram -join ';')"
}
if ($VerifyLoadFailure) {
    $path = $SnapshotPaths[0]
    $captureId = (Get-Content -LiteralPath $path -Raw | ConvertFrom-Json).captureId
    $temporary = "$path.viewer-test"
    if (Test-Path -LiteralPath $temporary) { throw 'Viewer failure-test backup already exists.' }
    Move-Item -LiteralPath $path -Destination $temporary
    try {
        $viewer = Open-Snapshot $captureId
        $null = Find-Element $viewer 'Snapshot could not be displayed'
        $viewer.GetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern).Close()
        Write-Output 'SNAPSHOT_VIEWER_MISSING_FILE_ERROR_PASS'
    }
    finally { Move-Item -LiteralPath $temporary -Destination $path }
}
if ($LeaveOpen) {
    $document = Get-Content -LiteralPath $SnapshotPaths[0] -Raw | ConvertFrom-Json
    $null = Open-Snapshot $document.captureId
}
Write-Output "SNAPSHOT_HISTORY_UI_PASS entries=$($SnapshotPaths.Count)"

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

param([string]$SampleRoot = (Split-Path $PSScriptRoot -Parent))

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Xml.Linq
$x = [System.Xml.Linq.XNamespace]'http://schemas.microsoft.com/winfx/2006/xaml'
$failures = [System.Collections.Generic.List[string]]::new()
$documents = @{}
$checks = 0

function Assert-Source($Condition, [string]$Message) {
    $script:checks++
    if (-not $Condition) { $script:failures.Add($Message) }
}

function Read-Xaml([string]$File) {
    if (-not $documents.ContainsKey($File)) {
        $documents[$File] = [System.Xml.Linq.XDocument]::Load(
            (Join-Path $SampleRoot $File), [System.Xml.Linq.LoadOptions]::SetLineInfo)
    }
    return $documents[$File]
}

function Find-Named([string]$File, [string]$Name) {
    return (Read-Xaml $File).Descendants() |
        Where-Object { $_.Attribute($x + 'Name').Value -eq $Name }
}

function Assert-Label([string]$File, [string]$Name, [string]$Label) {
    $control = @(Find-Named $File $Name)
    Assert-Source ($control.Count -eq 1) "$File must contain one $Name."
    if ($control.Count -eq 1) {
        Assert-Source ($control[0].Attribute('AutomationProperties.LabeledBy').Value -eq
            "{Binding ElementName=$Label}") "$File $Name must use its visible $Label."
    }
    $target = @(Find-Named $File $Label)
    Assert-Source ($target.Count -eq 1 -and
        -not [string]::IsNullOrWhiteSpace($target[0].Attribute('Text').Value)) "$File $Label must contain visible label text."
}

function Assert-Name([string]$File, [string]$Name, [string]$Expected) {
    $control = @(Find-Named $File $Name)
    Assert-Source ($control.Count -eq 1) "$File must contain one $Name."
    if ($control.Count -eq 1) {
        Assert-Source ($control[0].Attribute('AutomationProperties.Name').Value -eq $Expected) "$File $Name must expose '$Expected'."
    }
}

foreach ($pair in @(
    @('FilterBox', 'FilterLabel'), @('GroupCombo', 'GroupLabel'),
    @('SortCombo', 'SortLabel'), @('CycleCombo', 'CycleLabel'))) {
    Assert-Label 'ShapingPage.xaml' $pair[0] $pair[1]
}
Assert-Name 'ShapingPage.xaml' 'DirectionCombo' 'Sort direction'
Assert-Name 'ShapingPage.xaml' 'ViaCombo' 'Sort API'
Assert-Label 'Pages\TextWrapPage.xaml' 'ColumnWidthSlider' 'ColumnWidthLabel'
Assert-Label 'Pages\TextWrapPage.xaml' 'MaxLinesComboBox' 'MaxLinesLabel'
Assert-Label 'Pages\FilePropertiesPage.xaml' 'FileList' 'FilesLabel'
Assert-Name 'Pages\TaskManagerPage.xaml' 'ViewModeCombo' 'Process view'
Assert-Name 'Pages\TaskManagerPage.xaml' 'SearchBox' 'Search processes'
Assert-Name 'Pages\FileExplorerPage.xaml' 'AddressBar' 'Folder path'
Assert-Name 'Pages\FileExplorerPage.xaml' 'BackButton' 'Back'
Assert-Name 'Pages\FileExplorerPage.xaml' 'UpButton' 'Up'
$refresh = @((Read-Xaml 'Pages\FileExplorerPage.xaml').Descendants() |
    Where-Object { $_.Attribute('Click').Value -eq 'OnRefreshClick' })
Assert-Source ($refresh.Count -eq 1 -and $refresh[0].Attribute('AutomationProperties.Name').Value -eq 'Refresh') 'File Explorer refresh icon must be named.'

$templateNames = @{
    'Pages\DensityReadOnlyPage.xaml' = @{
        FirstNameCell = 'First name'; LastNameCell = 'Last name'; RoleCell = 'Role'; EmailCell = 'Email'
    }
    'Pages\MixedControlsPage.xaml' = @{
        JoinDateCell = '{Binding JoinDateAccessibleName, Mode=OneWay}'
        ShiftStartCell = '{Binding ShiftStartAccessibleName, Mode=OneWay}'
        DepartmentCell = 'Department'; IsActiveCell = 'Active'
    }
    'Pages\CellFlyoutsPage.xaml' = @{
        DatePickerCell = 'Join date'; DepartmentComboCell = 'Department'
    }
}
foreach ($file in $templateNames.Keys) {
    foreach ($key in $templateNames[$file].Keys) {
        $template = @((Read-Xaml $file).Descendants() |
            Where-Object { $_.Attribute($x + 'Key').Value -eq $key })
        Assert-Source ($template.Count -eq 1) "$file must contain one $key template."
        if ($template.Count -ne 1) { continue }
        $control = @($template[0].Elements())[0]
        Assert-Source ($control.Attribute('AutomationProperties.Name').Value -eq $templateNames[$file][$key]) "$file $key must expose its field name."
        if ($file -like '*MixedControls*') {
            Assert-Source ($control.Attribute('IsTabStop').Value -eq 'False' -and
                $control.Attribute('IsHitTestVisible').Value -eq 'False') "$file $key must remain a read-only display example."
            Assert-Source ($control.Attribute('IsEnabled').Value -eq 'False') "$file $key must expose disabled semantics, not enabled-looking inert controls."
            $valueProperty = @{
                DatePicker = 'Date'; TimePicker = 'Time'; ComboBox = 'SelectedItem'; CheckBox = 'IsChecked'
            }[$control.Name.LocalName]
            Assert-Source ($control.Attribute($valueProperty).Value -match 'Mode=OneWay') "$file $key must not write display-only values back to the model."
        }
        if ($file -like '*DensityReadOnly*') {
            Assert-Source ($control.Attribute('Text').Value -match 'Mode=TwoWay' -and
                $control.Attribute('Text').Value -notmatch 'UpdateSourceTrigger=Explicit') "$file $key must remain an ordinary bound TextBox, not a framework transaction."
        }
    }
}

$rowActions = @{
    UpdateRowButton = @('Update role', 'OnUpdateRowClick', 'VirtualizationUpdateRowButton')
    InsertRowButton = @('Insert row', 'OnInsertRowClick', 'VirtualizationInsertRowButton')
    RemoveRowButton = @('Remove row', 'OnRemoveRowClick', 'VirtualizationRemoveRowButton')
    ResetRowsButton = @('Reset rows', 'OnResetRowsClick', 'VirtualizationResetRowsButton')
}
foreach ($name in $rowActions.Keys) {
    $button = @(Find-Named 'Pages\VirtualizationPage.xaml' $name)
    Assert-Source ($button.Count -eq 1) "Virtualization must contain $name."
    if ($button.Count -eq 1) {
        Assert-Source ($button[0].Attribute('Content').Value -eq $rowActions[$name][0] -and
            $button[0].Attribute('Click').Value -eq $rowActions[$name][1] -and
            $button[0].Attribute('AutomationProperties.AutomationId').Value -eq $rowActions[$name][2]) "Virtualization $name must be labeled and wired to its real mutation action."
    }
}

$performance = Read-Xaml 'Pages\PerformancePage.xaml'
foreach ($id in @('Load10k', 'Load100k', 'Sort', 'Filter', 'ClearFilter')) {
    $button = @($performance.Descendants() |
        Where-Object { $_.Attribute('AutomationProperties.AutomationId').Value -eq "${id}Button" })
    Assert-Source ($button.Count -eq 1 -and $button[0].Attribute('AutomationProperties.LabeledBy').Value -eq
        "{Binding ElementName=${id}Label}") "Performance ${id} Run button must identify its scenario."
}
foreach ($name in @('Load10kResult', 'Load100kResult', 'SortResult', 'FilterResult', 'ClearFilterResult', 'SnapshotResult', 'RunAllResult')) {
    $result = @(Find-Named 'Pages\PerformancePage.xaml' $name)
    Assert-Source ($result.Count -eq 1 -and $result[0].Attribute('AutomationProperties.LiveSetting').Value -eq 'Polite') "Performance $name must be a polite live region."
}

foreach ($file in @('Controls\SamplePresenter.xaml', 'Controls\SamplePageHeader.xaml')) {
    $headings = @((Read-Xaml $file).Descendants() |
        Where-Object { $_.Attribute('AutomationProperties.HeadingLevel').Value -eq 'Level1' })
    Assert-Source ($headings.Count -eq 1 -and $headings[0].Attribute('TextWrapping').Value -eq 'Wrap') "$file must have one wrapping page heading."
}

$nav = @((Read-Xaml 'MainWindow.xaml').Descendants() |
    Where-Object { $_.Name.LocalName -eq 'NavigationViewItem' -and $_.Attribute('Tag') })
Assert-Source ($nav.Count -eq 29) 'The gallery must retain all 29 navigation destinations.'
$map = Get-Content (Join-Path $SampleRoot 'MainWindow.xaml.cs') -Raw
foreach ($item in $nav) {
    $tag = $item.Attribute('Tag').Value
    Assert-Source ($map.Contains('["' + $tag + '"] = typeof(')) "Navigation destination $tag must resolve to a page."
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Output "FAIL: $_" }
    throw "$($failures.Count) of $checks sample accessibility source checks failed."
}
Write-Output "PASS: $checks sample accessibility source checks. These do not replace build, UIA, keyboard, or Narrator validation."

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
// Tabular aliases keep the sample code concise.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Single-page RTL fixture for the TableView's RTL-sensitive call sites: column flow,
/// resize gripper alignment, sort-chevron edge, group-header indent, and the in-place
/// editor's caret order.
///
/// Sorting, filtering, grouping and editing are all enabled here deliberately. RTL bugs
/// surface in the adornments those features add - the chevron, the group header, the
/// editor caret - not in a plain grid, so a read-only unsorted table cannot expose them.
///
/// Data is fully synthetic (Row N, Dept N). 50 rows so virtualization engages.
/// </summary>
public sealed partial class RTLPlaygroundPage : Page
{
    public ObservableCollection<RtlRow> Rows { get; } = new();

    // One source for the page. Filter/GroupBy mutate in place and return the same
    // instance (TableViewSource.cpp:49-50), so the projection reshapes live rather than
    // being rebuilt and reassigned - which would drop selection, scroll offset and group
    // expansion on every keystroke.
    private TableViewSource? _source;

    // Requested shaping, straight off the pickers.
    private string _shapingMode = "flat";
    private string _groupKey = "Department";

    // Mirror the request only once GroupBy / ClearGroupBy has actually returned, so no
    // readout and no enable/disable guard can claim a grouping the source never took.
    private string _appliedMode = "flat";
    private string _appliedGroupKey = "none";

    public RTLPlaygroundPage()
    {
        InitializeComponent();

        for (int i = 0; i < 50; i++)
        {
            Rows.Add(new RtlRow
            {
                Index = i + 1,
                Name = $"Row {i + 1}",
                Department = $"Dept {(i % 6) + 1}",
                Role = $"Role {(i % 4) + 1}",
                Region = $"Region {(i % 5) + 1}",
                JoinDate = $"2026-{((i % 12) + 1):D2}-{((i % 28) + 1):D2}",
                Notes = $"Notes for row {i + 1}",
                Salary = 50000 + (i * 750),
                Status = (i % 3 == 0) ? "Active" : "Inactive",
            });
        }

        _source = TableViewSource.From(Rows);
        PlaygroundTable.ItemsSource = _source;

        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
    }

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        PlaygroundTable.SelectionChanged += OnSelectionChanged;
        UpdateReadout();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        if (PlaygroundTable != null)
        {
            PlaygroundTable.SelectionChanged -= OnSelectionChanged;
        }
    }

    private void OnRtlToggled(object sender, RoutedEventArgs e)
    {
        if (sender is ToggleSwitch toggle && PlaygroundTable != null)
        {
            PlaygroundTable.FlowDirection = toggle.IsOn
                ? FlowDirection.RightToLeft
                : FlowDirection.LeftToRight;
            UpdateReadout();
        }
    }

    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        var order = new[] { IndexColumn, NameColumn, DepartmentColumn, RoleColumn, RegionColumn, JoinDateColumn, NotesColumn, SalaryColumn, StatusColumn };
        for (int target = 0; target < order.Length; target++)
        {
            var current = PlaygroundTable.Columns.IndexOf(order[target]);
            if (current >= 0 && current != target)
            {
                var moved = PlaygroundTable.Columns[current];
                PlaygroundTable.Columns.RemoveAt(current);
                PlaygroundTable.Columns.Insert(target, moved);
            }
        }

        LastReorderText.Text = "Reset column order.";
    }

    private void OnFilterChanged(object sender, TextChangedEventArgs e) => ApplyShaping();

    // ShapingModeSelector / GroupBySelector set SelectedIndex in XAML, which raises
    // SelectionChanged during InitializeComponent — before _source, the table and the
    // later-declared buttons and readouts exist. ApplyShaping bails while _source is null
    // and every control it touches is null-guarded.
    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _shapingMode = tag;
        ApplyShaping();
    }

    private void OnGroupByChanged(object sender, SelectionChangedEventArgs e)
    {
        if (GroupBySelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        ApplyShaping();
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        PlaygroundTable?.ExpandAllGroups();
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        PlaygroundTable?.CollapseAllGroups();
    }

    private void OnClearShapingClick(object sender, RoutedEventArgs e)
    {
        if (FilterBox is not null) { FilterBox.Text = string.Empty; }
        if (ShapingModeSelector is not null) { ShapingModeSelector.SelectedIndex = 0; }
        _shapingMode = "flat";
        ApplyShaping();
    }

    /// <summary>
    /// Reshapes the single source in place. Filter and GroupBy compose, so a filtered
    /// grouped view is one projection rather than two passes over the data.
    /// </summary>
    private void ApplyShaping()
    {
        if (_source is null || PlaygroundTable is null)
        {
            return;
        }

        var term = FilterBox?.Text?.Trim() ?? string.Empty;
        if (string.IsNullOrEmpty(term))
        {
            _source.ClearFilter();
        }
        else
        {
            _source.Filter(item => item is RtlRow r && Matches(r, term));
        }

        if (_shapingMode != "grouped")
        {
            _source.ClearGroupBy();
            _appliedMode = "flat";
            _appliedGroupKey = "none";
        }
        else
        {
            var key = _groupKey;
            // The identity overload is used so group identity survives a reshape instead
            // of groups being recreated on every filter keystroke. The two delegates do NOT
            // receive the same thing: the key selector is handed the row item, the identity
            // selector is handed the group KEY produced above (TableViewSource.idl). Testing
            // the argument against RtlRow here would yield an empty identity, which fails
            // fast with E_INVALIDARG.
            _source.GroupBy(
                item => (object)GroupValue(item, key),
                groupKey => groupKey?.ToString() ?? "(none)");

            // Only now is grouping genuinely applied; every readout reads these, never the
            // requested _shapingMode / _groupKey.
            _appliedMode = "grouped";
            _appliedGroupKey = key;
        }

        // case "hierarchy":
        // case "groupedhierarchy":
        //     Hierarchical (tree) rows are not available in this release, which is why the two
        //     matching ComboBoxItems ship disabled with a tooltip rather than hidden. No
        //     hierarchy verb exists on TableViewSource or TableView today — the only trace in
        //     the control source is TableViewRowInfo.h, which reserves row metadata "when
        //     hierarchical (tree) rows land" — so this stub stays prose rather than naming a
        //     member that does not exist. When hierarchy ships, apply it to this same source
        //     here, alongside the Filter and GroupBy stages above so they compose instead of
        //     replacing one another, and set the applied-mode field only after it returns.

        UpdateReadout();
    }

    private static bool Matches(RtlRow r, string term) =>
        r.Name.Contains(term, StringComparison.CurrentCultureIgnoreCase)
        || r.Department.Contains(term, StringComparison.CurrentCultureIgnoreCase)
        || r.Role.Contains(term, StringComparison.CurrentCultureIgnoreCase)
        || r.Region.Contains(term, StringComparison.CurrentCultureIgnoreCase)
        || r.Status.Contains(term, StringComparison.CurrentCultureIgnoreCase);

    // Never returns the empty string: an empty group identity is an E_INVALIDARG fail-fast.
    private static string GroupValue(object item, string key)
    {
        if (item is not RtlRow r)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Department" => r.Department,
            "Region" => r.Region,
            "Status" => r.Status,
            _ => null,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private void OnSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs e) => UpdateReadout();

    private void UpdateReadout()
    {
        // Reachable during InitializeComponent (the RTL ToggleSwitch sets IsOn in XAML,
        // raising Toggled before the Options-rail readout TextBlocks exist). Guard so the
        // init-time call no-ops; OnPageLoaded re-runs UpdateReadout.
        if (PlaygroundTable is null || FlowDirectionText is null
            || RowsLoadedText is null || SelectedCountText is null)
        {
            return;
        }

        var grouped = _appliedMode == "grouped";
        var term = FilterBox?.Text?.Trim() ?? string.Empty;
        var filtered = !string.IsNullOrEmpty(term);

        // Expand/Collapse act on group containers, so they mean nothing on a flat table.
        // Disabled rather than hidden: hiding shifts the rail's layout every time the
        // shaping mode changes, and a disabled control still teaches the dependency.
        if (GroupBySelector is not null) { GroupBySelector.IsEnabled = grouped; }
        if (ExpandAllButton is not null) { ExpandAllButton.IsEnabled = grouped; }
        if (CollapseAllButton is not null) { CollapseAllButton.IsEnabled = grouped; }
        if (ClearShapingButton is not null) { ClearShapingButton.IsEnabled = grouped || filtered; }

        FlowDirectionText.Text = PlaygroundTable.FlowDirection.ToString();
        RowsLoadedText.Text = Rows.Count.ToString(CultureInfo.CurrentCulture);
        SelectedCountText.Text = (PlaygroundTable.SelectedItem is null ? 0 : 1).ToString(CultureInfo.CurrentCulture);

        if (GroupedByText is not null)
        {
            GroupedByText.Text = grouped ? _appliedGroupKey : "(none)";
        }

        if (ShownRowsText is not null)
        {
            // The projection exposes no count, so the predicate is re-run here purely for
            // the readout. A TableViewSource.Count would remove this duplicate work.
            var shown = filtered ? Rows.Count(r => Matches(r, term)) : Rows.Count;
            ShownRowsText.Text = shown.ToString(CultureInfo.CurrentCulture);
        }
    }
}

/// <summary>
/// Local row record for the RTL playground. Implements INotifyPropertyChanged so in-place
/// edits committed by the built-in text editor are reflected back in the cell. Without it
/// an edit writes to the object but the cell keeps showing the stale value, which reads as
/// "the column is not editable".
/// </summary>
public sealed class RtlRow : INotifyPropertyChanged
{
    private int _index;
    private string _name = string.Empty;
    private string _department = string.Empty;
    private string _role = string.Empty;
    private string _region = string.Empty;
    private string _joinDate = string.Empty;
    private string _notes = string.Empty;
    private double _salary;
    private string _status = string.Empty;

    public int Index { get => _index; set => Set(ref _index, value); }
    public string Name { get => _name; set => Set(ref _name, value); }
    public string Department { get => _department; set => Set(ref _department, value); }
    public string Role { get => _role; set => Set(ref _role, value); }
    public string Region { get => _region; set => Set(ref _region, value); }
    public string JoinDate { get => _joinDate; set => Set(ref _joinDate, value); }
    public string Notes { get => _notes; set => Set(ref _notes, value); }
    public double Salary { get => _salary; set => Set(ref _salary, value); }
    public string Status { get => _status; set => Set(ref _status, value); }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void Set<T>(ref T field, T value, [CallerMemberName] string? name = null)
    {
        if (Equals(field, value))
        {
            return;
        }

        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }
}

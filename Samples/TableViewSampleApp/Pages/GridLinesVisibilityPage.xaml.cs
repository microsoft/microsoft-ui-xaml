// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
using TableViewGridLinesVisibility = Microsoft.UI.Xaml.Controls.Tabular.TableViewGridLinesVisibility;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates TableView.GridLinesVisibility (None / Horizontal / Vertical / All)
/// for WPF DataGrid parity. The same DP is exercised across flat and grouped
/// modes — and against both default theme banding and a custom row background —
/// so reviewers can verify the lines stay correct through group headers and tinted rows.
///
/// The grouped mode is real grouping: one TableViewSource is reshaped in place
/// with GroupBy / ClearGroupBy, so the group header bands that the grid lines
/// have to coexist with are actually present.
/// </summary>
public sealed partial class GridLinesVisibilityPage : Page, INotifyPropertyChanged
{
    private readonly ObservableCollection<Person> _rows = new();

    private TableViewSource? _source;

    // Requested shaping mode versus the mode actually applied to the source.
    // GroupBy fails fast, so these can differ; every readout and every
    // enable/disable guard reads _appliedMode.
    private string _mode = "flat";
    private string _appliedMode = "flat";
    private string _groupKey = "Department";
    private bool _allGroupsCollapsed;

    private TableViewGridLinesVisibility _lines = TableViewGridLinesVisibility.All;
    private string _statusText = string.Empty;

    public GridLinesVisibilityPage()
    {
        foreach (var person in PersonData.Take(40))
        {
            _rows.Add(person);
        }

        InitializeComponent();

        DemoTable.HeadersVisibility = TableViewHeadersVisibility.Column;

        _source = TableViewSource.From(_rows);
        DemoTable.ItemsSource = _source;

        ApplyShaping();
        ApplyBanding();
    }

    public string StatusText
    {
        get => _statusText;
        private set
        {
            if (_statusText != value)
            {
                _statusText = value;
                OnPropertyChanged();
            }
        }
    }

    private void OnGridLinesVisibilitySelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (DemoTable is null ||
            sender is not RadioButtons { SelectedItem: FrameworkElement { Tag: string tag } })
        {
            return;
        }

        // Case-sensitive on purpose so a typo trips Debug.Fail rather than
        // silently defaulting to Horizontal.
        if (!Enum.TryParse<TableViewGridLinesVisibility>(tag, ignoreCase: false, out var value))
        {
            Debug.Fail($"GridLinesVisibilityPage: unrecognised grid-lines Tag '{tag}'.");
            return;
        }

        _lines = value;
        DemoTable.GridLinesVisibility = value;
        UpdateStatus();
    }

    private void OnBandingSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (DemoTable is null)
        {
            return;
        }

        ApplyBanding();
        UpdateStatus();
    }

    private void ApplyBanding()
    {
        if (DemoTable is null || CustomBandingRadio is null) return;

        if (CustomBandingRadio.IsChecked == true)
        {
            DemoTable.Style = (Style)Resources["CustomBandingTableViewStyle"];
            return;
        }

        DemoTable.Style = null;
    }

    // ----- Shaping -----

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null ||
            sender is not ComboBox { SelectedItem: ComboBoxItem { Tag: string tag } })
        {
            return;
        }

        _mode = tag;
        ApplyShaping();
    }

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || GroupKeyCombo?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        // Re-applying with the same mode proves the grouped state survives a key change.
        ApplyShaping();
    }

    private void ApplyShaping()
    {
        if (_source is null)
        {
            return;
        }

        // Reshape IN PLACE: Filter/GroupBy mutate and return the same instance,
        // so the source is never rebuilt per change.
        if (_mode != "grouped")
        {
            _source.ClearGroupBy();
        }
        else
        {
            var key = _groupKey;
            // The two delegates receive DIFFERENT things despite both parameters
            // being named `item`:
            //   TableViewKeySelector(Object item)      -> receives the ROW ITEM
            //   TableViewIdentitySelector(Object item) -> receives the GROUP KEY
            // An item-typed identity lambda returns empty, which is a fail-fast,
            // so grouping would silently never apply.
            _source.GroupBy(
                item => (object)GroupValue(item, key),
                groupKey => groupKey?.ToString() ?? "(none)");
        }

        // case "hierarchy":
        // case "groupedhierarchy":
        //     Hierarchical rows are not available in this release: neither
        //     TableViewSource.idl nor TableView.idl exposes a hierarchy verb
        //     (the only shaping verbs are Filter / GroupBy / Sort and their
        //     Clear* counterparts). When the control ships hierarchy support,
        //     apply it to this same source here, alongside the GroupBy stage
        //     above so the two compose, then set _appliedMode as below.

        _appliedMode = _mode;
        _allGroupsCollapsed = false;

        if (_appliedMode == "grouped")
        {
            DemoTable.ExpandAllGroups();
        }

        UpdateShapingGates();
        UpdateStatus();
    }

    private void UpdateShapingGates()
    {
        if (GroupKeyCombo is null)
        {
            return;
        }

        var grouped = _appliedMode == "grouped";

        GroupKeyCombo.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
        ReassignRowButton.IsEnabled = grouped;
    }

    // ----- Actions -----

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped") return;

        DemoTable.ExpandAllGroups();
        _allGroupsCollapsed = false;
        UpdateStatus("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped") return;

        DemoTable.CollapseAllGroups();
        _allGroupsCollapsed = true;
        UpdateStatus("Collapsed all groups");
    }

    private void OnReassignRowClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped" || _rows.Count == 0) return;

        var person = _rows[0];
        var before = GroupValue(person, _groupKey);

        if (_groupKey == "Active")
        {
            person.IsActive = !person.IsActive;
        }
        else
        {
            person.Department = PersonData.Departments
                .FirstOrDefault(d => !string.Equals(d, person.Department, StringComparison.Ordinal))
                ?? person.Department;
        }

        UpdateStatus($"Moved \"{person.FullName}\" from {before} to {GroupValue(person, _groupKey)}");
    }

    private void OnRemoveRowClick(object sender, RoutedEventArgs e)
    {
        if (_rows.Count == 0)
        {
            UpdateStatus("Remove skipped — no rows left");
            return;
        }

        var person = _rows[0];
        _rows.RemoveAt(0);
        UpdateStatus($"Removed \"{person.FullName}\" — watch the row separator above it close up");
    }

    // ----- Helpers -----
    //
    // GroupValue never returns the empty string: an empty group identity is a
    // fail-fast in GroupBy, so a blank property has to be coalesced.

    private static string GroupValue(object item, string key)
    {
        if (item is not Person person)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Role" => person.Role,
            "Active" => person.IsActive ? "Active" : "Inactive",
            _ => person.Department,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private static string ModeLabel(string mode) => mode switch
    {
        "grouped" => "Grouped",
        "hierarchy" => "Hierarchy",
        "groupedhierarchy" => "Grouped hierarchy",
        _ => "Flat",
    };

    private void UpdateStatus(string? message = null)
    {
        var banding = CustomBandingRadio is not null && CustomBandingRadio.IsChecked == true ? "custom banding" : "theme banding";
        var lines = _lines switch
        {
            TableViewGridLinesVisibility.None       => "no grid lines",
            TableViewGridLinesVisibility.Horizontal => "horizontal lines (row separators)",
            TableViewGridLinesVisibility.Vertical   => "vertical lines (column separators)",
            TableViewGridLinesVisibility.All        => "all grid lines (rows + columns)",
            _ => _lines.ToString(),
        };

        // Readouts describe the APPLIED mode, never the requested one.
        var key = _groupKey;
        var shaping = _appliedMode == "grouped"
            ? $"{ModeLabel(_appliedMode)} by {key} · {_rows.Select(p => GroupValue(p, key)).Distinct(StringComparer.Ordinal).Count():N0} groups · {(_allGroupsCollapsed ? "all collapsed" : "all expanded")}"
            : ModeLabel(_appliedMode);

        var prefix = message is null ? string.Empty : $"{message}. ";
        StatusText = $"{prefix}{shaping} · {_rows.Count:N0} rows · {lines} · {banding}";
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}

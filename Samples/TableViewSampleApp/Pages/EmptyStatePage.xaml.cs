// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates TableView.EmptyTemplate — the DataTemplate the control renders,
/// centered over the row area, whenever ItemsSource is null or resolves to zero
/// rows. Three data sources separate the two ways of being empty (a null
/// ItemsSource and a live source that holds no items), and the shaping selector
/// covers the interesting edge case: grouping a set that has nothing in it
/// still has to show the empty state rather than an empty group band.
/// </summary>
public sealed partial class EmptyStatePage : Page, INotifyPropertyChanged
{
    private TableViewSource? _source;

    // Requested shaping mode versus the mode actually applied to the source.
    // GroupBy fails fast, so these can differ; every readout and every
    // enable/disable guard reads _appliedMode.
    private string _mode = "flat";
    private string _appliedMode = "flat";
    private string _groupKey = "Department";
    private bool _allGroupsCollapsed;
    private bool _sourceAttached = true;
    private string _statusText = string.Empty;

    public EmptyStatePage()
    {
        InitializeComponent();

        foreach (var person in PersonData.Take(20))
        {
            Rows.Add(person);
        }

        _source = TableViewSource.From(Rows);
        DemoTable.ItemsSource = _source;

        ApplyShaping();
    }

    public ObservableCollection<Person> Rows { get; } = new();

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

    private void OnDataSourceSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (DemoTable is null ||
            sender is not RadioButtons { SelectedItem: FrameworkElement { Tag: string tag } })
        {
            return;
        }

        switch (tag)
        {
            case "Null":
                // A null ItemsSource resolves to zero rows, so the control swaps in EmptyTemplate.
                _sourceAttached = false;
                DemoTable.ItemsSource = null;
                UpdateStatus("ItemsSource set to null");
                break;

            case "ZeroRows":
                // The other way of being empty: a live, shaped source that holds no items.
                _sourceAttached = true;
                DemoTable.ItemsSource = _source;
                Rows.Clear();
                UpdateStatus("Source kept, every row removed");
                break;

            default:
                _sourceAttached = true;
                DemoTable.ItemsSource = _source;
                if (Rows.Count == 0)
                {
                    RefillRows();
                }

                UpdateStatus("Rows restored");
                break;
        }

        UpdateShapingGates();
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
        // so the source is never rebuilt per change. Grouping an empty set is
        // legal — the projection simply yields no groups.
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

        if (_appliedMode == "grouped" && _sourceAttached && Rows.Count > 0)
        {
            DemoTable.ExpandAllGroups();
        }

        UpdateShapingGates();
        UpdateStatus($"Shaping applied: {ModeLabel(_appliedMode)}");
    }

    private void UpdateShapingGates()
    {
        if (GroupKeyCombo is null)
        {
            return;
        }

        var grouped = _appliedMode == "grouped";
        var hasRows = _sourceAttached && Rows.Count > 0;

        GroupKeyCombo.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped && hasRows;
        CollapseAllButton.IsEnabled = grouped && hasRows;
        ReassignRowButton.IsEnabled = grouped && hasRows;
        RemoveAllRowsButton.IsEnabled = hasRows;
        RestoreRowsButton.IsEnabled = !hasRows;
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
        if (_appliedMode != "grouped" || Rows.Count == 0) return;

        var person = Rows[0];
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

    private void OnRemoveAllRowsClick(object sender, RoutedEventArgs e)
    {
        // Emptying a GROUPED source: every group disappears with its last member,
        // and the EmptyTemplate has to take over the row area, not an empty band.
        Rows.Clear();
        DataSourceRadioButtons.SelectedIndex = 1;
        UpdateStatus("Removed every row while the source stayed attached");
    }

    private void OnRestoreRowsClick(object sender, RoutedEventArgs e)
    {
        _sourceAttached = true;
        DemoTable.ItemsSource = _source;
        RefillRows();
        DataSourceRadioButtons.SelectedIndex = 0;
        UpdateStatus("Restored the rows");
    }

    private void RefillRows()
    {
        foreach (var person in PersonData.Take(20))
        {
            Rows.Add(person);
        }
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
        var key = _groupKey;
        var groups = Rows.Select(p => GroupValue(p, key)).Distinct(StringComparer.Ordinal).Count();

        // Readouts describe the APPLIED mode, never the requested one.
        var shaping = _appliedMode == "grouped"
            ? $"Grouped by {key} · {groups:N0} groups · {(_allGroupsCollapsed ? "all collapsed" : "all expanded")}"
            : "Flat (no grouping)";

        string state;
        if (!_sourceAttached)
        {
            state = "ItemsSource is null · EmptyTemplate shown";
        }
        else if (Rows.Count == 0)
        {
            state = _appliedMode == "grouped"
                ? "Source attached but holds zero rows · no groups · EmptyTemplate shown"
                : "Source attached but holds zero rows · EmptyTemplate shown";
        }
        else
        {
            state = $"{Rows.Count:N0} rows · EmptyTemplate hidden";
        }

        var prefix = message is null ? string.Empty : $"{message}. ";
        StatusText = $"{prefix}{state} · {shaping}";
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}

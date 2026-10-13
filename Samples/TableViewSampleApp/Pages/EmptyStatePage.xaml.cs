// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates TableView.EmptyTemplate: the DataTemplate the control centres over the row area
/// whenever there are no rows to show. The page reaches that state three ways (a null
/// ItemsSource, a source emptied in place, and a filter that matches nothing), flat or grouped.
/// </summary>
public sealed partial class EmptyStatePage : Page
{
    private const int RowCount = 40;

    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private bool _sourceAttached = true;       // false while TableView.ItemsSource is null
    private bool _filteredToZero;
    private bool _isBulkUpdate;
    private bool _syncingDataSource;           // set while code moves DataSourceSelector
    private bool _personHandlersAttached;

    public EmptyStatePage()
    {
        _source = TableViewSource.From(People);
        InitializeComponent();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        RefreshReadouts();
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(RowCount);

    public TableViewSource? Source => _source;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        _personHandlersAttached = true;
        foreach (var person in People)
        {
            person.PropertyChanged += OnPersonChanged;
        }

        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        _personHandlersAttached = false;
    }

    // ---- Data source: the ways of being empty ---------------------------------------------

    private void OnDataSourceChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="0"), and when an action moves the
        // selector to match what it just did; neither is a user choice.
        if (!IsLoaded || _syncingDataSource)
        {
            return;
        }

        var tag = SampleShaping.SelectedTag(DataSourceSelector, "Populated");
        ApplyDataSource(tag);
        SetLastAction(tag switch
        {
            "Null" => "Data source -> Null ItemsSource (TableView.ItemsSource = null)",
            "ZeroRows" => "Data source -> Zero rows (source attached, collection cleared)",
            _ => string.Format(CultureInfo.CurrentCulture, "Data source -> Populated ({0:N0} rows)", People.Count),
        });
    }

    private void ApplyDataSource(string tag)
    {
        switch (tag)
        {
            case "Null":
                // A null ItemsSource resolves to zero rows, so the control shows EmptyTemplate.
                _sourceAttached = false;
                PeopleTable.ItemsSource = null;
                break;

            case "ZeroRows":
                // The other way of being empty: a live, shaped source that holds no items.
                AttachSource();
                ClearPeople();
                break;

            default:
                AttachSource();
                ClearFilter();
                if (People.Count == 0)
                {
                    RefillPeople();
                }

                break;
        }
    }

    private void AttachSource()
    {
        if (!_sourceAttached)
        {
            // Grouping and filtering live on the source, so they come back with it.
            PeopleTable.ItemsSource = _source;
            _sourceAttached = true;
        }
    }

    private void SelectDataSource(string tag)
    {
        _syncingDataSource = true;
        try
        {
            DataSourceSelector.SelectedIndex = tag switch { "Null" => 1, "ZeroRows" => 2, _ => 0 };
        }
        finally
        {
            _syncingDataSource = false;
        }
    }

    // ---- Actions ------------------------------------------------------------------------

    private void OnRemoveAllRowsClick(object sender, RoutedEventArgs e)
    {
        // Emptying a GROUPED source: every group disappears with its last member, and the
        // EmptyTemplate takes over the row area rather than an empty group band.
        var removed = People.Count;
        SelectDataSource("ZeroRows");
        ApplyDataSource("ZeroRows");
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed every row ({0:N0}); the source stays attached", removed));
    }

    private void OnRemoveLastRowClick(object sender, RoutedEventArgs e)
    {
        if (!_sourceAttached)
        {
            SetLastAction("ItemsSource is null; choose Populated first.");
            return;
        }

        if (People.Count == 0)
        {
            SetLastAction("No rows to remove.");
            return;
        }

        var person = People[^1];
        person.PropertyChanged -= OnPersonChanged;
        People.RemoveAt(People.Count - 1);
        if (People.Count == 0)
        {
            SelectDataSource("ZeroRows");
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0}; {1:N0} rows left", person.FullName, People.Count));
    }

    private void OnFilterToZeroClick(object sender, RoutedEventArgs e)
    {
        if (_source is null || !_sourceAttached)
        {
            SetLastAction("ItemsSource is null; choose Populated first.");
            return;
        }

        // Nobody joins in the future, so the filter matches nothing while the collection keeps
        // every row. Filter mutates the source in place, like GroupBy.
        var today = DateTimeOffset.Now;
        _source.Filter(item => item is Person p && p.JoinDate > today);
        _filteredToZero = true;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Filtered to people who joined after today: 0 of {0:N0} match", People.Count));
    }

    private void OnMoveSelectedRowClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var key = _appliedMode == "grouped" ? _appliedKey : nameof(Person.Department);
        var from = SampleShaping.KeyOf(person, key);
        _isBulkUpdate = true;
        try
        {
            if (key == nameof(Person.IsActive))
            {
                person.IsActive = !person.IsActive;
            }
            else
            {
                person.Department = SampleShaping.Next(PersonData.Departments, person.Department);
            }
        }
        finally
        {
            _isBulkUpdate = false;
        }

        // GroupBy takes a delegate, not a property path, so the control cannot re-bucket the row
        // on PropertyChanged; re-apply the grouping when the grouped-on value changed.
        ReapplyIfGroupedOn(key);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, SampleShaping.KeyOf(person, key)));
    }

    private void OnRestoreRowsClick(object sender, RoutedEventArgs e)
    {
        AttachSource();
        ClearFilter();
        ClearPeople();
        RefillPeople();
        SelectDataSource("Populated");
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored {0:N0} rows", People.Count));
    }

    private void ClearFilter()
    {
        if (_filteredToZero)
        {
            _source?.ClearFilter();
            _filteredToZero = false;
        }
    }

    private void ClearPeople()
    {
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        People.Clear();
    }

    private void RefillPeople()
    {
        foreach (var person in PersonData.Take(RowCount))
        {
            if (_personHandlersAttached)
            {
                person.PropertyChanged += OnPersonChanged;
            }

            People.Add(person);
        }
    }

    // In-cell edits: the Active checkbox writes IsActive, which is a group key.
    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || _isBulkUpdate || e.PropertyName != nameof(Person.IsActive))
        {
            return;
        }

        ReapplyIfGroupedOn(e.PropertyName);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Active -> {0} for {1}", person.IsActive ? "checked" : "unchecked", person.FullName));
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || ItemsSourceText is null || EmptyTemplateText is null)
        {
            return;
        }

        var shown = !_sourceAttached ? 0 : _filteredToZero ? 0 : People.Count;
        ItemsSourceText.Text = !_sourceAttached ? "null" : _filteredToZero ? "TableViewSource (filtered)" : "TableViewSource";
        EmptyTemplateText.Text = shown == 0 ? "Shown" : "Hidden";
        RowsText.Text = shown == People.Count
            ? SampleShaping.RowCountText(People.Count)
            : string.Format(CultureInfo.CurrentCulture, "{0:N0} shown of {1:N0}", shown, People.Count);
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex="0"), before the
        // later-declared elements exist. Guard every element this path touches.
        if (_source is null || PeopleTable is null || ShapingModeSelector is null || GroupKeySelector is null
            || ExpandAllButton is null || CollapseAllButton is null || ShapingModeText is null)
        {
            return;
        }

        var mode = SampleShaping.SelectedTag(ShapingModeSelector, "flat");
        var key = SampleShaping.SelectedTag(GroupKeySelector, "Department");
        var selected = PeopleTable.SelectedItem;

        switch (mode)
        {
            case "grouped":
                // The key selector receives the ROW; the identity selector receives the KEY.
                // Grouping an empty set is legal: the projection simply yields no groups.
                _source.GroupBy(item => SampleShaping.KeyOf(item as Person, key), SampleShaping.GroupIdentity);
                break;
            // case "hierarchy":
            // case "groupedHierarchy":
            //     Hierarchical (tree) rows are not available in this release, so the two matching
            //     ComboBoxItems ship disabled. TableViewSource and TableView have no hierarchy
            //     member today. When hierarchy ships, apply it to this same source here, composed
            //     with the GroupBy stage above rather than replacing it, and set _appliedMode only
            //     after the call returns.
            default:
                _source.ClearGroupBy();
                mode = "flat";
                break;
        }

        _appliedMode = mode;
        _appliedKey = key;

        // Re-applying GroupBy can drop the selection when the selected row changed group.
        SampleShaping.Reselect(PeopleTable, selected, People.Count + PersonData.Departments.Count + 2, RefreshReadouts);
        UpdateShapingGating();
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SampleShaping.Label(GroupKeySelector))
                : "Shaping -> Flat");
        }
    }

    // Call after ANY write to the grouped-on property: from an action or from an in-cell edit.
    private void ReapplyIfGroupedOn(string? propertyName)
    {
        if (_appliedMode == "grouped" && propertyName == _appliedKey)
        {
            ApplyShaping(announce: false);
        }
    }

    private void UpdateShapingGating()
    {
        var grouped = _appliedMode == "grouped";
        GroupKeySelector.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
        ShapingModeText.Text = SampleShaping.ShapingText(grouped, GroupKeySelector);
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        PeopleTable.ExpandAllGroups();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        PeopleTable.CollapseAllGroups();
        SetLastAction("Collapsed all groups");
    }

    // The only writer of LastActionText.
    private void SetLastAction(string message)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = message;
        }

        RefreshReadouts();
    }

    #endregion
}
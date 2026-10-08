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
/// Grid lines visibility: TableView.GridLinesVisibility (None / Horizontal / Vertical / All)
/// across text and template cells, against default or custom row banding, flat or grouped.
/// </summary>
public sealed partial class GridLinesVisibilityPage : Page
{
    private const int InitialRows = 40;

    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private int _nextPersonIndex = InitialRows;
    private bool _isBulkUpdate;

    public GridLinesVisibilityPage()
    {
        _source = TableViewSource.From(People);
        InitializeComponent();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        RefreshReadouts();
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(InitialRows);

    public TableViewSource? Source => _source;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
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
    }

    // ---- Grid lines and banding ---------------------------------------------------------

    private void OnGridLinesChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="3") before the table exists.
        if (PeopleTable is null || GridLinesText is null)
        {
            return;
        }

        PeopleTable.GridLinesVisibility = SampleShaping.SelectedTag(GridLinesSelector, "All") switch
        {
            "None" => TableViewGridLinesVisibility.None,
            "Horizontal" => TableViewGridLinesVisibility.Horizontal,
            "Vertical" => TableViewGridLinesVisibility.Vertical,
            _ => TableViewGridLinesVisibility.All,
        };
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "GridLinesVisibility -> {0}", PeopleTable.GridLinesVisibility));
    }

    private void OnRowBandingChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null || RowBandingText is null)
        {
            return;
        }

        var custom = SampleShaping.SelectedTag(RowBandingSelector, "default") == "custom";
        PeopleTable.Style = custom ? (Style)Resources["CustomBandingTableViewStyle"] : null;
        SetLastAction(custom
            ? "Row banding -> custom RowBackground / AlternatingRowBackground"
            : "Row banding -> default theme");
    }

    // ---- In-cell edits ------------------------------------------------------------------

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || _isBulkUpdate)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(Person.IsActive):
                ReapplyIfGroupedOn(e.PropertyName);
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Active -> {0} for {1}", person.IsActive ? "checked" : "unchecked", person.FullName));
                break;
            case nameof(Person.JoinDate):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Join date -> {0:d} for {1}", person.JoinDate, person.FullName));
                break;
        }
    }

    // ---- Actions ------------------------------------------------------------------------

    private void OnSelectThirdClick(object sender, RoutedEventArgs e)
    {
        if (People.Count < 3)
        {
            SetLastAction("There are fewer than three people; restore the rows first.");
            return;
        }

        var person = People[2];
        SetLastAction(SampleShaping.Reselect(PeopleTable, person, People.Count * 2)
            ? string.Format(CultureInfo.CurrentCulture, "Selected {0}", person.FullName)
            : string.Format(CultureInfo.CurrentCulture, "{0} is not displayed (collapsed group?); expand the groups first.", person.FullName));
    }

    private void OnMoveToNextGroupClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        // Moves along the key the Group key selector names, so the row always changes group
        // when grouped (and the same value changes when flat).
        var key = SampleShaping.SelectedTag(GroupKeySelector, "Department");
        var from = SampleShaping.KeyOf(person, key);
        _isBulkUpdate = true;
        try
        {
            switch (key)
            {
                case nameof(Person.Office):
                    person.Office = SampleShaping.Next(PersonData.Offices, person.Office);
                    break;
                case nameof(Person.IsActive):
                    person.IsActive = !person.IsActive;
                    break;
                default:
                    person.Department = SampleShaping.Next(PersonData.Departments, person.Department);
                    break;
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

    private void OnAddPersonClick(object sender, RoutedEventArgs e)
    {
        if (_nextPersonIndex >= PersonData.All.Count)
        {
            SetLastAction("No more people in the sample data.");
            return;
        }

        var person = PersonData.Take(_nextPersonIndex + 1)[_nextPersonIndex];
        _nextPersonIndex++;
        person.PropertyChanged += OnPersonChanged;
        People.Add(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} ({1})", person.FullName, person.Department));
    }

    private void OnRemovePersonClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        person.PropertyChanged -= OnPersonChanged;
        People.Remove(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0}", person.FullName));
    }

    private void OnRestoreRowsClick(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        // The same collection is refilled, so the TableViewSource and its grouping stay in place.
        People.Clear();
        foreach (var person in PersonData.Take(InitialRows))
        {
            person.PropertyChanged += OnPersonChanged;
            People.Add(person);
        }

        _nextPersonIndex = InitialRows;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored the original {0:N0} people", InitialRows));
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || GridLinesText is null || RowBandingText is null || PeopleTable is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(People.Count);
        GridLinesText.Text = PeopleTable.GridLinesVisibility.ToString();
        RowBandingText.Text = SampleShaping.Label(RowBandingSelector);
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
        SampleShaping.Reselect(PeopleTable, selected, People.Count * 2, RefreshReadouts);
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

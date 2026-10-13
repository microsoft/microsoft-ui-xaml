// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Selection: SelectionMode, the index-based selection API, and what selection does across a
/// reshape, an insert above the selected row, a group change and a remove.
/// </summary>
public sealed partial class SelectionPage : Page
{
    private const int InitialRows = 50;

    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private bool _isBulkUpdate;
    private int _changeCount;
    private int _insertedCount;
    private string _lastDelta = "(no SelectionChanged yet)";

    public SelectionPage()
    {
        _source = TableViewSource.From(People);
        InitializeComponent();
        PeopleTable.ItemsSource = _source;
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        RefreshReadouts();
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(InitialRows);

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        PeopleTable.SelectionChanged += OnTableSelectionChanged;
        People.CollectionChanged += OnPeopleCollectionChanged;
        foreach (var person in People)
        {
            person.PropertyChanged += OnPersonChanged;
        }

        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        PeopleTable.SelectionChanged -= OnTableSelectionChanged;
        People.CollectionChanged -= OnPeopleCollectionChanged;
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }
    }

    private void OnPeopleCollectionChanged(object? sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
    {
        foreach (Person person in e.OldItems ?? Array.Empty<Person>())
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        foreach (Person person in e.NewItems ?? Array.Empty<Person>())
        {
            person.PropertyChanged += OnPersonChanged;
        }
    }

    // ---- Selection mode -----------------------------------------------------------------

    private void OnSelectionModeChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="1"), before the table exists.
        if (PeopleTable is null || SelectFirstButton is null)
        {
            return;
        }

        var single = SampleShaping.SelectedTag(SelectionModeSelector, "Single") == "Single";
        PeopleTable.SelectionMode = single ? TableViewSelectionMode.Single : TableViewSelectionMode.None;
        SetLastAction(single ? "Selection mode -> Single" : "Selection mode -> None");
    }

    private bool IsSingleMode => PeopleTable.SelectionMode == TableViewSelectionMode.Single;

    // ---- Selection from code --------------------------------------------------------------

    private void OnSelectFirstClick(object sender, RoutedEventArgs e) => SelectDisplayedRow(fromEnd: false);

    private void OnSelectLastClick(object sender, RoutedEventArgs e) => SelectDisplayedRow(fromEnd: true);

    /// <summary>
    /// Selects the first or last row in DISPLAY order. Select(index) takes a display index, and
    /// when the table is grouped every group header takes one too, so the last row's index is
    /// rows + groups - 1, not People.Count - 1. Select ignores header and out-of-range indexes,
    /// so the sample walks in from the end until IsSelected confirms a row took the selection.
    /// </summary>
    private void SelectDisplayedRow(bool fromEnd)
    {
        var which = fromEnd ? "last" : "first";
        if (!IsSingleMode)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Ignored Select {0}: SelectionMode is None.", which));
            return;
        }

        var max = People.Count + (_appliedMode == "grouped" ? GroupCount() : 0) - 1;
        for (var step = 0; step <= max; step++)
        {
            var index = fromEnd ? max - step : step;
            PeopleTable.Select(index);
            if (PeopleTable.IsSelected(index))
            {
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Select({0}) selected the {1} row, {2}", index, which, Describe(PeopleTable.SelectedItem)));
                return;
            }
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "No {0} row to select: every row is hidden in a collapsed group.", which));
    }

    private void OnClearSelectionClick(object sender, RoutedEventArgs e)
    {
        var had = PeopleTable.SelectedItem;
        PeopleTable.DeselectAll();
        SetLastAction(had is null ? "DeselectAll(): nothing was selected." : "DeselectAll() cleared " + Describe(had));
    }

    // ---- Row changes around the selection -------------------------------------------------

    private void OnMoveGroupClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        // Flat mode has no grouped-on property; move the department, the default group key.
        var key = _appliedMode == "grouped" ? _appliedKey : nameof(Person.Department);
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
        SetLastAction(string.Format(
            CultureInfo.CurrentCulture,
            "Moved {0} from {1} to {2}; selection is {3}",
            person.FullName,
            from,
            SampleShaping.KeyOf(person, key),
            ReferenceEquals(PeopleTable.SelectedItem, person) ? "still on that person" : "no longer on that person"));
    }

    private void OnInsertAboveClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        // The next person from the shared dataset, placed in the selected person's group.
        var next = (InitialRows + _insertedCount) % PersonData.All.Count;
        var person = PersonData.Take(next + 1)[next];
        _insertedCount++;
        person.Department = selected.Department;
        person.Office = selected.Office;
        person.IsActive = selected.IsActive;

        var before = _changeCount;
        People.Insert(People.IndexOf(selected), person);

        // A SelectionChanged raised by the insert lands before this continuation runs.
        DispatcherQueue.TryEnqueue(() =>
        {
            if (!IsLoaded)
            {
                return;
            }

            SetLastAction(string.Format(
                CultureInfo.CurrentCulture,
                "Inserted {0} above {1}; {2}",
                person.FullName,
                selected.FullName,
                _changeCount == before ? "no SelectionChanged was raised" : "SelectionChanged was raised"));
        });
    }

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        People.Remove(selected);

        DispatcherQueue.TryEnqueue(() =>
        {
            if (!IsLoaded)
            {
                return;
            }

            SetLastAction(PeopleTable.SelectedItem is null
                ? string.Format(CultureInfo.CurrentCulture, "Removed {0}; the selection cleared", selected.FullName)
                : string.Format(CultureInfo.CurrentCulture, "Removed {0}; the selection moved to {1}", selected.FullName, Describe(PeopleTable.SelectedItem)));
        });
    }

    // ---- Events ---------------------------------------------------------------------------

    private void OnTableSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        // Reselect probes indexes after a reshape; only the final state is worth reporting.
        if (SampleShaping.IsReselecting)
        {
            return;
        }

        _changeCount++;
        _lastDelta = string.Format(CultureInfo.CurrentCulture, "+[{0}] -[{1}]", DescribeAll(args.AddedItems), DescribeAll(args.RemovedItems));
        RefreshReadouts();
    }

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || _isBulkUpdate)
        {
            return;
        }

        // In-cell editors (Active checkbox, Office list) write the model directly.
        string? change = e.PropertyName switch
        {
            nameof(Person.IsActive) => person.IsActive ? "Active -> checked" : "Active -> unchecked",
            nameof(Person.Office) => "Office -> " + person.Office,
            _ => null,
        };

        if (change is null)
        {
            return;
        }

        ReapplyIfGroupedOn(e.PropertyName);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} for {1}; selection is on {2}", change, person.FullName, Describe(PeopleTable.SelectedItem)));
    }

    private static string DescribeAll(IList<object>? items) =>
        items is null || items.Count == 0 ? "none" : string.Join(", ", items.Select(Describe));

    private static string Describe(object? item) => item is Person person ? person.FullName : "(none)";

    private int GroupCount() =>
        People.Select(p => SampleShaping.GroupIdentity(SampleShaping.KeyOf(p, _appliedKey))).Distinct(StringComparer.Ordinal).Count();

    private void RefreshReadouts()
    {
        if (PeopleTable is null || RowsText is null || SelectedItemText is null)
        {
            return;
        }

        var item = PeopleTable.SelectedItem;
        var index = PeopleTable.SelectedIndex;
        SelectedItemText.Text = Describe(item);
        SelectedIndexText.Text = index >= 0
            ? index.ToString(CultureInfo.CurrentCulture)
            : item is null
                ? string.Format(CultureInfo.CurrentCulture, "{0} (none)", -1)
                : string.Format(CultureInfo.CurrentCulture, "{0} (the row is in a collapsed group)", -1);
        ChangeCountText.Text = _changeCount.ToString("N0", CultureInfo.CurrentCulture);
        SelectionDeltaText.Text = _lastDelta;
        RowsText.Text = SampleShaping.RowCountText(People.Count);

        var single = IsSingleMode;
        SelectFirstButton.IsEnabled = single;
        SelectLastButton.IsEnabled = single;
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent, before the later-declared elements exist.
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
        SampleShaping.Reselect(PeopleTable, selected, People.Count + PersonData.Departments.Count + PersonData.Offices.Count, RefreshReadouts);
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

        // Collapse and expand settle SelectedIndex after layout; read it again once that has run.
        DispatcherQueue.TryEnqueue(() =>
        {
            if (IsLoaded)
            {
                RefreshReadouts();
            }
        });
    }

    #endregion
}

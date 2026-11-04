// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
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
public sealed partial class SelectionPage : SamplePageBase
{
    private const int InitialRows = 50;

    private int _insertedCount;

    public SelectionPage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        PeopleTable.ItemsSource = Source;
        // </snippet>
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
        TrackItems(People, OnPersonChanged);
    }

    // <snippet>
    public ObservableCollection<Person> People { get; } = PersonData.Take(InitialRows);
    // </snippet>

    // <snippet>
    public TableViewSource Source { get; }
    // </snippet>

    // ---- Selection mode -----------------------------------------------------------------

    private void OnSelectionModeChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="1"), before the table exists. // snippet:skip
        if (PeopleTable is null || SelectFirstButton is null) // snippet:skip
        { // snippet:skip
            return; // snippet:skip
        } // snippet:skip

        var single = (string)((ComboBoxItem)((ComboBox)sender).SelectedItem).Tag == "Single";
        PeopleTable.SelectionMode = single ? TableViewSelectionMode.Single : TableViewSelectionMode.None;
        SetLastAction(single ? "Selection mode -> Single" : "Selection mode -> None");
    }

    private bool IsSingleMode => PeopleTable.SelectionMode == TableViewSelectionMode.Single;

    // ---- Selection from code --------------------------------------------------------------

    private void OnSelectFirstClick(object sender, RoutedEventArgs e) => SelectDisplayedRow(fromEnd: false);

    private void OnSelectLastClick(object sender, RoutedEventArgs e) => SelectDisplayedRow(fromEnd: true);

    private void SelectDisplayedRow(bool fromEnd)
    {
        var which = fromEnd ? "last" : "first";
        if (!IsSingleMode)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Ignored Select {0}: SelectionMode is None.", which));
            return;
        }

        if (People.Count == 0)
        {
            SetLastAction("There is no row to select.");
            return;
        }

        var index = SelectFirstSelectable(People.Count + (IsGrouped ? GroupCount() : 0) - 1, fromEnd);
        if (index >= 0)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Select({0}) selected the {1} row, {2}", index, which, Describe(PeopleTable.SelectedItem)));
            return;
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "No {0} row to select: every row is hidden in a collapsed group.", which));
    }

    // <snippet>
    // Select takes a display index; group headers take indexes too but cannot be selected, so walk
    // in from one end until IsSelected confirms a row took the selection.
    private int SelectFirstSelectable(int lastIndex, bool fromEnd)
    {
        for (var step = 0; step <= lastIndex; step++)
        {
            var index = fromEnd ? lastIndex - step : step;
            PeopleTable.Select(index);
            if (PeopleTable.IsSelected(index))
            {
                return index;
            }
        }

        return -1;
    }
    // </snippet>

    // Group headers take display indexes too.
    private int GroupCount() =>
        People.Select(p => SampleShaping.GroupIdentity(SampleShaping.KeyOf(p, AppliedGroupKey))).Distinct(StringComparer.Ordinal).Count();

    // <snippet>
    private void OnClearSelectionClick(object sender, RoutedEventArgs e)
    {
        var had = PeopleTable.SelectedItem; // snippet:skip
        PeopleTable.DeselectAll();
        SetLastAction(had is null ? "DeselectAll(): nothing was selected." : "DeselectAll() cleared " + Describe(had));
    }
    // </snippet>

    // ---- Row changes around the selection -------------------------------------------------

    private void OnMoveGroupClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        // Flat mode has no grouped-on property; move the department, the default group key.
        var key = IsGrouped ? AppliedGroupKey : nameof(Person.Department);
        var from = SampleShaping.KeyOf(person, key);
        using (BeginBulkUpdate())
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
        PersonData.ReportToDepartmentHead(person, People);

        var before = _changeCount;
        People.Insert(People.IndexOf(selected), person);

        // A SelectionChanged raised by the insert lands before this continuation runs.
        EnqueueIfLoaded(() =>
        {
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

        EnqueueIfLoaded(() =>
        {
            SetLastAction(PeopleTable.SelectedItem is null
                ? string.Format(CultureInfo.CurrentCulture, "Removed {0}; the selection cleared", selected.FullName)
                : string.Format(CultureInfo.CurrentCulture, "Removed {0}; the selection moved to {1}", selected.FullName, Describe(PeopleTable.SelectedItem)));
        });
    }

    // ---- Events ---------------------------------------------------------------------------

    // <snippet>
    // Raised after the selection has settled, so sender.SelectedItem and SelectedIndex hold the new
    // state. Replacing a selection raises one event with both the removed and the added row.
    private void OnTableSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        _changeCount++; // snippet:skip
        _lastDelta = string.Format(CultureInfo.CurrentCulture, "+[{0}] -[{1}]", DescribeAll(args.AddedItems), DescribeAll(args.RemovedItems));
        RefreshReadouts();
    }
    // </snippet>

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || IsBulkUpdating)
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

    // Every Last action on this page also re-reads the readouts once layout has run: collapse and
    // expand settle SelectedIndex after layout.
    protected override void OnLastActionSet(string message) => EnqueueIfLoaded(RefreshReadouts);
}

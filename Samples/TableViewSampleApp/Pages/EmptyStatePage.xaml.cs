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
public sealed partial class EmptyStatePage : SamplePageBase
{
    private const int RowCount = 40;

    private bool _sourceAttached = true;       // false while TableView.ItemsSource is null
    private bool _filteredToZero;
    private bool _syncingDataSource;           // set while code moves DataSourceSelector

    public EmptyStatePage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        // </snippet>
        Shaping.ProbeLimit = () => People.Count + PersonData.Departments.Count + 2;
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
        TrackItems(People, OnPersonChanged);
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(RowCount);

    public TableViewSource Source { get; }

    // <snippet>
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
            PeopleTable.ItemsSource = Source;
            _sourceAttached = true;
        }
    }
    // </snippet>

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
        People.RemoveAt(People.Count - 1);
        if (People.Count == 0)
        {
            SelectDataSource("ZeroRows");
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0}; {1:N0} rows left", person.FullName, People.Count));
    }

    // <snippet>
    private void OnFilterToZeroClick(object sender, RoutedEventArgs e)
    {
        if (!_sourceAttached)
        {
            SetLastAction("ItemsSource is null; choose Populated first.");
            return;
        }

        // Nobody joins in the future, so the filter matches nothing while the collection keeps
        // every row. Filter mutates the source in place, like GroupBy.
        var today = DateTimeOffset.Now;
        Source.Filter(item => item is Person p && p.JoinDate > today);
        _filteredToZero = true;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Filtered to people who joined after today: 0 of {0:N0} match", People.Count));
    }
    // </snippet>

    private void OnMoveSelectedRowClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var key = IsGrouped ? AppliedGroupKey : nameof(Person.Department);
        var from = SampleShaping.KeyOf(person, key);
        using (BeginBulkUpdate())
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
            Source.ClearFilter();
            _filteredToZero = false;
        }
    }

    private void ClearPeople() => People.Clear();

    private void RefillPeople()
    {
        foreach (var person in PersonData.Take(RowCount))
        {
            People.Add(person);
        }
    }

    // In-cell edits: the Active checkbox writes IsActive, which is a group key.
    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || IsBulkUpdating || e.PropertyName != nameof(Person.IsActive))
        {
            return;
        }

        ReapplyIfGroupedOn(e.PropertyName);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Active -> {0} for {1}", person.IsActive ? "checked" : "unchecked", person.FullName));
    }
}

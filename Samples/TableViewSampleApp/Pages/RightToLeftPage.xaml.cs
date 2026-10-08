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
using TableViewSampleApp.Models;
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Right-to-left layout: TableView.FlowDirection on a people table that mixes text and template
/// cells, with Arabic and Hebrew rows, editing, sorting, filtering, grouping and column moves, so
/// every right-to-left-sensitive adornment can be checked.
/// </summary>
public sealed partial class RightToLeftPage : SamplePageBase
{
    private readonly TableViewColumn[] _originalColumnOrder;
    private int _nextRtlPerson = RtlPersonData.MixedInCount;
    private int _nextRename;

    public RightToLeftPage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        // </snippet>
        _originalColumnOrder = PeopleTable.Columns.ToArray();
        Shaping.ProbeLimit = () => People.Count * 2;
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
        TrackItems(People, OnPersonChanged);
        TrackLifetime(() => PeopleTable.Sorted += OnTableSorted, () => PeopleTable.Sorted -= OnTableSorted);
    }

    // PersonData.Take(40) with the first five Arabic / Hebrew people mixed into the top rows.
    public ObservableCollection<Person> People { get; } = RtlPersonData.Mixed();

    public TableViewSource Source { get; }

    // <snippet>
    // ---- Flow direction and filter ------------------------------------------------------

    private void OnRtlToggled(object sender, RoutedEventArgs e)
    {
        // Toggled fires during InitializeComponent (IsOn="True"); the XAML already sets RightToLeft.
        if (!IsLoaded)
        {
            return;
        }

        PeopleTable.FlowDirection = RtlToggle.IsOn ? FlowDirection.RightToLeft : FlowDirection.LeftToRight;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "FlowDirection -> {0}", PeopleTable.FlowDirection));
    }

    private void OnFilterChanged(object sender, TextChangedEventArgs e)
    {
        if (FilterBox is null)
        {
            return;
        }

        var term = FilterBox.Text.Trim();
        if (term.Length == 0)
        {
            Source.ClearFilter();
            SetLastAction("ClearFilter()");
        }
        else
        {
            // Filter reshapes the same source in place and composes with GroupBy and the sort.
            Source.Filter(item => item is Person person && Matches(person, term));
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Filter -> \"{0}\"", term));
        }
    }

    private static bool Matches(Person person, string term) =>
        person.FullName.Contains(term, StringComparison.CurrentCultureIgnoreCase)
        || person.Department.Contains(term, StringComparison.CurrentCultureIgnoreCase)
        || person.Office.Contains(term, StringComparison.CurrentCultureIgnoreCase)
        || person.Bio.Contains(term, StringComparison.CurrentCultureIgnoreCase);
    // </snippet>

    // ---- In-cell edits ------------------------------------------------------------------

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(Person.FirstName):
            case nameof(Person.LastName):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Name -> {0}", person.FullName));
                break;
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

    // <snippet>
    private void OnSortFirstNameClick(object sender, RoutedEventArgs e)
    {
        var direction = FirstNameColumn.SortDirection == TableViewSortDirection.Ascending
            ? TableViewSortDirection.Descending
            : TableViewSortDirection.Ascending;
        PeopleTable.SortByColumn(FirstNameColumn, direction);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "SortByColumn(First name, {0})", direction));
    }
    // </snippet>

    private void OnRenameClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        // OnPersonChanged reports the new name.
        person.FirstName = RtlPersonData.Renames[_nextRename++ % RtlPersonData.Renames.Count];
    }

    private void OnAddPersonClick(object sender, RoutedEventArgs e)
    {
        var person = RtlPersonData.Create(_nextRtlPerson++, RtlPersonData.FirstEmployeeId + People.Count);
        People.Add(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} ({1})", person.FullName, person.Department));
    }

    private void OnMoveColumnEarlierClick(object sender, RoutedEventArgs e) => MoveColumn(DepartmentColumn, -1);

    private void OnMoveColumnLaterClick(object sender, RoutedEventArgs e) => MoveColumn(DepartmentColumn, +1);

    // <snippet>
    // TableView.Columns is the live column collection: moving an entry moves the column.
    private void MoveColumn(TableViewColumn column, int offset)
    {
        var columns = PeopleTable.Columns;
        var from = columns.IndexOf(column);
        var to = from + offset;
        if (from < 0 || to < 0 || to >= columns.Count)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} is already the {1} column", column.Header, offset < 0 ? "first" : "last"));
            return;
        }

        columns.RemoveAt(from);
        columns.Insert(to, column);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} to column {1} of {2}", column.Header, to + 1, columns.Count));
    }
    // </snippet>

    private void OnResetColumnOrderClick(object sender, RoutedEventArgs e)
    {
        var columns = PeopleTable.Columns;
        for (var target = 0; target < _originalColumnOrder.Length; target++)
        {
            var current = columns.IndexOf(_originalColumnOrder[target]);
            if (current >= 0 && current != target)
            {
                var column = columns[current];
                columns.RemoveAt(current);
                columns.Insert(target, column);
            }
        }

        SetLastAction("Reset the column order");
    }
}

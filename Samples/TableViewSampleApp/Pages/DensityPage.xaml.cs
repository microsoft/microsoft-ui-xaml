// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewDensity = Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Density: TableView.Density (Compact / Standard / Comfortable) switched live over text and
/// read-only template cells, alongside grouping, whose headers keep a fixed height.
/// </summary>
public sealed partial class DensityPage : SamplePageBase
{
    private readonly Queue<Person> _spares = new(PersonData.Take(60).Skip(40));
    private readonly List<Person> _stash = new();

    public DensityPage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        // </snippet>
        Shaping.ProbeLimit = () => MaxProbeIndex;
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource Source { get; }

    private int MaxProbeIndex => People.Count + PersonData.Roles.Count;

    // <snippet>
    // ---- Density ------------------------------------------------------------------------

    private void OnDensityChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="1"), when the table already uses the
        // Standard default.
        if (PeopleTable is null || DensitySelector is null || !IsLoaded)
        {
            return;
        }

        var density = Enum.Parse<TableViewDensity>(SampleShaping.SelectedTag(DensitySelector, "Standard"));
        PeopleTable.Density = density;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Density -> {0}", density));
    }

    // ---- Actions ------------------------------------------------------------------------

    private void OnMoveGroupClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var key = IsGrouped ? AppliedGroupKey : nameof(Person.Department);
        var from = SampleShaping.KeyOf(person, key); // snippet:skip
        switch (key)
        {
            case nameof(Person.Role):
                person.Role = SampleShaping.Next(PersonData.Roles, person.Role);
                break;
            case nameof(Person.IsActive):
                person.IsActive = !person.IsActive;
                break;
            default:
                person.Department = SampleShaping.Next(PersonData.Departments, person.Department);
                break;
        }

        // GroupBy takes a delegate, not a property path, so the control cannot re-bucket the row
        // on PropertyChanged; re-apply the grouping when the grouped-on value changed.
        ReapplyIfGroupedOn(key);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, SampleShaping.KeyOf(person, key)));
    }
    // </snippet>

    private void OnAddPersonClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person selected)
        {
            SetLastAction("No row selected.");
            return;
        }

        if (!_spares.TryDequeue(out var person))
        {
            SetLastAction("Every spare person has been added already.");
            return;
        }

        // Give the newcomer the selected row's grouped-on value so they land in its group.
        person.Department = selected.Department;
        person.Role = selected.Role;
        person.IsActive = selected.IsActive;
        People.Insert(People.IndexOf(selected) + 1, person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} to {1}", person.FullName, SampleShaping.KeyOf(person, AppliedGroupKey)));
    }

    private void OnRemoveRowClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        People.Remove(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0} from {1}", person.FullName, SampleShaping.KeyOf(person, AppliedGroupKey)));
    }

    private void OnToggleEmptyClick(object sender, RoutedEventArgs e)
    {
        if (People.Count > 0)
        {
            _stash.Clear();
            _stash.AddRange(People);
            People.Clear();
            EmptyToggleButton.Content = "Restore rows";
            SetLastAction("Cleared every row");
        }
        else
        {
            foreach (var person in _stash)
            {
                People.Add(person);
            }

            _stash.Clear();
            EmptyToggleButton.Content = "Clear all rows";
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored {0:N0} rows", People.Count));
        }
    }

    private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (!SampleShaping.IsReselecting)
        {
            RefreshReadouts();
        }
    }
}

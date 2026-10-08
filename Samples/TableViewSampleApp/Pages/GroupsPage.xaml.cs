// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Input;
using TableViewSampleApp.Controls;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Grouped rows: TableViewSource.GroupBy reshapes one source in place, TableView.GroupHeaderTemplate
/// customises the header, and ExpandAllGroups / CollapseAllGroups drive every group. The page
/// starts Grouped by Department. Edits and actions that change a group key re-apply GroupBy,
/// because the key selector is a delegate the control cannot observe.
/// </summary>
public sealed partial class GroupsPage : SamplePageBase
{
    // New hires for "Add a person": realistic PersonData rows that are not in the table yet.
    private readonly Queue<Person> _newHires;

    // Every removed row, so "Restore all rows" brings back exactly what the actions removed.
    private readonly List<Person> _removed = new();

    private readonly TappedEventHandler _tappedHandler;
    private readonly DoubleTappedEventHandler _doubleTappedHandler;
    private readonly KeyEventHandler _keyDownHandler;

    // Measured in OnShapingApplying, against the key the groups were built on.
    private bool _wasAllCollapsed;

    public GroupsPage()
    {
        var pool = PersonData.Take(80);
        People = new ObservableCollection<Person>(pool.Take(60));
        _newHires = new Queue<Person>(pool.Skip(60));
        _tappedHandler = OnTableTapped;
        _doubleTappedHandler = OnTableDoubleTapped;
        _keyDownHandler = OnTableKeyDown;

        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        ApplyGroupHeaderTemplate();
        // </snippet>

        // Grouping is this page's subject, so it starts Grouped (ShapingOptions InitialMode="grouped");
        // InitializeSample applies the grouping now that every element exists.
        Shaping.ProbeLimit = () => People.Count + 64;
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
        TrackItems(People, OnPersonChanged);

        // There is no table-level event for one group being toggled from its header, so listen
        // for the input that toggles it (handledEventsToo: the header marks the input handled).
        TrackLifetime(
            () =>
            {
                PeopleTable.AddHandler(UIElement.TappedEvent, _tappedHandler, true);
                PeopleTable.AddHandler(UIElement.DoubleTappedEvent, _doubleTappedHandler, true);
                PeopleTable.AddHandler(UIElement.KeyDownEvent, _keyDownHandler, true);
            },
            () =>
            {
                PeopleTable.RemoveHandler(UIElement.TappedEvent, _tappedHandler);
                PeopleTable.RemoveHandler(UIElement.DoubleTappedEvent, _doubleTappedHandler);
                PeopleTable.RemoveHandler(UIElement.KeyDownEvent, _keyDownHandler);
            });
    }

    public ObservableCollection<Person> People { get; }

    public TableViewSource Source { get; }

    // <snippet>
    // ---- Group header template ----------------------------------------------------------

    private void OnHeaderTemplateChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null || HeaderTemplateSelector is null)
        {
            return;
        }

        ApplyGroupHeaderTemplate();
        if (IsLoaded) // snippet:skip
        { // snippet:skip
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Group header template -> {0}", SampleShaping.Label(HeaderTemplateSelector)));
        } // snippet:skip
    }

    private void ApplyGroupHeaderTemplate()
    {
        PeopleTable.GroupHeaderTemplate = SampleShaping.SelectedTag(HeaderTemplateSelector, "custom") == "custom"
            ? (DataTemplate)Resources["CustomGroupHeaderTemplate"]
            : null;
    }

    // ---- In-cell edits --------------------------------------------------------------------

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || IsBulkUpdating)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(Person.Department):
            case nameof(Person.IsActive):
                // GroupBy takes a delegate, not a property path, so the control cannot re-bucket
                // the row on PropertyChanged; re-apply the grouping when the grouped-on value changed.
                ReapplyIfGroupedOn(e.PropertyName);
                SetLastAction(string.Format(
                    CultureInfo.CurrentCulture,
                    "Edited in the cell: {0} is now in {1}",
                    person.FullName,
                    SampleShaping.KeyOf(person, e.PropertyName)));
                break;
            case nameof(Person.JoinDate): // snippet:skip
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Join date -> {0:d} for {1}", person.JoinDate, person.FullName));
                break; // snippet:skip
        }
    }

    // ---- Expansion across a reshape -------------------------------------------------------

    protected override void OnShapingApplying(ShapingApplyingEventArgs e)
    {
        _wasAllCollapsed = ExpansionSummary() == AllCollapsed;
    }

    // Re-applying GroupBy rebuilds the groups, so restore the bulk expansion state the readout
    // reports; a mixed state resets to expanded.
    protected override void OnShapingApplied(ShapingAppliedEventArgs e)
    {
        if (!e.IsGrouped)
        {
            return;
        }

        var collapse = _wasAllCollapsed;
        _collapsedGroups.Clear();
        if (collapse)
        {
            _collapsedGroups.UnionWith(CurrentGroupIdentities());
        }

        EnqueueIfLoaded(() =>
        {
            if (!IsGrouped)
            {
                return;
            }

            if (collapse)
            {
                PeopleTable.CollapseAllGroups();
            }
            else
            {
                PeopleTable.ExpandAllGroups();
            }
        });
    }
    // </snippet>

    // ---- Actions ----------------------------------------------------------------------------

    // The key the actions act on: the applied one when grouped, the selected one when flat.
    private string ActionKey => IsGrouped ? AppliedGroupKey : Shaping.SelectedKey;

    private void OnMoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var key = ActionKey;
        var from = SampleShaping.KeyOf(person, key);
        using (BeginBulkUpdate())
        {
            switch (key)
            {
                case nameof(Person.Office):
                    person.Office = SampleShaping.Next(PersonData.Offices, person.Office);
                    break;
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
        }

        ReapplyIfGroupedOn(key);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, SampleShaping.KeyOf(person, key)));
    }

    private void OnMoveEveryFourthClick(object sender, RoutedEventArgs e)
    {
        var moved = 0;
        using (BeginBulkUpdate())
        {
            for (var i = 0; i < People.Count; i += 4)
            {
                People[i].Department = SampleShaping.Next(PersonData.Departments, People[i].Department);
                moved++;
            }
        }

        ReapplyIfGroupedOn(nameof(Person.Department));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0:N0} rows (every 4th) to the next department", moved));
    }

    // <snippet>
    // Adding and removing rows needs no GroupBy call: the projection follows collection changes.
    private void OnAddPersonClick(object sender, RoutedEventArgs e)
    {
        if (!_newHires.TryDequeue(out var person))
        {
            SetLastAction("No more new hires to add.");
            return;
        }

        // Give the new hire the first group's key, so the row joins a live group.
        var key = ActionKey;
        var firstKey = InViewOrder().Select(p => SampleShaping.KeyOf(p, key) as string).FirstOrDefault();
        if (firstKey is not null)
        {
            switch (key)
            {
                case nameof(Person.Office):
                    person.Office = firstKey;
                    break;
                case nameof(Person.Role):
                    person.Role = firstKey;
                    break;
                case nameof(Person.IsActive):
                    person.IsActive = firstKey == "Active";
                    break;
                default:
                    person.Department = firstKey;
                    break;
            }
        }

        People.Add(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} to {1}", person.FullName, SampleShaping.KeyOf(person, key)));
    }
    // </snippet>

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var key = ActionKey;
        var group = SampleShaping.KeyOf(person, key);
        var wasLast = People.Count(p => Equals(SampleShaping.KeyOf(p, key), group)) == 1;
        People.Remove(person);
        _removed.Add(person);
        SetLastAction(string.Format(
            CultureInfo.CurrentCulture,
            wasLast ? "Removed {0}, the last person in {1}: that group is gone" : "Removed {0} from {1}",
            person.FullName,
            group));
    }

    private void OnRemoveSmallestGroupClick(object sender, RoutedEventArgs e)
    {
        var key = ActionKey;
        var smallest = InViewOrder()
            .GroupBy(p => SampleShaping.KeyOf(p, key))
            .OrderBy(g => g.Count())
            .FirstOrDefault();
        if (smallest is null)
        {
            SetLastAction("There are no groups to remove.");
            return;
        }

        var members = smallest.ToList();
        foreach (var person in members)
        {
            People.Remove(person);
        }

        _removed.AddRange(members);
        SetLastAction(string.Format(
            CultureInfo.CurrentCulture,
            members.Count == 1 ? "Removed the {0} group ({1:N0} person): its header is gone" : "Removed the {0} group ({1:N0} people): its header is gone",
            smallest.Key,
            members.Count));
    }

    private void OnEmptyToggleClick(object sender, RoutedEventArgs e)
    {
        if (People.Count > 0)
        {
            _removed.AddRange(People);
            People.Clear();
            SetLastAction("Removed all rows: the grouped projection has no groups and the EmptyTemplate shows");
            return;
        }

        foreach (var person in _removed)
        {
            People.Add(person);
        }

        var restored = _removed.Count;
        _removed.Clear();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored {0:N0} rows; the grouping applies to them again", restored));
    }
}

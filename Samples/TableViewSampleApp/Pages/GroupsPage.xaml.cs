// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using Windows.System;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Grouped rows: TableViewSource.GroupBy reshapes one source in place, TableView.GroupHeaderTemplate
/// customises the header, and ExpandAllGroups / CollapseAllGroups drive every group. The page
/// starts Grouped by Department. Edits and actions that change a group key re-apply GroupBy,
/// because the key selector is a delegate the control cannot observe.
/// </summary>
public sealed partial class GroupsPage : Page
{
    private const string AllExpanded = "All expanded";
    private const string AllCollapsed = "All collapsed";

    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private bool _isBulkUpdate;

    // Group identities (SampleShaping.GroupIdentity) the page has collapsed. The control has no
    // public per-group state query, so the page records every expansion change it makes or sees.
    private readonly HashSet<string> _collapsedGroups = new();

    // New hires for "Add a person": realistic PersonData rows that are not in the table yet.
    private readonly Queue<Person> _newHires;

    // Every removed row, so "Restore all rows" brings back exactly what the actions removed.
    private readonly List<Person> _removed = new();

    private readonly TappedEventHandler _tappedHandler;
    private readonly DoubleTappedEventHandler _doubleTappedHandler;
    private readonly KeyEventHandler _keyDownHandler;

    public GroupsPage()
    {
        var pool = PersonData.Take(80);
        People = new ObservableCollection<Person>(pool.Take(60));
        _newHires = new Queue<Person>(pool.Skip(60));
        _source = TableViewSource.From(People);
        _tappedHandler = OnTableTapped;
        _doubleTappedHandler = OnTableDoubleTapped;
        _keyDownHandler = OnTableKeyDown;

        InitializeComponent();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;

        // Grouping is this page's subject, so it starts Grouped (ShapingModeSelector
        // SelectedIndex="1"). The SelectionChanged that fired during InitializeComponent was
        // ignored by the init guard; apply the grouping now that every element exists.
        ApplyGroupHeaderTemplate();
        ApplyShaping(announce: false);
    }

    public ObservableCollection<Person> People { get; }

    public TableViewSource? Source => _source;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        People.CollectionChanged += OnPeopleCollectionChanged;
        foreach (var person in People)
        {
            person.PropertyChanged += OnPersonChanged;
        }

        // There is no table-level event for one group being toggled from its header, so listen
        // for the input that toggles it (handledEventsToo: the header marks the input handled).
        PeopleTable.AddHandler(UIElement.TappedEvent, _tappedHandler, true);
        PeopleTable.AddHandler(UIElement.DoubleTappedEvent, _doubleTappedHandler, true);
        PeopleTable.AddHandler(UIElement.KeyDownEvent, _keyDownHandler, true);
        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        People.CollectionChanged -= OnPeopleCollectionChanged;
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        PeopleTable.RemoveHandler(UIElement.TappedEvent, _tappedHandler);
        PeopleTable.RemoveHandler(UIElement.DoubleTappedEvent, _doubleTappedHandler);
        PeopleTable.RemoveHandler(UIElement.KeyDownEvent, _keyDownHandler);
    }

    private void OnPeopleCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
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

    // ---- Group header template ----------------------------------------------------------

    private void OnHeaderTemplateChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null || HeaderTemplateSelector is null)
        {
            return;
        }

        ApplyGroupHeaderTemplate();
        if (IsLoaded)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Group header template -> {0}", SampleShaping.Label(HeaderTemplateSelector)));
        }
    }

    private void ApplyGroupHeaderTemplate()
    {
        PeopleTable.GroupHeaderTemplate = SampleShaping.SelectedTag(HeaderTemplateSelector, "custom") == "custom"
            ? (DataTemplate)Resources["CustomGroupHeaderTemplate"]
            : null;
    }

    // ---- One group toggled from its header ----------------------------------------------

    // A header click or Enter/Space toggles its group; Right and Left expand and collapse it (the
    // reverse in RTL). The control resolves the group at once but applies the change on a later
    // dispatcher turn, so header.IsExpanded still holds the OLD state here and for a while after.
    // The page therefore computes the new state itself instead of reading the header back.
    private void OnTableTapped(object sender, TappedRoutedEventArgs e) => NoteHeaderExpansion(e.OriginalSource as DependencyObject, desired: null);

    // The second click of a double-click toggles the group again but raises DoubleTapped, not Tapped.
    private void OnTableDoubleTapped(object sender, DoubleTappedRoutedEventArgs e) => NoteHeaderExpansion(e.OriginalSource as DependencyObject, desired: null);

    private void OnTableKeyDown(object sender, KeyRoutedEventArgs e)
    {
        var rtl = PeopleTable.FlowDirection == FlowDirection.RightToLeft;
        bool? desired = e.Key switch
        {
            VirtualKey.Right => !rtl,
            VirtualKey.Left => rtl,
            _ => null,
        };

        if (desired is not null || e.Key is VirtualKey.Enter or VirtualKey.Space)
        {
            NoteHeaderExpansion(e.OriginalSource as DependencyObject, desired);
        }
    }

    private void NoteHeaderExpansion(DependencyObject? source, bool? desired)
    {
        while (source is not null && source is not TableViewGroupHeader)
        {
            source = VisualTreeHelper.GetParent(source);
        }

        if (_appliedMode != "grouped" || source is not TableViewGroupHeader { IsExpandable: true } header
            || header.Content is not TableViewGroupInfo group)
        {
            return;
        }

        var identity = SampleShaping.GroupIdentity(group.Key);
        var expanded = desired ?? _collapsedGroups.Contains(identity);
        if (expanded)
        {
            _collapsedGroups.Remove(identity);
        }
        else
        {
            _collapsedGroups.Add(identity);
        }

        SetLastAction(string.Format(
            CultureInfo.CurrentCulture,
            "{0} the {1} group from its header",
            expanded ? "Expanded" : "Collapsed",
            group.KeyText));
    }

    // Identities of the groups the current rows produce.
    private HashSet<string> CurrentGroupIdentities()
    {
        var key = _appliedKey;
        return People.Select(p => SampleShaping.GroupIdentity(SampleShaping.KeyOf(p, key))).ToHashSet();
    }

    private string ExpansionSummary()
    {
        var groups = CurrentGroupIdentities();
        if (groups.Count == 0)
        {
            return "(no groups)";
        }

        var collapsed = groups.Count(_collapsedGroups.Contains);
        return collapsed == 0 ? AllExpanded
            : collapsed == groups.Count ? AllCollapsed
            : string.Format(CultureInfo.CurrentCulture, "Mixed: {0:N0} of {1:N0} collapsed", collapsed, groups.Count);
    }

    // ---- In-cell edits --------------------------------------------------------------------

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || _isBulkUpdate)
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
            case nameof(Person.JoinDate):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Join date -> {0:d} for {1}", person.JoinDate, person.FullName));
                break;
        }
    }

    // ---- Actions ----------------------------------------------------------------------------

    // The key the actions act on: the applied one when grouped, the selected one when flat.
    private string ActionKey => _appliedMode == "grouped" ? _appliedKey : SampleShaping.SelectedTag(GroupKeySelector, "Department");

    private void OnMoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var key = ActionKey;
        var from = SampleShaping.KeyOf(person, key);
        _isBulkUpdate = true;
        try
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
        finally
        {
            _isBulkUpdate = false;
        }

        ReapplyIfGroupedOn(key);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, SampleShaping.KeyOf(person, key)));
    }

    private void OnMoveEveryFourthClick(object sender, RoutedEventArgs e)
    {
        var moved = 0;
        _isBulkUpdate = true;
        try
        {
            for (var i = 0; i < People.Count; i += 4)
            {
                People[i].Department = SampleShaping.Next(PersonData.Departments, People[i].Department);
                moved++;
            }
        }
        finally
        {
            _isBulkUpdate = false;
        }

        ReapplyIfGroupedOn(nameof(Person.Department));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0:N0} rows (every 4th) to the next department", moved));
    }

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

    // ---- Readouts ---------------------------------------------------------------------------

    private void RefreshReadouts()
    {
        if (PeopleTable is null || GroupsText is null || ExpansionText is null || RowsText is null || EmptyToggleButton is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(People.Count);
        EmptyToggleButton.Content = People.Count > 0 ? "Remove all rows" : "Restore all rows";

        if (_appliedMode != "grouped")
        {
            GroupsText.Text = "(grouping off)";
            ExpansionText.Text = "(grouping off)";
            return;
        }

        var key = _appliedKey;
        var groups = InViewOrder()
            .GroupBy(p => SampleShaping.KeyOf(p, key))
            .Select(g => string.Format(CultureInfo.CurrentCulture, "{0} {1:N0}", g.Key, g.Count()))
            .ToList();
        GroupsText.Text = groups.Count == 0
            ? "0 (empty source)"
            : string.Format(CultureInfo.CurrentCulture, "{0:N0}: {1}", groups.Count, string.Join(", ", groups));
        ExpansionText.Text = ExpansionSummary();
    }

    // The rows in the order the table shows them (shared rules: SampleShaping.InViewOrder).
    private IEnumerable<Person> InViewOrder() =>
        SampleShaping.InViewOrder(PeopleTable, People, SortKey, _appliedMode == "grouped" ? p => SampleShaping.KeyOf(p, _appliedKey) : null);

    private static IComparable? SortKey(Person person, string path) => path switch
    {
        nameof(Person.FullName) => person.FullName,
        nameof(Person.Department) => person.Department,
        nameof(Person.IsActive) => person.IsActive,
        nameof(Person.JoinDate) => person.JoinDate,
        nameof(Person.Salary) => person.Salary,
        _ => null,
    };

    // Called by ApplyShaping after every reshape. Re-applying GroupBy rebuilds the groups, so
    // restore the bulk expansion state the readout reports; a mixed state resets to expanded.
    private void OnShapingApplied(bool collapse)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        _collapsedGroups.Clear();
        if (collapse)
        {
            _collapsedGroups.UnionWith(CurrentGroupIdentities());
        }

        DispatcherQueue.TryEnqueue(() =>
        {
            if (!IsLoaded || _appliedMode != "grouped")
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

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex), before the
        // later-declared elements exist. Guard EVERY element this path touches.
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
        // Measured against the key the groups were built on, before _appliedKey moves on.
        var wasAllCollapsed = ExpansionSummary() == AllCollapsed;
        _appliedKey = key;

        // Re-applying GroupBy can drop the selection when the selected row changed group.
        SampleShaping.Reselect(PeopleTable, selected, People.Count + 64, RefreshReadouts);
        UpdateShapingGating();
        OnShapingApplied(wasAllCollapsed);
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SampleShaping.Label(GroupKeySelector))
                : "Shaping -> Flat");
        }
        else
        {
            RefreshReadouts();
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
        _collapsedGroups.Clear();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        PeopleTable.CollapseAllGroups();
        _collapsedGroups.Clear();
        _collapsedGroups.UnionWith(CurrentGroupIdentities());
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

/// <summary>Expansion chip text for the custom group header.</summary>
public sealed partial class GroupExpansionTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => value is bool isExpanded && isExpanded ? "Expanded" : "Collapsed";

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotImplementedException();
}

/// <summary>
/// Formats TableViewGroupInfo.ItemCount for the custom group header. Bound to ItemCount (Int32)
/// rather than the ItemCountText projection so the header always shows a count.
/// </summary>
public sealed partial class GroupCountTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        var count = value is int number ? number : 0;
        return string.Format(CultureInfo.CurrentCulture, count == 1 ? "{0} item" : "{0} items", count);
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotImplementedException();
}

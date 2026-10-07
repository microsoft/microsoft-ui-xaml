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
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates grouped rows with the public TableViewSource.GroupBy overload,
/// TableView.GroupHeaderTemplate, and ExpandAllGroups / CollapseAllGroups.
/// </summary>
public sealed partial class GroupsPage : Page
{
    private enum GroupExpansionState
    {
        AllExpanded,
        AllCollapsed,
        Flat,
    }

    private readonly ObservableCollection<Person> _people;
    private readonly List<Person> _stashedPeople = new();

    // Reshaped IN PLACE: GroupBy / ClearGroupBy mutate and return the same TableViewSource
    // (TableViewSource.cpp:49-50), so selection, scroll offset and group expansion survive.
    private TableViewSource? _source;
    private string _mode = "grouped";          // requested
    private string _appliedMode = "grouped";   // applied - every readout and guard reads THIS
    private string _keyName = "Department";
    private int _addedPersonCount;
    private bool _suppressAutoReshape;
    private GroupExpansionState _expansionState = GroupExpansionState.AllExpanded;

    public GroupsPage()
    {
        _people = new ObservableCollection<Person>(PersonData.Take(60));

        InitializeComponent();
        ApplyGroupHeaderTemplate();
        ApplyShaping();

        // The in-cell Department ComboBox / Active CheckBox edit the very value the table may be
        // grouped on, so listen for the commit and re-run the shaping stage - otherwise the row
        // keeps its old group until something else reshapes.
        _people.CollectionChanged += OnPeopleCollectionChanged;
        foreach (var person in _people)
        {
            person.PropertyChanged += OnPersonPropertyChanged;
        }
    }

    private void OnPeopleCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.OldItems is not null)
        {
            foreach (Person person in e.OldItems)
            {
                person.PropertyChanged -= OnPersonPropertyChanged;
            }
        }

        if (e.NewItems is not null)
        {
            foreach (Person person in e.NewItems)
            {
                person.PropertyChanged += OnPersonPropertyChanged;
            }
        }
    }

    private void OnPersonPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_suppressAutoReshape || _appliedMode != "grouped")
        {
            return;
        }

        var affectsGroupKey = e.PropertyName switch
        {
            nameof(Person.Department) => _keyName == "Department",
            nameof(Person.Role) => _keyName == "Role",
            nameof(Person.IsActive) => _keyName == "Active",
            _ => false,
        };

        if (!affectsGroupKey)
        {
            return;
        }

        var person = sender as Person;
        ApplyShaping();
        if (person is not null)
        {
            SetLastAction(string.Format(
                CultureInfo.InvariantCulture,
                "Edited in cell - {0} {1} moved to {2}",
                person.FirstName,
                person.LastName,
                GroupValue(person, _keyName)));
        }
    }

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTableControl is null || ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _mode = tag;
        ApplyShaping();
        SetLastAction(string.Format(CultureInfo.InvariantCulture, "Shaping mode -> {0}", _appliedMode == "grouped" ? "Grouped" : "Flat"));
    }

    private void OnGroupByChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTableControl is null)
        {
            return;
        }

        if (GroupBySelector?.SelectedItem is ComboBoxItem item && item.Tag is string tag)
        {
            _keyName = tag;
            ApplyShaping();
            SetLastAction(string.Format(CultureInfo.InvariantCulture, "Group key -> {0}", GroupLabel(tag)));
        }
    }

    private void OnPeopleSelectionChanged(object sender, SelectionChangedEventArgs e) => UpdateReadout();

    private void OnGroupHeaderTemplateToggled(object sender, RoutedEventArgs e)
    {
        if (PeopleTableControl is null)
        {
            return;
        }

        ApplyGroupHeaderTemplate();
        UpdateReadout();
    }

    private void ApplyShaping()
    {
        if (PeopleTableControl is null)
        {
            return;
        }

        if (_source is null)
        {
            _source = TableViewSource.From(_people);
            PeopleTableControl.ItemsSource = _source;
        }

        switch (_mode)
        {
            case "grouped":
                var key = _keyName;
                // TableViewKeySelector receives the ROW ITEM; TableViewIdentitySelector receives
                // the GROUP KEY (TableViewSource.idl:12-16). An item-typed identity lambda returns
                // an empty identity and fails fast with E_INVALIDARG, so grouping would silently
                // never apply.
                _source.GroupBy(
                    item => (object)GroupValue(item, key),
                    groupKey => groupKey?.ToString() ?? "(none)");
                break;

            // case "hierarchy":
            // case "groupedhierarchy":
            //     Hierarchical rows are not available in this release, and no hierarchy API exists
            //     on TableViewSource or TableView yet, so there is deliberately no call written
            //     here to copy. When the control ships hierarchy support, apply it to this same
            //     source instance alongside the GroupBy stage above so the two compose, and drop
            //     the IsEnabled="False" from the matching options in the XAML.
            //     break;

            default:
                _source.ClearGroupBy();
                break;
        }

        // Set ONLY after the shaping call returns.
        _appliedMode = _mode;

        if (_appliedMode == "grouped")
        {
            if (_expansionState == GroupExpansionState.Flat)
            {
                _expansionState = GroupExpansionState.AllExpanded;
            }

            ApplyExpansionState();
        }
        else
        {
            _expansionState = GroupExpansionState.Flat;
        }

        UpdateReadout();
    }

    private void ApplyGroupHeaderTemplate()
    {
        // Null-guarded: Toggled fires from the declarative IsOn="True" DURING InitializeComponent,
        // before the rest of the Options rail exists.
        if (PeopleTableControl is null || UseCustomHeaderTemplateToggle is null)
        {
            return;
        }

        PeopleTableControl.GroupHeaderTemplate = UseCustomHeaderTemplateToggle.IsOn
            ? (DataTemplate)Resources["CustomGroupHeaderTemplate"]
            : null;
    }

    private void OnExpandAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            UpdateReadout();
            return;
        }

        PeopleTableControl.ExpandAllGroups();
        _expansionState = GroupExpansionState.AllExpanded;
        UpdateReadout();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            UpdateReadout();
            return;
        }

        PeopleTableControl.CollapseAllGroups();
        _expansionState = GroupExpansionState.AllCollapsed;
        UpdateReadout();
        SetLastAction("Collapsed all groups");
    }

    private void OnMoveSelectedGroupClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTableControl?.SelectedItem is not Person selected)
        {
            return;
        }

        var before = GroupValue(selected, _keyName);
        _suppressAutoReshape = true;
        switch (_keyName)
        {
            case "Role":
                selected.Role = NextInRing(PersonData.All.Select(p => p.Role).Distinct(StringComparer.Ordinal).OrderBy(r => r, StringComparer.Ordinal).ToList(), selected.Role);
                break;
            case "Active":
                selected.IsActive = !selected.IsActive;
                break;
            default:
                selected.Department = NextInRing(PersonData.Departments, selected.Department);
                break;
        }

        // Re-running the shaping stage re-reads every key, so the mutated row re-buckets.
        _suppressAutoReshape = false;
        ApplyShaping();
        SetLastAction(string.Format(
            CultureInfo.InvariantCulture,
            "{0} {1}: {2} -> {3}",
            selected.FirstName,
            selected.LastName,
            before,
            GroupValue(selected, _keyName)));
    }

    private static string NextInRing(IReadOnlyList<string> ring, string current)
    {
        if (ring.Count == 0)
        {
            return current;
        }

        var index = -1;
        for (var i = 0; i < ring.Count; i++)
        {
            if (string.Equals(ring[i], current, StringComparison.Ordinal))
            {
                index = i;
                break;
            }
        }

        return ring[(index + 1 + ring.Count) % ring.Count];
    }

    private void OnAddPersonClick(object sender, RoutedEventArgs e)
    {
        _addedPersonCount++;
        var departments = PersonData.Departments;
        var added = new Person
        {
            FirstName = "New",
            LastName = string.Format(CultureInfo.InvariantCulture, "Hire {0}", _addedPersonCount),
            Email = string.Format(CultureInfo.InvariantCulture, "new.hire{0}@contoso.com", _addedPersonCount),
            Department = departments.Count > 0 ? departments[0] : "Engineering",
            Role = "Associate",
            JoinDate = DateTimeOffset.Now,
            Salary = 65000,
            IsActive = true,
        };

        _people.Insert(0, added);
        ApplyShaping();
        SetLastAction(string.Format(CultureInfo.InvariantCulture, "Added a row to {0}", added.Department));
    }

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTableControl?.SelectedItem is not Person selected)
        {
            return;
        }

        var group = GroupValue(selected, _keyName);
        if (_people.Remove(selected))
        {
            ApplyShaping();
            SetLastAction(string.Format(CultureInfo.InvariantCulture, "Removed {0} {1} from {2}", selected.FirstName, selected.LastName, group));
        }
    }

    private void OnEmptyToggleClick(object sender, RoutedEventArgs e)
    {
        if (_stashedPeople.Count > 0)
        {
            foreach (var person in _stashedPeople)
            {
                _people.Add(person);
            }

            var restored = _stashedPeople.Count;
            _stashedPeople.Clear();
            ApplyShaping();
            SetLastAction(string.Format(CultureInfo.InvariantCulture, "Restored {0} rows", restored));
            return;
        }

        _stashedPeople.AddRange(_people);
        _people.Clear();
        ApplyShaping();
        SetLastAction("Removed all rows - the grouped projection now has zero groups");
    }

    private void SetLastAction(string text)
    {
        if (LastActionTextBlock is not null)
        {
            LastActionTextBlock.Text = text;
        }
    }

    private void OnShuffleClick(object sender, RoutedEventArgs e)
    {
        var random = new Random();
        var departments = PersonData.Departments;
        var touched = 0;
        _suppressAutoReshape = true;
        for (int i = 0; i < _people.Count; i += 4)
        {
            _people[i].Department = departments[random.Next(departments.Count)];
            touched++;
        }

        _suppressAutoReshape = false;
        ApplyShaping();
        SetLastAction(string.Format(CultureInfo.InvariantCulture, "Reassigned {0} rows across departments", touched));
    }

    private void ApplyExpansionState()
    {
        DispatcherQueue.TryEnqueue(() =>
        {
            if (PeopleTableControl is null || _appliedMode != "grouped")
            {
                return;
            }

            if (_expansionState == GroupExpansionState.AllCollapsed)
            {
                PeopleTableControl.CollapseAllGroups();
            }
            else
            {
                PeopleTableControl.ExpandAllGroups();
            }

            UpdateReadout();
        });
    }

    private void UpdateReadout()
    {
        if (SourceModeTextBlock is null)
        {
            return;
        }

        var grouped = _appliedMode == "grouped";
        var hasSelection = PeopleTableControl?.SelectedItem is Person;

        SetActionState(ExpandAllButton, grouped, "Expand every group.");
        SetActionState(CollapseAllButton, grouped, "Collapse every group.");
        SetActionState(MoveGroupButton, hasSelection, "Rewrites the selected row's group key, then re-applies GroupBy so the row moves between groups.", "Select a row first.");
        SetActionState(RemovePersonButton, hasSelection, "Removes the row; its group disappears when it was the last one.", "Select a row first.");

        if (EmptyToggleButton is not null)
        {
            EmptyToggleButton.Content = _stashedPeople.Count > 0
                ? "Restore all rows"
                : "Remove all rows (group an empty set)";
        }

        if (UseCustomHeaderTemplateToggle is not null)
        {
            HeaderTemplateTextBlock.Text = UseCustomHeaderTemplateToggle.IsOn ? "Custom GroupHeaderTemplate" : "Built-in group header template";
        }

        if (grouped)
        {
            var groups = GetGroupCounts().ToList();
            int groupCount = groups.Count;
            int totalCount = groups.Sum(g => g.Count);

            SourceModeTextBlock.Text = string.Format(CultureInfo.InvariantCulture, "Grouped by {0}", GroupLabel(_keyName));
            ExpansionStateTextBlock.Text = _expansionState == GroupExpansionState.AllCollapsed
                ? "All groups collapsed"
                : "All groups expanded";
            GroupCountTextBlock.Text = groupCount.ToString(CultureInfo.InvariantCulture);
            TotalPeopleTextBlock.Text = totalCount.ToString(CultureInfo.InvariantCulture);
            SourceShapeTextBlock.Text = string.Format(
                CultureInfo.InvariantCulture,
                "_source.GroupBy({0} key selector, group-key identity selector)",
                GroupLabel(_keyName));
            PerGroupCountsTextBlock.Text = groupCount == 0
                ? "(no rows - empty grouped projection)"
                : string.Join(", ", groups.Select(g => string.Format(CultureInfo.InvariantCulture, "{0}: {1}", g.Key, g.Count)));
        }
        else
        {
            SourceModeTextBlock.Text = "Flat TableViewSource";
            ExpansionStateTextBlock.Text = "Grouping off";
            GroupCountTextBlock.Text = "(n/a)";
            TotalPeopleTextBlock.Text = _people.Count.ToString(CultureInfo.InvariantCulture);
            SourceShapeTextBlock.Text = "_source.ClearGroupBy()";
            PerGroupCountsTextBlock.Text = "(grouping off)";
        }
    }

    private static void SetActionState(Button? button, bool enabled, string enabledTip, string disabledTip = "Available once Grouped mode is selected.")
    {
        if (button is null)
        {
            return;
        }

        button.IsEnabled = enabled;
        ToolTipService.SetToolTip(button, enabled ? enabledTip : disabledTip);
    }

    private IEnumerable<(string Key, int Count)> GetGroupCounts()
    {
        var keyName = _keyName;

        return _people
            .GroupBy(person => GroupValue(person, keyName), StringComparer.Ordinal)
            .OrderBy(g => g.Key, StringComparer.Ordinal)
            .Select(g => (g.Key, g.Count()));
    }

    // Never returns string.Empty: an empty group identity is an E_INVALIDARG fail-fast.
    private static string GroupValue(object item, string keyName)
    {
        if (item is not Person person)
        {
            return "(none)";
        }

        var value = keyName switch
        {
            "Role" => person.Role,
            "Active" => person.IsActive ? "Active" : "Inactive",
            _ => person.Department,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private static string GroupLabel(string keyName) => keyName switch
    {
        "Role" => "role",
        "Active" => "active status",
        _ => "department",
    };
}

public sealed partial class GroupExpansionTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => value is bool isExpanded && isExpanded ? "Expanded" : "Collapsed";

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotImplementedException();
}

/// <summary>
/// The department vocabulary for the in-cell Department ComboBox. Exposed as a page resource
/// because WinUI has no x:Array, and a cell template cannot reach a page property by ElementName.
/// </summary>
public sealed class DepartmentChoices : List<string>
{
    public DepartmentChoices() => AddRange(PersonData.Departments);
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

/// <summary>
/// Formats a numeric salary as whole currency units so the column reads as money rather than a
/// bare integer.
/// </summary>
public sealed partial class CurrencyTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => value is double amount
            ? amount.ToString("C0", CultureInfo.CurrentCulture)
            : value?.ToString() ?? string.Empty;

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotImplementedException();
}

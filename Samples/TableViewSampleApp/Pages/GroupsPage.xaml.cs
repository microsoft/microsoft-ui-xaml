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
    private bool _groupingEnabled;
    private string _keyName = "Department";
    private GroupExpansionState _expansionState = GroupExpansionState.AllExpanded;

    public GroupsPage()
    {
        _people = new ObservableCollection<Person>(PersonData.Take(60));

        InitializeComponent();
        ApplyGroupHeaderTemplate();
        ApplyGroupingMode(grouped: true);
    }

    private void OnGroupingToggled(object sender, RoutedEventArgs e)
    {
        if (PeopleTableControl is null)
        {
            return;
        }

        ApplyGroupingMode(GroupingToggleControl.IsOn);
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
            ApplyGroupingMode(_groupingEnabled);
        }
    }

    private void OnGroupHeaderTemplateToggled(object sender, RoutedEventArgs e)
    {
        if (PeopleTableControl is null)
        {
            return;
        }

        ApplyGroupHeaderTemplate();
        UpdateReadout();
    }

    private void ApplyGroupingMode(bool grouped)
    {
        _groupingEnabled = grouped;

        if (grouped)
        {
            if (_expansionState == GroupExpansionState.Flat)
            {
                _expansionState = GroupExpansionState.AllExpanded;
            }

            PeopleTableControl.ItemsSource = TableViewSource
                .From(_people)
                .GroupBy(GetGroupKeySelector(_keyName), GroupIdentity);
            ApplyExpansionState();
        }
        else
        {
            _expansionState = GroupExpansionState.Flat;
            PeopleTableControl.ItemsSource = TableViewSource.From(_people).ClearGroupBy();
        }

        UpdateReadout();
    }

    private void ApplyGroupHeaderTemplate()
    {
        PeopleTableControl.GroupHeaderTemplate = UseCustomHeaderTemplateToggle.IsOn
            ? (DataTemplate)Resources["CustomGroupHeaderTemplate"]
            : null;
    }

    private void OnExpandAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (!_groupingEnabled)
        {
            UpdateReadout();
            return;
        }

        PeopleTableControl.ExpandAllGroups();
        _expansionState = GroupExpansionState.AllExpanded;
        UpdateReadout();
    }

    private void OnCollapseAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (!_groupingEnabled)
        {
            UpdateReadout();
            return;
        }

        PeopleTableControl.CollapseAllGroups();
        _expansionState = GroupExpansionState.AllCollapsed;
        UpdateReadout();
    }

    private void OnShuffleClick(object sender, RoutedEventArgs e)
    {
        var random = new Random();
        var departments = PersonData.Departments;
        for (int i = 0; i < _people.Count; i += 4)
        {
            _people[i].Department = departments[random.Next(departments.Count)];
        }

        ApplyGroupingMode(_groupingEnabled);
    }

    private void ApplyExpansionState()
    {
        DispatcherQueue.TryEnqueue(() =>
        {
            if (PeopleTableControl is null || !_groupingEnabled)
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

        HeaderTemplateTextBlock.Text = UseCustomHeaderTemplateToggle.IsOn ? "Custom GroupHeaderTemplate" : "Built-in group header template";

        if (_groupingEnabled)
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
                "TableViewSource.From(_people).GroupBy({0} selector, GroupIdentity)",
                GroupLabel(_keyName));
            PerGroupCountsTextBlock.Text = string.Join(", ", groups.Select(g => string.Format(CultureInfo.InvariantCulture, "{0}: {1}", g.Key, g.Count)));
        }
        else
        {
            SourceModeTextBlock.Text = "Flat TableViewSource";
            ExpansionStateTextBlock.Text = "Grouping off";
            GroupCountTextBlock.Text = "(n/a)";
            TotalPeopleTextBlock.Text = _people.Count.ToString(CultureInfo.InvariantCulture);
            SourceShapeTextBlock.Text = "TableViewSource.From(_people).ClearGroupBy()";
            PerGroupCountsTextBlock.Text = "(grouping off)";
        }
    }

    private IEnumerable<(string Key, int Count)> GetGroupCounts()
    {
        var keySelector = GetPersonGroupKeySelector(_keyName);

        return _people
            .GroupBy(keySelector)
            .OrderBy(g => g.Key, StringComparer.Ordinal)
            .Select(g => (g.Key, g.Count()));
    }

    private static Func<Person, string> GetPersonGroupKeySelector(string keyName) => keyName switch
    {
        "Role" => person => CoalesceGroupKey(person.Role),
        "Active" => person => person.IsActive ? "Active" : "Inactive",
        _ => person => CoalesceGroupKey(person.Department),
    };

    private static TableViewKeySelector GetGroupKeySelector(string keyName) => item =>
    {
        var person = (Person)item;
        return GetPersonGroupKeySelector(keyName)(person);
    };

    private static string GroupIdentity(object key) => CoalesceGroupKey(key?.ToString());

    private static string CoalesceGroupKey(string? key) => string.IsNullOrWhiteSpace(key) ? "(none)" : key;

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

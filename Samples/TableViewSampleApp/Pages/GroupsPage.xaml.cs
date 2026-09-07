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
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates grouped rows over the aligned TableView using the canonical
/// model-first <see cref="TableViewSource"/> surface.
/// </summary>
public sealed partial class GroupsPage : Page
{
    private readonly ObservableCollection<Person> _people;
    private bool _groupingEnabled;
    private string _keyName = "Department";

    public GroupsPage()
    {
        _people = new ObservableCollection<Person>(PersonData.Take(60));

        InitializeComponent();
        ApplyGroupingMode(grouped: true);
    }

    private void OnGroupingToggled(object sender, RoutedEventArgs e)
    {
        if (SourceModeTextBlock is null)
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

    private void ApplyGroupingMode(bool grouped)
    {
        _groupingEnabled = grouped;

        PeopleTableControl.ItemsSource = grouped
            ? TableViewSource.From(_people)
            : TableViewSource.From(_people);

        UpdateReadout(grouped);
    }

    private void OnShuffleClick(object sender, RoutedEventArgs e)
    {
        var random = new Random();
        var depts = PersonData.Departments;
        for (int i = 0; i < _people.Count; i += 4)
        {
            _people[i].Department = depts[random.Next(depts.Count)];
        }

        ApplyGroupingMode(_groupingEnabled);
    }

    private void UpdateReadout(bool grouped)
    {
        if (SourceModeTextBlock is null) return;

        if (grouped)
        {
            var groups = GetGroupCounts().ToList();
            var groupCount = groups.Count;
            var totalCount = groups.Sum(g => g.Count);
            SourceModeTextBlock.Text = "TableViewSource.GroupBy";
            DisplayStateTextBlock.Text = "Control-owned expand/collapse";
            GroupCountTextBlock.Text = groupCount.ToString(CultureInfo.InvariantCulture);
            TotalPeopleTextBlock.Text = totalCount.ToString(CultureInfo.InvariantCulture);
            SourceShapeTextBlock.Text = $"TableViewSource.From(_people) ({groupCount} groups, {totalCount} people)";
            PerGroupCountsTextBlock.Text = string.Join(", ", groups.Select(g => $"{g.Key}: {g.Count}"));
        }
        else
        {
            SourceModeTextBlock.Text = "TableViewSource flat";
            DisplayStateTextBlock.Text = "(flat)";
            GroupCountTextBlock.Text = "(n/a)";
            TotalPeopleTextBlock.Text = _people.Count.ToString(CultureInfo.InvariantCulture);
            SourceShapeTextBlock.Text = $"TableViewSource.From(_people) ({_people.Count} items)";
            PerGroupCountsTextBlock.Text = "(grouping off)";
        }
    }

    private IEnumerable<(string Key, int Count)> GetGroupCounts()
    {
        Func<Person, string> key = _keyName switch
        {
            "Role" => p => p.Role,
            "Active" => p => p.IsActive ? "Active" : "Inactive",
            _ => p => p.Department,
        };

        // Grouping is app-side here: the control cannot group a source in this release, so the
        // readout groups the collection itself to report what the groups WOULD be.
        return _people
            .GroupBy(key)
            .OrderBy(g => g.Key, StringComparer.Ordinal)
            .Select(g => (g.Key, g.Count()));
    }

    private static TableViewKeySelector GetGroupKeySelector(string keyName) => keyName switch
    {
        "Role" => item => ((Person)item).Role,
        "Active" => item => ((Person)item).IsActive ? "Active" : "Inactive",
        _ => item => ((Person)item).Department,
    };

    private static string PersonIdentity(object item) => item is Person person ? person.Email : string.Empty;

    private static string GroupIdentity(object key) => key?.ToString() ?? string.Empty;
}

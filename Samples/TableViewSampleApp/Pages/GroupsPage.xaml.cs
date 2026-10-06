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
    private readonly string[] _originalDepartments;
    private bool _updating = true;
    private int _rotationStep;
    private bool _groupingEnabled;
    private string _keyName = "Department";

    public GroupsPage()
    {
        _people = new ObservableCollection<Person>(PersonData.Take(60));
        _originalDepartments = _people.Select(person => person.Department).ToArray();

        InitializeComponent();
        _updating = false;
        ApplyGroupingMode(grouped: true);
    }

    private void OnGroupingToggled(object sender, RoutedEventArgs e)
    {
        if (_updating || SourceModeTextBlock is null)
        {
            return;
        }

        ApplyGroupingMode(GroupingToggleControl.IsOn);
    }

    private void OnGroupByChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_updating || PeopleTableControl is null)
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
            ? TableViewSource.From(_people).GroupBy(GetGroupKeySelector(_keyName))
            : TableViewSource.From(_people);

        UpdateReadout(grouped);
    }

    private void OnRotateDepartmentsClick(object sender, RoutedEventArgs e)
    {
        var depts = PersonData.Departments.ToArray();
        _rotationStep = (_rotationStep + 1) % depts.Length;
        for (int i = 0; i < _people.Count; i += 4)
        {
            var originalIndex = Array.IndexOf(depts, _originalDepartments[i]);
            _people[i].Department = depts[(originalIndex + _rotationStep) % depts.Length];
        }

        ApplyGroupingMode(_groupingEnabled);
    }

    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        _updating = true;
        try
        {
            _rotationStep = 0;
            for (int i = 0; i < _people.Count; i++)
                _people[i].Department = _originalDepartments[i];
            _keyName = "Department";
            GroupBySelector.SelectedIndex = 0;
            GroupingToggleControl.IsOn = true;
            PeopleTableControl.ClearSort();
            PeopleTableControl.DeselectAll();
        }
        finally
        {
            _updating = false;
        }
        ApplyGroupingMode(grouped: true);
        PeopleTableControl.ExpandAllGroups();
    }

    private void UpdateReadout(bool grouped)
    {
        if (SourceModeTextBlock is null) return;
        LastMutationTextBlock.Text = _rotationStep == 0
            ? "Department rotation: baseline (step 0)."
            : $"Department rotation: step {_rotationStep}; 15 of 60 source objects changed from baseline.";

        if (grouped)
        {
            var groups = GetGroupCounts().ToList();
            var groupCount = groups.Count;
            var totalCount = groups.Sum(g => g.Count);
            SourceModeTextBlock.Text = "TableViewSource.GroupBy";
            DisplayStateTextBlock.Text = "Control-owned expand/collapse";
            GroupCountTextBlock.Text = groupCount.ToString(CultureInfo.InvariantCulture);
            TotalPeopleTextBlock.Text = totalCount.ToString(CultureInfo.InvariantCulture);
            SourceShapeTextBlock.Text = $"TableViewSource.From(_people).GroupBy({_keyName}) ({groupCount} groups, {totalCount} people)";
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

        // Summarize the same keys passed to the control's grouped source.
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

}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
using TableViewDensity = Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates <c>TableView.Density</c> — the Compact / Standard / Comfortable
/// row-height and cell-padding presets — switched live over plain text columns.
///
/// Grouping is demonstrated on the same table on purpose: a group header has its
/// own height, so density and grouping are a real interaction rather than two
/// unrelated knobs. Read-only / in-place editing lives on the Cell editing page.
/// </summary>
public sealed partial class DensityReadOnlyPage : Page
{
    private TableViewDensity _density = TableViewDensity.Standard;
    private TableViewSource? _source;
    private string _mode = "flat";
    private string _groupKey = "Department";
    private string _appliedMode = "Flat";
    private readonly List<Person> _stash = new();

    public DensityReadOnlyPage()
    {
        InitializeComponent();

        foreach (var person in PersonData.Take(20))
        {
            People.Add(person);
        }

        _source = TableViewSource.From(People);
        DemoTable.ItemsSource = _source;

        // Re-apply the XAML defaults now that DemoTable exists (the Standard radio's
        // Checked handler ran during InitializeComponent while DemoTable was still null).
        DemoTable.Density = _density;

        ApplyGrouping();
    }

    public ObservableCollection<Person> People { get; } = new();

    private void OnDensitySelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (DemoTable is null ||
            sender is not RadioButtons { SelectedItem: FrameworkElement { Tag: string tag } })
        {
            return;
        }

        if (!Enum.TryParse<TableViewDensity>(tag, ignoreCase: false, out var density))
        {
            Debug.Fail($"DensityReadOnlyPage: unrecognised density Tag '{tag}'.");
            return;
        }

        _density = density;
        DemoTable.Density = density;
        UpdateStatus();
    }

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _mode = tag;
        ApplyGrouping();
    }

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || GroupKeySelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        ApplyGrouping();
    }

    private void OnExpandAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode == "Flat")
        {
            return;
        }

        DemoTable.ExpandAllGroups();
        UpdateStatus();
    }

    private void OnCollapseAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode == "Flat")
        {
            return;
        }

        DemoTable.CollapseAllGroups();
        UpdateStatus();
    }

    /// <summary>
    /// Reshapes the existing <see cref="TableViewSource"/> in place. GroupBy /
    /// ClearGroupBy mutate and return the same instance, so the source is never
    /// rebuilt — rebuilding would drop selection, scroll offset and expansion.
    /// </summary>
    private void ApplyGrouping()
    {
        if (_source is null)
        {
            return;
        }

        var grouped = _mode == "grouped";
        switch (_mode)
        {
            case "grouped":
            {
                var key = _groupKey;
                // The key selector receives the item; the identity selector receives the
                // group KEY produced above, so it only has to stringify it.
                _source.GroupBy(
                    item => (object)GroupValue(item, key),
                    groupKey => groupKey?.ToString() ?? "(none)");

                // Applied state is recorded only after the reshape returns, so the readout
                // can never claim a mode the source never took.
                _appliedMode = $"Grouped by {key}";
                break;
            }

            // case "hierarchy":
            // case "groupedhierarchy":
            //     Hierarchical rows are not available in this release. TableViewSource.idl
            //     exposes only Filter / GroupBy / Sort and their Clear* counterparts, so
            //     there is no hierarchy verb to call here yet and nothing is written rather
            //     than naming an API that does not exist. When the control ships hierarchy
            //     support, apply it to THIS same source instance alongside the GroupBy stage
            //     above so the two axes compose rather than replace one another, then remove
            //     IsEnabled="False" from the two hierarchy items in the Shaping mode selector.
            //     break;

            default:
                _source.ClearGroupBy();
                _appliedMode = "Flat";
                break;
        }

        if (ExpandAllButton is not null)
        {
            GroupKeySelector.IsEnabled = grouped;
            ExpandAllButton.IsEnabled = grouped;
            CollapseAllButton.IsEnabled = grouped;
        }

        if (grouped)
        {
            DispatcherQueue.TryEnqueue(() => DemoTable?.ExpandAllGroups());
        }

        UpdateStatus();
    }

    private static string GroupValue(object item, string key)
    {
        if (item is not Person person)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Role" => person.Role,
            _ => person.Department,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private void UpdateStatus()
    {
        if (DensityValueText is null)
        {
            return;
        }

        DensityValueText.Text = _density.ToString();
        RowCountValueText.Text = People.Count.ToString("N0", CultureInfo.CurrentCulture);
        GroupedByValueText.Text = _appliedMode;
    }

    // ---- actions -------------------------------------------------------------------

    private void OnTableSelectionChanged(TableView sender, SelectionChangedEventArgs args) =>
        UpdateActionAvailability();

    private void UpdateActionAvailability()
    {
        if (ChangeGroupValueButton is null)
        {
            return;
        }

        var hasSelection = DemoTable.SelectedItem is Person;
        ChangeGroupValueButton.IsEnabled = hasSelection;
        DuplicateRowButton.IsEnabled = hasSelection;
        RemoveRowButton.IsEnabled = hasSelection;
    }

    /// <summary>
    /// Rewrites whichever property the table is grouped on, so the row moves
    /// between groups in place rather than through a source rebuild.
    /// </summary>
    private void OnChangeGroupValueClick(object sender, RoutedEventArgs e)
    {
        if (DemoTable.SelectedItem is not Person person)
        {
            return;
        }

        var byRole = _appliedMode.EndsWith("Role", StringComparison.Ordinal);
        var pool = byRole
            ? People.Select(p => p.Role).Distinct(StringComparer.Ordinal).OrderBy(r => r, StringComparer.Ordinal).ToList()
            : PersonData.Departments.ToList();

        if (pool.Count == 0)
        {
            return;
        }

        var current = byRole ? person.Role : person.Department;
        var next = pool[(pool.IndexOf(current) + 1) % pool.Count];

        if (byRole)
        {
            person.Role = next;
            SetLastAction($"{person.FullName} moved to role {next}.");
        }
        else
        {
            person.Department = next;
            SetLastAction($"{person.FullName} moved to department {next}.");
        }
    }

    private void OnDuplicateRowClick(object sender, RoutedEventArgs e)
    {
        if (DemoTable.SelectedItem is not Person person)
        {
            return;
        }

        var copy = new Person
        {
            FirstName = person.FirstName,
            LastName = person.LastName + " (copy)",
            Email = person.Email,
            Department = person.Department,
            Role = person.Role,
            JoinDate = person.JoinDate,
            Salary = person.Salary,
            IsActive = person.IsActive,
        };

        People.Insert(People.IndexOf(person) + 1, copy);
        SetLastAction($"Added {copy.FullName} to {copy.Department}.");
    }

    private void OnRemoveRowClick(object sender, RoutedEventArgs e)
    {
        if (DemoTable.SelectedItem is not Person person)
        {
            return;
        }

        People.Remove(person);
        SetLastAction($"Removed {person.FullName} from {person.Department}.");
        UpdateActionAvailability();
    }

    /// <summary>
    /// Edge case: grouping over an empty set, then over a repopulated one.
    /// </summary>
    private void OnToggleEmptyClick(object sender, RoutedEventArgs e)
    {
        if (People.Count > 0)
        {
            _stash.Clear();
            foreach (var person in People)
            {
                _stash.Add(person);
            }

            People.Clear();
            EmptyToggleButton.Content = "Restore rows";
            SetLastAction("Cleared every row; the source is now grouped over an empty set.");
        }
        else
        {
            foreach (var person in _stash)
            {
                People.Add(person);
            }

            _stash.Clear();
            EmptyToggleButton.Content = "Clear all rows";
            SetLastAction("Restored the original rows in source order.");
        }

        UpdateActionAvailability();
    }

    private void SetLastAction(string action)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = action;
        }

        UpdateStatus();
    }
}

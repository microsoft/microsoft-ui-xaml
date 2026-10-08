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
public sealed partial class DensityReadOnlyPage : Page
{
    private readonly Queue<Person> _spares = new(PersonData.Take(60).Skip(40));
    private readonly List<Person> _stash = new();
    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";

    public DensityReadOnlyPage()
    {
        _source = TableViewSource.From(People);
        InitializeComponent();
        RefreshReadouts();
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource? Source => _source;

    private int MaxProbeIndex => People.Count + PersonData.Roles.Count;

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

        var key = _appliedMode == "grouped" ? _appliedKey : nameof(Person.Department);
        var from = SampleShaping.KeyOf(person, key);
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
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} to {1}", person.FullName, SampleShaping.KeyOf(person, _appliedKey)));
    }

    private void OnRemoveRowClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        People.Remove(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0} from {1}", person.FullName, SampleShaping.KeyOf(person, _appliedKey)));
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

    private void RefreshReadouts()
    {
        if (RowsText is null || DensityText is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(People.Count);
        DensityText.Text = PeopleTable.Density.ToString();
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex="0"), before the
        // later-declared elements exist. Guard every element this path touches.
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
        _appliedKey = key;

        // Re-applying GroupBy can drop the selection when the selected row changed group.
        SampleShaping.Reselect(PeopleTable, selected, MaxProbeIndex, RefreshReadouts);

        UpdateShapingGating();
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SampleShaping.Label(GroupKeySelector))
                : "Shaping -> Flat");
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
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        PeopleTable.CollapseAllGroups();
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

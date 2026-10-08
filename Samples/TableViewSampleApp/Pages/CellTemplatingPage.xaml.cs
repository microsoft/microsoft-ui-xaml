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
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableViewSampleApp.Templates;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Cell templating: TableViewTemplateColumn cells that are read-only visuals (avatar, chip) or
/// two-way bound editors (CheckBox, date and time pickers, ComboBox, Button + Flyout), the
/// templates shared from Templates\PersonCellTemplates.xaml, and a run-time CellTemplate swap.
/// This is the reference page the other feature pages copy.
/// </summary>
public sealed partial class CellTemplatingPage : Page
{
    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private bool _isBulkUpdate;

    public CellTemplatingPage()
    {
        _source = TableViewSource.From(People);
        InitializeComponent();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        RefreshReadouts();
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource? Source => _source;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.PropertyChanged += OnPersonChanged;
        }

        PersonCellTemplates.DetailsOpened += OnDetailsOpened;
        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        PersonCellTemplates.DetailsOpened -= OnDetailsOpened;
    }

    // ---- Join date editor: swap the column's CellTemplate at run time -----------------------

    private void OnJoinDateEditorChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="0"), when the XAML already shows the
        // compact editor and the column is not attached to the table yet.
        if (JoinDateColumn is null || !IsLoaded)
        {
            return;
        }

        var full = SampleShaping.SelectedTag(JoinDateEditorSelector, "calendar") == "datepicker";
        var resources = Application.Current.Resources;
        JoinDateColumn.CellTemplate = (DataTemplate)resources[full ? "JoinDatePickerTemplate" : "JoinCalendarDateTemplate"];
        JoinDateColumn.Width = new GridLength((double)resources[full ? "ColWidthDatePicker" : "ColWidthCalendarDate"]);
        SetLastAction(full
            ? "Join date editor -> DatePicker (full, 310 px)"
            : "Join date editor -> CalendarDatePicker (compact, 170 px)");
    }

    // ---- In-cell edits ------------------------------------------------------------------

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || _isBulkUpdate)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(Person.IsActive):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Active -> {0} for {1}", person.IsActive ? "checked" : "unchecked", person.FullName));
                break;
            case nameof(Person.JoinDate):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Join date -> {0:d} for {1}", person.JoinDate, person.FullName));
                break;
            case nameof(Person.ShiftStart):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Shift start -> {0:hh\\:mm} for {1}", person.ShiftStart, person.FullName));
                break;
            case nameof(Person.Office):
            case nameof(Person.Department):
                ReapplyIfGroupedOn(e.PropertyName);
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} -> {1} for {2}",
                    e.PropertyName, e.PropertyName == nameof(Person.Office) ? person.Office : person.Department, person.FullName));
                break;
        }
    }

    private void OnDetailsOpened(object? sender, Person person) =>
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Opened the details flyout for {0}", person.FullName));

    // ---- Actions ------------------------------------------------------------------------

    private void OnCheckAllActiveClick(object sender, RoutedEventArgs e) => SetAllActive(true);

    private void OnUncheckAllActiveClick(object sender, RoutedEventArgs e) => SetAllActive(false);

    private void SetAllActive(bool isActive)
    {
        _isBulkUpdate = true;
        try
        {
            foreach (var person in People)
            {
                person.IsActive = isActive;
            }
        }
        finally
        {
            _isBulkUpdate = false;
        }

        ReapplyIfGroupedOn(nameof(Person.IsActive));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} all {1:N0} rows", isActive ? "Checked Active on" : "Unchecked Active on", People.Count));
    }

    private void OnChangeDepartmentClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = person.Department;
        var to = SampleShaping.Next(PersonData.Departments, from);
        _isBulkUpdate = true;
        try
        {
            person.Department = to;
        }
        finally
        {
            _isBulkUpdate = false;
        }

        // GroupBy takes a delegate, not a property path, so the control cannot re-bucket the row
        // on PropertyChanged; re-apply the grouping when the grouped-on value changed.
        ReapplyIfGroupedOn(nameof(Person.Department));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, to));
    }

    private void OnOpenDetailsClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        // Success is reported by OnDetailsOpened.
        if (!PersonCellTemplates.TryOpenDetails(PeopleTable, person))
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "The row for {0} is not on screen; scroll to it first.", person.FullName));
        }
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || ActiveCountText is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(People.Count);
        ActiveCountText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} of {1:N0}", People.Count(p => p.IsActive), People.Count);
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex="0"), before the
        // later-declared elements exist. Guard EVERY element this path touches, including the
        // ones UpdateShapingGating writes, or the page fails to load.
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
        SampleShaping.Reselect(PeopleTable, selected, People.Count + PersonData.Departments.Count + PersonData.Offices.Count, RefreshReadouts);
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

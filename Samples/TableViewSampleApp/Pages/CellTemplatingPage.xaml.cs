// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
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
/// </summary>
public sealed partial class CellTemplatingPage : SamplePageBase
{
    public CellTemplatingPage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        // </snippet>
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));

        // <snippet>
        // The editors are two-way bound, so nothing commits them by hand. The page only observes
        // the model to report each commit, while it is loaded.
        TrackItems(People, OnPersonChanged);
        TrackLifetime(
            () => PersonCellTemplates.DetailsOpened += OnDetailsOpened,
            () => PersonCellTemplates.DetailsOpened -= OnDetailsOpened);
        // </snippet>
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(40);

    public TableViewSource Source { get; }

    // <snippet>
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
    // </snippet>

    // ---- In-cell edits ------------------------------------------------------------------

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || IsBulkUpdating)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(Person.IsActive):
                ReapplyIfGroupedOn(e.PropertyName);
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
        using (BeginBulkUpdate())
        {
            foreach (var person in People)
            {
                person.IsActive = isActive;
            }
        }

        ReapplyIfGroupedOn(nameof(Person.IsActive));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} all {1:N0} rows", isActive ? "Checked Active on" : "Unchecked Active on", People.Count));
    }

    // <snippet>
    private void OnChangeDepartmentClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = person.Department; // snippet:skip
        var to = SampleShaping.Next(PersonData.Departments, person.Department);
        using (BeginBulkUpdate())
        {
            person.Department = to;
        }

        // GroupBy takes a delegate, not a property path, so the control cannot re-bucket the row
        // on PropertyChanged; re-apply the grouping when the grouped-on value changed.
        ReapplyIfGroupedOn(nameof(Person.Department));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, to));
    }
    // </snippet>

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
}

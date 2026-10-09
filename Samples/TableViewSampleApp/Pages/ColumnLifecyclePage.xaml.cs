// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
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

namespace TableViewSampleApp.Pages;

/// <summary>
/// The whole lifecycle of a column, driven through the observable TableView.Columns vector.
/// Show / hide uses TableViewColumn.Visibility, add / remove grows and shrinks the vector, and
/// reorder moves the same TableViewColumn instance to a new index, which is why a column keeps
/// its width, template and sort settings wherever it lands. The rows underneath can be grouped
/// at the same time, to show that column changes and group headers are independent.
/// </summary>
public sealed partial class ColumnLifecyclePage : SamplePageBase
{
    private const int RowCount = 40;
    private const string JoinDateHeader = "Join date";

    private readonly TableViewColumn[] _canonical;
    private int _nextPersonIndex = RowCount;   // PersonData row used by the next "Add a person"

    public ColumnLifecyclePage()
    {
        // <snippet>
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();

        // Index = the Tag of the matching visibility checkbox.
        _canonical = new TableViewColumn[] { ColAvatar, ColName, ColDepartment, ColRole, ColSalary, ColActive, ColShift };
        // </snippet>
        PopulateColumnPicker();
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
        TrackItems(People, OnPersonChanged);
    }

    public ObservableCollection<Person> People { get; } = PersonData.Take(RowCount);

    public TableViewSource Source { get; }

    // <snippet>
    // ---- Column visibility: TableViewColumn.Visibility --------------------------------------

    private void OnColumnToggle(object sender, RoutedEventArgs e)
    {
        // IsChecked="True" raises Checked during InitializeComponent, before _canonical is
        // assigned; the XAML already matches. Reset also writes IsChecked and reports itself.
        if (_canonical is null || IsBulkUpdating || sender is not CheckBox { Tag: string tag } box || !int.TryParse(tag, out var index))
        {
            return;
        }

        var column = _canonical[index];
        var visible = box.IsChecked == true;
        column.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} the {1} column", visible ? "Showed" : "Hid", column.Header));
    }

    // ---- Column order: move, add, remove in TableView.Columns ------------------------------

    private void OnMoveLeftClick(object sender, RoutedEventArgs e) => MoveSelectedColumn(-1);

    private void OnMoveRightClick(object sender, RoutedEventArgs e) => MoveSelectedColumn(+1);

    private void MoveSelectedColumn(int direction)
    {
        if (SelectedColumn() is not { } column)
        {
            SetLastAction("No column chosen.");
            return;
        }

        var from = PeopleTable.Columns.IndexOf(column);
        var to = from + direction;
        if (to < 0 || to >= PeopleTable.Columns.Count)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} is already the {1} column.", column.Header, direction < 0 ? "first" : "last"));
            return;
        }

        // Moving the entry in the observable Columns vector IS the reorder; the header band and
        // the realized rows follow. The instance is reused, so its width travels with it.
        PeopleTable.Columns.RemoveAt(from);
        PeopleTable.Columns.Insert(to, column);

        PopulateColumnPicker();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from position {1} to {2}", column.Header, from + 1, to + 1));
    }

    private void OnAddJoinDateColumnClick(object sender, RoutedEventArgs e)
    {
        if (FindColumn(JoinDateHeader) is not null)
        {
            SetLastAction("The Join date column is already there.");
            return;
        }

        // A template column added at run time through the same vector the text columns live in.
        // SortMemberPath makes it sortable by the date value behind the picker.
        var resources = Application.Current.Resources;
        PeopleTable.Columns.Add(new TableViewTemplateColumn
        {
            Header = JoinDateHeader,
            Width = new GridLength((double)resources["ColWidthDatePicker"]),
            CellTemplate = (DataTemplate)resources["JoinDatePickerTemplate"],
            SortMemberPath = nameof(Person.JoinDate),
            HeaderToolTip = "Date the person joined. Editable; sorts by the JoinDate value.",
        });

        PopulateColumnPicker();
        SetLastAction("Added the Join date column at the end");
    }

    private void OnRemoveJoinDateColumnClick(object sender, RoutedEventArgs e)
    {
        if (FindColumn(JoinDateHeader) is not { } joinDate)
        {
            SetLastAction("There is no Join date column to remove.");
            return;
        }

        PeopleTable.Columns.Remove(joinDate);
        PopulateColumnPicker();
        SetLastAction("Removed the Join date column");
    }

    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        for (var target = 0; target < _canonical.Length; target++)
        {
            var current = PeopleTable.Columns.IndexOf(_canonical[target]);
            if (current >= 0 && current != target)
            {
                PeopleTable.Columns.RemoveAt(current);
                PeopleTable.Columns.Insert(target, _canonical[target]);
            }
        }

        using (BeginBulkUpdate())
        {
            foreach (var box in new[] { VisAvatarCheckBox, VisNameCheckBox, VisDepartmentCheckBox, VisRoleCheckBox, VisSalaryCheckBox, VisActiveCheckBox, VisShiftCheckBox })
            {
                var column = _canonical[int.Parse((string)box.Tag, CultureInfo.InvariantCulture)];
                var visible = column != ColShift;
                column.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
                box.IsChecked = visible;
            }
        }

        PopulateColumnPicker();
        SetLastAction("Restored the starting column order and visibility");
    }
    // </snippet>

    private TableViewColumn? FindColumn(string header) =>
        PeopleTable.Columns.FirstOrDefault(c => string.Equals(c.Header?.ToString(), header, StringComparison.Ordinal));

    private TableViewColumn? SelectedColumn() =>
        ColumnPicker.SelectedItem is ComboBoxItem { Tag: string header } ? FindColumn(header) : null;

    private void PopulateColumnPicker()
    {
        var previous = (ColumnPicker.SelectedItem as ComboBoxItem)?.Tag as string;
        ColumnPicker.Items.Clear();
        for (var i = 0; i < PeopleTable.Columns.Count; i++)
        {
            var header = PeopleTable.Columns[i].Header?.ToString() ?? string.Empty;
            ColumnPicker.Items.Add(new ComboBoxItem { Content = string.Format(CultureInfo.CurrentCulture, "{0}. {1}", i + 1, header), Tag = header });
        }

        // Keep the reader's pick across a reorder.
        var index = 0;
        for (var i = 0; i < ColumnPicker.Items.Count; i++)
        {
            if (ColumnPicker.Items[i] is ComboBoxItem { Tag: string tag } && string.Equals(tag, previous, StringComparison.Ordinal))
            {
                index = i;
                break;
            }
        }

        ColumnPicker.SelectedIndex = ColumnPicker.Items.Count > 0 ? index : -1;
    }

    // ---- Actions: rows ----------------------------------------------------------------------

    // The key an action moves a row along: the applied group key, or Department when flat.
    private string ActionKey => IsGrouped ? AppliedGroupKey : nameof(Person.Department);

    private void OnMoveRowClick(object sender, RoutedEventArgs e)
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
                case nameof(Person.IsActive):
                    person.IsActive = !person.IsActive;
                    break;
                case nameof(Person.Role):
                    // Cycle through the roles already in the table, so the row joins an existing group.
                    person.Role = SampleShaping.Next(ExistingValues(p => p.Role), person.Role);
                    break;
                default:
                    person.Department = SampleShaping.Next(ExistingValues(p => p.Department), person.Department);
                    break;
            }
        }

        // GroupBy takes a delegate, not a property path, so the control cannot re-bucket the row
        // on PropertyChanged; re-apply the grouping when the grouped-on value changed.
        ReapplyIfGroupedOn(key);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, SampleShaping.KeyOf(person, key)));
    }

    private void OnAddPersonClick(object sender, RoutedEventArgs e)
    {
        var all = PersonData.All;
        var person = PersonData.Take(Math.Min(_nextPersonIndex + 1, all.Count))[^1];
        _nextPersonIndex = (_nextPersonIndex + 1) % all.Count;

        var key = ActionKey;
        if (People.Count > 0)
        {
            var first = People[0];
            switch (key)
            {
                case nameof(Person.IsActive):
                    person.IsActive = first.IsActive;
                    break;
                case nameof(Person.Role):
                    person.Role = first.Role;
                    break;
                default:
                    person.Department = first.Department;
                    break;
            }
        }

        People.Add(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} to {1}", person.FullName, SampleShaping.KeyOf(person, key)));
    }

    private void OnRemoveRowClick(object sender, RoutedEventArgs e)
    {
        if (People.Count == 0)
        {
            SetLastAction("No rows to remove.");
            return;
        }

        var person = People[0];
        People.RemoveAt(0);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0}; {1:N0} rows left", person.FullName, People.Count));
    }

    private void OnRestoreRowsClick(object sender, RoutedEventArgs e)
    {
        // Clear() raises Reset, which carries no old items: detach them first.
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        People.Clear();
        foreach (var person in PersonData.Take(RowCount))
        {
            People.Add(person);
        }

        _nextPersonIndex = RowCount;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored the original {0:N0} rows", People.Count));
    }

    private List<string> ExistingValues(Func<Person, string> value) =>
        People.Select(value).Distinct(StringComparer.Ordinal).OrderBy(v => v, StringComparer.CurrentCulture).ToList();

    // In-cell edits through the Active checkbox, the Shift picker and the Join date picker.
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
        }
    }
}

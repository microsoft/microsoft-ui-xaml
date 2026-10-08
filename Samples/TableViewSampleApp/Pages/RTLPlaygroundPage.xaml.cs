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
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Right-to-left layout: TableView.FlowDirection on a people table that mixes text and template
/// cells, with Arabic and Hebrew rows, editing, sorting, filtering, grouping and column moves, so
/// every right-to-left-sensitive adornment can be checked.
/// </summary>
public sealed partial class RTLPlaygroundPage : Page
{
    // Page-local seed (FIX-PLAN R6 exception): people with Arabic and Hebrew names and notes. The
    // first five are mixed into the table; "Add a person" adds the rest in turn.
    private static readonly (string First, string Last, string Department, string Role, string Office, string Notes)[] s_rtlPeople =
    {
        ("ليلى", "حداد", "Design", "Design Lead", "Dublin", "تقود فريق التصميم وتراجع ملاحظات المستخدمين كل أسبوع."),
        ("נועה", "כהן", "Engineering", "QA Engineer", "London", "מובילה את צוות הבדיקות ואחראית על נגישות המוצר."),
        ("يوسف", "الخطيب", "Engineering", "Senior Engineer", "Singapore", "يعمل على تحسين أداء تطبيق Contoso على الأجهزة المحمولة."),
        ("אורי", "לוי", "Operations", "Ops Manager", "Toronto", "מתחזק את מערכת הניטור ומתאם את סבב הכוננויות."),
        ("نور", "منصور", "Marketing", "Brand Manager", "London", "تنسق مواعيد الإطلاق مع فرق التسويق والمبيعات."),
        ("תמר", "אברהם", "Product", "Product Manager", "Seattle", "מתאמת בין צוותי העיצוב והפיתוח לקראת כל גרסה."),
        ("عمر", "صالح", "Finance", "Financial Analyst", "Dublin", "يعد تقارير الميزانية الربعية ويتابع النفقات."),
        ("מיכל", "פרץ", "HR", "Recruiter", "Redmond", "אחראית על גיוס מהנדסים ועל תהליך הקליטה."),
    };

    // First names the "Rename selected person" action cycles through.
    private static readonly string[] s_renames = ["سارة", "רוני", "كريم", "דניאל"];

    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private TableViewColumn[] _originalColumnOrder = Array.Empty<TableViewColumn>();
    private int _nextRtlPerson;
    private int _nextRename;

    public RTLPlaygroundPage()
    {
        People = SeedPeople();
        _source = TableViewSource.From(People);
        InitializeComponent();
        _originalColumnOrder = PeopleTable.Columns.ToArray();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        RefreshReadouts();
    }

    public ObservableCollection<Person> People { get; }

    public TableViewSource? Source => _source;

    // PersonData.Take(40) with the first five Arabic / Hebrew people mixed into the top rows.
    private ObservableCollection<Person> SeedPeople()
    {
        var people = PersonData.Take(40);
        for (var i = 0; i < 5; i++)
        {
            people.Insert(i * 2, NextRtlPerson(1041 + i));
        }

        return people;
    }

    private Person NextRtlPerson(int employeeId)
    {
        var seed = s_rtlPeople[_nextRtlPerson++ % s_rtlPeople.Length];
        return new Person
        {
            FirstName = seed.First,
            LastName = seed.Last,
            Email = string.Format(CultureInfo.CurrentCulture, "employee{0}@contoso.com", employeeId),
            Department = seed.Department,
            Role = seed.Role,
            Office = seed.Office,
            Bio = seed.Notes,
            EmployeeId = employeeId,
            IsActive = employeeId % 5 != 0,
            JoinDate = new DateTimeOffset(DateTimeOffset.Now.Date.AddDays(-97 * (employeeId % 13 + 1)), TimeSpan.Zero),
            ShiftStart = new TimeSpan(8 + employeeId % 3, 0, 0),
            Salary = 120_000 + 2_500 * (employeeId % 9),
        };
    }

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.PropertyChanged += OnPersonChanged;
        }

        PeopleTable.Sorted += OnTableSorted;
        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        PeopleTable.Sorted -= OnTableSorted;
    }

    // ---- Flow direction and filter ------------------------------------------------------

    private void OnRtlToggled(object sender, RoutedEventArgs e)
    {
        // Toggled fires during InitializeComponent (IsOn="True"); the XAML already sets RightToLeft.
        if (!IsLoaded || PeopleTable is null)
        {
            return;
        }

        PeopleTable.FlowDirection = RtlToggle.IsOn ? FlowDirection.RightToLeft : FlowDirection.LeftToRight;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "FlowDirection -> {0}", PeopleTable.FlowDirection));
    }

    private void OnFilterChanged(object sender, TextChangedEventArgs e)
    {
        if (_source is null || FilterBox is null)
        {
            return;
        }

        var term = FilterBox.Text.Trim();
        if (term.Length == 0)
        {
            _source.ClearFilter();
            SetLastAction("ClearFilter()");
        }
        else
        {
            // Filter reshapes the same source in place and composes with GroupBy and the sort.
            _source.Filter(item => item is Person person && Matches(person, term));
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Filter -> \"{0}\"", term));
        }
    }

    private static bool Matches(Person person, string term) =>
        person.FullName.Contains(term, StringComparison.CurrentCultureIgnoreCase)
        || person.Department.Contains(term, StringComparison.CurrentCultureIgnoreCase)
        || person.Office.Contains(term, StringComparison.CurrentCultureIgnoreCase)
        || person.Bio.Contains(term, StringComparison.CurrentCultureIgnoreCase);

    // ---- In-cell edits ------------------------------------------------------------------

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(Person.FirstName):
            case nameof(Person.LastName):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Name -> {0}", person.FullName));
                break;
            case nameof(Person.IsActive):
                ReapplyIfGroupedOn(e.PropertyName);
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Active -> {0} for {1}", person.IsActive ? "checked" : "unchecked", person.FullName));
                break;
            case nameof(Person.JoinDate):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Join date -> {0:d} for {1}", person.JoinDate, person.FullName));
                break;
        }
    }

    // ---- Actions ------------------------------------------------------------------------

    private void OnSortFirstNameClick(object sender, RoutedEventArgs e)
    {
        var direction = FirstNameColumn.SortDirection == TableViewSortDirection.Ascending
            ? TableViewSortDirection.Descending
            : TableViewSortDirection.Ascending;
        PeopleTable.SortByColumn(FirstNameColumn, direction);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "SortByColumn(First name, {0})", direction));
    }

    // Raised for header clicks and for SortByColumn alike.
    private void OnTableSorted(TableView sender, TableViewSortedEventArgs e)
    {
        SortText.Text = e.Column is null || e.Direction == TableViewSortDirection.None
            ? "None"
            : string.Format(CultureInfo.CurrentCulture, "{0} {1}", e.Column.Header, e.Direction);
    }

    private void OnRenameClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        // OnPersonChanged reports the new name.
        person.FirstName = s_renames[_nextRename++ % s_renames.Length];
    }

    private void OnAddPersonClick(object sender, RoutedEventArgs e)
    {
        var person = NextRtlPerson(1041 + People.Count);
        person.PropertyChanged += OnPersonChanged;
        People.Add(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} ({1})", person.FullName, person.Department));
    }

    private void OnMoveColumnEarlierClick(object sender, RoutedEventArgs e) => MoveColumn(DepartmentColumn, -1);

    private void OnMoveColumnLaterClick(object sender, RoutedEventArgs e) => MoveColumn(DepartmentColumn, +1);

    // TableView.Columns is the live column collection: moving an entry moves the column.
    private void MoveColumn(TableViewColumn column, int offset)
    {
        var columns = PeopleTable.Columns;
        var from = columns.IndexOf(column);
        var to = from + offset;
        if (from < 0 || to < 0 || to >= columns.Count)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} is already the {1} column", column.Header, offset < 0 ? "first" : "last"));
            return;
        }

        columns.RemoveAt(from);
        columns.Insert(to, column);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} to column {1} of {2}", column.Header, to + 1, columns.Count));
    }

    private void OnResetColumnOrderClick(object sender, RoutedEventArgs e)
    {
        var columns = PeopleTable.Columns;
        for (var target = 0; target < _originalColumnOrder.Length; target++)
        {
            var current = columns.IndexOf(_originalColumnOrder[target]);
            if (current >= 0 && current != target)
            {
                var column = columns[current];
                columns.RemoveAt(current);
                columns.Insert(target, column);
            }
        }

        SetLastAction("Reset the column order");
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || ShownText is null || FlowDirectionText is null || ColumnOrderText is null || PeopleTable is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(People.Count);
        var term = FilterBox?.Text.Trim() ?? string.Empty;
        var shown = term.Length == 0 ? People.Count : People.Count(p => Matches(p, term));
        ShownText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} of {1:N0}", shown, People.Count);
        FlowDirectionText.Text = PeopleTable.FlowDirection.ToString();
        ColumnOrderText.Text = PeopleTable.Columns.SequenceEqual(_originalColumnOrder)
            ? "Original"
            : string.Join(", ", PeopleTable.Columns.Select(c => c.Header));
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
            //     with the Filter and GroupBy stages rather than replacing them, and set
            //     _appliedMode only after the call returns.
            default:
                _source.ClearGroupBy();
                mode = "flat";
                break;
        }

        _appliedMode = mode;
        _appliedKey = key;

        // Re-applying GroupBy can drop the selection when the selected row changed group.
        SampleShaping.Reselect(PeopleTable, selected, People.Count * 2, RefreshReadouts);
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

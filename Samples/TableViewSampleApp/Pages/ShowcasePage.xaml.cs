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
using Microsoft.UI.Xaml.Navigation;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableViewSampleApp.Templates;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewSelectionMode = Microsoft.UI.Xaml.Controls.Tabular.TableViewSelectionMode;
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Showcase: one people directory that combines live-updating template cells, header sort,
/// column resize and visibility, selection modes, headers visibility and grouping over a single
/// TableViewSource that is reshaped in place.
/// </summary>
public sealed partial class ShowcasePage : SamplePageBase
{
    // Read by the Showcase tint converters, which XAML instantiates as page resources.
    public static bool Vibrant = true;

    private const double MaxSalary = 230_000;
    private const int LiveRowCount = 5;

    private readonly Random _liveRandom = new();
    private readonly Dictionary<CheckBox, TableViewColumn> _columnToggles = new();
    private readonly DispatcherTimer _liveTimer = new();
    private bool _isResorting;                 // suppresses the Sorted readout for code re-sorts
    private bool _ready;                       // false while InitializeComponent fires handlers

    public ShowcasePage()
    {
        // <snippet>
        Vibrant = true;
        FillPeople(100);
        Source = TableViewSource.From(People);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        // </snippet>

        _columnToggles[ShiftColumnCheckBox] = ShiftColumn;
        _columnToggles[OfficeColumnCheckBox] = OfficeColumn;
        _columnToggles[EmailColumnCheckBox] = EmailColumn;
        _columnToggles[RoleColumnCheckBox] = RoleColumn;
        _columnToggles[DetailsColumnCheckBox] = DetailsColumn;

        _liveTimer.Interval = TimeSpan.FromMilliseconds(UpdateIntervalSlider.Value);
        _liveTimer.Tick += OnLiveTimerTick;

        _ready = true;
        InitializeSample(Status, Shaping.Attach(PeopleTable, Source));
        TrackItems(People, OnPersonChanged);
        TrackLifetime(
            () => PersonCellTemplates.DetailsOpened += OnDetailsOpened,
            () => PersonCellTemplates.DetailsOpened -= OnDetailsOpened);
        TrackTimer(_liveTimer, () => LiveToggle.IsOn);
    }

    public ObservableCollection<Person> People { get; } = new();

    public TableViewSource Source { get; }

    // Upper bound of the display indexes (rows plus one header per group).
    private int LastDisplayIndex => People.Count + PersonData.Roles.Count + PersonData.Departments.Count;

    protected override void OnNavigatedFrom(NavigationEventArgs e)
    {
        _liveTimer.Stop();
        base.OnNavigatedFrom(e);
    }

    private void FillPeople(int count)
    {
        People.Clear();
        foreach (var person in PersonData.Take(count))
        {
            People.Add(person);
        }
    }

    // ---- Live updates -------------------------------------------------------------------

    private void OnLiveToggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        if (LiveToggle.IsOn)
        {
            _liveTimer.Start();
        }
        else
        {
            _liveTimer.Stop();
        }

        SetLastAction(LiveToggle.IsOn ? "Live updates -> On" : "Live updates -> Off");
    }

    private void OnUpdateIntervalChanged(object sender, Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        _liveTimer.Interval = TimeSpan.FromMilliseconds(e.NewValue);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Update interval -> {0:N0} ms", e.NewValue));
    }

    // <snippet>
    private void OnLiveTimerTick(object? sender, object e)
    {
        var flipped = false;
        using (BeginBulkUpdate())
        {
            foreach (var person in People.Take(LiveRowCount))
            {
                // Nudge Salary within the dataset band and flip Active now and then. Person raises
                // PropertyChanged, so only these cells re-run their bindings and tint converters.
                var next = Math.Clamp(person.Salary + ((_liveRandom.NextDouble() - 0.5) * 12_000), 110_000, MaxSalary);
                person.Salary = Math.Round(next / 100.0) * 100.0;
                if (_liveRandom.NextDouble() < 0.15)
                {
                    person.IsActive = !person.IsActive;
                    flipped = true;
                }
            }
        }

        // Neither grouping nor sorting observes PropertyChanged, so re-apply whichever one is
        // keyed on a value the tick just changed.
        if (flipped)
        {
            ReapplyIfGroupedOn(nameof(Person.IsActive));
        }

        ResortIfSortedOn(nameof(Person.Salary), nameof(Person.IsActive));
        RefreshReadouts();
    }

    private void OnVibrantToggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        Vibrant = VibrantToggle.IsOn;

        // The tint converters read Vibrant, which no binding observes. Re-assigning the
        // CellTemplate regenerates the realized cells of just the two tinted columns.
        foreach (var column in new[] { DepartmentColumn, SalaryColumn })
        {
            var template = column.CellTemplate;
            column.CellTemplate = null;
            column.CellTemplate = template;
        }

        SetLastAction(Vibrant ? "Vibrant cells -> On" : "Vibrant cells -> Off");
    }
    // </snippet>

    // ---- Table options --------------------------------------------------------------------

    private void OnRowCountChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        var count = int.Parse(SampleShaping.SelectedTag(RowCountSelector, "100"), NumberStyles.Integer, CultureInfo.InvariantCulture);
        var stopwatch = System.Diagnostics.Stopwatch.StartNew();

        // Grouped, every Add re-buckets the projection; refill flat and group once at the end.
        var grouped = IsGrouped;
        if (grouped)
        {
            Source.ClearGroupBy();
        }

        FillPeople(count);
        if (grouped)
        {
            Shaping.Apply(announce: false);
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Row count -> {0:N0} (refilled in {1:N0} ms)", count, stopwatch.ElapsedMilliseconds));
    }

    // <snippet>
    private void OnSelectionModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        var mode = Enum.Parse<TableViewSelectionMode>(SampleShaping.SelectedTag(SelectionModeSelector, "Single"));
        PeopleTable.SelectionMode = mode;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "SelectionMode -> {0}", mode));
    }

    private void OnHeadersVisibilityChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        var visibility = Enum.Parse<TableViewHeadersVisibility>(SampleShaping.SelectedTag(HeadersVisibilitySelector, "Column"));
        PeopleTable.HeadersVisibility = visibility;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "HeadersVisibility -> {0}", visibility));
    }

    private void OnColumnCheckBoxClick(object sender, RoutedEventArgs e)
    {
        if (sender is not CheckBox checkBox || !_columnToggles.TryGetValue(checkBox, out var column))
        {
            return;
        }

        var show = checkBox.IsChecked == true;
        column.Visibility = show ? Visibility.Visible : Visibility.Collapsed;
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} column -> {1}", column.Header, show ? "shown" : "hidden"));
    }
    // </snippet>

    // ---- Sorting ----------------------------------------------------------------------------

    private void OnPeopleTableSorted(TableView sender, TableViewSortedEventArgs args)
    {
        if (_isResorting)
        {
            return;
        }

        SetLastAction(args.Column is null || args.Direction == TableViewSortDirection.None
            ? "Sort cleared"
            : string.Format(CultureInfo.CurrentCulture, "Sorted by {0} ({1})", args.Column.Header, args.Direction));
    }

    // <snippet>
    // The control sorts once, when the sort is applied. Re-declaring the same path on the source
    // re-reads every key (TableViewSource.Sort replaces that axis in place), so a row whose sorted
    // value changed moves to its new position.
    private void ResortIfSortedOn(params string[] propertyNames)
    {
        var column = SampleShaping.ActiveSortColumn(PeopleTable);
        if (column is null || Array.IndexOf(propertyNames, column.SortMemberPath) < 0)
        {
            return;
        }

        _isResorting = true;
        try
        {
            Source.Sort(column.SortMemberPath, column.SortDirection);
            OnSortRedeclared();   // keeps a sort that ordered the groups doing so
        }
        finally
        {
            _isResorting = false;
        }

        // Sort raises a Reset, and the control keeps the selection on the same item.
        RefreshReadouts();
    }
    // </snippet>

    // ---- In-cell edits ----------------------------------------------------------------------

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || IsBulkUpdating)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(Person.IsActive):
            case nameof(Person.JoinDate):
            case nameof(Person.ShiftStart):
            case nameof(Person.Office):
                ReapplyIfGroupedOn(e.PropertyName);
                ResortIfSortedOn(e.PropertyName);
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Edited {0} for {1}", e.PropertyName, person.FullName));
                break;
        }
    }

    private void OnDetailsOpened(object? sender, Person person) =>
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Opened the details flyout for {0}", person.FullName));

    // ---- Actions ------------------------------------------------------------------------

    private void OnSelectFirstClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectionMode == TableViewSelectionMode.None)
        {
            SetLastAction("SelectionMode is None, so no row can be selected.");
            return;
        }

        // Select ignores group-header indexes, so walk forward to the first data row. Clear any
        // current selection first, so the walk starts even when a later row is selected.
        PeopleTable.DeselectAll();
        for (var i = 0; i <= LastDisplayIndex && PeopleTable.SelectedItem is null; i++)
        {
            PeopleTable.Select(i);
        }

        SetLastAction(PeopleTable.SelectedItem is Person person
            ? string.Format(CultureInfo.CurrentCulture, "Selected {0}", person.FullName)
            : "There is no row to select.");
    }

    private void OnClearSelectionClick(object sender, RoutedEventArgs e)
    {
        PeopleTable.DeselectAll();
        SetLastAction("Cleared the selection");
    }

    private void OnMoveSelectedGroupClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var key = IsGrouped ? AppliedGroupKey : nameof(Person.Department);
        var from = SampleShaping.KeyOf(person, key);
        using (BeginBulkUpdate())
        {
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
        }

        ReapplyIfGroupedOn(key);
        ResortIfSortedOn(key);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", person.FullName, from, SampleShaping.KeyOf(person, key)));
    }

    private void OnRaiseSalaryClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        var before = person.Salary;
        person.Salary = Math.Min(MaxSalary, Math.Round(before * 1.1 / 100.0) * 100.0);
        ResortIfSortedOn(nameof(Person.Salary));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Raised {0} from {1:C0} to {2:C0}", person.FullName, before, person.Salary));
    }

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            SetLastAction("No row selected.");
            return;
        }

        People.Remove(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0} ({1})", person.FullName, SampleShaping.KeyOf(person, AppliedGroupKey)));
    }

    private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        RefreshReadouts();
    }
}

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
public sealed partial class ShowcasePage : Page
{
    // Read by the Showcase tint converters, which XAML instantiates as page resources.
    public static bool Vibrant = true;

    private const double MaxSalary = 230_000;
    private const int LiveRowCount = 5;

    private readonly Random _liveRandom = new();
    private readonly Dictionary<CheckBox, TableViewColumn> _columnToggles = new();
    private DispatcherTimer? _liveTimer;
    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Department";
    private bool _isBulkUpdate;                // suppresses per-row PropertyChanged handling
    private bool _isResorting;                 // suppresses the Sorted readout for code re-sorts
    private bool _ready;                       // false while InitializeComponent fires handlers

    public ShowcasePage()
    {
        Vibrant = true;
        FillPeople(100);
        _source = TableViewSource.From(People);
        InitializeComponent();

        _columnToggles[ShiftColumnCheckBox] = ShiftColumn;
        _columnToggles[OfficeColumnCheckBox] = OfficeColumn;
        _columnToggles[EmailColumnCheckBox] = EmailColumn;
        _columnToggles[RoleColumnCheckBox] = RoleColumn;
        _columnToggles[DetailsColumnCheckBox] = DetailsColumn;

        _ready = true;
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        RefreshReadouts();
    }

    public ObservableCollection<Person> People { get; } = new();

    public TableViewSource? Source => _source;

    private int MaxProbeIndex => People.Count + PersonData.Roles.Count + PersonData.Departments.Count;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        Track(People, attach: true);
        PersonCellTemplates.DetailsOpened += OnDetailsOpened;
        if (LiveToggle.IsOn)
        {
            StartLiveUpdates();
        }

        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        StopLiveUpdates();
        Track(People, attach: false);
        PersonCellTemplates.DetailsOpened -= OnDetailsOpened;
    }

    protected override void OnNavigatedFrom(NavigationEventArgs e)
    {
        StopLiveUpdates();
        base.OnNavigatedFrom(e);
    }

    private void Track(IEnumerable<Person> people, bool attach)
    {
        foreach (var person in people)
        {
            person.PropertyChanged -= OnPersonChanged;
            if (attach)
            {
                person.PropertyChanged += OnPersonChanged;
            }
        }
    }

    private void FillPeople(int count)
    {
        Track(People, attach: false);
        People.Clear();
        foreach (var person in PersonData.Take(count))
        {
            People.Add(person);
        }

        if (IsLoaded)
        {
            Track(People, attach: true);
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
            StartLiveUpdates();
        }
        else
        {
            StopLiveUpdates();
        }

        SetLastAction(LiveToggle.IsOn ? "Live updates -> On" : "Live updates -> Off");
    }

    private void OnUpdateIntervalChanged(object sender, Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        if (_liveTimer is not null)
        {
            _liveTimer.Interval = TimeSpan.FromMilliseconds(e.NewValue);
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Update interval -> {0:N0} ms", e.NewValue));
    }

    private void StartLiveUpdates()
    {
        StopLiveUpdates();
        _liveTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(UpdateIntervalSlider.Value) };
        _liveTimer.Tick += OnLiveTimerTick;
        _liveTimer.Start();
    }

    private void StopLiveUpdates()
    {
        if (_liveTimer is null)
        {
            return;
        }

        _liveTimer.Stop();
        _liveTimer.Tick -= OnLiveTimerTick;
        _liveTimer = null;
    }

    private void OnLiveTimerTick(object? sender, object e)
    {
        var flipped = false;
        _isBulkUpdate = true;
        try
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
        finally
        {
            _isBulkUpdate = false;
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
        var grouped = _appliedMode == "grouped";
        if (grouped)
        {
            _source?.ClearGroupBy();
        }

        FillPeople(count);
        if (grouped)
        {
            ApplyShaping(announce: false);
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Row count -> {0:N0} (refilled in {1:N0} ms)", count, stopwatch.ElapsedMilliseconds));
    }

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

    private TableViewColumn? ActiveSortColumn() =>
        PeopleTable.Columns.FirstOrDefault(column => column.SortDirection != TableViewSortDirection.None);

    // The control sorts once, when the sort is applied. Re-declaring the same path on the source
    // re-reads every key (TableViewSource.Sort replaces that axis in place), so a row whose sorted
    // value changed moves to its new position.
    private void ResortIfSortedOn(params string[] propertyNames)
    {
        var column = ActiveSortColumn();
        if (_source is null || column is null || Array.IndexOf(propertyNames, column.SortMemberPath) < 0)
        {
            return;
        }

        var selected = PeopleTable.SelectedItem;
        _isResorting = true;
        try
        {
            _source.Sort(column.SortMemberPath, column.SortDirection);
        }
        finally
        {
            _isResorting = false;
        }

        SampleShaping.Reselect(PeopleTable, selected, MaxProbeIndex, RefreshReadouts);
    }

    // ---- In-cell edits ----------------------------------------------------------------------

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not Person person || _isBulkUpdate)
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

        // Select ignores group-header indexes, so walk forward to the first data row.
        for (var i = 0; i <= MaxProbeIndex && PeopleTable.SelectedItem is null; i++)
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

        var key = _appliedMode == "grouped" ? _appliedKey : nameof(Person.Department);
        var from = SampleShaping.KeyOf(person, key);
        _isBulkUpdate = true;
        try
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
        finally
        {
            _isBulkUpdate = false;
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

        person.PropertyChanged -= OnPersonChanged;
        People.Remove(person);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0} ({1})", person.FullName, SampleShaping.KeyOf(person, _appliedKey)));
    }

    // ---- Readouts ---------------------------------------------------------------------------

    private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (!SampleShaping.IsReselecting)
        {
            RefreshReadouts();
        }
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || SelectionText is null || SortText is null || ColumnsText is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(People.Count);
        SelectionText.Text = PeopleTable.SelectedItem is Person person
            ? string.Format(CultureInfo.CurrentCulture, "{0} ({1})", person.FullName, PeopleTable.SelectionMode)
            : string.Format(CultureInfo.CurrentCulture, "(none) ({0})", PeopleTable.SelectionMode);

        var sortColumn = ActiveSortColumn();
        SortText.Text = sortColumn is null
            ? "(none)"
            : string.Format(CultureInfo.CurrentCulture, "{0} ({1})", sortColumn.Header, sortColumn.SortDirection);

        var shown = PeopleTable.Columns.Count(column => column.Visibility == Visibility.Visible);
        ColumnsText.Text = string.Format(CultureInfo.CurrentCulture, "{0} of {1} shown", shown, PeopleTable.Columns.Count);
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

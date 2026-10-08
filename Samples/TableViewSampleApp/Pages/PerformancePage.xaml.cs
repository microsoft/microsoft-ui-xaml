// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Controls;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Performance: timed runs of the common TableView workloads. Data is generated on a background
/// thread before the clock starts; the clock covers the call plus one synchronous layout pass.
/// The load runs assign a pre-built collection once (the pattern to use); the per-item Add run
/// shows the pattern to avoid.
/// </summary>
public sealed partial class PerformancePage : SamplePageBase
{
    private const int InitialRows = 1_000;
    private const int PerItemCap = 10_000;
    private const int GroupedAddBudgetMs = 2_000;   // per-row Adds under GroupBy stop here (see RunPerItemAddGroupedAsync)
    private const string FilterDepartment = "Engineering";

    private ObservableCollection<Person> _people;
    private TableViewSource _source;           // one per loaded dataset; reshaped in place
    private TableViewTemplateColumn? _photoColumn;
    private TableViewTemplateColumn? _departmentChipColumn;
    private bool _templateColumns;
    private bool _sortDescending = true;       // the first Sort click sorts ascending
    private bool _isRunning;

    public PerformancePage()
    {
        _people = new ObservableCollection<Person>(PersonData.Many(InitialRows));
        _source = TableViewSource.From(_people);
        InitializeComponent();
        PerfTable.ItemsSource = _source;
        CaptureBaseline();

        // No Reselect: no action on this page edits a group key, and a plain reshape keeps the
        // selection on its item. Probing 100,000 indexes would also defeat the measurement.
        Shaping.RestoreSelection = false;
        Shaping.TimeCall = Timed;   // GroupBy, ClearGroupBy, Expand all and Collapse all are timed like every run
        InitializeSample(Status, Shaping.Attach(PerfTable, _source));
    }

    // ---- Column set ---------------------------------------------------------------------

    private void OnColumnSetChanged(object sender, SelectionChangedEventArgs e)
    {
        // Fires during InitializeComponent (SelectedIndex="0"); the XAML already shows text columns.
        if (PerfTable is null || !IsLoaded)
        {
            return;
        }

        var useTemplates = SampleShaping.SelectedTag(ColumnSetSelector, "text") == "template";
        if (useTemplates == _templateColumns)
        {
            return;
        }

        var resources = Application.Current.Resources;
        _photoColumn ??= new TableViewTemplateColumn
        {
            Header = "Photo",
            Width = new GridLength((double)resources["ColWidthAvatar"]),
            CellTemplate = (DataTemplate)resources["AvatarTemplate"],
            CanSort = false,
        };
        _departmentChipColumn ??= new TableViewTemplateColumn
        {
            Header = "Department",
            Width = new GridLength((double)resources["ColWidthChip"]),
            CellTemplate = (DataTemplate)resources["DepartmentChipTemplate"],
            SortMemberPath = nameof(Person.Department),
        };

        var columns = PerfTable.Columns;
        if (useTemplates)
        {
            var departmentIndex = columns.IndexOf(DepartmentTextColumn);
            columns[departmentIndex] = _departmentChipColumn;
            columns.Insert(columns.IndexOf(IdColumn) + 1, _photoColumn);
        }
        else
        {
            columns.Remove(_photoColumn);
            columns[columns.IndexOf(_departmentChipColumn)] = DepartmentTextColumn;
        }

        _templateColumns = useTemplates;
        SetLastAction(useTemplates
            ? "Column set -> Template columns (Photo and Department chip)"
            : "Column set -> Text columns");
    }

    private string ColumnSetLabel => _templateColumns ? "template columns" : "text columns";

    // <snippet>
    // ---- Timed runs ---------------------------------------------------------------------

    // Settles the heap, then times the mutation plus one synchronous measure and arrange, so the
    // window ends when the new rows are laid out rather than when the C# call returns.
    private long Timed(Action mutation)
    {
        SettleHeap();

        var stopwatch = Stopwatch.StartNew();
        mutation();
        PerfTable.UpdateLayout();
        stopwatch.Stop();
        return stopwatch.ElapsedMilliseconds;
    }

    private static void SettleHeap()
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
    }
    // </snippet>

    private bool BeginRun()
    {
        if (_isRunning)
        {
            SetLastAction("A run is already in progress.");
            return false;
        }

        _isRunning = true;
        return true;
    }

    private async void OnLoad10kClick(object sender, RoutedEventArgs e) => await LoadAsync(10_000, Load10kText);

    private async void OnLoad100kClick(object sender, RoutedEventArgs e) => await LoadAsync(100_000, Load100kText);

    // <snippet>
    private async Task LoadAsync(int count, TextBlock result)
    {
        if (!BeginRun())
        {
            return;
        }

        try
        {
            // DO: build the rows off the UI thread and off the clock, then bind them once.
            var rows = await Task.Run(() => PersonData.Many(count));
            var ms = Timed(() =>
            {
                _people = new ObservableCollection<Person>(rows);
                _source = TableViewSource.From(_people);
                PerfTable.ItemsSource = _source;
            });

            _sortDescending = true;
            result.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} ms, {1}", ms, ColumnSetLabel);

            // A new dataset gets the current shaping; GroupBy is timed separately (Group row).
            Shaping.Attach(PerfTable, _source);
            if (IsGrouped)
            {
                Shaping.Apply(announce: false);
            }

            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Loaded {0:N0} rows in one assignment: {1:N0} ms ({2})", count, ms, ColumnSetLabel));
        }
        finally
        {
            _isRunning = false;
        }
    }
    // </snippet>

    private void OnSortClick(object sender, RoutedEventArgs e)
    {
        if (!BeginRun())
        {
            return;
        }

        try
        {
            var direction = _sortDescending ? SortDirection.Ascending : SortDirection.Descending;
            var ms = Timed(() => PerfTable.SortByColumn(NameColumn, direction));
            _sortDescending = direction == SortDirection.Descending;
            var label = direction == SortDirection.Ascending ? "ascending" : "descending";
            SortText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} ms, {1:N0} rows {2}", ms, _people.Count, label);
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Sorted {0:N0} rows by Name ({1}): {2:N0} ms", _people.Count, label, ms));
        }
        finally
        {
            _isRunning = false;
        }
    }

    private void OnFilterClick(object sender, RoutedEventArgs e)
    {
        if (!BeginRun())
        {
            return;
        }

        try
        {
            var source = _source;
            var ms = Timed(() => source.Filter(item => item is Person { Department: FilterDepartment }));
            var matches = _people.Count(p => p.Department == FilterDepartment);   // off the clock
            FilterText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} ms, {1:N0} of {2:N0} rows match", ms, matches, _people.Count);
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Filtered to {0}: {1:N0} of {2:N0} rows in {3:N0} ms", FilterDepartment, matches, _people.Count, ms));
        }
        finally
        {
            _isRunning = false;
        }
    }

    private void OnClearFilterClick(object sender, RoutedEventArgs e)
    {
        if (!BeginRun())
        {
            return;
        }

        try
        {
            var source = _source;
            var ms = Timed(() => source.ClearFilter());
            FilterText.Text = string.Format(CultureInfo.CurrentCulture, "Cleared in {0:N0} ms, {1:N0} rows", ms, _people.Count);
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Cleared the filter: {0:N0} rows in {1:N0} ms", _people.Count, ms));
        }
        finally
        {
            _isRunning = false;
        }
    }

    // <snippet>
    private async void OnPerItemAddClick(object sender, RoutedEventArgs e)
    {
        if (!BeginRun())
        {
            return;
        }

        try
        {
            var rows = await Task.Run(() => PersonData.Many(PerItemCap));
            if (IsGrouped)
            {
                await RunPerItemAddGroupedAsync(rows);
                return;
            }

            var people = _people;
            // DON'T: one CollectionChanged notification, and one projection update, per row.
            var ms = Timed(() =>
            {
                people.Clear();
                foreach (var person in rows)
                {
                    people.Add(person);
                }
            });

            _sortDescending = true;
            PerItemAddText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} ms for {1:N0} Add calls, {2}", ms, PerItemCap, ColumnSetLabel);
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0:N0} rows one at a time: {1:N0} ms. Compare with Load 10,000.", PerItemCap, ms));
        }
        finally
        {
            _isRunning = false;
        }
    }

    // Under an active GroupBy every Add regroups every row added so far, synchronously, so 10,000
    // Adds cost O(n^2) and would freeze the window for minutes. The grouped run therefore does
    // two things: (1) it times per-row Adds under the live grouping only until a time cap, and
    // (2) it does the real 10,000 Adds with the grouping detached, then groups once.
    private async Task RunPerItemAddGroupedAsync(IReadOnlyList<Person> rows)
    {
        var key = Shaping.SelectedKey;
        var keyLabel = Shaping.SelectedKeyLabel;
        var source = _source;
        var people = _people;
        Shaping.SetBusy(true);
        try
        {
            // (1) DON'T: per-row Add while grouped, stopped at GroupedAddBudgetMs.
            PerItemAddText.Text = "Adding under GroupBy (capped)…";
            await Task.Delay(50);
            int added = 0;
            int tailStartIndex = 0;
            long tailStartMs = 0;
            long tailMs = 0;
            var probeMs = Timed(() =>
            {
                people.Clear();
                var stopwatch = Stopwatch.StartNew();
                foreach (var person in rows)
                {
                    if (stopwatch.ElapsedMilliseconds >= GroupedAddBudgetMs)
                    {
                        break;
                    }

                    if (added % 100 == 0)
                    {
                        tailStartIndex = added;
                        tailStartMs = stopwatch.ElapsedMilliseconds;
                    }

                    people.Add(person);
                    added++;
                }

                tailMs = stopwatch.ElapsedMilliseconds - tailStartMs;
            });
            var perAddMs = (double)tailMs / Math.Max(1, added - tailStartIndex);

            // Let the window repaint between the two blocking steps.
            PerItemAddText.Text = "Adding with the grouping detached…";
            await Task.Delay(50);

            // (2) DO: detach the grouping, add per row against the flat source, group once.
            long addMs = 0;
            var regroupMs = 0L;
            var totalMs = Timed(() =>
            {
                var stopwatch = Stopwatch.StartNew();
                source.ClearGroupBy();
                people.Clear();
                foreach (var person in rows)
                {
                    people.Add(person);
                }

                addMs = stopwatch.ElapsedMilliseconds;
                source.GroupBy(item => SampleShaping.KeyOf(item as Person, key), SampleShaping.GroupIdentity);
                regroupMs = stopwatch.ElapsedMilliseconds - addMs;
            });

            _sortDescending = true;
            PerItemAddText.Text = string.Format(
                CultureInfo.CurrentCulture,
                "Grouped by {0}: {1:N0} of {2:N0} Adds before the {3:N0} ms cap ({4:N0} ms, last Adds {5:N1} ms each). Grouping detached: {2:N0} Adds plus one GroupBy in {6:N0} ms ({7:N0} ms Adds, {8:N0} ms GroupBy), {9}",
                keyLabel, added, PerItemCap, GroupedAddBudgetMs, probeMs, perAddMs, totalMs, addMs, regroupMs, ColumnSetLabel);
            SetLastAction(string.Format(
                CultureInfo.CurrentCulture,
                "Under GroupBy only {0:N0} of {1:N0} one-at-a-time Adds fit in {2:N0} ms. With the grouping detached and applied once, all {1:N0} took {3:N0} ms.",
                added, PerItemCap, GroupedAddBudgetMs, totalMs));
        }
        finally
        {
            Shaping.SetBusy(false);
        }
    }

    // ---- Shaping is timed like every other run (Shaping.TimeCall = Timed) ------------------

    protected override void OnShapingApplied(ShapingAppliedEventArgs e)
    {
        var ms = e.ElapsedMilliseconds;
        GroupText.Text = e.IsGrouped
            ? string.Format(CultureInfo.CurrentCulture, "GroupBy {0}: {1:N0} ms, {2:N0} rows", e.KeyLabel, ms, _people.Count)
            : string.Format(CultureInfo.CurrentCulture, "ClearGroupBy: {0:N0} ms, {1:N0} rows", ms, _people.Count);
        e.Message += string.Format(CultureInfo.CurrentCulture, " in {0:N0} ms", ms);
    }

    protected override void OnShapingAction(ShapingActionEventArgs e) =>
        e.Message += string.Format(CultureInfo.CurrentCulture, " in {0:N0} ms", e.ElapsedMilliseconds);
    // </snippet>

    // ---- Memory -------------------------------------------------------------------------

    private void CaptureBaseline()
    {
        SettleHeap();
        _baselineWorkingSet = Process.GetCurrentProcess().WorkingSet64;
        _baselineManagedHeap = GC.GetTotalMemory(forceFullCollection: true);
    }

    private void OnSnapshotClick(object sender, RoutedEventArgs e)
    {
        SettleHeap();
        // The managed heap is exact after a full collection; the working set is the OS view and
        // also covers native growth such as XAML element trees, but it can lag.
        var workingSet = Process.GetCurrentProcess().WorkingSet64;
        var heap = GC.GetTotalMemory(forceFullCollection: true);
        ShowMemory(workingSet, heap);
        SetLastAction("Took a memory snapshot (deltas from the baseline)");
    }

    private void OnRebaselineClick(object sender, RoutedEventArgs e)
    {
        CaptureBaseline();
        MemoryText.Text = "Baseline reset; take a snapshot to see growth from here.";
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Reset the memory baseline at {0:T}", DateTime.Now));
    }
}

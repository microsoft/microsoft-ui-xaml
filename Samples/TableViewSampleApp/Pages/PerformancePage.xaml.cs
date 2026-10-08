// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
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
public sealed partial class PerformancePage : Page
{
    private const int InitialRows = 1_000;
    private const int PerItemCap = 10_000;
    private const string FilterDepartment = "Engineering";

    private ObservableCollection<Person> _people;
    private TableViewSource? _source;          // one per loaded dataset; reshaped in place
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private TableViewTemplateColumn? _photoColumn;
    private TableViewTemplateColumn? _departmentChipColumn;
    private bool _templateColumns;
    private bool _sortDescending = true;       // the first Sort click sorts ascending
    private bool _isRunning;
    private long _baselineWorkingSet;
    private long _baselineManagedHeap;

    public PerformancePage()
    {
        _people = new ObservableCollection<Person>(PersonData.Many(InitialRows));
        _source = TableViewSource.From(_people);
        InitializeComponent();
        PerfTable.ItemsSource = _source;
        BuildText.Text = string.Format(CultureInfo.CurrentCulture, "{0}, {1}", IsDebugBuild ? "Debug" : "Release", RuntimeInformation.ProcessArchitecture);
        CaptureBaseline();
        RefreshReadouts();
    }

    private static bool IsDebugBuild
    {
        get
        {
#if DEBUG
            return true;
#else
            return false;
#endif
        }
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

    // ---- Timed runs ---------------------------------------------------------------------

    // Settles the heap, then times the mutation plus one synchronous measure and arrange, so the
    // window ends when the new rows are laid out rather than when the C# call returns.
    private long Timed(Action mutation)
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();

        var stopwatch = Stopwatch.StartNew();
        mutation();
        PerfTable.UpdateLayout();
        stopwatch.Stop();
        return stopwatch.ElapsedMilliseconds;
    }

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
            if (_appliedMode == "grouped")
            {
                ApplyShaping(announce: false);
            }

            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Loaded {0:N0} rows in one assignment: {1:N0} ms ({2})", count, ms, ColumnSetLabel));
        }
        finally
        {
            _isRunning = false;
        }
    }

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
        if (_source is null || !BeginRun())
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
        if (_source is null || !BeginRun())
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

    private async void OnPerItemAddClick(object sender, RoutedEventArgs e)
    {
        if (!BeginRun())
        {
            return;
        }

        try
        {
            var rows = await Task.Run(() => PersonData.Many(PerItemCap));
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

    // ---- Memory -------------------------------------------------------------------------

    private void CaptureBaseline()
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        _baselineWorkingSet = Process.GetCurrentProcess().WorkingSet64;
        _baselineManagedHeap = GC.GetTotalMemory(forceFullCollection: true);
    }

    private void OnSnapshotClick(object sender, RoutedEventArgs e)
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        // The managed heap is exact after a full collection; the working set is the OS view and
        // also covers native growth such as XAML element trees, but it can lag.
        var workingSet = Process.GetCurrentProcess().WorkingSet64;
        var heap = GC.GetTotalMemory(forceFullCollection: true);
        const double MB = 1024 * 1024;
        MemoryText.Text = string.Format(
            CultureInfo.CurrentCulture,
            "Managed heap {0:N0} MB ({1:+0;-0;0} MB), working set {2:N0} MB ({3:+0;-0;0} MB)",
            heap / MB,
            (heap - _baselineManagedHeap) / MB,
            workingSet / MB,
            (workingSet - _baselineWorkingSet) / MB);
        SetLastAction("Took a memory snapshot (deltas from the baseline)");
    }

    private void OnRebaselineClick(object sender, RoutedEventArgs e)
    {
        CaptureBaseline();
        MemoryText.Text = "Baseline reset; take a snapshot to see growth from here.";
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Reset the memory baseline at {0:T}", DateTime.Now));
    }

    private void RefreshReadouts()
    {
        if (RowsText is null)
        {
            return;
        }

        RowsText.Text = SampleShaping.RowCountText(_people.Count);
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent, before the later-declared elements exist.
        if (_source is null || PerfTable is null || ShapingModeSelector is null || GroupKeySelector is null
            || ExpandAllButton is null || CollapseAllButton is null || ShapingModeText is null || GroupText is null)
        {
            return;
        }

        var mode = SampleShaping.SelectedTag(ShapingModeSelector, "flat");
        var key = SampleShaping.SelectedTag(GroupKeySelector, "Department");
        var source = _source;
        long ms;

        switch (mode)
        {
            case "grouped":
                // The key selector receives the ROW; the identity selector receives the KEY.
                ms = Timed(() => source.GroupBy(item => SampleShaping.KeyOf(item as Person, key), SampleShaping.GroupIdentity));
                break;
            // case "hierarchy":
            // case "groupedHierarchy":
            //     Hierarchical (tree) rows are not available in this release, so the two matching
            //     ComboBoxItems ship disabled. TableViewSource and TableView have no hierarchy
            //     member today. When hierarchy ships, apply it to this same source here, composed
            //     with the GroupBy stage above rather than replacing it, and set _appliedMode only
            //     after the call returns.
            default:
                ms = Timed(() => source.ClearGroupBy());
                mode = "flat";
                break;
        }

        _appliedMode = mode;

        // No Reselect here: no action on this page edits a group key, and a plain reshape keeps
        // the selection on its item. Probing 100,000 indexes would also defeat the measurement.
        UpdateShapingGating();
        GroupText.Text = mode == "grouped"
            ? string.Format(CultureInfo.CurrentCulture, "GroupBy {0}: {1:N0} ms, {2:N0} rows", SampleShaping.Label(GroupKeySelector), ms, _people.Count)
            : string.Format(CultureInfo.CurrentCulture, "ClearGroupBy: {0:N0} ms, {1:N0} rows", ms, _people.Count);
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0} in {1:N0} ms", SampleShaping.Label(GroupKeySelector), ms)
                : string.Format(CultureInfo.CurrentCulture, "Shaping -> Flat in {0:N0} ms", ms));
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
        var ms = Timed(() => PerfTable.ExpandAllGroups());
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Expanded all groups in {0:N0} ms", ms));
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        var ms = Timed(() => PerfTable.CollapseAllGroups());
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Collapsed all groups in {0:N0} ms", ms));
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

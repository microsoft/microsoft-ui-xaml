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
using SortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;

namespace TableViewSampleApp.Pages;

public sealed partial class TaskManagerPage : SamplePageBase
{
    private readonly DispatcherTimer _timer = new() { Interval = TimeSpan.FromSeconds(2) };
    private readonly Random _rng = new();
    private readonly List<ProcessItem> _stashedProcesses = new();
    private string? _searchText;
    private int _newTaskCount;
    private (TableViewColumn? Column, SortDirection Direction) _reportedSort;

    public TaskManagerPage()
    {
        // <snippet>
        RandomizeAll();
        Source = TableViewSource.From(Processes);  // created once; filtered, sorted and grouped in place
        InitializeComponent();

        ProcessTable.HeadersVisibility = TableViewHeadersVisibility.Column;
        ProcessTable.GridLinesVisibility = TableViewGridLinesVisibility.Horizontal;
        ProcessTable.Density = TableViewDensity.Compact;
        // Metrics open descending, as in Task Manager: the busiest process comes first.
        foreach (var column in new[] { CpuColumn, MemoryColumn, DiskColumn, NetworkColumn })
        {
            column.SortCycle = TableViewSortCycle.DescendingAscending;
        }

        _timer.Tick += OnTimerTick;
        // </snippet>

        UpdateTotals();
        InitializeSample(Status, Shaping.Attach(ProcessTable, Source, (row, key) => ProcessData.GroupKeyOf(row as ProcessItem, key)));
        TrackTimer(_timer, () => LiveUpdatesToggle.IsOn);
    }

    public ObservableCollection<ProcessItem> Processes { get; } = ProcessData.All();

    public TableViewSource Source { get; }

    // ---- Live data ----------------------------------------------------------------------

    private void OnLiveUpdatesToggled(object sender, RoutedEventArgs e)
    {
        // Toggled can fire while InitializeComponent applies IsOn; Loaded starts the timer then.
        if (!IsLoaded)
        {
            return;
        }

        if (LiveUpdatesToggle.IsOn)
        {
            _timer.Start();
            SetLastAction("Live updates -> resumed");
        }
        else
        {
            _timer.Stop();
            SetLastAction("Live updates -> paused");
        }
    }

    private void OnUpdateIntervalChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!IsLoaded)
        {
            return;
        }

        var seconds = int.Parse(SampleShaping.SelectedTag(UpdateIntervalSelector, "2"), CultureInfo.InvariantCulture);
        _timer.Interval = TimeSpan.FromSeconds(seconds);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Update interval -> {0}", SampleShaping.Label(UpdateIntervalSelector)));
    }

    // <snippet>
    private void OnTimerTick(object? sender, object e)
    {
        RandomizeAll();
        UpdateTotals(); // snippet:skip
        ResortIfMetricSorted();
    }

    // A value change raised through INotifyPropertyChanged updates the cell but does not move the
    // row: the projection re-sorts on collection changes only. Re-declaring the active sort with
    // the column's own path re-sorts the same source and keeps that column's sort indicator.
    private void ResortIfMetricSorted()
    {
        var column = SampleShaping.ActiveSortColumn(ProcessTable);
        if (column is null || column.SortMemberPath is not
            (nameof(ProcessItem.CpuPercent) or nameof(ProcessItem.MemoryMB) or nameof(ProcessItem.DiskMBps) or nameof(ProcessItem.NetworkMbps)))
        {
            return;
        }

        Source.Sort(column.SortMemberPath, column.SortDirection);
    }
    // </snippet>

    private void OnProcessTableSorted(TableView sender, TableViewSortedEventArgs args)
    {
        // The live re-sort raises Sorted with the same column and direction on every tick; report
        // only real changes.
        if (_reportedSort == (args.Column, args.Direction))
        {
            return;
        }

        _reportedSort = (args.Column, args.Direction);
        SetLastAction(args.Column is null || args.Direction == SortDirection.None
            ? "Sort cleared"
            : string.Format(CultureInfo.CurrentCulture, "Sorted by {0}, {1}", args.Column.Header, args.Direction == SortDirection.Descending ? "descending" : "ascending"));
    }

    // <snippet>
    // ---- Search (TableViewSource.Filter) ------------------------------------------------

    private void OnSearchBoxTextChanged(AutoSuggestBox sender, AutoSuggestBoxTextChangedEventArgs args)
    {
        if (args.Reason != AutoSuggestionBoxTextChangeReason.UserInput)
        {
            return;
        }

        var query = sender.Text?.Trim();
        _searchText = string.IsNullOrEmpty(query) ? null : query;
        ApplySearch();
        SetLastAction(_searchText is { } text
            ? string.Format(CultureInfo.CurrentCulture, "Search \"{0}\" -> {1:N0} of {2:N0} processes", text, VisibleCount(), Processes.Count)
            : "Search cleared");
    }

    // Filter evaluates the predicate when it reshapes, not when a row's value changes, so call this
    // again after a write to a searched property.
    private void ApplySearch()
    {
        if (_searchText is not { } text)
        {
            Source.ClearFilter();
            return;
        }

        Source.Filter(item => item is ProcessItem process && MatchesSearch(process, text));
    }

    private static bool MatchesSearch(ProcessItem process, string query) =>
        process.Name.Contains(query, StringComparison.CurrentCultureIgnoreCase)
        || process.Category.Contains(query, StringComparison.CurrentCultureIgnoreCase)
        || process.StatusText.Contains(query, StringComparison.CurrentCultureIgnoreCase);
    // </snippet>

    private int VisibleCount() =>
        _searchText is { } text ? Processes.Count(process => MatchesSearch(process, text)) : Processes.Count;

    // ---- Actions ------------------------------------------------------------------------

    private void OnRunNewTaskClick(object sender, RoutedEventArgs e)
    {
        var process = ProcessData.NewTask(_newTaskCount++);
        RandomizeValues(process);

        // The projection places the new row by the active sort and into its group.
        Processes.Add(process);
        UpdateTotals();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Started {0} in Apps", process.Name));
    }

    private void OnEndTaskClick(object sender, RoutedEventArgs e)
    {
        if (ProcessTable.SelectedItem is not ProcessItem process)
        {
            SetLastAction("No process selected.");
            return;
        }

        Processes.Remove(process);
        UpdateTotals();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Ended {0} ({1})", process.Name, process.Category));
    }

    private void OnSortByCpuClick(object sender, RoutedEventArgs e)
    {
        ProcessTable.SortByColumn(CpuColumn, SortDirection.Descending);
        SetLastAction("Sorted by CPU, highest first; each live tick re-sorts");
    }

    private void OnMoveCategoryClick(object sender, RoutedEventArgs e)
    {
        if (ProcessTable.SelectedItem is not ProcessItem process)
        {
            SetLastAction("No process selected.");
            return;
        }

        var from = process.Category;
        process.Category = SampleShaping.Next(ProcessData.Categories, from);

        // GroupBy takes a delegate, not a property path, so re-apply it to re-bucket the row. The
        // search matches on Category too, so re-apply it as well (it would keep showing the row).
        ReapplyIfGroupedOn(nameof(ProcessItem.Category));
        if (_searchText is not null)
        {
            ApplySearch();
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", process.Name, from, process.Category));
    }

    private void OnRemoveAllClick(object sender, RoutedEventArgs e)
    {
        if (_stashedProcesses.Count > 0)
        {
            foreach (var process in _stashedProcesses)
            {
                Processes.Add(process);
            }

            var restored = _stashedProcesses.Count;
            _stashedProcesses.Clear();
            RemoveAllButton.Content = "Remove all rows";
            UpdateTotals();
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored {0:N0} processes", restored));
            return;
        }

        _stashedProcesses.AddRange(Processes);
        Processes.Clear();
        RemoveAllButton.Content = "Restore all rows";
        UpdateTotals();
        SetLastAction(IsGrouped
            ? "Removed all rows; the grouped projection now has zero groups"
            : "Removed all rows");
    }

    private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        RefreshReadouts();
    }

    // ---- Simulation ---------------------------------------------------------------------

    private void RandomizeAll()
    {
        foreach (var process in Processes)
        {
            if (process.Children.Count > 0)
            {
                foreach (var child in process.Children)
                {
                    RandomizeValues(child);
                }

                AggregateFromChildren(process);
            }
            else
            {
                RandomizeValues(process);
            }
        }
    }

    private void RandomizeValues(ProcessItem process)
    {
        var memoryDrift = (_rng.NextDouble() - 0.5) * process.MemoryBaseline * 0.08;
        process.MemoryMB = Math.Max(0.5, Math.Round(process.MemoryBaseline + memoryDrift, 1));

        if (process.IsSuspended)
        {
            // A suspended process holds its memory but uses no CPU, disk or network.
            process.CpuPercent = 0;
            process.DiskMBps = 0;
            process.NetworkMbps = 0;
            return;
        }

        process.CpuPercent = Math.Round(process.IsHighCpu ? (_rng.NextDouble() * 15) + 5 : _rng.NextDouble() * 3, 1);
        process.DiskMBps = Math.Round(process.IsHighDisk ? _rng.NextDouble() * 2 : _rng.NextDouble() * 0.3, 1);
        process.NetworkMbps = Math.Round(process.IsHighNetwork ? _rng.NextDouble() * 1.5 : _rng.NextDouble() * 0.2, 1);
    }

    private static void AggregateFromChildren(ProcessItem parent)
    {
        parent.CpuPercent = Math.Round(parent.Children.Sum(child => child.CpuPercent), 1);
        parent.MemoryMB = Math.Round(parent.Children.Sum(child => child.MemoryMB), 1);
        parent.DiskMBps = Math.Round(parent.Children.Sum(child => child.DiskMBps), 1);
        parent.NetworkMbps = Math.Round(parent.Children.Sum(child => child.NetworkMbps), 1);
    }
}

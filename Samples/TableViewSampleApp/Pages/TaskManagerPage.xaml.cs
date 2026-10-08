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
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Converters;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using Windows.UI;
using SortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Task Manager scenario: template columns (icon + name, status, four heat-map metric cells),
/// live data from a DispatcherTimer that stays sorted, search through TableViewSource.Filter and
/// the Task Manager category bands through TableViewSource.GroupBy. The page follows the app
/// theme; the title-bar button owns it.
/// </summary>
public sealed partial class TaskManagerPage : Page
{
    private static readonly string[] s_categories = { "Apps", "Background processes", "Windows processes" };

    // Run new task cycles through real inbox apps.
    private static readonly (string Name, string Glyph, double Memory)[] s_newTasks =
    {
        ("Paint", "\uE790", 62.4),
        ("Snipping Tool", "\uE722", 38.9),
        ("Calculator", "\uE8EF", 21.7),
        ("Clock", "\uE823", 17.3),
        ("Photos", "\uEB9F", 148.2),
        ("Media Player", "\uE8D6", 74.6),
    };

    private const int SimulatedCores = 12;
    private const double SimulatedMemoryMB = 16 * 1024;

    private readonly DispatcherTimer _timer = new() { Interval = TimeSpan.FromSeconds(2) };
    private readonly Random _rng = new();
    private readonly List<ProcessItem> _stashedProcesses = new();
    private TableViewSource? _source;          // created ONCE; filtered, sorted and grouped in place
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Category";
    private string? _searchText;
    private int _newTaskCount;
    private (TableViewColumn? Column, SortDirection Direction) _reportedSort;

    public TaskManagerPage()
    {
        PopulateProcesses();
        _source = TableViewSource.From(Processes);
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
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        UpdateTotals();
        RefreshReadouts();
    }

    public ObservableCollection<ProcessItem> Processes { get; } = new();

    public TableViewSource? Source => _source;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        if (LiveUpdatesToggle.IsOn)
        {
            _timer.Start();
        }

        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e) => _timer.Stop();

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

    private void OnTimerTick(object? sender, object e)
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

        UpdateTotals();
        ResortIfMetricSorted();
    }

    // A value change raised through INotifyPropertyChanged updates the cell but does not move the
    // row: the projection re-sorts on collection changes only. Re-declaring the active sort with
    // the column's own path re-sorts the same source and keeps that column's sort indicator.
    private void ResortIfMetricSorted()
    {
        var column = ActiveSortColumn();
        if (_source is null || column is null || column.SortMemberPath is not
            (nameof(ProcessItem.CpuPercent) or nameof(ProcessItem.MemoryMB) or nameof(ProcessItem.DiskMBps) or nameof(ProcessItem.NetworkMbps)))
        {
            return;
        }

        _source.Sort(column.SortMemberPath, column.SortDirection);
    }

    private TableViewColumn? ActiveSortColumn() =>
        ProcessTable?.Columns.FirstOrDefault(column => column.SortDirection != SortDirection.None);

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

    // ---- Search (TableViewSource.Filter) ------------------------------------------------

    private void OnSearchBoxTextChanged(AutoSuggestBox sender, AutoSuggestBoxTextChangedEventArgs args)
    {
        if (_source is null || args.Reason != AutoSuggestionBoxTextChangeReason.UserInput)
        {
            return;
        }

        var query = sender.Text?.Trim();
        _searchText = string.IsNullOrEmpty(query) ? null : query;
        if (_searchText is null)
        {
            _source.ClearFilter();
            SetLastAction("Search cleared");
            return;
        }

        var text = _searchText;
        _source.Filter(item => item is ProcessItem process && MatchesSearch(process, text));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Search \"{0}\" -> {1:N0} of {2:N0} processes", text, VisibleCount(), Processes.Count));
    }

    private static bool MatchesSearch(ProcessItem process, string query) =>
        process.Name.Contains(query, StringComparison.CurrentCultureIgnoreCase)
        || process.Category.Contains(query, StringComparison.CurrentCultureIgnoreCase)
        || process.StatusText.Contains(query, StringComparison.CurrentCultureIgnoreCase);

    private int VisibleCount() =>
        _searchText is { } text ? Processes.Count(process => MatchesSearch(process, text)) : Processes.Count;

    // ---- Actions ------------------------------------------------------------------------

    private void OnRunNewTaskClick(object sender, RoutedEventArgs e)
    {
        var template = s_newTasks[_newTaskCount % s_newTasks.Length];
        var round = (_newTaskCount / s_newTasks.Length) + 1;
        _newTaskCount++;

        var process = new ProcessItem
        {
            Name = round == 1 ? template.Name : string.Format(CultureInfo.CurrentCulture, "{0} ({1})", template.Name, round),
            Category = "Apps",
            IconGlyph = template.Glyph,
            MemoryBaseline = template.Memory,
            StatusText = ProcessItem.Running,
        };
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
        process.Category = SampleShaping.Next(s_categories, from);

        // GroupBy takes a delegate, not a property path, so re-apply it to re-bucket the row.
        ReapplyIfGroupedOn(nameof(ProcessItem.Category));
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
        SetLastAction(_appliedMode == "grouped"
            ? "Removed all rows; the grouped projection now has zero groups"
            : "Removed all rows");
    }

    private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (SampleShaping.IsReselecting)
        {
            return;
        }

        RefreshReadouts();
    }

    // ---- Simulation ---------------------------------------------------------------------

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

    private void UpdateTotals()
    {
        if (TotalsText is null)
        {
            return;
        }

        // CpuPercent is per core, so overall utilization is the sum over the core count.
        TotalsText.Text = string.Format(
            CultureInfo.CurrentCulture,
            "CPU {0:N0}%, memory {1:N0}%, disk {2:N1} MB/s, network {3:N1} Mbps",
            Math.Min(Processes.Sum(p => p.CpuPercent) / SimulatedCores, 100),
            Math.Min(Processes.Sum(p => p.MemoryMB) / SimulatedMemoryMB * 100, 100),
            Processes.Sum(p => p.DiskMBps),
            Processes.Sum(p => p.NetworkMbps));
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || SelectedProcessText is null || LiveText is null || ProcessTable is null)
        {
            return;
        }

        var visible = VisibleCount();
        RowsText.Text = visible == Processes.Count
            ? SampleShaping.RowCountText(visible)
            : string.Format(CultureInfo.CurrentCulture, "{0:N0} of {1:N0} (search)", visible, Processes.Count);
        SelectedProcessText.Text = ProcessTable.SelectedItem is ProcessItem process ? process.Name : "(none)";
        if (EndTaskButton is not null)
        {
            EndTaskButton.IsEnabled = ProcessTable.SelectedItem is ProcessItem;
        }

        LiveText.Text = LiveUpdatesToggle?.IsOn == false
            ? "Paused"
            : string.Format(CultureInfo.CurrentCulture, "Every {0}", SampleShaping.Label(UpdateIntervalSelector).ToLower(CultureInfo.CurrentCulture));
    }

    // Group key resolution for this page's model (FIX-PLAN §1.6 R2). Never returns a blank key.
    private static object KeyOf(ProcessItem? process, string key)
    {
        var value = key == nameof(ProcessItem.StatusText) ? process?.StatusText : process?.Category;
        return string.IsNullOrWhiteSpace(value) ? SampleShaping.NoneKey : value;
    }

    private void PopulateProcesses()
    {
        // ── Apps ──
        Processes.Add(new ProcessItem
        {
            Name = "Calendar",
            Category = "Apps",
            IconGlyph = "\uE787",
            MemoryBaseline = 28.4,
            Children =
            [
                new() { Name = "Calendar", Category = "Apps", IconGlyph = "\uE787", MemoryBaseline = 10.7 },
                new() { Name = "Crashpad", Category = "Apps", IconGlyph = "\uE7BA", MemoryBaseline = 1.3 },
                new() { Name = "WebView2 GPU Process", Category = "Apps", IconGlyph = "\uE943", MemoryBaseline = 1.7 },
                new() { Name = "WebView2 Manager", Category = "Apps", IconGlyph = "\uE912", MemoryBaseline = 10.9 },
                new() { Name = "WebView2 Utility: Network Service", Category = "Apps", IconGlyph = "\uE968", MemoryBaseline = 2.4 },
                new() { Name = "WebView2 Utility: Storage Service", Category = "Apps", IconGlyph = "\uEDA2", MemoryBaseline = 1.4 },
                new() { Name = "WebView2: Calendar in Taskbar", Category = "Apps", IconGlyph = "\uE774", MemoryBaseline = 0.8, StatusText = ProcessItem.EfficiencyMode, IsEfficiency = true },
            ]
        });

        Processes.Add(new ProcessItem
        {
            Name = "Files",
            Category = "Apps",
            IconGlyph = "\uE8B7",
            MemoryBaseline = 34.6,
            Children =
            [
                new() { Name = "Crashpad", Category = "Apps", IconGlyph = "\uE7BA", MemoryBaseline = 1.5 },
                new() { Name = "Files", Category = "Apps", IconGlyph = "\uE8B7", MemoryBaseline = 13.0 },
                new() { Name = "WebView2 GPU Process", Category = "Apps", IconGlyph = "\uE943", MemoryBaseline = 1.7 },
                new() { Name = "WebView2 Manager", Category = "Apps", IconGlyph = "\uE912", MemoryBaseline = 12.7 },
                new() { Name = "WebView2 Utility: Network Service", Category = "Apps", IconGlyph = "\uE968", MemoryBaseline = 3.9 },
                new() { Name = "WebView2 Utility: Storage Service", Category = "Apps", IconGlyph = "\uEDA2", MemoryBaseline = 1.8 },
                new() { Name = "WebView2: Files Search App", Category = "Apps", IconGlyph = "\uE774", MemoryBaseline = 0.5, StatusText = ProcessItem.EfficiencyMode, IsEfficiency = true },
            ]
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft Edge",
            Category = "Apps",
            IconGlyph = "\uE774",
            MemoryBaseline = 2371.1,
            IsHighCpu = true,
            IsHighNetwork = true,
            Children =
            [
                new() { Name = "Browser", Category = "Apps", IconGlyph = "\uE774", MemoryBaseline = 320, IsHighCpu = true },
                new() { Name = "GPU Process", Category = "Apps", IconGlyph = "\uE943", MemoryBaseline = 198 },
                new() { Name = "Utility: Audio Service", Category = "Apps", IconGlyph = "\uE8D6", MemoryBaseline = 42 },
                new() { Name = "Utility: Network Service", Category = "Apps", IconGlyph = "\uE968", MemoryBaseline = 56, IsHighNetwork = true },
                new() { Name = "Utility: Storage Service", Category = "Apps", IconGlyph = "\uEDA2", MemoryBaseline = 28 },
                new() { Name = "Extension: uBlock Origin", Category = "Apps", IconGlyph = "\uE71B", MemoryBaseline = 45 },
                new() { Name = "Tab: GitHub", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 180 },
                new() { Name = "Tab: Stack Overflow", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 155 },
                new() { Name = "Tab: YouTube", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 340, IsHighCpu = true },
                new() { Name = "Tab: Microsoft Learn", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 120 },
                new() { Name = "Tab: Outlook", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 205 },
                new() { Name = "Tab: Twitter", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 175 },
                new() { Name = "Tab: Azure Portal", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 260, IsHighCpu = true },
                new() { Name = "Service Worker", Category = "Apps", IconGlyph = "\uE713", MemoryBaseline = 35 },
                new() { Name = "Crashpad Handler", Category = "Apps", IconGlyph = "\uE7BA", MemoryBaseline = 2.4 },
            ]
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft Excel",
            Category = "Apps",
            IconGlyph = "\uE9F9",
            MemoryBaseline = 246.8,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft OneNote",
            Category = "Apps",
            IconGlyph = "\uE70B",
            MemoryBaseline = 166.9,
            IsHighNetwork = true,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft Teams",
            Category = "Apps",
            IconGlyph = "\uE902",
            MemoryBaseline = 264.8,
            IsHighCpu = true,
            IsHighNetwork = true,
            StatusText = ProcessItem.EfficiencyMode,
            IsEfficiency = true,
            Children =
            [
                new() { Name = "Teams Main", Category = "Apps", IconGlyph = "\uE902", MemoryBaseline = 180, IsHighCpu = true },
                new() { Name = "Teams GPU", Category = "Apps", IconGlyph = "\uE943", MemoryBaseline = 32 },
                new() { Name = "Teams Utility", Category = "Apps", IconGlyph = "\uE713", MemoryBaseline = 18 },
                new() { Name = "Teams Media", Category = "Apps", IconGlyph = "\uE8D6", MemoryBaseline = 34.8, IsHighNetwork = true },
            ]
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft Visual Studio 2022",
            Category = "Apps",
            IconGlyph = "\uE7C3",
            MemoryBaseline = 642.8,
            IsHighCpu = true,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft Visual Studio 2022",
            Category = "Apps",
            IconGlyph = "\uE7C3",
            MemoryBaseline = 1159.1,
            IsHighCpu = true,
            IsHighDisk = true,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft Visual Studio 2022",
            Category = "Apps",
            IconGlyph = "\uE7C3",
            MemoryBaseline = 962.4,
            IsHighCpu = true,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft Visual Studio 2022",
            Category = "Apps",
            IconGlyph = "\uE7C3",
            MemoryBaseline = 1301.1,
            IsHighCpu = true,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft Word",
            Category = "Apps",
            IconGlyph = "\uE8D2",
            MemoryBaseline = 360.3,
            IsHighCpu = true,
        });

        Processes.Add(new ProcessItem
        {
            Name = "MUXControlsTestApp",
            Category = "Apps",
            IconGlyph = "\uE737",
            MemoryBaseline = 170.1,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Notepad.exe",
            Category = "Apps",
            IconGlyph = "\uE70F",
            MemoryBaseline = 57.3,
            Children =
            [
                new() { Name = "Notepad", Category = "Apps", IconGlyph = "\uE70F", MemoryBaseline = 32 },
                new() { Name = "Notepad", Category = "Apps", IconGlyph = "\uE70F", MemoryBaseline = 25.3 },
            ]
        });

        Processes.Add(new ProcessItem
        {
            Name = "Outlook",
            Category = "Apps",
            IconGlyph = "\uE715",
            MemoryBaseline = 456.4,
            IsHighCpu = true,
            IsHighNetwork = true,
            Children =
            [
                new() { Name = "Outlook", Category = "Apps", IconGlyph = "\uE715", MemoryBaseline = 220 },
                new() { Name = "Crashpad", Category = "Apps", IconGlyph = "\uE7BA", MemoryBaseline = 1.8 },
                new() { Name = "GPU Process", Category = "Apps", IconGlyph = "\uE943", MemoryBaseline = 45 },
                new() { Name = "Utility: Network Service", Category = "Apps", IconGlyph = "\uE968", MemoryBaseline = 32, IsHighNetwork = true },
                new() { Name = "Utility: Storage Service", Category = "Apps", IconGlyph = "\uEDA2", MemoryBaseline = 18 },
                new() { Name = "Service Worker", Category = "Apps", IconGlyph = "\uE713", MemoryBaseline = 14 },
                new() { Name = "Renderer: Mail", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 68 },
                new() { Name = "Renderer: Calendar", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 42 },
                new() { Name = "Manager", Category = "Apps", IconGlyph = "\uE912", MemoryBaseline = 15.6 },
            ]
        });

        Processes.Add(new ProcessItem
        {
            Name = "Task Manager",
            Category = "Apps",
            IconGlyph = "\uE9D9",
            MemoryBaseline = 38.2,
            IsHighCpu = true,
        });

        // ── Background processes ──
        Processes.Add(new ProcessItem
        {
            Name = "Antimalware Service Executable",
            Category = "Background processes",
            IconGlyph = "\uE74D",
            MemoryBaseline = 210.8,
            IsHighCpu = true,
            IsHighDisk = true,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Application Frame Host",
            Category = "Background processes",
            IconGlyph = "\uE737",
            MemoryBaseline = 14.2,
        });

        Processes.Add(new ProcessItem
        {
            Name = "COM Surrogate",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 3.1,
        });

        Processes.Add(new ProcessItem
        {
            Name = "COM Surrogate",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 2.8,
        });

        Processes.Add(new ProcessItem
        {
            Name = "CTF Loader",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 5.4,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Desktop Window Manager",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 124.6,
            IsHighCpu = true,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Device Census",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 6.2,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Host Process for Windows Tasks",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 8.8,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft Distributed Transaction Coordinator",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 4.1,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Microsoft Text Input Application",
            Category = "Background processes",
            IconGlyph = "\uE765",
            MemoryBaseline = 42.3,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Registry",
            Category = "Background processes",
            IconGlyph = "\uE74C",
            MemoryBaseline = 58.0,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Runtime Broker",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 22.5,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Runtime Broker",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 15.8,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Search Host",
            Category = "Background processes",
            IconGlyph = "\uE721",
            MemoryBaseline = 92.3,
            IsHighCpu = true,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Security Health Service",
            Category = "Background processes",
            IconGlyph = "\uE74D",
            MemoryBaseline = 7.6,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Shell Infrastructure Host",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 18.4,
        });

        Processes.Add(new ProcessItem
        {
            Name = "StartMenuExperienceHost.exe",
            Category = "Background processes",
            IconGlyph = "\uE700",
            MemoryBaseline = 56.7,
        });

        Processes.Add(new ProcessItem
        {
            Name = "System",
            Category = "Background processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 0.1,
        });

        Processes.Add(new ProcessItem
        {
            Name = "System Idle Process",
            Category = "Background processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 0.0,
        });

        Processes.Add(new ProcessItem
        {
            Name = "TextInputHost.exe",
            Category = "Background processes",
            IconGlyph = "\uE765",
            MemoryBaseline = 68.9,
        });

        Processes.Add(new ProcessItem
        {
            Name = "User Manager",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 5.2,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Widget Platform",
            Category = "Background processes",
            IconGlyph = "\uE71D",
            MemoryBaseline = 45.1,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Windows Security",
            Category = "Background processes",
            IconGlyph = "\uE74D",
            MemoryBaseline = 0.6,
        });

        // ── Windows processes ──
        Processes.Add(new ProcessItem
        {
            Name = "Client Server Runtime Process",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 2.1,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Client Server Runtime Process",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 1.8,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Local Security Authority Process",
            Category = "Windows processes",
            IconGlyph = "\uE72E",
            MemoryBaseline = 18.4,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Service Host: Local System",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 12.3,
            Children =
            [
                new() { Name = "Background Intelligent Transfer Service", Category = "Windows processes", IconGlyph = "\uE770", MemoryBaseline = 2.1 },
                new() { Name = "Cryptographic Services", Category = "Windows processes", IconGlyph = "\uE72E", MemoryBaseline = 4.6 },
                new() { Name = "Windows Update", Category = "Windows processes", IconGlyph = "\uE895", MemoryBaseline = 5.6 },
            ]
        });

        Processes.Add(new ProcessItem
        {
            Name = "Service Host: Network Service",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 8.7,
            Children =
            [
                new() { Name = "DNS Client", Category = "Windows processes", IconGlyph = "\uE968", MemoryBaseline = 3.2 },
                new() { Name = "Network List Service", Category = "Windows processes", IconGlyph = "\uE968", MemoryBaseline = 5.5 },
            ]
        });

        Processes.Add(new ProcessItem
        {
            Name = "Services",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 9.4,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Windows Logon Application",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 4.2,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Windows Start",
            Category = "Windows processes",
            IconGlyph = "\uE700",
            MemoryBaseline = 32.4,
        });

        Processes.Add(new ProcessItem
        {
            Name = "Windows Explorer",
            Category = "Windows processes",
            IconGlyph = "\uE8B7",
            MemoryBaseline = 112.5,
            IsHighCpu = true,
        });

        // Every row states its status: Task Manager shows Running unless a process is suspended or
        // in efficiency mode. Suspended UWP hosts use no CPU (see RandomizeValues).
        foreach (var process in Processes.Concat(Processes.SelectMany(parent => parent.Children)))
        {
            process.StatusText = process.IsEfficiency
                ? ProcessItem.EfficiencyMode
                : process.Name is "Search Host" or "Widget Platform" ? ProcessItem.Suspended : ProcessItem.Running;
        }

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

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex="0"), before the
        // later-declared elements exist. Guard every element this path touches.
        if (_source is null || ProcessTable is null || ShapingModeSelector is null || GroupKeySelector is null
            || ExpandAllButton is null || CollapseAllButton is null || ShapingModeText is null)
        {
            return;
        }

        var mode = SampleShaping.SelectedTag(ShapingModeSelector, "flat");
        var key = SampleShaping.SelectedTag(GroupKeySelector, "Category");
        var selected = ProcessTable.SelectedItem;

        switch (mode)
        {
            case "grouped":
                // The key selector receives the ROW; the identity selector receives the KEY.
                _source.GroupBy(item => KeyOf(item as ProcessItem, key), SampleShaping.GroupIdentity);
                break;
            // case "hierarchy":
            // case "groupedHierarchy":
            //     Hierarchical (tree) rows are not available in this release, so the two matching
            //     ComboBoxItems ship disabled. TableViewSource and TableView have no hierarchy
            //     member today. When hierarchy ships, apply it to this same source here, composed
            //     with the GroupBy stage above rather than replacing it, so each process expands
            //     into its Children; set _appliedMode only after the call returns.
            default:
                _source.ClearGroupBy();
                mode = "flat";
                break;
        }

        _appliedMode = mode;
        _appliedKey = key;

        // Re-applying GroupBy can drop the selection when the selected row changed group.
        SampleShaping.Reselect(ProcessTable, selected, Processes.Count + s_categories.Length + 3, RefreshReadouts);
        UpdateShapingGating();
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SampleShaping.Label(GroupKeySelector))
                : "Shaping -> Flat");
        }
    }

    // Call after ANY write to the grouped-on property.
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
        ProcessTable.ExpandAllGroups();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        ProcessTable.CollapseAllGroups();
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

/// <summary>
/// Heat-map tint for a Task Manager metric cell: one blue hue whose opacity deepens with load, as
/// in Windows 11 Task Manager. ConverterParameter names the metric (Cpu, Memory, Disk, Network).
/// In a high-contrast theme the tint is dropped so the system colours apply.
/// </summary>
public sealed partial class ProcessHeatBrushConverter : IValueConverter
{
    private static SolidColorBrush[]? s_levels;

    // Thresholds for the four steps above the base tint, per metric.
    private static readonly double[] s_cpu = { 0.5, 2, 5, 10 };
    private static readonly double[] s_memory = { 50, 200, 500, 1000 };
    private static readonly double[] s_disk = { 0.1, 0.5, 2, 5 };
    private static readonly double[] s_network = { 0.05, 0.3, 1, 5 };

    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (ChipBrushes.IsHighContrast)
        {
            return ChipBrushes.Transparent;
        }

        // Created on first use, on the UI thread, and shared by every cell.
        s_levels ??= new[] { 22, 38, 58, 82, 110 }
            .Select(alpha => new SolidColorBrush(Color.FromArgb((byte)alpha, 96, 160, 240)))
            .ToArray();

        var thresholds = (parameter as string) switch
        {
            "Memory" => s_memory,
            "Disk" => s_disk,
            "Network" => s_network,
            _ => s_cpu,
        };

        var load = value is double d ? d : 0;
        return s_levels[thresholds.Count(threshold => load > threshold)];
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}

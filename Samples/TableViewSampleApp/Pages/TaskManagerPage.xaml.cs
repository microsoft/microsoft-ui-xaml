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
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewSortedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs;
// #44 enum unification: TableViewSortDirection was removed in favor of the shared Data enum.
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;

using TableViewSampleApp.Data;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Task Manager-style process dashboard backed by the aligned TableViewSource surface.
/// </summary>
public sealed partial class TaskManagerPage : Page
{
    private readonly DispatcherTimer _timer;
    private readonly Random _rng = new();
    private string? _searchText;

    // Reshaping happens IN PLACE on this one instance: Filter / GroupBy / Sort mutate and return
    // the same TableViewSource (TableViewSource.cpp:49-50), so rebuilding it per change would
    // needlessly drop selection, scroll offset and group expansion.
    private TableViewSource? _source;
    private string _mode = "flat";          // requested
    private string _appliedMode = "flat";   // applied - every readout and guard reads THIS
    private string _groupKey = "Category";
    private readonly List<ProcessItem> _stashedProcesses = new();
    private int _addedProcessCount;

    private static readonly string[] s_categories =
    {
        "Apps",
        "Background processes",
        "Windows processes",
    };

    public ObservableCollection<ProcessItem> Processes { get; } = new();

    public TaskManagerPage()
    {
        InitializeComponent();
        ProcessTable.HeadersVisibility = TableViewHeadersVisibility.Column;
        ProcessTable.GridLinesVisibility = TableViewGridLinesVisibility.Horizontal;
        ProcessTable.Density = TableViewDensity.Compact;
        // No AlternatingRowBackground: real Task Manager doesn't zebra-stripe its rows,
        // so banding stays off — only the per-cell heat map tints rows by load.

        PopulateProcesses();
        ApplyProcessSource();
        UpdateStatusBar();
        UpdateSummary();

        _timer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(2) };
        _timer.Tick += OnTimerTick;

        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
    }

    private bool _syncingTheme;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        if (!_timer.IsEnabled)
        {
            _timer.Start();
        }

        // Default to the system / inherited theme; reflect it on the toggle WITHOUT forcing
        // RequestedTheme, so the page stays on the system theme until the user explicitly
        // switches to dark or light.
        _syncingTheme = true;
        ThemeToggle.IsOn = ActualTheme == ElementTheme.Dark;
        _syncingTheme = false;
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e) => _timer.Stop();

    private void ApplyProcessSource()
    {
        if (ProcessTable is null)
        {
            return;
        }

        if (_source is null)
        {
            _source = TableViewSource.From(Processes);
            ProcessTable.ItemsSource = _source;
        }

        var searchText = _searchText;
        if (string.IsNullOrEmpty(searchText))
        {
            _source.ClearFilter();
        }
        else
        {
            _source.Filter(item => MatchesSearch((ProcessItem)item, searchText));
        }

        switch (_mode)
        {
            case "grouped":
                var key = _groupKey;
                // The two delegates receive DIFFERENT things despite both parameters being named
                // `item`: TableViewKeySelector gets the ROW ITEM, TableViewIdentitySelector gets
                // the GROUP KEY (TableViewSource.idl:12-16). An item-typed identity lambda would
                // return an empty identity, which fails fast with E_INVALIDARG.
                _source.GroupBy(
                    item => (object)GroupValue(item, key),
                    groupKey => groupKey?.ToString() ?? "(none)");
                break;

            // case "hierarchy":
            // case "groupedhierarchy":
            //     Hierarchical rows are not available in this release, and no hierarchy API exists
            //     on TableViewSource or TableView yet - so there is deliberately no call written
            //     out here to copy. When the control ships hierarchy support, apply it to this
            //     same source instance alongside the GroupBy stage above so the two compose, and
            //     drop the IsEnabled="False" from the matching options in the XAML.
            //     break;

            default:
                _source.ClearGroupBy();
                break;
        }

        // Set ONLY after the shaping call returns: a readout driven by the REQUESTED mode lies
        // about the table whenever GroupBy throws.
        _appliedMode = _mode;

        ApplyActiveSort(_source);
        UpdateShapingUi();
        UpdateStatusBar();
    }

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (ProcessTable is null || ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _mode = tag;
        ApplyProcessSource();
        SetLastAction(string.Format(CultureInfo.InvariantCulture, "Shaping mode -> {0}", ModeLabel(_appliedMode)));
    }

    private void OnShapingGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (ProcessTable is null || ShapingGroupKeyCombo?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        ApplyProcessSource();
        SetLastAction(string.Format(CultureInfo.InvariantCulture, "Group key -> {0}", tag));
    }

    private void UpdateShapingUi()
    {
        var grouped = _appliedMode == "grouped";
        var hasSelection = ProcessTable?.SelectedItem is ProcessItem;

        if (ShapingGroupKeyCombo is not null)
        {
            ShapingGroupKeyCombo.IsEnabled = grouped;
            ToolTipService.SetToolTip(ShapingGroupKeyCombo, grouped
                ? "Change the GroupBy key while grouped - expansion and selection survive."
                : "Available once Grouped mode is selected.");
        }

        SetGroupActionState(ExpandAllButton, grouped, "Expand every group.");
        SetGroupActionState(CollapseAllButton, grouped, "Collapse every group.");

        if (MoveCategoryButton is not null)
        {
            MoveCategoryButton.IsEnabled = hasSelection;
            ToolTipService.SetToolTip(MoveCategoryButton, hasSelection
                ? "Rewrites the selected process's Category, then re-applies GroupBy so the row moves to another group."
                : "Select a process first.");
        }

        if (EmptyToggleButton is not null)
        {
            EmptyToggleButton.Content = _stashedProcesses.Count > 0
                ? "Restore all rows"
                : "Remove all rows (group an empty set)";
        }

        if (AppliedModeText is null)
        {
            return;
        }

        AppliedModeText.Text = grouped
            ? string.Format(CultureInfo.InvariantCulture, "Grouped by {0}", _groupKey)
            : "Flat (no shaping)";

        var groups = GroupCounts().ToList();
        GroupCountText.Text = !grouped
            ? "(n/a - flat)"
            : groups.Count == 0
                ? "0 (empty result)"
                : string.Join(", ", groups.Select(g => string.Format(CultureInfo.InvariantCulture, "{0} ({1})", g.Key, g.Count)));
        RowCountText.Text = VisibleProcesses().Count().ToString(CultureInfo.InvariantCulture);
    }

    private static void SetGroupActionState(Button? button, bool grouped, string enabledTip)
    {
        if (button is null)
        {
            return;
        }

        button.IsEnabled = grouped;
        ToolTipService.SetToolTip(button, grouped ? enabledTip : "Available once Grouped mode is selected.");
    }

    private IEnumerable<ProcessItem> VisibleProcesses()
    {
        var searchText = _searchText;
        return string.IsNullOrEmpty(searchText)
            ? Processes
            : Processes.Where(process => MatchesSearch(process, searchText));
    }

    private IEnumerable<(string Key, int Count)> GroupCounts()
    {
        var key = _groupKey;
        return VisibleProcesses()
            .GroupBy(process => GroupValue(process, key), StringComparer.Ordinal)
            .OrderBy(group => group.Key, StringComparer.Ordinal)
            .Select(group => (group.Key, group.Count()));
    }

    // Never returns string.Empty: an empty group identity is an E_INVALIDARG fail-fast.
    private static string GroupValue(object item, string key)
    {
        if (item is not ProcessItem process)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Status" => process.IsEfficiency
                ? "Efficiency mode"
                : (string.IsNullOrWhiteSpace(process.StatusText) ? "Running" : process.StatusText),
            _ => process.Category,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private static string ModeLabel(string mode) => mode == "grouped" ? "Grouped" : "Flat";

    private void SetLastAction(string text)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = text;
        }
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        ProcessTable.ExpandAllGroups();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        ProcessTable.CollapseAllGroups();
        SetLastAction("Collapsed all groups");
    }

    private void OnMoveSelectedCategoryClick(object sender, RoutedEventArgs e)
    {
        if (ProcessTable?.SelectedItem is not ProcessItem selected)
        {
            return;
        }

        var index = Array.IndexOf(s_categories, selected.Category);
        var next = s_categories[(index + 1 + s_categories.Length) % s_categories.Length];
        var previous = string.IsNullOrWhiteSpace(selected.Category) ? "(none)" : selected.Category;
        selected.Category = next;

        // Re-running the shaping stage re-reads every key, so the mutated row re-buckets.
        ApplyProcessSource();
        SetLastAction(string.Format(CultureInfo.InvariantCulture, "{0}: {1} -> {2}", selected.Name, previous, next));
    }

    private void OnAddProcessClick(object sender, RoutedEventArgs e)
    {
        _addedProcessCount++;
        var added = new ProcessItem
        {
            Name = string.Format(CultureInfo.InvariantCulture, "Sample app {0}", _addedProcessCount),
            Category = "Apps",
            IconGlyph = "\uE7B8",
            MemoryBaseline = 18 + (_addedProcessCount * 3),
        };
        RandomizeValues(added);
        Processes.Insert(0, added);

        ApplyProcessSource();
        UpdateSummary();
        SetLastAction(string.Format(CultureInfo.InvariantCulture, "Added {0} to the Apps group", added.Name));
    }

    private void OnEmptyToggleClick(object sender, RoutedEventArgs e)
    {
        if (_stashedProcesses.Count > 0)
        {
            foreach (var process in _stashedProcesses)
            {
                Processes.Add(process);
            }

            var restored = _stashedProcesses.Count;
            _stashedProcesses.Clear();
            ApplyProcessSource();
            UpdateSummary();
            SetLastAction(string.Format(CultureInfo.InvariantCulture, "Restored {0} rows", restored));
            return;
        }

        _stashedProcesses.AddRange(Processes);
        Processes.Clear();
        ApplyProcessSource();
        UpdateSummary();
        SetLastAction("Removed all rows - the grouped projection now has zero groups");
    }

    private void ApplyActiveSort(TableViewSource source)
    {
        // Clear first: the keySelector Sort overload declares an ANONYMOUS axis, so re-declaring
        // one without clearing would stack a second axis rather than replace the first.
        source.ClearSort();

        var sortColumn = SampleShape.ActiveSortColumn(ProcessTable);
        if (ProcessTable is null ||
            sortColumn is null ||
            SampleShape.ActiveSortDirection(ProcessTable) == TableViewSortDirection.None ||
            string.IsNullOrEmpty(sortColumn.SortMemberPath))
        {
            return;
        }

        var path = sortColumn.SortMemberPath;
        source.Sort(item => SortKey(item, path), SampleShape.ActiveSortDirection(ProcessTable));
    }

    private void OnSearchBoxTextChanged(AutoSuggestBox sender, AutoSuggestBoxTextChangedEventArgs args)
    {
        if (args.Reason != AutoSuggestionBoxTextChangeReason.UserInput)
        {
            return;
        }

        _searchText = SearchBox.Text?.Trim();
        ApplyProcessSource();
    }

    private static object? SortKey(object item, string path)
    {
        if (item is not ProcessItem process)
        {
            return null;
        }

        return path switch
        {
            nameof(ProcessItem.Name) => process.Name,
            nameof(ProcessItem.StatusText) => process.StatusText,
            nameof(ProcessItem.CpuPercent) => process.CpuPercent,
            nameof(ProcessItem.MemoryMB) => process.MemoryMB,
            nameof(ProcessItem.DiskMBps) => process.DiskMBps,
            nameof(ProcessItem.NetworkMbps) => process.NetworkMbps,
            _ => null,
        };
    }

    private static string ProcessIdentity(object item) => item is ProcessItem process ? process.Key : string.Empty;

    private void OnProcessTableSorted(TableView sender, TableViewSortedEventArgs args)
    {
        ApplyProcessSource();
    }

    private void OnThemeToggled(object sender, RoutedEventArgs e)
    {
        if (_syncingTheme)
        {
            return;
        }

        RequestedTheme = ThemeToggle.IsOn ? ElementTheme.Dark : ElementTheme.Light;
    }

    private void OnTimerTick(object? sender, object e)
    {
        foreach (var process in Processes)
        {
            RandomizeValues(process);
            if (process.Children is { Count: > 0 })
            {
                foreach (var child in process.Children)
                {
                    RandomizeValues(child);
                }

                AggregateFromChildren(process);
            }
        }

        UpdateSummary();
    }

    private void RandomizeValues(ProcessItem p)
    {
        var cpuBase = p.IsHighCpu ? _rng.NextDouble() * 15 + 5 : _rng.NextDouble() * 3;
        p.CpuPercent = Math.Round(cpuBase, 1);

        var memDrift = (_rng.NextDouble() - 0.5) * p.MemoryBaseline * 0.08;
        p.MemoryMB = Math.Max(0.5, Math.Round(p.MemoryBaseline + memDrift, 1));

        var diskBase = p.IsHighDisk ? _rng.NextDouble() * 2 : _rng.NextDouble() * 0.3;
        p.DiskMBps = Math.Round(diskBase, 1);

        var netBase = p.IsHighNetwork ? _rng.NextDouble() * 1.5 : _rng.NextDouble() * 0.2;
        p.NetworkMbps = Math.Round(netBase, 1);
    }

    private static void AggregateFromChildren(ProcessItem parent)
    {
        if (parent.Children is not { Count: > 0 }) return;

        double cpu = 0, mem = 0, disk = 0, net = 0;
        foreach (var child in parent.Children)
        {
            cpu += child.CpuPercent;
            mem += child.MemoryMB;
            disk += child.DiskMBps;
            net += child.NetworkMbps;
        }

        parent.CpuPercent = Math.Round(cpu, 1);
        parent.MemoryMB = Math.Round(mem, 1);
        parent.DiskMBps = Math.Round(disk, 1);
        parent.NetworkMbps = Math.Round(net, 1);
    }

    private void UpdateSummary()
    {
        var totalCpu = Processes.Sum(p => p.CpuPercent);
        var totalMem = Processes.Sum(p => p.MemoryMB);
        var totalDisk = Processes.Sum(p => p.DiskMBps);
        var totalNet = Processes.Sum(p => p.NetworkMbps);

        // Per-process CpuPercent is modeled per-core (a process can exceed 100% individually and
        // the set sums well above 100%), so overall utilization = total / core count — NOT a raw
        // clamped sum, which pinned this readout at 100%.
        const int simulatedCores = 12;
        CpuSummary.Text = $"{Math.Min(totalCpu / simulatedCores, 100):F0}%";
        MemorySummary.Text = $"{totalMem / 1024 / 16 * 100:F0}%";
        DiskSummary.Text = $"{Math.Min(totalDisk, 100):F0}%";
        NetworkSummary.Text = $"{Math.Min(totalNet, 100):F0}%";
    }

    private void UpdateStatusBar()
    {
        var total = Processes.Sum(p => 1 + p.Children.Count);
        var processCount = Processes.Count.ToString(CultureInfo.InvariantCulture);
        var searchText = _searchText;
        if (!string.IsNullOrEmpty(searchText))
        {
            processCount = string.Format(CultureInfo.InvariantCulture,
                "{0} (filtered)",
                Processes.Count(process => MatchesSearch(process, searchText)));
        }

        StatusBar.Text = string.Format(CultureInfo.InvariantCulture,
            "Processes: {0}   Threads: {1}   Handles: {2}",
            processCount,
            total * 12,
            total * 347);
    }

    private static bool MatchesSearch(ProcessItem process, string query)
    {
        return (process.Name?.Contains(query, StringComparison.OrdinalIgnoreCase) ?? false)
            || (process.Category?.Contains(query, StringComparison.OrdinalIgnoreCase) ?? false)
            || (process.StatusText?.Contains(query, StringComparison.OrdinalIgnoreCase) ?? false);
    }

    private void OnEndTaskClick(object sender, RoutedEventArgs e)
    {
        if (ProcessTable.SelectedItem is not ProcessItem selected)
        {
            return;
        }

        foreach (var process in Processes)
        {
            if (process.Children.Remove(selected))
            {
                UpdateStatusBar();
                SelectedProcessText.Text = "(none)";
                ApplyProcessSource();
                SetLastAction(string.Format(CultureInfo.InvariantCulture, "Removed child {0}", selected.Name));
                return;
            }
        }

        if (Processes.Remove(selected))
        {
            UpdateStatusBar();
            SelectedProcessText.Text = "(none)";
            ApplyProcessSource();
            SetLastAction(string.Format(CultureInfo.InvariantCulture, "Removed {0} from its group", selected.Name));
        }
    }

    private void OnSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        SelectedProcessText.Text = ProcessTable.SelectedItem is ProcessItem process
            ? process.Name
            : "(none)";
        UpdateShapingUi();
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
                new() { Name = "WebView2: Calendar in Taskbar", Category = "Apps", IconGlyph = "\uE774", MemoryBaseline = 0.8, StatusText = "Efficiency …", IsEfficiency = true },
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
                new() { Name = "WebView2: Files Search App", Category = "Apps", IconGlyph = "\uE774", MemoryBaseline = 0.5, StatusText = "Efficiency …", IsEfficiency = true },
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
            StatusText = "Efficiency …",
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
    }
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;
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

    private static Brush? GetBrushResource(string key)
    {
        return Application.Current.Resources.TryGetValue(key, out var resource) && resource is Brush brush ? brush : null;
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

        // The picker chooses the data shaping:
        //   0 Grouped by category – Apps / Background / Windows group-header bands
        //   1 Flat list           – ungrouped
        var mode = ViewModeCombo?.SelectedIndex ?? 0;
        var searchText = _searchText;

        var source = TableViewSource.From(Processes);
        if (!string.IsNullOrEmpty(searchText))
        {
            source = source.Filter(item => MatchesSearch((ProcessItem)item, searchText));
        }

        if (mode == 0)
        {
            // Grouping a source is not in this release.
        }

        ProcessTable.ItemsSource = ApplyActiveSort(source);
        UpdateStatusBar();
    }

    private void OnViewModeChanged(object sender, SelectionChangedEventArgs e) => ApplyProcessSource();

    private void OnSearchBoxTextChanged(AutoSuggestBox sender, AutoSuggestBoxTextChangedEventArgs args)
    {
        if (args.Reason != AutoSuggestionBoxTextChangeReason.UserInput)
        {
            return;
        }

        _searchText = SearchBox.Text?.Trim();
        ApplyProcessSource();
    }

    private TableViewSource ApplyActiveSort(TableViewSource source)
    {
        var sortColumn = SampleShape.ActiveSortColumn(ProcessTable);
        if (ProcessTable is null ||
            sortColumn is null ||
            SampleShape.ActiveSortDirection(ProcessTable) == TableViewSortDirection.None ||
            string.IsNullOrEmpty(sortColumn.SortMemberPath))
        {
            return source;
        }

        return source.Sort(item => SortKey(item, sortColumn.SortMemberPath), SampleShape.ActiveSortDirection(ProcessTable));
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

    private static string GroupIdentity(object key) => key?.ToString() ?? string.Empty;

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
                return;
            }
        }

        if (Processes.Remove(selected))
        {
            UpdateStatusBar();
            SelectedProcessText.Text = "(none)";
            ApplyProcessSource();
        }
    }

    private void OnSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        SelectedProcessText.Text = ProcessTable.SelectedItem is ProcessItem process
            ? process.Name
            : "(none)";
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

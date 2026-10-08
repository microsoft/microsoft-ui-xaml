// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;

namespace TableViewSampleApp.Models;

/// <summary>
/// One process row on the Task Manager page. The model holds data only: the heat-map tint of the
/// metric cells comes from ProcessHeatBrushConverter, and the efficiency leaf binds IsEfficiency
/// directly (x:Bind converts bool to Visibility).
/// </summary>
public sealed class ProcessItem : INotifyPropertyChanged
{
    public const string Running = "Running";
    public const string Suspended = "Suspended";
    public const string EfficiencyMode = "Efficiency mode";

    public string Name { get; set; } = string.Empty;

    public string IconGlyph { get; set; } = "\uE7BA";

    private string _category = string.Empty;
    public string Category
    {
        get => _category;
        set => SetField(ref _category, value);
    }

    // Always one of Running, Suspended or Efficiency mode; the cell trims it if it must.
    private string _statusText = Running;
    public string StatusText
    {
        get => _statusText;
        set => SetField(ref _statusText, value);
    }

    public bool IsEfficiency { get; set; }

    public bool IsSuspended => StatusText == Suspended;

    // Child processes (browser tabs, WebView2 helpers) are not rows: their values are summed into
    // the parent's metrics on every tick. They become expandable rows when hierarchy ships.
    public ObservableCollection<ProcessItem> Children { get; set; } = new();

    // Behaviour hints for the simulation.
    public bool IsHighCpu { get; set; }
    public bool IsHighDisk { get; set; }
    public bool IsHighNetwork { get; set; }
    public double MemoryBaseline { get; set; } = 10;

    private double _cpuPercent;
    public double CpuPercent
    {
        get => _cpuPercent;
        set { if (SetField(ref _cpuPercent, value)) { OnPropertyChanged(nameof(CpuDisplay)); } }
    }

    private double _memoryMB;
    public double MemoryMB
    {
        get => _memoryMB;
        set { if (SetField(ref _memoryMB, value)) { OnPropertyChanged(nameof(MemoryDisplay)); } }
    }

    private double _diskMBps;
    public double DiskMBps
    {
        get => _diskMBps;
        set { if (SetField(ref _diskMBps, value)) { OnPropertyChanged(nameof(DiskDisplay)); } }
    }

    private double _networkMbps;
    public double NetworkMbps
    {
        get => _networkMbps;
        set { if (SetField(ref _networkMbps, value)) { OnPropertyChanged(nameof(NetworkDisplay)); } }
    }

    // Task Manager formatting, in the current culture.
    public string CpuDisplay => CpuPercent < 0.05
        ? string.Format(CultureInfo.CurrentCulture, "{0}%", 0)
        : string.Format(CultureInfo.CurrentCulture, "{0:N1}%", CpuPercent);

    public string MemoryDisplay => MemoryMB >= 1
        ? string.Format(CultureInfo.CurrentCulture, "{0:N1} MB", MemoryMB)
        : string.Format(CultureInfo.CurrentCulture, "{0:N0} KB", MemoryMB * 1024);

    public string DiskDisplay => DiskMBps < 0.05
        ? string.Format(CultureInfo.CurrentCulture, "{0} MB/s", 0)
        : string.Format(CultureInfo.CurrentCulture, "{0:N1} MB/s", DiskMBps);

    public string NetworkDisplay => NetworkMbps < 0.05
        ? string.Format(CultureInfo.CurrentCulture, "{0} Mbps", 0)
        : string.Format(CultureInfo.CurrentCulture, "{0:N1} Mbps", NetworkMbps);

    public event PropertyChangedEventHandler? PropertyChanged;

    public override string ToString() => Name;

    private void OnPropertyChanged(string name) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));

    private bool SetField<T>(ref T field, T value, [CallerMemberName] string? name = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value))
        {
            return false;
        }

        field = value;
        OnPropertyChanged(name!);
        return true;
    }
}

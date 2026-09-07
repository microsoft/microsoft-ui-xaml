// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;
using Windows.UI;

namespace TableViewSampleApp.Models;
public sealed class ProcessItem : INotifyPropertyChanged
{
    // ── Static brushes (shared across all items) ──
    // Match Windows 11 Task Manager: ONE blue hue whose opacity deepens with load
    // (the hue never changes — only intensity). Every data cell carries the base tint.
    private static readonly SolidColorBrush TransparentBrush = new(Colors.Transparent);
    private static readonly SolidColorBrush BaseBrush = new(Color.FromArgb(22, 96, 160, 240));      // base column tint
    private static readonly SolidColorBrush LowBrush = new(Color.FromArgb(38, 96, 160, 240));       // faint
    private static readonly SolidColorBrush MedBrush = new(Color.FromArgb(58, 96, 160, 240));       // light
    private static readonly SolidColorBrush HighBrush = new(Color.FromArgb(82, 96, 160, 240));      // medium
    private static readonly SolidColorBrush VeryHighBrush = new(Color.FromArgb(110, 96, 160, 240)); // strong

    // ── Identity ──
    // Globally-unique, stable per-instance key for diagnostics and generated rows.
    private static int s_nextKey;
    public string Key { get; } = System.Threading.Interlocked.Increment(ref s_nextKey)
        .ToString(System.Globalization.CultureInfo.InvariantCulture);

    public string Name { get; set; } = string.Empty;
    public string Category { get; set; } = string.Empty;
    public string IconGlyph { get; set; } = "\uE7BA";
    public string IconPath { get; set; } = string.Empty;

    // ── Visibility helpers for icon/glyph ──
    public Visibility HasIcon => !IsCategoryNode && !string.IsNullOrEmpty(IconPath) ? Visibility.Visible : Visibility.Collapsed;
    public Visibility HasGlyph => !IsCategoryNode && string.IsNullOrEmpty(IconPath) ? Visibility.Visible : Visibility.Collapsed;

    // ── Status ──
    public string StatusText { get; set; } = string.Empty;
    public bool IsEfficiency { get; set; }
    public Visibility HasStatusText => string.IsNullOrEmpty(StatusText) ? Visibility.Collapsed : Visibility.Visible;
    public Visibility IsEfficiencyMode => IsEfficiency ? Visibility.Visible : Visibility.Collapsed;

    // ── Child process data used for telemetry aggregation ──
    public ObservableCollection<ProcessItem> Children { get; set; } = new();

    public bool IsCategoryNode { get; set; }

    // ── Behavioral hints (for simulation) ──
    public bool IsHighCpu { get; set; }
    public bool IsHighDisk { get; set; }
    public bool IsHighNetwork { get; set; }
    public double MemoryBaseline { get; set; } = 10;

    // ── Live stats ──
    private double _cpuPercent;
    public double CpuPercent
    {
        get => _cpuPercent;
        set { if (SetField(ref _cpuPercent, value)) { OnPropertyChanged(nameof(CpuDisplay)); OnPropertyChanged(nameof(CpuBackground)); } }
    }

    private double _memoryMB;
    public double MemoryMB
    {
        get => _memoryMB;
        set { if (SetField(ref _memoryMB, value)) { OnPropertyChanged(nameof(MemoryDisplay)); OnPropertyChanged(nameof(MemoryBackground)); } }
    }

    private double _diskMBps;
    public double DiskMBps
    {
        get => _diskMBps;
        set { if (SetField(ref _diskMBps, value)) { OnPropertyChanged(nameof(DiskDisplay)); OnPropertyChanged(nameof(DiskBackground)); } }
    }

    private double _networkMbps;
    public double NetworkMbps
    {
        get => _networkMbps;
        set { if (SetField(ref _networkMbps, value)) { OnPropertyChanged(nameof(NetworkDisplay)); OnPropertyChanged(nameof(NetworkBackground)); } }
    }

    // ── Display strings (Task Manager formatting) ──
    public string CpuDisplay => IsCategoryNode ? string.Empty : (CpuPercent < 0.05 ? "0%" : $"{CpuPercent:F1}%");

    public string MemoryDisplay
    {
        get
        {
            if (IsCategoryNode) return string.Empty;
            if (MemoryMB >= 1000) return $"{MemoryMB:N1} MB";
            if (MemoryMB >= 1) return $"{MemoryMB:F1} MB";
            return $"{MemoryMB * 1024:F0} KB";
        }
    }

    public string DiskDisplay => IsCategoryNode ? string.Empty : (DiskMBps < 0.05 ? "0 MB/s" : $"{DiskMBps:F1} MB/s");
    public string NetworkDisplay => IsCategoryNode ? string.Empty : (NetworkMbps < 0.05 ? "0 Mbps" : $"{NetworkMbps:F1} Mbps");

    // ── Heat-map backgrounds (amber → orange → red based on intensity) ──
    public Brush CpuBackground => IsCategoryNode ? TransparentBrush : CpuPercent switch
    {
        > 10 => VeryHighBrush,
        > 5 => HighBrush,
        > 2 => MedBrush,
        > 0.5 => LowBrush,
        _ => BaseBrush
    };

    public Brush MemoryBackground => IsCategoryNode ? TransparentBrush : MemoryMB switch
    {
        > 1000 => VeryHighBrush,
        > 500 => HighBrush,
        > 200 => MedBrush,
        > 50 => LowBrush,
        _ => BaseBrush
    };

    public Brush DiskBackground => IsCategoryNode ? TransparentBrush : DiskMBps switch
    {
        > 5 => VeryHighBrush,
        > 2 => HighBrush,
        > 0.5 => MedBrush,
        > 0.1 => LowBrush,
        _ => BaseBrush
    };

    public Brush NetworkBackground => IsCategoryNode ? TransparentBrush : NetworkMbps switch
    {
        > 5 => VeryHighBrush,
        > 1 => HighBrush,
        > 0.3 => MedBrush,
        > 0.05 => LowBrush,
        _ => BaseBrush
    };

    // ── INotifyPropertyChanged ──
    public event PropertyChangedEventHandler? PropertyChanged;

    public override string ToString() => Key;

    private void OnPropertyChanged(string name) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));

    private bool SetField<T>(ref T field, T value, [CallerMemberName] string? name = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value)) return false;
        field = value;
        OnPropertyChanged(name!);
        return true;
    }
}

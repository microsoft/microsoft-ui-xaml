// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Microsoft.Windows.ApplicationModel.Resources;
using OnlineXamlProfiler.Core;
using Windows.Foundation;

namespace OnlineXamlProfiler.App;

public sealed partial class ProcessStatisticsPanel : UserControl
{
    private readonly ResourceLoader _resources = new();
    private ProcessStatisticsHistory? _history;
    private IReadOnlyList<CapturedSnapshot> _snapshots = [];
    private CapturedSnapshot? _selected;

    public ProcessStatisticsPanel()
    {
        InitializeComponent();
        ActualThemeChanged += (_, _) => DrawGraphs();
        Update(null, [], null, Text("StatisticsIdle"));
    }

    public void Update(ProcessStatisticsHistory? history, IReadOnlyList<CapturedSnapshot> snapshots,
        CapturedSnapshot? selected, string status)
    {
        _history = history;
        _snapshots = snapshots;
        _selected = selected;
        SamplingStatus.Text = status;
        var latest = history?.Latest;
        var memory = latest?.Memory;
        RamValue.Text = Format("RamReading", Bytes(memory?.WorkingSetBytes), Bytes(memory?.PrivateWorkingSetBytes));
        RamPeak.Text = Format("WorkingSetPeak", Bytes(memory?.PeakWorkingSetBytes));
        CommitValue.Text = Bytes(memory?.PrivateCommitBytes);
        CommitPeak.Text = Format("CommitPeak", Bytes(memory?.PeakPrivateCommitBytes));
        GpuMemoryValue.Text = Format("GpuMemoryReading", Bytes(latest?.GpuMemory?.DedicatedBytes), Bytes(latest?.GpuMemory?.SharedBytes));
        GpuMemoryPeak.Text = Format("ObservedPeak", Bytes(history?.ObservedGpuMemoryPeakBytes));
        GpuEngineValue.Text = latest?.GpuEngine is { } engine
            ? engine.UtilizationPercent.ToString("F1", CultureInfo.CurrentCulture) + " %" : Text("StatisticsUnavailable");
        string? instance = latest?.GpuEngine?.Engine;
        int engineType = instance?.IndexOf("_engtype_", StringComparison.Ordinal) ?? -1;
        GpuEngineName.Text = engineType >= 0
            ? instance![(engineType + "_engtype_".Length)..] + " | " + instance[..engineType]
            : instance ?? Text("StatisticsNoEngine");
        ToolTipService.SetToolTip(GpuEngineName, instance ?? GpuEngineName.Text);
        CounterDiagnostic.Text = latest is null ? "" : string.Join(" | ", new[]
        {
            latest.MemoryError, memory?.PrivateWorkingSetError ?? "", latest.GpuMemoryError, latest.GpuEngineError,
        }.Where(message => message.Length != 0).Distinct());

        SnapshotReading.Text = selected is null ? Text("StatisticsNoSnapshot") :
            selected.Statistics is not { } sample ? Format("StatisticsSnapshotMissing", selected.Label) :
            Format("StatisticsSnapshotReading", selected.Label, sample.TimeUtc.ToLocalTime(),
                Stopwatch.GetElapsedTime(selected.CounterTimestamp, sample.CounterTimestamp).TotalMilliseconds,
                Bytes(sample.Memory?.WorkingSetBytes), Bytes(sample.Memory?.PrivateWorkingSetBytes),
                Bytes(sample.Memory?.PrivateCommitBytes), Bytes(sample.GpuMemory?.DedicatedBytes),
                Bytes(sample.GpuMemory?.SharedBytes),
                sample.GpuEngine is { } gpu ? gpu.UtilizationPercent.ToString("F1", CultureInfo.CurrentCulture) + " %" : Text("StatisticsUnavailable"),
                Bytes(sample.Memory?.PeakWorkingSetBytes), Bytes(sample.Memory?.PeakPrivateCommitBytes),
                Bytes(selected.ObservedGpuMemoryPeakBytes));
        if (selected?.Statistics is { } captured)
        {
            string diagnostic = string.Join(" | ", new[]
            {
                captured.MemoryError, captured.Memory?.PrivateWorkingSetError ?? "",
                captured.GpuMemoryError, captured.GpuEngineError,
            }.Where(message => message.Length != 0).Distinct());
            if (diagnostic.Length != 0) { SnapshotReading.Text += Environment.NewLine + diagnostic; }
        }
        ToolTipService.SetToolTip(CounterDiagnostic, CounterDiagnostic.Text);
        ToolTipService.SetToolTip(SnapshotReading, SnapshotReading.Text);
        DrawGraphs();
    }

    private void Graph_SizeChanged(object sender, SizeChangedEventArgs args) => DrawGraphs();

    private void DrawGraphs()
    {
        if (RamGraph is null) { return; }
        Draw(RamGraph, RamScale, false, sample => sample.Memory?.WorkingSetBytes, sample => sample.Memory?.PrivateWorkingSetBytes);
        Draw(CommitGraph, CommitScale, false, sample => sample.Memory?.PrivateCommitBytes);
        Draw(GpuMemoryGraph, GpuMemoryScale, false, sample => sample.GpuMemory?.DedicatedBytes, sample => sample.GpuMemory?.SharedBytes);
        Draw(GpuEngineGraph, GpuEngineScale, true, sample => sample.GpuEngine?.UtilizationPercent);
    }

    private void Draw(Canvas canvas, TextBlock scaleLabel, bool percent,
        params Func<ProcessStatisticsSample, double?>[] series)
    {
        canvas.Children.Clear();
        double width = canvas.ActualWidth, height = canvas.ActualHeight;
        if (width <= 0 || height <= 4) { return; }
        canvas.Clip = new RectangleGeometry { Rect = new(0, 0, width, height) };
        var samples = _history?.Samples ?? [];
        var values = samples.SelectMany(sample => series.Select(select => select(sample)))
            .Where(value => value.HasValue).Select(value => value!.Value).ToArray();
        double maximum = percent ? 100 : Math.Max(1024 * 1024, values.DefaultIfEmpty(0).Max() * 1.1);
        scaleLabel.Text = values.Length == 0 ? Text("StatisticsNoSamples") :
            Format("StatisticsScale", percent ? "100 %" : Bytes(maximum));
        var primary = (Brush)Resources["GraphPrimary"];
        var secondary = (Brush)Resources["GraphSecondary"];
        var gridBrush = (Brush)Resources["GraphGrid"];
        for (int index = 0; index <= 2; ++index)
        {
            double y = 2 + (height - 4) * index / 2;
            canvas.Children.Add(new Line { X1 = 0, X2 = width, Y1 = y, Y2 = y, Stroke = gridBrush });
        }
        if (samples.Count == 0) { return; }
        long end = samples[^1].CounterTimestamp;
        double X(long timestamp) => width * (1 - Stopwatch.GetElapsedTime(timestamp, end).TotalSeconds / ProcessStatisticsHistory.Duration.TotalSeconds);
        for (int index = 0; index < series.Length; ++index)
        {
            Polyline? line = null;
            ProcessStatisticsSample? previous = null;
            foreach (var sample in samples)
            {
                if (series[index](sample) is not { } value)
                {
                    line = null;
                    previous = null;
                    continue;
                }
                if (line is null || (previous is not null &&
                    Stopwatch.GetElapsedTime(previous.CounterTimestamp, sample.CounterTimestamp).TotalSeconds > 2.5))
                {
                    line = new Polyline { Stroke = index == 0 ? primary : secondary, StrokeThickness = 2 };
                    if (index != 0) { line.StrokeDashArray = new DoubleCollection { 3, 2 }; }
                    canvas.Children.Add(line);
                }
                line.Points.Add(new Point(X(sample.CounterTimestamp), height - 2 - value / maximum * (height - 4)));
                previous = sample;
            }
        }
        foreach (var snapshot in _snapshots.Where(snapshot =>
            snapshot.Document.ProcessId == _history!.ProcessId && snapshot.Document.ProcessStartTime == _history.ProcessStartTime &&
            snapshot.CounterTimestamp >= samples[0].CounterTimestamp))
        {
            double x = X(snapshot.CounterTimestamp);
            if (x < 0 || x > width) { continue; }
            canvas.Children.Add(new Line
            {
                X1 = x, X2 = x, Y1 = 0, Y2 = height, Stroke = secondary,
                StrokeThickness = ReferenceEquals(snapshot, _selected) ? 2 : 1,
                StrokeDashArray = new DoubleCollection { 1, 3 },
            });
        }
    }

    private string Bytes(double? bytes) => bytes is { } value
        ? Format("StatisticsMiB", value / (1024 * 1024)) : Text("StatisticsUnavailable");
    private string Format(string key, params object[] values) => string.Format(CultureInfo.CurrentCulture, Text(key), values);
    private string Text(string key) => _resources.GetString(key);
}

using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Windows.Foundation;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Metrics;

namespace WinUIProfiler.App.Views;

public sealed partial class MetricChart : UserControl
{
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private IReadOnlyList<MetricSeries> series = [];
    private bool percent;
    private long frequency = Stopwatch.Frequency;
    private long totalSamples;
    private long firstTimestampQpc;
    public MetricChart()
    {
        InitializeComponent();
        ActualThemeChanged += Chart_ActualThemeChanged;
    }

    protected override AutomationPeer OnCreateAutomationPeer() => new FrameworkElementAutomationPeer(this);
    public void ShowSeries(MetricHistory? history, bool isPercent, long qpcFrequency)
    {
        percent = isPercent;
        frequency = qpcFrequency;
        totalSamples = history?.TotalSamples ?? 0;
        firstTimestampQpc = history?.FirstTimestampQpc ?? 0;
        series = history?.Snapshot().Where(item => item.IsPercent == percent).ToArray() ?? [];
        Heading.Text = Text(percent ? "UtilizationHeading" : "MemoryHeading");
        AutomationProperties.SetName(this, Text(percent ? "UtilizationAutomation" : "MemoryAutomation"));
        Render();
    }

    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] arguments) => string.Format(CultureInfo.CurrentCulture, Text(key), arguments);
    private void Chart_ActualThemeChanged(FrameworkElement sender, object args) => Render();
    private void Plot_SizeChanged(object sender, SizeChangedEventArgs args) => Render();
    private void Render()
    {
        if (Plot is null)
        {
            return;
        }

        var width = Plot.ActualWidth;
        var height = Plot.ActualHeight;
        var layout = MetricChartLayout.Create(series, percent, firstTimestampQpc, frequency);
        AxisScale.Text = Format("ChartScale", layout.Maximum.ToString("G4", CultureInfo.CurrentCulture), percent ? "%" : "MiB");
        var elapsed = (layout.StartQpc - firstTimestampQpc) / (double)frequency;
        WindowStart.Text = Format("ChartTime", elapsed.ToString("N0", CultureInfo.CurrentCulture));
        WindowEnd.Text = Format("ChartTime", (elapsed + MetricHistory.WindowSeconds).ToString("N0", CultureInfo.CurrentCulture));
        var presentation = new List<MetricChartSeries>();
        for (var index = 0; index < series.Count; index++)
        {
            var item = series[index];
            var current = item.Points.LastOrDefault();
            var valueText = current?.Value is { } value ? Format("ChartValue", value.ToString(percent ? "G4" : "N1", CultureInfo.CurrentCulture), percent ? "%" : "MiB") : Text("Unavailable");
            var label = Format("ChartLegend",
Text(item.Metric.ToString()),
valueText,
current?.Source ?? Text("Waiting"),
current?.UnavailableReason ?? "");
            var brush = (Brush)Resources[$"SeriesBrush{index}"];
            var dashes = index == 0 ? new DoubleCollection() : new DoubleCollection
            {
                index * 3,
                2
            };
            presentation.Add(new(label, brush, dashes, Geometry(item, layout, width, height)));
        }

        Paths.ItemsSource = presentation;
        Legend.ItemsSource = presentation;
        EmptyState.Text = Text("Waiting");
        EmptyState.Visibility = series.Count == 0 ? Visibility.Visible : Visibility.Collapsed;
        AutomationProperties.SetItemStatus(this,
JsonSerializer.Serialize(new
{
    totalSamples,
    maximum = layout.Maximum,
    windowStartQpc = layout.StartQpc,
    windowDurationQpc = layout.WindowQpc,
    unit = percent ? "Percent" : "MiB",
    series = series.Select(item => new { metric = item.Metric.ToString(), points = item.Points.Count, firstQpc = item.Points.FirstOrDefault()?.TimestampQpc, lastQpc = item.Points.LastOrDefault()?.TimestampQpc, firstX = layout.GetX(item.Points.FirstOrDefault()?.TimestampQpc ?? firstTimestampQpc, 1), lastX = layout.GetX(item.Points.LastOrDefault()?.TimestampQpc ?? firstTimestampQpc, 1), value = item.Points.LastOrDefault()?.Value, source = item.Points.LastOrDefault()?.Source, reason = item.Points.LastOrDefault()?.UnavailableReason, gaps = item.Points.Count(point => point.Value is null) })
}));
        AutomationProperties.SetHelpText(this, string.Join("; ", presentation.Select(item => item.Label)));
    }

    private PathGeometry Geometry(MetricSeries item, MetricChartLayout layout, double width, double height)
    {
        var geometry = new PathGeometry();
        PathFigure? figure = null;
        long? previous = null;
        foreach (var point in item.Points)
        {
            if (point.Value is not { } number || width <= 0 || height <= 0)
            {
                figure = null;
                previous = null;
                continue;
            }

            var position = new Point(layout.GetX(point.TimestampQpc, width), height - Math.Clamp(number / layout.Maximum, 0, 1) * height);
            if (figure is null ||
previous.HasValue &&
point.TimestampQpc - previous.Value > frequency * MetricHistory.SamplingInterval.TotalSeconds * 3)
            {
                figure = new PathFigure
                {
                    StartPoint = position,
                    IsClosed = false,
                    IsFilled = false
                };
                geometry.Figures.Add(figure);
            }
            else
            {
                figure.Segments.Add(new LineSegment { Point = position });
            }

            previous = point.TimestampQpc;
        }

        return geometry;
    }
}

using System.Globalization;
using System.Text.Json;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.ViewManagement;
using WinUIProfiler.Engine.Metrics;

namespace WinUIProfiler.App.Views;

public sealed partial class MetricChart : UserControl
{
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly AccessibilitySettings accessibility = new();
    private IReadOnlyList<MetricSeries> series = [];
    private bool percent;
    private long frequency, totalSamples;
    private static readonly Color[] Colors = [Color.FromArgb(255, 0, 120, 212),
        Color.FromArgb(255, 179, 91, 0), Color.FromArgb(255, 16, 124, 16)];

    public MetricChart()
    {
        InitializeComponent();
        ActualThemeChanged += (_, _) => Render();
    }

    protected override AutomationPeer OnCreateAutomationPeer() => new FrameworkElementAutomationPeer(this);

    public void ShowSeries(MetricHistory? history, bool isPercent, long qpcFrequency)
    {
        percent = isPercent;
        frequency = qpcFrequency;
        totalSamples = history?.TotalSamples ?? 0;
        series = history?.Snapshot().Where(item => item.IsPercent == percent).ToArray() ?? [];
        Heading.Text = Text(percent ? "UtilizationHeading" : "MemoryHeading");
        AutomationProperties.SetName(this, Text(percent ? "UtilizationAutomation" : "MemoryAutomation"));
        Render();
    }

    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] arguments) =>
        string.Format(CultureInfo.CurrentCulture, Text(key), arguments);

    private void Plot_SizeChanged(object sender, SizeChangedEventArgs args) => Render();

    private void Render()
    {
        if (Plot is null) return;
        Plot.Children.Clear();
        Legend.Children.Clear();
        var width = Plot.ActualWidth;
        var height = Plot.ActualHeight;
        var maximum = percent ? 100d : Math.Max(64, Math.Ceiling(series.SelectMany(item => item.Points)
            .Select(point => point.Value ?? 0).DefaultIfEmpty(0).Max() * 1.1 / 64) * 64);
        AxisScale.Text = Format("ChartScale", maximum.ToString("N0", CultureInfo.CurrentCulture), percent ? "%" : "MiB");
        var latest = series.SelectMany(item => item.Points).Select(point => point.TimestampQpc).DefaultIfEmpty(0).Max();
        var window = frequency > 0 ? frequency * 120d : 1;
        var foreground = Heading.Foreground;
        for (var row = 0; row <= 4 && width > 0 && height > 0; row++)
            Plot.Children.Add(new Line { X1 = 0, X2 = width, Y1 = row * height / 4, Y2 = row * height / 4,
                Stroke = foreground, Opacity = accessibility.HighContrast ? 0.7 : 0.15, StrokeThickness = 1 });
        for (var index = 0; index < series.Count; index++)
        {
            var item = series[index];
            var brush = accessibility.HighContrast ? foreground : new SolidColorBrush(Colors[index % Colors.Length]);
            var current = item.Points.LastOrDefault();
            var valueText = current?.Value is { } value
                ? Format("ChartValue", value.ToString("N1", CultureInfo.CurrentCulture), percent ? "%" : "MiB")
                : Text("Unavailable");
            Legend.Children.Add(new TextBlock
            {
                Text = Format("ChartLegend", Text(item.Metric.ToString()), valueText, current?.Source ?? Text("Waiting"),
                    current?.UnavailableReason ?? ""),
                Foreground = brush, TextWrapping = TextWrapping.Wrap,
                FontSize = 12
            });
            Polyline? line = null;
            long? previous = null;
            foreach (var point in item.Points)
            {
                if (point.Value is not { } number || width <= 0 || height <= 0)
                {
                    line = null;
                    previous = null;
                    continue;
                }
                if (line is null || previous.HasValue && point.TimestampQpc - previous.Value > frequency * 3)
                {
                    line = new() { Stroke = brush, StrokeThickness = 2 };
                    if (index > 0) line.StrokeDashArray = new DoubleCollection { index * 3, 2 };
                    Plot.Children.Add(line);
                }
                line.Points.Add(new Point(Math.Clamp(1 - (latest - point.TimestampQpc) / window, 0, 1) * width,
                    height - Math.Clamp(number / maximum, 0, 1) * height));
                previous = point.TimestampQpc;
            }
        }
        if (series.Count == 0) Legend.Children.Add(new TextBlock { Text = Text("Waiting"), TextWrapping = TextWrapping.Wrap });
        var summary = JsonSerializer.Serialize(new
        {
            totalSamples,
            unit = percent ? "Percent" : "MiB",
            series = series.Select(item => new
            {
                metric = item.Metric.ToString(), points = item.Points.Count,
                firstQpc = item.Points.FirstOrDefault()?.TimestampQpc, lastQpc = item.Points.LastOrDefault()?.TimestampQpc,
                value = item.Points.LastOrDefault()?.Value, source = item.Points.LastOrDefault()?.Source,
                reason = item.Points.LastOrDefault()?.UnavailableReason,
                gaps = item.Points.Count(point => point.Value is null)
            })
        });
        AutomationProperties.SetItemStatus(this, summary);
        AutomationProperties.SetHelpText(this, string.Join("; ", Legend.Children.OfType<TextBlock>().Select(text => text.Text)));
    }
}

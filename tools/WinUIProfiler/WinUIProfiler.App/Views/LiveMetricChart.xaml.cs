using System.Globalization;
using System.Text.Json;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.Foundation;
using WinUIProfiler.App.ViewModels;

namespace WinUIProfiler.App.Views;

public sealed partial class LiveMetricChart : UserControl
{
    public static readonly DependencyProperty TitleProperty = DependencyProperty.Register(
        nameof(Title), typeof(string), typeof(LiveMetricChart), new PropertyMetadata(""));
    public string Title { get => (string)GetValue(TitleProperty); set => SetValue(TitleProperty, value); }
    private LiveMetricSeries? series;
    private LiveMetricGraphs? model;

    public LiveMetricChart()
    {
        InitializeComponent();
        ActualThemeChanged += (_, _) => Render();
    }

    protected override AutomationPeer OnCreateAutomationPeer() => new FrameworkElementAutomationPeer(this);

    public void ShowSeries(LiveMetricSeries value, LiveMetricGraphs graphs)
    {
        series = value;
        model = graphs;
        Render();
    }

    private void Plot_SizeChanged(object sender, SizeChangedEventArgs args) => Render();

    private void Render()
    {
        if (series is null || model is null) return;
        var unit = series.IsPercent ? "%" : "MiB";
        CurrentValue.Text = series.Current?.Value is { } value
            ? value.ToString("N1", CultureInfo.CurrentCulture) + " " + unit
            : series.Current is null ? WaitingLabel.Text : UnavailableLabel.Text;
        MaximumValue.Text = "0 - " + series.Maximum.ToString("N1", CultureInfo.CurrentCulture) + " " + unit;
        SourceStatus.Text = series.Current is { } current
            ? $"{current.Sample.Source} | {current.Sample.Value.Quality.Kind}/{current.Sample.Value.Quality.Coverage}" +
                (current.Sample.Value.Quality.Reason is { } reason ? " | " + reason : "")
            : WaitingLabel.Text;
        ToolTipService.SetToolTip(this, SourceStatus.Text);
        Plot.Children.Clear();
        var vertices = 0;
        if (Plot.ActualWidth > 0 && Plot.ActualHeight > 0 && model.EndQpc > model.StartQpc)
        {
            Plot.Clip = new RectangleGeometry { Rect = new(0, 0, Plot.ActualWidth, Plot.ActualHeight) };
            var brush = (Brush)Application.Current.Resources["AccentFillColorDefaultBrush"];
            foreach (var segment in series.Segments)
            {
                var points = new PointCollection();
                foreach (var point in segment)
                {
                    points.Add(new Point(
                        (point.TimestampQpc - model.StartQpc) / (double)(model.EndQpc - model.StartQpc) * Plot.ActualWidth,
                        (1 - point.Value!.Value / series.Maximum) * (Plot.ActualHeight - 4) + 2));
                    vertices++;
                }
                if (points.Count == 1)
                {
                    var marker = new Ellipse { Width = 4, Height = 4, Fill = brush };
                    Canvas.SetLeft(marker, points[0].X - 2);
                    Canvas.SetTop(marker, points[0].Y - 2);
                    Plot.Children.Add(marker);
                }
                else Plot.Children.Add(new Polyline { Points = points, Stroke = brush, StrokeThickness = 2 });
            }
        }
        AutomationProperties.SetItemStatus(this, JsonSerializer.Serialize(new
        {
            metric = series.Metric.ToString(),
            sessionId = model.Scope?.SessionId,
            processId = model.Scope?.Process.ProcessId,
            watermark = model.Watermark.IngestOrdinal,
            points = series.Points.Count,
            measured = series.Points.Count(point => point.Value is not null),
            latestQpc = series.Current?.TimestampQpc,
            value = series.Current?.Value,
            rawValue = series.Current?.Sample.Value.Integer,
            unit,
            source = series.Current?.Sample.Source,
            quality = series.Current?.Sample.Value.Quality,
            vertices
        }));
    }
}

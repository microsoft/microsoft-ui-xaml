using Microsoft.UI.Xaml.Media;

namespace WinUIProfiler.App.Views;

public sealed record MetricChartSeries(string Label, Brush Stroke, DoubleCollection Dashes, PathGeometry Geometry);

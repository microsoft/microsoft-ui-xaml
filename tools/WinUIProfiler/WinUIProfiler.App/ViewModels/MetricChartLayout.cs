using WinUIProfiler.Engine.Metrics;

namespace WinUIProfiler.App.ViewModels;

internal readonly record struct MetricChartLayout(double Maximum, long StartQpc, long WindowQpc)
{
    public static MetricChartLayout Create(IReadOnlyList<MetricSeries> series, bool percent, long firstTimestampQpc, long qpcFrequency)
    {
        ArgumentNullException.ThrowIfNull(series);
        ArgumentOutOfRangeException.ThrowIfNegative(firstTimestampQpc);
        if (qpcFrequency is <= 0 or > long.MaxValue / MetricHistory.WindowSeconds)
        {
            throw new ArgumentOutOfRangeException(nameof(qpcFrequency));
        }

        var points = series.SelectMany(item => item.Points).ToArray();
        var observed = points.Select(point => point.Value ?? 0).DefaultIfEmpty(0).Max();
        var maximum = percent ? observed > 0 ? Math.Min(100, 10 * observed) : 1 : Math.Max(64, Math.Ceiling(observed * 1.1 / 64) * 64);
        var latest = points.Select(point => point.TimestampQpc).DefaultIfEmpty(firstTimestampQpc).Max();
        var window = qpcFrequency * MetricHistory.WindowSeconds;
        return new(maximum, Math.Max(firstTimestampQpc, latest - window), window);
    }

    public double GetX(long timestampQpc, double width)
    {
        return Math.Clamp((timestampQpc - StartQpc) / (double)WindowQpc, 0, 1) * width;
    }
}

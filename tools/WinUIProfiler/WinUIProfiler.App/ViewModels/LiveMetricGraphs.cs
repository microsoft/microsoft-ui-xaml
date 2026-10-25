using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.App.ViewModels;

public sealed record LiveMetricPoint(long TimestampQpc, double? Value, MetricSample Sample);

public sealed class LiveMetricSeries
{
    public MetricKind Metric { get; }
    public IReadOnlyList<LiveMetricPoint> Points { get; }
    public IReadOnlyList<IReadOnlyList<LiveMetricPoint>> Segments { get; }
    public bool IsPercent => Metric is MetricKind.CpuUtilization or MetricKind.GpuUtilization;
    public double Maximum { get; }
    public LiveMetricPoint? Current => Points.Count == 0 ? null : Points[^1];

    internal LiveMetricSeries(MetricKind metric, IEnumerable<MetricSample> samples, long frequency)
    {
        Metric = metric;
        Points = Array.AsReadOnly(samples.Select(sample => new LiveMetricPoint(sample.Interval.EndQpc,
            sample.Value.Kind == ValueKind.Missing ? null :
                IsPercent ? sample.Value.Number ?? sample.Value.Integer :
                    (sample.Value.Number ?? sample.Value.Integer) / 1048576d, sample)).ToArray());
        var segments = new List<IReadOnlyList<LiveMetricPoint>>();
        var current = new List<LiveMetricPoint>();
        long? previous = null;
        foreach (var point in Points)
        {
            if (point.Value is null || previous is { } prior && point.TimestampQpc - prior > frequency * 3)
            {
                if (current.Count > 0) segments.Add(Array.AsReadOnly(current.ToArray()));
                current.Clear();
            }
            if (point.Value is not null) current.Add(point);
            previous = point.TimestampQpc;
        }
        if (current.Count > 0) segments.Add(Array.AsReadOnly(current.ToArray()));
        Segments = segments.AsReadOnly();
        var highest = Points.Max(point => point.Value) ?? 0;
        Maximum = IsPercent ? 100 : Math.Max(1, Math.Ceiling(highest / 16) * 16);
    }
}

public sealed class LiveMetricGraphs
{
    public const int MaximumPoints = 121;
    private static readonly MetricKind[] Kinds = [MetricKind.CpuUtilization, MetricKind.GpuUtilization,
        MetricKind.ProcessWorkingSet, MetricKind.ProcessPrivateWorkingSet, MetricKind.ProcessPrivateCommit];
    public event EventHandler? Changed;
    public SessionScope? Scope { get; private set; }
    public CommittedWatermark Watermark { get; private set; }
    public long Frequency { get; private set; }
    public long StartQpc { get; private set; }
    public long EndQpc => Watermark.TimestampQpc;
    public IReadOnlyList<LiveMetricSeries> Series { get; private set; } = Empty();

    public void Reset()
    {
        Scope = null;
        Watermark = default;
        Frequency = StartQpc = 0;
        Series = Empty();
        Changed?.Invoke(this, EventArgs.Empty);
    }

    public bool Apply(QueryPage<MetricSample> page, long frequency)
    {
        ArgumentNullException.ThrowIfNull(page);
        if (frequency <= 0 || frequency > long.MaxValue / 120)
            throw new ArgumentOutOfRangeException(nameof(frequency));
        if (page.Page.HasMore) throw new ArgumentException("A complete bounded metric window is required.", nameof(page));
        if (Scope is { } scope && (scope != page.Context.Scope || Frequency != frequency))
            throw new ArgumentException("Reset the graphs before selecting another session or clock.", nameof(page));
        if (Scope is not null && (page.Context.Watermark.IngestOrdinal < Watermark.IngestOrdinal ||
            page.Context.Watermark.TimestampQpc < Watermark.TimestampQpc))
            throw new ArgumentException("A live graph cannot move behind its committed boundary.", nameof(page));
        if (Scope is not null && page.Context.Watermark == Watermark) return false;
        var start = Math.Max(0, page.Context.Watermark.TimestampQpc - frequency * 120);
        var groups = page.Items.Where(sample => MetricProcessor.IsProcessGraphMetric(sample.Metric, sample.Gpu) &&
            sample.Interval.EndQpc >= start).ToLookup(sample => sample.Metric);
        var series = Kinds.Select(kind => new LiveMetricSeries(kind, groups[kind]
            .OrderBy(sample => sample.Interval.EndQpc).TakeLast(MaximumPoints), frequency)).ToArray();
        Scope = page.Context.Scope;
        Watermark = page.Context.Watermark;
        Frequency = frequency;
        StartQpc = start;
        Series = Array.AsReadOnly(series);
        Changed?.Invoke(this, EventArgs.Empty);
        return true;
    }

    private static IReadOnlyList<LiveMetricSeries> Empty() =>
        Array.AsReadOnly(Kinds.Select(kind => new LiveMetricSeries(kind, [], 1)).ToArray());
}

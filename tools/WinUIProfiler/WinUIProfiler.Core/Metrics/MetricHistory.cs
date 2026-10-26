using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Metrics;

public enum ProcessMetric { Cpu, Gpu, WorkingSet, PrivateWorkingSet, PrivateCommit }
public sealed record MetricValue(ProcessMetric Metric, double? Value, string Source, string? UnavailableReason);
public sealed record MetricReading(ProcessInstanceKey Process, long TimestampQpc, IReadOnlyList<MetricValue> Values);
public sealed record MetricPoint(long TimestampQpc, double? Value, string Source, string? UnavailableReason);
public sealed record MetricSeries(ProcessMetric Metric, bool IsPercent, IReadOnlyList<MetricPoint> Points);

public sealed class MetricHistory
{
    private readonly object gate = new();
    private readonly ProcessInstanceKey process;
    private readonly long windowQpc;
    private readonly Dictionary<ProcessMetric, Queue<MetricPoint>> points =
        Enum.GetValues<ProcessMetric>().ToDictionary(metric => metric, _ => new Queue<MetricPoint>());
    private long totalSamples, lastQpc = -1;

    public MetricHistory(ProcessInstanceKey process, long qpcFrequency)
    {
        process.Validate();
        if (qpcFrequency is <= 0 or > long.MaxValue / 120)
            throw new ArgumentOutOfRangeException(nameof(qpcFrequency));
        this.process = process;
        windowQpc = qpcFrequency * 120;
    }

    public long TotalSamples { get { lock (gate) return totalSamples; } }

    public void Add(MetricReading reading)
    {
        ArgumentNullException.ThrowIfNull(reading);
        if (reading.Process != process) throw new ArgumentException("Metric sample belongs to another process instance.");
        if (reading.TimestampQpc < 0) throw new ArgumentException("Metric timestamp must be nonnegative.");
        if (reading.Values is null || reading.Values.Count != 5)
            throw new ArgumentException("Each acquisition requires all five explicit metric values.");
        var values = reading.Values.ToArray();
        if (values.Any(value => value is null) || values.Select(value => value.Metric).Distinct().Count() != 5)
            throw new ArgumentException("Metric series must be unique.");
        foreach (var value in values)
        {
            Require.Enum(value.Metric);
            Require.Text(value.Source);
            if (value.Value is { } number)
            {
                if (!double.IsFinite(number) || number < 0 || value.Metric <= ProcessMetric.Gpu && number > 100)
                    throw new ArgumentException("Metric value is outside its valid range.");
            }
            else Require.Text(value.UnavailableReason);
        }
        lock (gate)
        {
            if (reading.TimestampQpc < lastQpc) throw new ArgumentException("Metric timestamps must be ordered.");
            foreach (var value in values)
            {
                var series = points[value.Metric];
                series.Enqueue(new(reading.TimestampQpc, value.Value, value.Source, value.UnavailableReason));
                while (series.Count > 121 || series.Count > 0 && series.Peek().TimestampQpc < reading.TimestampQpc - windowQpc)
                    series.Dequeue();
            }
            lastQpc = reading.TimestampQpc;
            totalSamples++;
        }
    }

    public IReadOnlyList<MetricSeries> Snapshot()
    {
        lock (gate)
            return points.Select(pair => new MetricSeries(pair.Key, pair.Key <= ProcessMetric.Gpu,
                Array.AsReadOnly(pair.Value.ToArray()))).ToArray();
    }
}

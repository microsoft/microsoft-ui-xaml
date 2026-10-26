using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Metrics;
using Xunit;

namespace WinUIProfiler.Core.Tests;

public class MetricConversionTests
{
    [Fact]
    public void Memory_uses_MiB_and_preserves_three_distinct_metrics()
    {
        foreach (var metric in new[] { ProcessMetric.WorkingSet, ProcessMetric.PrivateWorkingSet, ProcessMetric.PrivateCommit })
        {
            var result = ProcessMetricSession.ToValue(metric,
                new(ValueKind.Integer, ValueUnit.Bytes, 1572864, null, null, null, DataQuality.Complete), "Native memory");
            Assert.Equal(1.5, result.Value);
            Assert.Equal(metric, result.Metric);
        }
    }

    [Fact]
    public void Warmup_is_not_a_zero_percent_measurement()
    {
        var result = ProcessMetricSession.ToValue(ProcessMetric.Cpu,
            new(ValueKind.Missing, ValueUnit.Percent, null, null, null, null,
                new(MeasurementKind.Unavailable, CoverageState.Unavailable, "Warmup")), "Native CPU");
        Assert.Null(result.Value);
        Assert.Equal("Warmup", result.UnavailableReason);
    }

    [Fact]
    public void Unknown_metric_is_rejected() =>
        Assert.Throws<ArgumentOutOfRangeException>(() => ProcessMetricSession.ToValue((ProcessMetric)(-1),
            new(ValueKind.Number, ValueUnit.Percent, null, 25, null, null, DataQuality.Complete), "CPU"));
}

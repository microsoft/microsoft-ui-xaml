using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Metrics;
using Xunit;

namespace WinUIProfiler.Core.Tests;

public class MetricHistoryTests
{
    private static readonly ProcessInstanceKey Process = new(42, 123, Guid.NewGuid());
    private static MetricReading Reading(long time, double? cpu = 20, ProcessInstanceKey? process = null) =>
        new(process ?? Process, time, [
            new(ProcessMetric.Cpu, cpu, "CPU source", cpu is null ? "Warmup" : null),
            new(ProcessMetric.Gpu, 30, "GPU source", null),
            new(ProcessMetric.WorkingSet, 100, "Memory source", null),
            new(ProcessMetric.PrivateWorkingSet, 60, "Memory source", null),
            new(ProcessMetric.PrivateCommit, 150, "Memory source", null)]);

    [Fact]
    public void Has_two_panels_with_five_distinct_series()
    {
        var history = new MetricHistory(Process, 1000);
        history.Add(Reading(1000));
        var series = history.Snapshot();
        Assert.Equal(5, series.Count);
        Assert.Equal(2, series.Count(s => s.IsPercent));
        Assert.Equal(3, series.Count(s => !s.IsPercent));
        Assert.Equal(20, series.Single(s => s.Metric == ProcessMetric.Cpu).Points.Single().Value);
    }

    [Fact]
    public void History_is_bounded_by_point_count()
    {
        var history = new MetricHistory(Process, 1000);
        for (var i = 0; i < 300; i++) history.Add(Reading(i));
        Assert.All(history.Snapshot(), s => Assert.Equal(121, s.Points.Count));
        Assert.Equal(300, history.TotalSamples);
    }

    [Fact]
    public void History_is_bounded_by_two_minutes()
    {
        var history = new MetricHistory(Process, 1000);
        history.Add(Reading(1000));
        history.Add(Reading(122001));
        Assert.All(history.Snapshot(), s => Assert.Single(s.Points));
    }

    [Fact]
    public void Missing_values_are_gaps_not_zero()
    {
        var history = new MetricHistory(Process, 1000);
        history.Add(Reading(1000, null));
        history.Add(Reading(2000, 0));
        var points = history.Snapshot().Single(s => s.Metric == ProcessMetric.Cpu).Points;
        Assert.Null(points[0].Value);
        Assert.Equal("Warmup", points[0].UnavailableReason);
        Assert.Equal(0, points[1].Value);
    }

    [Fact]
    public void Foreign_process_and_out_of_order_samples_are_rejected()
    {
        var history = new MetricHistory(Process, 1000);
        history.Add(Reading(1000));
        Assert.Throws<ArgumentException>(() => history.Add(Reading(1001, process: new(43, 123, Guid.NewGuid()))));
        Assert.Throws<ArgumentException>(() => history.Add(Reading(999)));
        Assert.Equal(1, history.TotalSamples);
    }

    [Fact]
    public void Snapshot_does_not_mutate_when_capture_advances()
    {
        var history = new MetricHistory(Process, 1000);
        history.Add(Reading(1000));
        var saved = history.Snapshot();
        history.Add(Reading(2000));
        Assert.All(saved, s => Assert.Single(s.Points));
        Assert.All(history.Snapshot(), s => Assert.Equal(2, s.Points.Count));
    }
}

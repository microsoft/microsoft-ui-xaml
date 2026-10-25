using WinUIProfiler.Contracts;
using WinUIProfiler.App.ViewModels;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class LiveMetricGraphTests
{
    [Fact]
    public void Graph_window_is_bounded_and_missing_CPU_remains_a_gap()
    {
        var model = new LiveMetricGraphs();
        var scope = new SessionScope(Guid.NewGuid(), new(42, 100, Guid.NewGuid()));
        var samples = Enumerable.Range(0, 301).Select(i => new MetricSample(i * 1000,
            MetricKind.CpuUtilization, i == 300
                ? new(ValueKind.Missing, ValueUnit.Percent, null, null, null, null,
                    new(MeasurementKind.Unavailable, CoverageState.Unavailable, "Counter reset."))
                : new(ValueKind.Number, ValueUnit.Percent, null, 25, null, null, DataQuality.Complete),
            new(i * 1000, i * 1000), "synthetic", "1")).ToArray();
        var page = new QueryPage<MetricSample>(new(scope, new(301, 300000), DataQuality.Complete),
            samples, new(512, 0, null, false));
        model.Apply(page, 1000);
        Assert.Equal(5, model.Series.Count);
        var cpu = model.Series.Single(series => series.Metric == MetricKind.CpuUtilization);
        Assert.Equal(121, cpu.Points.Count);
        Assert.Null(cpu.Points[^1].Value);
        Assert.Equal(25d, cpu.Points[0].Value);
        Assert.Equal(100, cpu.Maximum);
    }

    [Fact]
    public void Memory_is_MiB_but_retains_the_original_byte_measurement()
    {
        var model = new LiveMetricGraphs();
        model.Apply(Page([Sample(MetricKind.ProcessPrivateCommit, 1000, 67108864)]), 1000);
        var memory = model.Series.Single(series => series.Metric == MetricKind.ProcessPrivateCommit);
        Assert.Equal(64, memory.Current!.Value);
        Assert.Equal(67108864, memory.Current.Sample.Value.Integer);
        Assert.True(memory.Maximum >= 64);
    }

    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public void Missing_samples_and_long_collection_pauses_break_polylines(bool missing)
    {
        var model = new LiveMetricGraphs();
        var samples = missing
            ? new[] { Sample(MetricKind.CpuUtilization, 1000), Sample(MetricKind.CpuUtilization, 2000, null),
                Sample(MetricKind.CpuUtilization, 3000) }
            : new[] { Sample(MetricKind.CpuUtilization, 1000), Sample(MetricKind.CpuUtilization, 5000) };
        model.Apply(Page(samples), 1000);
        var cpu = model.Series[0];
        Assert.Equal(2, cpu.Segments.Count);
        Assert.All(cpu.Segments, segment => Assert.Single(segment));
    }

    [Fact]
    public void GPU_uses_overall_busiest_engine_only()
    {
        var overall = Sample(MetricKind.GpuUtilization, 1000, 25) with
            { Gpu = new(new(123, 0, 0), GpuMetricScope.OverallBusiestEngine, new(1, GpuEngineClass.ThreeD)) };
        var engine = Sample(MetricKind.GpuUtilization, 1001, 80) with
            { Gpu = new(new(123, 0, 0), GpuMetricScope.PerEngine, new(2, GpuEngineClass.Copy)) };
        var model = new LiveMetricGraphs();
        model.Apply(Page([overall, engine]), 1000);
        var gpu = model.Series[1];
        Assert.Single(gpu.Points);
        Assert.Equal(25, gpu.Current!.Value);
    }

    [Fact]
    public void Refresh_replaces_the_window_and_reconnect_requires_reset()
    {
        var model = new LiveMetricGraphs();
        var first = Page([Sample(MetricKind.CpuUtilization, 1000)]);
        Assert.True(model.Apply(first, 1000));
        Assert.False(model.Apply(first, 1000));
        var second = Page([Sample(MetricKind.CpuUtilization, 1000), Sample(MetricKind.CpuUtilization, 2000)],
            first.Context.Scope);
        model.Apply(second, 1000);
        Assert.Equal(2, model.Series[0].Points.Count);
        Assert.Throws<ArgumentException>(() => model.Apply(first, 1000));
        Assert.Throws<ArgumentException>(() => model.Apply(second, 1001));
        var another = Page([Sample(MetricKind.CpuUtilization, 3000)]);
        Assert.Throws<ArgumentException>(() => model.Apply(another, 1000));
        model.Reset();
        Assert.Null(model.Scope);
        Assert.All(model.Series, series => Assert.Empty(series.Points));
        model.Apply(another, 1000);
        Assert.Single(model.Series[0].Points);
        Assert.Equal(another.Context.Scope, model.Scope);
    }

    [Theory]
    [InlineData(MetricKind.CpuUtilization)]
    [InlineData(MetricKind.GpuUtilization)]
    [InlineData(MetricKind.ProcessWorkingSet)]
    [InlineData(MetricKind.ProcessPrivateWorkingSet)]
    [InlineData(MetricKind.ProcessPrivateCommit)]
    public void Unavailable_is_a_gap_not_zero(MetricKind kind)
    {
        var model = new LiveMetricGraphs();
        model.Apply(Page([Sample(kind, 1000, null)]), 1000);
        var series = model.Series.Single(series => series.Metric == kind);
        Assert.Null(series.Current!.Value);
        Assert.Empty(series.Segments);
        Assert.Equal("Counter unavailable.", series.Current.Sample.Value.Quality.Reason);
    }

    private static MetricSample Sample(MetricKind kind, long timestamp, long? value = 25)
    {
        var percent = kind is MetricKind.CpuUtilization or MetricKind.GpuUtilization;
        var unit = percent ? ValueUnit.Percent : ValueUnit.Bytes;
        var typed = value is null
            ? new TypedValue(ValueKind.Missing, unit, null, null, null, null,
                new(MeasurementKind.Unavailable, CoverageState.Unavailable, "Counter unavailable."))
            : percent ? new(ValueKind.Number, unit, null, value, null, null, DataQuality.Complete)
                : new(ValueKind.Integer, unit, value, null, null, null, DataQuality.Complete);
        return new(timestamp, kind, typed, new(timestamp, timestamp), "synthetic", "1");
    }

    private static QueryPage<MetricSample> Page(MetricSample[] samples, SessionScope? scope = null) =>
        new(new(scope ?? new(Guid.NewGuid(), new(42, 100, Guid.NewGuid())),
            new(samples.Length, samples.Max(sample => sample.TimestampQpc)), DataQuality.Complete),
            samples, new(512, 0, null, false));
}

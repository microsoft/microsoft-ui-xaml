using System.Reflection;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class LiveMetricTests
{
    [Fact]
    public void Process_only_projection_keeps_five_metrics_and_excludes_per_engine_GPU()
    {
        using var fixture = new StoreFixture();
        var processor = new MetricProcessor(processOnly: true);
        var metrics = new[] { MetricKind.CpuUtilization, MetricKind.GpuUtilization,
            MetricKind.ProcessWorkingSet, MetricKind.ProcessPrivateWorkingSet,
            MetricKind.ProcessPrivateCommit, MetricKind.SystemCpuUtilization };
        var sources = metrics.Select((kind, index) => Source(fixture, kind, index + 1)).ToList();
        sources.Add(Source(fixture, MetricKind.GpuUtilization, 7,
            new(new(123, 0, 0), GpuMetricScope.PerEngine, new(1, GpuEngineClass.ThreeD))));
        var rows = processor.Process(sources, new(fixture.Scope, new(7, 70),
            StoreFixture.Partial, new(1024 * 1024, 100))).ToArray();
        Assert.Equal(5, rows.Length);
        Assert.Equal(metrics.Take(5), rows.Select(row => Assert.IsType<MetricSample>(row.Value).Metric));
        Assert.DoesNotContain(rows, row => ((MetricSample)row.Value).Gpu?.Scope == GpuMetricScope.PerEngine);
    }

    [Fact]
    public void Collector_header_admission_does_not_require_header_PID_to_be_the_target()
    {
        bool Accept(Guid provider, int producer, int target) =>
            BufferedRuntimeEvents.AcceptsHeader(provider, producer, target);
        Assert.True(Accept(EventCatalog.CollectorProviderId, Environment.ProcessId, 42));
        Assert.False(Accept(EventCatalog.CollectorProviderId, 43, 42));
        Assert.True(Accept(EventCatalog.StandardXamlProviderId, 42, 42));
        Assert.False(Accept(EventCatalog.StandardXamlProviderId, 43, 42));
        Assert.False(Accept(EventCatalog.RuntimeProviderId, Environment.ProcessId, 42));
        Assert.False(Accept(Guid.NewGuid(), 42, 42));
    }

    private static NormalizedEvent Source(StoreFixture fixture, MetricKind kind, int ordinal,
        GpuSampleIdentity? gpu = null)
    {
        var original = fixture.Batch(ordinal, ordinal).Sources[0];
        var percent = kind is MetricKind.CpuUtilization or MetricKind.GpuUtilization or MetricKind.SystemCpuUtilization;
        var payload = new MetricPayload(kind, percent
            ? new(ValueKind.Number, ValueUnit.Percent, null, 10, null, null, DataQuality.Complete)
            : new(ValueKind.Integer, ValueUnit.Bytes, 1024, null, null, null, DataQuality.Complete),
            new(ordinal * 10 - 1, ordinal * 10), "synthetic", "1",
            gpu ?? (kind == MetricKind.GpuUtilization ? new(new(123, 0, 0), GpuMetricScope.OverallBusiestEngine, null) : null));
        var e = original.Envelope;
        return EventCodec.Normalize(new(e.SessionId, e.Process, e.ProducerProcessId, e.ThreadId,
            e.TimestampQpc, e.IngestOrdinal, e.SourceSequence, e.ActivityId, e.Identity,
            EventCodec.EncodePayload(payload, fixture.Scope.Process)), original.Stream);
    }
}

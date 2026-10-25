using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Processing;

public sealed class MetricProcessor : IEventProcessor
{
    private readonly bool processOnly;
    public MetricProcessor(bool processOnly = false) => this.processOnly = processOnly;

    public static bool IsProcessGraphMetric(MetricKind metric, GpuSampleIdentity? gpu) =>
        metric is MetricKind.CpuUtilization or MetricKind.ProcessWorkingSet or
            MetricKind.ProcessPrivateWorkingSet or MetricKind.ProcessPrivateCommit ||
        metric == MetricKind.GpuUtilization && (gpu is null || gpu.Scope == GpuMetricScope.OverallBusiestEngine);

    public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
    {
        foreach (var source in events)
        {
            var envelope = source.Envelope;
            if (envelope.Identity.ProviderId != EventCatalog.CollectorProviderId || envelope.Identity.EventId != 2)
                continue;
            var payload = EventCodec.ReadPayload<MetricPayload>(envelope);
            if (processOnly && !IsProcessGraphMetric(payload.Metric, payload.Gpu)) continue;
            yield return new(envelope.IngestOrdinal, new MetricSample(envelope.TimestampQpc, payload.Metric,
                payload.Value, payload.Interval, payload.Source, payload.SourceVersion, payload.Gpu)
            {
                SourceObservation = payload.SourceObservation
            });
        }
    }

    // Metrics carry independent source intervals; transport loss is retained by the owner.
    public void Invalidate(HealthSnapshot health) { }
}

using System.Diagnostics;
using System.Diagnostics.Tracing;
using System.Text;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed class CollectorMetricEmitter
{
    private readonly SessionScope scope;
    private readonly Guid epoch = Guid.NewGuid();
    private readonly object gate = new();
    private ulong sequence;
    private static readonly CollectorEventSource Source;
    private static readonly UTF8Encoding WireText = new(false, true);

    static CollectorMetricEmitter() => Source = new();
    internal static void EnsureRegistered() => _ = Source;

    internal ProducerStreamKey Stream => new(EventCatalog.CollectorProviderId, checked((uint)Environment.ProcessId), epoch);

    internal bool TryEmit(ProcessPayload payload)
    {
        EventValidation.Validate(payload, scope.Process, Stream);
        ValidateText(payload.Quality.Reason);
        if (payload.CreationQpc > Stopwatch.GetTimestamp())
            throw new ArgumentException("Process creation cannot follow its collector observation.", nameof(payload));
        lock (gate)
        {
            if (!Source.IsEnabled(EventLevel.Informational, (EventKeywords)EventKeyword.Capabilities)) return false;
            Source.Write("Process", new EventSourceOptions
            {
                Level = EventLevel.Informational,
                Keywords = (EventKeywords)EventKeyword.Capabilities
            }, new
            {
                SemanticEventId = (ushort)1,
                EventSchemaVersion = (byte)1,
                SessionId = scope.SessionId,
                ProducerEpoch = epoch,
                SourceSequence = checked(++sequence),
                TargetProcessId = scope.Process.ProcessId,
                TargetCreationTimeFileTime = scope.Process.CreationTimeFileTime!.Value,
                TargetInstanceId = scope.Process.InstanceId,
                QpcFrequency = Stopwatch.Frequency,
                Operation = checked((byte)payload.Operation),
                HasCreationQpc = payload.CreationQpc is not null,
                CreationQpc = payload.CreationQpc ?? 0,
                MeasurementKind = checked((byte)payload.Quality.Kind),
                Coverage = checked((byte)payload.Quality.Coverage),
                Reason = payload.Quality.Reason ?? string.Empty
            });
            return true;
        }
    }

    internal bool TryEmit(LossPayload payload)
    {
        ArgumentNullException.ThrowIfNull(payload);
        if (payload.Domains?.Count > 256)
            throw new ArgumentException("Collector loss exceeds its domain bound.", nameof(payload));
        payload = payload with { Domains = payload.Domains?.ToArray() };
        EventValidation.Validate(payload, scope.Process, Stream);
        ValidateText(payload.Reason);
        if (payload.Domains?.Count > 256 || payload.Interval.EndQpc > Stopwatch.GetTimestamp())
            throw new ArgumentException("Collector loss exceeds its domain bound or observation timestamp.", nameof(payload));
        var domains = (payload.Domains ?? []).Select(domain =>
            new { domain.EventId, domain.EventVersion }).ToArray();
        lock (gate)
        {
            if (!Source.IsEnabled(EventLevel.Warning, (EventKeywords)EventKeyword.Capabilities)) return false;
            Source.Write("Loss", new EventSourceOptions
            {
                Level = EventLevel.Warning,
                Keywords = (EventKeywords)EventKeyword.Capabilities
            }, new
            {
                SemanticEventId = (ushort)3,
                EventSchemaVersion = (byte)1,
                SessionId = scope.SessionId,
                ProducerEpoch = epoch,
                SourceSequence = checked(++sequence),
                TargetProcessId = scope.Process.ProcessId,
                TargetCreationTimeFileTime = scope.Process.CreationTimeFileTime!.Value,
                TargetInstanceId = scope.Process.InstanceId,
                QpcFrequency = Stopwatch.Frequency,
                IntervalStartQpc = payload.Interval.StartQpc,
                IntervalEndQpc = payload.Interval.EndQpc,
                HasLostEvents = payload.LostEvents is not null,
                LostEvents = payload.LostEvents ?? 0,
                payload.Reason,
                HasDomains = payload.Domains is not null,
                DomainCount = checked((uint)domains.Length),
                Domains = domains
            });
            return true;
        }
    }

    internal CollectorMetricEmitter(SessionScope scope)
    {
        scope.Validate();
        if (scope.Process.CreationTimeFileTime is not > 0)
            throw new ArgumentException("Collector metrics require the target's actual creation time.", nameof(scope));
        this.scope = scope;
    }

    internal bool TryEmit(NativeMetricBatch batch)
    {
        if (batch.Process != scope.Process || batch.QpcFrequency != Stopwatch.Frequency)
            throw new ArgumentException("Collector metrics must use this session's target and actual QPC frequency.", nameof(batch));
        return TryEmitMetrics(NativeMetricPayloads.Create(batch, new(batch.QpcFrequency, batch.QpcFrequency)),
            new(batch.BeginQpc, batch.EndQpc));
    }

    internal bool TryEmitMetrics(IReadOnlyList<MetricPayload> observations, TimeRange acquisitionQpc)
    {
        ArgumentNullException.ThrowIfNull(observations);
        var count = observations.Count;
        if (count is < 1 or > NativeMetricPayloads.MaximumPayloadCount || acquisitionQpc.EndQpc > Stopwatch.GetTimestamp())
            throw new ArgumentException("Collector metric batch exceeds its bounds or observation timestamp.", nameof(observations));
        var payloads = new MetricPayload[count];
        for (var i = 0; i < count; i++)
        {
            var payload = observations[i];
            ArgumentNullException.ThrowIfNull(payload);
            EventValidation.Validate(payload, scope.Process);
            if (payload.Interval != acquisitionQpc || payload.SourceObservation is null)
                throw new ArgumentException("Collector metric interval/source observation does not match its acquisition.", nameof(observations));
            ValidateText(payload.Value.Quality.Reason);
            ValidateText(payload.Source);
            ValidateText(payload.SourceVersion);
            payloads[i] = payload;
        }
        lock (gate)
        {
            if (!Source.IsEnabled(EventLevel.Informational, (EventKeywords)EventKeyword.Metrics)) return false;
            foreach (var payload in payloads)
            {
                var observation = payload.SourceObservation!;
                var gpu = payload.Gpu;
                Source.Write("Metric", new EventSourceOptions
                {
                    Level = EventLevel.Informational,
                    Keywords = (EventKeywords)EventKeyword.Metrics
                }, new
                {
                    SemanticEventId = (ushort)2,
                    EventSchemaVersion = (byte)1,
                    SessionId = scope.SessionId,
                    ProducerEpoch = epoch,
                    SourceSequence = checked(++sequence),
                    TargetProcessId = scope.Process.ProcessId,
                    TargetCreationTimeFileTime = scope.Process.CreationTimeFileTime!.Value,
                    TargetInstanceId = scope.Process.InstanceId,
                    AcquisitionBeginQpc = acquisitionQpc.StartQpc,
                    AcquisitionEndQpc = acquisitionQpc.EndQpc,
                    QpcFrequency = Stopwatch.Frequency,
                    Metric = checked((byte)payload.Metric),
                    ValueKind = checked((byte)payload.Value.Kind),
                    ValueUnit = checked((byte)payload.Value.Unit),
                    Integer = payload.Value.Integer ?? 0,
                    Number = payload.Value.Number ?? 0,
                    MeasurementKind = checked((byte)payload.Value.Quality.Kind),
                    Coverage = checked((byte)payload.Value.Quality.Coverage),
                    Reason = payload.Value.Quality.Reason ?? string.Empty,
                    payload.Source,
                    payload.SourceVersion,
                    HasGpu = gpu is not null,
                    GpuScope = checked((byte)(gpu?.Scope ?? default)),
                    HasAdapter = gpu?.Adapter is not null,
                    AdapterLuidLow = gpu?.Adapter?.LuidLowPart ?? 0,
                    AdapterLuidHigh = gpu?.Adapter?.LuidHighPart ?? 0,
                    PhysicalAdapterIndex = gpu?.Adapter?.PhysicalAdapterIndex ?? 0,
                    HasEngine = gpu?.Engine is not null,
                    EngineId = gpu?.Engine?.EngineId ?? 0,
                    EngineClass = checked((byte)(gpu?.Engine?.Class ?? default)),
                    NativeClock = checked((byte)observation.Clock),
                    NativeFrequency = observation.Frequency,
                    NativeBegin = observation.Begin,
                    NativeEnd = observation.End,
                    NativeStatus = checked((byte)observation.Status),
                    NativeAcquisitionBegin = observation.AcquisitionBegin,
                    NativeAcquisitionEnd = observation.AcquisitionEnd,
                    observation.NativeError,
                    NativeErrorDomain = checked((byte)observation.ErrorDomain),
                    observation.PdhError
                });
            }
            return true;
        }
    }

    private static void ValidateText(string? text)
    {
        if (text is null) return;
        try
        {
            if (text.Contains('\0') || WireText.GetByteCount(text) > 1024)
                throw new ArgumentException("Collector wire text exceeds 1024 UTF-8 bytes or contains an embedded NUL.", nameof(text));
        }
        catch (EncoderFallbackException error)
        {
            throw new ArgumentException("Collector wire text contains invalid Unicode.", nameof(text), error);
        }
    }

    [EventSource(Name = EventCatalog.CollectorProviderName, Guid = "b58fa3ac-f7e8-4a24-88cd-478ab589de21")]
    private sealed class CollectorEventSource : EventSource
    {
        internal CollectorEventSource() : base(
            EventSourceSettings.EtwSelfDescribingEventFormat | EventSourceSettings.ThrowOnEventWriteErrors) { }
    }
}

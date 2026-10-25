using System.Diagnostics;
using System.Diagnostics.Tracing;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

[Collection("P03 native ETW")]
public sealed class CollectorMetricTests
{
    [Fact]
    public void Collector_accepts_bounded_full_engine_and_configured_memory_batch()
    {
        using var listener = new CollectorListener();
        var batch = NativeMetricPayloadTests.MaximumBatch(Batch());
        var payloads = NativeMetricPayloads.Create(batch, new(batch.QpcFrequency, batch.QpcFrequency));
        var emitter = new CollectorMetricEmitter(new(Guid.NewGuid(), batch.Process));
        Assert.Equal(1293, payloads.Count);
        Assert.True(emitter.TryEmitMetrics(payloads,
            new(batch.BeginQpc, batch.EndQpc)));
        Assert.Equal(1293, listener.Records.Count);
        Assert.Equal(1293ul, new NativeFields(listener.Records[^1].Fields).U64("SourceSequence"));
    }

    [Theory]
    [InlineData("empty")]
    [InlineData("null")]
    [InlineData("interval")]
    [InlineData("observation")]
    [InlineData("bound")]
    public void Independent_metrics_reject_invalid_batch_before_any_sequence_reservation(string fault)
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var interval = new TimeRange(batch.BeginQpc, batch.EndQpc);
        var payload = NativeMetricPayloads.Create(batch, new(batch.QpcFrequency, batch.QpcFrequency))[0];
        IReadOnlyList<MetricPayload> invalid = fault switch
        {
            "empty" => [],
            "null" => [payload, null!],
            "interval" => [payload, payload with { Interval = new(0, 1) }],
            "observation" => [payload, payload with { SourceObservation = null }],
            _ => new OversizedMetrics()
        };
        var emitter = new CollectorMetricEmitter(new(Guid.NewGuid(), batch.Process));
        Assert.ThrowsAny<ArgumentException>(() => emitter.TryEmitMetrics(invalid, interval));
        Assert.Empty(listener.Records);
        Assert.True(emitter.TryEmitMetrics([payload], interval));
        Assert.Equal(1ul, new NativeFields(Assert.Single(listener.Records).Fields).U64("SourceSequence"));
    }

    private sealed class OversizedMetrics : IReadOnlyList<MetricPayload>
    {
        public int Count => 1294;
        public MetricPayload this[int index] => throw new InvalidOperationException("Oversized metrics must not be read.");
        public IEnumerator<MetricPayload> GetEnumerator() => throw new InvalidOperationException("Oversized metrics must not be enumerated.");
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() => GetEnumerator();
    }

    [Fact]
    public void Collector_accepts_independent_memory_gauge_with_hresult_and_physical_adapter_provenance()
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        var interval = new TimeRange(batch.BeginQpc, batch.EndQpc);
        var payload = NativeMetricPayloads.Create(batch, new(batch.QpcFrequency, batch.QpcFrequency))
            .Single(metric => metric.Metric == MetricKind.GpuDedicatedResidentBytes) with
        {
            Source = "DXCore AdapterMemoryUsageByProcessBytes",
            Gpu = new(new PhysicalAdapterKey(123, -1, 2), GpuMetricScope.PerAdapterMemory, null),
            SourceObservation = new(MetricClock.Boot100ns, 10_000_000, 100, 200,
                MetricSampleStatus.Unavailable, 100, 200, 0x887A0005, MetricErrorDomain.HResult, 0)
        };
        var emitter = new CollectorMetricEmitter(scope);
        Assert.True(emitter.TryEmitMetrics([payload], interval));
        var record = Assert.Single(listener.Records);
        var decoded = Assert.IsType<DecodedEvent>(
            new EventDecoder(scope, new(Stopwatch.Frequency, Stopwatch.Frequency)).Decode(record));
        Assert.Equal(payload, EventCodec.ReadPayload<MetricPayload>(decoded.Event.Envelope));
        Assert.Equal(emitter.Stream, decoded.Event.Stream);
        Assert.Equal(scope.Process, decoded.Provenance.OriginalTarget);
    }

    [Fact]
    public void Collector_construction_registers_provider_before_transport_enablement()
    {
        var batch = Batch();
        _ = new CollectorMetricEmitter(new(Guid.NewGuid(), batch.Process));
        Assert.Single(EventSource.GetSources(), source => source.Guid == EventCatalog.CollectorProviderId);
    }

    [Theory]
    [InlineData(ProcessOperation.Start, true)]
    [InlineData(ProcessOperation.Attach, false)]
    [InlineData(ProcessOperation.Exit, false)]
    public void Collector_process_wire_preserves_operation_optional_creation_and_clock(
        ProcessOperation operation, bool hasCreation)
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        var emitter = new CollectorMetricEmitter(scope);
        var quality = new DataQuality(MeasurementKind.Measured, CoverageState.Partial, "Collector lifecycle fixture.");
        long? creation = hasCreation ? batch.BeginQpc : null;
        Assert.True(emitter.TryEmit(new ProcessPayload(operation, creation, quality)));
        var record = Assert.Single(listener.Records);
        var decoded = Assert.IsType<DecodedEvent>(
            new EventDecoder(scope, new(Stopwatch.Frequency, 3_000_001)).Decode(record));
        var payload = EventCodec.ReadPayload<ProcessPayload>(decoded.Event.Envelope);
        Assert.Equal(operation, payload.Operation);
        Assert.Equal(creation is { } value
            ? checked((long)((Int128)value * 3_000_001 / Stopwatch.Frequency)) : (long?)null, payload.CreationQpc);
        Assert.Equal(quality, payload.Quality);
        Assert.Equal(emitter.Stream, decoded.Event.Stream);
        Assert.Equal(scope.Process, decoded.Provenance.OriginalTarget);
        Assert.NotEqual(scope.Process.ProcessId, decoded.Event.Envelope.ProducerProcessId);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    public void Collector_loss_wire_preserves_unknown_counts_and_optional_domains(int domainMode)
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        var emitter = new CollectorMetricEmitter(scope);
        SequenceDomainKey[]? domains = domainMode switch
        {
            0 => null,
            1 => [],
            _ => [new(emitter.Stream, scope.Process, 2, 1)]
        };
        ulong? count = domainMode == 0 ? null : 7;
        Assert.True(emitter.TryEmit(new LossPayload(emitter.Stream.ProducerEpoch,
            new(batch.BeginQpc, batch.EndQpc), count, "Collector loss fixture.", domains)));
        var record = Assert.Single(listener.Records);
        var decoded = Assert.IsType<DecodedEvent>(
            new EventDecoder(scope, new(Stopwatch.Frequency, 3_000_001)).Decode(record));
        var payload = EventCodec.ReadPayload<LossPayload>(decoded.Event.Envelope);
        Assert.Equal(emitter.Stream.ProducerEpoch, payload.ProducerEpoch);
        Assert.Equal(count, payload.LostEvents);
        Assert.Equal("Collector loss fixture.", payload.Reason);
        Assert.Equal(new TimeRange(
            checked((long)((Int128)batch.BeginQpc * 3_000_001 / Stopwatch.Frequency)),
            checked((long)((Int128)batch.EndQpc * 3_000_001 / Stopwatch.Frequency))), payload.Interval);
        if (domains is null) Assert.Null(payload.Domains);
        else Assert.Equal(domains, payload.Domains);
        Assert.Equal(emitter.Stream, decoded.Event.Stream);
    }

    [Theory]
    [InlineData(false, "nul")]
    [InlineData(false, "surrogate")]
    [InlineData(false, "oversized")]
    [InlineData(true, "nul")]
    [InlineData(true, "surrogate")]
    [InlineData(true, "oversized")]
    public void Collector_lifecycle_rejects_invalid_wire_text_before_reserving_sequence(bool loss, string problem)
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var emitter = new CollectorMetricEmitter(new(Guid.NewGuid(), batch.Process));
        var reason = problem switch
        {
            "nul" => "invalid\0reason",
            "surrogate" => "invalid\uD800reason",
            _ => new string('x', 1025)
        };
        if (loss)
            Assert.Throws<ArgumentException>(() => emitter.TryEmit(new LossPayload(emitter.Stream.ProducerEpoch,
                new(batch.BeginQpc, batch.EndQpc), null, reason)));
        else
            Assert.Throws<ArgumentException>(() => emitter.TryEmit(new ProcessPayload(ProcessOperation.Attach,
                null, new(MeasurementKind.Measured, CoverageState.Partial, reason))));
        Assert.Empty(listener.Records);
        Assert.True(emitter.TryEmit(new ProcessPayload(ProcessOperation.Attach, null, DataQuality.Complete)));
        Assert.Equal(1ul, new NativeFields(Assert.Single(listener.Records).Fields).U64("SourceSequence"));
    }

    [Fact]
    public void Collector_loss_rejects_domain_bound_before_copying_or_enumerating()
    {
        var batch = Batch();
        var emitter = new CollectorMetricEmitter(new(Guid.NewGuid(), batch.Process));
        Assert.Throws<ArgumentException>(() => emitter.TryEmit(new LossPayload(emitter.Stream.ProducerEpoch,
            new(batch.BeginQpc, batch.EndQpc), null, "Oversized domains.", new OversizedDomains())));
    }

    [Theory]
    [InlineData("CreationQpc")]
    [InlineData("QpcFrequency")]
    [InlineData("FutureCreation")]
    public void Collector_process_decoder_rejects_inactive_or_inconsistent_clocks(string field)
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        Assert.True(new CollectorMetricEmitter(scope).TryEmit(
            new ProcessPayload(ProcessOperation.Attach, null, DataQuality.Complete)));
        var record = Assert.Single(listener.Records);
        var fields = record.Fields.ToDictionary();
        if (field == "FutureCreation")
        {
            fields["HasCreationQpc"] = true;
            fields["CreationQpc"] = checked(record.TimestampQpc + 1);
        }
        else fields[field] = field == "CreationQpc" ? 1L : 999L;
        Assert.Throws<InvalidDataException>(() =>
            new EventDecoder(scope, new(Stopwatch.Frequency, Stopwatch.Frequency)).Decode(record.WithFields(fields)));
    }

    [Theory]
    [InlineData("LostEvents")]
    [InlineData("DomainCount")]
    [InlineData("HasDomains")]
    [InlineData("IntervalEndQpc")]
    public void Collector_loss_decoder_rejects_inactive_counts_domains_and_future_interval(string field)
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        var emitter = new CollectorMetricEmitter(scope);
        Assert.True(emitter.TryEmit(new LossPayload(emitter.Stream.ProducerEpoch,
            new(batch.BeginQpc, batch.EndQpc), null, "Loss validation.",
            [new(emitter.Stream, scope.Process, 2, 1)])));
        var record = Assert.Single(listener.Records);
        var fields = record.Fields.ToDictionary();
        fields[field] = field switch
        {
            "LostEvents" => 1ul,
            "DomainCount" => 0u,
            "HasDomains" => false,
            _ => checked(record.TimestampQpc + 1)
        };
        Assert.Throws<InvalidDataException>(() =>
            new EventDecoder(scope, new(Stopwatch.Frequency, Stopwatch.Frequency)).Decode(record.WithFields(fields)));
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void Collector_lifecycle_routes_by_payload_session_not_collector_header(bool loss)
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        var emitter = new CollectorMetricEmitter(scope);
        Assert.True(loss ? emitter.TryEmit(new LossPayload(emitter.Stream.ProducerEpoch,
            new(batch.BeginQpc, batch.EndQpc), null, "Foreign session fixture."))
            : emitter.TryEmit(new ProcessPayload(ProcessOperation.Attach, null, DataQuality.Complete)));
        var foreign = new SessionScope(Guid.NewGuid(), scope.Process);
        Assert.Null(new EventDecoder(foreign, new(Stopwatch.Frequency, Stopwatch.Frequency))
            .Decode(Assert.Single(listener.Records)));
    }

    [Fact]
    public void Collector_loss_rejects_foreign_epoch_and_domain_before_emitting()
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        var emitter = new CollectorMetricEmitter(scope);
        var interval = new TimeRange(batch.BeginQpc, batch.EndQpc);
        Assert.Throws<ArgumentException>(() => emitter.TryEmit(
            new LossPayload(Guid.NewGuid(), interval, null, "Foreign epoch.")));
        var foreign = new ProcessInstanceKey(scope.Process.ProcessId, scope.Process.CreationTimeFileTime, Guid.NewGuid());
        Assert.Throws<ArgumentException>(() => emitter.TryEmit(new LossPayload(emitter.Stream.ProducerEpoch,
            interval, null, "Foreign target domain.", [new(emitter.Stream, foreign, 2, 1)])));
        Assert.Empty(listener.Records);
    }

    private sealed class OversizedDomains : IReadOnlyList<SequenceDomainKey>
    {
        public int Count => 257;
        public SequenceDomainKey this[int index] => throw new InvalidOperationException("Oversized domains must not be read.");
        public IEnumerator<SequenceDomainKey> GetEnumerator() => throw new InvalidOperationException("Oversized domains must not be enumerated.");
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() => GetEnumerator();
    }

    [Theory]
    [InlineData(80, 2, "EventTrace/PartitionInfoExtensionV2", true)]
    [InlineData(81, 2, "EventTrace/PartitionInfoExtensionV2", false)]
    [InlineData(80, 3, "EventTrace/PartitionInfoExtensionV2", false)]
    [InlineData(80, 2, "EventTrace/FutureObservation", false)]
    public void Only_exact_known_partition_transport_metadata_is_not_a_semantic_observation(
        byte opcode, byte version, string name, bool transport)
    {
        var batch = Batch();
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        var record = new NativeRecord(Guid.Parse("9e814aad-3204-11d2-9a82-006008a86939"),
            name, 0, 0, 1, 1, batch.EndQpc, null, new Dictionary<string, object>(),
            descriptorId: ushort.MaxValue, descriptorVersion: version, opcode: opcode);
        var decoder = new EventDecoder(scope, new(Stopwatch.Frequency, Stopwatch.Frequency));
        if (transport) Assert.Null(decoder.Decode(record));
        else Assert.Throws<NotSupportedException>(() => decoder.Decode(record));
    }

    [Fact]
    public void Collector_emitter_supplies_typed_fields_and_qualified_target_not_header_pid()
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        var emitter = new CollectorMetricEmitter(scope);
        Assert.True(emitter.TryEmit(batch));
        var records = listener.Records;
        Assert.Equal(15, records.Count);
        foreach (var record in records)
        {
            var fields = new NativeFields(record.Fields);
            Assert.Equal(scope.SessionId, fields.Guid("SessionId"));
            Assert.Equal(batch.Process.ProcessId, fields.U32("TargetProcessId"));
            Assert.Equal(batch.Process.CreationTimeFileTime, fields.I64("TargetCreationTimeFileTime"));
            Assert.Equal(batch.Process.InstanceId, fields.Guid("TargetInstanceId"));
            Assert.NotEqual(batch.Process.ProcessId, record.ProducerProcessId);
            Assert.Equal(checked((uint)Environment.ProcessId), record.ProducerProcessId);
            Assert.Equal(Stopwatch.Frequency, fields.I64("QpcFrequency"));
        }
        Assert.Equal(Enumerable.Range(1, 15).Select(value => (ulong)value),
            records.Select(record => new NativeFields(record.Fields).U64("SourceSequence")));
    }

    [Fact]
    public void Collector_decoder_routes_two_sessions_by_payload_identity_and_preserves_clocks()
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var first = new SessionScope(Guid.NewGuid(), batch.Process);
        var second = new SessionScope(Guid.NewGuid(), new(batch.Process.ProcessId,
            batch.Process.CreationTimeFileTime, Guid.NewGuid()));
        Assert.True(new CollectorMetricEmitter(first).TryEmit(batch));
        var other = batch with
        {
            Process = second.Process,
            Cpu = batch.Cpu with { Sample = batch.Cpu.Sample with { Process = second.Process } },
            Memory = batch.Memory with { Process = second.Process },
            Gpu = batch.Gpu with { Sample = batch.Gpu.Sample with { Process = second.Process } }
        };
        Assert.True(new CollectorMetricEmitter(second).TryEmit(other));
        var decoder = new EventDecoder(first, new(Stopwatch.Frequency, 3_000_001));
        var decoded = listener.Records.Select(decoder.Decode).OfType<DecodedEvent>().ToArray();
        Assert.Equal(15, decoded.Length);
        Assert.All(decoded, item => Assert.Equal(first.Process, item.Event.Envelope.Process));
        var payloads = decoded.Select(item => EventCodec.ReadPayload<MetricPayload>(item.Event.Envelope)).ToArray();
        var cpu = Assert.Single(payloads, p => p.Metric == MetricKind.CpuUtilization);
        Assert.Equal(MetricClock.Boot100ns, cpu.SourceObservation!.Clock);
        Assert.Equal(100ul, cpu.SourceObservation.Begin);
        var gpu = Assert.Single(payloads, p => p.Metric == MetricKind.GpuUtilization &&
            p.Gpu!.Scope == GpuMetricScope.OverallBusiestEngine);
        Assert.Equal(MetricClock.UtcFileTime100ns, gpu.SourceObservation!.Clock);
        Assert.Equal(134000000000000000ul, gpu.SourceObservation.Begin);
        Assert.All(payloads, p => Assert.Equal(new TimeRange(
            checked((long)((Int128)batch.BeginQpc * 3_000_001 / Stopwatch.Frequency)),
            checked((long)((Int128)batch.EndQpc * 3_000_001 / Stopwatch.Frequency))), p.Interval));
    }

    [Theory]
    [InlineData("QpcFrequency")]
    [InlineData("AcquisitionEndQpc")]
    [InlineData("Integer")]
    [InlineData("HasAdapter")]
    [InlineData("NativeFrequency")]
    public void Collector_decoder_rejects_corrupt_clocks_inactive_arms_and_source_metadata(string field)
    {
        using var listener = new CollectorListener();
        var batch = Batch();
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        Assert.True(new CollectorMetricEmitter(scope).TryEmit(batch));
        var record = listener.Records[0];
        var fields = record.Fields.ToDictionary();
        fields[field] = field switch
        {
            "QpcFrequency" or "NativeFrequency" => 999L,
            "AcquisitionEndQpc" => checked(record.TimestampQpc + 1),
            "Integer" => 1L,
            _ => true
        };
        var decoder = new EventDecoder(scope, new(Stopwatch.Frequency, Stopwatch.Frequency));
        if (field == "NativeFrequency")
            Assert.Throws<ArgumentException>(() => decoder.Decode(record.WithFields(fields)));
        else
            Assert.Throws<InvalidDataException>(() => decoder.Decode(record.WithFields(fields)));
    }

    private static NativeMetricBatch Batch()
    {
        var now = Stopwatch.GetTimestamp();
        return NativeMetricPayloadTests.Batch() with
        {
            BeginQpc = now - 10,
            EndQpc = now,
            QpcFrequency = Stopwatch.Frequency
        };
    }

    private sealed class CollectorListener : EventListener
    {
        internal List<NativeRecord> Records { get; } = [];
        protected override void OnEventSourceCreated(EventSource source)
        {
            if (source.Guid == EventCatalog.CollectorProviderId)
                EnableEvents(source, EventLevel.Informational,
                    (EventKeywords)(EventKeyword.Capabilities | EventKeyword.Metrics));
        }
        protected override void OnEventWritten(EventWrittenEventArgs data)
        {
            if (data.EventSource.Guid != EventCatalog.CollectorProviderId ||
                data.EventName is not ("Metric" or "Process" or "Loss")) return;
            Records.Add(new(data.EventSource.Guid, data.EventName, (byte)data.Level, (ulong)data.Keywords,
                checked((uint)Environment.ProcessId), 7, Stopwatch.GetTimestamp(), null,
                data.PayloadNames!.Zip(data.Payload!).ToDictionary(pair => pair.First, pair => pair.Second!)));
        }
    }
}

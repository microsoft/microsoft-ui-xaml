using System.Diagnostics;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;
using Xunit.Abstractions;

namespace WinUIProfiler.Engine.Tests;

[Collection("P03 native ETW")]
public sealed class CollectorEtlTests(ITestOutputHelper output)
{
    [Fact]
    public async Task Real_collector_lifecycle_etl_preserves_loss_domains_and_durable_health()
    {
        var batch = NativeMetricPayloadTests.Batch() with
        {
            BeginQpc = Stopwatch.GetTimestamp() - 10,
            EndQpc = Stopwatch.GetTimestamp(),
            QpcFrequency = Stopwatch.Frequency
        };
        var scope = new SessionScope(Guid.NewGuid(), batch.Process);
        var emitter = new CollectorMetricEmitter(scope);
        var quality = new DataQuality(MeasurementKind.Measured, CoverageState.Partial, "Collector lifecycle ETL fixture.");
        var directory = Path.Combine(IngestionTests.Root, @"BuildOutput\ProfilerHarness",
            "CollectorLifecycle-" + scope.SessionId.ToString("N"));
        Directory.CreateDirectory(directory);
        var etl = Path.Combine(directory, "lifecycle.etl");
        using (var session = new TraceEventSession("WinUIProfilerCollector-" + scope.SessionId.ToString("N"),
            etl, TraceEventSessionOptions.Create | TraceEventSessionOptions.NoRestartOnCreate)
            { EnableProviderTimeoutMSec = 10_000 })
        {
            session.EnableProvider(EventCatalog.CollectorProviderId, TraceEventLevel.Informational, 0x101);
            Assert.True(emitter.TryEmit(new ProcessPayload(ProcessOperation.Attach, null, quality)));
            Assert.True(emitter.TryEmit(batch));
            Assert.True(emitter.TryEmit(new LossPayload(emitter.Stream.ProducerEpoch,
                new(batch.BeginQpc, batch.EndQpc), 7, "Explicit fixture loss.",
                [new(emitter.Stream, scope.Process, 2, 1)])));
            Assert.True(emitter.TryEmit(new ProcessPayload(ProcessOperation.Exit, null, quality)));
        }
        var raw = new List<NativeRecord>();
        using (var source = new ETWTraceEventSource(etl))
        {
            source.Dynamic.All += data =>
            {
                if (data.ProviderGuid == EventCatalog.CollectorProviderId) raw.Add(NativeRecord.Capture(data));
            };
            source.Process();
        }
        Assert.Equal(18, raw.Count);
        var decoder = new EventDecoder(scope, new(Stopwatch.Frequency, 3_000_001));
        var decoded = raw.Select(record => Assert.IsType<DecodedEvent>(decoder.Decode(record))).ToArray();
        Assert.Equal(Enumerable.Range(1, 18).Select(value => (ulong?)value),
            decoded.Select(value => value.Event.Envelope.SourceSequence));
        Assert.All(decoded, value => Assert.Equal(emitter.Stream, value.Event.Stream));
        var loss = EventCodec.ReadPayload<LossPayload>(decoded[16].Event.Envelope);
        Assert.Equal(7ul, loss.LostEvents);
        Assert.Equal(new SequenceDomainKey(emitter.Stream, scope.Process, 2, 1), Assert.Single(loss.Domains!));
        Assert.Equal(ProcessOperation.Attach,
            EventCodec.ReadPayload<ProcessPayload>(decoded[0].Event.Envelope).Operation);
        Assert.Equal(ProcessOperation.Exit,
            EventCodec.ReadPayload<ProcessPayload>(decoded[17].Event.Envelope).Operation);

        var path = Path.Combine(directory, "lifecycle.sqlite");
        var info = new SessionInfo(SessionMode.Offline, SessionStatus.Recording, 3_000_001, null, [], null);
        using (var store = SessionStore.Create(path, scope, info, quality))
        {
            await using var worker = new ProcessingWorker(store, info, processors: [new MetricProcessor()]);
            await new OfflineEtlSource(etl, scope, info.QpcFrequency).RunAsync(worker);
            var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(SessionStatus.Stopped, result.Status);
            Assert.Equal(18, result.DurableBoundary.IngestOrdinal);
            Assert.Equal(1, result.Health.Count(IngestionIssue.ProducerLoss));
            Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
            Assert.Equal(0, result.Health.Count(IngestionIssue.UnknownSchema));
        }
        using var reopened = SessionStore.OpenReadOnly(path);
        var metrics = await reopened.Queries.GetMetrics(new(scope,
            new(0, checked(reopened.Watermark.TimestampQpc + 1)), new(128)), default);
        Assert.Equal(15, metrics.Items.Count);
        Assert.Contains(reopened.ReadIngestionDiagnostics(),
            diagnostic => diagnostic.Issue == IngestionIssue.ProducerLoss && diagnostic.Count == 1);
    }

    [Fact]
    public async Task Real_live_collector_routes_two_simultaneous_sessions_without_cross_attribution()
    {
        using var process = Process.GetCurrentProcess();
        var target = LiveEtwSource.Identify(process);
        var first = new SessionScope(Guid.NewGuid(), target);
        var second = new SessionScope(Guid.NewGuid(), new(target.ProcessId, target.CreationTimeFileTime, Guid.NewGuid()));
        var directory = Path.Combine(IngestionTests.Root, @"BuildOutput\ProfilerHarness", "CollectorLive-" + first.SessionId.ToString("N"));
        Directory.CreateDirectory(directory);
        var quality = new DataQuality(MeasurementKind.Measured, CoverageState.Partial,
            "Synthetic counters through real simultaneous collector transports.");
        var info = new SessionInfo(SessionMode.LiveAttach, SessionStatus.Recording, 3_000_001, null, [], null);
        using var firstStore = SessionStore.Create(Path.Combine(directory, "first.sqlite"), first, info, quality);
        using var secondStore = SessionStore.Create(Path.Combine(directory, "second.sqlite"), second, info, quality);
        var firstProjection = new SignallingMetrics();
        var secondProjection = new SignallingMetrics();
        await using var firstWorker = new ProcessingWorker(firstStore, info, processors: [firstProjection]);
        await using var secondWorker = new ProcessingWorker(secondStore, info, processors: [secondProjection]);
        await using var firstSource = new LiveEtwSource(first, info.QpcFrequency);
        await using var secondSource = new LiveEtwSource(second, info.QpcFrequency);
        _ = firstSource.RunAsync(firstWorker);
        _ = secondSource.RunAsync(secondWorker);
        await Task.WhenAll(firstSource.Started, secondSource.Started).WaitAsync(TimeSpan.FromSeconds(10));
        foreach (var scope in new[] { first, second })
        {
            var original = NativeMetricPayloadTests.Batch();
            var batch = original with
            {
                Process = scope.Process,
                BeginQpc = Stopwatch.GetTimestamp() - 10,
                EndQpc = Stopwatch.GetTimestamp(),
                QpcFrequency = Stopwatch.Frequency,
                Cpu = original.Cpu with { Sample = original.Cpu.Sample with { Process = scope.Process } },
                Memory = original.Memory with { Process = scope.Process },
                Gpu = original.Gpu with { Sample = original.Gpu.Sample with { Process = scope.Process } }
            };
            Assert.True(new CollectorMetricEmitter(scope).TryEmit(batch));
        }
        await Task.WhenAll(firstProjection.Ready, secondProjection.Ready).WaitAsync(TimeSpan.FromSeconds(10));
        await Task.WhenAll(firstSource.StopAsync(), secondSource.StopAsync());
        var results = await Task.WhenAll(firstWorker.Completion, secondWorker.Completion).WaitAsync(TimeSpan.FromSeconds(10));
        Assert.All(results, result =>
        {
            Assert.Equal(SessionStatus.Stopped, result.Status);
            Assert.Equal(15, result.DurableBoundary.IngestOrdinal);
            Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
            Assert.Equal(0, result.Health.Count(IngestionIssue.UnknownSchema));
        });
        foreach (var (store, scope) in new[] { (firstStore, first), (secondStore, second) })
        {
            var rows = await store.Queries.GetMetrics(new(scope,
                new(0, checked(store.Watermark.TimestampQpc + 1)), new(128)), default);
            Assert.Equal(15, rows.Items.Count);
            Assert.Equal(scope.Process, store.ReadSourceProvenance(1)!.OriginalTarget);
            Assert.All(rows.Items, row => Assert.NotNull(row.SourceObservation));
        }
    }

    [Fact]
    public async Task Real_collector_etl_preserves_two_session_isolation_source_clocks_and_queries()
    {
        var batch = NativeMetricPayloadTests.Batch() with
        {
            BeginQpc = Stopwatch.GetTimestamp() - 10,
            EndQpc = Stopwatch.GetTimestamp(),
            QpcFrequency = Stopwatch.Frequency
        };
        var first = new SessionScope(Guid.NewGuid(), batch.Process);
        var second = new SessionScope(Guid.NewGuid(), new(batch.Process.ProcessId,
            batch.Process.CreationTimeFileTime, Guid.NewGuid()));
        var firstEmitter = new CollectorMetricEmitter(first);
        var secondEmitter = new CollectorMetricEmitter(second);
        var directory = Path.Combine(IngestionTests.Root, @"BuildOutput\ProfilerHarness", "Collector-" + first.SessionId.ToString("N"));
        Directory.CreateDirectory(directory);
        var etl = Path.Combine(directory, "metrics.etl");
        using (var session = new TraceEventSession("WinUIProfilerCollector-" + first.SessionId.ToString("N"),
            etl, TraceEventSessionOptions.Create | TraceEventSessionOptions.NoRestartOnCreate)
            { EnableProviderTimeoutMSec = 10_000 })
        {
            session.EnableProvider(EventCatalog.CollectorProviderId, TraceEventLevel.Informational, 0x100);
            Assert.True(firstEmitter.TryEmit(batch));
            Assert.True(secondEmitter.TryEmit(batch with
            {
                Process = second.Process,
                Cpu = batch.Cpu with { Sample = batch.Cpu.Sample with { Process = second.Process } },
                Memory = batch.Memory with { Process = second.Process },
                Gpu = batch.Gpu with { Sample = batch.Gpu.Sample with { Process = second.Process } }
            }));
        }
        var metadata = OfflineEtlSource.ReadMetadata(etl);
        Assert.Equal(Stopwatch.Frequency, metadata.QpcFrequency);
        Assert.Equal(0, metadata.EventsLost);
        Assert.Equal(0, metadata.BuffersLost);
        var raw = new List<NativeRecord>();
        using (var source = new ETWTraceEventSource(etl))
        {
            source.Dynamic.All += data =>
            {
                if (data.ProviderGuid == EventCatalog.CollectorProviderId)
                    raw.Add(NativeRecord.Capture(data));
                else if (data.EventName == "EventTrace/PartitionInfoExtensionV2")
                    output.WriteLine($"Transport metadata: provider={data.ProviderGuid}, id={(int)data.ID}, version={data.Version}, opcode={(int)data.Opcode}, level={(int)data.Level}, keywords={(ulong)data.Keywords:X16}.");
            };
            Assert.True(source.Process());
        }
        Assert.Equal(30, raw.Count);
        output.WriteLine($"Collector native metadata: {raw[0].Name}, level={raw[0].Level}, keywords={raw[0].Keywords:X16}, bytes={raw[0].RawPayload.Length}.");
        output.WriteLine(string.Join(", ", raw[0].Fields.Select(field => field.Key + ":" + field.Value.GetType().Name)));
        Assert.All(raw, record =>
        {
            Assert.NotEmpty(record.RawPayload.ToArray());
            Assert.Equal(checked((uint)Environment.ProcessId), record.ProducerProcessId);
            Assert.NotEqual(first.Process.ProcessId, record.ProducerProcessId);
        });
        foreach (var scope in new[] { first, second })
        {
            var path = Path.Combine(directory, scope.SessionId.ToString("N") + ".sqlite");
            var info = new SessionInfo(SessionMode.Offline, SessionStatus.Recording, 3_000_001, null, [], null);
            var quality = new DataQuality(MeasurementKind.Measured, CoverageState.Partial,
                "Synthetic counters emitted through real collector ETL; no native parity claim.");
            using (var store = SessionStore.Create(path, scope, info, quality))
            {
                await using var worker = new ProcessingWorker(store, info, processors: [new MetricProcessor()]);
                await new OfflineEtlSource(etl, scope, info.QpcFrequency).RunAsync(worker);
                var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
                output.WriteLine($"Import status={result.Status}, accepted={result.Accepted}, processed={result.Processed}, reason={result.StopReason}.");
                foreach (var sample in result.Health.Samples)
                    output.WriteLine($"{sample.Diagnostic.ProviderId}/{sample.Diagnostic.EventName}: {sample.Diagnostic.Reason}");
                Assert.Equal(SessionStatus.Completed, result.Status);
                Assert.Equal(15, result.DurableBoundary.IngestOrdinal);
                Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
                Assert.Equal(0, result.Health.Count(IngestionIssue.UnknownSchema));
            }
            using var reopened = SessionStore.OpenReadOnly(path);
            var rows = await reopened.Queries.GetMetrics(new(scope,
                new(0, checked(reopened.Watermark.TimestampQpc + 1)), new(128)), default);
            Assert.Equal(15, rows.Items.Count);
            var mapping = Assert.Single(reopened.ReadSourceClocks()).Mapping;
            Assert.Equal(Stopwatch.Frequency, mapping.SourceFrequency);
            Assert.Equal(3_000_001, mapping.SessionFrequency);
            Assert.All(rows.Items, row => Assert.Equal(new TimeRange(
                mapping.Convert(batch.BeginQpc), mapping.Convert(batch.EndQpc)), row.Interval));
            var cpu = Assert.Single(rows.Items, row => row.Metric == MetricKind.CpuUtilization);
            Assert.Equal(10, cpu.Value.Number);
            Assert.Equal(100ul, cpu.SourceObservation!.Begin);
            Assert.Equal(MetricClock.Boot100ns, cpu.SourceObservation.Clock);
            var gpu = Assert.Single(rows.Items, row => row.Metric == MetricKind.GpuUtilization &&
                row.Gpu!.Scope == GpuMetricScope.OverallBusiestEngine);
            Assert.Equal(70, gpu.Value.Number);
            Assert.Equal(134000000000000000ul, gpu.SourceObservation!.Begin);
            Assert.Equal(MetricClock.UtcFileTime100ns, gpu.SourceObservation.Clock);
            Assert.Equal(scope.Process, reopened.ReadSourceProvenance(1)!.OriginalTarget);
        }
    }

    private sealed class SignallingMetrics : IEventProcessor
    {
        private readonly MetricProcessor inner = new();
        private readonly TaskCompletionSource ready = new(TaskCreationOptions.RunContinuationsAsynchronously);
        private int count;
        internal Task Ready => ready.Task;
        public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
        {
            foreach (var change in inner.Process(events, context))
            {
                yield return change;
                if (++count == 15) ready.TrySetResult();
            }
        }
        public void Invalidate(HealthSnapshot health) => inner.Invalidate(health);
    }
}

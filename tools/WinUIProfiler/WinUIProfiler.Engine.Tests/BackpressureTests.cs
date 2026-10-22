using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class BackpressureTests
{
    [Fact]
    public async Task Accepted_epoch_tracking_stops_at_its_bound_and_retains_each_metadata_gap()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        await using var worker = new ProcessingWorker(store, f.Info);
        var source = f.Batch(1, 1).Sources[0];
        for (var index = 0; index < ProcessingWorker.MaximumAcceptedStreams; index++)
        {
            var next = EventCodec.Normalize(source.Envelope, new(source.Stream.ProviderId, source.Stream.ProducerProcessId, Guid.NewGuid()));
            Assert.True(worker.TryEnqueue(new(next, new(f.Scope.Process, 10))));
        }
        var overflow = EventCodec.Normalize(source.Envelope, new(source.Stream.ProviderId, source.Stream.ProducerProcessId, Guid.NewGuid()));
        Assert.False(worker.TryEnqueue(new(overflow, new(f.Scope.Process, 10))));
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(20));
        Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
        Assert.Equal(ProcessingWorker.MaximumAcceptedStreams, result.Accepted);
        Assert.Equal(result.Accepted, result.DurableBoundary.IngestOrdinal);
        Assert.Equal(ProcessingWorker.MaximumAcceptedStreams, result.Health.Count(IngestionIssue.MissingMetadata));
        Assert.Equal(ProcessingWorker.MaximumAcceptedStreams, store.ReadDiagnosticSamples().Count);
    }

    [Fact]
    public async Task Oversized_first_owner_source_never_invokes_a_processor()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var processor = new ThreadProcessor();
        await using var worker = new ProcessingWorker(store, f.Info, new(BatchBytes: 1024), [processor]);
        var identity = EventCatalog.Get(EventCatalog.RuntimeProviderId, 3).Identity;
        var envelope = new EventEnvelope(f.Scope.SessionId, f.Scope.Process, 42, 7, 10, 1, 1, null, identity,
            EventCodec.EncodePayload(new TypeResolvedPayload(f.Epoch, 1, new string('x', 3000)), f.Scope.Process));
        Assert.True(worker.TryEnqueue(new(EventCodec.Normalize(envelope, new(identity.ProviderId, 42, f.Epoch)), new(f.Scope.Process, 10))));
        worker.Complete(SourceCompletion.FiniteEnd);
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(0, processor.Events);
        Assert.Equal(new(0, 0), result.DurableBoundary);
        Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
    }

    [Fact]
    public async Task Projection_byte_exhaustion_stops_enumeration_and_remaining_processors()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var first = new IncrementalProcessor(f);
        var remaining = new ThreadProcessor();
        await using var worker = new ProcessingWorker(store, f.Info, new(BatchBytes: 4096), [first, remaining]);
        var source = f.Batch(1, 1).Sources[0];
        Assert.True(worker.TryEnqueue(new(source, new(f.Scope.Process, 10))));
        worker.Complete(SourceCompletion.FiniteEnd);
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(1, first.Generated);
        Assert.Equal(0, remaining.Events);
        Assert.Equal(new(0, 0), result.DurableBoundary);
        Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(-1)]
    public async Task Owner_input_boundary_includes_exact_metadata_provenance_and_header_bytes(int adjustment)
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var identity = EventCatalog.Get(EventCatalog.RuntimeProviderId, 3).Identity;
        var envelope = new EventEnvelope(f.Scope.SessionId, f.Scope.Process, 42, 7, 10, 1, 1, null, identity,
            EventCodec.EncodePayload(new TypeResolvedPayload(f.Epoch, 1, new string('x', 700)), f.Scope.Process));
        var source = EventCodec.Normalize(envelope, new(identity.ProviderId, 42, f.Epoch));
        var provenance = new SourceProvenance(f.Scope.Process, 10, new(Guid.NewGuid(), 1, new(f.Info.QpcFrequency, f.Info.QpcFrequency)));
        var quality = new DataQuality(MeasurementKind.Measured, CoverageState.Partial, "Runtime observational/baseline coverage is not established.");
        int Bytes(object value) => System.Text.Encoding.UTF8.GetByteCount(System.Text.Json.JsonSerializer.Serialize(value, value.GetType(), WireJson.Options));
        var exact = 32 + EventCodec.Encode(envelope).Length + Bytes(provenance) + Bytes(f.Info) + Bytes(quality);
        Assert.True(exact >= 1024);
        await using var worker = new ProcessingWorker(store, f.Info, new(BatchBytes: exact + adjustment));
        Assert.True(worker.TryEnqueue(new(source, provenance)));
        worker.Complete(SourceCompletion.FiniteEnd);
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(adjustment == 0 ? 1 : 0, result.DurableBoundary.IngestOrdinal);
        Assert.Equal(adjustment == 0 ? SessionStatus.Completed : SessionStatus.StoppedAtLimit, result.Status);
    }

    [Fact]
    public async Task Projection_budget_is_shared_across_processors_and_single_record_is_capped()
    {
        foreach (var giant in new[] { false, true })
        {
            using var f = new StoreFixture();
            using var store = f.Create();
            var first = new IncrementalProcessor(f, 100, 1);
            var second = new IncrementalProcessor(f, giant ? 70_000 : 3500);
            var last = new ThreadProcessor();
            await using var worker = new ProcessingWorker(store, f.Info, new(BatchBytes: giant ? 200_000 : 4096), [first, second, last]);
            var source = f.Batch(1, 1).Sources[0];
            Assert.True(worker.TryEnqueue(new(source, new(f.Scope.Process, 10))));
            worker.Complete(SourceCompletion.FiniteEnd);
            var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(1, first.Generated);
            Assert.Equal(1, second.Generated);
            Assert.Equal(0, last.Events);
            Assert.Equal(new(0, 0), result.DurableBoundary);
            Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
        }
    }

    [Fact]
    public async Task Incremental_projection_limit_preserves_the_previous_source_and_durable_boundary()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var limited = new IncrementalProcessor(f, minimumOrdinal: 2);
        var remaining = new ThreadProcessor();
        await using var worker = new ProcessingWorker(store, f.Info, new(BatchItems: 1, BatchBytes: 4096), [limited, remaining]);
        var first = f.Batch(1, 1).Sources[0];
        Assert.True(worker.TryEnqueue(new(first, new(f.Scope.Process, 10))));
        Assert.True(SpinWait.SpinUntil(() => store.Watermark.IngestOrdinal == 1, TimeSpan.FromSeconds(10)));
        var second = f.Batch(2, 2).Sources[0];
        Assert.True(worker.TryEnqueue(new(second, new(f.Scope.Process, 20))));
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(1, limited.Generated);
        Assert.Equal(1, remaining.Events);
        Assert.Equal(new(1, 10), result.DurableBoundary);
        Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
        using var db = f.Raw();
        Assert.Equal(1L, StoreFixture.Scalar(db, "SELECT count(*) FROM SourceEvents"));
    }

    private sealed class IncrementalProcessor(StoreFixture fixture, int textLength = 3500, int count = 20, long minimumOrdinal = 1) : IEventProcessor
    {
        internal int Generated;
        public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context) =>
            events[0].Envelope.IngestOrdinal < minimumOrdinal ? [] : new GeneratedChanges(() =>
            {
                Generated++;
                var sample = (MetricSample)fixture.Batch(1, 1).Changes[0].Value;
                return new(events[0].Envelope.IngestOrdinal, sample with { Source = new string('x', textLength) });
            }, count);
        public void Invalidate(HealthSnapshot health) { }
    }
    private sealed class GeneratedChanges(Func<ProjectionChange> next, int count) : IReadOnlyList<ProjectionChange>
    {
        public int Count => count;
        public ProjectionChange this[int index] => next();
        public IEnumerator<ProjectionChange> GetEnumerator()
        {
            for (var index = 0; index < Count; index++) yield return next();
        }
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() => GetEnumerator();
    }

    [Fact]
    public async Task Distinct_unknown_schema_diagnostics_are_bounded_durable_and_stop_instead_of_eviction()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        await using var worker = new ProcessingWorker(store, f.Info);
        using var acquisition = worker.RegisterAcquisition();
        for (var i = 0; i <= SessionHealth.MaximumDiagnosticSamples; i++)
            worker.Report(new(IngestionIssue.UnknownSchema, "unsupported contract version", EventCatalog.RuntimeProviderId, "Future-" + i, 42, i, "AB"));
        acquisition.Dispose();
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
        Assert.Equal(SessionHealth.MaximumDiagnosticSamples, store.ReadDiagnosticSamples().Count);
        Assert.Contains(store.ReadDiagnosticSamples(), sample => sample.Diagnostic.EventName == "Future-0");
        Assert.Contains(store.ReadDiagnosticSamples(), sample => sample.Diagnostic.EventName == "Future-127");
        Assert.Equal(1, result.Health.Count(IngestionIssue.DiagnosticLimit));
    }
    [Fact]
    public async Task Actual_SQLite_page_limit_stops_acquisition_and_retains_last_durable_history()
    {
        using var f = new StoreFixture();
        using var store = f.Create(new(1024));
        await using var worker = new ProcessingWorker(store, f.Info, new(BatchItems: 1));
        var first = f.Batch(1, 1).Sources[0];
        Assert.True(worker.TryEnqueue(new(first, new(f.Scope.Process, 10))));
        Assert.True(SpinWait.SpinUntil(() => store.Watermark.IngestOrdinal == 1, TimeSpan.FromSeconds(10)));
        using (var db = f.Raw())
            StoreFixture.Sql(db, "CREATE TRIGGER fill_disk BEFORE INSERT ON SourceEvents BEGIN UPDATE SourceEvents SET Envelope=zeroblob(8388608); END");
        var second = f.Batch(2, 2).Sources[0];
        Assert.True(worker.TryEnqueue(new(second, new(f.Scope.Process, 20))));
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.Failed, result.Status);
        Assert.Equal(new(1, 10), result.DurableBoundary);
        Assert.Equal(13, store.ReadFailure()!.SqliteErrorCode);
        Assert.Contains("SQLITE_FULL", result.StopReason);
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(new(1, 10), reopened.Watermark);
        Assert.Equal(SessionStatus.Failed, (await reopened.Queries.GetSession(new(f.Scope), default)).Value.Status);
    }

    [Fact]
    public async Task Cancellation_admission_races_drain_every_accepted_event_without_fabricating_target_loss()
    {
        for (var iteration = 0; iteration < 12; iteration++)
        {
            using var f = new StoreFixture();
            using var store = f.Create();
            await using var worker = new ProcessingWorker(store, f.Info);
            using var start = new ManualResetEventSlim();
            var sources = f.Batch(1, 32).Sources;
            var callbacks = Task.Run(() =>
            {
                start.Wait();
                foreach (var source in sources)
                    worker.TryEnqueue(new(source, new(f.Scope.Process, source.Envelope.TimestampQpc)));
            });
            var cancellation = Task.Run(() => { start.Wait(); worker.Complete(SourceCompletion.Cancelled); });
            start.Set();
            await Task.WhenAll(callbacks, cancellation).WaitAsync(TimeSpan.FromSeconds(10));
            var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(result.Accepted, result.DurableBoundary.IngestOrdinal);
            Assert.Equal(SessionStatus.Stopped, result.Status);
            Assert.Equal(0, result.Health.Count(IngestionIssue.ProducerLoss));
            Assert.Equal(0, result.Health.Count(IngestionIssue.SequenceGap));
        }
    }
    [Fact]
    public async Task Reserved_final_health_waits_for_acquisition_callbacks_after_owner_drain()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var processor = new ThreadProcessor();
        await using var worker = new ProcessingWorker(store, f.Info, processors: [processor]);
        using var sourceLifetime = worker.RegisterAcquisition();
        worker.Complete(SourceCompletion.UserStop);
        await worker.ProcessingDrained.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.False(worker.Completion.IsCompleted);
        var tracker = new EtwLossTracker(worker);
        tracker.Update(3, 2);
        tracker.Update(3, 2);
        tracker.Update(4, 2);
        sourceLifetime.Dispose();
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(4, result.Health.Count(IngestionIssue.EtwEventsLost));
        Assert.Equal(2, result.Health.Count(IngestionIssue.EtwBuffersLost));
        Assert.Equal(4, processor.Latest!.Count(IngestionIssue.EtwEventsLost));
        Assert.Equal(4, store.ReadIngestionDiagnostics().Single(r => r.Issue == IngestionIssue.EtwEventsLost).Count);
        Assert.Throws<InvalidOperationException>(() => worker.Report(new(IngestionIssue.EtwEventsLost, "too late")));
    }

    [Fact]
    public async Task Reordered_sequences_and_late_QPC_are_not_loss_and_collector_header_is_not_target()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        await using var worker = new ProcessingWorker(store, f.Info);
        foreach (var index in new[] { 3, 1, 2 })
        {
            var source = f.Batch(index, index).Sources[0];
            Assert.NotEqual(source.Envelope.Process.ProcessId, source.Envelope.ProducerProcessId);
            Assert.True(worker.TryEnqueue(new(source, new(source.Envelope.Process, source.Envelope.TimestampQpc))));
        }
        worker.Complete(SourceCompletion.FiniteEnd);
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(0, result.Health.Count(IngestionIssue.SequenceGap));
        Assert.Equal(SessionStatus.Completed, result.Status);
        Assert.Equal(new(3, 30), result.DurableBoundary);
        using var db = f.Raw();
        using var command = SessionStore.Command(db, "SELECT Envelope FROM SourceEvents ORDER BY Ordinal");
        using var rows = command.ExecuteReader();
        var sequences = new List<ulong?>();
        while (rows.Read())
        {
            var e = EventCodec.Decode((byte[])rows[0]);
            Assert.Equal(99u, e.ProducerProcessId);
            Assert.Equal(f.Scope.Process, e.Process);
            sequences.Add(e.SourceSequence);
        }
        Assert.Equal(new ulong?[] { 3, 1, 2 }, sequences);
    }

    [Fact]
    public async Task Ingress_byte_limit_is_independent_of_item_limit_and_never_evicts()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        await using var worker = new ProcessingWorker(store, f.Info, new(IngressItems: 1024, IngressBytes: 1024));
        var payload = new TypeResolvedPayload(f.Epoch, 1, new string('x', 2048));
        var identity = EventCatalog.Get(EventCatalog.RuntimeProviderId, 3).Identity;
        var envelope = new EventEnvelope(f.Scope.SessionId, f.Scope.Process, 42, 7, 1, 1, 1, null, identity,
            EventCodec.EncodePayload(payload, f.Scope.Process));
        var source = EventCodec.Normalize(envelope, new(identity.ProviderId, 42, f.Epoch));
        Assert.False(worker.TryEnqueue(new(source, new(f.Scope.Process, 1))));
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
        Assert.Equal(0, result.Accepted);
        Assert.Equal(new(0, 0), result.DurableBoundary);
        Assert.Equal(1, result.Health.Count(IngestionIssue.QueueOverflow));
    }

    [Fact]
    public async Task Sustained_persistence_backlog_stops_at_a_durable_boundary()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        await using var worker = new ProcessingWorker(store, f.Info,
            new(PersistenceItems: 1, BatchItems: 1, BacklogTimeoutMilliseconds: 100));
        var first = f.Batch(1, 1).Sources[0];
        Assert.True(worker.TryEnqueue(new(first, new(f.Scope.Process, 10))));
        Assert.True(SpinWait.SpinUntil(() => store.Watermark.IngestOrdinal == 1, TimeSpan.FromSeconds(10)));
        Monitor.Enter(store.Gate);
        try
        {
            foreach (var source in f.Batch(2, 8).Sources)
                Assert.True(worker.TryEnqueue(new(source, new(f.Scope.Process, source.Envelope.TimestampQpc))));
            Assert.True(SpinWait.SpinUntil(() => worker.ProcessingDrained.IsCompleted, TimeSpan.FromSeconds(10)));
        }
        finally { Monitor.Exit(store.Gate); }
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
        Assert.Equal(1, result.Health.Count(IngestionIssue.PersistenceBacklog));
        Assert.True(result.Accepted > result.DurableBoundary.IngestOrdinal);
        Assert.Equal(result.DurableBoundary, store.Watermark);
    }

    [Fact]
    public async Task Blocked_SQLite_writer_bounds_both_queues_and_callbacks_do_not_wait_for_storage()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var options = new ProcessingOptions(IngressItems: 2, IngressBytes: 8192, PersistenceItems: 1, PersistenceBytes: 8192, BatchItems: 1, BatchBytes: 4096);
        await using var worker = new ProcessingWorker(store, f.Info, options);
        DecodedEvent Event(int index)
        {
            var source = f.Batch(index, index).Sources[0];
            return new(source, new(source.Envelope.Process, source.Envelope.TimestampQpc));
        }
        Assert.True(worker.TryEnqueue(Event(1)));
        Assert.True(SpinWait.SpinUntil(() => store.Watermark.IngestOrdinal == 1, TimeSpan.FromSeconds(10)));
        Monitor.Enter(store.Gate);
        try
        {
            Assert.True(worker.TryEnqueue(Event(2)));
            Assert.True(worker.TryEnqueue(Event(3)));
            Assert.True(SpinWait.SpinUntil(() => worker.PersistenceUsage.Items == 1, TimeSpan.FromSeconds(10)));
            var overflow = false;
            Exception? callbackFailure = null;
            using var callbackDone = new ManualResetEventSlim();
            var callback = new Thread(() =>
            {
                try
                {
                    for (var i = 4; i < 30; i++)
                        if (!worker.TryEnqueue(Event(i))) { overflow = true; break; }
                }
                catch (Exception error) { callbackFailure = error; }
                finally { callbackDone.Set(); }
            }) { IsBackground = true };
            callback.Start();
            Assert.True(callbackDone.Wait(TimeSpan.FromSeconds(2)), "ETW callback admission waited for the blocked SQLite writer.");
            callback.Join();
            Assert.Null(callbackFailure);
            Assert.True(overflow);
            Assert.InRange(worker.IngressUsage.Items, 0, options.IngressItems);
            Assert.InRange(worker.IngressUsage.Bytes, 0, options.IngressBytes);
            Assert.InRange(worker.PersistenceUsage.Items, 0, options.PersistenceItems);
            Assert.InRange(worker.PersistenceUsage.Bytes, 0, options.PersistenceBytes);
            for (var i = 0; i < 1000; i++)
                worker.Report(new(IngestionIssue.UnknownSchema, "unsupported schema", EventCatalog.RuntimeProviderId, "future", 42, 10, "FF"));
        }
        finally { Monitor.Exit(store.Gate); }
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
        Assert.Equal(1, result.Health.Count(IngestionIssue.QueueOverflow));
        Assert.Equal(1000, result.Health.Count(IngestionIssue.UnknownSchema));
        Assert.Equal(1000, store.ReadIngestionDiagnostics().Single(r => r.Issue == IngestionIssue.UnknownSchema).Count);
        Assert.Equal(result.DurableBoundary, store.Watermark);
    }

    [Fact]
    public async Task Terminal_SQLite_failure_uses_preallocated_sidecar_and_never_reports_completion()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        using (var db = f.Raw())
            StoreFixture.Sql(db, "CREATE TRIGGER fail_control BEFORE INSERT ON IngestionDiagnostics BEGIN SELECT RAISE(ABORT,'blocked health'); END");
        await using var worker = new ProcessingWorker(store, f.Info);
        worker.Report(new(IngestionIssue.DecodeFailure, "bad wire"));
        worker.Complete(SourceCompletion.FiniteEnd);
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.Failed, result.Status);
        Assert.Equal(new(0, 0), result.DurableBoundary);
        Assert.Equal(4096, new FileInfo(f.Path + ".failure.json").Length);
        Assert.Null(result.FailureRecordError);
        Assert.Equal(SessionStatus.Failed, (await store.Queries.GetSession(new(f.Scope), default)).Value.Status);
    }

    [Fact]
    public async Task Empty_recording_has_durable_terminal_health_without_fake_sources()
    {
        using var f = new StoreFixture();
        using (var store = f.Create())
        {
            await using var worker = new ProcessingWorker(store, f.Info);
            worker.Report(new(IngestionIssue.DecodeFailure, "bad required field"));
            worker.Complete(SourceCompletion.Cancelled);
            var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(new(0, 0), result.DurableBoundary);
            Assert.Equal(SessionStatus.Stopped, result.Status);
            Assert.Equal(1, result.Health.Count(IngestionIssue.DecodeFailure));
        }
        using var reader = SessionStore.OpenReadOnly(f.Path);
        Assert.False(reader.UnfinishedRecording);
        Assert.Equal(SessionStatus.Stopped, (await reader.Queries.GetSession(new(f.Scope), default)).Value.Status);
        Assert.Single(reader.ReadIngestionDiagnostics(), r => r.Issue == IngestionIssue.DecodeFailure);
        using var db = f.Raw();
        Assert.Equal(0L, StoreFixture.Scalar(db, "SELECT count(*) FROM SourceEvents"));
    }

    [Fact]
    public async Task Dedicated_owner_and_separate_writer_preserve_committed_batch_boundary()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var processors = Enumerable.Range(0, 5).Select(_ => new ThreadProcessor()).ToArray();
        await using var worker = new ProcessingWorker(store, f.Info, processors: processors);
        foreach (var source in f.Batch(1, 3).Sources)
            Assert.True(worker.TryEnqueue(new(source, new(source.Envelope.Process, source.Envelope.TimestampQpc))));
        worker.Complete(SourceCompletion.FiniteEnd);
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(new(3, 30), result.DurableBoundary);
        Assert.NotEqual(Environment.CurrentManagedThreadId, worker.OwnerThreadId);
        Assert.NotEqual(worker.OwnerThreadId, worker.WriterThreadId);
        foreach (var processor in processors)
        {
            Assert.Equal(3, processor.Events);
            Assert.Equal(worker.OwnerThreadId, processor.ThreadId);
            Assert.Equal(processors[0].Batches.Count, processor.Batches.Count);
            for (var index = 0; index < processor.Batches.Count; index++)
                Assert.Same(processors[0].Batches[index], processor.Batches[index]);
        }
        Assert.Equal(SessionStatus.Completed, result.Status);
    }

    private sealed class ThreadProcessor : IEventProcessor
    {
        internal int ThreadId;
        internal int Events;
        internal readonly List<IReadOnlyList<NormalizedEvent>> Batches = [];
        internal HealthSnapshot? Latest;
        public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
        {
            if (ThreadId == 0) ThreadId = Environment.CurrentManagedThreadId;
            Assert.Equal(ThreadId, Environment.CurrentManagedThreadId);
            Events += events.Count;
            Batches.Add(events);
            return [];
        }
        public void Invalidate(HealthSnapshot health)
        {
            if (ThreadId == 0) ThreadId = Environment.CurrentManagedThreadId;
            Assert.Equal(ThreadId, Environment.CurrentManagedThreadId);
            Latest = health;
        }
    }
}

using System.Security.Cryptography;
using System.Text;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

[Collection("P03 native ETW")]
public sealed class P03StorageTests
{
    [Fact]
    public async Task Source_clock_registration_is_bounded_and_persisted_mappings_cannot_be_replaced()
    {
        using var f = new StoreFixture();
        using (var store = f.Create())
        {
            await using var worker = new ProcessingWorker(store, f.Info);
            for (var index = 0; index < SourceClockMetadata.MaximumSources; index++)
                worker.RegisterSourceClock(new(Guid.NewGuid(), 1, new(10_000_000, f.Info.QpcFrequency, index)));
            Assert.Throws<InvalidDataException>(() => worker.RegisterSourceClock(new(Guid.NewGuid(), 1, new(1, f.Info.QpcFrequency))));
            Assert.Equal(SessionStatus.StoppedAtLimit, (await worker.Completion).Status);
        }
        using var writable = SessionStore.OpenWritable(f.Path);
        var clocks = writable.ReadSourceClocks();
        Assert.Equal(SourceClockMetadata.MaximumSources, clocks.Count);
        var changed = clocks[0] with { Mapping = clocks[0].Mapping with { OriginQpc = 9999 } };
        // A separate unfinished recording exercises writer-side immutability too.
        using var other = new StoreFixture();
        using var otherStore = other.Create();
        var writer = otherStore.CreateWriter();
        writer.CommitHealth(new([]), clocks: [clocks[0]]);
        Assert.Throws<ArgumentException>(() => writer.CommitHealth(new([]), clocks: [changed]));
        Assert.Equal(clocks[0], Assert.Single(otherStore.ReadSourceClocks()));
    }

    [Fact]
    public async Task Real_offline_import_retains_clock_when_all_runtime_records_are_foreign()
    {
        using var f = new StoreFixture();
        Directory.CreateDirectory(Path.GetDirectoryName(f.Path)!);
        long sourceFrequency;
        var info = f.Info with { Mode = SessionMode.Offline, QpcFrequency = 3_000_001 };
        using (var fixtures = new NativeFixtures())
        {
            sourceFrequency = OfflineEtlSource.ReadMetadata(fixtures.Files[0]).QpcFrequency;
            using var store = SessionStore.Create(f.Path, f.Scope, info, StoreFixture.Partial);
            await using var worker = new ProcessingWorker(store, info);
            await new OfflineEtlSource(fixtures.Files[0], f.Scope, info.QpcFrequency).RunAsync(worker);
            var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(new(0, 0), result.DurableBoundary);
            Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
            Assert.DoesNotContain(result.Health.Samples, sample => sample.Diagnostic.ProviderId == EventCatalog.RuntimeProviderId);
        }
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        var clock = Assert.Single(reopened.ReadSourceClocks());
        Assert.Equal(sourceFrequency, clock.Mapping.SourceFrequency);
        Assert.Equal(info.QpcFrequency, clock.Mapping.SessionFrequency);
        Assert.Equal(0, clock.Mapping.OriginQpc);
        Assert.Equal(1, clock.ClockType);
    }

    [Fact]
    public async Task Original_clock_mapping_survives_reopen_without_ETL_or_capabilities()
    {
        using var f = new StoreFixture();
        Directory.CreateDirectory(Path.GetDirectoryName(f.Path)!);
        DecodedEvent decoded;
        SourceClockMetadata clock;
        SessionScope scope;
        using (var fixtures = new NativeFixtures())
        {
            var record = fixtures.Read().First(r => r.Name == "Frame");
            scope = new(Guid.NewGuid(), record.ReadRuntimeTarget());
            var decoder = new EventDecoder(scope, new(System.Diagnostics.Stopwatch.Frequency, 3_000_001, record.TimestampQpc - 10_001));
            clock = decoder.SourceClock;
            decoded = decoder.Decode(record)!;
        }
        var info = f.Info with { Mode = SessionMode.Offline, QpcFrequency = 3_000_001 };
        using (var store = SessionStore.Create(f.Path, scope, info, StoreFixture.Partial))
        {
            await using var worker = new ProcessingWorker(store, info);
            Assert.True(worker.TryEnqueue(decoded));
            worker.Complete(SourceCompletion.FiniteEnd);
            Assert.Equal(1, (await worker.Completion).Health.Count(IngestionIssue.MissingMetadata));
        }
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(clock, Assert.Single(reopened.ReadSourceClocks()));
        var provenance = reopened.ReadSourceProvenance(1)!;
        Assert.Equal(clock, provenance.Clock);
        Assert.Equal(decoded.Provenance.OriginalTimestampQpc, provenance.OriginalTimestampQpc);
        using var db = f.Raw();
        using var command = SessionStore.Command(db, "SELECT Envelope FROM SourceEvents");
        var envelope = EventCodec.Decode((byte[])command.ExecuteScalar()!);
        Assert.Equal(clock.Mapping.Convert(provenance.OriginalTimestampQpc), envelope.TimestampQpc);
        Assert.Equal(10_001L * 3_000_001 / System.Diagnostics.Stopwatch.Frequency, envelope.TimestampQpc);
    }

    [Fact]
    public async Task Known_clock_is_durable_at_zero_boundary_for_diagnostic_only_import()
    {
        using var f = new StoreFixture();
        var clock = new SourceClockMetadata(Guid.NewGuid(), 1, new(10_000_000, f.Info.QpcFrequency, 12345));
        using (var store = f.Create())
        {
            await using var worker = new ProcessingWorker(store, f.Info);
            worker.RegisterSourceClock(clock);
            Assert.Throws<ArgumentException>(() => worker.RegisterSourceClock(clock with { Mapping = clock.Mapping with { OriginQpc = 12346 } }));
            worker.Report(new(IngestionIssue.UnknownSchema, "metadata known but no accepted observation"));
            worker.Complete(SourceCompletion.FiniteEnd);
            Assert.Equal(new(0, 0), (await worker.Completion).DurableBoundary);
        }
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(clock, Assert.Single(reopened.ReadSourceClocks()));
        Assert.Null(reopened.ReadSourceProvenance(1));
    }

    [Fact]
    public void Diagnostic_reader_rejects_an_extra_category_instead_of_silently_truncating_it()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var health = new SessionHealth();
        foreach (var issue in Enum.GetValues<IngestionIssue>()) health.Report(new(issue, "bounded category"));
        store.CreateWriter().CommitHealth(health.Snapshot());
        using var db = f.Raw();
        StoreFixture.Sql(db, "INSERT INTO IngestionDiagnostics SELECT 13,Count,CommittedOrdinal,PayloadVersion,SampleJson FROM IngestionDiagnostics WHERE Issue=0");
        Assert.Throws<ArgumentException>(() => store.ReadIngestionDiagnostics());
    }

    [Fact]
    public async Task Worker_does_not_blindly_resume_history_without_reconstructing_processor_state()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        store.CreateWriter().Commit(f.Batch(1, 1));
        ProcessingWorker? unexpected = null;
        try { Assert.Throws<InvalidOperationException>(() => unexpected = new(store, f.Info)); }
        finally { if (unexpected is not null) await unexpected.DisposeAsync(); }
    }
    [Theory]
    [InlineData(1)]
    [InlineData(2)]
    public async Task Legacy_schemas_are_read_without_clock_fabrication_and_upgrade_only_on_explicit_writable_open(int version)
    {
        using var f = new StoreFixture();
        using (var store = f.Create())
            store.CreateWriter().Commit(f.Batch(1, 1) with { Provenance = [new(1, new(f.Scope.Process, 10))] });
        using (var db = f.Raw())
        {
            StoreFixture.Sql(db, "ALTER TABLE SourceProvenance DROP COLUMN SourceClockId; DROP TABLE SourceClocks;");
            if (version == 1)
                StoreFixture.Sql(db, "DROP TABLE IngestionDiagnostics; DROP TABLE IngestionDiagnosticSamples; DROP TABLE SessionTerminal; DROP TABLE SourceProvenance;");
            StoreFixture.Sql(db, $"PRAGMA user_version={version};");
            using var command = SessionStore.Command(db, "SELECT type,name,sql FROM sqlite_master WHERE type IN ('table','index') AND name NOT LIKE 'sqlite_%' ORDER BY type,name");
            using var rows = command.ExecuteReader();
            var text = new StringBuilder();
            while (rows.Read()) text.Append(rows.GetString(0)).Append('|').Append(rows.GetString(1)).Append('|')
                .Append(rows.IsDBNull(2) ? "" : rows.GetString(2)).Append('\n');
            rows.Close();
            using var update = SessionStore.Command(db, "UPDATE Sessions SET SchemaHash=$hash", null,
                ("$hash", Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(text.ToString())))));
            update.ExecuteNonQuery();
        }
        using (var read = SessionStore.OpenReadOnly(f.Path))
        {
            Assert.Empty(read.ReadIngestionDiagnostics());
            Assert.Empty(read.ReadDiagnosticSamples());
            Assert.Empty(read.ReadSourceClocks());
            if (version == 2)
            {
                var provenance = read.ReadSourceProvenance(1)!;
                Assert.Null(provenance.Clock);
                Assert.Equal(10, provenance.OriginalTimestampQpc);
            }
            else Assert.Null(read.ReadSourceProvenance(1));
            Assert.Single((await read.Queries.GetMetrics(f.Metrics(), default)).Items);
            using var db = f.Raw();
            Assert.Equal((long)version, StoreFixture.Scalar(db, "PRAGMA user_version"));
        }
        using (var upgraded = SessionStore.OpenWritable(f.Path))
        {
            Assert.Equal(new(1, 10), upgraded.Watermark);
            upgraded.CreateWriter().Commit(f.Batch(2, 2));
        }
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(new(2, 20), reopened.Watermark);
        Assert.Empty(reopened.ReadSourceClocks());
        if (version == 2) Assert.Null(reopened.ReadSourceProvenance(1)!.Clock);
        using (var db = f.Raw()) Assert.Equal(3L, StoreFixture.Scalar(db, "PRAGMA user_version"));
        Assert.Equal(2, (await reopened.Queries.GetMetrics(f.Metrics(), default)).Items.Count);
    }

    [Fact]
    public async Task Terminal_health_does_not_rewrite_source_checkpoint_or_source_anchored_projections()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        var writer = store.CreateWriter();
        var boundary = writer.Commit(f.Batch(1, 1));
        var health = new SessionHealth();
        health.Report(new(IngestionIssue.EtwBuffersLost, "transport lost buffers"), 7);
        var partial = new DataQuality(MeasurementKind.Unavailable, CoverageState.Partial, "transport stopped without closure");
        writer.CommitHealth(health.Snapshot(), f.Info with { Status = SessionStatus.Stopped, StopReason = "User stop." }, partial);
        Assert.Equal(boundary, store.Watermark);
        using var db = f.Raw();
        Assert.Equal(SessionStatus.Recording, SessionStore.ReadCheckpoint(db, boundary).Info.Status);
        var info = await store.Queries.GetSession(new(f.Scope), default);
        Assert.Equal(SessionStatus.Stopped, info.Value.Status);
        Assert.Equal(partial, info.Context.Quality);
        var metrics = await store.Queries.GetMetrics(f.Metrics(new(128, 0, boundary)), default);
        Assert.Single(metrics.Items);
        Assert.Equal(StoreFixture.Partial, metrics.Context.Quality);
        Assert.Equal(7, store.ReadIngestionDiagnostics().Single().Count);
        Assert.Throws<InvalidOperationException>(() => writer.Commit(f.Batch(2, 2)));
    }

    [Fact]
    public async Task Worker_uses_settled_receipt_evidence_not_reordered_arrival_pairs_for_sequence_continuity()
    {
        using var f = new StoreFixture();
        using var store = f.Create();
        await using var worker = new ProcessingWorker(store, f.Info);
        using var acquisition = worker.RegisterAcquisition();
        foreach (var i in new[] { 3, 1, 2 })
        {
            var source = f.Batch(i, i).Sources[0];
            worker.TryEnqueue(new(source, new(f.Scope.Process, source.Envelope.TimestampQpc)));
        }
        worker.Complete(SourceCompletion.UserStop);
        await worker.ProcessingDrained.WaitAsync(TimeSpan.FromSeconds(10));
        var domain = f.Batch(1, 1).Sources[0].SequenceDomain;
        var listenerId = Guid.NewGuid(); var continuityId = Guid.NewGuid();
        var listener = new ListenerDomainInterval(listenerId, continuityId, domain, new(0, 100), DataQuality.Complete);
        var window = new SequenceObservationWindow(0, 3,
        [
            new(listenerId, continuityId, domain, 3, 30, 1),
            new(listenerId, continuityId, domain, 1, 10, 2),
            new(listenerId, continuityId, domain, 2, 20, 3)
        ]);
        Assert.Equal(SequenceContinuityState.Unproven, worker.ReconcileSequences(window, listener).State);
        var cut = new ProducerSequenceCut(Guid.NewGuid(), domain, 3, 3);
        var receipt = new SequenceReceiptEvidence(listenerId, continuityId, cut.CutId, 0, 3, 3);
        Assert.Equal(SequenceContinuityState.Contiguous, worker.ReconcileSequences(window, listener, cut, receipt: receipt).State);
        Assert.Throws<ArgumentException>(() => worker.ReconcileSequences(window, listener, cut, receipt: receipt with { ProcessedThroughOrdinal = 4 }));
        acquisition.Dispose();
        Assert.Equal(0, (await worker.Completion).Health.Count(IngestionIssue.SequenceGap));
    }
}

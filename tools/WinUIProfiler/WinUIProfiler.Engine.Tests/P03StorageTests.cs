using System.Security.Cryptography;
using System.Text;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class P03StorageTests
{
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
    [Fact]
    public async Task Legacy_schema_one_is_read_without_migration_and_upgrades_only_on_explicit_writable_open()
    {
        using var f = new StoreFixture();
        using (var store = f.Create()) store.CreateWriter().Commit(f.Batch(1, 1));
        using (var db = f.Raw())
        {
            StoreFixture.Sql(db, "DROP TABLE IngestionDiagnostics; DROP TABLE IngestionDiagnosticSamples; DROP TABLE SessionTerminal; DROP TABLE SourceProvenance; PRAGMA user_version=1;");
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
            Assert.Single((await read.Queries.GetMetrics(f.Metrics(), default)).Items);
            using var db = f.Raw();
            Assert.Equal(1L, StoreFixture.Scalar(db, "PRAGMA user_version"));
        }
        using (var upgraded = SessionStore.OpenWritable(f.Path))
        {
            Assert.Equal(new(1, 10), upgraded.Watermark);
            upgraded.CreateWriter().Commit(f.Batch(2, 2));
        }
        using var reopened = SessionStore.OpenReadOnly(f.Path);
        Assert.Equal(new(2, 20), reopened.Watermark);
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

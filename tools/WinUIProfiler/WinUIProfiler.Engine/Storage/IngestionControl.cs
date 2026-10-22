using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using static WinUIProfiler.Engine.Storage.SessionStore;

namespace WinUIProfiler.Engine.Storage;

public sealed partial class SessionWriter
{
    // No fabricated source ordinal: diagnostic aggregates and terminal metadata bind to the existing durable checkpoint.
    public CommittedWatermark CommitHealth(HealthSnapshot health, SessionInfo? terminal = null, DataQuality? quality = null)
    {
        ArgumentNullException.ThrowIfNull(health);
        lock (store.Gate)
        {
            store.CheckDisposed();
            if (store.Failed) throw new InvalidOperationException("A failed recording cannot resume.");
            var db = store.WriterConnection!;
            var prior = ReadCheckpoint(db, null);
            if (ReadTerminal(db) is not null || prior.Info.Status != SessionStatus.Recording)
                throw new InvalidOperationException("The recording is already terminal.");
            if (terminal is not null)
            {
                if (quality is null || terminal.Status == SessionStatus.Recording || terminal.Mode != prior.Info.Mode || terminal.QpcFrequency != prior.Info.QpcFrequency)
                    throw new ArgumentException("Terminal metadata must retain mode/clock and name a terminal state.");
                terminal = QueryResultValidation.Freeze(terminal, new(store.Scope, prior.Watermark, quality));
                _ = EncodeBounded(terminal); _ = EncodeBounded(quality);
            }
            try
            {
                using var transaction = db.BeginTransaction();
                foreach (var issue in health.Issues)
                {
                    var json = Encode(issue.First);
                    if (System.Text.Encoding.UTF8.GetByteCount(json) > 16384) throw new ArgumentException("Diagnostic sample exceeds its bound.");
                    Execute(db, """
                        INSERT INTO IngestionDiagnostics VALUES($issue,$count,$ordinal,1,$sample)
                        ON CONFLICT(Issue) DO UPDATE SET Count=max(Count,excluded.Count),CommittedOrdinal=excluded.CommittedOrdinal
                        """, transaction, ("$issue", (int)issue.Issue), ("$count", issue.Count),
                        ("$ordinal", prior.Watermark.IngestOrdinal), ("$sample", json));
                }
                foreach (var sample in health.Samples)
                    Execute(db, """
                        INSERT INTO IngestionDiagnosticSamples VALUES($key,$count,$ordinal,1,$sample)
                        ON CONFLICT(SampleKey) DO UPDATE SET Count=max(Count,excluded.Count),CommittedOrdinal=excluded.CommittedOrdinal
                        """, transaction, ("$key", sample.Key), ("$count", sample.Count),
                        ("$ordinal", prior.Watermark.IngestOrdinal), ("$sample", Encode(sample.Diagnostic)));
                using (var count = Command(db, "SELECT count(*) FROM IngestionDiagnosticSamples", transaction))
                    if ((long)count.ExecuteScalar()! > SessionHealth.MaximumDiagnosticSamples)
                        throw new InvalidDataException("Persisted diagnostic capacity exceeded; no history is evicted.");
                if (terminal is not null)
                    Execute(db, "INSERT INTO SessionTerminal VALUES(1,$ordinal,$qpc,1,$info,$quality)", transaction,
                        ("$ordinal", prior.Watermark.IngestOrdinal), ("$qpc", prior.Watermark.TimestampQpc),
                        ("$info", EncodeBounded(terminal)), ("$quality", EncodeBounded(quality)));
                transaction.Commit();
                return prior.Watermark;
            }
            catch (Exception error) when (error is SqliteException or IOException or UnauthorizedAccessException)
            {
                throw new SessionWriteException(error, store.RecordFailure(prior.Watermark, error));
            }
        }
    }
}

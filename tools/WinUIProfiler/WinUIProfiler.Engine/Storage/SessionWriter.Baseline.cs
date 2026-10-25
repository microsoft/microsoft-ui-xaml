using System.Text;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using static WinUIProfiler.Engine.Storage.SessionStore;

namespace WinUIProfiler.Engine.Storage;

public sealed partial class SessionWriter
{
    public const int MaximumBaselineRecords = MaximumBatchRecords * 32;
    public const long MaximumBaselineBytes = (long)MaximumBatchBytes * 4;

    public CommittedWatermark CommitBaseline(NormalizedEvent end, SessionInfo info, DataQuality quality,
        IEnumerable<IReadOnlyList<ProjectionChange>> pages, SourceProvenance? provenance = null,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        Require.NotNull(pages);
        lock (store.Gate)
        {
            store.CheckDisposed();
            if (store.Failed) throw new InvalidOperationException("A failed recording cannot resume.");
            var db = store.WriterConnection!;
            var previous = ReadCheckpoint(db, null);
            if (previous.Info.Status != SessionStatus.Recording || ReadTerminal(db) is not null)
                throw new InvalidOperationException("The recording is already terminal.");
            var header = PrepareBaselineHeader(end, store.Scope, previous.Watermark, info, quality, provenance);
            var source = header.Source;
            var e = source.Envelope;
            var context = header.Context;
            var boundary = context.Watermark;
            var frozenInfo = header.Info;
            if (frozenInfo.Mode != previous.Info.Mode || frozenInfo.QpcFrequency != previous.Info.QpcFrequency)
                throw new ArgumentException("Session mode and QPC frequency cannot change.");
            long totalBytes = header.Bytes;
            var totalRecords = 0;
            try
            {
                using var transaction = db.BeginTransaction();
                InsertCheckpoint(db, transaction, boundary, frozenInfo, quality);
                PersistClocks(db, transaction, provenance?.Clock is { } clock ? [clock] : [],
                    frozenInfo.QpcFrequency, e.IngestOrdinal);
                InsertSource(db, transaction, boundary, source);
                if (provenance is not null) InsertProvenance(db, transaction, e.IngestOrdinal, provenance);
                // The existing unique index checks cross-page keys without retaining the capture's rows in RAM.
                using var duplicate = Command(db,
                    "SELECT 1 FROM ProjectionRows WHERE Dataset=$dataset AND RowKey=$key AND SourceOrdinal=$source LIMIT 1",
                    transaction, ("$dataset", null), ("$key", null), ("$source", e.IngestOrdinal));
                foreach (var page in pages)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    if (page is null || page.Count is < 1 or > MaximumBatchRecords)
                        throw new ArgumentException("Baseline page record bound exceeded.");
                    var declaredCount = page.Count;
                    var pageRecords = 0;
                    long pageBytes = 0;
                    foreach (var change in page)
                    {
                        cancellationToken.ThrowIfCancellationRequested();
                        if (++pageRecords > declaredCount)
                            throw new ArgumentException("Baseline pages require bounded, typed changes referencing this End.");
                        var frozen = FreezeBaselineChange(change, source, context);
                        pageBytes += frozen.Bytes; totalBytes += frozen.Bytes;
                        if (++totalRecords > MaximumBaselineRecords || totalBytes > MaximumBaselineBytes || pageBytes > MaximumBatchBytes)
                            throw new ProcessingLimitException();
                        var row = ProjectionRow.Create(e.IngestOrdinal, e.TimestampQpc, frozen.Change.Value, frozen.Change.Deleted);
                        duplicate.Parameters["$dataset"].Value = row.Dataset;
                        duplicate.Parameters["$key"].Value = row.Key;
                        if (duplicate.ExecuteScalar() is not null)
                            throw new ArgumentException("A baseline End may contain only one revision per projection key.");
                        row.Insert(db, transaction, boundary);
                    }
                    if (pageRecords != declaredCount)
                        throw new ArgumentException("A baseline page changed while it was being consumed.");
                }
                Execute(db, "INSERT INTO QualityIntervals VALUES($commit,$start,$end,$kind,$coverage,$reason)", transaction,
                    ("$commit", e.IngestOrdinal), ("$start", e.TimestampQpc), ("$end", boundary.TimestampQpc),
                    ("$kind", (int)quality.Kind), ("$coverage", (int)quality.Coverage), ("$reason", quality.Reason));
                cancellationToken.ThrowIfCancellationRequested();
                transaction.Commit();
                return boundary;
            }
            catch (Exception error) when (error is SqliteException or IOException or UnauthorizedAccessException)
            {
                throw new SessionWriteException(error, store.RecordFailure(previous.Watermark, error));
            }
        }
    }

    internal static (NormalizedEvent Source, SessionInfo Info, ResultContext Context, long Bytes) PrepareBaselineHeader(
        NormalizedEvent end, SessionScope scope, CommittedWatermark previous, SessionInfo info,
        DataQuality quality, SourceProvenance? provenance)
    {
        Require.NotNull(end); Require.NotNull(info); Require.NotNull(quality);
        var source = EventCodec.Normalize(end.Envelope, end.Stream);
        if (source.Stream.ProviderId != EventCatalog.RuntimeProviderId || source.Envelope.Identity.EventId != 13 ||
            source.Envelope.SourceSequence is null or 0)
            throw new ArgumentException("Atomic baseline publication requires its actual runtime End source.");
        var capture = EventCodec.ReadPayload<CaptureBoundaryPayload>(source.Envelope);
        if (capture.Kind != CaptureKind.Baseline || capture.Phase != CapturePhase.End || capture.Quality.Coverage == CoverageState.Unavailable)
            throw new ArgumentException("Atomic baseline publication requires a baseline End, not another capture phase or kind.");
        var e = source.Envelope;
        if (e.SessionId != scope.SessionId || e.Process != scope.Process || e.IngestOrdinal != checked(previous.IngestOrdinal + 1))
            throw new ArgumentException("Baseline End must be the next canonical source in this session/process.");
        var context = new ResultContext(scope, new(e.IngestOrdinal, Math.Max(previous.TimestampQpc, e.TimestampQpc)), quality);
        var frozenInfo = QueryResultValidation.Freeze(info, context);
        var bytes = BatchAccounting.HeaderBytes + BatchAccounting.SourceBytes(source, null) +
            Encoding.UTF8.GetByteCount(EncodeBounded(frozenInfo)) + Encoding.UTF8.GetByteCount(EncodeBounded(quality)) +
            (provenance is null ? 0 : ValidateProvenance(source, provenance, frozenInfo.QpcFrequency));
        if (bytes > MaximumBatchBytes) throw new ArgumentException("Baseline metadata exceeds the page byte bound.");
        return (source, frozenInfo, context, bytes);
    }

    internal static (ProjectionChange Change, long Bytes, (string Dataset, string Key) Key) FreezeBaselineChange(
        ProjectionChange change, NormalizedEvent end, ResultContext context)
    {
        if (change is null || change.SourceOrdinal != end.Envelope.IngestOrdinal ||
            change.Value is not (ObjectHistoryEntry or TreeEntry or CompositionTreeEntry or CorrelationEntry or ResourceUsageEntry or FrameInfo or StartupEntry))
            throw new ArgumentException("Baseline pages require bounded, typed changes referencing this End.");
        var value = Freeze(change.Value, context);
        var bytes = BatchAccounting.JsonBytes(value, ProjectionBudget.MaximumRecordBytes) + BatchAccounting.ChangeHeaderBytes;
        var row = ProjectionRow.Create(change.SourceOrdinal, end.Envelope.TimestampQpc, value, change.Deleted);
        return (change with { Value = value }, bytes, (row.Dataset, row.Key));
    }
}

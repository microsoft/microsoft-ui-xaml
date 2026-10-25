using System.Security.Cryptography;
using System.Text;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;
using static WinUIProfiler.Engine.Storage.SessionStore;
using static WinUIProfiler.Engine.Storage.SessionWriter;

namespace WinUIProfiler.Engine.Query;

public sealed class ProfilerQueryService : IProfilerQueryService
{
    public const int MaximumMaterializedBytes = 16 * 1024 * 1024;
    private readonly SessionStore store;
    private readonly Func<SqliteConnection> openReader;
    internal ProfilerQueryService(SessionStore store, Func<SqliteConnection>? openReader = null)
    {
        this.store = store;
        this.openReader = openReader ?? store.OpenReader;
    }

    public ValueTask<QueryResult<SessionInfo>> GetSession(GetSessionRequest request, CancellationToken cancellationToken)
    {
        Validate(request, cancellationToken);
        using var db = openReader();
        return new(SqliteReadExecution.Run(db, cancellationToken, () => ReadSession(db)));
    }
    private QueryResult<SessionInfo> ReadSession(SqliteConnection db)
    {
        var checkpoint = ReadTerminal(db) ?? ReadCheckpoint(db, null);
        var info = checkpoint.Info;
        var quality = checkpoint.Quality;
        var failure = store.ReadFailure();
        if (failure is not null)
        {
            if (failure.Watermark.IngestOrdinal > checkpoint.Watermark.IngestOrdinal)
            {
                checkpoint = ReadCheckpoint(db, null);
                info = checkpoint.Info;
            }
            if (failure.Watermark != checkpoint.Watermark) throw new InvalidDataException("Failure record does not match the durable boundary.");
            info = info with { Status = SessionStatus.Failed, StopReason = failure.Reason };
            quality = new(MeasurementKind.Unavailable, CoverageState.Partial, failure.Reason);
        }
        return new(new(store.Scope, checkpoint.Watermark, quality), info);
    }
    public ValueTask<QueryPage<MetricSample>> GetMetrics(GetMetricsRequest request, CancellationToken cancellationToken) =>
        Page<MetricSample>(request, "Metric", cancellationToken);
    public ValueTask<QueryPage<StartupEntry>> GetStartupTimeline(GetStartupTimelineRequest request, CancellationToken cancellationToken) =>
        Page<StartupEntry>(request, "Startup", cancellationToken);
    public ValueTask<QueryPage<FrameInfo>> GetFrames(GetFramesRequest request, CancellationToken cancellationToken) =>
        Page<FrameInfo>(request, "Frame", cancellationToken);
    public ValueTask<QueryPage<ObjectHistoryEntry>> GetObjectHistory(GetObjectHistoryRequest request, CancellationToken cancellationToken) =>
        Page<ObjectHistoryEntry>(request, "History", cancellationToken);
    public ValueTask<QueryPage<TreeEntry>> GetXamlTree(GetXamlTreeRequest request, CancellationToken cancellationToken) =>
        Page<TreeEntry>(request, "Xaml", cancellationToken);
    public ValueTask<QueryPage<CompositionTreeEntry>> GetCompositionTree(GetCompositionTreeRequest request, CancellationToken cancellationToken) =>
        Page<CompositionTreeEntry>(request, "Composition", cancellationToken);
    public ValueTask<QueryPage<CorrelationEntry>> GetCorrelations(GetCorrelationsRequest request, CancellationToken cancellationToken) =>
        Page<CorrelationEntry>(request, "Correlation", cancellationToken);
    public ValueTask<QueryPage<ResourceUsageEntry>> GetResourceUsage(GetResourceUsageRequest request, CancellationToken cancellationToken) =>
        Page<ResourceUsageEntry>(request, "Resource", cancellationToken);
    public ValueTask<QueryPage<SnapshotMetadata>> GetSnapshots(GetSnapshotsRequest request, CancellationToken cancellationToken) =>
        Page<SnapshotMetadata>(request, "Snapshot", cancellationToken);
    public ValueTask<QueryPage<SnapshotDelta>> CompareSnapshots(CompareSnapshotsRequest request, CancellationToken cancellationToken)
    {
        Validate(request, cancellationToken);
        throw new NotSupportedException("Snapshot attribute compatibility and comparison require later snapshot processors. No delta has been observed.");
    }
    public ValueTask<QueryPage<RetentionPath>> GetRetentionPaths(GetRetentionPathsRequest request, CancellationToken cancellationToken)
    {
        Validate(request, cancellationToken);
        throw new NotSupportedException("Snapshot-scoped ownership graph validation and retention traversal require later deliverables. No proven path is available.");
    }

    private ValueTask<QueryPage<T>> Page<T>(object request, string dataset, CancellationToken cancellationToken) where T : IQueryValue
    {
        Validate(request, cancellationToken);
        using var db = openReader();
        return new(SqliteReadExecution.Run(db, cancellationToken, () => ReadPage<T>(db, request, dataset, cancellationToken)));
    }
    private QueryPage<T> ReadPage<T>(SqliteConnection db, object request, string dataset, CancellationToken cancellationToken) where T : IQueryValue
    {
        var page = RequestPage(request);
        using var transaction = db.BeginTransaction(deferred: true);
        var checkpoint = ReadCheckpoint(db, page.AtWatermark, transaction);
        var parameters = new List<(string Name, object? Value)>
        {
            ("$dataset", dataset), ("$boundary", checkpoint.Watermark.IngestOrdinal),
            ("$limit", page.Limit + 1), ("$offset", page.Offset)
        };
        var filters = new List<string>();
        var availability = new List<string>();
        long? at = null;
        void Add(string sql, string name, object? value, bool governsAvailability = false)
        {
            filters.Add(sql); parameters.Add((name, value));
            if (governsAvailability) availability.Add(sql);
        }
        void Range(TimeRange range, bool overlap = false)
        {
            if (range.StartQpc == range.EndQpc)
                Add(overlap ? "r.TimestampQpc<=$start AND (r.EndQpc>$start OR r.TimestampQpc=r.EndQpc AND r.TimestampQpc=$start)" : "r.TimestampQpc=$start", "$start", range.StartQpc);
            else
            {
                Add(overlap ? "(r.EndQpc>$start OR r.TimestampQpc=r.EndQpc AND r.TimestampQpc>=$start)" : "r.TimestampQpc>=$start", "$start", range.StartQpc);
                Add("r.TimestampQpc<$end", "$end", range.EndQpc);
            }
        }
        ObjectKey? root = null;
        switch (request)
        {
            case GetMetricsRequest r:
                Range(r.Range);
                if (r.Metric is { } metric) Add("r.Metric=$metric", "$metric", (int)metric, true);
                break;
            case GetFramesRequest r: Range(r.Range, true); break;
            case GetObjectHistoryRequest r:
                Range(r.Range); Add("r.ObjectKey=$object", "$object", ObjectId(r.Object), true); break;
            case GetXamlTreeRequest r: at = r.AtQpc; root = r.Root; break;
            case GetCompositionTreeRequest r: at = r.AtQpc; root = r.Root; break;
            case GetCorrelationsRequest r:
                at = r.AtQpc;
                Add("(r.ObjectKey=$object OR r.ParentKey=$object)", "$object", ObjectId(r.Object), true);
                filters.Add("(r.EndQpc>$at OR r.TimestampQpc=r.EndQpc AND r.TimestampQpc=$at)");
                break;
            case GetResourceUsageRequest r:
                at = Math.Min(checkpoint.Watermark.TimestampQpc,
                    r.Range.EndQpc == r.Range.StartQpc ? r.Range.EndQpc : r.Range.EndQpc - 1);
                if (r.Owner is { } owner) Add("r.OwnerKey=$owner", "$owner", ObjectId(owner), true);
                break;
            case GetSnapshotsRequest r:
                if (r.Kind is { } kind) Add("r.SnapshotKind=$kind", "$kind", (int)kind, true);
                break;
        }
        if (at is { } qpc)
        {
            if (qpc > checkpoint.Watermark.TimestampQpc) throw new ArgumentException("Point-in-time query extends beyond the committed timestamp.");
            Add("r.TimestampQpc<=$at", "$at", qpc, true);
        }
        var dominance = at is null ? "newer.SourceOrdinal>r.SourceOrdinal" :
            "(newer.TimestampQpc>r.TimestampQpc OR newer.TimestampQpc=r.TimestampQpc AND newer.SourceOrdinal>r.SourceOrdinal)";
        var latest = $"""
            r.CommitOrdinal<=$boundary AND r.Dataset=$dataset AND r.Deleted=0
            AND NOT EXISTS(SELECT 1 FROM ProjectionRows newer
                WHERE newer.Dataset=r.Dataset AND newer.RowKey=r.RowKey
                AND newer.CommitOrdinal<= $boundary AND {dominance}
            """;
        latest += at is null ? ")" : " AND newer.TimestampQpc<=$at)";
        if (root is { } selected)
        {
            parameters.Add(("$root", ObjectId(selected)));
            availability.Add("r.ObjectKey=$root");
            filters.Add($"""
                r.ObjectKey IN (
                    WITH RECURSIVE descendants(k,Expand) AS (
                        SELECT $root,1 UNION
                        SELECT r.ObjectKey,{(dataset == "Xaml" ? "json_extract(r.PayloadJson,'$.connected')" : "1")}
                        FROM ProjectionRows r JOIN descendants d ON r.ParentKey=d.k
                        WHERE d.Expand=1 AND {latest} AND r.TimestampQpc<=$at
                        {(dataset == "Xaml" ? """
                        AND NOT EXISTS(SELECT 1 FROM ObjectLifetimes l JOIN ProjectionRows h ON h.Id=l.ProjectionId
                            WHERE (l.ObjectKey=r.ObjectKey OR l.ObjectKey=r.ParentKey) AND l.Operation=1
                            AND h.CommitOrdinal<=$boundary AND h.TimestampQpc<=$at)
                        """ : "")}
                    ) SELECT k FROM descendants)
                """);
        }
        if (dataset == "Xaml")
        {
            // A qualified object cannot be reborn after destruction. Late topology must not
            // resurrect it, even when the destruction arrived before its first tree edge.
            filters.Add("""
                NOT EXISTS(SELECT 1 FROM ObjectLifetimes l JOIN ProjectionRows h ON h.Id=l.ProjectionId
                    WHERE (l.ObjectKey=r.ObjectKey OR l.ObjectKey=r.ParentKey) AND l.Operation=1
                    AND h.CommitOrdinal<=$boundary AND h.TimestampQpc<=$at)
                """);
        }
        using var observed = Command(db, "SELECT EXISTS(SELECT 1 FROM ProjectionRows r WHERE r.Dataset=$dataset AND r.CommitOrdinal<=$boundary" +
            (availability.Count == 0 ? "" : " AND " + string.Join(" AND ", availability)) + ")", transaction, parameters.ToArray());
        var hasObservations = Convert.ToInt64(observed.ExecuteScalar()) != 0;
        var quality = hasObservations ? checkpoint.Quality :
            new DataQuality(MeasurementKind.Unavailable, CoverageState.Unavailable, $"No explicit {dataset} projection has been committed for this selection; later processors must supply observations.");
        var context = new ResultContext(store.Scope, checkpoint.Watermark, quality);
        using var command = Command(db, $"""
            SELECT r.PayloadJson,r.PayloadVersion,length(CAST(r.PayloadJson AS BLOB)) FROM ProjectionRows r WHERE {latest}
            {(filters.Count == 0 ? "" : " AND " + string.Join(" AND ", filters))}
            ORDER BY r.TimestampQpc,r.SourceOrdinal,r.RowKey LIMIT $limit OFFSET $offset
            """, transaction, parameters.ToArray());
        var items = new List<T>(page.Limit);
        var hasMore = false;
        long bytes = 0;
        using (var rows = command.ExecuteReader())
        {
            while (rows.Read())
            {
                cancellationToken.ThrowIfCancellationRequested();
                if (items.Count == page.Limit) { hasMore = true; break; }
                if (rows.GetInt32(1) != 1) throw new InvalidDataException("Unsupported projection payload version.");
                var length = rows.GetInt64(2);
                if (length > ContractVersions.MaximumPayloadBytes) throw new InvalidDataException("Projection exceeds its payload bound.");
                bytes += length;
                if (bytes > MaximumMaterializedBytes) throw new QueryMaterializationLimitException("Page exceeds the 16 MiB materialization budget. Reduce the requested limit.");
                items.Add(Json<T>(rows.GetString(0)));
            }
        }
        if (dataset == "Xaml")
        {
            var losses = ReadXamlLosses(db, transaction, checkpoint.Watermark, cancellationToken, ref bytes);
            for (var i = 0; i < items.Count; i++)
                items[i] = (T)(IQueryValue)EnrichXaml(db, transaction, (TreeEntry)(IQueryValue)items[i],
                    at!.Value, checkpoint.Watermark, losses, cancellationToken, ref bytes);
        }
        cancellationToken.ThrowIfCancellationRequested();
        return new(context, items, new(page.Limit, page.Offset, hasMore ? page.Offset + page.Limit : null, hasMore));
    }

    private sealed record XamlGap(Guid? Epoch, long? StartQpc, long? EndQpc);
    private static IReadOnlyList<XamlGap> ReadXamlLosses(SqliteConnection db, SqliteTransaction transaction,
        CommittedWatermark watermark, CancellationToken cancellationToken, ref long bytes)
    {
        using var command = Command(db, """
            SELECT Envelope,length(Envelope),ProviderId FROM SourceEvents
            WHERE CommitOrdinal<=$boundary AND
            ((ProviderId=$runtime AND EventId=15) OR (ProviderId=$collector AND EventId=3))
            """, transaction, ("$boundary", watermark.IngestOrdinal),
            ("$runtime", EventCatalog.RuntimeProviderId.ToString("N")), ("$collector", EventCatalog.CollectorProviderId.ToString("N")));
        var result = new List<XamlGap>();
        using (var rows = command.ExecuteReader())
        {
            while (rows.Read())
            {
                cancellationToken.ThrowIfCancellationRequested();
                bytes += rows.GetInt64(1);
                if (bytes > MaximumMaterializedBytes) throw new QueryMaterializationLimitException("Tree continuity evidence exceeds the materialization budget.");
                var envelope = EventCodec.Decode((byte[])rows[0]);
                var loss = EventCodec.ReadPayload<LossPayload>(envelope);
                var collector = rows.GetString(2) == EventCatalog.CollectorProviderId.ToString("N");
                if (collector || loss.Domains is null || loss.Domains.Count == 0 ||
                    loss.Domains.Any(d => d.EventId is 2 or 3 or 4 or 5 or 16))
                    result.Add(new(collector ? null : loss.ProducerEpoch, loss.Interval.StartQpc, loss.Interval.EndQpc));
            }
        }
        // Final invalidation may have no subsequent source on which to hang a revision.
        // Persisted diagnostics have optional real timestamps; unknown intervals stay unknown.
        using var diagnostics = Command(db, """
            SELECT SampleJson,length(CAST(SampleJson AS BLOB)) FROM IngestionDiagnostics
            WHERE CommittedOrdinal<=$boundary
            """, transaction, ("$boundary", watermark.IngestOrdinal));
        using (var rows = diagnostics.ExecuteReader())
        {
            while (rows.Read())
            {
                cancellationToken.ThrowIfCancellationRequested();
                bytes += rows.GetInt64(1);
                if (bytes > MaximumMaterializedBytes) throw new QueryMaterializationLimitException("Tree diagnostic evidence exceeds the materialization budget.");
                var diagnostic = Json<IngestionDiagnostic>(rows.GetString(0));
                if (diagnostic.Issue is not (IngestionIssue.IdentityPartial or IngestionIssue.MissingMetadata or IngestionIssue.ProducerLoss))
                    result.Add(new(null, diagnostic.TimestampQpc, null));
            }
        }
        return result;
    }

    private static TreeEntry EnrichXaml(SqliteConnection db, SqliteTransaction transaction, TreeEntry tree,
        long at, CommittedWatermark watermark, IReadOnlyList<XamlGap> losses,
        CancellationToken cancellationToken, ref long bytes)
    {
        using var metadata = Command(db, """
            SELECT PayloadJson,length(CAST(PayloadJson AS BLOB)) FROM ProjectionRows
            WHERE Id IN (
                (SELECT Id FROM ProjectionRows
                    WHERE Dataset='History' AND ObjectKey=$object AND CommitOrdinal<=$boundary AND TimestampQpc<=$at
                    AND json_extract(PayloadJson,'$.observation.kind') IN ('type','lifetime')
                    AND coalesce(json_extract(PayloadJson,'$.observation.name'),
                        json_extract(PayloadJson,'$.observation.typeName')) IS NOT NULL
                    ORDER BY TimestampQpc DESC,SourceOrdinal DESC LIMIT 1),
                (SELECT Id FROM ProjectionRows
                    WHERE Dataset='History' AND ObjectKey=$object AND CommitOrdinal<=$boundary AND TimestampQpc<=$at
                    AND json_extract(PayloadJson,'$.observation.kind') IN ('type','lifetime')
                    AND json_extract(PayloadJson,'$.observation.classification') IS NOT NULL
                    ORDER BY TimestampQpc DESC,SourceOrdinal DESC LIMIT 1))
            ORDER BY TimestampQpc,SourceOrdinal
            """, transaction, ("$object", ObjectId(tree.Object)), ("$boundary", watermark.IngestOrdinal), ("$at", at));
        using (var row = metadata.ExecuteReader())
            while (row.Read())
            {
                cancellationToken.ThrowIfCancellationRequested();
                bytes += row.GetInt64(1);
                if (bytes > MaximumMaterializedBytes) throw new QueryMaterializationLimitException("Tree type evidence exceeds the materialization budget.");
                var history = Json<ObjectHistoryEntry>(row.GetString(0));
                tree = history.Observation switch
                {
                    TypeObservation t => tree with { TypeName = t.Name, Classification = t.Classification ?? tree.Classification },
                    LifetimeObservation l => tree with { TypeName = l.TypeName ?? tree.TypeName, Classification = l.Classification ?? tree.Classification },
                    _ => tree
                };
            }
        using var observed = Command(db, """
            SELECT TimestampQpc FROM ProjectionRows WHERE Dataset='Xaml' AND ObjectKey=$object
            AND CommitOrdinal<=$boundary AND TimestampQpc<=$at
            AND RowKey=$key
            ORDER BY TimestampQpc DESC,SourceOrdinal DESC LIMIT 1
            """, transaction, ("$object", ObjectId(tree.Object)), ("$boundary", watermark.IngestOrdinal),
            ("$at", at), ("$key", $"{ObjectId(tree.Object)}:{(int)tree.Relationship}:{(int)tree.Membership}"));
        var timestamp = Convert.ToInt64(observed.ExecuteScalar());
        if (losses.Any(l => (l.Epoch is null || l.Epoch == tree.Object.ProducerEpoch) &&
            (l.StartQpc is null || l.StartQpc <= at) && (l.EndQpc is null || l.EndQpc >= timestamp)))
            tree = tree with { Quality = Processing.ProjectionSupport.Uncertain("Membership is unknown across a reported loss interval; a fresh observation is required.") };
        return tree;
    }

    private void Validate(object request, CancellationToken cancellationToken)
    {
        QueryValidation.Validate(request, cancellationToken);
        var scope = request switch
        {
            GetSessionRequest r => r.Scope, GetMetricsRequest r => r.Scope, GetStartupTimelineRequest r => r.Scope,
            GetFramesRequest r => r.Scope, GetObjectHistoryRequest r => r.Scope, GetXamlTreeRequest r => r.Scope,
            GetCompositionTreeRequest r => r.Scope, GetCorrelationsRequest r => r.Scope, GetResourceUsageRequest r => r.Scope,
            GetSnapshotsRequest r => r.Scope, CompareSnapshotsRequest r => r.Scope, GetRetentionPathsRequest r => r.Scope,
            _ => throw new ArgumentException("Unknown query.")
        };
        if (scope != store.Scope) throw new ArgumentException("Query session/process does not match this database.");
        store.CheckDisposed();
    }
    private static PageRequest RequestPage(object request) => request switch
    {
        GetMetricsRequest r => r.Page, GetStartupTimelineRequest r => r.Page, GetFramesRequest r => r.Page,
        GetObjectHistoryRequest r => r.Page, GetXamlTreeRequest r => r.Page, GetCompositionTreeRequest r => r.Page,
        GetCorrelationsRequest r => r.Page, GetResourceUsageRequest r => r.Page, GetSnapshotsRequest r => r.Page,
        CompareSnapshotsRequest r => r.Page, GetRetentionPathsRequest r => r.Page,
        _ => throw new ArgumentException("Not a paged query.")
    };
    private static string Fingerprint(object request)
    {
        var page = new PageRequest(RequestPage(request).Limit);
        object canonical = request switch
        {
            GetMetricsRequest r => r with { Page = page }, GetStartupTimelineRequest r => r with { Page = page },
            GetFramesRequest r => r with { Page = page }, GetObjectHistoryRequest r => r with { Page = page },
            GetXamlTreeRequest r => r with { Page = page }, GetCompositionTreeRequest r => r with { Page = page },
            GetCorrelationsRequest r => r with { Page = page }, GetResourceUsageRequest r => r with { Page = page },
            GetSnapshotsRequest r => r with { Page = page }, CompareSnapshotsRequest r => r with { Page = page },
            GetRetentionPathsRequest r => r with { Page = page },
            _ => throw new ArgumentException("Not a paged query.")
        };
        var json = request.GetType().Name + ":" + System.Text.Json.JsonSerializer.Serialize(canonical, canonical.GetType(), WireJson.Options);
        return Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(json)));
    }
    public string? Continuation(object request, PageInfo page, ResultContext context)
    {
        Validate(request, default);
        if (context.Scope != store.Scope || page.Limit != RequestPage(request).Limit || page.Offset != RequestPage(request).Offset ||
            RequestPage(request).AtWatermark is { } boundary && boundary != context.Watermark)
            throw new ArgumentException("Page/context does not match the request.");
        using var db = store.OpenReader(); _ = ReadCheckpoint(db, context.Watermark);
        if (page.NextOffset is not { } offset) return null;
        var body = Encoding.UTF8.GetBytes(Encode(new ContinuationBody(1, Fingerprint(request), context.Watermark, offset)));
        return Convert.ToBase64String(body) + "." + Convert.ToHexString(HMACSHA256.HashData(store.TokenKey, body));
    }
    public PageRequest Resume(object request, string token)
    {
        Validate(request, default);
        if (token is null || token.Length > 4096) throw new ArgumentException("Invalid continuation token.");
        try
        {
            var parts = token.Split('.');
            if (parts.Length != 2) throw new FormatException();
            var body = Convert.FromBase64String(parts[0]);
            if (!CryptographicOperations.FixedTimeEquals(Convert.FromHexString(parts[1]), HMACSHA256.HashData(store.TokenKey, body)))
                throw new FormatException();
            var continuation = Json<ContinuationBody>(Encoding.UTF8.GetString(body));
            if (continuation.Version != 1 || continuation.Fingerprint != Fingerprint(request) ||
                RequestPage(request).AtWatermark is { } boundary && boundary != continuation.Watermark)
                throw new FormatException();
            var page = new PageRequest(RequestPage(request).Limit, continuation.Offset, continuation.Watermark);
            if (page.Offset < 1 || page.Offset > long.MaxValue - page.Limit) throw new FormatException();
            using var db = store.OpenReader(); _ = ReadCheckpoint(db, page.AtWatermark);
            return page;
        }
        catch (Exception error) when (error is FormatException or System.Text.Json.JsonException or InvalidDataException)
        { throw new ArgumentException("Invalid continuation token or changed query filters.", nameof(token), error); }
    }
    private sealed record ContinuationBody(int Version, string Fingerprint, CommittedWatermark Watermark, long Offset);
}

public sealed class QueryMaterializationLimitException(string message) : InvalidOperationException(message);

using System.Text;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using static WinUIProfiler.Engine.Storage.SessionStore;

namespace WinUIProfiler.Engine.Storage;

public sealed partial class SessionWriter
{
    private SnapshotFact[] SnapshotFacts(SessionBatch batch, ProjectionRow[] projections,
        Dictionary<long, NormalizedEvent> sources, SqliteConnection db, CommittedWatermark previous)
    {
        var attributes = batch.SnapshotAttributes?.ToArray() ?? [];
        var edges = batch.RetentionEdges?.ToArray() ?? [];
        if ((long)attributes.Length + edges.Length + projections.Length > MaximumBatchRecords)
            throw new ArgumentException("Projection/fact record bound exceeded.");
        var knownSnapshots = projections.Where(row => row.Value is SnapshotMetadata && !row.Deleted)
            .Select(row => ((SnapshotMetadata)row.Value).SnapshotId).ToHashSet();
        void Snapshot(Guid id)
        {
            Require.Id(id);
            if (knownSnapshots.Contains(id)) return;
            using var command = Command(db, """
                SELECT EXISTS(SELECT 1 FROM ProjectionRows r WHERE r.Dataset='Snapshot' AND r.SnapshotId=$id
                AND r.CommitOrdinal<=$boundary AND r.Deleted=0 AND NOT EXISTS(
                    SELECT 1 FROM ProjectionRows n WHERE n.Dataset=r.Dataset AND n.RowKey=r.RowKey
                    AND n.CommitOrdinal<=$boundary AND n.SourceOrdinal>r.SourceOrdinal))
                """, null, ("$id", id.ToString("N")), ("$boundary", previous.IngestOrdinal));
            if (Convert.ToInt64(command.ExecuteScalar()) == 0) throw new ArgumentException("Snapshot fact refers to an unknown snapshot.");
            knownSnapshots.Add(id);
        }
        long Source(long ordinal)
        {
            if (!sources.TryGetValue(ordinal, out var source)) throw new ArgumentException("Snapshot fact requires a source in this batch.");
            return source.Envelope.TimestampQpc;
        }
        void Object(ObjectKey key)
        {
            key.Validate();
            if (key.Process != store.Scope.Process) throw new ArgumentException("Snapshot fact belongs to another process.");
        }
        var facts = new List<SnapshotFact>();
        foreach (var attribute in attributes)
        {
            ArgumentNullException.ThrowIfNull(attribute);
            Snapshot(attribute.SnapshotId);
            var key = Require.NotNull(attribute.Key); Require.Enum(key.Attribute); Require.Enum(key.Grouping);
            if (key.Object is { } obj) Object(obj);
            if (key.RootId is { } root) Require.Id(root);
            if (key.TypeName is { } type) Require.Text(type);
            if (key.ResourceKind is { } resource) Require.Enum(resource);
            var qualifiers = (key.Object is null ? 0 : 1) + (key.RootId is null ? 0 : 1) +
                (key.TypeName is null ? 0 : 1) + (key.ResourceKind is null ? 0 : 1);
            if (key.Grouping switch
                {
                    ComparisonGrouping.Session => qualifiers != 0,
                    ComparisonGrouping.Object => qualifiers != 1 || key.Object is null,
                    ComparisonGrouping.Root => qualifiers != 1 || key.RootId is null,
                    ComparisonGrouping.Type => qualifiers != 1 || key.TypeName is null,
                    ComparisonGrouping.ResourceKind => qualifiers != 1 || key.ResourceKind is null,
                    _ => true
                })
                throw new ArgumentException("Snapshot attribute has an ambiguous grouping key.");
            var value = Require.NotNull(attribute.Value); value.Validate();
            bool count = key.Attribute is SnapshotAttribute.ObjectCount or SnapshotAttribute.ConnectedElementCount or SnapshotAttribute.CanonicalVisualCount;
            if (value.Unit != (count ? ValueUnit.Count : ValueUnit.Bytes) || value.Kind is not (ValueKind.Integer or ValueKind.Number or ValueKind.Missing) ||
                count && value.Kind == ValueKind.Number || value.Integer is < 0 || value.Number is < 0)
                throw new ArgumentException("Snapshot attributes must retain nonnegative numeric values or explicit missing values and the attribute's units.");
            facts.Add(new(attribute.SourceOrdinal, Source(attribute.SourceOrdinal), attribute.SnapshotId,
                "SnapshotAttribute", Encode(key), Encode(attribute), attribute, null));
        }
        foreach (var edge in edges)
        {
            ArgumentNullException.ThrowIfNull(edge); Snapshot(edge.SnapshotId);
            var value = Require.NotNull(edge.Edge); Object(value.From); Object(value.To);
            Require.Text(value.Relationship); Require.Enum(value.Evidence); Require.NotNull(value.Quality);
            facts.Add(new(edge.SourceOrdinal, Source(edge.SourceOrdinal), edge.SnapshotId, "RetentionEdge",
                Encode(new EdgeIdentity(value.From, value.To, value.Relationship)), Encode(edge), null, edge));
        }
        if (facts.Any(fact => Encoding.UTF8.GetByteCount(fact.Payload) > ContractVersions.MaximumPayloadBytes))
            throw new ArgumentException("Snapshot fact exceeds its payload bound.");
        if (facts.Select(fact => (fact.Dataset, fact.SnapshotId, fact.Key, fact.Source)).Distinct().Count() != facts.Count)
            throw new ArgumentException("Duplicate snapshot fact revision.");
        return facts.ToArray();
    }
    private sealed record EdgeIdentity(ObjectKey From, ObjectKey To, string Relationship);
    private sealed record SnapshotFact(long Source, long Timestamp, Guid SnapshotId, string Dataset, string Key,
        string Payload, SnapshotAttributeChange? Attribute, RetentionEdgeChange? Edge)
    {
        internal void Insert(SqliteConnection db, SqliteTransaction transaction, CommittedWatermark boundary)
        {
            Execute(db, """
                INSERT INTO ProjectionRows(CommitOrdinal,SourceOrdinal,Dataset,RowKey,TimestampQpc,EndQpc,SnapshotId,Deleted,PayloadVersion,PayloadJson)
                VALUES($commit,$source,$dataset,$key,$qpc,$qpc,$snapshot,0,1,$payload)
                """, transaction, ("$commit", boundary.IngestOrdinal), ("$source", Source), ("$dataset", Dataset),
                ("$key", SnapshotId.ToString("N") + ":" + Key), ("$qpc", Timestamp), ("$snapshot", SnapshotId.ToString("N")), ("$payload", Payload));
            using var command = Command(db, "SELECT last_insert_rowid()", transaction);
            var id = (long)command.ExecuteScalar()!;
            if (Attribute is { } attribute)
            {
                var key = attribute.Key; var value = attribute.Value;
                Execute(db, """
                    INSERT INTO SnapshotAttributes VALUES($id,$snapshot,$attribute,$key,$group,$object,$type,$root,$resource,
                    $kind,$unit,$integer,$number,$boolean,$text,$measurement,$coverage,$reason)
                    """, transaction, ("$id", id), ("$snapshot", SnapshotId.ToString("N")), ("$attribute", (int)key.Attribute), ("$key", Key),
                    ("$group", (int)key.Grouping), ("$object", key.Object is { } obj ? ObjectId(obj) : null), ("$type", key.TypeName),
                    ("$root", key.RootId?.ToString("N")), ("$resource", key.ResourceKind is { } resource ? (int)resource : null),
                    ("$kind", (int)value.Kind), ("$unit", (int)value.Unit), ("$integer", value.Integer), ("$number", value.Number),
                    ("$boolean", value.Boolean is { } boolean ? boolean ? 1 : 0 : null), ("$text", value.Text),
                    ("$measurement", (int)value.Quality.Kind), ("$coverage", (int)value.Quality.Coverage), ("$reason", value.Quality.Reason));
            }
            if (Edge is { } edge)
            {
                var value = edge.Edge;
                Execute(db, "INSERT INTO RetentionEdges VALUES($id,0,$snapshot,$from,$to,$relationship,$evidence,$kind,$coverage,$reason)", transaction,
                    ("$id", id), ("$snapshot", SnapshotId.ToString("N")), ("$from", ObjectId(value.From)), ("$to", ObjectId(value.To)),
                    ("$relationship", value.Relationship), ("$evidence", (int)value.Evidence),
                    ("$kind", (int)value.Quality.Kind), ("$coverage", (int)value.Quality.Coverage), ("$reason", value.Quality.Reason));
            }
        }
    }
}

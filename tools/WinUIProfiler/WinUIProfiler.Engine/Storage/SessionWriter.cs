using System.Globalization;
using System.Text;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;
using static WinUIProfiler.Engine.Storage.SessionStore;

namespace WinUIProfiler.Engine.Storage;

public sealed partial class SessionWriter
{
    public const int MaximumBatchRecords = 4096;
    public const int MaximumBatchBytes = 16 * 1024 * 1024;
    private readonly SessionStore store;
    internal SessionWriter(SessionStore store) => this.store = store;

    public CommittedWatermark Commit(SessionBatch batch, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        ArgumentNullException.ThrowIfNull(batch);
        lock (store.Gate)
        {
            store.CheckDisposed();
            if (store.Failed) throw new InvalidOperationException("A failed recording cannot resume.");
            var db = store.WriterConnection!;
            var previous = ReadCheckpoint(db, null);
            if (previous.Info.Status != SessionStatus.Recording || ReadTerminal(db) is not null) throw new InvalidOperationException("The recording is already terminal.");
            ArgumentNullException.ThrowIfNull(batch.Sources); ArgumentNullException.ThrowIfNull(batch.Changes);
            ArgumentNullException.ThrowIfNull(batch.Quality);
            if (batch.Sources.Count is < 1 or > MaximumBatchRecords || batch.Changes.Count > MaximumBatchRecords)
                throw new ArgumentException("Batch record bound exceeded.");
            // Capture caller-owned lists before validation or persistence.
            var sources = batch.Sources.Select(source => EventCodec.Normalize(source.Envelope, source.Stream)).ToArray();
            long ordinal = previous.Watermark.IngestOrdinal;
            long timestamp = previous.Watermark.TimestampQpc;
            var sourceMap = new Dictionary<long, NormalizedEvent>();
            long bytes = BatchAccounting.HeaderBytes;
            foreach (var source in sources)
            {
                var e = source.Envelope;
                if (e.SessionId != store.Scope.SessionId || e.Process != store.Scope.Process || e.IngestOrdinal != checked(ordinal + 1))
                    throw new ArgumentException("Batch sources must be contiguous and belong to the selected session/process.");
                ordinal = e.IngestOrdinal; timestamp = Math.Max(timestamp, e.TimestampQpc);
                bytes += EventCodec.Encode(e).Length;
                sourceMap.Add(ordinal, source);
            }
            var boundary = new CommittedWatermark(ordinal, timestamp);
            var context = new ResultContext(store.Scope, boundary, batch.Quality);
            var info = QueryResultValidation.Freeze(batch.Info, context);
            _ = EncodeBounded(info); _ = EncodeBounded(batch.Quality);
            bytes += Encoding.UTF8.GetByteCount(EncodeBounded(info)) + Encoding.UTF8.GetByteCount(EncodeBounded(batch.Quality));
            if (info.Mode != previous.Info.Mode || info.QpcFrequency != previous.Info.QpcFrequency)
                throw new ArgumentException("Session mode and QPC frequency cannot change.");
            var changes = batch.Changes.Select(change =>
            {
                ArgumentNullException.ThrowIfNull(change);
                if (!sourceMap.TryGetValue(change.SourceOrdinal, out var source))
                    throw new ArgumentException("Every projection change needs a source in this transaction.");
                var value = Freeze(change.Value, context);
                var row = ProjectionRow.Create(change.SourceOrdinal, source.Envelope.TimestampQpc, value, change.Deleted);
                bytes += Encoding.UTF8.GetByteCount(row.Payload) + BatchAccounting.ChangeHeaderBytes;
                return row;
            }).ToArray();
            if (changes.Select(row => (row.Dataset, row.Key, row.Source)).Distinct().Count() != changes.Length)
                throw new ArgumentException("A source may contain only one revision per projection key.");
            var facts = SnapshotFacts(batch, changes, sourceMap, db, previous.Watermark);
            var provenance = batch.Provenance?.ToArray() ?? [];
            if (provenance.Length > MaximumBatchRecords || provenance.Select(p => p.SourceOrdinal).Distinct().Count() != provenance.Length)
                throw new ArgumentException("Provenance must be bounded and unique per source.");
            foreach (var p in provenance)
            {
                if (!sourceMap.TryGetValue(p.SourceOrdinal, out var source))
                    throw new ArgumentException("Provenance needs a source and its actual target PID.");
                bytes += ValidateProvenance(source, p.Value, info.QpcFrequency);
            }
            bytes += facts.Sum(fact => (long)Encoding.UTF8.GetByteCount(fact.Payload));
            if (bytes > MaximumBatchBytes) throw new ArgumentException("Batch byte bound exceeded.");
            cancellationToken.ThrowIfCancellationRequested();
            try
            {
                using var transaction = db.BeginTransaction();
                InsertCheckpoint(db, transaction, boundary, info, batch.Quality);
                PersistClocks(db, transaction, provenance.Select(p => p.Value.Clock).OfType<Ingestion.SourceClockMetadata>().Distinct().ToArray(),
                    info.QpcFrequency, boundary.IngestOrdinal);
                foreach (var source in sources)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    InsertSource(db, transaction, boundary, source);
                }
                foreach (var p in provenance)
                    InsertProvenance(db, transaction, p.SourceOrdinal, p.Value);
                foreach (var row in changes)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    row.Insert(db, transaction, boundary);
                }
                foreach (var fact in facts)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    fact.Insert(db, transaction, boundary);
                }
                Execute(db, "INSERT INTO QualityIntervals VALUES($commit,$start,$end,$kind,$coverage,$reason)", transaction,
                    ("$commit", ordinal), ("$start", sources.Min(source => source.Envelope.TimestampQpc)), ("$end", timestamp),
                    ("$kind", (int)batch.Quality.Kind), ("$coverage", (int)batch.Quality.Coverage), ("$reason", batch.Quality.Reason));
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

    internal static IQueryValue Freeze(IQueryValue value, ResultContext context) => value switch
    {
        MetricSample row => QueryResultValidation.Freeze(row, context),
        StartupEntry row => QueryResultValidation.Freeze(row, context),
        FrameInfo row => QueryResultValidation.Freeze(row, context),
        ObjectHistoryEntry row => QueryResultValidation.Freeze(row, context),
        TreeEntry row => QueryResultValidation.Freeze(row, context),
        CompositionTreeEntry row => QueryResultValidation.Freeze(row, context),
        CorrelationEntry row => QueryResultValidation.Freeze(row, context),
        ResourceUsageEntry row => QueryResultValidation.Freeze(row, context),
        SnapshotMetadata row => QueryResultValidation.Freeze(row, context),
        _ => throw new NotSupportedException("Only explicit persisted projections are accepted. Snapshot comparison and retention traversal require later processors.")
    };
    private static long ValidateProvenance(NormalizedEvent source, Ingestion.SourceProvenance value, long sessionFrequency)
    {
        value.OriginalTarget.Validate(); Require.Nonnegative(value.OriginalTimestampQpc);
        if (value.OriginalTarget.ProcessId != source.Envelope.Process.ProcessId)
            throw new ArgumentException("Provenance needs a source and its actual target PID.");
        if (value.Clock is { } clock)
        {
            clock.Validate();
            if (clock.Mapping.SessionFrequency != sessionFrequency || clock.Mapping.Convert(value.OriginalTimestampQpc) != source.Envelope.TimestampQpc)
                throw new ArgumentException("Provenance clock mapping does not match its normalized source.");
        }
        return BatchAccounting.JsonBytes(value, 4096);
    }
    private static void InsertProvenance(SqliteConnection db, SqliteTransaction transaction, long ordinal, Ingestion.SourceProvenance value) =>
        Execute(db, "INSERT INTO SourceProvenance VALUES($source,$pid,$creation,$instance,$qpc,$clock)", transaction,
            ("$source", ordinal), ("$pid", (long)value.OriginalTarget.ProcessId),
            ("$creation", value.OriginalTarget.CreationTimeFileTime), ("$instance", value.OriginalTarget.InstanceId.ToString("N")),
            ("$qpc", value.OriginalTimestampQpc), ("$clock", value.Clock?.SourceId.ToString("N")));

    private static void InsertSource(SqliteConnection db, SqliteTransaction transaction, CommittedWatermark boundary, NormalizedEvent source)
    {
        var e = source.Envelope;
        Execute(db, "INSERT INTO SourceEvents VALUES($ordinal,$commit,$instance,$qpc,$provider,$pid,$epoch,$event,$version,$sequence,1,$envelope)", transaction,
            ("$ordinal", e.IngestOrdinal), ("$commit", boundary.IngestOrdinal), ("$instance", e.Process.InstanceId.ToString("N")),
            ("$qpc", e.TimestampQpc), ("$provider", source.Stream.ProviderId.ToString("N")), ("$pid", (long)source.Stream.ProducerProcessId),
            ("$epoch", source.Stream.ProducerEpoch.ToString("N")), ("$event", e.Identity.EventId), ("$version", e.Identity.Version),
            ("$sequence", e.SourceSequence is { } sequence ? U64(sequence) : null), ("$envelope", EventCodec.Encode(e)));
        if (EventCatalog.Get(e.Identity.ProviderId, e.Identity.EventId).PayloadType == typeof(RuntimeCapabilitiesPayload))
        {
            foreach (var capability in EventCodec.ReadPayload<RuntimeCapabilitiesPayload>(e).Capabilities)
                Execute(db, "INSERT INTO Capabilities VALUES($source,$capability,$version,$kind,$coverage,$reason)", transaction,
                    ("$source", e.IngestOrdinal), ("$capability", (int)capability.Capability), ("$version", capability.Version),
                    ("$kind", (int)capability.Quality.Kind), ("$coverage", (int)capability.Quality.Coverage), ("$reason", capability.Quality.Reason));
        }
        if (EventCatalog.Get(e.Identity.ProviderId, e.Identity.EventId).PayloadType == typeof(CaptureBoundaryPayload))
        {
            var capture = EventCodec.ReadPayload<CaptureBoundaryPayload>(e);
            Execute(db, "INSERT INTO CaptureEpochs VALUES($source,$id,$kind,$phase,$root,$dispatcher,$payload)", transaction,
                ("$source", e.IngestOrdinal), ("$id", capture.CaptureId.ToString("N")), ("$kind", (int)capture.Kind), ("$phase", (int)capture.Phase),
                ("$root", capture.RootId?.ToString("N")), ("$dispatcher", capture.DispatcherId is { } d ? U64(d) : null), ("$payload", Encode(capture)));
        }
    }
    internal static string U64(ulong value) => value.ToString("X16", CultureInfo.InvariantCulture);
    internal static string ObjectId(ObjectKey key) => $"{key.Process.InstanceId:N}:{(int)key.Domain}:{key.ProducerEpoch:N}:{U64(key.RuntimeObjectId)}";

    private sealed class ProjectionRow
    {
        internal required IQueryValue Value { get; init; }
        internal required string Dataset { get; init; }
        internal required string Key { get; init; }
        internal required string Payload { get; init; }
        internal long Source { get; init; }
        internal long Timestamp { get; init; }
        internal long End { get; init; }
        internal ObjectKey? Object { get; init; }
        internal ObjectKey? Parent { get; init; }
        internal TreeEntry? Tree { get; init; }
        internal bool Deleted { get; init; }
        internal static ProjectionRow Create(long source, long qpc, IQueryValue value, bool deleted)
        {
            var (dataset, key, timestamp, end, obj, parent, tree) = Describe(source, qpc, value);
            if (value is ObjectHistoryEntry history && (history.IngestOrdinal != source || history.TimestampQpc != qpc))
                throw new ArgumentException("Object history must retain its exact source ordinal and timestamp.");
            if (deleted && value is MetricSample or ObjectHistoryEntry or StartupEntry)
                throw new ArgumentException("Immutable observations cannot be deleted.");
            var payload = System.Text.Json.JsonSerializer.Serialize(value, value.GetType(), WireJson.Options);
            if (Encoding.UTF8.GetByteCount(payload) > ContractVersions.MaximumPayloadBytes) throw new ArgumentException("Projection payload bound exceeded.");
            return new() { Source = source, Value = value, Dataset = dataset, Key = key, Timestamp = timestamp, End = end, Object = obj, Parent = parent, Tree = tree, Deleted = deleted, Payload = payload };
        }
        private static (string Dataset, string Key, long Timestamp, long End, ObjectKey? Object, ObjectKey? Parent, TreeEntry? Tree) Describe(long source, long qpc, IQueryValue value)
        {
            string TreeKey(TreeEntry tree) => $"{ObjectId(tree.Object)}:{(int)tree.Relationship}:{(int)tree.Membership}";
            return value switch
            {
                MetricSample r => ("Metric", source.ToString("D20", CultureInfo.InvariantCulture), r.TimestampQpc, r.Interval.EndQpc, null, null, null),
                StartupEntry r => ("Startup", $"{r.Phase}:{r.Range.StartQpc}", r.Range.StartQpc, r.Range.EndQpc, null, null, null),
                FrameInfo r => ("Frame", $"{r.Attempt.ProducerEpoch:N}:{U64(r.Attempt.AttemptId)}:{r.Attempt.RootId:N}:{U64(r.Attempt.DispatcherId)}", r.Range.StartQpc, r.Range.EndQpc, null, null, null),
                ObjectHistoryEntry r => ("History", $"{source:D20}:{ObjectId(r.Object)}:{ObservationKey(r.Observation)}", r.TimestampQpc, r.TimestampQpc, r.Object, null, null),
                TreeEntry r when r.Object.Domain == IdentityDomain.XamlCore => ("Xaml", TreeKey(r), qpc, qpc, r.Object, r.Parent, r),
                CompositionTreeEntry r => ("Composition", TreeKey(r.Topology), qpc, qpc, r.Topology.Object, r.Topology.Parent, r.Topology),
                CorrelationEntry r => ("Correlation", $"{ObjectId(r.From)}:{ObjectId(r.To)}:{(int)r.Kind}", r.Validity.StartQpc, r.Validity.EndQpc, r.From, r.To, null),
                ResourceUsageEntry r => ("Resource", $"{ObjectId(r.Resource)}:{(r.Owner is { } owner ? ObjectId(owner) : "-")}:{r.BackingAllocationId:N}", qpc, qpc, r.Resource, r.Owner, null),
                SnapshotMetadata r => ("Snapshot", r.SnapshotId.ToString("N"), r.Extent.StartedQpc, r.Extent.EndedQpc ?? r.Extent.StartedQpc, null, null, null),
                _ => throw new NotSupportedException("Unsupported persisted projection.")
            };
        }
        private static string ObservationKey(ObjectObservation observation) => observation switch
        {
            PropertyObservation r => $"property:{U64(r.Property.PropertyId)}:{r.Property.DeclaringType}:{r.Property.Name}",
            VisualPropertyObservation r => $"visual:{(int)r.Property}",
            MembershipObservation r => $"membership:{(int)r.Relationship}:{(int)r.Membership}",
            CorrelationObservation r => $"correlation:{ObjectId(r.Other)}:{(int)r.Relationship}:{r.IsFrom}",
            AllocationObservation r => $"allocation:{r.AllocatorEpoch:N}:{U64(r.AllocationId)}",
            BoundaryObservation r => $"boundary:{(int)r.Boundary}:{U64(r.OperationId)}:{(int)r.Phase}",
            ResourceObservation r => $"resource:{r.DeviceEpoch:N}:{r.BackingAllocationId:N}:{(r.Owner is { } owner ? ObjectId(owner) : "-")}",
            LifetimeObservation => "lifetime",
            TypeObservation => "type",
            _ => throw new NotSupportedException("Unknown observation.")
        };
        internal void Insert(SqliteConnection db, SqliteTransaction transaction, CommittedWatermark boundary)
        {
            Execute(db, """
                INSERT INTO ProjectionRows(CommitOrdinal,SourceOrdinal,Dataset,RowKey,TimestampQpc,EndQpc,
                ObjectKey,ParentKey,RuntimeId,Domain,RootId,DispatcherId,Metric,SnapshotId,SnapshotKind,OwnerKey,Deleted,PayloadVersion,PayloadJson)
                VALUES($commit,$source,$dataset,$key,$qpc,$end,$object,$parent,$runtime,$domain,$root,$dispatcher,$metric,$snapshot,$snapshotKind,$owner,$deleted,1,$payload)
                """, transaction,
                ("$commit", boundary.IngestOrdinal), ("$source", Source), ("$dataset", Dataset), ("$key", Key),
                ("$qpc", Timestamp), ("$end", End), ("$object", Object is { } obj ? ObjectId(obj) : null),
                ("$parent", Parent is { } parent ? ObjectId(parent) : null), ("$runtime", Object is { } o ? U64(o.RuntimeObjectId) : null),
                ("$domain", Object is { } d ? (int)d.Domain : null),
                ("$root", (Tree?.RootId ?? (Value as FrameInfo)?.Attempt.RootId)?.ToString("N")),
                ("$dispatcher", (Tree?.DispatcherId ?? (Value as FrameInfo)?.Attempt.DispatcherId) is { } dispatcher ? U64(dispatcher) : null),
                ("$metric", Value is MetricSample sample ? (int)sample.Metric : null),
                ("$snapshot", (Value as SnapshotMetadata)?.SnapshotId.ToString("N")),
                ("$snapshotKind", Value is SnapshotMetadata snapshot ? (int)snapshot.Kind : null),
                ("$owner", Value is ResourceUsageEntry { Owner: { } resourceOwner } ? ObjectId(resourceOwner) : null), ("$deleted", Deleted ? 1 : 0), ("$payload", Payload));
            using var idCommand = Command(db, "SELECT last_insert_rowid()", transaction);
            var id = (long)idCommand.ExecuteScalar()!;
            void Insert(string table, string columns, params object?[] values)
            {
                var parameters = values.Select((v, i) => ($"$v{i}", v)).Prepend(("$id", (object?)id)).ToArray();
                Execute(db, $"INSERT INTO {table}(ProjectionId,{columns}) VALUES($id,{string.Join(',', values.Select((_, i) => $"$v{i}"))})", transaction, parameters);
            }
            void Typed(string table, string columns, object?[] prefix, TypedValue typed) => Insert(table,
                columns + ",ValueKind,Unit,IntegerValue,NumberValue,BooleanValue,TextValue,Measurement,Coverage,Reason",
                [.. prefix, (int)typed.Kind, (int)typed.Unit, typed.Integer, typed.Number, typed.Boolean is { } b ? b ? 1 : 0 : null,
                    typed.Text, (int)typed.Quality.Kind, (int)typed.Quality.Coverage, typed.Quality.Reason]);
            void Relationship(ObjectKey from, ObjectKey? to, int kind, long start, long end) =>
                Insert("Relationships", "FromKey,ToKey,Relationship,RuntimeId,StartQpc,EndQpc",
                    ObjectId(from), to is { } t ? ObjectId(t) : null, kind, U64(from.RuntimeObjectId), start, end);
            if (Tree is { } tree)
            {
                Relationship(tree.Object, tree.Parent, (int)tree.Relationship, Timestamp, End);
                Insert("Roots", "RootId,DispatcherId,ObjectKey,Connected", tree.RootId.ToString("N"), U64(tree.DispatcherId), ObjectId(tree.Object), tree.Connected ? 1 : 0);
            }
            switch (Value)
            {
                case CompositionTreeEntry r:
                    foreach (var property in r.Properties)
                        Typed("ObjectProperties", "ObjectKey,PropertyId", [ObjectId(r.Topology.Object), U64((ulong)property.Property)], property.Value);
                    break;
                case MetricSample r:
                    Typed("MetricSamples", "Metric", [(int)r.Metric], r.Value);
                    break;
                case ObjectHistoryEntry r:
                    switch (r.Observation)
                    {
                        case LifetimeObservation l:
                            Insert("ObjectLifetimes", "ObjectKey,Operation,Address,TypeName", ObjectId(r.Object), (int)l.Operation, l.Address is { } address ? U64(address) : null, l.TypeName);
                            break;
                        case PropertyObservation p:
                            Typed("ObjectProperties", "ObjectKey,PropertyId", [ObjectId(r.Object), U64(p.Property.PropertyId)], p.Current);
                            break;
                        case VisualPropertyObservation p:
                            Typed("ObjectProperties", "ObjectKey,PropertyId", [ObjectId(r.Object), U64((ulong)p.Property)], p.Value);
                            break;
                        case AllocationObservation a:
                            Insert("ResourceAllocations", "AllocatorEpoch,AllocationId,PreviousAllocationId,Operation,RequestedBytes,ActualBytes,RequestedBytesHigh,RequestedBytesLow,ActualBytesHigh,ActualBytesLow",
                                a.AllocatorEpoch.ToString("N"), U64(a.AllocationId), a.PreviousAllocationId is { } previous ? U64(previous) : null,
                                (int)a.Operation, U64(a.RequestedBytes), a.ActualBytes is { } actual ? U64(actual) : null,
                                (long)(a.RequestedBytes >> 32), (long)(a.RequestedBytes & uint.MaxValue),
                                a.ActualBytes is { } high ? (long)(high >> 32) : null, a.ActualBytes is { } low ? (long)(low & uint.MaxValue) : null);
                            break;
                    }
                    break;
                case FrameInfo r:
                    Insert("FrameAttempts", "ProducerEpoch,AttemptId,RootId,DispatcherId,Status,HResult", r.Attempt.ProducerEpoch.ToString("N"), U64(r.Attempt.AttemptId),
                        r.Attempt.RootId.ToString("N"), U64(r.Attempt.DispatcherId), (int)r.AttemptStatus, r.HResult);
                    if (r.Submission is { } submission)
                        Insert("FrameSubmissions", "ProducerEpoch,FrameId,SubmittedQpc", submission.ProducerEpoch.ToString("N"), U64(submission.SubmittedFrameId), submission.SubmittedQpc);
                    foreach (var change in r.Changes)
                        Insert("FrameChanges", "IngestOrdinal,TimestampQpc,ObjectKey,Relation,PayloadJson", change.IngestOrdinal, change.TimestampQpc,
                            change.Object is { } changed ? ObjectId(changed) : null, (int)change.Relation, Encode(change));
                    break;
                case CorrelationEntry r:
                    Relationship(r.From, r.To, (int)r.Kind, r.Validity.StartQpc, r.Validity.EndQpc);
                    break;
                case ResourceUsageEntry r:
                    Typed("Resources", "ResourceKey,DeviceEpoch,AllocationId,ResourceKind,Provenance",
                        [ObjectId(r.Resource), r.DeviceEpoch.ToString("N"), r.BackingAllocationId?.ToString("N"), (int)r.Classification.Kind, (int)r.Provenance], r.Size);
                    if (r.Owner is { } owner) Insert("ResourceOwners", "ResourceKey,OwnerKey", ObjectId(r.Resource), ObjectId(owner));
                    break;
                case SnapshotMetadata r:
                    Insert("Snapshots", "SnapshotId,Kind,State,StartQpc,EndQpc", r.SnapshotId.ToString("N"), (int)r.Kind, (int)r.State, r.Extent.StartedQpc, r.Extent.EndedQpc);
                    if (r.Artifact is { } artifact)
                        Insert("SnapshotArtifacts", "SnapshotId,RelativePath,Sha256", r.SnapshotId.ToString("N"), artifact.RelativePath, artifact.Sha256);
                    break;
            }
        }
    }
}

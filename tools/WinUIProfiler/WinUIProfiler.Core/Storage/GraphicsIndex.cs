// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// Process-local DirectX lifetimes and explicit handle correlations. Like ObjectIndex and FrameIndex, this runs on
// the writer thread in the raw event's transaction. No timestamp/thread heuristic associates kernel and runtime devices.
internal sealed class GraphicsIndex : IDisposable
{
    private enum Kind { Device, Buffer, Texture1D, Texture2D, Texture3D, AdapterAllocation, DeviceAllocation }
    private enum Phase { Start, Stop, Rundown, Reference }
    private enum IdentitySpace { Device, Resource, AdapterAllocation, DeviceAllocation }
    private readonly record struct Identity(IdentitySpace Space, ulong Handle, ulong Qualifier = 0);
    private readonly record struct GlobalIdentity(ulong Adapter, ulong Allocation);

    private sealed class GlobalHistory
    {
        internal long? EndQpc { get; set; }
        internal long? TrimmedEndQpc { get; set; }
        internal long LastSeq { get; set; }
        internal int References { get; set; }
    }

    private sealed record Shape(Kind Kind, Phase Phase, Dictionary<string, int> Fields)
    {
        internal object? Value(ProfilerEvent data, string name) =>
            Fields.TryGetValue(name, out var index) ? data.Fields[index].Value : null;

        internal ulong? OptionalHandle(ProfilerEvent data, string name) => Value(data, name) switch
        {
            ulong value => value,
            uint value => value,
            long value => unchecked((ulong)value),
            int value => unchecked((uint)value),
            _ => null
        };

        internal ulong Handle(ProfilerEvent data, string name) => OptionalHandle(data, name) ?? 0;

        internal string? HandleText(ProfilerEvent data, string name) =>
            OptionalHandle(data, name) is { } handle ? $"0x{handle:x16}" : null;
    }

    private sealed class Lifetime(long id, Identity identity, Kind kind, Phase phase, long sequence, long qpc)
    {
        internal long Id { get; } = id;
        internal Identity Identity { get; } = identity;
        internal Kind Kind { get; } = kind;
        internal long FirstSeq { get; } = sequence;
        internal long FirstQpc { get; } = qpc;
        internal long KnownFromSeq { get; set; } = sequence;
        internal long KnownFromQpc { get; set; } = qpc;
        internal long LastQpc { get; set; } = qpc;
        internal long? StartQpc { get; set; } = phase == Phase.Start ? qpc : null;
        internal long? EndQpc { get; set; }
        internal bool ReferenceOnly { get; set; } = phase == Phase.Reference;
        internal long? DeviceId { get; set; }
        internal ulong Resource { get; set; }
        internal GlobalIdentity Global { get; set; }
        internal ulong RegisteredResource { get; set; }
        internal GlobalIdentity RegisteredGlobal { get; set; }
        internal Dictionary<GlobalIdentity, (GlobalHistory History, long? StartQpc, long? ConfirmedQpc)>? GlobalObservations { get; set; }
    }

    // Exactly one runtime resource may claim a kernel resource handle, or exactly one adapter allocation a global
    // handle. Multiple device allocations per resource/global allocation are legitimate.
    private sealed class Correlations(string kind, bool uniqueSource, ulong handle, ulong qualifier = 0)
    {
        internal string Kind { get; } = kind;
        internal bool UniqueSource { get; } = uniqueSource;
        internal ulong Handle { get; } = handle;
        internal ulong Qualifier { get; } = qualifier;
        internal HashSet<Lifetime> Sources { get; } = [];
        internal HashSet<Lifetime> Targets { get; } = [];
        internal Dictionary<(long Source, long Target), long> Open { get; } = [];
    }

    private readonly SqliteConnection connection;
    private readonly Dictionary<long, Shape?> shapes = [];
    private readonly Dictionary<Identity, Lifetime> active = [];
    private readonly Dictionary<long, Lifetime> activeDevices = [];
    private readonly Dictionary<ulong, Correlations> resources = [];
    private readonly Dictionary<GlobalIdentity, Correlations> globals = [];
    private readonly Dictionary<GlobalIdentity, GlobalHistory> globalHistory = [];
    private readonly Dictionary<Correlations, string> pending = [];
    private readonly SqliteCommand insertObject;
    private readonly SqliteCommand observeObject;
    private readonly SqliteCommand endObject;
    private readonly SqliteCommand history;
    private readonly SqliteCommand globalEnds;
    private readonly SqliteCommand insertEvent;
    private readonly SqliteCommand device;
    private readonly SqliteCommand deviceReference;
    private readonly SqliteCommand resource;
    private readonly SqliteCommand allocation;
    private readonly SqliteCommand insertLink;
    private readonly SqliteCommand endLink;
    private readonly SqliteCommand disjointLinks;
    private readonly SqliteCommand[] commands;
    private long objectCount;

    // Allow room for lifetime metadata, evidence and correlation rows before the writer trims a batch.
    internal static int EstimatedBytes(ProfilerEvent data) => TypeOf(data) is null ? 0 : 1_000;

    internal GraphicsIndex(SqliteConnection connection)
    {
        this.connection = connection;
        insertObject = Sql.Prepare(connection, """
            INSERT INTO GraphicsObjects (GraphicsObjectId, Kind, Handle, Qualifier, FirstSeq, FirstQpc,
                KnownFromSeq, KnownFromQpc, LastSeq, LastQpc, FirstObservation, StartSeq, StartQpc)
            VALUES ($id, $kind, $handle, $qualifier, $seq, $qpc, $seq, $qpc, $seq, $qpc, $phase, $startSeq, $startQpc)
            """, ("$id", SqliteType.Integer), ("$kind", SqliteType.Text), ("$handle", SqliteType.Text),
            ("$qualifier", SqliteType.Text), ("$seq", SqliteType.Integer), ("$qpc", SqliteType.Integer),
            ("$phase", SqliteType.Text), ("$startSeq", SqliteType.Integer), ("$startQpc", SqliteType.Integer));
        observeObject = Sql.Prepare(connection, """
            UPDATE GraphicsObjects SET LastSeq = $seq, LastQpc = MAX(LastQpc, $qpc),
                KnownFromSeq = CASE WHEN $qpc < KnownFromQpc THEN $seq ELSE KnownFromSeq END,
                KnownFromQpc = MIN(KnownFromQpc, $qpc),
                StartSeq = COALESCE(StartSeq, $startSeq), StartQpc = COALESCE(StartQpc, $startQpc)
            WHERE GraphicsObjectId = $id
            """, ("$id", SqliteType.Integer), ("$seq", SqliteType.Integer), ("$qpc", SqliteType.Integer),
            ("$startSeq", SqliteType.Integer), ("$startQpc", SqliteType.Integer));
        endObject = Sql.Prepare(connection, """
            UPDATE GraphicsObjects SET EndSeq = CASE WHEN StopSeq IS NULL THEN $seq ELSE EndSeq END,
                EndQpc = CASE WHEN StopSeq IS NULL THEN $qpc ELSE EndQpc END, EndReason = $reason,
                StopSeq = COALESCE(StopSeq, $stopSeq), StopQpc = COALESCE(StopQpc, $stopQpc)
            WHERE GraphicsObjectId = $id
            """, ("$id", SqliteType.Integer), ("$seq", SqliteType.Integer), ("$qpc", SqliteType.Integer),
            ("$reason", SqliteType.Text), ("$stopSeq", SqliteType.Integer), ("$stopQpc", SqliteType.Integer));
        // Keep Stop-only histories ambiguous when no earlier existence evidence rules out the incoming Start.
        history = Sql.Prepare(connection, """
            SELECT g.GraphicsObjectId, g.Kind, g.FirstSeq, g.FirstQpc, g.LastQpc, g.FirstObservation, g.StartQpc,
                g.EndSeq, g.EndQpc, g.DeviceId, g.StopQpc,
                CASE WHEN $earlier AND g.StartQpc IS NULL AND $qpc <= g.KnownFromQpc THEN (SELECT MAX(p.LastQpc) FROM GraphicsObjects p
                    WHERE p.Kind IN ($kind1, $kind2, $kind3, $kind4) AND p.Handle = g.Handle AND p.Qualifier = g.Qualifier
                        AND p.GraphicsObjectId <> g.GraphicsObjectId AND p.KnownFromQpc < g.KnownFromQpc
                        AND (p.StartQpc IS NOT NULL OR (p.StopQpc IS NOT NULL AND p.KnownFromQpc < $qpc)
                            OR p.Kind <> g.Kind OR p.DeviceId <> COALESCE(g.DeviceId, $device))) END,
                g.KnownFromSeq, g.KnownFromQpc
            FROM GraphicsObjects g
            WHERE g.Kind IN ($kind1, $kind2, $kind3, $kind4) AND g.Handle = $handle AND g.Qualifier = $qualifier
                AND (g.EndQpc >= $qpc OR g.GraphicsObjectId = $candidate)
            ORDER BY g.EndQpc, g.GraphicsObjectId
            """, ("$kind1", SqliteType.Text), ("$kind2", SqliteType.Text), ("$kind3", SqliteType.Text),
            ("$kind4", SqliteType.Text), ("$handle", SqliteType.Text), ("$qualifier", SqliteType.Text),
            ("$qpc", SqliteType.Integer), ("$device", SqliteType.Integer), ("$earlier", SqliteType.Integer),
            ("$candidate", SqliteType.Integer));
        globalEnds = Sql.Prepare(connection, """
            SELECT MAX(EndQpc) FROM GraphicsObjects
            WHERE Kind = 'AdapterAllocation' AND Handle = $handle AND Qualifier = $adapter
            """, ("$handle", SqliteType.Text), ("$adapter", SqliteType.Text));
        insertEvent = Sql.Prepare(connection,
            "INSERT INTO GraphicsObjectEvents (Seq, GraphicsObjectId, Role) VALUES ($seq, $id, $role)",
            ("$seq", SqliteType.Integer), ("$id", SqliteType.Integer), ("$role", SqliteType.Text));
        deviceReference = Sql.Prepare(connection, """
            SELECT 1 FROM GraphicsObjects
            WHERE GraphicsObjectId = $id AND Kind = 'Device' AND $qpc >= COALESCE(StartQpc, KnownFromQpc)
                AND (EndQpc IS NULL OR $qpc <= EndQpc)
            """, ("$id", SqliteType.Integer), ("$qpc", SqliteType.Integer));
        device = Sql.Prepare(connection, """
            UPDATE GraphicsObjects SET pIDXGIDevice = NULLIF(COALESCE($dxgiDevice, pIDXGIDevice), '0x0000000000000000'),
                pIDXGIAdapter = NULLIF(COALESCE($dxgiAdapter, pIDXGIAdapter), '0x0000000000000000'),
                hKMAdapter = NULLIF(COALESCE($kernelAdapter, hKMAdapter), '0x0000000000000000'),
                hUMAdapter = NULLIF(COALESCE($userAdapter, hUMAdapter), '0x0000000000000000'),
                hKMDevice = NULLIF(COALESCE($kernelDevice, hKMDevice), '0x0000000000000000'),
                hUMDevice = NULLIF(COALESCE($userDevice, hUMDevice), '0x0000000000000000')
            WHERE GraphicsObjectId = $id
            """, ("$id", SqliteType.Integer), ("$dxgiDevice", SqliteType.Text), ("$dxgiAdapter", SqliteType.Text),
            ("$kernelAdapter", SqliteType.Text), ("$userAdapter", SqliteType.Text), ("$kernelDevice", SqliteType.Text),
            ("$userDevice", SqliteType.Text));
        resource = Sql.Prepare(connection, """
            UPDATE GraphicsObjects SET DeviceId = COALESCE(DeviceId, $device),
                hKMResource = NULLIF(COALESCE($kernel, hKMResource), '0x0000000000000000'),
                hUMResource = NULLIF(COALESCE($user, hUMResource), '0x0000000000000000')
            WHERE GraphicsObjectId = $id
            """, ("$id", SqliteType.Integer), ("$device", SqliteType.Integer), ("$kernel", SqliteType.Text), ("$user", SqliteType.Text));
        allocation = Sql.Prepare(connection, """
            UPDATE GraphicsObjects SET pDxgAdapter = NULLIF(COALESCE($adapter, pDxgAdapter), '0x0000000000000000'),
                hDevice = NULLIF(COALESCE($device, hDevice), '0x0000000000000000'),
                hVidMmGlobalAlloc = NULLIF(COALESCE($global, hVidMmGlobalAlloc), '0x0000000000000000'),
                hThunkAllocation = NULLIF(COALESCE($thunkAllocation, hThunkAllocation), '0x0000000000000000'),
                hThunkResource = NULLIF(COALESCE($thunkResource, hThunkResource), '0x0000000000000000'),
                hDxgResource = NULLIF(COALESCE($kernel, hDxgResource), '0x0000000000000000'),
                hDxgSharedResource = NULLIF(COALESCE($shared, hDxgSharedResource), '0x0000000000000000'),
                SizeBytes = COALESCE($size, SizeBytes)
            WHERE GraphicsObjectId = $id
            """, ("$id", SqliteType.Integer), ("$adapter", SqliteType.Text), ("$device", SqliteType.Text),
            ("$global", SqliteType.Text), ("$thunkAllocation", SqliteType.Text), ("$thunkResource", SqliteType.Text),
            ("$kernel", SqliteType.Text), ("$shared", SqliteType.Text), ("$size", SqliteType.Text));
        insertLink = Sql.Prepare(connection, """
            INSERT INTO GraphicsLinks (SourceId, TargetId, Kind, Handle, Qualifier, StartSeq)
            VALUES ($source, $target, $kind, $handle, $qualifier, $seq)
            """, ("$source", SqliteType.Integer), ("$target", SqliteType.Integer), ("$kind", SqliteType.Text),
            ("$handle", SqliteType.Text), ("$qualifier", SqliteType.Text), ("$seq", SqliteType.Integer));
        endLink = Sql.Prepare(connection, """
            UPDATE GraphicsLinks SET EndSeq = $seq, EndReason = $reason
            WHERE SourceId = $source AND TargetId = $target AND Kind = $kind AND StartSeq = $start
            """, ("$source", SqliteType.Integer), ("$target", SqliteType.Integer), ("$kind", SqliteType.Text),
            ("$start", SqliteType.Integer), ("$seq", SqliteType.Integer), ("$reason", SqliteType.Text));
        disjointLinks = Sql.Prepare(connection, """
            DELETE FROM GraphicsLinks
            WHERE (SourceId = $id OR TargetId = $id) AND EXISTS (
                SELECT 1 FROM GraphicsObjects s JOIN GraphicsObjects t
                    ON t.GraphicsObjectId = GraphicsLinks.TargetId
                WHERE s.GraphicsObjectId = GraphicsLinks.SourceId
                    AND (s.EndQpc < t.StartQpc OR t.EndQpc < s.StartQpc))
            """, ("$id", SqliteType.Integer));
        commands = [insertObject, observeObject, endObject, history, globalEnds, insertEvent, device, deviceReference, resource, allocation, insertLink, endLink, disjointLinks];
    }

    internal void Add(long sequence, long typeId, ProfilerEvent data, SqliteTransaction transaction)
    {
        var shape = ShapeOf(typeId, data);
        if (shape is null) return;
        foreach (var command in commands) command.Transaction = transaction;
        var identity = shape.Kind switch
        {
            Kind.Device => new Identity(IdentitySpace.Device, shape.Handle(data, "pID3D11Device")),
            Kind.AdapterAllocation => new(IdentitySpace.AdapterAllocation, shape.Handle(data, "hVidMmGlobalAlloc"), shape.Handle(data, "pDxgAdapter")),
            Kind.DeviceAllocation => new(IdentitySpace.DeviceAllocation, shape.Handle(data, "hVidMmAlloc"), shape.Handle(data, "hDevice")),
            _ => new(IdentitySpace.Resource, shape.Handle(data, "pID3D11Resource"))
        };
        if (identity.Handle == 0) return; // Unkeyed events remain available in the raw tables.

        var parentHandle = shape.Kind is >= Kind.Buffer and <= Kind.Texture3D ? shape.Handle(data, "pID3D11Device") : 0;
        (Lifetime Lifetime, bool Latest)? parent = null;
        if (parentHandle != 0)
            parent = Observe(new(IdentitySpace.Device, parentHandle), Kind.Device, Phase.Reference, sequence, data.TimestampQpc);
        var (tracked, latest) = Observe(identity, shape.Kind, shape.Phase, sequence, data.TimestampQpc, parent?.Lifetime.Id);
        if (shape.Kind == Kind.Device)
        {
            if (latest) SaveDevice(tracked.Id, data, shape);
        }
        else if (shape.Kind is >= Kind.Buffer and <= Kind.Texture3D)
        {
            if (tracked.DeviceId is null && parent is { } observed)
            {
                if (observed.Latest) SaveDevice(observed.Lifetime.Id, data, shape);
                tracked.DeviceId = observed.Lifetime.Id;
            }
            else if (parent is null && tracked.DeviceId is { } parentId)
            {
                Set(deviceReference, ("$id", parentId), ("$qpc", data.TimestampQpc));
                if (deviceReference.ExecuteScalar() is not null)
                {
                    if (activeDevices.TryGetValue(parentId, out var owner))
                        Record(owner, Phase.Reference, sequence, data.TimestampQpc);
                    else
                        Record(parentId, Phase.Reference, sequence, data.TimestampQpc);
                }
            }
            Set(resource, ("$id", tracked.Id), ("$device", tracked.DeviceId),
                ("$kernel", latest ? shape.HandleText(data, "hKMResource") : null),
                ("$user", latest ? shape.HandleText(data, "hUMResource") : null));
            resource.ExecuteNonQuery();
        }
        else if (latest)
        {
            Set(allocation, ("$id", tracked.Id), ("$adapter", shape.HandleText(data, "pDxgAdapter")),
                ("$device", shape.HandleText(data, "hDevice")), ("$global", shape.HandleText(data, "hVidMmGlobalAlloc")),
                ("$thunkAllocation", shape.HandleText(data, "hThunkAllocation")), ("$thunkResource", shape.HandleText(data, "hThunkResource")),
                ("$kernel", shape.HandleText(data, "hDxgResource")), ("$shared", shape.HandleText(data, "hDxgSharedResource")),
                ("$size", UnsignedDecimal(shape.Value(data, "allocSize"))));
            allocation.ExecuteNonQuery();
        }

        if (tracked.EndQpc is null)
        {
            // Earlier evidence can refine eligibility, but cannot borrow or replace newer bridge handles.
            var global = new GlobalIdentity(
                shape.OptionalHandle(data, "pDxgAdapter") ?? (latest ? tracked.Global.Adapter : 0),
                shape.OptionalHandle(data, "hVidMmGlobalAlloc") ?? (latest ? tracked.Global.Allocation : 0));
            if (shape.Kind == Kind.DeviceAllocation && global.Adapter != 0 && global.Allocation != 0)
            {
                var observations = tracked.GlobalObservations ??= [];
                if (!observations.TryGetValue(global, out var observation))
                {
                    observation = (GlobalHistoryOf(global, sequence), null, null);
                    observation.History.References++;
                }
                if (shape.Phase == Phase.Start) observation.StartQpc = data.TimestampQpc;
                if (shape.Phase is Phase.Start or Phase.Rundown &&
                    shape.OptionalHandle(data, "hVidMmGlobalAlloc") is not null)
                    observation.ConfirmedQpc = Math.Max(observation.ConfirmedQpc ?? long.MinValue, data.TimestampQpc);
                observations[global] = observation;
            }
            if (latest)
            {
                var bridge = shape.Kind == Kind.DeviceAllocation ? "hThunkResource" : "hKMResource";
                if (shape.OptionalHandle(data, bridge) is { } resourceHandle) tracked.Resource = resourceHandle;
                tracked.Global = global;
            }
            RefreshCorrelations(tracked);
        }
        if (shape.Phase == Phase.Stop) End(tracked, sequence, data.TimestampQpc, "Stop");
        foreach (var (bucket, reason) in pending)
        {
            Reconcile(bucket, sequence, reason);
            if (bucket.Sources.Count != 0 || bucket.Targets.Count != 0) continue;
            if (bucket.UniqueSource) resources.Remove(bucket.Handle);
            else globals.Remove(new(bucket.Qualifier, bucket.Handle));
        }
        pending.Clear();
    }

    private Shape? ShapeOf(long typeId, ProfilerEvent data)
    {
        if (!shapes.TryGetValue(typeId, out var shape))
        {
            var type = TypeOf(data);
            shape = type is { } selected ? new(selected.Kind, selected.Phase,
                data.Fields.Select((field, index) => (field.Name, index)).ToDictionary(field => field.Name, field => field.index)) : null;
            shapes[typeId] = shape;
        }
        return shape;
    }

    private static (Kind Kind, Phase Phase)? TypeOf(ProfilerEvent data) => (data.Provider, data.EventId) switch
    {
        (XamlProvider.Direct3D11, >= 3 and <= 17) => ((Kind)((data.EventId.Value - 3) / 3), (Phase)((data.EventId.Value - 3) % 3)),
        (XamlProvider.DxgKrnl, >= 33 and <= 35) => (Kind.AdapterAllocation, (Phase)(data.EventId.Value - 33)),
        (XamlProvider.DxgKrnl, >= 36 and <= 38) => (Kind.DeviceAllocation, (Phase)(data.EventId.Value - 36)),
        _ => null
    };

    private (Lifetime Lifetime, bool Latest) Observe(Identity identity, Kind kind, Phase phase, long sequence, long qpc, long? deviceId = null)
    {
        active.TryGetValue(identity, out var tracked);
        var ownerChanged = deviceId is not null && tracked?.DeviceId is not null && tracked.DeviceId != deviceId;
        var extend = tracked is { StartQpc: null } && tracked.Kind == kind && !ownerChanged &&
            phase != Phase.Stop && qpc <= tracked.KnownFromQpc;
        var detached = tracked is not null && (qpc < tracked.KnownFromQpc && !tracked.ReferenceOnly ||
            phase == Phase.Stop && qpc < tracked.LastQpc || phase == Phase.Start && qpc < tracked.LastQpc ||
            ownerChanged && qpc < tracked.LastQpc);
        (long Seq, long Qpc)? boundary = null;
        if (tracked is null || detached || extend)
        {
            var past = FindHistorical(identity, kind, phase, qpc, deviceId, extend ? tracked : null, out boundary);
            if (past is not null)
            {
                var latest = qpc >= past.LastQpc || past == tracked && past.ReferenceOnly;
                Record(past, phase, sequence, qpc);
                return (past, latest);
            }
            if (tracked is not null)
            {
                var next = (Seq: tracked.KnownFromSeq, Qpc: Math.Max(tracked.KnownFromQpc, qpc));
                if (boundary is null || next.Qpc < boundary.Value.Qpc) boundary = next;
            }
            detached = boundary is not null;
        }
        if (!detached && tracked is not null && (tracked.Kind != kind || phase == Phase.Start || ownerChanged))
        {
            End(tracked, sequence, qpc, "HandleReuse");
            tracked = null;
        }
        if (detached || tracked is null)
        {
            tracked = new(++objectCount, identity, kind, phase, sequence, qpc);
            Set(insertObject, ("$id", tracked.Id), ("$kind", kind.ToString()), ("$handle", Hex(identity.Handle)),
                ("$qualifier", Hex(identity.Qualifier) ?? ""), ("$seq", sequence), ("$qpc", qpc), ("$phase", phase.ToString()),
                ("$startSeq", phase == Phase.Start ? sequence : null), ("$startQpc", phase == Phase.Start ? qpc : null));
            insertObject.ExecuteNonQuery();
            if (!detached)
            {
                active[identity] = tracked;
                if (kind == Kind.Device) activeDevices.Add(tracked.Id, tracked);
            }
            else if (phase == Phase.Stop) End(tracked, sequence, qpc, "Stop");
            else End(tracked, boundary!.Value.Seq, boundary.Value.Qpc, "HandleReuse");
        }
        var newest = qpc >= tracked.LastQpc || tracked.ReferenceOnly;
        Record(tracked, phase, sequence, qpc);
        return (tracked, newest);
    }

    private Lifetime? FindHistorical(Identity identity, Kind kind, Phase phase, long qpc, long? deviceId, Lifetime? candidate,
        out (long Seq, long Qpc)? boundary)
    {
        var resourceIdentity = identity.Space == IdentitySpace.Resource;
        Set(history, ("$kind1", resourceIdentity ? Kind.Buffer.ToString() : kind.ToString()),
            ("$kind2", resourceIdentity ? Kind.Texture1D.ToString() : kind.ToString()),
            ("$kind3", resourceIdentity ? Kind.Texture2D.ToString() : kind.ToString()),
            ("$kind4", resourceIdentity ? Kind.Texture3D.ToString() : kind.ToString()),
            ("$handle", Hex(identity.Handle)), ("$qualifier", Hex(identity.Qualifier) ?? ""),
            ("$qpc", qpc), ("$device", deviceId), ("$earlier", phase != Phase.Stop), ("$candidate", candidate?.Id));
        boundary = null;
        Lifetime? match = null;
        using var rows = history.ExecuteReader();
        while (rows.Read())
        {
            var firstQpc = rows.GetInt64(3);
            var startQpc = rows.IsDBNull(6) ? (long?)null : rows.GetInt64(6);
            var knownFrom = rows.GetInt64(13);
            (long Seq, long Qpc)? next = qpc < knownFrom ? (rows.GetInt64(12), knownFrom) :
                rows.IsDBNull(8) ? null : (rows.GetInt64(7), rows.GetInt64(8));
            if (next is { } bound && (boundary is null || bound.Qpc < boundary.Value.Qpc)) boundary = bound;
            if (rows.GetString(1) != kind.ToString() ||
                deviceId is { } owner && !rows.IsDBNull(9) && rows.GetInt64(9) != owner) continue;
            if (phase == Phase.Start)
            {
                if (startQpc is { } start ? start != qpc :
                    qpc > knownFrom || !rows.IsDBNull(11) && qpc < rows.GetInt64(11)) continue;
            }
            else if (qpc < (startQpc ?? knownFrom) &&
                (startQpc is not null || phase == Phase.Stop || !rows.IsDBNull(11) && qpc < rows.GetInt64(11))) continue;
            if (phase == Phase.Stop && (qpc < rows.GetInt64(4) || !rows.IsDBNull(10) && qpc != rows.GetInt64(10))) continue;
            if (match is not null) return null; // Multiple plausible lifetimes remain separate evidence.
            match = candidate is { } current && current.Id == rows.GetInt64(0) ? current :
                new(rows.GetInt64(0), identity, kind, Enum.Parse<Phase>(rows.GetString(5)), rows.GetInt64(2), firstQpc)
            {
                LastQpc = rows.GetInt64(4), StartQpc = startQpc, EndQpc = rows.IsDBNull(8) ? null : rows.GetInt64(8),
                DeviceId = rows.IsDBNull(9) ? null : rows.GetInt64(9)
            };
            match.KnownFromSeq = rows.GetInt64(12);
            match.KnownFromQpc = knownFrom;
        }
        return match;
    }

    private GlobalHistory GlobalHistoryOf(GlobalIdentity identity, long sequence)
    {
        if (!globalHistory.TryGetValue(identity, out var retained))
        {
            globalHistory[identity] = retained = new()
            {
                EndQpc = GlobalEndOf(identity)
            };
        }
        retained.LastSeq = sequence;
        return retained;
    }

    private long? GlobalEndOf(GlobalIdentity identity)
    {
        Set(globalEnds, ("$handle", Hex(identity.Allocation)), ("$adapter", Hex(identity.Adapter)));
        return globalEnds.ExecuteScalar() is long end ? end : null;
    }

    private void Record(Lifetime tracked, Phase phase, long sequence, long qpc)
    {
        var promoted = phase == Phase.Start && tracked.StartQpc is null;
        tracked.LastQpc = Math.Max(tracked.LastQpc, qpc);
        if (qpc < tracked.KnownFromQpc)
        {
            tracked.KnownFromSeq = sequence;
            tracked.KnownFromQpc = qpc;
        }
        if (phase == Phase.Start) tracked.StartQpc ??= qpc;
        if (phase != Phase.Reference) tracked.ReferenceOnly = false;
        Record(tracked.Id, phase, sequence, qpc);
        if (promoted) DiscardDisjointLinks(tracked.Id);
    }

    private void Record(long id, Phase phase, long sequence, long qpc)
    {
        Set(observeObject, ("$id", id), ("$seq", sequence), ("$qpc", qpc),
            ("$startSeq", phase == Phase.Start ? sequence : null), ("$startQpc", phase == Phase.Start ? qpc : null));
        observeObject.ExecuteNonQuery();
        Set(insertEvent, ("$seq", sequence), ("$id", id), ("$role", phase.ToString()));
        insertEvent.ExecuteNonQuery();
    }

    private void SaveDevice(long id, ProfilerEvent data, Shape shape)
    {
        Set(device, ("$id", id), ("$dxgiDevice", shape.HandleText(data, "pIDXGIDevice")),
            ("$dxgiAdapter", shape.HandleText(data, "pIDXGIAdapter")), ("$kernelAdapter", shape.HandleText(data, "hKMAdapter")),
            ("$userAdapter", shape.HandleText(data, "hUMAdapter")), ("$kernelDevice", shape.HandleText(data, "hKMDevice")),
            ("$userDevice", shape.HandleText(data, "hUMDevice")));
        device.ExecuteNonQuery();
    }

    private void End(Lifetime tracked, long sequence, long qpc, string reason)
    {
        var previousEnd = tracked.EndQpc;
        tracked.EndQpc = qpc;
        if (active.GetValueOrDefault(tracked.Identity) == tracked) active.Remove(tracked.Identity);
        if (tracked.Kind == Kind.Device) activeDevices.Remove(tracked.Id);
        Set(endObject, ("$id", tracked.Id), ("$seq", sequence), ("$qpc", qpc), ("$reason", reason),
            ("$stopSeq", reason == "Stop" ? sequence : null), ("$stopQpc", reason == "Stop" ? qpc : null));
        endObject.ExecuteNonQuery();
        DiscardDisjointLinks(tracked.Id);
        if (tracked.GlobalObservations is { } observations)
        {
            foreach (var (retained, _, _) in observations.Values) retained.References--;
            tracked.GlobalObservations = null;
        }
        RemoveResource(tracked, reason);
        RemoveGlobal(tracked, reason);
        if (tracked.Kind == Kind.AdapterAllocation && tracked.Identity.Qualifier != 0)
        {
            var identity = new GlobalIdentity(tracked.Identity.Qualifier, tracked.Identity.Handle);
            var retained = GlobalHistoryOf(identity, sequence);
            if (previousEnd is { } previous && qpc < previous && retained.EndQpc == previous)
            {
                var end = GlobalEndOf(identity);
                retained.EndQpc = retained.TrimmedEndQpc is { } trimmed ? Math.Max(end ?? long.MinValue, trimmed) : end;
            }
            else retained.EndQpc = Math.Max(retained.EndQpc ?? long.MinValue, qpc);
            if (globals.TryGetValue(identity, out var bucket)) pending[bucket] = reason;
        }
    }

    private void DiscardDisjointLinks(long id)
    {
        Set(disjointLinks, ("$id", id));
        disjointLinks.ExecuteNonQuery();
    }

    private void RefreshCorrelations(Lifetime tracked)
    {
        // Remove a changed bridge before joining its replacement; payload handles are never treated as permanent mappings.
        if (tracked.RegisteredResource != 0 && tracked.RegisteredResource != tracked.Resource)
        {
            RemoveResource(tracked, "IdentityChanged");
        }
        if (tracked.RegisteredGlobal != default && tracked.RegisteredGlobal != tracked.Global)
        {
            RemoveGlobal(tracked, "IdentityChanged");
        }
        if (tracked.RegisteredResource == 0 && tracked.Resource != 0 &&
            (tracked.Kind is >= Kind.Buffer and <= Kind.Texture3D or Kind.DeviceAllocation))
        {
            if (!resources.TryGetValue(tracked.Resource, out var bucket))
                resources[tracked.Resource] = bucket = new("ResourceAllocation", uniqueSource: true, tracked.Resource);
            (tracked.Kind == Kind.DeviceAllocation ? bucket.Targets : bucket.Sources).Add(tracked);
            tracked.RegisteredResource = tracked.Resource;
            pending.TryAdd(bucket, "AmbiguousIdentity");
        }
        if (tracked.Global.Adapter != 0 && tracked.Global.Allocation != 0 &&
            tracked.Kind is Kind.DeviceAllocation or Kind.AdapterAllocation)
        {
            if (!globals.TryGetValue(tracked.Global, out var bucket))
                globals[tracked.Global] = bucket = new("AllocationGlobal", uniqueSource: false,
                    tracked.Global.Allocation, tracked.Global.Adapter);
            if (tracked.RegisteredGlobal == default)
            {
                (tracked.Kind == Kind.DeviceAllocation ? bucket.Sources : bucket.Targets).Add(tracked);
                tracked.RegisteredGlobal = tracked.Global;
            }
            pending.TryAdd(bucket, "AmbiguousIdentity");
        }
    }

    private void RemoveResource(Lifetime tracked, string reason)
    {
        var key = tracked.RegisteredResource;
        if (resources.TryGetValue(key, out var resourceBucket))
        {
            Remove(resourceBucket, tracked, reason);
        }
        tracked.RegisteredResource = 0;
    }

    private void RemoveGlobal(Lifetime tracked, string reason)
    {
        var key = tracked.RegisteredGlobal;
        if (globals.TryGetValue(key, out var globalBucket))
        {
            Remove(globalBucket, tracked, reason);
        }
        tracked.RegisteredGlobal = default;
    }

    private void Remove(Correlations bucket, Lifetime tracked, string reason)
    {
        if (bucket.Sources.Remove(tracked) | bucket.Targets.Remove(tracked)) pending[bucket] = reason;
    }

    private void Reconcile(Correlations bucket, long sequence, string reason)
    {
        var wanted = new HashSet<(long Source, long Target)>();
        if ((bucket.UniqueSource ? bucket.Sources.Count : bucket.Targets.Count) == 1)
            foreach (var source in bucket.Sources)
                foreach (var target in bucket.Targets)
                {
                    if (!bucket.UniqueSource)
                    {
                        var (retained, start, confirmed) = source.GlobalObservations![new(bucket.Qualifier, bucket.Handle)];
                        // A new source creation after the old end is independent of the target's observation order.
                        // Otherwise a surviving source needs explicit confirmation newer than the replacement.
                        if (retained.EndQpc is { } end && (start is null || start <= end) &&
                            (confirmed is null || confirmed <= Math.Max(end, target.KnownFromQpc)))
                            continue;
                    }
                    wanted.Add((source.Id, target.Id));
                }
        foreach (var (pair, start) in bucket.Open.ToArray())
        {
            if (wanted.Contains(pair)) continue;
            Set(endLink, ("$source", pair.Source), ("$target", pair.Target), ("$kind", bucket.Kind),
                ("$start", start), ("$seq", sequence), ("$reason", reason));
            endLink.ExecuteNonQuery();
            bucket.Open.Remove(pair);
        }
        foreach (var pair in wanted)
        {
            if (!bucket.Open.TryAdd(pair, sequence)) continue;
            Set(insertLink, ("$source", pair.Source), ("$target", pair.Target), ("$kind", bucket.Kind),
                ("$handle", Hex(bucket.Handle)), ("$qualifier", Hex(bucket.Qualifier) ?? ""), ("$seq", sequence));
            insertLink.ExecuteNonQuery();
        }
    }

    // Closed history leaves with its raw events. Open lifetimes and referenced parents remain, explicitly marked as
    // trimmed; deleting their creation evidence must not turn a later Stop into a fabricated new creation.
    internal void Trim(long cutoff, SqliteTransaction transaction)
    {
        Sql.Execute(connection, """
            DELETE FROM GraphicsObjectEvents WHERE Seq < $cutoff;
            DELETE FROM GraphicsLinks WHERE EndSeq < $cutoff;
            UPDATE GraphicsLinks SET Trimmed = 1 WHERE StartSeq < $cutoff;
            UPDATE GraphicsObjects SET Trimmed = 1 WHERE FirstSeq < $cutoff;
            """, transaction, ("$cutoff", cutoff));
        using (var remove = Sql.Prepare(connection, """
            DELETE FROM GraphicsObjects WHERE EndSeq < $cutoff AND (Kind = 'Device') = $devices
                AND NOT EXISTS (SELECT 1 FROM GraphicsObjectEvents e WHERE e.GraphicsObjectId = GraphicsObjects.GraphicsObjectId)
                AND NOT EXISTS (SELECT 1 FROM GraphicsLinks l
                    WHERE l.SourceId = GraphicsObjects.GraphicsObjectId OR l.TargetId = GraphicsObjects.GraphicsObjectId)
                AND NOT EXISTS (SELECT 1 FROM GraphicsObjects r WHERE r.DeviceId = GraphicsObjects.GraphicsObjectId)
            RETURNING Kind, Handle, Qualifier, EndQpc
            """, ("$cutoff", SqliteType.Integer), ("$devices", SqliteType.Integer)))
        {
            remove.Transaction = transaction;
            foreach (var devices in new[] { false, true })
            {
                Set(remove, ("$cutoff", cutoff), ("$devices", devices));
                using var rows = remove.ExecuteReader();
                while (rows.Read())
                {
                    if (rows.GetString(0) != "AdapterAllocation" || rows.GetString(2).Length == 0) continue;
                    var identity = new GlobalIdentity(
                        ulong.Parse(rows.GetString(2).AsSpan(2), NumberStyles.HexNumber, CultureInfo.InvariantCulture),
                        ulong.Parse(rows.GetString(1).AsSpan(2), NumberStyles.HexNumber, CultureInfo.InvariantCulture));
                    if (globalHistory.TryGetValue(identity, out var retained))
                        // Only deleted rows become permanent bounds; retained rows can still receive earlier Stops.
                        retained.TrimmedEndQpc = Math.Max(retained.TrimmedEndQpc ?? long.MinValue, rows.GetInt64(3));
                }
            }
        }
        foreach (var (identity, retained) in globalHistory.ToArray())
            if (retained.References == 0 && retained.LastSeq < cutoff &&
                !active.ContainsKey(new(IdentitySpace.AdapterAllocation, identity.Allocation, identity.Adapter)))
                globalHistory.Remove(identity);
    }

    private static string? Hex(ulong value) => value == 0 ? null : $"0x{value:x16}";

    private static string? UnsignedDecimal(object? value) => value switch
    {
        ulong number => number.ToString(CultureInfo.InvariantCulture),
        uint number => number.ToString(CultureInfo.InvariantCulture),
        long number when number >= 0 => number.ToString(CultureInfo.InvariantCulture),
        int number when number >= 0 => number.ToString(CultureInfo.InvariantCulture),
        _ => null
    };

    private static void Set(SqliteCommand command, params (string Name, object? Value)[] values)
    {
        foreach (var (name, value) in values) command.Parameters[name].Value = value ?? DBNull.Value;
    }

    public void Dispose()
    {
        foreach (var command in commands) command.Dispose();
    }
}

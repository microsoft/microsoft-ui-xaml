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
        internal long LastQpc { get; set; } = qpc;
        internal long? StartQpc { get; set; } = phase == Phase.Start ? qpc : null;
        internal long? EndQpc { get; set; }
        internal long KnownFromQpc => StartQpc ?? FirstQpc;
        internal bool ReferenceOnly { get; set; } = phase == Phase.Reference;
        internal long? DeviceId { get; set; }
        internal ulong Resource { get; set; }
        internal GlobalIdentity Global { get; set; }
        internal ulong RegisteredResource { get; set; }
        internal GlobalIdentity RegisteredGlobal { get; set; }
        internal bool GlobalCorrelatable { get; set; } = true;
        internal long? GlobalInvalidatedQpc { get; set; }
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
    private readonly Dictionary<ulong, Correlations> resources = [];
    private readonly Dictionary<GlobalIdentity, Correlations> globals = [];
    private readonly Dictionary<Correlations, string> pending = [];
    private readonly SqliteCommand insertObject;
    private readonly SqliteCommand observeObject;
    private readonly SqliteCommand endObject;
    private readonly SqliteCommand history;
    private readonly SqliteCommand insertEvent;
    private readonly SqliteCommand device;
    private readonly SqliteCommand resource;
    private readonly SqliteCommand allocation;
    private readonly SqliteCommand insertLink;
    private readonly SqliteCommand endLink;
    private readonly SqliteCommand[] commands;
    private long objectCount;

    // Allow room for lifetime metadata, evidence and correlation rows before the writer trims a batch.
    internal static int EstimatedBytes(ProfilerEvent data) => TypeOf(data) is null ? 0 : 1_000;

    internal GraphicsIndex(SqliteConnection connection)
    {
        this.connection = connection;
        insertObject = Sql.Prepare(connection, """
            INSERT INTO GraphicsObjects (GraphicsObjectId, Kind, Handle, Qualifier, FirstSeq, FirstQpc, LastSeq, LastQpc,
                FirstObservation, StartSeq, StartQpc)
            VALUES ($id, $kind, $handle, $qualifier, $seq, $qpc, $seq, $qpc, $phase, $startSeq, $startQpc)
            """, ("$id", SqliteType.Integer), ("$kind", SqliteType.Text), ("$handle", SqliteType.Text),
            ("$qualifier", SqliteType.Text), ("$seq", SqliteType.Integer), ("$qpc", SqliteType.Integer),
            ("$phase", SqliteType.Text), ("$startSeq", SqliteType.Integer), ("$startQpc", SqliteType.Integer));
        observeObject = Sql.Prepare(connection, """
            UPDATE GraphicsObjects SET LastSeq = $seq, LastQpc = MAX(LastQpc, $qpc),
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
        history = Sql.Prepare(connection, """
            SELECT g.GraphicsObjectId, g.Kind, g.FirstSeq, g.FirstQpc, g.LastQpc, g.FirstObservation, g.StartQpc,
                g.EndSeq, g.EndQpc, g.DeviceId, g.StopQpc,
                CASE WHEN $start AND g.StartQpc IS NULL THEN (SELECT MAX(p.LastQpc) FROM GraphicsObjects p
                    WHERE p.Kind IN ($kind1, $kind2, $kind3, $kind4) AND p.Handle = g.Handle AND p.Qualifier = g.Qualifier
                        AND p.GraphicsObjectId <> g.GraphicsObjectId AND COALESCE(p.StartQpc, p.FirstQpc) < g.FirstQpc
                        AND (p.StartQpc IS NOT NULL OR p.Kind <> g.Kind OR p.DeviceId <> COALESCE(g.DeviceId, $device))) END
            FROM GraphicsObjects g
            WHERE g.Kind IN ($kind1, $kind2, $kind3, $kind4) AND g.Handle = $handle AND g.Qualifier = $qualifier AND g.EndQpc >= $qpc
            ORDER BY g.EndQpc, g.GraphicsObjectId
            """, ("$kind1", SqliteType.Text), ("$kind2", SqliteType.Text), ("$kind3", SqliteType.Text),
            ("$kind4", SqliteType.Text), ("$handle", SqliteType.Text), ("$qualifier", SqliteType.Text),
            ("$qpc", SqliteType.Integer), ("$device", SqliteType.Integer), ("$start", SqliteType.Integer));
        insertEvent = Sql.Prepare(connection,
            "INSERT INTO GraphicsObjectEvents (Seq, GraphicsObjectId, Role) VALUES ($seq, $id, $role)",
            ("$seq", SqliteType.Integer), ("$id", SqliteType.Integer), ("$role", SqliteType.Text));
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
        commands = [insertObject, observeObject, endObject, history, insertEvent, device, resource, allocation, insertLink, endLink];
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
                Record(parentId, Phase.Reference, sequence, data.TimestampQpc);
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

        if (latest && tracked.EndQpc is null)
        {
            var bridge = shape.Kind == Kind.DeviceAllocation ? "hThunkResource" : "hKMResource";
            if (shape.OptionalHandle(data, bridge) is { } resourceHandle) tracked.Resource = resourceHandle;
            var global = new GlobalIdentity(
                shape.OptionalHandle(data, "pDxgAdapter") ?? tracked.Global.Adapter,
                shape.OptionalHandle(data, "hVidMmGlobalAlloc") ?? tracked.Global.Allocation);
            if (global != tracked.Global)
            {
                tracked.GlobalInvalidatedQpc = null;
                tracked.GlobalCorrelatable = true;
            }
            tracked.Global = global;
            if (!tracked.GlobalCorrelatable && globals.TryGetValue(global, out var bucket))
                foreach (var target in bucket.Targets)
                    tracked.GlobalInvalidatedQpc = Math.Max(tracked.GlobalInvalidatedQpc ?? long.MinValue, target.KnownFromQpc);
            if (shape.Phase is Phase.Start or Phase.Rundown &&
                shape.OptionalHandle(data, "hVidMmGlobalAlloc") is not null &&
                data.TimestampQpc > (tracked.GlobalInvalidatedQpc ?? long.MinValue))
                tracked.GlobalCorrelatable = true;
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
        var detached = tracked is not null && (qpc < tracked.KnownFromQpc && !tracked.ReferenceOnly ||
            phase == Phase.Stop && qpc < tracked.LastQpc || phase == Phase.Start && qpc < tracked.LastQpc && !tracked.ReferenceOnly ||
            ownerChanged && qpc < tracked.LastQpc);
        (long Seq, long Qpc)? boundary = null;
        if (tracked is null || detached)
        {
            var past = FindHistorical(identity, kind, phase, qpc, deviceId, out boundary);
            if (past is not null)
            {
                var latest = qpc >= past.LastQpc;
                Record(past, phase, sequence, qpc);
                return (past, latest);
            }
            if (tracked is not null)
            {
                var next = (Seq: tracked.FirstSeq, Qpc: Math.Max(tracked.KnownFromQpc, qpc));
                if (boundary is null || next.Qpc < boundary.Value.Qpc) boundary = next;
            }
            detached = boundary is not null;
        }
        if (!detached && tracked is not null && (tracked.Kind != kind || phase == Phase.Start && !tracked.ReferenceOnly || ownerChanged))
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
            if (!detached) active[identity] = tracked;
            else if (phase == Phase.Stop) End(tracked, sequence, qpc, "Stop");
            else End(tracked, boundary!.Value.Seq, boundary.Value.Qpc, "HandleReuse");
        }
        var newest = qpc >= tracked.LastQpc || tracked.ReferenceOnly;
        if (phase != Phase.Reference) tracked.ReferenceOnly = false;
        Record(tracked, phase, sequence, qpc);
        return (tracked, newest);
    }

    private Lifetime? FindHistorical(Identity identity, Kind kind, Phase phase, long qpc, long? deviceId, out (long Seq, long Qpc)? boundary)
    {
        var resourceIdentity = identity.Space == IdentitySpace.Resource;
        Set(history, ("$kind1", resourceIdentity ? Kind.Buffer.ToString() : kind.ToString()),
            ("$kind2", resourceIdentity ? Kind.Texture1D.ToString() : kind.ToString()),
            ("$kind3", resourceIdentity ? Kind.Texture2D.ToString() : kind.ToString()),
            ("$kind4", resourceIdentity ? Kind.Texture3D.ToString() : kind.ToString()),
            ("$handle", Hex(identity.Handle)), ("$qualifier", Hex(identity.Qualifier) ?? ""),
            ("$qpc", qpc), ("$device", deviceId), ("$start", phase == Phase.Start));
        boundary = null;
        Lifetime? match = null;
        using var rows = history.ExecuteReader();
        while (rows.Read())
        {
            var firstQpc = rows.GetInt64(3);
            var startQpc = rows.IsDBNull(6) ? (long?)null : rows.GetInt64(6);
            var knownFrom = startQpc ?? firstQpc;
            var next = (Seq: qpc < knownFrom ? rows.GetInt64(2) : rows.GetInt64(7),
                Qpc: qpc < knownFrom ? knownFrom : rows.GetInt64(8));
            if (boundary is null || next.Qpc < boundary.Value.Qpc) boundary = next;
            if (rows.GetString(1) != kind.ToString() ||
                deviceId is { } owner && !rows.IsDBNull(9) && rows.GetInt64(9) != owner) continue;
            if (phase == Phase.Start)
            {
                if (startQpc is { } start ? start != qpc :
                    qpc > firstQpc || !rows.IsDBNull(11) && qpc < rows.GetInt64(11)) continue;
            }
            else if (qpc < (startQpc ?? firstQpc)) continue;
            if (phase == Phase.Stop && (qpc < rows.GetInt64(4) || !rows.IsDBNull(10) && qpc != rows.GetInt64(10))) continue;
            if (match is not null) return null; // Multiple plausible lifetimes remain separate evidence.
            match = new(rows.GetInt64(0), identity, kind, Enum.Parse<Phase>(rows.GetString(5)), rows.GetInt64(2), firstQpc)
            {
                LastQpc = rows.GetInt64(4), StartQpc = startQpc, EndQpc = rows.GetInt64(8),
                DeviceId = rows.IsDBNull(9) ? null : rows.GetInt64(9)
            };
        }
        return match;
    }

    private void Record(Lifetime tracked, Phase phase, long sequence, long qpc)
    {
        tracked.LastQpc = Math.Max(tracked.LastQpc, qpc);
        if (phase == Phase.Start) tracked.StartQpc ??= qpc;
        Record(tracked.Id, phase, sequence, qpc);
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
        tracked.EndQpc = qpc;
        if (active.GetValueOrDefault(tracked.Identity) == tracked) active.Remove(tracked.Identity);
        Set(endObject, ("$id", tracked.Id), ("$seq", sequence), ("$qpc", qpc), ("$reason", reason),
            ("$stopSeq", reason == "Stop" ? sequence : null), ("$stopQpc", reason == "Stop" ? qpc : null));
        endObject.ExecuteNonQuery();
        if (tracked.Kind == Kind.AdapterAllocation && globals.TryGetValue(tracked.RegisteredGlobal, out var bucket))
        {
            // Do not join surviving, unclosed device-allocation records to a later reuse of this global pointer.
            foreach (var child in bucket.Sources.Where(child => child.LastQpc <= qpc).ToArray())
            {
                child.GlobalCorrelatable = false;
                child.GlobalInvalidatedQpc = Math.Max(child.GlobalInvalidatedQpc ?? long.MinValue, qpc);
                RemoveGlobal(child, "GlobalAllocationEnded");
            }
        }
        RemoveResource(tracked, reason);
        RemoveGlobal(tracked, reason);
    }

    private void RefreshCorrelations(Lifetime tracked)
    {
        // Remove a changed bridge before joining its replacement; payload handles are never treated as permanent mappings.
        if (tracked.RegisteredResource != 0 && tracked.RegisteredResource != tracked.Resource)
        {
            RemoveResource(tracked, "IdentityChanged");
        }
        if (tracked.RegisteredGlobal != default && (tracked.RegisteredGlobal != tracked.Global || !tracked.GlobalCorrelatable))
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
        if (tracked.RegisteredGlobal == default && tracked.Global.Adapter != 0 && tracked.Global.Allocation != 0 && tracked.GlobalCorrelatable &&
            tracked.Kind is Kind.DeviceAllocation or Kind.AdapterAllocation)
        {
            if (!globals.TryGetValue(tracked.Global, out var bucket))
                globals[tracked.Global] = bucket = new("AllocationGlobal", uniqueSource: false,
                    tracked.Global.Allocation, tracked.Global.Adapter);
            (tracked.Kind == Kind.DeviceAllocation ? bucket.Sources : bucket.Targets).Add(tracked);
            tracked.RegisteredGlobal = tracked.Global;
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
                foreach (var target in bucket.Targets) wanted.Add((source.Id, target.Id));
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
            DELETE FROM GraphicsObjects WHERE EndSeq < $cutoff
                AND NOT EXISTS (SELECT 1 FROM GraphicsObjectEvents e WHERE e.GraphicsObjectId = GraphicsObjects.GraphicsObjectId)
                AND NOT EXISTS (SELECT 1 FROM GraphicsLinks l
                    WHERE l.SourceId = GraphicsObjects.GraphicsObjectId OR l.TargetId = GraphicsObjects.GraphicsObjectId)
                AND NOT EXISTS (SELECT 1 FROM GraphicsObjects r WHERE r.DeviceId = GraphicsObjects.GraphicsObjectId);
            """, transaction, ("$cutoff", cutoff));
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

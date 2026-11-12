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

        internal ulong Handle(ProfilerEvent data, string name) => Value(data, name) switch
        {
            ulong value => value,
            uint value => value,
            long value => unchecked((ulong)value),
            int value => unchecked((uint)value),
            _ => 0
        };
    }

    private sealed class Lifetime(long id, Identity identity, Kind kind, Phase phase, long sequence, long qpc)
    {
        internal long Id { get; } = id;
        internal Identity Identity { get; } = identity;
        internal Kind Kind { get; } = kind;
        internal long FirstSeq { get; } = sequence;
        internal long FirstQpc { get; } = qpc;
        internal bool ReferenceOnly { get; set; } = phase == Phase.Reference;
        internal long? DeviceId { get; set; }
        internal ulong Resource { get; set; }
        internal GlobalIdentity Global { get; set; }
        internal ulong RegisteredResource { get; set; }
        internal GlobalIdentity RegisteredGlobal { get; set; }
        internal bool Correlatable { get; set; } = true;
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
    private readonly SqliteCommand insertObject;
    private readonly SqliteCommand observeObject;
    private readonly SqliteCommand endObject;
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
            UPDATE GraphicsObjects SET LastSeq = $seq, LastQpc = $qpc,
                StartSeq = COALESCE(StartSeq, $startSeq), StartQpc = COALESCE(StartQpc, $startQpc)
            WHERE GraphicsObjectId = $id
            """, ("$id", SqliteType.Integer), ("$seq", SqliteType.Integer), ("$qpc", SqliteType.Integer),
            ("$startSeq", SqliteType.Integer), ("$startQpc", SqliteType.Integer));
        endObject = Sql.Prepare(connection, """
            UPDATE GraphicsObjects SET EndSeq = $seq, EndQpc = $qpc, EndReason = $reason, StopSeq = $stopSeq, StopQpc = $stopQpc
            WHERE GraphicsObjectId = $id
            """, ("$id", SqliteType.Integer), ("$seq", SqliteType.Integer), ("$qpc", SqliteType.Integer),
            ("$reason", SqliteType.Text), ("$stopSeq", SqliteType.Integer), ("$stopQpc", SqliteType.Integer));
        insertEvent = Sql.Prepare(connection,
            "INSERT INTO GraphicsObjectEvents (Seq, GraphicsObjectId, Role) VALUES ($seq, $id, $role)",
            ("$seq", SqliteType.Integer), ("$id", SqliteType.Integer), ("$role", SqliteType.Text));
        device = Sql.Prepare(connection, """
            UPDATE GraphicsObjects SET pIDXGIDevice = COALESCE($dxgiDevice, pIDXGIDevice),
                pIDXGIAdapter = COALESCE($dxgiAdapter, pIDXGIAdapter), hKMAdapter = COALESCE($kernelAdapter, hKMAdapter),
                hUMAdapter = COALESCE($userAdapter, hUMAdapter), hKMDevice = COALESCE($kernelDevice, hKMDevice),
                hUMDevice = COALESCE($userDevice, hUMDevice)
            WHERE GraphicsObjectId = $id
            """, ("$id", SqliteType.Integer), ("$dxgiDevice", SqliteType.Text), ("$dxgiAdapter", SqliteType.Text),
            ("$kernelAdapter", SqliteType.Text), ("$userAdapter", SqliteType.Text), ("$kernelDevice", SqliteType.Text),
            ("$userDevice", SqliteType.Text));
        resource = Sql.Prepare(connection, """
            UPDATE GraphicsObjects SET DeviceId = COALESCE(DeviceId, $device),
                hKMResource = COALESCE($kernel, hKMResource), hUMResource = COALESCE($user, hUMResource)
            WHERE GraphicsObjectId = $id
            """, ("$id", SqliteType.Integer), ("$device", SqliteType.Integer), ("$kernel", SqliteType.Text), ("$user", SqliteType.Text));
        allocation = Sql.Prepare(connection, """
            UPDATE GraphicsObjects SET pDxgAdapter = COALESCE($adapter, pDxgAdapter), hDevice = COALESCE($device, hDevice),
                hVidMmGlobalAlloc = COALESCE($global, hVidMmGlobalAlloc), hThunkAllocation = COALESCE($thunkAllocation, hThunkAllocation),
                hThunkResource = COALESCE($thunkResource, hThunkResource), hDxgResource = COALESCE($kernel, hDxgResource),
                hDxgSharedResource = COALESCE($shared, hDxgSharedResource), SizeBytes = COALESCE($size, SizeBytes)
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
        commands = [insertObject, observeObject, endObject, insertEvent, device, resource, allocation, insertLink, endLink];
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

        // Earlier observations must not change a newer instance. A reference-only device can still acquire its Start.
        var earlierInstance = active.TryGetValue(identity, out var current) && (shape.Phase == Phase.Stop || !current.ReferenceOnly) &&
            data.TimestampQpc < current.FirstQpc;
        var tracked = Observe(identity, shape.Kind, shape.Phase, sequence, data.TimestampQpc, earlierInstance);
        if (shape.Kind == Kind.Device)
        {
            SaveDevice(tracked.Id, data, shape);
        }
        else if (shape.Kind is >= Kind.Buffer and <= Kind.Texture3D)
        {
            var parentHandle = shape.Handle(data, "pID3D11Device");
            if (!earlierInstance && tracked.DeviceId is null && parentHandle != 0 &&
                (!active.TryGetValue(new(IdentitySpace.Device, parentHandle), out var parent) || parent.FirstQpc <= data.TimestampQpc))
            {
                var observed = Observe(new(IdentitySpace.Device, parentHandle), Kind.Device, Phase.Reference, sequence, data.TimestampQpc);
                SaveDevice(observed.Id, data, shape);
                tracked.DeviceId = observed.Id;
            }
            else if (!earlierInstance && tracked.DeviceId is { } parentId)
            {
                Record(parentId, Phase.Reference, sequence, data.TimestampQpc);
            }
            Set(resource, ("$id", tracked.Id), ("$device", tracked.DeviceId),
                ("$kernel", Hex(shape.Handle(data, "hKMResource"))), ("$user", Hex(shape.Handle(data, "hUMResource"))));
            resource.ExecuteNonQuery();
        }
        else
        {
            Set(allocation, ("$id", tracked.Id), ("$adapter", Hex(shape.Handle(data, "pDxgAdapter"))),
                ("$device", Hex(shape.Handle(data, "hDevice"))), ("$global", Hex(shape.Handle(data, "hVidMmGlobalAlloc"))),
                ("$thunkAllocation", Hex(shape.Handle(data, "hThunkAllocation"))), ("$thunkResource", Hex(shape.Handle(data, "hThunkResource"))),
                ("$kernel", Hex(shape.Handle(data, "hDxgResource"))), ("$shared", Hex(shape.Handle(data, "hDxgSharedResource"))),
                ("$size", UnsignedDecimal(shape.Value(data, "allocSize"))));
            allocation.ExecuteNonQuery();
        }

        if (!earlierInstance)
        {
            var resourceHandle = shape.Handle(data, shape.Kind == Kind.DeviceAllocation ? "hThunkResource" : "hKMResource");
            var global = new GlobalIdentity(shape.Handle(data, "pDxgAdapter"), shape.Handle(data, "hVidMmGlobalAlloc"));
            if (resourceHandle != 0) tracked.Resource = resourceHandle;
            if (global.Adapter != 0 && global.Allocation != 0) tracked.Global = global;
            // A new explicit observation can re-establish identity after a global allocation ended.
            if (shape.Phase is Phase.Start or Phase.Rundown) tracked.Correlatable = true;
            RefreshCorrelations(tracked, sequence);
        }
        if (shape.Phase == Phase.Stop) End(tracked, sequence, data.TimestampQpc, "Stop");
        else if (earlierInstance) End(tracked, current!.FirstSeq, current.FirstQpc, "HandleReuse");
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

    private Lifetime Observe(Identity identity, Kind kind, Phase phase, long sequence, long qpc, bool detached = false)
    {
        active.TryGetValue(identity, out var tracked);
        if (!detached && tracked is not null && (tracked.Kind != kind || phase == Phase.Start && !tracked.ReferenceOnly))
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
        }
        if (phase != Phase.Reference) tracked.ReferenceOnly = false;
        Record(tracked.Id, phase, sequence, qpc);
        return tracked;
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
        Set(device, ("$id", id), ("$dxgiDevice", Hex(shape.Handle(data, "pIDXGIDevice"))),
            ("$dxgiAdapter", Hex(shape.Handle(data, "pIDXGIAdapter"))), ("$kernelAdapter", Hex(shape.Handle(data, "hKMAdapter"))),
            ("$userAdapter", Hex(shape.Handle(data, "hUMAdapter"))), ("$kernelDevice", Hex(shape.Handle(data, "hKMDevice"))),
            ("$userDevice", Hex(shape.Handle(data, "hUMDevice"))));
        device.ExecuteNonQuery();
    }

    private void End(Lifetime tracked, long sequence, long qpc, string reason)
    {
        if (active.GetValueOrDefault(tracked.Identity) == tracked) active.Remove(tracked.Identity);
        Set(endObject, ("$id", tracked.Id), ("$seq", sequence), ("$qpc", qpc), ("$reason", reason),
            ("$stopSeq", reason == "Stop" ? sequence : null), ("$stopQpc", reason == "Stop" ? qpc : null));
        endObject.ExecuteNonQuery();
        if (tracked.Kind == Kind.AdapterAllocation && globals.TryGetValue(tracked.Global, out var bucket))
        {
            // Do not join surviving, unclosed device-allocation records to a later reuse of this global pointer.
            foreach (var child in bucket.Sources.Where(child => child.FirstQpc <= qpc).ToArray())
            {
                child.Correlatable = false;
                RemoveCorrelations(child, sequence, "GlobalAllocationEnded");
            }
        }
        RemoveCorrelations(tracked, sequence, reason);
    }

    private void RefreshCorrelations(Lifetime tracked, long sequence)
    {
        // Remove a changed bridge before joining its replacement; payload handles are never treated as permanent mappings.
        if (tracked.RegisteredResource != 0 && (tracked.RegisteredResource != tracked.Resource || !tracked.Correlatable))
        {
            RemoveResource(tracked, sequence, "IdentityChanged");
        }
        if (tracked.RegisteredGlobal != default && (tracked.RegisteredGlobal != tracked.Global || !tracked.Correlatable))
        {
            RemoveGlobal(tracked, sequence, "IdentityChanged");
        }
        if (!tracked.Correlatable) return;
        if (tracked.RegisteredResource == 0 && tracked.Resource != 0 &&
            (tracked.Kind is >= Kind.Buffer and <= Kind.Texture3D or Kind.DeviceAllocation))
        {
            if (!resources.TryGetValue(tracked.Resource, out var bucket))
                resources[tracked.Resource] = bucket = new("ResourceAllocation", uniqueSource: true, tracked.Resource);
            (tracked.Kind == Kind.DeviceAllocation ? bucket.Targets : bucket.Sources).Add(tracked);
            tracked.RegisteredResource = tracked.Resource;
            Reconcile(bucket, sequence, "AmbiguousIdentity");
        }
        if (tracked.RegisteredGlobal == default && tracked.Global != default &&
            tracked.Kind is Kind.DeviceAllocation or Kind.AdapterAllocation)
        {
            if (!globals.TryGetValue(tracked.Global, out var bucket))
                globals[tracked.Global] = bucket = new("AllocationGlobal", uniqueSource: false,
                    tracked.Global.Allocation, tracked.Global.Adapter);
            (tracked.Kind == Kind.DeviceAllocation ? bucket.Sources : bucket.Targets).Add(tracked);
            tracked.RegisteredGlobal = tracked.Global;
            Reconcile(bucket, sequence, "AmbiguousIdentity");
        }
    }

    private void RemoveCorrelations(Lifetime tracked, long sequence, string reason)
    {
        RemoveResource(tracked, sequence, reason);
        RemoveGlobal(tracked, sequence, reason);
    }

    private void RemoveResource(Lifetime tracked, long sequence, string reason)
    {
        var key = tracked.RegisteredResource;
        if (resources.TryGetValue(key, out var resourceBucket))
        {
            Remove(resourceBucket, tracked, sequence, reason);
            if (resourceBucket.Sources.Count == 0 && resourceBucket.Targets.Count == 0) resources.Remove(key);
        }
        tracked.RegisteredResource = 0;
    }

    private void RemoveGlobal(Lifetime tracked, long sequence, string reason)
    {
        var key = tracked.RegisteredGlobal;
        if (globals.TryGetValue(key, out var globalBucket))
        {
            Remove(globalBucket, tracked, sequence, reason);
            if (globalBucket.Sources.Count == 0 && globalBucket.Targets.Count == 0) globals.Remove(key);
        }
        tracked.RegisteredGlobal = default;
    }

    private void Remove(Correlations bucket, Lifetime tracked, long sequence, string reason)
    {
        if (bucket.Sources.Remove(tracked) | bucket.Targets.Remove(tracked)) Reconcile(bucket, sequence, reason);
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

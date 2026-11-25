// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.Engine.Storage;

public sealed record GpuMemoryCoverage(bool GraphicsIndex, bool Direct3D, bool ElementVisuals,
    bool VisualSurfaces, bool SurfaceTextures);

public sealed record GpuMemoryElement(long ObjectId, ulong Address, string? Type, string? Name);

public sealed record GpuTextureObservation(long Sequence, long Qpc, ulong ResourceAddress,
    uint Width, uint Height, uint MipLevels, uint ArraySize, uint Format, uint SampleCount, int OffsetX, int OffsetY);

public enum GpuTextureLinkStatus { Available, ResourceNotAliveOrRecorded, AmbiguousLifetime, ObservationBeforeLifetime }

public sealed record GpuElementTextureLink(long ObjectId, ulong VisualAddress, ulong SurfaceAddress, uint Role,
    long ElementLinkQpc, long SurfaceLinkQpc, GpuTextureObservation Observation, long? TextureId, GpuTextureLinkStatus Status);

public sealed record SessionGpuMemory(SessionFileInfo Session, long? AtQpc, double? AtMilliseconds,
    long QpcFrequency, long? StartQpc, GpuMemoryCoverage Coverage, IReadOnlyList<GpuMemoryElement> Elements,
    IReadOnlyList<SnapshotTexture> Textures, IReadOnlyList<GpuElementTextureLink> Links);

public static partial class SessionFile
{
    public static GpuMemoryCoverage ReadGpuMemoryCoverage(string path)
    {
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        return ReadGpuMemoryCoverage(connection, transaction, long.MaxValue);
    }

    // Replays only explicit associations, in event-time order, in the same read transaction as resource lifetimes.
    // Update textures are observations, not proof of persistent backing storage or exclusive element ownership.
    public static SessionGpuMemory ReadGpuMemory(string path, double? atMilliseconds = null, CancellationToken token = default)
    {
        if (atMilliseconds is { } time && (!double.IsFinite(time) || time < 0))
            throw new ArgumentOutOfRangeException(nameof(atMilliseconds), "The time must be finite and nonnegative.");
        token.ThrowIfCancellationRequested();
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        var (info, frequency, start) = ReadSession(connection, transaction);
        long? last;
        long? completeFrom;
        using (var command = Command(connection, transaction,
            "SELECT (SELECT MAX(Qpc) FROM Events), CompleteFromQpc FROM SessionInfo"))
        {
            using var reader = command.ExecuteReader();
            if (!reader.Read()) throw new InvalidDataException("The session file has no session record.");
            last = NullableInt64(reader, 0);
            completeFrom = NullableInt64(reader, 1);
            SessionQueries.Report(command, 1);
        }

        var at = last;
        if (atMilliseconds is { } requested)
        {
            if (start is null || last is null || requested > (last.Value - start.Value) * 1000.0 / frequency)
                throw new InvalidDataException("The requested memory time is outside the stored recording.");
            at = Math.Min(last.Value, checked(start.Value + (long)Math.Round(requested * frequency / 1000)));
        }
        if (at < completeFrom)
            throw new InvalidDataException("The requested memory time was deleted by session retention.");
        var coverage = ReadGpuMemoryCoverage(connection, transaction, at ?? long.MinValue);
        if (at is null)
            return new(info, null, null, frequency, start, coverage, [], [], []);

        var textures = coverage.GraphicsIndex ? ReadTexturesAt(connection, transaction, at.Value, token) : [];
        var elements = ReadMemoryElements(connection, transaction, at.Value, token);
        var owners = new Dictionary<ulong, (long ObjectId, long Qpc)>();
        var bindings = new Dictionary<ulong, Dictionary<uint, (ulong Surface, long Qpc)>>();
        var initialized = new HashSet<ulong>();
        var observations = new Dictionary<ulong, Dictionary<ulong, GpuTextureObservation>>();
        var launcher = HasColumn(connection, transaction, "EventTypes", "Launcher") ? "AND t.Launcher = 0" : "";
        ReadEvents(connection, transaction, info, frequency, start, (_, _, _) => $"""
            SELECT e.Qpc, e.ThreadId, e.TypeId, e.Payload, e.Seq,
                (SELECT r.ObjectId FROM ObjectEvents r JOIN Objects o ON o.ObjectId = r.ObjectId
                 WHERE r.Seq = e.Seq AND o.Kind = 'Element' LIMIT 1)
            FROM Events e JOIN EventTypes t ON t.TypeId = e.TypeId
            WHERE t.Provider = 'XamlProfiler' AND t.TraceLogging = 1 {launcher} AND e.Qpc <= $qpc
                AND t.Name IN ('WucVisualElementChanged', 'VisualSurfaceChanged', 'DCompSurfaceInit',
                    'DCompSurfaceMemoryChanged', 'DCompSurfaceTextureObserved')
            ORDER BY e.Qpc, e.Seq
            """, [("$qpc", at.Value)], (data, reader) =>
        {
            switch (data.Name)
            {
                case "WucVisualElementChanged":
                {
                    var visual = MemoryHandle(data, "VisualId");
                    // The renderer reports the new association before this primitive's brush/mask bindings.
                    // Recycled or pointer-reused visuals must not inherit an earlier primitive's surfaces.
                    bindings.Remove(visual);
                    if (MemoryHandle(data, "UIElementId") == 0 || reader.IsDBNull(5))
                        owners.Remove(visual);
                    else
                        owners[visual] = (reader.GetInt64(5), data.TimestampQpc);
                    break;
                }
                case "VisualSurfaceChanged":
                {
                    var visual = MemoryHandle(data, "VisualId");
                    var role = MemoryUInt(data, "Role");
                    var surface = MemoryHandle(data, "SurfaceId");
                    if (surface == 0)
                    {
                        if (bindings.TryGetValue(visual, out var roles))
                            roles.Remove(role);
                    }
                    else
                    {
                        if (!bindings.TryGetValue(visual, out var roles))
                            bindings.Add(visual, roles = []);
                        roles[role] = (surface, data.TimestampQpc);
                    }
                    break;
                }
                case "DCompSurfaceInit":
                {
                    var surface = MemoryHandle(data, "SurfaceId");
                    observations.Remove(surface);
                    if (!initialized.Add(surface))
                        foreach (var roles in bindings.Values)
                            foreach (var role in roles.Where(pair => pair.Value.Surface == surface).Select(pair => pair.Key).ToArray())
                                roles.Remove(role);
                    break;
                }
                case "DCompSurfaceMemoryChanged":
                    if (MemoryField(data, "IsAllocation") is not bool allocated)
                        throw new InvalidDataException("DCompSurfaceMemoryChanged has no Boolean IsAllocation.");
                    if (!allocated)
                        observations.Remove(MemoryHandle(data, "SurfaceId"));
                    break;
                case "DCompSurfaceTextureObserved":
                {
                    var surface = MemoryHandle(data, "SurfaceId");
                    if (!observations.TryGetValue(surface, out var updates))
                        observations.Add(surface, updates = []);
                    var resource = MemoryHandle(data, "ResourceId");
                    updates[resource] = new(reader.GetInt64(4), data.TimestampQpc, resource,
                        MemoryUInt(data, "Width"), MemoryUInt(data, "Height"), MemoryUInt(data, "MipLevels"),
                        MemoryUInt(data, "ArraySize"), MemoryUInt(data, "Format"), MemoryUInt(data, "SampleCount"),
                        MemoryInt(data, "OffsetX"), MemoryInt(data, "OffsetY"));
                    break;
                }
            }
        }, token);

        var byHandle = textures.GroupBy(texture => texture.Handle, StringComparer.Ordinal)
            .ToDictionary(group => group.Key, group => group.ToArray(), StringComparer.Ordinal);
        var links = new List<GpuElementTextureLink>();
        foreach (var (visual, roles) in bindings.OrderBy(pair => pair.Key))
        {
            token.ThrowIfCancellationRequested();
            if (!owners.TryGetValue(visual, out var owner))
                continue;
            foreach (var (role, binding) in roles.OrderBy(pair => pair.Key))
            {
                if (!observations.TryGetValue(binding.Surface, out var updates))
                    continue;
                foreach (var update in updates.Values.OrderBy(update => update.ResourceAddress))
                {
                    byHandle.TryGetValue($"0x{update.ResourceAddress:x16}", out var matches);
                    var texture = matches is { Length: 1 } ? matches[0] : null;
                    var status = matches is null ? GpuTextureLinkStatus.ResourceNotAliveOrRecorded
                        : matches.Length != 1 ? GpuTextureLinkStatus.AmbiguousLifetime
                        : update.Qpc < Math.Max(texture!.StartQpc ?? texture.KnownFromQpc, texture.KnownFromQpc)
                            ? GpuTextureLinkStatus.ObservationBeforeLifetime : GpuTextureLinkStatus.Available;
                    links.Add(new(owner.ObjectId, visual, binding.Surface, role, owner.Qpc, binding.Qpc,
                        update, status == GpuTextureLinkStatus.Available ? texture!.Id : null, status));
                }
            }
        }
        return new(info, at, start is null ? null : (at.Value - start.Value) * 1000.0 / frequency,
            frequency, start, coverage, elements, textures, links);
    }

    private static GpuMemoryCoverage ReadGpuMemoryCoverage(SqliteConnection connection, SqliteTransaction transaction, long qpc)
    {
        var indexed = Sql.Scalar(connection, """
            SELECT COUNT(*) FROM sqlite_master WHERE type = 'table'
                AND name IN ('GraphicsObjects', 'GraphicsObjectEvents', 'GraphicsLinks')
            """, transaction) == 3;
        var launcher = HasColumn(connection, transaction, "EventTypes", "Launcher") ? "AND t.Launcher = 0" : "";
        using var command = Command(connection, transaction, $"""
            SELECT EXISTS (SELECT 1 FROM EventTypes t JOIN Events e ON e.TypeId = t.TypeId
                    WHERE t.Provider = 'Direct3D11' AND e.Qpc <= $qpc {launcher}),
                EXISTS (SELECT 1 FROM EventTypes t JOIN Events e ON e.TypeId = t.TypeId
                    WHERE t.Provider = 'XamlProfiler' AND t.TraceLogging = 1 AND t.Name = 'WucVisualElementChanged'
                        AND e.Qpc <= $qpc {launcher}),
                EXISTS (SELECT 1 FROM EventTypes t JOIN Events e ON e.TypeId = t.TypeId
                    WHERE t.Provider = 'XamlProfiler' AND t.TraceLogging = 1 AND t.Name = 'VisualSurfaceChanged'
                        AND e.Qpc <= $qpc {launcher}),
                EXISTS (SELECT 1 FROM EventTypes t JOIN Events e ON e.TypeId = t.TypeId
                    WHERE t.Provider = 'XamlProfiler' AND t.TraceLogging = 1 AND t.Name = 'DCompSurfaceTextureObserved'
                        AND e.Qpc <= $qpc {launcher})
            """, ("$qpc", qpc));
        using var reader = command.ExecuteReader();
        if (!reader.Read()) throw new InvalidDataException("The GPU memory coverage could not be read.");
        var coverage = new GpuMemoryCoverage(indexed, reader.GetBoolean(0), reader.GetBoolean(1), reader.GetBoolean(2), reader.GetBoolean(3));
        SessionQueries.Report(command, 1);
        return coverage;
    }

    private static IReadOnlyList<GpuMemoryElement> ReadMemoryElements(SqliteConnection connection,
        SqliteTransaction transaction, long qpc, CancellationToken token)
    {
        using var command = Command(connection, transaction, """
            SELECT o.ObjectId, o.Address, o.Type, o.Name FROM Objects o
            WHERE o.Kind = 'Element' AND EXISTS (
                SELECT 1 FROM ObjectEvents r JOIN Events e ON e.Seq = r.Seq WHERE r.ObjectId = o.ObjectId AND e.Qpc <= $qpc)
            ORDER BY o.ObjectId
            """, ("$qpc", qpc));
        using var reader = command.ExecuteReader();
        var elements = new List<GpuMemoryElement>();
        while (reader.Read())
        {
            token.ThrowIfCancellationRequested();
            elements.Add(new(reader.GetInt64(0), unchecked((ulong)reader.GetInt64(1)), NullableString(reader, 2), NullableString(reader, 3)));
        }
        SessionQueries.Report(command, elements.Count);
        return elements;
    }

    private static object? MemoryField(ProfilerEvent data, string name) =>
        data.Fields.FirstOrDefault(field => field.Name == name).Value;

    private static ulong MemoryHandle(ProfilerEvent data, string name) => MemoryField(data, name) switch
    {
        ulong value => value,
        long value => unchecked((ulong)value),
        uint value => value,
        int value => unchecked((uint)value),
        _ => throw new InvalidDataException($"{data.Name} has no integer {name}.")
    };

    private static uint MemoryUInt(ProfilerEvent data, string name) => MemoryField(data, name) switch
    {
        uint value => value,
        int value when value >= 0 => (uint)value,
        _ => throw new InvalidDataException($"{data.Name} has no unsigned 32-bit {name}.")
    };

    private static int MemoryInt(ProfilerEvent data, string name) => MemoryField(data, name) is int value ? value
        : throw new InvalidDataException($"{data.Name} has no signed 32-bit {name}.");
}

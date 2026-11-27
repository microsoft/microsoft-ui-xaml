using System.Globalization;
using System.Text.Json;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.Engine.Storage;

public static partial class SessionFile
{
    // Both sides of a comparison read the same SQLite transaction, including while recording continues.
    public static IReadOnlyList<SnapshotTextureCapture> ReadTextureSnapshots(string? path,
        IReadOnlyList<TreeSnapshotDocument> snapshots, CancellationToken token = default)
    {
        ArgumentNullException.ThrowIfNull(snapshots);
        token.ThrowIfCancellationRequested();
        if (path is null)
            return snapshots.Select(_ => new SnapshotTextureCapture(SnapshotTextureStatus.NotRecorded, null, [])).ToArray();
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        if (Sql.Scalar(connection, """
            SELECT COUNT(*) FROM sqlite_master WHERE name IN ('GraphicsObjects', 'GraphicsObjectEvents', 'GraphicsLinks')
                AND type = 'table'
            """, transaction) != 3)
            return snapshots.Select(_ => new SnapshotTextureCapture(SnapshotTextureStatus.NotRecorded, null, [])).ToArray();
        return snapshots.Select(snapshot => ReadTextures(connection, transaction, snapshot, token)).ToArray();
    }

    private static SnapshotTextureCapture ReadTextures(SqliteConnection connection, SqliteTransaction transaction,
        TreeSnapshotDocument snapshot, CancellationToken token)
    {
        token.ThrowIfCancellationRequested();
        SnapshotTextureCapture capture;
        using (var command = Command(connection, transaction, """
            SELECT SessionId, ProcessId, ProcessCreatedUtc, Source, QpcFrequency, StartQpc, CompleteFromQpc,
                EndedUtc, LostByEtw, DroppedByProfiler,
                EXISTS (SELECT 1 FROM EventTypes t JOIN Events e ON e.TypeId = t.TypeId
                    WHERE t.Provider = 'Direct3D11' AND e.Qpc <= $qpc)
                    OR EXISTS (SELECT 1 FROM GraphicsObjects
                        WHERE Kind IN ('Device', 'Buffer', 'Texture1D', 'Texture2D', 'Texture3D')
                            AND COALESCE(StartQpc, KnownFromQpc) <= $qpc)
            FROM SessionInfo
            """, ("$qpc", snapshot.EndQpc)))
        {
            using var reader = command.ExecuteReader();
            if (!reader.Read()) throw new InvalidDataException("The session file has no session record.");
            var status = reader.GetInt64(1) != snapshot.ProcessId || reader.IsDBNull(2) ||
                ParseUtc(reader.GetString(2)).ToFileTimeUtc() != snapshot.CreationTimeFileTime
                    ? SnapshotTextureStatus.DifferentProcess
                : reader.GetString(3) != "Live" || reader.IsDBNull(4) ||
                    reader.GetInt64(4) != snapshot.QpcFrequency ? SnapshotTextureStatus.DifferentClock
                : reader.IsDBNull(5) || snapshot.EndQpc < reader.GetInt64(5) ? SnapshotTextureStatus.OutsideRecording
                : !reader.IsDBNull(6) && snapshot.EndQpc < reader.GetInt64(6) ? SnapshotTextureStatus.HistoryTrimmed
                : !reader.GetBoolean(10) ? SnapshotTextureStatus.NoEvents : SnapshotTextureStatus.Available;
            capture = new(status, Guid.Parse(reader.GetString(0)), [])
            {
                AtQpc = snapshot.EndQpc,
                StartQpc = reader.IsDBNull(5) ? 0 : reader.GetInt64(5),
                QpcFrequency = snapshot.QpcFrequency,
                RecordingActive = reader.IsDBNull(7),
                Trimmed = !reader.IsDBNull(6),
                LostByEtw = reader.IsDBNull(8) ? null : reader.GetInt32(8),
                DroppedByProfiler = NullableInt64(reader, 9)
            };
            SessionQueries.Report(command, 1);
        }
        if (!capture.Available) return capture;

        return capture with { Textures = ReadTexturesAt(connection, transaction, snapshot.EndQpc, token) };
    }

    private static IReadOnlyList<SnapshotTexture> ReadTexturesAt(SqliteConnection connection, SqliteTransaction transaction,
        long atQpc, CancellationToken token)
    {
        var resources = new Dictionary<long, bool>();
        var textures = new Dictionary<long, SnapshotTexture>();
        using (var command = Command(connection, transaction, """
            SELECT g.GraphicsObjectId, g.Kind, g.Handle, g.StartQpc, g.EndQpc, g.EndReason, g.Trimmed,
                CASE WHEN g.Kind <> 'Buffer' THEN (SELECT (SELECT json_group_object(n.Name, json(e.Payload -> n.Field))
                    FROM (SELECT key AS Field, value AS Name
                        FROM json_each('["' || replace(t.FieldNames, ',', '","') || '"]') WHERE value IN
                        ('Width', 'Height', 'Depth', 'MipLevels', 'ArraySize', 'Format', 'Dimension', 'Usage',
                         'BindFlags', 'CPUAccessFlags', 'MiscFlags', 'SampleCount', 'SampleQuality', 'StructureByteStride')
                        ORDER BY value) n)
                 FROM GraphicsObjectEvents ge JOIN Events e ON e.Seq = ge.Seq JOIN EventTypes t ON t.TypeId = e.TypeId
                 WHERE ge.GraphicsObjectId = g.GraphicsObjectId AND ge.Role IN ('Start', 'Rundown') AND e.Qpc <= $qpc
                 ORDER BY e.Qpc DESC, e.Seq DESC LIMIT 1) END, g.KnownFromQpc
            FROM GraphicsObjects g
            WHERE g.Kind IN ('Buffer', 'Texture1D', 'Texture2D', 'Texture3D')
                AND COALESCE(g.StartQpc, g.KnownFromQpc) <= $qpc AND (g.EndQpc IS NULL OR g.EndQpc > $qpc)
            ORDER BY g.GraphicsObjectId
            """, ("$qpc", atQpc)))
        {
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                token.ThrowIfCancellationRequested();
                resources.Add(reader.GetInt64(0), reader.GetBoolean(6));
                if (reader.GetString(1) == "Buffer") continue;
                var texture = new SnapshotTexture(reader.GetInt64(0), reader.GetString(1), reader.GetString(2),
                    null, NullableInt64(reader, 3), NullableInt64(reader, 4),
                    NullableString(reader, 5), reader.GetBoolean(6), NullableString(reader, 7) ?? "{}", [])
                {
                    KnownFromQpc = reader.GetInt64(8)
                };
                textures.Add(texture.Id, texture);
            }
            SessionQueries.Report(command, resources.Count);
        }

        var allocations = new Dictionary<long, List<SnapshotTextureAllocation>>();
        using (var command = Command(connection, transaction, """
            WITH Links AS (
                SELECT l.* FROM GraphicsLinks l JOIN Events b ON b.Seq = l.StartSeq
                LEFT JOIN Events e ON e.Seq = l.EndSeq
                WHERE b.Qpc <= $qpc AND (l.EndSeq IS NULL OR e.Qpc > $qpc)),
            Globals AS (
                SELECT l.*, a.Trimmed AS AllocationTrimmed
                FROM Links l JOIN GraphicsObjects a ON a.GraphicsObjectId = l.TargetId
                WHERE l.Kind = 'AllocationGlobal' AND COALESCE(a.StartQpc, a.KnownFromQpc) <= $qpc
                    AND (a.EndQpc IS NULL OR a.EndQpc > $qpc))
            SELECT r.GraphicsObjectId, d.GraphicsObjectId, d.Handle, rd.Handle, NULLIF(d.Qualifier, ''),
                da.TargetId, da.Handle, da.Qualifier,
                rd.Trimmed OR COALESCE(da.Trimmed, 0) OR COALESCE(da.AllocationTrimmed, 0) OR r.Trimmed OR d.Trimmed
            FROM GraphicsObjects r JOIN Links rd ON rd.SourceId = r.GraphicsObjectId AND rd.Kind = 'ResourceAllocation'
            JOIN GraphicsObjects d ON d.GraphicsObjectId = rd.TargetId
            LEFT JOIN Globals da ON da.SourceId = d.GraphicsObjectId
            WHERE r.Kind IN ('Texture1D', 'Texture2D', 'Texture3D')
                AND COALESCE(r.StartQpc, r.KnownFromQpc) <= $qpc AND (r.EndQpc IS NULL OR r.EndQpc > $qpc)
                AND COALESCE(d.StartQpc, d.KnownFromQpc) <= $qpc AND (d.EndQpc IS NULL OR d.EndQpc > $qpc)
            ORDER BY r.GraphicsObjectId, d.GraphicsObjectId, da.TargetId
            """, ("$qpc", atQpc)))
        {
            using var reader = command.ExecuteReader();
            long rows = 0;
            while (reader.Read())
            {
                token.ThrowIfCancellationRequested();
                var id = reader.GetInt64(0);
                if (!allocations.TryGetValue(id, out var links)) allocations.Add(id, links = []);
                links.Add(new(reader.GetInt64(1), reader.GetString(2), reader.GetString(3), NullableString(reader, 4),
                    NullableInt64(reader, 5), NullableString(reader, 6), NullableString(reader, 7),
                    null, reader.GetBoolean(8)));
                rows++;
            }
            SessionQueries.Report(command, rows);
        }
        var ids = new HashSet<long>(resources.Keys);
        foreach (var allocation in allocations.Values.SelectMany(links => links))
        {
            ids.Add(allocation.DeviceAllocationId);
            if (allocation.AdapterAllocationId is { } adapterId) ids.Add(adapterId);
        }
        var fields = ReadTextureFields(connection, transaction, ids, atQpc, token);
        string? HandleAtSnapshot(long id, string field) =>
            fields.TryGetValue((id, field), out var value) && value != 0 ? $"0x{value:x16}" : null;

        var claims = new Dictionary<ulong, int>();
        var completeClaims = true;
        foreach (var (id, trimmed) in resources)
        {
            token.ThrowIfCancellationRequested();
            if (fields.TryGetValue((id, "hKMResource"), out var handle))
            {
                if (handle != 0) claims[handle] = claims.GetValueOrDefault(handle) + 1;
            }
            else if (trimmed)
                // Missing history could hide a competing claim.
                completeClaims = false;
        }

        var result = new List<SnapshotTexture>(textures.Count);
        foreach (var texture in textures.Values)
        {
            token.ThrowIfCancellationRequested();
            var links = new List<SnapshotTextureAllocation>();
            if (completeClaims && fields.TryGetValue((texture.Id, "hKMResource"), out var handle) &&
                claims.GetValueOrDefault(handle) == 1 && allocations.TryGetValue(texture.Id, out var candidates))
            {
                foreach (var group in candidates.GroupBy(allocation =>
                    (allocation.DeviceAllocationId, allocation.KernelResource)))
                {
                    token.ThrowIfCancellationRequested();
                    var resource = group.First();
                    // StartSeq is arrival order; both endpoints must confirm the bridge at the snapshot's QPC.
                    if (HandleAtSnapshot(texture.Id, "hKMResource") != resource.KernelResource ||
                        HandleAtSnapshot(resource.DeviceAllocationId, "hThunkResource") != resource.KernelResource)
                        continue;
                    var complete = false;
                    foreach (var allocation in group)
                    {
                        if (allocation.AdapterAllocationId is not { } adapterId ||
                            HandleAtSnapshot(allocation.DeviceAllocationId, "hVidMmGlobalAlloc") != allocation.GlobalAllocation ||
                            HandleAtSnapshot(allocation.DeviceAllocationId, "pDxgAdapter") != allocation.Adapter ||
                            HandleAtSnapshot(adapterId, "hVidMmGlobalAlloc") != allocation.GlobalAllocation ||
                            HandleAtSnapshot(adapterId, "pDxgAdapter") != allocation.Adapter)
                            continue;
                        links.Add(allocation with
                        {
                            SizeBytes = fields.TryGetValue((adapterId, "allocSize"), out var size) ? size : null
                        });
                        complete = true;
                    }
                    if (!complete)
                        links.Add(resource with { AdapterAllocationId = null, GlobalAllocation = null, Adapter = null, SizeBytes = null });
                }
            }
            result.Add(texture with
            {
                DeviceHandle = HandleAtSnapshot(texture.Id, "pID3D11Device"),
                Allocations = links.Distinct().ToArray()
            });
        }
        return result;
    }

    private static Dictionary<(long Id, string Field), ulong> ReadTextureFields(SqliteConnection connection,
        SqliteTransaction transaction, IReadOnlyCollection<long> ids, long qpc, CancellationToken token)
    {
        var fields = new Dictionary<(long Id, string Field), ulong>();
        if (ids.Count == 0) return fields;
        using var command = Command(connection, transaction, """
            WITH Evidence AS (
                SELECT ge.GraphicsObjectId, n.value AS Name, e.Payload -> n.key AS Value,
                    json_extract('["' || replace(t.FieldTypes, ',', '","') || '"]', '$[' || n.key || ']') AS FieldType,
                    e.Qpc, e.Seq
                FROM GraphicsObjectEvents ge JOIN Events e ON e.Seq = ge.Seq JOIN EventTypes t ON t.TypeId = e.TypeId,
                    json_each('["' || replace(t.FieldNames, ',', '","') || '"]') n
                WHERE ge.GraphicsObjectId IN (SELECT value FROM json_each($ids))
                    AND ge.Role IN ('Start', 'Rundown') AND e.Qpc <= $qpc
                    AND n.value IN ('pID3D11Device', 'hKMResource', 'hThunkResource', 'hVidMmGlobalAlloc', 'pDxgAdapter', 'allocSize')),
            Latest AS (
                SELECT *, ROW_NUMBER() OVER (PARTITION BY GraphicsObjectId, Name ORDER BY Qpc DESC, Seq DESC) AS Revision
                FROM Evidence WHERE FieldType IN ('Int32', 'UInt32', 'Int64', 'UInt64')
                    AND (Name <> 'allocSize' OR Value NOT LIKE '-%')
                    AND (Name <> 'pID3D11Device' OR Value <> '0'))
            SELECT GraphicsObjectId, Name, Value, FieldType FROM Latest WHERE Revision = 1
            """, ("$ids", JsonSerializer.Serialize(ids)), ("$qpc", qpc));
        using var reader = command.ExecuteReader();
        while (reader.Read())
        {
            token.ThrowIfCancellationRequested();
            var text = reader.GetString(2);
            var value = text.StartsWith('-')
                ? reader.GetString(3) == nameof(FieldType.Int32)
                    ? unchecked((uint)int.Parse(text, CultureInfo.InvariantCulture))
                    : unchecked((ulong)long.Parse(text, CultureInfo.InvariantCulture))
                : ulong.Parse(text, CultureInfo.InvariantCulture);
            fields.Add((reader.GetInt64(0), reader.GetString(1)), value);
        }
        SessionQueries.Report(command, fields.Count);
        return fields;
    }
}

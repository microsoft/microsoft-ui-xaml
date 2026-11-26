// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Storage;

public enum SnapshotCompositionStatus
{
    Available, NotRecorded, DifferentProcess, DifferentClock, OutsideRecording, HistoryTrimmed, NoEvents, Unavailable
}

public sealed record SnapshotCompositionCapture(SnapshotCompositionStatus Status, long AtQpc,
    CompositionRecording? Recording = null)
{
    public bool Available => Status == SnapshotCompositionStatus.Available;
    public string? Error { get; init; }
}

public sealed record NativeCompositionLifetime(long NativeResourceId, int ProcessId, string ResourcePointer,
    string ResourceHandle, long? ResourceType, bool? CreateShared, bool? OpenShared, string? CreationChannel,
    string? ReleaseChannel, string? ExternalHandleAndChannel, long? CreationSeq, long? CreationQpc, long? ReleaseSeq,
    long? ReleaseQpc, double? CreationMilliseconds, double? ReleaseMilliseconds, double? ObservedDurationMilliseconds,
    string Status, long? BoundSeq, long? BoundQpc, string? BoundReason, bool Trimmed);

public sealed record WinUICompositionLifetime(long WinUISeq, int ProcessId, int ThreadId, long? ResourceKind,
    string ResourceName, double? WinUIMilliseconds, long CandidateCount, string Correlation, long? NativeResourceId,
    string? ResourcePointer, string? ResourceHandle, long? ResourceType, long? CreationSeq, long? ReleaseSeq,
    double? CreationMilliseconds, double? ReleaseMilliseconds, double? GapMilliseconds, string? NativeStatus,
    bool? NativeTrimmed);

// Counts describe retained event observations, not unique COM objects, GPU allocations or leak counts.
public sealed record CompositionRecording(SessionFileInfo Session, long NativeCreationObservations,
    long NativeReleaseObservations, long MatchedReleaseObservations, IReadOnlyList<NativeCompositionLifetime> NativeLifetimes,
    IReadOnlyList<WinUICompositionLifetime> WinUICreations);

public static partial class SessionFile
{
    public static CompositionRecording ReadComposition(string path, CancellationToken cancellationToken = default)
    {
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        return ReadComposition(connection, transaction, cancellationToken);
    }

    // Snapshot state is bounded by its end QPC, not by when the viewer opens. Trace replay is not presumed to
    // share a live snapshot's clock, and PID alone is insufficient when a process ID has been reused.
    public static SnapshotCompositionCapture ReadCompositionSnapshot(string? path, TreeSnapshotDocument snapshot,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(snapshot);
        cancellationToken.ThrowIfCancellationRequested();
        if (path is null) return new(SnapshotCompositionStatus.NotRecorded, snapshot.EndQpc);
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        if (!HasCompositionViews(connection, transaction))
            return new(SnapshotCompositionStatus.NotRecorded, snapshot.EndQpc);
        using (var command = Command(connection, transaction, """
            SELECT ProcessId, ProcessCreatedUtc, Source, QpcFrequency, StartQpc, CompleteFromQpc,
                EXISTS (SELECT 1 FROM Events e JOIN EventTypes t ON t.TypeId = e.TypeId
                    WHERE t.Provider = 'DirectComposition' AND t.Launcher = 0 AND t.EventId IN (1, 30) AND e.Qpc <= $qpc)
                    OR EXISTS (SELECT 1 FROM GraphicsObjects WHERE Kind = 'CompositionResource' AND KnownFromQpc <= $qpc)
            FROM SessionInfo
            """, ("$qpc", snapshot.EndQpc)))
        {
            using var reader = command.ExecuteReader();
            if (!reader.Read()) throw new InvalidDataException("The session file has no session record.");
            var status = reader.GetInt64(0) != snapshot.ProcessId || reader.IsDBNull(1) ||
                ParseUtc(reader.GetString(1)).ToFileTimeUtc() != snapshot.CreationTimeFileTime
                    ? SnapshotCompositionStatus.DifferentProcess
                : reader.GetString(2) != "Live" || reader.IsDBNull(3) ||
                    reader.GetInt64(3) != snapshot.QpcFrequency ? SnapshotCompositionStatus.DifferentClock
                : reader.IsDBNull(4) || snapshot.EndQpc < reader.GetInt64(4) ? SnapshotCompositionStatus.OutsideRecording
                : !reader.IsDBNull(5) && snapshot.EndQpc < reader.GetInt64(5) ? SnapshotCompositionStatus.HistoryTrimmed
                : !reader.GetBoolean(6) ? SnapshotCompositionStatus.NoEvents : SnapshotCompositionStatus.Available;
            SessionQueries.Report(command, 1);
            if (status != SnapshotCompositionStatus.Available) return new(status, snapshot.EndQpc);
        }
        return new(SnapshotCompositionStatus.Available, snapshot.EndQpc,
            ReadComposition(connection, transaction, cancellationToken, snapshot.EndQpc));
    }

    private static bool HasCompositionViews(SqliteConnection connection, SqliteTransaction transaction) =>
        Sql.Scalar(connection, """
            SELECT COUNT(*) FROM sqlite_master
            WHERE type = 'view' AND name IN ('CompositionLifetimes', 'WinUICompositionLifetimes')
            """, transaction) == 2 && HasColumn(connection, transaction, "EventTypes", "EventId");

    private static CompositionRecording ReadComposition(SqliteConnection connection, SqliteTransaction transaction,
        CancellationToken cancellationToken, long? atQpc = null)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var (info, _, _) = ReadSession(connection, transaction);
        if (!HasCompositionViews(connection, transaction))
            throw new InvalidDataException("This session has no composition lifetime index. Re-import the original ETL with this profiler.");

        long creations;
        long releases;
        long matched;
        using (var command = Command(connection, transaction, """
            SELECT COALESCE(SUM(t.EventId = 1), 0), COALESCE(SUM(t.EventId = 30), 0),
                COALESCE(SUM(t.EventId = 30 AND g.StartSeq IS NOT NULL AND g.StartQpc <= e.Qpc), 0)
            FROM Events e JOIN EventTypes t ON t.TypeId = e.TypeId
            LEFT JOIN GraphicsObjectEvents ge ON ge.Seq = e.Seq AND ge.Role = 'Stop'
            LEFT JOIN GraphicsObjects g ON g.GraphicsObjectId = ge.GraphicsObjectId AND g.Kind = 'CompositionResource'
            WHERE t.Provider = 'DirectComposition' AND t.Launcher = 0 AND t.EventId IN (1, 30)
                AND ($qpc IS NULL OR e.Qpc <= $qpc)
            """, ("$qpc", atQpc)))
        {
            using var reader = command.ExecuteReader();
            if (!reader.Read()) throw new InvalidDataException("The session has no composition observation counts.");
            creations = reader.GetInt64(0);
            releases = reader.GetInt64(1);
            matched = reader.GetInt64(2);
            SessionQueries.Report(command, 1);
        }

        var native = new List<NativeCompositionLifetime>();
        using (var command = Command(connection, transaction, """
            SELECT NativeResourceId, ProcessId, ResourcePointer, ResourceHandle, ResourceType, CreateShared, OpenShared,
                CreationChannel, ReleaseChannel, ExternalHandleAndChannel, CreationSeq, CreationQpc, ReleaseSeq, ReleaseQpc,
                CreationMs, ReleaseMs, ObservedDurationMs, Status, BoundSeq, BoundQpc, BoundReason, Trimmed
            FROM CompositionLifetimes
            WHERE $qpc IS NULL OR CreationQpc <= $qpc OR ReleaseQpc <= $qpc
            ORDER BY COALESCE(CreationQpc, ReleaseQpc), NativeResourceId
            """, ("$qpc", atQpc)))
        {
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                cancellationToken.ThrowIfCancellationRequested();
                var row = new NativeCompositionLifetime(reader.GetInt64(0), reader.GetInt32(1), reader.GetString(2), reader.GetString(3),
                    NullableInt64(reader, 4), NullableBoolean(reader, 5), NullableBoolean(reader, 6),
                    NullableString(reader, 7), NullableString(reader, 8), NullableString(reader, 9),
                    NullableInt64(reader, 10), NullableInt64(reader, 11), NullableInt64(reader, 12), NullableInt64(reader, 13),
                    NullableDouble(reader, 14), NullableDouble(reader, 15), NullableDouble(reader, 16), reader.GetString(17),
                    NullableInt64(reader, 18), NullableInt64(reader, 19), NullableString(reader, 20), reader.GetBoolean(21));
                if (atQpc is { } cutoff)
                {
                    if (row.ReleaseQpc > cutoff)
                        row = row with { ReleaseSeq = null, ReleaseQpc = null, ReleaseChannel = null,
                            ReleaseMilliseconds = null, ObservedDurationMilliseconds = null, Status = "EndNotObserved" };
                    if (row.BoundQpc > cutoff)
                        row = row with { BoundSeq = null, BoundQpc = null, BoundReason = null };
                }
                native.Add(row);
            }
            SessionQueries.Report(command, native.Count);
        }

        var winui = new List<WinUICompositionLifetime>();
        var nativeById = native.ToDictionary(row => row.NativeResourceId);
        using (var command = Command(connection, transaction, """
            SELECT WinUISeq, ProcessId, ThreadId, ResourceKind, ResourceName, WinUIMs, CandidateCount, Correlation,
                NativeResourceId, ResourcePointer, ResourceHandle, ResourceType, CreationSeq, ReleaseSeq, CreationMs,
                ReleaseMs, GapMs, NativeStatus, NativeTrimmed
            FROM WinUICompositionLifetimes
            WHERE $qpc IS NULL OR WinUISeq IN (SELECT Seq FROM WinUICompositionCreations WHERE Qpc <= $qpc)
            ORDER BY WinUISeq
            """, ("$qpc", atQpc)))
        {
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                cancellationToken.ThrowIfCancellationRequested();
                var row = new WinUICompositionLifetime(reader.GetInt64(0), reader.GetInt32(1), reader.GetInt32(2), NullableInt64(reader, 3),
                    reader.GetString(4), NullableDouble(reader, 5), reader.GetInt64(6), reader.GetString(7),
                    NullableInt64(reader, 8), NullableString(reader, 9), NullableString(reader, 10), NullableInt64(reader, 11),
                    NullableInt64(reader, 12), NullableInt64(reader, 13), NullableDouble(reader, 14), NullableDouble(reader, 15),
                    NullableDouble(reader, 16), NullableString(reader, 17), NullableBoolean(reader, 18));
                if (atQpc is not null && row.NativeResourceId is { } nativeId && nativeById.TryGetValue(nativeId, out var lifetime))
                    row = row with { ReleaseSeq = lifetime.ReleaseSeq, ReleaseMilliseconds = lifetime.ReleaseMilliseconds,
                        NativeStatus = lifetime.Status };
                winui.Add(row);
            }
            SessionQueries.Report(command, winui.Count);
        }
        return new(info, creations, releases, matched, native, winui);
    }
}

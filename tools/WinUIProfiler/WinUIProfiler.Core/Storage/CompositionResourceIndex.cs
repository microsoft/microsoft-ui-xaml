// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// Retained event evidence only: no live-object map or pointer lifetime state survives trimming.
internal sealed class CompositionResourceIndex : IDisposable
{
    internal const string Schema = """
        CREATE TABLE CompositionResourceEvidence (
            Seq INTEGER PRIMARY KEY REFERENCES Events (Seq),
            Qpc INTEGER NOT NULL,
            ThreadId INTEGER NOT NULL,
            Kind TEXT NOT NULL,        -- Create, Release, Identity or WinUI
            InputStatus TEXT NOT NULL,
            ChannelId TEXT,            -- exact unsigned values, never SQLite REAL
            ResourceHandle TEXT,
            ResourcePointer TEXT,      -- native resource owner, not a public COM interface
            PublicObjectId TEXT,       -- canonical public IUnknown identity, not IVisual
            ResourceType INTEGER,
            CreateShared INTEGER,
            OpenShared INTEGER,
            ResourceKind INTEGER,
            ResourceName TEXT);
        CREATE INDEX CompositionNativeIdentity ON CompositionResourceEvidence
            (Kind, ChannelId, ResourceHandle, ResourcePointer, Qpc);
        CREATE INDEX CompositionPublicIdentity ON CompositionResourceEvidence (Kind, PublicObjectId, Qpc);
        CREATE VIEW CompositionResourceLifetimes AS
        -- One observed resource-handle generation, scoped to this recording's process instance.
        -- A Release is resource release, NOT final COM destruction or GPU retirement.
        -- Handle reuse bounds missing release evidence; NoReleaseObserved is not a leak or a live count.
        WITH Creates AS (
            SELECT e.*,
                ROW_NUMBER() OVER (PARTITION BY ChannelId, ResourceHandle ORDER BY Qpc, Seq) AS Generation,
                LEAD(Qpc) OVER (PARTITION BY ChannelId, ResourceHandle ORDER BY Qpc, Seq) AS NextCreateQpc,
                COUNT(*) OVER (PARTITION BY ChannelId, ResourceHandle, Qpc) AS ConcurrentCreations
            FROM CompositionResourceEvidence e WHERE Kind = 'Create' AND InputStatus = 'Valid'),
        Ends AS (
            SELECT c.*,
                (SELECT MIN(r.Qpc) FROM CompositionResourceEvidence r
                 WHERE r.Kind = 'Release' AND r.InputStatus = 'Valid'
                    AND r.ChannelId = c.ChannelId AND r.ResourceHandle = c.ResourceHandle
                    AND r.ResourcePointer = c.ResourcePointer AND r.Qpc >= c.Qpc
                    AND (c.NextCreateQpc IS NULL OR r.Qpc < c.NextCreateQpc)) AS ReleaseQpc
            FROM Creates c)
        SELECT c.Seq AS ResourceId, s.SessionId, s.ProcessId, c.Generation, c.ChannelId,
            c.ResourceHandle, c.ResourcePointer, c.ResourceType, c.CreateShared, c.OpenShared,
            c.Qpc AS CreateQpc, c.NextCreateQpc, c.ConcurrentCreations,
            (SELECT MIN(r.Seq) FROM CompositionResourceEvidence r
             WHERE r.Kind = 'Release' AND r.InputStatus = 'Valid'
                AND r.ChannelId = c.ChannelId AND r.ResourceHandle = c.ResourceHandle
                AND r.ResourcePointer = c.ResourcePointer AND r.Qpc = c.ReleaseQpc) AS ReleaseSeq,
            c.ReleaseQpc, COALESCE(c.ReleaseQpc, c.NextCreateQpc) AS EndQpc,
            CASE WHEN c.ConcurrentCreations > 1 THEN 'AmbiguousCreation'
                 WHEN c.ReleaseQpc IS NOT NULL THEN 'Released'
                 WHEN c.NextCreateQpc IS NOT NULL THEN 'HandleReuse'
                 ELSE 'NoReleaseObserved' END AS ResourceState,
            s.CompleteFromSeq, s.LostByEtw, s.DroppedByProfiler
        FROM Ends c CROSS JOIN SessionInfo s;
        CREATE VIEW CompositionResourceLinks AS
        -- WinUI attribution requires its own creation event and an exact public-identity association.
        -- Compositor/channel ownership alone never attributes app-created objects to WinUI.
        -- QPC, not buffer arrival order, bounds handles and reused public addresses. Multiple candidates
        -- or multiple WinUI creations claiming one generation remain unmatched, never guessed.
        WITH Lifetimes AS MATERIALIZED (SELECT * FROM CompositionResourceLifetimes),
        Candidates AS MATERIALIZED (
            SELECT w.Seq AS WinUICreateSeq, l.ResourceId, MIN(i.Seq) AS IdentitySeq
            FROM CompositionResourceEvidence w
            JOIN CompositionResourceEvidence i ON i.Kind = 'Identity' AND i.InputStatus = 'Valid'
                AND i.PublicObjectId = w.PublicObjectId AND i.Qpc <= w.Qpc
            JOIN Lifetimes l ON l.ChannelId = i.ChannelId AND l.ResourceHandle = i.ResourceHandle
                AND l.ResourcePointer = i.ResourcePointer
                AND i.Qpc >= l.CreateQpc AND w.Qpc >= l.CreateQpc
                AND (l.NextCreateQpc IS NULL OR w.Qpc < l.NextCreateQpc)
                AND (l.ReleaseQpc IS NULL OR w.Qpc <= l.ReleaseQpc)
            WHERE w.Kind = 'WinUI' AND w.InputStatus = 'Valid'
                AND NOT EXISTS (
                    SELECT 1 FROM CompositionResourceEvidence newer
                    WHERE newer.Kind = 'Identity' AND newer.InputStatus = 'Valid'
                        AND newer.PublicObjectId = i.PublicObjectId
                        AND newer.Qpc > i.Qpc AND newer.Qpc <= w.Qpc)
            GROUP BY w.Seq, l.ResourceId),
        Counts AS (
            SELECT WinUICreateSeq, COUNT(*) AS CandidateCount, MIN(ResourceId) AS CandidateId,
                MIN(IdentitySeq) AS IdentitySeq FROM Candidates GROUP BY WinUICreateSeq),
        Claims AS (
            SELECT ResourceId, COUNT(*) AS WinUICreations FROM Candidates GROUP BY ResourceId),
        Resolved AS (
            SELECT w.*, COALESCE(c.CandidateCount, 0) AS CandidateCount, c.CandidateId, c.IdentitySeq,
                CASE WHEN w.InputStatus <> 'Valid' THEN w.InputStatus
                     WHEN c.CandidateCount IS NULL THEN 'NoIdentityMatch'
                     WHEN c.CandidateCount > 1 THEN 'AmbiguousIdentity'
                     WHEN l.ConcurrentCreations > 1 THEN 'AmbiguousCreation'
                     WHEN claims.WinUICreations > 1 THEN 'DuplicateWinUICreation'
                     ELSE 'Matched' END AS LinkStatus
            FROM CompositionResourceEvidence w
            LEFT JOIN Counts c ON c.WinUICreateSeq = w.Seq
            LEFT JOIN Lifetimes l ON l.ResourceId = c.CandidateId
            LEFT JOIN Claims claims ON claims.ResourceId = c.CandidateId
            WHERE w.Kind = 'WinUI')
        SELECT r.Seq AS WinUICreateSeq, r.Qpc AS WinUICreateQpc, r.ThreadId, r.ResourceKind,
            r.ResourceName, r.PublicObjectId, r.LinkStatus, r.CandidateCount,
            CASE WHEN r.LinkStatus = 'Matched' THEN r.IdentitySeq END AS IdentitySeq,
            l.ResourceId, l.Generation, l.ChannelId, l.ResourceHandle, l.ResourcePointer, l.ResourceType,
            l.CreateQpc AS NativeCreateQpc, l.ReleaseSeq, l.ReleaseQpc, l.EndQpc, l.ResourceState,
            s.SessionId, s.ProcessId, s.CompleteFromSeq, s.LostByEtw, s.DroppedByProfiler
        FROM Resolved r
        LEFT JOIN Lifetimes l ON r.LinkStatus = 'Matched' AND l.ResourceId = r.CandidateId
        CROSS JOIN SessionInfo s;
        CREATE VIEW CompositionResourceSummary AS
        -- Counts retained WinUI creation observations only, never all objects on the same compositor.
        -- Unmatched includes old producers, unsupported surface mappings and ambiguous evidence.
        -- Released is a matched resource release; ReleaseNotObserved is not final object liveness.
        SELECT ResourceKind, ResourceName, COUNT(*) AS CreatedObservations,
            SUM(PublicObjectId IS NOT NULL) AS PublicIdentityObserved,
            SUM(LinkStatus = 'Matched') AS MatchedResources,
            SUM(LinkStatus = 'Matched' AND ResourceState = 'Released') AS ReleasedResources,
            SUM(LinkStatus = 'Matched' AND ResourceState <> 'Released') AS ReleaseNotObserved,
            SUM(LinkStatus <> 'Matched') AS UnmatchedObservations
        FROM CompositionResourceLinks GROUP BY ResourceKind, ResourceName;
        """;

    private readonly SqliteCommand insert;
    private readonly SqliteCommand trim;

    internal CompositionResourceIndex(SqliteConnection connection)
    {
        insert = Sql.Prepare(connection, """
            INSERT INTO CompositionResourceEvidence (Seq, Qpc, ThreadId, Kind, InputStatus, ChannelId, ResourceHandle,
                ResourcePointer, PublicObjectId, ResourceType, CreateShared, OpenShared, ResourceKind, ResourceName)
            VALUES ($seq, $qpc, $thread, $kind, $status, $channel, $handle, $pointer, $public, $type,
                $createShared, $openShared, $resourceKind, $resourceName)
            """, ("$seq", SqliteType.Integer), ("$qpc", SqliteType.Integer), ("$thread", SqliteType.Integer),
            ("$kind", SqliteType.Text), ("$status", SqliteType.Text), ("$channel", SqliteType.Text),
            ("$handle", SqliteType.Text), ("$pointer", SqliteType.Text), ("$public", SqliteType.Text),
            ("$type", SqliteType.Integer), ("$createShared", SqliteType.Integer), ("$openShared", SqliteType.Integer),
            ("$resourceKind", SqliteType.Integer), ("$resourceName", SqliteType.Text));
        trim = Sql.Prepare(connection, "DELETE FROM CompositionResourceEvidence WHERE Seq < $cutoff",
            ("$cutoff", SqliteType.Integer));
    }

    internal static int EstimatedBytes(ProfilerEvent data) => KindOf(data) is null ? 0 : 512;

    internal void Add(long sequence, ProfilerEvent data, SqliteTransaction transaction)
    {
        if (KindOf(data) is not { } kind) return;
        uint? channel = null, handle = null, resourceType = null, resourceKind = null;
        ulong? pointer = null, publicId = null;
        bool? createShared = null, openShared = null;
        string? resourceName = null;
        string status;
        if (kind == "WinUI")
        {
            resourceKind = UInt32(Value(data, "ResourceKind"));
            resourceName = Value(data, "ResourceName") as string;
            publicId = Nonzero(Value(data, "PublicObjectId"));
            status = Boolean(Value(data, "HasLifetime")) != false || string.IsNullOrEmpty(resourceName)
                ? "MalformedWinUICreation"
                : publicId is null ? "MissingPublicIdentity" : "Valid";
        }
        else
        {
            channel = UInt32(Value(data, kind == "Identity" ? "ChannelId" : "channelHandle"));
            handle = UInt32(Value(data, kind == "Identity" ? "ResourceHandle" : "resourceHandle"));
            pointer = Nonzero(Value(data, kind == "Identity" ? "ResourcePointer" : "resourcePointer"));
            if (kind == "Identity") publicId = Nonzero(Value(data, "PublicObjectId"));
            if (kind == "Create")
            {
                resourceType = UInt32(Value(data, "resourceType"));
                createShared = Boolean(Value(data, "createShared"));
                openShared = Boolean(Value(data, "openShared"));
            }
            status = kind != "Identity" && data.Version != 0 ? "UnsupportedNativeVersion"
                : channel is null || handle is null or 0 || pointer is null ||
                    kind == "Identity" && publicId is null ||
                    kind == "Create" && (resourceType is null || createShared is null || openShared is null)
                    ? "MalformedNativeIdentity" : "Valid";
        }

        insert.Transaction = transaction;
        Set("$seq", sequence);
        Set("$qpc", data.TimestampQpc);
        Set("$thread", data.ThreadId);
        Set("$kind", kind);
        Set("$status", status);
        Set("$channel", Text(channel));
        Set("$handle", Text(handle));
        Set("$pointer", Text(pointer));
        Set("$public", Text(publicId));
        Set("$type", resourceType is { } type ? (long)type : null);
        Set("$createShared", createShared);
        Set("$openShared", openShared);
        Set("$resourceKind", resourceKind);
        Set("$resourceName", resourceName);
        insert.ExecuteNonQuery();
    }

    internal void Trim(long cutoff, SqliteTransaction transaction)
    {
        trim.Transaction = transaction;
        trim.Parameters["$cutoff"].Value = cutoff;
        trim.ExecuteNonQuery();
    }

    private void Set(string name, object? value) => insert.Parameters[name].Value = value ?? DBNull.Value;

    private static string? KindOf(ProfilerEvent data) => data switch
    {
        { Provider: XamlProvider.DirectComposition, TraceLogging: false, EventId: 1 } => "Create",
        { Provider: XamlProvider.DirectComposition, TraceLogging: false, EventId: 30 } => "Release",
        { Provider: XamlProvider.CompositionDiagnostics, TraceLogging: true, Name: "CompositionResourceIdentity" } => "Identity",
        { Provider: XamlProvider.XamlProfiler, TraceLogging: true, Name: "GraphicsResourceCreated" }
            when UInt32(Value(data, "ResourceKind")) is <= 12 && Boolean(Value(data, "HasLifetime")) != true => "WinUI",
        _ => null
    };

    private static object? Value(ProfilerEvent data, string name)
    {
        object? result = null;
        var found = false;
        foreach (var field in data.Fields)
        {
            if (field.Name != name) continue;
            if (found) return null;
            result = field.Value;
            found = true;
        }
        return result;
    }

    private static ulong? Unsigned(object? value) => value switch
    {
        ulong number => number,
        uint number => number,
        long number => unchecked((ulong)number),
        int number => unchecked((uint)number),
        _ => null
    };

    private static uint? UInt32(object? value) => Unsigned(value) is { } number && number <= uint.MaxValue ? (uint)number : null;
    private static ulong? Nonzero(object? value) => Unsigned(value) is { } number && number != 0 ? number : null;
    private static string? Text(ulong? value) => value is { } number ? $"0x{number:x16}" : null;

    private static bool? Boolean(object? value) => value switch
    {
        bool flag => flag,
        _ => Unsigned(value) is 0 ? false : Unsigned(value) is 1 ? true : null
    };

    public void Dispose()
    {
        insert.Dispose();
        trim.Dispose();
    }
}

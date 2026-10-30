// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// ProcessStartMilliseconds is when the process was created, in milliseconds from the start of the recording; negative
// when it was created earlier (attach), null when unknown (a recorded trace).
public sealed record SessionFileInfo(int ProcessId, string? ProcessName, string Source, string? EtlPath,
    SessionCoverage Coverage, DateTime StartedUtc, DateTime? EndedUtc, string? EndReason, long StoredEvents,
    long? CompleteFromSeq, int? LostByEtw, long? DroppedByProfiler, long LimitBytes, long UIElements, long Frames,
    double? ProcessStartMilliseconds);

// A UI element and when its stored events were first and last, in milliseconds from the start of the recording.
public sealed record SessionObject(long ObjectId, long Address, string? Type, string? Name, long Events,
    double FirstMilliseconds, double LastMilliseconds);

// A UI element an event mentions: the field that holds its address, and the element's type and name when known.
public sealed record ElementMention(long ObjectId, string Field, string? Type, string? Name);

// A frame on a UI thread and its stored events by bucket (timing as in SessionSchema). Drawn is false for each thread's
// latest frame, which may still be in progress. Times are milliseconds from the start of the recording; null when no
// tick with this number is stored.
public sealed record SessionFrame(long FrameId, int ThreadId, long FrameNumber, int Ticks, bool Drawn, double? StartMilliseconds,
    double? LastTickStartMilliseconds, double? EndMilliseconds, double TickMilliseconds, long Events, long Xaml, long Composition,
    long Other);

// Reads a session file written by SessionWriter, also while it is still being written.
public static class SessionFile
{
    public static SessionFileInfo ReadInfo(string path)
    {
        using var connection = Open(path);
        return ReadSession(connection, null).Info;
    }

    // Delivers the stored events in Seq order. RelativeMilliseconds is measured from the start of the recording.
    public static long Read(string path, Action<ProfilerEvent> sink, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(sink);
        return ReadEvents(path, "SELECT Qpc, ThreadId, TypeId, Payload FROM Events ORDER BY Seq", [],
            (profilerEvent, _) => sink(profilerEvent), cancellationToken);
    }

    // The UI elements with stored events, in the order they were first seen; all, or the one with an ID or address.
    public static IReadOnlyList<SessionObject> ReadObjects(string path, long? objectId = null, long? address = null)
    {
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        var (_, frequency, startQpc) = ReadSession(connection, transaction);
        var start = startQpc.GetValueOrDefault();
        using var command = Command(connection, transaction, """
            WITH Spans AS (
                SELECT o.ObjectId, o.Address, o.Type, o.Name, COUNT(DISTINCT r.Seq) AS Events, MIN(r.Seq) AS FirstSeq,
                    MAX(r.Seq) AS LastSeq
                FROM Objects o JOIN ObjectEvents r ON r.ObjectId = o.ObjectId
                WHERE ($id IS NULL OR o.ObjectId = $id) AND ($address IS NULL OR o.Address = $address)
                GROUP BY o.ObjectId)
            SELECT s.ObjectId, s.Address, s.Type, s.Name, s.Events, f.Qpc, l.Qpc
            FROM Spans s JOIN Events f ON f.Seq = s.FirstSeq JOIN Events l ON l.Seq = s.LastSeq
            ORDER BY s.FirstSeq, s.ObjectId
            """, ("$id", objectId), ("$address", address));
        using var reader = command.ExecuteReader();
        var objects = new List<SessionObject>();
        while (reader.Read())
        {
            objects.Add(new(reader.GetInt64(0), reader.GetInt64(1), NullableString(reader, 2), NullableString(reader, 3),
                reader.GetInt64(4), (reader.GetInt64(5) - start) * 1000.0 / frequency, (reader.GetInt64(6) - start) * 1000.0 / frequency));
        }

        return objects;
    }

    // Delivers a page of the events that mention one element (count -1 for all) in Seq order, with the fields that hold
    // its address and the number of the frame the event belongs to, if any.
    public static long ReadTimeline(string path, long objectId, int offset, int count,
        Action<ProfilerEvent, IReadOnlyList<string>, long?> sink, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(sink);
        return ReadEvents(path, """
            SELECT e.Qpc, e.ThreadId, e.TypeId, e.Payload, GROUP_CONCAT(r.Field), f.FrameNumber
            FROM ObjectEvents r
                JOIN Events e ON e.Seq = r.Seq
                LEFT JOIN FrameEvents x ON x.Seq = r.Seq
                LEFT JOIN Frames f ON f.FrameId = x.FrameId
            WHERE r.ObjectId = $id
            GROUP BY r.Seq
            ORDER BY r.Seq
            LIMIT $count OFFSET $offset
            """, [("$id", objectId), ("$count", count), ("$offset", offset)],
            (profilerEvent, reader) => sink(profilerEvent,
                [.. reader.GetString(4).Split(',').Select(field => int.Parse(field, CultureInfo.InvariantCulture)).Order()
                    .Select(field => profilerEvent.Fields[field].Name)],
                reader.IsDBNull(5) ? null : reader.GetInt64(5)),
            cancellationToken);
    }

    // How many events mention one element.
    public static long CountTimeline(string path, long objectId)
    {
        using var connection = Open(path);
        using var command = Command(connection, null, "SELECT COUNT(DISTINCT Seq) FROM ObjectEvents WHERE ObjectId = $id",
            ("$id", objectId));
        return (long)command.ExecuteScalar()!;
    }

    // The UI element a tree snapshot node was when the snapshot was taken, from the WinRT object addresses the snapshot
    // recorded (its IUnknown and its XAML diagnostics handle). Events that carry both an element's address and its WinRT
    // object's map one to the other: the latest at or before the snapshot's QPC time, or else the first after it. The
    // element is then the one at that address at that time. Null when no stored event maps the addresses, for example when
    // the recording started after the element was created.
    public static long? FindSnapshotElement(string path, IReadOnlyCollection<long> addresses, long qpc)
    {
        ArgumentNullException.ThrowIfNull(addresses);
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        (string Event, string Element, string Peer)[] pairs =
        [
            ("PeerAssociated", "ElementId", "PeerHandle"), ("ElementEnteredTree", "ElementId", "PeerHandle"),
            ("ChildAdded", "ChildId", "ChildPeerHandle"), ("ChildInserted", "ChildId", "ChildPeerHandle")
        ];
        var shapes = new List<(long TypeId, int Element, int Peer)>();
        using (var command = Command(connection, transaction, "SELECT TypeId, Name, FieldNames FROM EventTypes"))
        {
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                var names = reader.GetString(2).Split(',');
                foreach (var pair in pairs.Where(pair => pair.Event == reader.GetString(1)))
                {
                    var element = Array.IndexOf(names, pair.Element);
                    var peer = Array.IndexOf(names, pair.Peer);
                    if (element >= 0 && peer >= 0)
                    {
                        shapes.Add((reader.GetInt64(0), element, peer));
                    }
                }
            }
        }

        if (shapes.Count == 0 || addresses.Count == 0)
        {
            return null;
        }

        string Field(Func<(long TypeId, int Element, int Peer), int> index) => "CASE TypeId " +
            string.Join(" ", shapes.Select(shape => $"WHEN {shape.TypeId} THEN json_extract(Payload, '$[{index(shape)}]')")) + " END";
        long? before = null;
        long? after = null;
        using (var command = Command(connection, transaction, $"""
            SELECT Qpc, Element
            FROM (SELECT Seq, Qpc, {Field(shape => shape.Element)} AS Element, {Field(shape => shape.Peer)} AS Peer
                FROM Events
                WHERE TypeId IN ({string.Join(", ", shapes.Select(shape => shape.TypeId))}))
            WHERE Peer IN (SELECT value FROM json_each($addresses))
            ORDER BY Seq
            """, ("$addresses", $"[{string.Join(",", addresses.Select(address => address.ToString(CultureInfo.InvariantCulture)))}]")))
        {
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                if (reader.GetInt64(0) <= qpc)
                {
                    before = reader.GetInt64(1);
                }
                else
                {
                    after = reader.GetInt64(1);
                    break;
                }
            }
        }

        if ((before ?? after) is not { } elementAddress)
        {
            return null;
        }

        using var objects = Command(connection, transaction, """
            SELECT o.ObjectId, MIN(e.Qpc)
            FROM Objects o JOIN ObjectEvents r ON r.ObjectId = o.ObjectId JOIN Events e ON e.Seq = r.Seq
            WHERE o.Address = $address
            GROUP BY o.ObjectId
            ORDER BY MIN(r.Seq)
            """, ("$address", elementAddress));
        using var rows = objects.ExecuteReader();
        long? found = null;
        while (rows.Read())
        {
            if (found is null || rows.GetInt64(1) <= qpc)
            {
                found = rows.GetInt64(0);
            }
        }

        return found;
    }

    // The frames with stored events, in the order they were first seen: all, or the one with an ID.
    public static IReadOnlyList<SessionFrame> ReadFrames(string path, long? frameId = null) =>
        QueryFrames(path, frameId, null).Frames;

    // The frames that changed after a read that saw events up to a Seq (null for all frames), in the order they were
    // first seen, and the Seq to pass next time. Trimming is not a change; it changes CompleteFromSeq instead.
    public static (IReadOnlyList<SessionFrame> Frames, long? LastSeq) ReadChangedFrames(string path, long? afterSeq) =>
        QueryFrames(path, null, afterSeq);

    // A frame's ticks and events only change with an event attributed to it, and whether it drew changes when a later
    // frame appears on its thread. So the frames that changed after a Seq are, on each thread with events after it, the
    // frames from the one below the lowest of those on.
    private static (IReadOnlyList<SessionFrame> Frames, long? LastSeq) QueryFrames(string path, long? frameId, long? afterSeq)
    {
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        var (_, frequency, startQpc) = ReadSession(connection, transaction);
        var start = startQpc.GetValueOrDefault();
        using var command = Command(connection, transaction, """
            WITH Changed AS (
                    SELECT f.ThreadId, MIN(f.FrameNumber) AS FrameNumber
                    FROM FrameEvents r JOIN Frames f ON f.FrameId = r.FrameId
                    WHERE r.Seq > $after
                    GROUP BY f.ThreadId),
                Since AS (
                    SELECT c.ThreadId, COALESCE((SELECT MAX(p.FrameNumber) FROM Frames p
                        WHERE p.ThreadId = c.ThreadId AND p.FrameNumber < c.FrameNumber), c.FrameNumber) AS FrameNumber
                    FROM Changed c),
                Latest AS (SELECT ThreadId, MAX(FrameNumber) AS FrameNumber FROM Frames GROUP BY ThreadId)
            SELECT f.FrameId, f.ThreadId, f.FrameNumber, f.Ticks, f.StartQpc, f.LastTickStartQpc, f.EndQpc, f.TickTimeQpc,
                f.FrameNumber < t.FrameNumber, COUNT(*), SUM(r.Bucket = 'Xaml'), SUM(r.Bucket = 'Composition'), SUM(r.Bucket = 'Other')
            FROM Frames f
                JOIN FrameEvents r ON r.FrameId = f.FrameId
                JOIN Latest t ON t.ThreadId = f.ThreadId
            WHERE ($id IS NULL OR f.FrameId = $id) AND ($after IS NULL OR f.FrameId IN (
                SELECT p.FrameId FROM Frames p JOIN Since s ON s.ThreadId = p.ThreadId AND p.FrameNumber >= s.FrameNumber))
            GROUP BY f.FrameId
            ORDER BY f.FrameId
            """, ("$id", frameId), ("$after", afterSeq));
        var frames = new List<SessionFrame>();
        using (var reader = command.ExecuteReader())
        {
            double? Milliseconds(int ordinal) => reader.IsDBNull(ordinal) ? null : (reader.GetInt64(ordinal) - start) * 1000.0 / frequency;
            while (reader.Read())
            {
                frames.Add(new(reader.GetInt64(0), reader.GetInt32(1), reader.GetInt64(2), reader.GetInt32(3), reader.GetBoolean(8),
                    Milliseconds(4), Milliseconds(5), Milliseconds(6), reader.GetInt64(7) * 1000.0 / frequency, reader.GetInt64(9),
                    reader.GetInt64(10), reader.GetInt64(11), reader.GetInt64(12)));
            }
        }

        using var last = Command(connection, transaction, "SELECT MAX(Seq) FROM FrameEvents");
        return (frames, last.ExecuteScalar() is long seq ? seq : null);
    }

    // Delivers a frame's events in Seq order with each event's bucket and the UI elements it mentions: optionally one
    // bucket's, those matching a filter (see FrameEventsQuery), and a page of them (count -1 for all).
    public static long ReadFrameEvents(string path, long frameId, string? bucket, string? filter, int offset, int count,
        Action<ProfilerEvent, string, IReadOnlyList<ElementMention>> sink, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(sink);
        var (where, parameters) = FrameEventsQuery(frameId, bucket, filter);
        return ReadEvents(path, $"""
            SELECT e.Qpc, e.ThreadId, e.TypeId, e.Payload, r.Bucket,
                (SELECT json_group_array(json_array(m.ObjectId, m.Field, o.Type, o.Name))
                FROM ObjectEvents m JOIN Objects o ON o.ObjectId = m.ObjectId
                WHERE m.Seq = r.Seq)
            {where}
            ORDER BY r.Seq
            LIMIT $count OFFSET $offset
            """, [.. parameters, ("$count", count), ("$offset", offset)],
            (profilerEvent, reader) => sink(profilerEvent, reader.GetString(4), Mentions(profilerEvent, reader.GetString(5))),
            cancellationToken);
    }

    // How many of a frame's events ReadFrameEvents would deliver without a page.
    public static long CountFrameEvents(string path, long frameId, string? bucket, string? filter)
    {
        using var connection = Open(path);
        var (where, parameters) = FrameEventsQuery(frameId, bucket, filter);
        using var command = Command(connection, null, $"SELECT COUNT(*) {where}", parameters);
        return (long)command.ExecuteScalar()!;
    }

    // A filter keeps the events whose name contains its text, case-insensitively. Text also matches string field values
    // that contain it; a number (decimal, or hex with 0x) matches numeric fields with exactly that value instead.
    private static (string Where, (string Name, object? Value)[] Parameters) FrameEventsQuery(long frameId, string? bucket,
        string? filter)
    {
        filter = string.IsNullOrWhiteSpace(filter) ? null : filter.Trim();
        long? number = filter is null ? null
            : filter.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
                ? ulong.TryParse(filter.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out var hex)
                    ? unchecked((long)hex) : null
                : long.TryParse(filter, NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out var value) ? value : null;
        return ("""
            FROM FrameEvents r JOIN Events e ON e.Seq = r.Seq JOIN EventTypes t ON t.TypeId = e.TypeId
            WHERE r.FrameId = $id AND ($bucket IS NULL OR r.Bucket = $bucket)
                AND ($like IS NULL OR t.Name LIKE $like ESCAPE '\' OR EXISTS (
                    SELECT 1 FROM json_each(e.Payload) v
                    WHERE ($number IS NULL AND v.type = 'text' AND v.value LIKE $like ESCAPE '\')
                        OR (v.type IN ('integer', 'real') AND v.value = $number)))
            """, [("$id", frameId), ("$bucket", bucket),
                ("$like", filter is null ? null : $"%{filter.Replace(@"\", @"\\").Replace("%", @"\%").Replace("_", @"\_")}%"),
                ("$number", number)]);
    }

    // Runs a query whose columns are Qpc, ThreadId, TypeId, Payload and then any others, which the sink reads from the
    // reader along with each event.
    private static long ReadEvents(string path, string query, (string Name, object? Value)[] parameters,
        Action<ProfilerEvent, SqliteDataReader> sink, CancellationToken cancellationToken)
    {
        using var connection = Open(path);
        // One read transaction, so a file still being written is read as one consistent snapshot: every stored event's
        // type is among the types read.
        using var transaction = connection.BeginTransaction(deferred: true);
        var (info, frequency, startQpc) = ReadSession(connection, transaction);
        // Files written before TraceLogging was added hold only TraceLogging events, apart from DxgKrnl's manifest events.
        var traceLogging = Sql.Scalar(connection, "SELECT COUNT(*) FROM pragma_table_info('EventTypes') WHERE name = 'TraceLogging'",
            transaction) > 0 ? "TraceLogging" : $"Provider <> '{XamlProvider.DxgKrnl}'";
        var types = new Dictionary<long, (XamlProvider Provider, bool TraceLogging, string Name, int Opcode, string[] Names, FieldType[] Types)>();
        using (var command = Command(connection, transaction,
            $"SELECT TypeId, Provider, Name, Opcode, FieldNames, FieldTypes, {traceLogging} FROM EventTypes"))
        {
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                var names = reader.GetString(4).Split(',', StringSplitOptions.RemoveEmptyEntries);
                var fieldTypes = reader.GetString(5).Split(',', StringSplitOptions.RemoveEmptyEntries)
                    .Select(EventPayload.ParseType).ToArray();
                if (fieldTypes.Length != names.Length)
                {
                    throw new InvalidDataException("An event type's field names and types do not match.");
                }

                types[reader.GetInt64(0)] = (Enum.Parse<XamlProvider>(reader.GetString(1)), reader.GetBoolean(6), reader.GetString(2),
                    reader.GetInt32(3), names, fieldTypes);
            }
        }

        long delivered = 0;
        using (var command = Command(connection, transaction, query, parameters))
        {
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                cancellationToken.ThrowIfCancellationRequested();
                var qpc = reader.GetInt64(0);
                startQpc ??= qpc;
                var type = types[reader.GetInt64(2)];
                sink(new(qpc, (qpc - startQpc.Value) * 1000.0 / frequency, info.ProcessId, reader.GetInt32(1), type.Provider,
                    type.TraceLogging, type.Name, type.Opcode, EventPayload.Read(reader.GetFieldValue<byte[]>(3), type.Names, type.Types)),
                    reader);
                delivered++;
            }
        }

        return delivered;
    }

    // The [ObjectId, Field, Type, Name] arrays of ReadFrameEvents' mentions column.
    private static IReadOnlyList<ElementMention> Mentions(ProfilerEvent profilerEvent, string json)
    {
        if (json == "[]")
        {
            return [];
        }

        static string? Text(JsonElement value) => value.ValueKind == JsonValueKind.Null ? null : value.GetString();
        using var document = JsonDocument.Parse(json);
        return [.. document.RootElement.EnumerateArray().Select(mention => new ElementMention(mention[0].GetInt64(),
            profilerEvent.Fields[mention[1].GetInt32()].Name, Text(mention[2]), Text(mention[3])))];
    }

    private static SqliteCommand Command(SqliteConnection connection, SqliteTransaction? transaction, string text,
        params (string Name, object? Value)[] parameters)
    {
        var command = connection.CreateCommand();
        command.Transaction = transaction;
        command.CommandText = text;
        foreach (var (name, value) in parameters)
        {
            command.Parameters.AddWithValue(name, value ?? DBNull.Value);
        }

        return command;
    }
    private static SqliteConnection Open(string path)
    {
        ArgumentException.ThrowIfNullOrEmpty(path);
        if (!File.Exists(path))
        {
            throw new FileNotFoundException("The session file was not found.", path);
        }

        var connection = new SqliteConnection(new SqliteConnectionStringBuilder
        {
            DataSource = path,
            Mode = SqliteOpenMode.ReadOnly,
            Pooling = false
        }.ToString());
        try
        {
            connection.Open();
            var version = Sql.Scalar(connection, "PRAGMA user_version");
            if (version != SessionSchema.Version)
            {
                throw new InvalidDataException(
                    $"'{path}' is not a WinUI Profiler session file of version {SessionSchema.Version} (found version {version}).");
            }

            return connection;
        }
        catch
        {
            connection.Dispose();
            throw;
        }
    }

    // A recording that stopped before its frequency was known falls back to this machine's QPC frequency.
    private static (SessionFileInfo Info, long Frequency, long? StartQpc) ReadSession(SqliteConnection connection,
        SqliteTransaction? transaction)
    {
        using var command = connection.CreateCommand();
        command.Transaction = transaction;
        // Files written before StartedQpc was added are otherwise the same; without it the process start is unknown.
        var startedQpc = Sql.Scalar(connection, "SELECT COUNT(*) FROM pragma_table_info('SessionInfo') WHERE name = 'StartedQpc'",
            transaction) > 0 ? "StartedQpc" : "NULL";
        command.CommandText = $"""
            SELECT ProcessId, ProcessName, Source, EtlPath, Coverage, StartedUtc, EndedUtc, EndReason, CompleteFromSeq,
                LostByEtw, DroppedByProfiler, LimitBytes, QpcFrequency, StartQpc,
                (SELECT COALESCE(MAX(Seq) - MIN(Seq) + 1, 0) FROM Events), (SELECT COUNT(*) FROM Objects), (SELECT COUNT(*) FROM Frames),
                ProcessCreatedUtc, {startedQpc}
            FROM SessionInfo
            """;
        using var reader = command.ExecuteReader();
        if (!reader.Read())
        {
            throw new InvalidDataException("The session file has no session record.");
        }

        var frequency = reader.IsDBNull(12) ? Stopwatch.Frequency : reader.GetInt64(12);
        long? startQpc = reader.IsDBNull(13) ? null : reader.GetInt64(13);
        var startedUtc = ParseUtc(reader.GetString(5));
        // StartedQpc was read with StartedUtc, so the process creation time maps onto the events' QPC clock.
        double? processStart = startQpc is { } start && !reader.IsDBNull(17) && !reader.IsDBNull(18)
            ? (reader.GetInt64(18) - start) * 1000.0 / frequency - (startedUtc - ParseUtc(reader.GetString(17))).TotalMilliseconds
            : null;
        var info = new SessionFileInfo(reader.GetInt32(0), NullableString(reader, 1), reader.GetString(2), NullableString(reader, 3),
            Enum.Parse<SessionCoverage>(reader.GetString(4)), startedUtc,
            reader.IsDBNull(6) ? null : ParseUtc(reader.GetString(6)), NullableString(reader, 7),
            reader.GetInt64(14), reader.IsDBNull(8) ? null : reader.GetInt64(8), reader.IsDBNull(9) ? null : reader.GetInt32(9),
            reader.IsDBNull(10) ? null : reader.GetInt64(10), reader.GetInt64(11), reader.GetInt64(15), reader.GetInt64(16), processStart);
        return (info, frequency, startQpc);
    }

    private static string? NullableString(SqliteDataReader reader, int ordinal) => reader.IsDBNull(ordinal) ? null : reader.GetString(ordinal);

    private static DateTime ParseUtc(string text) => DateTime.Parse(text, CultureInfo.InvariantCulture, DateTimeStyles.RoundtripKind);
}

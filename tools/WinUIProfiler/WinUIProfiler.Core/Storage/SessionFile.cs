// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// ProcessStartMilliseconds is when the process was created, in milliseconds from the start of the recording; negative
// when it was created earlier (attach, or a recorded trace, whose recording starts at the process's first stored event),
// null when unknown (a recorded trace without the kernel's process start event for it).
public sealed record SessionFileInfo(int ProcessId, string? ProcessName, string Source, string? EtlPath,
    SessionCoverage Coverage, DateTime StartedUtc, DateTime? EndedUtc, string? EndReason, long StoredEvents,
    long? CompleteFromSeq, int? LostByEtw, long? DroppedByProfiler, long LimitBytes, long UIElements, long Visuals, long Frames,
    double? ProcessStartMilliseconds);

// The kinds of objects the session file indexes (SessionSchema's Objects.Kind).
public enum ObjectKind
{
    Element,
    Visual
}

// A UI element or composition visual and when its stored events were first and last, in milliseconds from the start of the
// recording. A visual's name is its owner element's label.
public sealed record SessionObject(long ObjectId, ObjectKind Kind, long Address, string? Type, string? Name, long Events,
    double FirstMilliseconds, double LastMilliseconds);

// An object an event mentions: the field that holds its address, and the object's type and name when known. For a visual
// whose parent's children were cleared, the field is "child of" the parent's field.
public sealed record ObjectMention(long ObjectId, ObjectKind Kind, string Field, string? Type, string? Name);

// A tick on a UI thread and its stored events by bucket: a row of the Ticks view. Drawn is whether the tick drew: whether
// the thread's next tick has a higher frame number; null for the thread's latest tick. Times are milliseconds from the
// start of the recording; null when not stored (see SessionSchema). LastSeq is the tick's latest event: its stop, once it
// ended. Slow is whether it took more than a 60 Hz frame.
public sealed record SessionFrame(long FrameId, int ThreadId, long FrameNumber, bool? Drawn, double? StartMilliseconds,
    double? EndMilliseconds, long Events, long Xaml, long Composition, long Other, long LastSeq, bool Slow)
{
    public double? TickMilliseconds => EndMilliseconds - StartMilliseconds;
}

// The kinds of items in the Frames tab's list (the FrameList view's ItemType): a tick, a region of interest, or another
// interesting row outside every region.
public enum FrameListItemType
{
    Tick,
    Region,
    Row
}

// One item of the FrameList view, in list order: a tick (Tick), or a region or another interesting row (Row). RegionId is
// the region a tick ends; a region's own. InInteresting and InSlow say whether the Show filter's Interesting and Slow
// choices keep the item.
public sealed record FrameListItem(long ListIndex, FrameListItemType ItemType, SessionFrame? Tick, InterestingRow? Row,
    long? RegionId, bool InInteresting, bool InSlow);

// One of the first ticks that drew (the LaunchSummary view): its frame number and when it ended, from the process's start
// or from the start of the recording.
public sealed record LaunchFrame(long FrameNumber, double Milliseconds, bool FromProcessStart);

// The first frame drawn on the first UI thread: its number and when its last tick ended, in milliseconds from the start of
// the recording.
public sealed record DrawnFrame(int ThreadId, long FrameNumber, double EndMilliseconds);

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

    // Delivers the stored events of some types after a Seq (null for all), in Seq order. Returns the last Seq stored (null
    // when there is none), to pass next time, and the file's CompleteFromSeq as the events were read: when it is more than
    // one after the Seq passed, events after that Seq were deleted, to stay within the size limit, before they were read. A
    // type is a provider's ETW ID and an event name, which matches case-insensitively.
    public static (long? LastSeq, long? CompleteFromSeq) ReadEvents(string path,
        IReadOnlyCollection<(Guid ProviderId, string Name)> eventTypes, long? afterSeq, Action<ProfilerEvent> sink,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(eventTypes);
        ArgumentNullException.ThrowIfNull(sink);
        long? lastSeq = null;
        long? completeFromSeq = null;
        ReadEvents(path, (connection, transaction, types) =>
            {
                // The same snapshot as the events read, so the next read starts right after them, and a deletion is seen
                // along with the events it left.
                using (var command = Command(connection, transaction,
                    "SELECT (SELECT MAX(Seq) FROM Events), (SELECT CompleteFromSeq FROM SessionInfo)"))
                {
                    using var reader = command.ExecuteReader();
                    reader.Read();
                    lastSeq = reader.IsDBNull(0) ? null : reader.GetInt64(0);
                    completeFromSeq = reader.IsDBNull(1) ? null : reader.GetInt64(1);
                }

                var ids = types.Where(type => eventTypes.Any(wanted => wanted.ProviderId == type.Value.ProviderId &&
                        string.Equals(wanted.Name, type.Value.Name, StringComparison.OrdinalIgnoreCase)))
                    .Select(type => type.Key.ToString(CultureInfo.InvariantCulture)).ToArray();
                return $"""
                    SELECT Qpc, ThreadId, TypeId, Payload FROM Events
                    WHERE {(ids.Length == 0 ? "0" : $"TypeId IN ({string.Join(',', ids)})")} AND Seq > $after
                    ORDER BY Seq
                    """;
            }, [("$after", afterSeq ?? 0)], (profilerEvent, _) => sink(profilerEvent), cancellationToken);
        return (lastSeq, completeFromSeq);
    }

    // The first frame drawn on the UI thread with the first stored frame, as the Frames tab's launch line shows; null
    // while there is none, and once the file has deleted its oldest events to stay within its size limit, since the first
    // frames may have been among them (the Frames tab then shows no launch line either).
    public static DrawnFrame? ReadFirstDrawnFrame(string path)
    {
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        var (info, frequency, startQpc) = ReadSession(connection, transaction);
        if (info.CompleteFromSeq is not null)
        {
            return null;
        }

        using var command = Command(connection, transaction, """
            WITH Thread AS (SELECT ThreadId FROM Frames ORDER BY FrameId LIMIT 1)
            SELECT f.ThreadId, f.FrameNumber, f.EndQpc
            FROM Frames f JOIN Thread t ON t.ThreadId = f.ThreadId
            WHERE f.EndQpc IS NOT NULL
                AND f.FrameNumber < (SELECT MAX(p.FrameNumber) FROM Frames p WHERE p.ThreadId = f.ThreadId)
            ORDER BY f.FrameId
            LIMIT 1
            """);
        using var reader = command.ExecuteReader();
        return reader.Read() && startQpc is { } start
            ? new(reader.GetInt32(0), reader.GetInt64(1), (reader.GetInt64(2) - start) * 1000.0 / frequency)
            : null;
    }

    // The objects with stored events, in the order they were first seen: all, those of one kind, or the one with an ID or
    // address.
    public static IReadOnlyList<SessionObject> ReadObjects(string path, ObjectKind? kind = null, long? objectId = null, long? address = null)
    {
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        var (_, frequency, startQpc) = ReadSession(connection, transaction);
        var start = startQpc.GetValueOrDefault();
        using var command = Command(connection, transaction, """
            WITH Spans AS (
                SELECT o.ObjectId, o.Kind, o.Address, o.Type, o.Name, COUNT(DISTINCT r.Seq) AS Events,
                    MIN(r.Seq) AS FirstSeq, MAX(r.Seq) AS LastSeq
                FROM Objects o JOIN ObjectEvents r ON r.ObjectId = o.ObjectId
                WHERE ($kind IS NULL OR o.Kind = $kind) AND ($id IS NULL OR o.ObjectId = $id)
                    AND ($address IS NULL OR o.Address = $address)
                GROUP BY o.ObjectId)
            SELECT s.ObjectId, s.Kind, s.Address, s.Type, s.Name, s.Events, f.Qpc, l.Qpc
            FROM Spans s JOIN Events f ON f.Seq = s.FirstSeq JOIN Events l ON l.Seq = s.LastSeq
            ORDER BY s.FirstSeq, s.ObjectId
            """, ("$kind", kind?.ToString()), ("$id", objectId), ("$address", address));
        using var reader = command.ExecuteReader();
        var objects = new List<SessionObject>();
        while (reader.Read())
        {
            objects.Add(new(reader.GetInt64(0), Enum.Parse<ObjectKind>(reader.GetString(1)), reader.GetInt64(2), NullableString(reader, 3),
                NullableString(reader, 4), reader.GetInt64(5), (reader.GetInt64(6) - start) * 1000.0 / frequency,
                (reader.GetInt64(7) - start) * 1000.0 / frequency));
        }

        return objects;
    }

    // Delivers a page of the events that mention one object (count -1 for all) in Seq order, with the fields that hold
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
                    .Select(field => FieldName(profilerEvent, field))],
                reader.IsDBNull(5) ? null : reader.GetInt64(5)),
            cancellationToken);
    }

    // How many events mention one object.
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

        return ObjectAt(connection, transaction, ObjectKind.Element, elementAddress, qpc);
    }

    // The visual a composition snapshot node was when the snapshot was taken. The snapshot records the visual's IVisual
    // pointer, the same address the visual events carry. Null when no stored event mentions it, for example when the
    // recording started after the visual was inserted.
    public static long? FindSnapshotVisual(string path, long address, long qpc)
    {
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        return ObjectAt(connection, transaction, ObjectKind.Visual, address, qpc);
    }

    // The object of a kind at an address at a QPC time: of those stored there, the last first seen at or before that
    // time, or else the first.
    private static long? ObjectAt(SqliteConnection connection, SqliteTransaction transaction, ObjectKind kind, long address, long qpc)
    {
        using var objects = Command(connection, transaction, """
            SELECT o.ObjectId, MIN(e.Qpc)
            FROM Objects o JOIN ObjectEvents r ON r.ObjectId = o.ObjectId JOIN Events e ON e.Seq = r.Seq
            WHERE o.Address = $address AND o.Kind = $kind
            GROUP BY o.ObjectId
            ORDER BY MIN(r.Seq)
            """, ("$address", address), ("$kind", kind.ToString()));
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

    // The ticks with stored events (the Ticks view), in the order they were first seen: all, or the one with an ID.
    public static IReadOnlyList<SessionFrame> ReadFrames(string path, long? frameId = null)
    {
        using var connection = Open(path);
        using var command = Command(connection, null, """
            SELECT TickId, ThreadId, FrameNumber, Drew, StartMs, EndMs, Events, Xaml, Composition, Other, LastSeq, Slow
            FROM Ticks
            WHERE $id IS NULL OR TickId = $id
            ORDER BY TickId
            """, ("$id", frameId));
        using var reader = command.ExecuteReader();
        var frames = new List<SessionFrame>();
        while (reader.Read())
        {
            frames.Add(new(reader.GetInt64(0), reader.GetInt32(1), reader.GetInt64(2), NullableBoolean(reader, 3), NullableDouble(reader, 4),
                NullableDouble(reader, 5), reader.GetInt64(6), reader.GetInt64(7), reader.GetInt64(8), reader.GetInt64(9),
                reader.GetInt64(10), reader.GetBoolean(11)));
        }

        return frames;
    }

    // The Frames tab's list (the FrameList view), in order.
    public static IReadOnlyList<FrameListItem> ReadFrameList(string path)
    {
        using var connection = Open(path);
        using var command = Command(connection, null, """
            SELECT ListIndex, ItemType, TickId, RegionId, RowType, Name, Trigger, ThreadId, FrameNumber, StartSeq, StopSeq,
                StartMs, StopMs, Drew, Slow, Events, Xaml, Composition, Other, InInteresting, InSlow
            FROM FrameList
            ORDER BY ListIndex
            """);
        using var reader = command.ExecuteReader();
        var items = new List<FrameListItem>();
        while (reader.Read())
        {
            var type = Enum.Parse<FrameListItemType>(reader.GetString(1));
            var tick = type == FrameListItemType.Tick
                ? new SessionFrame(reader.GetInt64(2), reader.GetInt32(7), reader.GetInt64(8), NullableBoolean(reader, 13),
                    NullableDouble(reader, 11), NullableDouble(reader, 12), reader.GetInt64(15), reader.GetInt64(16), reader.GetInt64(17),
                    reader.GetInt64(18), reader.GetInt64(10), reader.GetBoolean(14))
                : null;
            var row = type == FrameListItemType.Tick ? null
                : new InterestingRow(Enum.Parse<InterestingRowType>(reader.GetString(4)), reader.GetString(5), reader.GetInt32(7),
                    reader.GetInt64(9), reader.GetInt64(10), reader.GetDouble(11), reader.GetDouble(12), NullableInt64(reader, 8),
                    NullableString(reader, 6), NullableInt64(reader, 3), NullableInt64(reader, 2));
            items.Add(new(reader.GetInt64(0), type, tick, row, NullableInt64(reader, 3), reader.GetBoolean(19), reader.GetBoolean(20)));
        }

        return items;
    }

    // The regions of interest and the interesting rows (the InterestingRows view) in file order, a region before the rows
    // that start with it: all, or those that start in one region (by its RegionId), the region first.
    public static IReadOnlyList<InterestingRow> ReadInterestingRows(string path, long? regionId = null)
    {
        using var connection = Open(path);
        using var command = Command(connection, null, """
            SELECT Type, Name, Trigger, ThreadId, StartSeq, StopSeq, StartMs, StopMs, FrameNumber, RegionId, TickId
            FROM InterestingRows
            WHERE $region IS NULL OR RegionId = $region
            ORDER BY StartSeq, Type <> 'Region'
            """, ("$region", regionId));
        using var reader = command.ExecuteReader();
        var rows = new List<InterestingRow>();
        while (reader.Read())
        {
            rows.Add(new(Enum.Parse<InterestingRowType>(reader.GetString(0)), reader.GetString(1), reader.GetInt32(3), reader.GetInt64(4),
                reader.GetInt64(5), reader.GetDouble(6), reader.GetDouble(7), NullableInt64(reader, 8), NullableString(reader, 2),
                NullableInt64(reader, 9), NullableInt64(reader, 10)));
        }

        return rows;
    }

    // When the first ticks that drew ended (the LaunchSummary view); empty for a trimmed file.
    public static IReadOnlyList<LaunchFrame> ReadLaunchSummary(string path)
    {
        using var connection = Open(path);
        using var command = Command(connection, null, "SELECT FrameNumber, Ms, MeasuredFrom FROM LaunchSummary ORDER BY Position");
        using var reader = command.ExecuteReader();
        var frames = new List<LaunchFrame>();
        while (reader.Read())
        {
            frames.Add(new(reader.GetInt64(0), reader.GetDouble(1), reader.GetString(2) == "ProcessStart"));
        }

        return frames;
    }

    // Delivers a tick's events in Seq order with each event's bucket and the objects it mentions: optionally one
    // bucket's, those matching a filter (see EventsQuery), and a page of them (count -1 for all).
    public static long ReadFrameEvents(string path, long frameId, string? bucket, string? filter, int offset, int count,
        Action<ProfilerEvent, string, IReadOnlyList<ObjectMention>> sink, CancellationToken cancellationToken = default) =>
        ReadScopedEvents(path, FrameScope(frameId), bucket, filter, offset, count, sink, cancellationToken);

    // How many of a tick's events ReadFrameEvents would deliver without a page.
    public static long CountFrameEvents(string path, long frameId, string? bucket, string? filter) =>
        CountScopedEvents(path, FrameScope(frameId), bucket, filter);

    // Delivers one thread's events from firstSeq to lastSeq, as ReadFrameEvents does a tick's: the events of an
    // InterestingRow. An event that belongs to no tick, such as one on a thread that never ticks, is in bucket Other.
    public static long ReadSpanEvents(string path, int threadId, long firstSeq, long lastSeq, string? bucket, string? filter,
        int offset, int count, Action<ProfilerEvent, string, IReadOnlyList<ObjectMention>> sink,
        CancellationToken cancellationToken = default) =>
        ReadScopedEvents(path, SpanScope(threadId, firstSeq, lastSeq), bucket, filter, offset, count, sink, cancellationToken);

    // How many events ReadSpanEvents would deliver without a page.
    public static long CountSpanEvents(string path, int threadId, long firstSeq, long lastSeq, string? bucket, string? filter) =>
        CountScopedEvents(path, SpanScope(threadId, firstSeq, lastSeq), bucket, filter);

    // Where a query's events come from and which of them it keeps. Every scope names a view with EventDetails' columns d.
    private sealed record EventScope(string From, string Where, (string Name, object? Value)[] Parameters);

    private static EventScope FrameScope(long frameId) => new("TickEvents d", "d.TickId = $id", [("$id", frameId)]);

    private static EventScope SpanScope(int threadId, long firstSeq, long lastSeq) =>
        new("EventDetails d", "d.Seq BETWEEN $first AND $last AND d.ThreadId = $thread",
            [("$first", firstSeq), ("$last", lastSeq), ("$thread", threadId)]);

    private static long ReadScopedEvents(string path, EventScope scope, string? bucket, string? filter, int offset, int count,
        Action<ProfilerEvent, string, IReadOnlyList<ObjectMention>> sink, CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(sink);
        var (where, parameters) = EventsQuery(scope, bucket, filter);
        return ReadEvents(path, $"""
            SELECT d.Qpc, d.ThreadId, d.TypeId, d.Payload, d.Bucket, d.Mentions
            {where}
            ORDER BY d.Seq
            LIMIT $count OFFSET $offset
            """, [.. parameters, ("$count", count), ("$offset", offset)],
            (profilerEvent, reader) => sink(profilerEvent, reader.GetString(4), Mentions(reader.GetString(5))), cancellationToken);
    }

    private static long CountScopedEvents(string path, EventScope scope, string? bucket, string? filter)
    {
        using var connection = Open(path);
        var (where, parameters) = EventsQuery(scope, bucket, filter);
        using var command = Command(connection, null, $"SELECT COUNT(*) {where}", parameters);
        return (long)command.ExecuteScalar()!;
    }

    // A filter keeps the events whose name contains its text, case-insensitively. Text also matches string field values
    // that contain it; a number (decimal, or hex with 0x) matches numeric fields with exactly that value instead.
    private static (string Where, (string Name, object? Value)[] Parameters) EventsQuery(EventScope scope, string? bucket,
        string? filter)
    {
        filter = string.IsNullOrWhiteSpace(filter) ? null : filter.Trim();
        long? number = filter is null ? null
            : filter.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
                ? ulong.TryParse(filter.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out var hex)
                    ? unchecked((long)hex) : null
                : long.TryParse(filter, NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out var value) ? value : null;
        return ($"""
            FROM {scope.From}
            WHERE {scope.Where} AND ($bucket IS NULL OR d.Bucket = $bucket)
                AND ($like IS NULL OR d.Name LIKE $like ESCAPE '\' OR EXISTS (
                    SELECT 1 FROM json_each(d.Payload) v
                    WHERE ($number IS NULL AND v.type = 'text' AND v.value LIKE $like ESCAPE '\')
                        OR (v.type IN ('integer', 'real') AND v.value = $number)))
            """, [.. scope.Parameters, ("$bucket", bucket),
                ("$like", filter is null ? null : $"%{filter.Replace(@"\", @"\\").Replace("%", @"\%").Replace("_", @"\_")}%"),
                ("$number", number)]);
    }

    // Runs a query whose columns are Qpc, ThreadId, TypeId, Payload and then any others, which the sink reads from the
    // reader along with each event.
    private static long ReadEvents(string path, string query, (string Name, object? Value)[] parameters,
        Action<ProfilerEvent, SqliteDataReader> sink, CancellationToken cancellationToken) =>
        ReadEvents(path, (_, _, _) => query, parameters, sink, cancellationToken);

    // The same, with a query that depends on the file's event types, read in the same snapshot as the events.
    private static long ReadEvents(string path,
        Func<SqliteConnection, SqliteTransaction, IReadOnlyDictionary<long, StoredType>, string> query,
        (string Name, object? Value)[] parameters, Action<ProfilerEvent, SqliteDataReader> sink, CancellationToken cancellationToken)
    {
        using var connection = Open(path);
        // One read transaction, so a file still being written is read as one consistent snapshot: every stored event's
        // type is among the types read.
        using var transaction = connection.BeginTransaction(deferred: true);
        var (info, frequency, startQpc) = ReadSession(connection, transaction);
        // Files written before marker providers were added have no events from them.
        var marker = HasColumn(connection, transaction, "EventTypes", "ProviderId") ? "ProviderName, ProviderId" : "NULL, NULL";
        var types = new Dictionary<long, StoredType>();
        using (var command = Command(connection, transaction,
            $"SELECT TypeId, Provider, Name, Opcode, FieldNames, FieldTypes, TraceLogging, {marker} FROM EventTypes"))
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

                var provider = Enum.Parse<XamlProvider>(reader.GetString(1));
                var markerProvider = provider == XamlProvider.Marker
                    ? new MarkerProvider(reader.GetString(7), Guid.Parse(reader.GetString(8)))
                    : null;
                types[reader.GetInt64(0)] = new(provider, markerProvider, reader.GetBoolean(6), reader.GetString(2),
                    reader.GetInt32(3), names, fieldTypes);
            }
        }

        long delivered = 0;
        using (var command = Command(connection, transaction, query(connection, transaction, types), parameters))
        {
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                cancellationToken.ThrowIfCancellationRequested();
                var qpc = reader.GetInt64(0);
                startQpc ??= qpc;
                var type = types[reader.GetInt64(2)];
                sink(new(qpc, (qpc - startQpc.Value) * 1000.0 / frequency, info.ProcessId, reader.GetInt32(1), type.Provider,
                    type.TraceLogging, type.Name, type.Opcode, EventPayload.Read(reader.GetFieldValue<byte[]>(3), type.Names, type.Types),
                    type.Marker), reader);
                delivered++;
            }
        }

        return delivered;
    }

    // An EventTypes row.
    private sealed record StoredType(XamlProvider Provider, MarkerProvider? Marker, bool TraceLogging, string Name, int Opcode,
        string[] Names, FieldType[] Types)
    {
        public Guid ProviderId => Marker?.Id ?? XamlProviders.IdOf(Provider);
    }

    private static bool HasColumn(SqliteConnection connection, SqliteTransaction? transaction, string table, string column) =>
        Sql.Scalar(connection, $"SELECT COUNT(*) FROM pragma_table_info('{table}') WHERE name = $column", transaction,
            ("$column", column)) > 0;

    // The mentions column of EventDetails: one object per mention, with the name of the field that holds the address.
    private static IReadOnlyList<ObjectMention> Mentions(string json)
    {
        if (json == "[]")
        {
            return [];
        }

        static string? Text(JsonElement value) => value.ValueKind == JsonValueKind.Null ? null : value.GetString();
        using var document = JsonDocument.Parse(json);
        return [.. document.RootElement.EnumerateArray().Select(mention => new ObjectMention(mention.GetProperty("ObjectId").GetInt64(),
            Enum.Parse<ObjectKind>(mention.GetProperty("Kind").GetString()!), mention.GetProperty("Field").GetString()!,
            Text(mention.GetProperty("Type")), Text(mention.GetProperty("Name"))))];
    }

    // The name of the field a mention is in (ObjectEvents.Field): -1 - i for a child whose parent is in field i.
    private static string FieldName(ProfilerEvent profilerEvent, int field) =>
        field >= 0 ? profilerEvent.Fields[field].Name : $"child of {profilerEvent.Fields[-1 - field].Name}";

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

            // Files written before the shared views were added are not read.
            if (Sql.Scalar(connection, "SELECT COUNT(*) FROM sqlite_master WHERE type = 'view' AND name = 'FrameList'") == 0)
            {
                throw new InvalidDataException(
                    $"'{path}' was written by an earlier WinUI Profiler, without the shared views. Record it again.");
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
        // Files written before ProcessStartQpc was added are otherwise the same.
        var processStartQpc = HasColumn(connection, transaction, "SessionInfo", "ProcessStartQpc") ? "ProcessStartQpc" : "NULL";
        command.CommandText = $"""
            SELECT ProcessId, ProcessName, Source, EtlPath, Coverage, StartedUtc, EndedUtc, EndReason, CompleteFromSeq,
                LostByEtw, DroppedByProfiler, LimitBytes, QpcFrequency, StartQpc,
                (SELECT COALESCE(MAX(Seq) - MIN(Seq) + 1, 0) FROM Events), (SELECT COUNT(*) FROM Objects WHERE Kind = 'Element'),
                (SELECT COUNT(*) FROM Frames), ProcessCreatedUtc, StartedQpc, (SELECT COUNT(*) FROM Objects WHERE Kind = 'Visual'),
                {processStartQpc}
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
        // A recorded trace has the process start on the events' clock. For a live session, StartedQpc was read with
        // StartedUtc, so the process creation time maps onto the events' QPC clock.
        double? processStart = startQpc is not { } start ? null
            : !reader.IsDBNull(20) ? (reader.GetInt64(20) - start) * 1000.0 / frequency
            : !reader.IsDBNull(17) && !reader.IsDBNull(18)
                ? (reader.GetInt64(18) - start) * 1000.0 / frequency - (startedUtc - ParseUtc(reader.GetString(17))).TotalMilliseconds
                : null;
        var info = new SessionFileInfo(reader.GetInt32(0), NullableString(reader, 1), reader.GetString(2), NullableString(reader, 3),
            Enum.Parse<SessionCoverage>(reader.GetString(4)), startedUtc,
            reader.IsDBNull(6) ? null : ParseUtc(reader.GetString(6)), NullableString(reader, 7),
            reader.GetInt64(14), reader.IsDBNull(8) ? null : reader.GetInt64(8), reader.IsDBNull(9) ? null : reader.GetInt32(9),
            reader.IsDBNull(10) ? null : reader.GetInt64(10), reader.GetInt64(11), reader.GetInt64(15), reader.GetInt64(19),
            reader.GetInt64(16), processStart);
        return (info, frequency, startQpc);
    }

    private static string? NullableString(SqliteDataReader reader, int ordinal) => reader.IsDBNull(ordinal) ? null : reader.GetString(ordinal);

    private static long? NullableInt64(SqliteDataReader reader, int ordinal) => reader.IsDBNull(ordinal) ? null : reader.GetInt64(ordinal);

    private static double? NullableDouble(SqliteDataReader reader, int ordinal) => reader.IsDBNull(ordinal) ? null : reader.GetDouble(ordinal);

    private static bool? NullableBoolean(SqliteDataReader reader, int ordinal) => reader.IsDBNull(ordinal) ? null : reader.GetBoolean(ordinal);

    private static DateTime ParseUtc(string text) => DateTime.Parse(text, CultureInfo.InvariantCulture, DateTimeStyles.RoundtripKind);
}

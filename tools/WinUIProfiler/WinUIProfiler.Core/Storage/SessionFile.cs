// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Globalization;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

public sealed record SessionFileInfo(int ProcessId, string? ProcessName, string Source, string? EtlPath,
    SessionCoverage Coverage, DateTime StartedUtc, DateTime? EndedUtc, string? EndReason, long StoredEvents,
    long? CompleteFromSeq, int? LostByEtw, long? DroppedByProfiler, long LimitBytes, long UIElements, long Frames);

// A UI element and when its stored events were first and last, in milliseconds from the start of the recording.
public sealed record SessionObject(long ObjectId, long Address, string? Type, string? Name, long Events,
    double FirstMilliseconds, double LastMilliseconds);

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
        return ReadEvents(path, "SELECT Qpc, ThreadId, TypeId, Payload, NULL FROM Events ORDER BY Seq", [],
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

    // Delivers the events that mention one element, in Seq order, with the event's field that holds its address.
    public static long ReadTimeline(string path, long objectId, Action<ProfilerEvent, string> sink,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(sink);
        return ReadEvents(path, """
            SELECT e.Qpc, e.ThreadId, e.TypeId, e.Payload, r.Field
            FROM ObjectEvents r JOIN Events e ON e.Seq = r.Seq
            WHERE r.ObjectId = $id
            ORDER BY r.Seq, r.Field
            """, [("$id", objectId)], (profilerEvent, field) => sink(profilerEvent, profilerEvent.Fields[(int)(long)field!].Name),
            cancellationToken);
    }

    // The frames with stored events, by thread and frame number; all, or the one with an ID.
    public static IReadOnlyList<SessionFrame> ReadFrames(string path, long? frameId = null)
    {
        using var connection = Open(path);
        using var transaction = connection.BeginTransaction(deferred: true);
        var (_, frequency, startQpc) = ReadSession(connection, transaction);
        var start = startQpc.GetValueOrDefault();
        using var command = Command(connection, transaction, """
            SELECT f.FrameId, f.ThreadId, f.FrameNumber, f.Ticks, f.StartQpc, f.LastTickStartQpc, f.EndQpc, f.TickTimeQpc,
                f.FrameNumber < t.Latest, COUNT(*), SUM(r.Bucket = 'Xaml'), SUM(r.Bucket = 'Composition'), SUM(r.Bucket = 'Other')
            FROM Frames f
                JOIN FrameEvents r ON r.FrameId = f.FrameId
                JOIN (SELECT ThreadId, MAX(FrameNumber) AS Latest FROM Frames GROUP BY ThreadId) t ON t.ThreadId = f.ThreadId
            WHERE $id IS NULL OR f.FrameId = $id
            GROUP BY f.FrameId
            ORDER BY f.ThreadId, f.FrameNumber
            """, ("$id", frameId));
        using var reader = command.ExecuteReader();
        double? Milliseconds(int ordinal) => reader.IsDBNull(ordinal) ? null : (reader.GetInt64(ordinal) - start) * 1000.0 / frequency;
        var frames = new List<SessionFrame>();
        while (reader.Read())
        {
            frames.Add(new(reader.GetInt64(0), reader.GetInt32(1), reader.GetInt64(2), reader.GetInt32(3), reader.GetBoolean(8),
                Milliseconds(4), Milliseconds(5), Milliseconds(6), reader.GetInt64(7) * 1000.0 / frequency, reader.GetInt64(9),
                reader.GetInt64(10), reader.GetInt64(11), reader.GetInt64(12)));
        }

        return frames;
    }

    // Delivers a frame's events in Seq order, with each event's bucket; optionally only one bucket's.
    public static long ReadFrameEvents(string path, long frameId, string? bucket, Action<ProfilerEvent, string> sink,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(sink);
        return ReadEvents(path, """
            SELECT e.Qpc, e.ThreadId, e.TypeId, e.Payload, r.Bucket
            FROM FrameEvents r JOIN Events e ON e.Seq = r.Seq
            WHERE r.FrameId = $id AND ($bucket IS NULL OR r.Bucket = $bucket)
            ORDER BY r.Seq
            """, [("$id", frameId), ("$bucket", bucket)], (profilerEvent, rowBucket) => sink(profilerEvent, (string)rowBucket!),
            cancellationToken);
    }

    // Runs a query whose columns are Qpc, ThreadId, TypeId, Payload and one more value passed along with each event.
    private static long ReadEvents(string path, string query, (string Name, object? Value)[] parameters,
        Action<ProfilerEvent, object?> sink, CancellationToken cancellationToken)
    {
        using var connection = Open(path);
        // One read transaction, so a file still being written is read as one consistent snapshot: every stored event's
        // type is among the types read.
        using var transaction = connection.BeginTransaction(deferred: true);
        var (info, frequency, startQpc) = ReadSession(connection, transaction);
        var types = new Dictionary<long, (XamlProvider Provider, bool TraceLogging, string Name, int Opcode, string[] Names, FieldType[] Types)>();
        using (var command = Command(connection, transaction,
            "SELECT TypeId, Provider, Name, Opcode, FieldNames, FieldTypes, TraceLogging FROM EventTypes"))
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
                    reader.IsDBNull(4) ? null : reader.GetValue(4));
                delivered++;
            }
        }

        return delivered;
    }

    private static SqliteCommand Command(SqliteConnection connection, SqliteTransaction transaction, string text,
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
        command.CommandText = """
            SELECT ProcessId, ProcessName, Source, EtlPath, Coverage, StartedUtc, EndedUtc, EndReason, CompleteFromSeq,
                LostByEtw, DroppedByProfiler, LimitBytes, QpcFrequency, StartQpc,
                (SELECT COALESCE(MAX(Seq) - MIN(Seq) + 1, 0) FROM Events), (SELECT COUNT(*) FROM Objects), (SELECT COUNT(*) FROM Frames)
            FROM SessionInfo
            """;
        using var reader = command.ExecuteReader();
        if (!reader.Read())
        {
            throw new InvalidDataException("The session file has no session record.");
        }

        var info = new SessionFileInfo(reader.GetInt32(0), NullableString(reader, 1), reader.GetString(2), NullableString(reader, 3),
            Enum.Parse<SessionCoverage>(reader.GetString(4)), ParseUtc(reader.GetString(5)),
            reader.IsDBNull(6) ? null : ParseUtc(reader.GetString(6)), NullableString(reader, 7),
            reader.GetInt64(14), reader.IsDBNull(8) ? null : reader.GetInt64(8), reader.IsDBNull(9) ? null : reader.GetInt32(9),
            reader.IsDBNull(10) ? null : reader.GetInt64(10), reader.GetInt64(11), reader.GetInt64(15), reader.GetInt64(16));
        var frequency = reader.IsDBNull(12) ? Stopwatch.Frequency : reader.GetInt64(12);
        return (info, frequency, reader.IsDBNull(13) ? null : reader.GetInt64(13));
    }

    private static string? NullableString(SqliteDataReader reader, int ordinal) => reader.IsDBNull(ordinal) ? null : reader.GetString(ordinal);

    private static DateTime ParseUtc(string text) => DateTime.Parse(text, CultureInfo.InvariantCulture, DateTimeStyles.RoundtripKind);
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Buffers;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Globalization;
using System.Text;
using System.Text.Encodings.Web;
using System.Text.Json;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.Engine.Storage;

public enum SessionCoverage
{
    ProcessStart,
    Connection,
    Trace
}

// Where a session's events come from. For a recorded trace, pass the trace's QpcFrequency
// (OfflineEtlSource.ReadQpcFrequency): a null one is derived from the events once they span a second, and until then
// readers time events with this machine's frequency. A null StartQpc makes the target's first event time zero.
// LauncherProcessId is the process that launched the target, when known, as the profiler for a launch from the app,
// though Windows may create a packaged app's process: the events of another process the session receives are that
// process's (MarkerProviderSettings.Launcher). MarkerConfig is the text of the marker configuration whose providers the
// session records, which the file saves, so its launch timeline has the same milestones wherever it is read.
public sealed record SessionStart(int ProcessId, string? ProcessName, DateTime? ProcessCreatedUtc, long? QpcFrequency,
    long? StartQpc, SessionCoverage Coverage, string? EtlPath = null, int? LauncherProcessId = null, string? LauncherName = null,
    string? MarkerConfig = null);

// For a recorded trace: when the target process was created, on the events' clock, and its executable name without .exe,
// from the kernel's process start event, and the process that created it, its launcher, which the event names.
// LauncherName is null when the trace doesn't have it.
public sealed record TraceProcessStart(long Qpc, string? Name, int LauncherProcessId, string? LauncherName);

// CompleteFromSeconds is set once older events were dropped to stay within the limit: the file holds every event
// from that many seconds after the recording started.
public sealed record RecordingStatus(string Path, long StoredEvents, long FileBytes, long LimitBytes, double? CompleteFromSeconds);

// Writes a session's events to its SQLite file from a writer thread, one transaction per batch, with the object index
// (ObjectIndex) and the frame index (FrameIndex) in the same transactions. Process waits while the writer is
// behind, so the ETW reader's queue stays the only place events are dropped. An event of another process than the
// target's is the launcher's: it is stored as one, and not indexed.
public sealed class SessionWriter : IEventProcessor, IDisposable
{
    public const long DefaultLimitBytes = 500L * 1024 * 1024;
    private const int BatchSize = 2_000;
    private static readonly TimeSpan BatchInterval = TimeSpan.FromMilliseconds(250);

    // Trimming deletes the oldest 5% of the stored events. It starts below the limit because the file keeps the freed
    // pages for new events, so the file itself stays within the limit.
    private const double TrimAt = 0.9;
    private const int TrimFraction = 20;

    // Space an event takes besides its payload: its other columns, B-tree cell overhead, and its rows in the frame and
    // object indexes.
    private const int RowOverheadBytes = 100;

    private readonly BlockingCollection<(long Sequence, ProfilerEvent Event)> queue = new(16_384);
    private readonly SqliteConnection connection;
    private readonly SqliteCommand insertEvent;
    private readonly SqliteCommand insertType;
    private readonly ObjectIndex objects;
    private readonly FrameIndex frames;
    private readonly int processId;
    // Event types written so far, by provider (and marker provider), kind (TraceLogging or manifest), name, opcode and
    // whether the launcher wrote them. Usually one layout each; a field that is sometimes null gives a second.
    private readonly Dictionary<(XamlProvider, Guid, bool, string, int, bool), List<EventLayout>> types = [];
    private long typeCount;
    private readonly ArrayBufferWriter<byte> payload = new();
    private readonly Utf8JsonWriter json;
    private readonly Thread writer;
    private readonly long pageSize;
    private readonly bool derivesFrequency;
    private (long Qpc, double Milliseconds)? firstEventTime;
    private long qpcFrequency;
    private long startQpc;
    private long oldestSequence;
    private long newestSequence;
    private long fileBytes;
    private long completeFromQpc;
    // A process start SetProcessStart gave that is not stored yet.
    private TraceProcessStart? pendingProcessStart;
    private volatile Exception? failure;
    private int closed;

    private SessionWriter(string path, SqliteConnection connection, SessionStart start, long limitBytes)
    {
        Path = path;
        LimitBytes = limitBytes;
        this.connection = connection;
        processId = start.ProcessId;
        derivesFrequency = start.QpcFrequency is null;
        qpcFrequency = start.QpcFrequency ?? 0;
        startQpc = start.StartQpc ?? 0;
        pageSize = Sql.Scalar(connection, "PRAGMA page_size");
        fileBytes = Sql.Scalar(connection, "PRAGMA page_count") * pageSize;
        insertEvent = Sql.Prepare(connection,
            "INSERT INTO Events (Seq, Qpc, ThreadId, TypeId, Payload) VALUES ($seq, $qpc, $thread, $type, $payload)",
            ("$seq", SqliteType.Integer), ("$qpc", SqliteType.Integer), ("$thread", SqliteType.Integer), ("$type", SqliteType.Integer),
            ("$payload", SqliteType.Text));
        insertType = Sql.Prepare(connection, """
            INSERT INTO EventTypes (TypeId, Provider, Name, Opcode, FieldNames, FieldTypes, TraceLogging, ProviderName, ProviderId,
                Launcher)
            VALUES ($id, $provider, $name, $opcode, $names, $types, $tracelogging, $providerName, $providerId, $launcher)
            """, ("$id", SqliteType.Integer), ("$provider", SqliteType.Text), ("$name", SqliteType.Text), ("$opcode", SqliteType.Integer),
            ("$names", SqliteType.Text), ("$types", SqliteType.Text), ("$tracelogging", SqliteType.Integer),
            ("$providerName", SqliteType.Text), ("$providerId", SqliteType.Text), ("$launcher", SqliteType.Integer));

        // The payload is never embedded in HTML, so only the escaping JSON requires is applied and type names stay readable.
        json = new(payload, new JsonWriterOptions { Encoder = JavaScriptEncoder.UnsafeRelaxedJsonEscaping });
        objects = new(connection);
        frames = new(connection);
        writer = new(Run) { IsBackground = true, Name = "WinUIProfiler session writer" };
        writer.Start();
    }

    public string Path { get; }

    public long LimitBytes { get; }

    // Set when writing failed; events up to the last committed batch stay in the file.
    public Exception? Failure => failure;

    public RecordingStatus Status
    {
        get
        {
            var newest = Interlocked.Read(ref newestSequence);
            var stored = newest == 0 ? 0 : newest - Interlocked.Read(ref oldestSequence) + 1;
            var completeFrom = Interlocked.Read(ref completeFromQpc);
            var frequency = Interlocked.Read(ref qpcFrequency);
            double? completeFromSeconds = completeFrom != 0 && frequency != 0
                ? (completeFrom - Interlocked.Read(ref startQpc)) / (double)frequency : null;
            return new(Path, stored, Interlocked.Read(ref fileBytes), LimitBytes, completeFromSeconds);
        }
    }

    // Creates a new session file; an existing file is never overwritten.
    public static SessionWriter Create(string path, SessionStart start, long limitBytes = DefaultLimitBytes)
    {
        ArgumentException.ThrowIfNullOrEmpty(path);
        ArgumentNullException.ThrowIfNull(start);
        ArgumentOutOfRangeException.ThrowIfLessThan(limitBytes, 1024 * 1024);
        path = System.IO.Path.GetFullPath(path);
        if (File.Exists(path))
        {
            throw new IOException($"The session file already exists: {path}");
        }

        Directory.CreateDirectory(System.IO.Path.GetDirectoryName(path)!);
        var connection = new SqliteConnection(new SqliteConnectionStringBuilder
        {
            DataSource = path,
            Mode = SqliteOpenMode.ReadWriteCreate,
            Pooling = false,
            // The writer keeps every reference valid; the bundled SQLite would otherwise check each one on insert and delete.
            ForeignKeys = false
        }.ToString());
        try
        {
            connection.Open();
            // WAL lets readers open the file while it is written, and a crash leaves it consistent up to the last
            // batch. NORMAL skips the flush on every commit; the file is still never corrupted.
            Sql.Execute(connection, "PRAGMA journal_mode = WAL; PRAGMA synchronous = NORMAL;");
            using (var transaction = connection.BeginTransaction())
            {
                Sql.Execute(connection, SessionSchema.Create, transaction);
                Sql.Execute(connection, $"PRAGMA user_version = {SessionSchema.Version}", transaction);
                // Read together, so a UTC time maps onto this machine's QPC clock. A trace was recorded earlier, maybe on
                // another machine, so its events are on a different clock and StartedQpc stays empty for it.
                var startedQpc = Stopwatch.GetTimestamp();
                var startedUtc = DateTime.UtcNow;
                Sql.Execute(connection, """
                    INSERT INTO SessionInfo (SessionId, ProcessId, ProcessName, ProcessCreatedUtc, QpcFrequency, StartQpc,
                        StartedUtc, StartedQpc, Source, EtlPath, Coverage, LimitBytes, LauncherProcessId, LauncherName,
                        MarkerConfig)
                    VALUES ($id, $pid, $name, $created, $frequency, $start, $started, $startedQpc, $source, $etl, $coverage, $limit,
                        $launcher, $launcherName, $markers)
                    """, transaction,
                    ("$id", Guid.NewGuid().ToString()), ("$pid", start.ProcessId), ("$name", start.ProcessName),
                    ("$created", start.ProcessCreatedUtc?.ToString("O", CultureInfo.InvariantCulture)),
                    ("$frequency", start.QpcFrequency), ("$start", start.StartQpc),
                    ("$started", startedUtc.ToString("O", CultureInfo.InvariantCulture)),
                    ("$startedQpc", start.EtlPath is null ? startedQpc : null),
                    ("$source", start.EtlPath is null ? "Live" : "Trace"), ("$etl", start.EtlPath),
                    ("$coverage", start.Coverage.ToString()), ("$limit", limitBytes),
                    ("$launcher", start.LauncherProcessId), ("$launcherName", start.LauncherName),
                    ("$markers", start.MarkerConfig));
                transaction.Commit();
            }

            return new(path, connection, start, limitBytes);
        }
        catch
        {
            connection.Dispose();
            // Best effort: the original error matters more than a leftover partial file.
            foreach (var file in new[] { path, path + "-wal", path + "-shm" })
            {
                try
                {
                    File.Delete(file);
                }
                catch (Exception error) when (error is IOException or UnauthorizedAccessException)
                {
                    Trace.TraceWarning($"Could not delete the partial session file {file}: {error.Message}");
                }
            }

            throw;
        }
    }

    // Waits while the writer's queue is full. After a failure, events are no longer stored.
    public void Process(long sequence, ProfilerEvent profilerEvent)
    {
        if (failure is null)
        {
            queue.Add((sequence, profilerEvent));
        }
    }

    // For a recorded trace: when the process was created and by which process, from the kernel's process start event.
    // Stored with the next batch of events, or when the file is closed; a later call replaces it. The process's name is
    // stored only when the session has none.
    public void SetProcessStart(TraceProcessStart processStart)
    {
        ArgumentNullException.ThrowIfNull(processStart);
        ArgumentOutOfRangeException.ThrowIfNegativeOrZero(processStart.Qpc);
        Interlocked.Exchange(ref pendingProcessStart, processStart);
    }

    private void Run()
    {
        var batch = new List<(long Sequence, ProfilerEvent Event)>(BatchSize);
        try
        {
            while (queue.TryTake(out var first, Timeout.Infinite))
            {
                batch.Add(first);
                var started = Stopwatch.GetTimestamp();
                while (batch.Count < BatchSize)
                {
                    var remaining = BatchInterval - Stopwatch.GetElapsedTime(started);
                    if (remaining <= TimeSpan.Zero || !queue.TryTake(out var next, remaining))
                    {
                        break;
                    }

                    batch.Add(next);
                }

                Write(batch);
                batch.Clear();
            }
        }
        catch (Exception error)
        {
            Trace.TraceError($"Session recording stopped: {error}");
            failure = error;
            // Keep taking so a producer waiting in Process is released; nothing more is stored.
            foreach (var _ in queue.GetConsumingEnumerable())
            {
            }
        }
    }

    private void Write(List<(long Sequence, ProfilerEvent Event)> batch)
    {
        // Encode first, so room for the whole batch is made before it is written and the file stays within the limit.
        var payloads = new string[batch.Count];
        long incomingBytes = 0;
        for (var i = 0; i < batch.Count; i++)
        {
            json.Reset();
            payload.ResetWrittenCount();
            EventPayload.Write(json, batch[i].Event.Fields);
            json.Flush();
            payloads[i] = Encoding.UTF8.GetString(payload.WrittenSpan);
            incomingBytes += payload.WrittenCount + RowOverheadBytes;
        }

        TrimIfNeeded(incomingBytes);
        using (var transaction = connection.BeginTransaction())
        {
            insertEvent.Transaction = transaction;
            insertType.Transaction = transaction;
            for (var i = 0; i < batch.Count; i++)
            {
                var (sequence, profilerEvent) = batch[i];
                var launcher = profilerEvent.ProcessId != processId;
                var typeId = TypeId(profilerEvent, launcher);
                insertEvent.Parameters["$seq"].Value = sequence;
                insertEvent.Parameters["$qpc"].Value = profilerEvent.TimestampQpc;
                insertEvent.Parameters["$thread"].Value = profilerEvent.ThreadId;
                insertEvent.Parameters["$type"].Value = typeId;
                insertEvent.Parameters["$payload"].Value = payloads[i];
                insertEvent.ExecuteNonQuery();
                // The indexes describe the target's objects and UI threads.
                if (!launcher)
                {
                    objects.Add(sequence, typeId, profilerEvent, transaction);
                    frames.Add(sequence, typeId, profilerEvent, transaction);
                }
            }

            UpdateTimeBase(batch, transaction);
            transaction.Commit();
        }

        if (Interlocked.Read(ref oldestSequence) == 0)
        {
            Interlocked.Exchange(ref oldestSequence, batch[0].Sequence);
        }

        Interlocked.Exchange(ref newestSequence, batch[^1].Sequence);
        Interlocked.Exchange(ref fileBytes, Sql.Scalar(connection, "PRAGMA page_count") * pageSize);
    }

    private long TypeId(ProfilerEvent profilerEvent, bool launcher)
    {
        var fields = profilerEvent.Fields;
        var key = (profilerEvent.Provider, profilerEvent.Marker?.Id ?? Guid.Empty, profilerEvent.TraceLogging, profilerEvent.Name,
            profilerEvent.Opcode, launcher);
        if (!types.TryGetValue(key, out var layouts))
        {
            types[key] = layouts = [];
        }

        foreach (var known in layouts)
        {
            if (known.Matches(fields))
            {
                return known.Id;
            }
        }

        var layout = new EventLayout(typeCount + 1, fields.Select(field => field.Name).ToArray(),
            fields.Select(field => EventPayload.TypeOf(field.Value)).ToArray());
        insertType.Parameters["$id"].Value = layout.Id;
        insertType.Parameters["$provider"].Value = profilerEvent.Provider.ToString();
        insertType.Parameters["$name"].Value = profilerEvent.Name;
        insertType.Parameters["$opcode"].Value = profilerEvent.Opcode;
        insertType.Parameters["$names"].Value = string.Join(',', layout.Names);
        insertType.Parameters["$types"].Value = string.Join(',', layout.Types);
        insertType.Parameters["$tracelogging"].Value = profilerEvent.TraceLogging;
        insertType.Parameters["$providerName"].Value = profilerEvent.ProviderName;
        insertType.Parameters["$providerId"].Value = profilerEvent.ProviderId.ToString();
        insertType.Parameters["$launcher"].Value = launcher;
        insertType.ExecuteNonQuery();
        layouts.Add(layout);
        typeCount = layout.Id;
        return layout.Id;
    }

    private sealed record EventLayout(long Id, string[] Names, FieldType[] Types)
    {
        public bool Matches(IReadOnlyList<EventField> fields)
        {
            if (fields.Count != Names.Length)
            {
                return false;
            }

            for (var i = 0; i < Names.Length; i++)
            {
                if (EventPayload.TypeOf(fields[i].Value) != Types[i] || fields[i].Name != Names[i])
                {
                    return false;
                }
            }

            return true;
        }
    }

    // A recorded trace gives no start: time zero is the target's first event, so the launcher's events before it have
    // negative times. Its frequency is normally given; if not, it comes from the events' QPC timestamps and their
    // milliseconds since the trace started, measured over at least a second.
    private void UpdateTimeBase(List<(long Sequence, ProfilerEvent Event)> batch, SqliteTransaction transaction)
    {
        var changed = false;
        if (Interlocked.Read(ref startQpc) == 0 &&
            batch.Find(item => item.Event.ProcessId == processId).Event is { } targetFirst)
        {
            Interlocked.Exchange(ref startQpc, targetFirst.TimestampQpc);
            changed = true;
        }

        if (derivesFrequency && Interlocked.Read(ref qpcFrequency) == 0)
        {
            var (first, last) = (batch[0].Event, batch[^1].Event);
            firstEventTime ??= (first.TimestampQpc, first.RelativeMilliseconds);
            var milliseconds = last.RelativeMilliseconds - firstEventTime.Value.Milliseconds;
            if (milliseconds >= 1000)
            {
                Interlocked.Exchange(ref qpcFrequency,
                    (long)Math.Round((last.TimestampQpc - firstEventTime.Value.Qpc) * 1000.0 / milliseconds));
                changed = true;
            }
        }

        if (changed)
        {
            Sql.Execute(connection, "UPDATE SessionInfo SET StartQpc = $start, QpcFrequency = $frequency", transaction,
                ("$start", Interlocked.Read(ref startQpc) is var start and not 0 ? start : (long?)null),
                ("$frequency", qpcFrequency == 0 ? null : qpcFrequency));
        }

        StoreProcessStart(transaction);
    }

    private void StoreProcessStart(SqliteTransaction? transaction)
    {
        if (Interlocked.Exchange(ref pendingProcessStart, null) is { } processStart)
        {
            Sql.Execute(connection, """
                UPDATE SessionInfo SET ProcessStartQpc = $processStart, ProcessName = COALESCE(ProcessName, $name),
                    LauncherProcessId = $launcher, LauncherName = $launcherName
                """, transaction,
                ("$processStart", processStart.Qpc), ("$name", processStart.Name),
                ("$launcher", processStart.LauncherProcessId), ("$launcherName", processStart.LauncherName));
        }
    }

    // Deletes the oldest events until the stored data plus the incoming batch fits under the trim point.
    private void TrimIfNeeded(long incomingBytes)
    {
        while ((Sql.Scalar(connection, "PRAGMA page_count") - Sql.Scalar(connection, "PRAGMA freelist_count")) * pageSize +
            incomingBytes > LimitBytes * TrimAt)
        {
            var oldest = Interlocked.Read(ref oldestSequence);
            var newest = Interlocked.Read(ref newestSequence);
            if (oldest == 0 || newest <= oldest)
            {
                return;
            }

            var cutoff = oldest + Math.Max(1, (newest - oldest + 1) / TrimFraction);
            long qpc;
            using (var transaction = connection.BeginTransaction())
            {
                objects.Trim(cutoff, transaction);
                frames.Trim(cutoff, transaction);
                Sql.Execute(connection, "DELETE FROM Events WHERE Seq < $cutoff", transaction, ("$cutoff", cutoff));
                // The launcher's events can be stored after later events of the process (LaunchWatcher), so the time is the
                // process's first event from the cutoff, or else the cutoff's.
                qpc = Sql.Scalar(connection, """
                    SELECT COALESCE(
                        (SELECT Qpc FROM Events WHERE Seq >= $cutoff
                            AND TypeId IN (SELECT TypeId FROM EventTypes WHERE Launcher = 0) ORDER BY Seq LIMIT 1),
                        (SELECT Qpc FROM Events WHERE Seq = $cutoff))
                    """, transaction, ("$cutoff", cutoff));
                Sql.Execute(connection, "UPDATE SessionInfo SET CompleteFromSeq = $cutoff, CompleteFromQpc = $qpc", transaction,
                    ("$cutoff", cutoff), ("$qpc", qpc));
                transaction.Commit();
            }

            Interlocked.Exchange(ref oldestSequence, cutoff);
            Interlocked.Exchange(ref completeFromQpc, qpc);
        }
    }

    // Stores the remaining events, records how the session ended and closes the file. Call after the last Process call.
    public void Close(string endReason, int? lostByEtw = null, long? droppedByProfiler = null)
    {
        if (Interlocked.Exchange(ref closed, 1) != 0)
        {
            return;
        }

        queue.CompleteAdding();
        writer.Join();
        try
        {
            if (failure is null)
            {
                StoreProcessStart(null);
                Sql.Execute(connection,
                    "UPDATE SessionInfo SET EndedUtc = $ended, EndReason = $reason, LostByEtw = $lost, DroppedByProfiler = $dropped",
                    null, ("$ended", DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture)), ("$reason", endReason),
                    ("$lost", lostByEtw), ("$dropped", droppedByProfiler));
                // Moves the write-ahead log into the database and leaves WAL mode, so the .sqlite file stands alone and
                // can be opened read-only without its -wal and -shm files.
                Sql.Execute(connection, "PRAGMA wal_checkpoint(TRUNCATE); PRAGMA journal_mode = DELETE;");
                Interlocked.Exchange(ref fileBytes, Sql.Scalar(connection, "PRAGMA page_count") * pageSize);
            }
        }
        catch (Exception error) when (error is SqliteException or IOException)
        {
            Trace.TraceError($"Closing the session file failed: {error}");
            failure = error;
        }
        finally
        {
            json.Dispose();
            objects.Dispose();
            frames.Dispose();
            insertEvent.Dispose();
            insertType.Dispose();
            connection.Dispose();
            queue.Dispose();
        }
    }

    public void Dispose() => Close("Closed");
}

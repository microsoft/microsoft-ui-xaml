// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Concurrent;
using System.Globalization;
using System.Text.Json;
using System.Text.RegularExpressions;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Sessions;
using WinUIProfiler.Mcp.Tools;
using static WinUIProfiler.Mcp.SelfTest.SessionFixtures;
using static WinUIProfiler.Mcp.SelfTest.ToolResults;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>
/// The SQL behind the answers: SessionFile reports each statement its reads run, with the values written in, and each
/// statement, run as reported, returns the rows reported. Each tool result lists the statements its call ran, and says what
/// the call computed from their rows.
/// </summary>
internal static class QuerySelfTests
{
    public static IEnumerable<SelfTestCase> Tests =>
    [
        new("queries.values", _ => Values()),
        new("queries.parameters", _ => Parameters()),
        new("queries.observers", Observers),
        new("queries.reads", Reads),
        new("queries.filters", Filters),
        new("queries.views", Views),
        new("queries.isolation", Isolation),
        new("queries.log", Log),
        new("queries.log.cleanup", LogCleanup),
        new("queries.log.error", LogError),
    ];

    /// <summary>
    /// Checks a tool result's computed text and the statements it lists, and returns them. Each statement is listed once,
    /// ends with a semicolon and isn't a check of the file's version or views. Run as listed on the files it ran on (the
    /// result's session file, or each session file it lists that could be read, unless the statement names others), it
    /// returns the rows listed.
    /// </summary>
    internal static IReadOnlyList<JsonElement> CheckQueries(JsonElement result, string what)
    {
        Expect.True(!string.IsNullOrWhiteSpace(result.Text("computed")), $"{what} says what it computed.");
        var session = result.Property("session").Text("sessionPath");
        List<string> given = session is not null ? [session]
            : result.IsNull("sessions") ? []
            : [.. result.Items("sessions").Where(row => row.IsNull("error")).Select(row => row.Text("path") ?? "")];
        var queries = result.Items("queries");
        var listed = new HashSet<string>(StringComparer.Ordinal);
        foreach (var query in queries)
        {
            var sql = query.Text("sql") ?? "";
            Expect.True(listed.Add(sql), $"{what} lists each statement once: {sql}");
            Expect.True(sql.EndsWith(';') && !sql.Contains('\r', StringComparison.Ordinal),
                $"{what}: a statement ends with a semicolon, and its line breaks are \\n: {sql}");
            Expect.True(!sql.Contains("PRAGMA", StringComparison.OrdinalIgnoreCase)
                && !sql.Contains("sqlite_master", StringComparison.OrdinalIgnoreCase),
                $"{what} doesn't list the checks of the file's version and views.");
            var runs = query.IsNull("runs") ? 1 : query.Long("runs");
            Expect.True(query.IsNull("runs") || runs > 1, $"{what} gives runs only for a statement that ran more than once: {sql}");
            List<string> files = query.IsNull("files") ? given : [.. query.Strings("files").Select(file => file ?? "")];
            Expect.True(files.Count > 0 && files.All(file => file.Length > 0), $"{what} names the files {sql} ran on.");

            // Each file once, or the same rows each time.
            var counts = files.Select(file => (long)Rerun(new SessionQuery(file, sql, 0)).Count).ToList();
            var rows = counts.Count == runs ? counts.Sum()
                : counts.Distinct().Count() == 1 ? runs * counts[0]
                : throw new SelfTestFailedException($"{what}: the rows of {runs} runs of {sql} on {files.Count} files can't be told.");
            Expect.Equal(rows, query.Long("rows"), $"{what}: the rows of {sql}, run as listed");
        }

        return queries;
    }

    /// <summary>Runs a reported statement as it is, on a read-only connection of its own, and returns its rows.</summary>
    internal static IReadOnlyList<object?[]> Rerun(SessionQuery query)
    {
        using var connection = new SqliteConnection(new SqliteConnectionStringBuilder
        {
            DataSource = query.Path,
            Mode = SqliteOpenMode.ReadOnly,
            Pooling = false,
        }.ToString());
        connection.Open();
        using var command = connection.CreateCommand();
        command.CommandText = query.Sql;

        // Microsoft.Data.Sqlite fails a statement with a parameter it has no value for, so this also checks every value
        // was written in.
        using var reader = command.ExecuteReader();
        var rows = new List<object?[]>();
        while (reader.Read())
        {
            var row = new object?[reader.FieldCount];
            for (var column = 0; column < row.Length; column++)
            {
                row[column] = reader.IsDBNull(column) ? null : reader.GetValue(column);
            }

            rows.Add(row);
        }

        return rows;
    }

    /// <summary>Checks that each statement, run as reported, returns the rows reported.</summary>
    internal static void RunAsReported(IEnumerable<SessionQuery> queries, string what)
    {
        foreach (var query in queries)
        {
            Expect.Equal(query.Rows, (long)Rerun(query).Count, $"{what}: the rows of {query.Sql}");
            Expect.True(!query.Sql.Contains('\r', StringComparison.Ordinal), $"{what}: the line breaks are \\n in {query.Sql}");
        }
    }

    // Each value, written into SQL, is the same value of the same type as when it's bound.
    private static void Values()
    {
        object[] values =
        [
            DBNull.Value, true, false, (sbyte)-5, (byte)200, (short)-30000, (ushort)60000, 42, -42, uint.MaxValue, 0L, long.MaxValue,
            long.MinValue, ulong.MaxValue, 1.0, -0.5, 0.1, 1e20, -2.5e-300, double.MaxValue, double.Epsilon, 1.5f, 0.1f, "", "plain",
            "it's", "''", @"C:\path\", "50%_off", "line\r\nbreak", "a\0b", "\u00e9\u4e2d\U0001F600", Array.Empty<byte>(),
            new byte[] { 0, 1, 0xAB, 0xFF },
        ];
        using var connection = new SqliteConnection("Data Source=:memory:");
        connection.Open();
        foreach (var value in values)
        {
            using var bound = connection.CreateCommand();
            bound.CommandText = "SELECT typeof($v), hex($v), $v";
            bound.Parameters.AddWithValue("$v", value);
            using var written = connection.CreateCommand();
            written.CommandText = SessionQueries.Inline(bound);
            Expect.Equal(Row(bound), Row(written), $"{Describe(value)}, written as {written.CommandText}");
        }

        foreach (var value in new object[] { 1.5m, 'c', double.NaN, double.PositiveInfinity, float.NegativeInfinity, Guid.Empty })
        {
            using var command = connection.CreateCommand();
            command.CommandText = "SELECT $v";
            command.Parameters.AddWithValue("$v", value);
            Expect.True(Throws<NotSupportedException>(() => SessionQueries.Inline(command)),
                $"{Describe(value)} isn't written into SQL.");
        }

        // The type, the bytes and the value as .NET reads it, exactly for a REAL.
        static string Row(SqliteCommand command)
        {
            using var reader = command.ExecuteReader();
            Expect.True(reader.Read(), $"{command.CommandText} returns a row.");
            var value = reader.IsDBNull(2) ? "NULL" : reader.GetValue(2) switch
            {
                double number => number.ToString("R", CultureInfo.InvariantCulture),
                byte[] bytes => Convert.ToHexString(bytes),
                var other => Convert.ToString(other, CultureInfo.InvariantCulture),
            };
            return $"{reader.GetString(0)} {reader.GetString(1)} {value}";
        }
    }

    // Where a parameter is replaced, and what's left as it is.
    private static void Parameters()
    {
        (string Sql, string Expected)[] cases =
        [
            ("SELECT $a, $a", "SELECT 7, 7"),
            ("SELECT :a, @a, $A, $b, $a1, ?, ?1", "SELECT :a, @a, $A, $b, $a1, ?, ?1"),
            ("SELECT '$a', \"$a\", [$a], `$a`", "SELECT '$a', \"$a\", [$a], `$a`"),
            ("SELECT 'it''s $a', \"a\"\"$a\", $a", "SELECT 'it''s $a', \"a\"\"$a\", 7"),
            ("SELECT a$a, x$a", "SELECT a$a, x$a"),
            ("SELECT $a -- $a\r\nFROM t", "SELECT 7 -- $a\nFROM t"),
            ("SELECT $a -- $a", "SELECT 7 -- $a"),
            ("SELECT /* $a\r\n */ $a", "SELECT /* $a\n */ 7"),
            ("SELECT /* $a", "SELECT /* $a"),
            ("SELECT 'a\r\nb',\r\n$a", "SELECT 'a\r\nb',\n7"),
            ("SELECT '$a", "SELECT '$a"),
            ("SELECT 1 -$n, 1 - $n, 2-$a, 4 - 2 / 1", "SELECT 1 - -5, 1 - -5, 2-7, 4 - 2 / 1"),
            ("SELECT :u, @u, $u", "SELECT 3, 3, 3"),
        ];
        using var command = new SqliteCommand();
        command.Parameters.AddWithValue("$a", 7);
        command.Parameters.AddWithValue("$n", -5);
        command.Parameters.AddWithValue("u", 3);
        foreach (var (sql, expected) in cases)
        {
            command.CommandText = sql;
            Expect.Equal(expected, SessionQueries.Inline(command), $"{sql} with its values written in");
        }
    }

    private static void Observers(SelfTestContext context)
    {
        var first = WriteMain(context, "first.sqlite");
        var second = WriteMain(context, "second.sqlite");
        SessionFile.ReadInfo(first);

        var outer = new List<SessionQuery>();
        var inner = new List<SessionQuery>();
        using (SessionQueries.Observe(outer.Add))
        {
            SessionFile.ReadInfo(first);
            Expect.Equal(1, outer.Count, "Statements a read runs inside an observer");
            Expect.True(string.Equals(Path.GetFullPath(first), outer[0].Path, StringComparison.OrdinalIgnoreCase),
                $"A statement names the file it read: {outer[0].Path}");
            Expect.Equal(1L, outer[0].Rows, "The rows the session record's statement read");

            var observed = SessionQueries.Observe(inner.Add);
            SessionFile.ReadFrames(first);
            Expect.Equal((2, 1), (outer.Count, inner.Count), "Statements seen by an observer, and by one observed inside it");
            Expect.Equal(outer[^1], inner[0], "The statement both saw");
            observed.Dispose();
            observed.Dispose();
            SessionFile.ReadFrames(first);
            Expect.Equal((3, 1), (outer.Count, inner.Count), "Statements seen once the inner observer is disposed");
        }

        SessionFile.ReadInfo(first);
        Expect.Equal(3, outer.Count, "Statements seen once both observers are disposed");

        // Disposed out of order, the outer first.
        var early = new List<SessionQuery>();
        var late = new List<SessionQuery>();
        var earlyScope = SessionQueries.Observe(early.Add);
        var lateScope = SessionQueries.Observe(late.Add);
        earlyScope.Dispose();
        SessionFile.ReadInfo(first);
        Expect.Equal((0, 1), (early.Count, late.Count), "Statements seen once the outer observer is disposed first");
        lateScope.Dispose();
        SessionFile.ReadInfo(first);
        Expect.Equal((0, 1), (early.Count, late.Count), "Statements seen once both are disposed");

        var flowed = new ConcurrentQueue<SessionQuery>();
        using (SessionQueries.Observe(flowed.Enqueue))
        {
            Task.Run(() => SessionFile.ReadInfo(first)).GetAwaiter().GetResult();
            var thread = new Thread(() => SessionFile.ReadInfo(first));
            thread.Start();
            thread.Join();
            Expect.Equal(2, flowed.Count, "Statements of a task and a thread started inside an observer");
        }

        var leaked = new List<SessionQuery>();
        Task.Run(() => SessionQueries.Observe(leaked.Add)).GetAwaiter().GetResult();
        SessionFile.ReadInfo(first);
        Expect.Equal(0, leaked.Count, "Statements an observer set in a task sees of its caller");

        // Two flows reading at once each see only their own statements.
        var seen = new[] { new ConcurrentQueue<SessionQuery>(), new ConcurrentQueue<SessionQuery>() };
        string[] paths = [first, second];
        using var barrier = new Barrier(2);
        var threads = Enumerable.Range(0, 2).Select(index => new Thread(() =>
        {
            using (SessionQueries.Observe(seen[index].Enqueue))
            {
                barrier.SignalAndWait();
                for (var round = 0; round < 20; round++)
                {
                    SessionFile.ReadInfo(paths[index]);
                    SessionFile.ReadFrames(paths[index]);
                }
            }
        })).ToList();
        threads.ForEach(thread => thread.Start());
        threads.ForEach(thread => thread.Join());
        for (var index = 0; index < 2; index++)
        {
            var path = Path.GetFullPath(paths[index]);
            Expect.Equal(40, seen[index].Count, $"Statements seen by the flow that read {path}");
            Expect.True(seen[index].All(query => string.Equals(query.Path, path, StringComparison.OrdinalIgnoreCase)),
                $"The flow that read {path} saw only its statements.");
        }
    }

    // Every read of SessionFile reports each statement it runs, and each returns the rows reported when it's run as is.
    private static void Reads(SelfTestContext context)
    {
        var path = WriteMain(context);
        var root = SessionFile.ReadObjects(path).First(item => item.Name == "Root");
        var tick = SessionFile.ReadFrames(path)[0].FrameId;
        var region = SessionFile.ReadInterestingRows(path).FirstOrDefault(row => row.RegionId is not null)?.RegionId ?? 1;
        (string Read, int Statements, Action Run)[] reads =
        [
            ("ReadInfo", 1, () => SessionFile.ReadInfo(path)),
            ("Read", 3, () => SessionFile.Read(path, _ => { })),
            ("ReadObjects", 2, () => SessionFile.ReadObjects(path)),
            ("ReadObjects of a kind", 2, () => SessionFile.ReadObjects(path, ObjectKind.Visual)),
            ("ReadObjects by ID", 2, () => SessionFile.ReadObjects(path, objectId: root.ObjectId)),
            ("ReadObjects by address", 2, () => SessionFile.ReadObjects(path, address: 0x2000)),
            ("ReadTimeline", 3, () => SessionFile.ReadTimeline(path, root.ObjectId, 0, -1, (_, _, _) => { })),
            ("ReadTimeline's second page", 3, () => SessionFile.ReadTimeline(path, root.ObjectId, 1, 1, (_, _, _) => { })),
            ("CountTimeline", 1, () => SessionFile.CountTimeline(path, root.ObjectId)),
            ("FindSnapshotElement", 3, () => Expect.True(SessionFile.FindSnapshotElement(path, [0x7100], long.MaxValue) is not null,
                "The snapshot's element is found.")),
            ("FindSnapshotVisual", 1, () => Expect.True(SessionFile.FindSnapshotVisual(path, 0xB000, long.MaxValue) is not null,
                "The snapshot's visual is found.")),
            ("ReadFrames", 1, () => SessionFile.ReadFrames(path)),
            ("ReadFrames of a tick", 1, () => SessionFile.ReadFrames(path, tick)),
            ("ReadFrameList", 1, () => SessionFile.ReadFrameList(path)),
            ("ReadInterestingRows", 1, () => SessionFile.ReadInterestingRows(path)),
            ("ReadInterestingRows of a region", 1, () => SessionFile.ReadInterestingRows(path, region)),
            ("ReadLaunchSummary", 1, () => SessionFile.ReadLaunchSummary(path)),
            ("ReadFrameEvents", 3, () => SessionFile.ReadFrameEvents(path, tick, null, null, 0, -1, (_, _, _) => { })),
            ("ReadFrameEvents of a bucket", 3, () => SessionFile.ReadFrameEvents(path, tick, "Xaml", "frame", 1, 2, (_, _, _) => { })),
            ("CountFrameEvents", 1, () => SessionFile.CountFrameEvents(path, tick, null, null)),
            ("ReadSpanEvents", 3, () => SessionFile.ReadSpanEvents(path, UiThread, 1, 16, null, null, 0, -1, (_, _, _) => { })),
            ("CountSpanEvents", 1, () => SessionFile.CountSpanEvents(path, UiThread, 1, 16, "Other", "0x1000")),
        ];
        foreach (var (read, statements, run) in reads)
        {
            var seen = new List<SessionQuery>();
            using (SessionQueries.Observe(seen.Add))
            {
                run();
            }

            Expect.Equal(statements, seen.Count, $"Statements {read} reported");
            Expect.True(seen.All(query => string.Equals(query.Path, Path.GetFullPath(path), StringComparison.OrdinalIgnoreCase)),
                $"{read}'s statements name the file it read.");
            Expect.True(!seen.Any(query => query.Sql.Contains("PRAGMA", StringComparison.OrdinalIgnoreCase)
                || query.Sql.Contains("sqlite_master", StringComparison.OrdinalIgnoreCase)),
                $"{read} doesn't report the checks of the file's version and views.");
            RunAsReported(seen, read);
        }

        // A count's statement computes the count the read returned.
        var counted = new List<SessionQuery>();
        long count;
        using (SessionQueries.Observe(counted.Add))
        {
            count = SessionFile.CountFrameEvents(path, tick, null, null);
        }

        Expect.Equal(count, (long)Rerun(counted[0])[0][0]!, "The count CountFrameEvents' statement computes");

        // A read that fails reports the statements it finished, not the one that failed.
        var failed = new List<SessionQuery>();
        using (SessionQueries.Observe(failed.Add))
        {
            Expect.True(Throws<OperationCanceledException>(() => SessionFile.Read(path, _ => { }, new CancellationToken(true))),
                "A cancelled read is cancelled.");
            Expect.Equal(2, failed.Count, "Statements a cancelled read reported");
            Expect.True(Throws<InvalidDataException>(() => SessionFile.ReadInfo(
                WriteForeign(Path.Combine(context.TempDirectory, "foreign.sqlite")))), "A file that isn't a session file isn't read.");
            Expect.Equal(2, failed.Count, "Statements reported for a file that isn't a session file");
        }
    }

    // Filters' text and numbers, written into the statements, keep the events they keep when bound.
    private static void Filters(SelfTestContext context)
    {
        var path = Write(Path.Combine(context.TempDirectory, "filters.sqlite"),
        [
            FrameStart(1.0, 1),
            Tl(2.0, UiThread, XamlProvider.Xaml, "Note", F("Text", @"it's 50%_off in C:\path"), F("Address", 0x1000L), F("Big", -1L)),
            Tl(3.0, UiThread, XamlProvider.Xaml, "Other", F("Text", "5x0 plain"), F("Count", 7)),
            FrameEnd(4.0, 1),
        ]);
        var tick = SessionFile.ReadFrames(path)[0].FrameId;
        (string? Filter, long Expected)[] filters =
        [
            (null, 4), ("   ", 4), ("it's", 1), ("50%_off", 1), ("5_%", 0), (@"C:\path", 1), (@"\", 1), ("0x1000", 1), ("4096", 1),
            ("0xFFFFFFFFFFFFFFFF", 1), ("-1", 1), ("-9223372036854775808", 0), ("note", 1), ("plain", 1),
        ];
        foreach (var (filter, expected) in filters)
        {
            var seen = new List<SessionQuery>();
            long counted;
            long delivered;
            using (SessionQueries.Observe(seen.Add))
            {
                counted = SessionFile.CountFrameEvents(path, tick, null, filter);
                delivered = SessionFile.ReadFrameEvents(path, tick, null, filter, 0, -1, (_, _, _) => { });
            }

            Expect.Equal(expected, counted, $"Events the filter '{filter}' keeps");
            Expect.Equal(expected, delivered, $"Events read with the filter '{filter}'");
            Expect.Equal(counted, (long)Rerun(seen[0])[0][0]!, $"The count of the filter '{filter}' run as reported");
            RunAsReported(seen, $"The filter '{filter}'");
        }

        var paged = new List<SessionQuery>();
        using (SessionQueries.Observe(paged.Add))
        {
            Expect.Equal(2L, SessionFile.ReadFrameEvents(path, tick, null, null, 1, 2, (_, _, _) => { }), "A page of two events");
        }

        RunAsReported(paged, "A page of two events");
        var quoted = new List<SessionQuery>();
        using (SessionQueries.Observe(quoted.Add))
        {
            SessionFile.CountFrameEvents(path, tick, null, "it's");
        }

        Expect.True(quoted[0].Sql.Contains("'%it''s%'", StringComparison.Ordinal), $"A quote is doubled: {quoted[0].Sql}");
    }

    // Each view of the tools that read a session file says what it computed, and lists the statements its reads ran.
    private static void Views(SelfTestContext context)
    {
        var main = WriteMain(context);
        var regions = Write(Path.Combine(context.TempDirectory, "regions.sqlite"), FrameToolSelfTests.RegionEvents);
        var tick = SessionFile.ReadFrames(main)[0].FrameId;
        var region = Run(() => FrameTools.Frames(sessionPath: regions, view: "regions")).Items("regions")[0].Long("regionId");
        var root = SessionFile.ReadObjects(main).First(item => item.Name == "Root").ObjectId;
        (string View, Func<Task<object>> Call)[] calls =
        [
            ("The frames summary", () => FrameTools.Frames(sessionPath: main)),
            ("The slowest ticks", () => FrameTools.Frames(sessionPath: main, view: "slowest")),
            ("The ticks with the most events", () => FrameTools.Frames(sessionPath: main, view: "slowest", orderBy: "events")),
            ("The regions", () => FrameTools.Frames(sessionPath: regions, view: "regions")),
            ("The regions by time", () => FrameTools.Frames(sessionPath: regions, view: "regions", orderBy: "time")),
            ("The rows", () => FrameTools.Frames(sessionPath: regions, view: "rows")),
            ("A region's rows by duration", () => FrameTools.Frames(sessionPath: regions, regionId: region, orderBy: "duration")),
            ("The rows of a type", () => FrameTools.Frames(sessionPath: regions, type: "UpdateLayout")),
            ("A tick", () => FrameTools.Frames(sessionPath: main, tickId: tick)),
            ("A page of a tick's bucket", () => FrameTools.Frames(sessionPath: main, tickId: tick, bucket: "Xaml", offset: 1, limit: 2)),
            ("The launch milestones", () => LaunchTools.Launch(sessionPath: main)),
            ("The launch frames", () => LaunchTools.Launch(view: "frames", sessionPath: main)),
            ("The object types", () => ObjectTools.Objects(sessionPath: main)),
            ("The visuals' types", () => ObjectTools.Objects(sessionPath: main, kind: "Visual")),
            ("The objects by events", () => ObjectTools.Objects(sessionPath: main, view: "list", orderBy: "events")),
            ("The objects of a type and name", () => ObjectTools.Objects(sessionPath: main, type: "button", name: "ok")),
            ("An object's history", () => ObjectTools.Objects(sessionPath: main, objectId: root)),
            ("An address's history", () => ObjectTools.Objects(sessionPath: main, address: "0x2000")),
            ("The session summary", () => SessionTools.Sessions(sessionPath: main)),
        ];
        foreach (var (view, call) in calls)
        {
            var queries = CheckQueries(Run(call), view);
            Expect.True(queries.Count > 0, $"{view} lists the statements it ran.");
            Expect.True(queries.All(query => query.IsNull("files")), $"{view} ran its statements only on the session file it read.");
        }

        // The tick view reads the session record, then the tick's events, whose read reads it again.
        var first = Run(() => FrameTools.Frames(sessionPath: main, tickId: tick)).Items("queries")[0];
        Expect.True(first.Text("sql")!.Contains("FROM SessionInfo", StringComparison.Ordinal),
            $"The tick view first reads the session record: {first.Text("sql")}");
        Expect.Equal(2L, first.Long("runs"), "Runs of the session record's statement, listed once");
    }

    // Calls that run at the same time each list only their own statements, and a call that fails leaves none to the next.
    private static void Isolation(SelfTestContext context)
    {
        var main = WriteMain(context);
        var other = WriteMain(context, "other.sqlite");
        var tick = SessionFile.ReadFrames(main)[0].FrameId;
        Func<Task<object>>[] calls =
        [
            () => FrameTools.Frames(sessionPath: main, tickId: tick),
            () => ObjectTools.Objects(sessionPath: other),
        ];
        var alone = calls.Select(call => Queries(Run(call))).ToList();

        // Session files' reads wait on each other's locks, so the calls of a round overlap.
        for (var round = 0; round < 3; round++)
        {
            var results = Task.WhenAll(Enumerable.Range(0, 8).Select(index => calls[index % 2]())).GetAwaiter().GetResult();
            for (var index = 0; index < results.Length; index++)
            {
                Expect.Equal(alone[index % 2], Queries(Json(results[index])),
                    $"The statements of call {index} of round {round}, which ran with 7 others");
            }
        }

        var summary = Queries(Run(() => FrameTools.Frames(sessionPath: main)));
        Expect.Equal($"There's no tick 999 in {main}. The slowest, regions and rows views give tick IDs.",
            Error(() => FrameTools.Frames(sessionPath: main, tickId: 999)), "The error for a tick that isn't there");
        Expect.Equal(summary, Queries(Run(() => FrameTools.Frames(sessionPath: main))),
            "The statements of the call after one that failed");

        static string Queries(JsonElement result) => result.Property("queries").GetRawText();
    }

    // The query log: each call's statements, as its result lists them, appended whole to one file that runs as a script.
    private static void Log(SelfTestContext context)
    {
        var time = new DateTimeOffset(2025, 1, 2, 3, 4, 5, 678, TimeSpan.FromHours(5.5));
        Expect.Equal(
            "-- 2025-01-02 03:04:05.678 +05:30  frames, view tick\n-- File: C:\\s.sqlite\n\n" +
            "SELECT 1\nFROM t;\n-- 1,234 rows, in 2 runs\n\n" +
            "SELECT 2;\n-- 1 row\n-- On: C:\\a.sqlite\n-- On: C:\\b.sqlite\n\n" +
            "-- Then: Counted them, by name and type.\n\n",
            QueryLog.Format(time, "frames", "tick", @"C:\s.sqlite",
                [new("SELECT 1\nFROM t;", 1234, 2, null), new("SELECT 2;", 1, 1, [@"C:\a.sqlite", @"C:\b.sqlite"])],
                "Counted them,\nby name\r\nand type."),
            "A call's part of the log");
        Expect.Equal(
            "-- 2025-01-02 03:04:05.678 +05:30  record_trace\n\n-- No SQL ran.\n\n-- Then: Listed the processes.\n\n",
            QueryLog.Format(time, "record_trace", null, null, [], "Listed the processes."),
            "The log of a call that ran no SQL");

        var main = WriteMain(context);
        var tick = SessionFile.ReadFrames(main)[0].FrameId;
        var unlogged = Run(() => FrameTools.Frames(sessionPath: main, tickId: tick));
        Expect.True(unlogged.IsNull("queryLog") && unlogged.IsNull("queryLogError"), "Without a query log, a result names none.");

        var folder = Path.Combine(context.TempDirectory, "logs");
        var log = new QueryLog(folder);
        var results = WithLog(log, () => new[]
        {
            Run(() => FrameTools.Frames(sessionPath: main, tickId: tick)),
            Run(() => LaunchTools.Launch(sessionPath: main)),
            Run(() => ObjectTools.Objects(sessionPath: main)),
            Run(() => SessionTools.Sessions(view: "list", folder: context.TempDirectory)),
        });
        var path = results[0].Text("queryLog")!;
        foreach (var result in results)
        {
            Expect.Equal(path, result.Text("queryLog"), $"The log of the call of {result.Text("view")}");
        }

        Expect.Equal(path, Directory.GetFiles(folder).Single(), "The log, the only file in its folder");
        Expect.True(Regex.IsMatch(Path.GetFileName(path), $@"^queries-\d{{8}}-\d{{6}}-{Environment.ProcessId}\.sql$"),
            $"The log's name has the time it was created and the process ID: {path}");

        var text = File.ReadAllText(path);
        Expect.True(Regex.IsMatch(text, $@"\A-- \d{{4}}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{{3}} [+-]\d\d:\d\d  frames, view tick\n" +
            $@"-- File: {Regex.Escape(main)}\n\n"), $"The log starts with the first call's tool, view and file:\n{text}");
        var at = 0;
        foreach (var (result, header) in results.Zip(new[]
            { "  frames, view tick\n", "  launch, view milestones\n", "  objects, view types\n", "  sessions, view list\n" }))
        {
            at = After(text, header, at);
            foreach (var query in result.Items("queries"))
            {
                at = After(text, query.Text("sql") + "\n-- ", at);
            }

            at = After(text, $"-- Then: {result.Text("computed")}\n", at);
        }

        // The list gives the path of the file it lists, so its statement doesn't name it. The log, which lists no
        // sessions, does.
        var record = results[3].Items("queries").Single();
        Expect.True(record.IsNull("files"), $"The list's statement doesn't name the file the list gives: {record.GetRawText()}");
        var list = text[text.IndexOf("  sessions, view list\n", StringComparison.Ordinal)..];
        Expect.True(list.Contains($"{record.Text("sql")}\n-- 1 row\n-- On: {main}\n\n", StringComparison.OrdinalIgnoreCase),
            $"The log names the file the list's statement ran on:\n{list}");

        // Every call read only main: the folder listed holds no other session file.
        var listed = results.SelectMany(result => result.Items("queries")).ToList();
        using (var connection = new SqliteConnection(new SqliteConnectionStringBuilder
        {
            DataSource = main,
            Mode = SqliteOpenMode.ReadOnly,
            Pooling = false,
        }.ToString()))
        {
            connection.Open();
            using var command = connection.CreateCommand();
            command.CommandText = text;
            using var reader = command.ExecuteReader();
            var index = 0;
            do
            {
                Expect.True(index < listed.Count, $"The log, run as a script, runs only the {listed.Count} statements listed.");
                var rows = 0L;
                while (reader.Read())
                {
                    rows++;
                }

                var runs = listed[index].IsNull("runs") ? 1 : listed[index].Long("runs");
                Expect.Equal(listed[index].Long("rows"), rows * runs, $"The rows of the log's statement {index}, run as a script");
                index++;
            }
            while (reader.NextResult());
            Expect.Equal(listed.Count, index, "The statements the log runs as a script");
        }

        // Calls at once append their parts whole, one after another.
        var together = WithLog(log, () => Task.WhenAll(
            Enumerable.Range(0, 8).Select(_ => FrameTools.Frames(sessionPath: main, tickId: tick))).GetAwaiter().GetResult());
        foreach (var result in together)
        {
            Expect.Equal(path, Json(result).Text("queryLog"), "The log of a call that ran with 7 others");
        }

        var all = File.ReadAllText(path);
        Expect.True(all.StartsWith(text, StringComparison.Ordinal), "Later calls append to the log.");
        var added = Regex.Replace(all[text.Length..], @"^-- \d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{3} [+-]\d\d:\d\d  ", "-- (time)  ",
            RegexOptions.Multiline);
        Expect.Equal(string.Concat(Enumerable.Repeat(added[..(added.Length / 8)], 8)), added,
            "The parts of 8 calls of the same view at once, each whole");

        static int After(string text, string part, int start)
        {
            var found = text.IndexOf(part, start, StringComparison.Ordinal);
            Expect.True(found >= 0, $"The log has, after offset {start}: {part}");
            return found + part.Length;
        }
    }

    // Creating a log deletes the folder's query logs not written to for 30 days, and nothing else.
    private static void LogCleanup(SelfTestContext context)
    {
        var main = WriteMain(context);
        var folder = Directory.CreateDirectory(Path.Combine(context.TempDirectory, "logs")).FullName;
        var expired = Old("queries-20250101-000000-1.sql", 31);
        var kept = new[]
        {
            Old("queries-20250101-000000-2.sql", 29),
            Old("publish-0123456789abcdef-20250101-000000.log", 31),
            Old("notes.sql", 31),
        };
        var result = WithLog(new QueryLog(folder), () => Run(() => FrameTools.Frames(sessionPath: main)));
        Expect.True(!File.Exists(expired), "A query log not written to for 31 days is deleted.");
        foreach (var file in kept)
        {
            Expect.True(File.Exists(file), $"{Path.GetFileName(file)} is kept.");
        }

        Expect.True(File.Exists(result.Text("queryLog")), "The new log is written.");

        string Old(string name, int days)
        {
            var file = Path.Combine(folder, name);
            File.WriteAllText(file, "-- old\n");
            File.SetLastWriteTimeUtc(file, DateTime.UtcNow.AddDays(-days));
            return file;
        }
    }

    // A log that can't be written doesn't fail the call: its result says why instead.
    private static void LogError(SelfTestContext context)
    {
        var main = WriteMain(context);
        var blocked = Path.Combine(context.TempDirectory, "logs");
        File.WriteAllText(blocked, "A file where the log's folder would be.");
        var result = WithLog(new QueryLog(blocked), () => Run(() => FrameTools.Frames(sessionPath: main)));
        Expect.True(result.IsNull("queryLog"), "A log that couldn't be written isn't named.");
        var error = result.Text("queryLogError");
        var expected = $"The SQL couldn't be written to the query log in {blocked}: ";
        Expect.True(error?.StartsWith(expected, StringComparison.Ordinal) == true && error.Length > expected.Length,
            $"The error of a log that couldn't be written says where and why: {error}");
        Expect.True(CheckQueries(result, "A call whose log couldn't be written").Count > 0, "It still lists its statements.");
    }

    private static T WithLog<T>(QueryLog log, Func<T> calls)
    {
        var before = QueryLog.Current;
        QueryLog.Current = log;
        try
        {
            return calls();
        }
        finally
        {
            QueryLog.Current = before;
        }
    }

    private static bool Throws<T>(Action action)
        where T : Exception
    {
        try
        {
            action();
        }
        catch (T)
        {
            return true;
        }

        return false;
    }

    private static string Describe(object value) => value switch
    {
        byte[] bytes => $"byte[{bytes.Length}]",
        _ => string.Create(CultureInfo.InvariantCulture, $"{value.GetType().Name} {value}"),
    };
}

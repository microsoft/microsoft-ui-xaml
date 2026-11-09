// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Concurrent;
using System.Globalization;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using static WinUIProfiler.Mcp.SelfTest.SessionFixtures;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>
/// The SQL behind the answers: SessionFile reports each statement its reads run, with the values written in, and each
/// statement, run as reported, returns the rows reported.
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
    ];

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

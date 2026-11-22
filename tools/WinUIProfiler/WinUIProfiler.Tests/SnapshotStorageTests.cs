using System.Text.Json;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Snapshots;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;

internal static class SnapshotStorageTests
{
    private static readonly DateTime Created = new(2026, 1, 1, 0, 0, 0, DateTimeKind.Utc);

    internal static async Task RunAsync()
    {
        var directory = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-SnapshotStorage-{Guid.NewGuid():N}");
        Directory.CreateDirectory(directory);
        try
        {
            await RoundTripAsync(Path.Combine(directory, "roundtrip.sqlite"));
            await RetentionAsync(Path.Combine(directory, "retention.sqlite"));
            await ProcessRoutingAsync(directory);
        }
        finally
        {
            foreach (var file in Directory.GetFiles(directory)) File.Delete(file);
            Directory.Delete(directory);
        }
        Console.WriteLine("Passed: SQLite snapshot persistence, exact counters/properties, comparisons, process isolation and retention.");
    }

    private static SessionStart Source(int pid = 42) => new(pid, $"Snapshots{pid}", Created, 1000, 0, SessionCoverage.Connection);

    private static async Task RoundTripAsync(string path)
    {
        var before = Capture(100);
        var after = Capture(200, changed: true);
        var queries = new List<SessionQuery>();
        using (var writer = SessionWriter.Create(path, Source()))
        {
            await writer.SaveSnapshotAsync(before, path + ".json");
            using (SessionQueries.Observe(queries.Add))
            {
                Require(SessionFile.ReadTreeSnapshots(path).Single() is { Roots: 1, XamlNodes: 2, CompositionNodes: 2, Correlations: 2 },
                    "A completed save must be visible to a live reader before the writer closes.");
                var stored = SessionFile.ReadTreeSnapshot(path, before.CaptureId);
                Require(TreeSnapshotCodec.Encode(stored).SequenceEqual(TreeSnapshotCodec.Encode(before)),
                    "The database must retain the entire capture independently of its JSON export.");
            }
            Require(!File.Exists(path + ".json") && queries.Count == 2,
                "Snapshot reads must not require the export and must report their shared SQL queries.");
            await writer.SaveSnapshotAsync(after, path + ".json");
            await ThrowsAsync<IOException>(() => writer.SaveSnapshotAsync(before, path + ".json"));
            await ThrowsAsync<InvalidDataException>(() => writer.SaveSnapshotAsync(after with { ProcessId = 43 }, path + ".json"));
            await ThrowsAsync<InvalidDataException>(() => writer.SaveSnapshotAsync(after with
                { CreationTimeFileTime = Created.AddSeconds(1).ToFileTimeUtc() }, path + ".json"));
            await ThrowsAsync<InvalidDataException>(() => writer.SaveSnapshotAsync(after with { QpcFrequency = 2000 }, path + ".json"));
            using var cancelled = new CancellationTokenSource();
            cancelled.Cancel();
            await ThrowsAsync<OperationCanceledException>(() => writer.SaveSnapshotAsync(Capture(300), path + ".json", cancelled.Token));
            writer.Process(1, new(50, 50, 42, 7, XamlProvider.Xaml, true, "Tick", 0, []));
            writer.Close("Test complete");
            Require(writer.Failure is null, "Invalid, duplicate or cancelled captures must not stop event recording.");
            await ThrowsAsync<ObjectDisposedException>(() => writer.SaveSnapshotAsync(Capture(400), path + ".json"));
        }
        Require(SessionFile.ReadInfo(path).StoredEvents == 1 &&
            SessionFile.ReadTreeSnapshots(path).Select(row => row.CaptureId).SequenceEqual([before.CaptureId, after.CaptureId]),
            "Only successful snapshots must survive reopening, alongside the session's events.");
        var diff = new SnapshotDiffData(SnapshotViewerData.FromDocument(SessionFile.ReadTreeSnapshot(path, before.CaptureId)),
            SnapshotViewerData.FromDocument(SessionFile.ReadTreeSnapshot(path, after.CaptureId)));
        Require(diff.Xaml.Count(SnapshotChange.Properties) == 1 && diff.Composition.Count(SnapshotChange.Properties) == 1,
            "Database-only snapshots must remain usable by the existing comparison engine.");
        using (var connection = SessionFile.Open(path))
        {
            using var command = connection.CreateCommand();
            command.CommandText = """
                SELECT NodeId, RootId, ParentId, Properties -> '$.unknown.counter', Properties -> '$.explicitNull'
                FROM SnapshotNodes WHERE CaptureId = $id AND Tree = 'Xaml' AND Name = 'Child'
                """;
            command.Parameters.AddWithValue("$id", before.CaptureId.ToString("D"));
            using (var reader = command.ExecuteReader())
            {
                Require(reader.Read() && reader.GetString(0) == (ulong.MaxValue - 2).ToString() &&
                    reader.GetString(1) == ulong.MaxValue.ToString() && reader.GetString(2) == (ulong.MaxValue - 1).ToString() &&
                    reader.GetString(3) == ulong.MaxValue.ToString() && reader.GetString(4) == "null",
                    "SQL views must preserve full unsigned IDs, nested counters and explicit JSON null.");
            }
            Require(Sql.Scalar(connection, "SELECT COUNT(*) FROM SnapshotRoots") == 2 &&
                Sql.Scalar(connection, """
                    SELECT COUNT(*) FROM SnapshotCorrelations c
                    JOIN SnapshotNodes x ON x.CaptureId = c.CaptureId AND x.Tree = 'Xaml' AND x.NodeId = c.XamlNodeId
                    JOIN SnapshotNodes v ON v.CaptureId = c.CaptureId AND v.Tree = 'Composition' AND v.NodeId = c.VisualNodeId
                    """) == 4, "Root and correlation views must retain capture-local identities in both trees.");
            command.CommandText = "SELECT TotalCreated, TotalDestroyed, LiveCount FROM SnapshotResources WHERE CaptureId = $id";
            using (var reader = command.ExecuteReader())
                Require(reader.Read() && reader.GetString(0) == ulong.MaxValue.ToString() &&
                    reader.GetString(1) == "0" && reader.GetString(2) == ulong.MaxValue.ToString(),
                    "Graphics counters must stay exact decimal text rather than SQLite REAL or wrapped signed integers.");
            Require(Sql.Scalar(connection, "SELECT AllocCount FROM SnapshotSummary WHERE CaptureId = $id",
                null, ("$id", before.CaptureId.ToString("D"))) == -1,
                "Unavailable signed allocator values must remain -1, not invented zeros.");
            Require(Sql.Scalar(connection, """
                SELECT COUNT(*) FROM SnapshotNodes b
                JOIN SnapshotRoots br ON br.CaptureId = b.CaptureId AND br.RootId = b.RootId
                JOIN SnapshotNodes a ON a.Tree = b.Tree AND a.ObjectAddress = b.ObjectAddress AND a.Type = b.Type
                JOIN SnapshotRoots ar ON ar.CaptureId = a.CaptureId AND ar.RootId = a.RootId
                    AND ar.ThreadId = br.ThreadId AND ar.XamlRootAddress = br.XamlRootAddress
                WHERE b.CaptureId = $before AND a.CaptureId = $after
                    AND (b.Properties -> '$.opacity') IS NOT (a.Properties -> '$.opacity')
                """, null, ("$before", before.CaptureId.ToString("D")), ("$after", after.CaptureId.ToString("D"))) == 2,
                "SQLite must compare property changes using rooted object identity, not changing capture-local node IDs.");
        }
        using (var connection = new SqliteConnection($"Data Source={path};Pooling=False"))
        {
            connection.Open();
            Sql.Execute(connection, """
                DROP VIEW SnapshotSummary; DROP VIEW SnapshotRoots; DROP VIEW SnapshotNodes;
                DROP VIEW SnapshotCorrelations; DROP VIEW SnapshotResources; DROP TABLE TreeSnapshots;
                """);
        }
        Require(SessionFile.ReadTreeSnapshots(path).Count == 0, "Earlier databases must remain readable without snapshots.");
        await ThrowsAsync<InvalidDataException>(() =>
        {
            SessionFile.ReadTreeSnapshot(path, before.CaptureId);
            return Task.CompletedTask;
        });
    }

    private static async Task RetentionAsync(string path)
    {
        using (var writer = SessionWriter.Create(path, Source(), 1024 * 1024))
        {
            var captures = new List<TreeSnapshotDocument>();
            for (var i = 0; i < 8; i++)
            {
                var capture = Capture(i * 100, padding: new string('x', 48_000));
                captures.Add(capture);
                await writer.SaveSnapshotAsync(capture, path + ".json");
            }
            var retained = SessionFile.ReadTreeSnapshots(path);
            Require(retained.Count is > 0 and < 8 && retained.Last().CaptureId == captures.Last().CaptureId &&
                !retained.Any(row => row.CaptureId == captures[0].CaptureId),
                "Size retention must delete whole oldest captures, including a capture whose end QPC is zero.");
            foreach (var capture in retained) SessionFile.ReadTreeSnapshot(path, capture.CaptureId);
            var oversized = Capture(900, padding: new string('x', 48_000));
            oversized = oversized with
            {
                XamlNodes = oversized.XamlNodes.Concat(Enumerable.Range(1, 24).Select(i =>
                    oversized.XamlNodes[1] with { Id = (ulong)i, ChildIndex = i })).ToArray()
            };
            await ThrowsAsync<IOException>(() => writer.SaveSnapshotAsync(oversized, path + ".json"));
            Require(SessionFile.ReadTreeSnapshots(path).Select(row => row.CaptureId).SequenceEqual(retained.Select(row => row.CaptureId)),
                "An oversized snapshot must fail explicitly without evicting retained history.");
            writer.Close("Test complete");
            Require(writer.Failure is null && new FileInfo(path).Length <= 1024 * 1024,
                "Snapshot-only recordings must respect the existing session size limit.");
        }
        File.Delete(path);
        using (var writer = SessionWriter.Create(path, Source(), 1024 * 1024))
        {
            var capture = Capture(1_000_000);
            await writer.SaveSnapshotAsync(capture, path + ".json");
            for (var i = 1; i <= 10_000; i++)
                writer.Process(i, new(i, i, 42, 7, XamlProvider.Xaml, true, "Tick", 0, [new("Text", new string('x', 120))]));
            writer.Close("Test complete");
            Require(writer.Failure is null && SessionFile.ReadInfo(path).CompleteFromSeq is not null &&
                SessionFile.ReadTreeSnapshots(path).Single().CaptureId == capture.CaptureId,
                "Trimming older ETW events must not invalidate a newer, self-contained stored snapshot.");
        }
    }

    private static async Task ProcessRoutingAsync(string directory)
    {
        using var group = new ProcessRecordingGroup([Source(), Source(43)], 42, null, directory);
        var capture = Capture(100);
        await group.SaveSnapshotAsync(capture, Path.Combine(directory, "capture.json"), default);
        group.Close("Test complete");
        Require(SessionFile.ReadTreeSnapshots(group.RecordingPaths[0]).Single().CaptureId == capture.CaptureId &&
            SessionFile.ReadTreeSnapshots(group.RecordingPaths[1]).Count == 0,
            "A startup recording group must save snapshots only in the connected target's database.");
    }

    private static TreeSnapshotDocument Capture(long endQpc, bool changed = false, string padding = "")
    {
        var startQpc = Math.Max(0, endQpc - 10);
        ulong root = changed ? 9 : ulong.MaxValue;
        ulong parent = changed ? 21 : ulong.MaxValue - 1;
        ulong child = changed ? 22 : ulong.MaxValue - 2;
        var properties = JsonSerializer.SerializeToElement(new
        {
            opacity = changed ? 0.5 : 1,
            unknown = new { counter = ulong.MaxValue, text = "quoted \"value\"" },
            explicitNull = (string?)null,
            padding
        });
        var empty = JsonSerializer.SerializeToElement(new { padding });
        return new(4, Guid.NewGuid(), 42, Created.ToFileTimeUtc(), 1000, startQpc, endQpc,
            [new(root, 7, startQpc, endQpc, "0xFFFFFFFFFFFFFFFF")],
            [new(parent, root, null, 0, "Grid", "Root", "0xFFFFFFFFFFFFFFF1", null, empty),
                new(child, root, parent, 0, "Button", "Child", "0xFFFFFFFFFFFFFFF2", "0xFFFFFFFFFFFFFFF3", properties)],
            [new(parent, root, null, 0, "ContainerVisual", "0xFFFFFFFFFFFFFFF4", "0xFFFFFFFFFFFFFFF5", empty),
                new(child, root, parent, 0, "SpriteVisual", "0xFFFFFFFFFFFFFFF6", "0xFFFFFFFFFFFFFFF7", properties)],
            [new(parent, parent), new(child, child)], ["Captured trees are not atomic."],
            new("available", 0, startQpc, endQpc, new(-1, 0, -1, 4, 40,
                [new(24, ulong.MaxValue, 0, ulong.MaxValue)])));
    }

    private static void Require(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }

    private static async Task ThrowsAsync<T>(Func<Task> action) where T : Exception
    {
        try { await action(); }
        catch (T) { return; }
        throw new InvalidOperationException($"Expected {typeof(T).Name}.");
    }
}

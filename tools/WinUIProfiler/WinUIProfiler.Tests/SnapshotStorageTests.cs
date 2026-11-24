using System.Text;
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
            await RetentionFailureAsync(Path.Combine(directory, "retention-failure.sqlite"));
            await EscapedDocumentAsync(Path.Combine(directory, "escaped.sqlite"));
            await ProcessRoutingAsync(directory);
            await StartupFailureAsync(directory);
        }
        finally
        {
            foreach (var file in Directory.GetFiles(directory)) File.Delete(file);
            Directory.Delete(directory);
        }
        Console.WriteLine("Passed: SQLite snapshots, escaped documents, comparisons, retention failures and JSON-only startup fallback.");
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
                WITH Nodes AS MATERIALIZED (
                    SELECT * FROM SnapshotNodes WHERE CaptureId IN ($before, $after)),
                Roots AS MATERIALIZED (
                    SELECT * FROM SnapshotRoots WHERE CaptureId IN ($before, $after))
                SELECT COUNT(*) FROM Nodes b
                JOIN Roots br ON br.CaptureId = b.CaptureId AND br.RootId = b.RootId
                JOIN Nodes a ON a.Tree = b.Tree AND a.ObjectAddress = b.ObjectAddress AND a.Type = b.Type
                JOIN Roots ar ON ar.CaptureId = a.CaptureId AND ar.RootId = a.RootId
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
        var closed = await ThrowsAsync<IOException>(() => group.SaveSnapshotAsync(Capture(200),
            Path.Combine(directory, "closed.json"), default));
        Require(closed.InnerException is ObjectDisposedException && closed.Message.Contains("closed.json"),
            "A recording closed after use must still report snapshot storage errors with the saved export path.");
    }

    private static async Task RetentionFailureAsync(string path)
    {
        using var writer = SessionWriter.Create(path, Source(), 1024 * 1024);
        for (var i = 1; i <= 1000; i++)
            writer.Process(i, new(i, i, 42, 7, XamlProvider.Xaml, true, "ElementEnteredTree", 0,
                [new("ElementId", (long)i), new("PeerHandle", 0x1000L + i), new("Text", new string('x', 256))]));
        await writer.SaveSnapshotAsync(Capture(1_000_000), path + ".json");
        var capture = Capture(2_000_000, padding: new string('x', 48_000));
        capture = capture with
        {
            XamlNodes = capture.XamlNodes.Concat(Enumerable.Range(1, 8).Select(i =>
                capture.XamlNodes[1] with { Id = (ulong)i, ChildIndex = i })).ToArray()
        };
        using (var connection = new SqliteConnection($"Data Source={path};Pooling=False"))
        {
            connection.Open();
            Require(SessionFile.ReadInfo(path).CompleteFromSeq is null &&
                Sql.Scalar(connection, "PRAGMA page_count") * Sql.Scalar(connection, "PRAGMA page_size") +
                    TreeSnapshotCodec.Encode(capture).LongLength > writer.LimitBytes * 0.9,
                "The fault must occur during snapshot-triggered retention, not while seeding events.");
            // Objects.Trim has already changed its caches when the later Events delete fails.
            Sql.Execute(connection, """
                CREATE TRIGGER FailRetention BEFORE DELETE ON Events
                BEGIN SELECT RAISE(ABORT, 'injected retention failure'); END
                """);
        }
        var error = await ThrowsAsync<SqliteException>(() => writer.SaveSnapshotAsync(capture, path + ".json")
            .WaitAsync(TimeSpan.FromSeconds(10)));
        Require(ReferenceEquals(writer.Failure, error) && error.Message.Contains("injected retention failure"),
            "Retention rollback must enter Run's fatal failure path before any producer can reuse stale index caches.");
        writer.Process(1001, new(1001, 1001, 42, 7, XamlProvider.Xaml, true, "ElementEnteredTree", 0,
            [new("ElementId", 1L), new("PeerHandle", 0x1001L)]));
        var subsequent = await ThrowsAsync<IOException>(() => writer.SaveSnapshotAsync(Capture(3_000_000), path + ".json"));
        Require(ReferenceEquals(subsequent.InnerException, error), "Later snapshot saves must expose the original writer failure.");
        writer.Close("Test complete");
        using var retained = SessionFile.Open(path);
        Require(Sql.Scalar(retained, "SELECT COUNT(*) FROM Events") == 1000 &&
            Sql.Scalar(retained, "SELECT COUNT(*) FROM Objects") == 1000 &&
            Sql.Scalar(retained, "SELECT COUNT(*) FROM TreeSnapshots") == 1,
            "Failed retention must roll back its rows and prevent further writes from stale cached state.");
    }

    private static async Task EscapedDocumentAsync(string path)
    {
        // Native JSON can contain raw icon/emoji text that System.Text.Json escapes even with relaxed encoding.
        var text = string.Concat(Enumerable.Repeat("\uE700\U0001F600", 7000));
        using var properties = JsonDocument.Parse("{\"text\":\"" + text + "\"}");
        var capture = Capture(100);
        capture = capture with
        {
            XamlNodes = capture.XamlNodes.Concat(Enumerable.Range(1, 540).Select(i =>
                capture.XamlNodes[1] with { Id = (ulong)i, ChildIndex = i + 1, ObjectAddress = $"0x{0x2000 + i:X}",
                    Properties = properties.RootElement })).ToArray()
        };
        var savedBytes = TreeSnapshotCodec.Encode(capture);
        var nativeBytes = Encoding.UTF8.GetBytes(Encoding.UTF8.GetString(savedBytes)
            .Replace("\\uE700", "\uE700", StringComparison.OrdinalIgnoreCase)
            .Replace("\\uD83D\\uDE00", "\U0001F600", StringComparison.OrdinalIgnoreCase));
        Require(nativeBytes.Length < TapProtocol.MaximumSnapshotBytes && savedBytes.Length > TapProtocol.MaximumSnapshotBytes,
            "The fixture must represent a valid transfer whose escaped saved document exceeds the transfer limit.");
        var native = TreeSnapshotCodec.Decode(nativeBytes);
        await ThrowsAsync<InvalidDataException>(() =>
        {
            TreeSnapshotCodec.Decode(savedBytes);
            return Task.CompletedTask;
        });
        var jsonPath = Path.ChangeExtension(path, ".json");
        await using (var output = File.Create(jsonPath)) await TreeSnapshotCodec.WriteAsync(output, native, default);
        using (var writer = SessionWriter.Create(path, Source()))
        {
            await writer.SaveSnapshotAsync(native, jsonPath);
            writer.Close("Test complete");
            Require(writer.Failure is null, "A valid escaped document must not stop snapshot recording.");
        }
        var fromFile = await SnapshotViewerData.LoadAsync(jsonPath);
        File.Delete(jsonPath);
        var fromDatabase = SnapshotViewerData.FromDocument(SessionFile.ReadTreeSnapshot(path, capture.CaptureId));
        Require(fromDatabase.Xaml.Nodes[1].Properties.GetProperty("text").GetString() == text &&
            new SnapshotDiffData(fromFile, fromDatabase).Xaml.Nodes.All(node => node.Change == SnapshotChange.None),
            "Expanded exports and database-only comparisons must validate stored documents, not transferred byte lengths.");
    }

    private static async Task StartupFailureAsync(string directory)
    {
        using (var group = new ProcessRecordingGroup([Source() with { ProcessName = "StartupFailure" }], 42, null, directory))
        {
            var recordingPath = group.SnapshotRecordingPath!;
            group.AbortStartup();
            for (var i = 1; i <= 2; i++)
            {
                var capture = Capture(i * 100);
                var jsonPath = Path.Combine(directory, $"json-only-{i}.json");
                await using (var output = File.Create(jsonPath)) await TreeSnapshotCodec.WriteAsync(output, capture, default);
                await group.SaveSnapshotAsync(capture, jsonPath, default);
                Require((await SnapshotViewerData.LoadAsync(jsonPath)).Document.CaptureId == capture.CaptureId,
                    "Initial and subsequent JSON-only captures must remain usable after ETW startup fails.");
            }
            Require(group.SnapshotRecordingPath is null && SessionFile.ReadTreeSnapshots(recordingPath).Count == 0,
                "Failed-startup recording inventories may stay visible, but JSON-only captures must not claim database storage.");
        }
        using (var group = new ProcessRecordingGroup([Source() with { ProcessName = "ActiveFailure" }], 42, null, directory))
        {
            var jsonPath = Path.Combine(directory, "active-failure.json");
            var invalid = await ThrowsAsync<IOException>(() => group.SaveSnapshotAsync(Capture(100) with { ProcessId = 43 },
                jsonPath, default));
            Require(invalid.InnerException is InvalidDataException && invalid.Message.Contains(jsonPath),
                "Active-recording validation failures must report the saved JSON path instead of silently falling back.");
            using (var connection = new SqliteConnection($"Data Source={group.SnapshotRecordingPath};Pooling=False"))
            {
                connection.Open();
                Sql.Execute(connection, """
                    CREATE TRIGGER FailSnapshot BEFORE INSERT ON TreeSnapshots
                    BEGIN SELECT RAISE(ABORT, 'injected snapshot failure'); END
                    """);
            }
            var error = await ThrowsAsync<IOException>(() => group.SaveSnapshotAsync(Capture(200), jsonPath, default));
            Require(error.InnerException is SqliteException && error.Message.Contains(jsonPath),
                "Active-recording SQL failures must remain visible and identify the saved export.");
        }
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

    private static async Task<T> ThrowsAsync<T>(Func<Task> action) where T : Exception
    {
        try { await action(); }
        catch (T error) { return error; }
        throw new InvalidOperationException($"Expected {typeof(T).Name}.");
    }
}

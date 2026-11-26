using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Storage;

internal static class ProcessMetricStorageTests
{
    internal static void Run()
    {
        var directory = Path.GetFullPath($".metric-storage-tests-{Guid.NewGuid():N}");
        Directory.CreateDirectory(directory);
        try
        {
            RoundTrip(Path.Combine(directory, "roundtrip.sqlite"));
            Retention(Path.Combine(directory, "retention.sqlite"));
            SharedTransactionRetention(Path.Combine(directory, "shared-retention.sqlite"));
            WriterFailure(Path.Combine(directory, "failure.sqlite"));
            OlderRecording(Path.Combine(directory, "older.sqlite"));
            PrimaryRouting(directory);
        }
        finally
        {
            Directory.Delete(directory, recursive: true);
        }
        Console.WriteLine("Passed: native metrics SQLite values/identity/clock, close drain, routing, retention and failures.");
    }

    private static readonly DateTime Created = new(2026, 11, 25, 0, 0, 0, DateTimeKind.Utc);
    private static SessionStart Start => new(42, "MetricTests", Created, 10_000_000, 100, SessionCoverage.Connection);
    private static ProcessInstanceKey Identity => new(42, Created.ToFileTimeUtc(), Guid.NewGuid());
    private static MetricReading Reading(ProcessInstanceKey identity, long qpc, string source = "native-test") =>
        new(identity, qpc,
        [
            new(ProcessMetric.Cpu, 0, source, null),
            new(ProcessMetric.Gpu, null, source, "GPU unavailable"),
            new(ProcessMetric.WorkingSet, 12.5, source, null),
            new(ProcessMetric.PrivateWorkingSet, 8.25, source, null),
            new(ProcessMetric.PrivateCommit, 20, source, null)
        ]);

    private static void RoundTrip(string path)
    {
        var identity = Identity;
        using (var writer = SessionWriter.Create(path, Start))
        {
            var values = Reading(identity, 123456).Values.ToList();
            writer.PostMetrics(new(identity, 123456, values));
            values.Clear();
            for (var i = 1; i < 64; i++) writer.PostMetrics(Reading(identity, 123456 + i));
            Reject(() => writer.PostMetrics(Reading(new(43, identity.CreationTimeFileTime, identity.InstanceId), 1234567)));
            Reject(() => writer.PostMetrics(Reading(new(42, identity.CreationTimeFileTime + 1, identity.InstanceId), 1234567)));
            Reject(() => writer.PostMetrics(Reading(Identity, 1234567)));
            Reject(() => writer.PostMetrics(Reading(identity, 123455)));
            writer.Close("Metric drain");
            Check(writer.Failure is null, "Closing must drain all accepted acquisitions without a writer failure.");
            try
            {
                writer.PostMetrics(Reading(identity, 1234567));
                throw new Exception("Posting after Close must fail.");
            }
            catch (ObjectDisposedException) { }
        }
        Check(!File.Exists(path + "-wal") && !File.Exists(path + "-shm"), "Finalized SQLite must stand alone.");
        using var connection = Open(path);
        Check(Sql.Scalar(connection, "SELECT COUNT(*) FROM ProcessMetrics") == 64 * 5, "Close must drain every metric.");
        using var command = connection.CreateCommand();
        command.CommandText = """
            SELECT Qpc, ProcessId, CreationTimeFileTime, InstanceId, Metric, Value, Unit, Source, UnavailableReason
            FROM ProcessMetrics WHERE AcquisitionId = 1 ORDER BY Metric
            """;
        using (var reader = command.ExecuteReader())
        {
            var count = 0;
            while (reader.Read())
            {
                count++;
                Check(reader.GetInt64(0) == 123456 && reader.GetInt64(1) == 42 &&
                    reader.GetInt64(2) == identity.CreationTimeFileTime && reader.GetString(3) == identity.InstanceId.ToString("D") &&
                    reader.GetString(7) == "native-test", "QPC, source and exact process instance must survive SQLite.");
                var name = reader.GetString(4);
                Check(reader.GetString(6) == (name is "cpu" or "gpu" ? "percent" : "MiB"), "Units must match graphs.");
                if (name == "gpu")
                    Check(reader.IsDBNull(5) && reader.GetString(8) == "GPU unavailable", "Missing is NULL, with its reason.");
                else
                {
                    var expected = name switch { "cpu" => 0, "workingSet" => 12.5, "privateWorkingSet" => 8.25, _ => 20 };
                    Check(reader.GetDouble(5) == expected && reader.IsDBNull(8), "Measured values, including zero, remain numeric.");
                }
            }
            Check(count == 5, "All five explicit graph series must be persisted.");
        }
        Check(Sql.Scalar(connection, "SELECT QpcFrequency FROM SessionInfo") == 10_000_000, "The session retains its QPC clock.");
        Check(Sql.Scalar(connection, "SELECT COUNT(*) FROM ProcessMetricRetention") == 0, "Untrimmed evidence has no retention cutoff.");
    }

    private static void Retention(string path)
    {
        const long limit = 1024 * 1024;
        var identity = Identity;
        using (var writer = SessionWriter.Create(path, Start, limit))
        {
            for (var i = 1; i <= 160; i++) writer.PostMetrics(Reading(identity, i, new string('s', 4000)));
            writer.Close("Metric retention");
            Check(writer.Failure is null, $"Retention must not stop persistence: {writer.Failure}");
            Check(writer.Status.FileBytes <= limit, "Metrics-only recordings must stay within their file limit.");
        }
        using var connection = Open(path);
        var rows = Sql.Scalar(connection, "SELECT COUNT(*) FROM ProcessMetrics");
        Check(rows is > 0 and < 800 && rows % 5 == 0, "Retention must delete whole acquisitions, not grow unbounded.");
        Check(Sql.Scalar(connection, "SELECT MAX(Qpc) FROM ProcessMetrics") == 160, "Retention must retain the final acquisition.");
        Check(Sql.Scalar(connection, "SELECT CompleteFromQpc FROM ProcessMetricRetention") ==
            Sql.Scalar(connection, "SELECT MIN(Qpc) FROM ProcessMetrics"), "Retention must declare its evidence boundary.");
    }

    private static void SharedTransactionRetention(string path)
    {
        var identity = Identity;
        using (var writer = SessionWriter.Create(path, Start))
        {
            writer.PostMetrics(Reading(identity, 101));
            writer.PostMetrics(Reading(identity, 102));
            writer.Close("Shared retention fixture");
            Check(writer.Failure is null, "The retention fixture must finish writing.");
        }
        using var connection = new SqliteConnection($"Data Source={path};Pooling=False");
        connection.Open();
        var index = new ProcessMetricIndex(connection);
        using (var transaction = connection.BeginTransaction())
        {
            Check(index.FirstQpc(transaction) == 101, "Metric retention must read through the snapshot transaction.");
            Check(index.TrimOldest(transaction) && index.FirstQpc(transaction) == 102,
                "Metric retention must participate in the existing snapshot transaction.");
            transaction.Rollback();
        }
        Check(Sql.Scalar(connection, "SELECT COUNT(*) FROM ProcessMetrics") == 10 &&
            Sql.Scalar(connection, "SELECT COUNT(*) FROM ProcessMetricRetention") == 0,
            "Rolling back snapshot retention must restore metric acquisitions and their evidence boundary.");
    }

    private static void WriterFailure(string path)
    {
        using var writer = SessionWriter.Create(path, Start);
        using (var connection = new SqliteConnection($"Data Source={path};Pooling=False"))
        {
            connection.Open();
            Sql.Execute(connection, """
                CREATE TRIGGER RejectMetric BEFORE INSERT ON ProcessMetrics
                BEGIN SELECT RAISE(FAIL, 'test persistence failure'); END
                """);
        }
        writer.PostMetrics(Reading(Identity, 1));
        writer.Close("Expected failure");
        Check(writer.Failure is SqliteException, "Native acquisition write failures must remain observable.");
    }

    private static void PrimaryRouting(string directory)
    {
        using var group = new ProcessRecordingGroup([Start, Start with { ProcessId = 43 }], 42, null, directory);
        var identity = Identity;
        group.PostMetrics(Reading(identity, 500));
        Reject(() => group.PostMetrics(Reading(new(43, identity.CreationTimeFileTime, Guid.NewGuid()), 501)));
        var paths = group.RecordingPaths;
        group.Close("Routing test");
        using var primary = Open(paths[0]);
        using var child = Open(paths[1]);
        Check(Sql.Scalar(primary, "SELECT COUNT(*) FROM ProcessMetrics") == 5 &&
            Sql.Scalar(child, "SELECT COUNT(*) FROM ProcessMetrics") == 0, "Only the correct primary writer receives GUI metrics.");
    }

    private static void OlderRecording(string path)
    {
        using (var writer = SessionWriter.Create(path, Start)) writer.Close("Older schema fixture");
        using (var connection = new SqliteConnection($"Data Source={path};Pooling=False"))
        {
            connection.Open();
            Sql.Execute(connection, "DROP TABLE ProcessMetrics; DROP TABLE ProcessMetricRetention;");
        }
        Check(SessionFile.ReadInfo(path).ProcessId == 42, "Existing read-only recordings must remain readable without metric tables.");
        using var older = Open(path);
        Check(Sql.Scalar(older, "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'ProcessMetrics'") == 0,
            "Older recordings have missing native metric evidence, not invented zero rows.");
    }

    private static SqliteConnection Open(string path)
    {
        var connection = new SqliteConnection($"Data Source={path};Mode=ReadOnly;Pooling=False");
        connection.Open();
        return connection;
    }

    private static void Reject(Action action)
    {
        try { action(); }
        catch (ArgumentException) { return; }
        throw new Exception("Invalid metric identity or timestamp must be rejected.");
    }

    private static void Check(bool condition, string message)
    {
        if (!condition) throw new Exception(message);
    }
}

using System.Diagnostics;
using System.Security.Cryptography;
using System.Text.Json;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;

internal static class QueueDiagnosticsTests
{
    internal static async Task RunAsync()
    {
        using var entered = new ManualResetEventSlim();
        using var release = new ManualResetEventSlim();
        var pipeline = new EventPipeline([new GateProcessor(entered, release)], capacity: 2);
        try
        {
            pipeline.Post(Event());
            Require(entered.Wait(TimeSpan.FromSeconds(5)), "The processor must start.");
            pipeline.Post(Event());
            pipeline.Post(Event());
            pipeline.Post(Event());
            Require(pipeline.Diagnostics is { Depth: 2, HighWaterMark: 2, AcceptedEvents: 3, DroppedEvents: 1 },
                "A full live queue must report its depth, accepted events and loss.");
        }
        finally
        {
            release.Set();
            pipeline.Dispose();
        }
        Require(pipeline.Failure is null &&
            pipeline.Diagnostics is { Depth: 0, ProcessedEvents: 3, DroppedEvents: 1, ProcessingMilliseconds: > 0 },
            "Final queue diagnostics must survive disposal and include the drained events.");

        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-Queue-{Guid.NewGuid():N}.sqlite");
        try
        {
            using var writer = SessionWriter.Create(path, new(1, "QueueTests", null, 10_000_000, 0, SessionCoverage.Trace));
            using var blocker = new SqliteConnection($"Data Source={path};Pooling=False");
            blocker.Open();
            using var transaction = blocker.BeginTransaction();
            var producer = Task.Run(() =>
            {
                for (var i = 1; i <= 24_000; i++) writer.Process(i, Event());
            });
            try
            {
                var timeout = Stopwatch.StartNew();
                while (writer.Diagnostics.BackpressureCount == 0 && !producer.IsCompleted &&
                    timeout.Elapsed < TimeSpan.FromSeconds(5))
                    await Task.Delay(10);
                Require(writer.Diagnostics is { HighWaterMark: 16_384, BackpressureCount: > 0 },
                    "A stalled SQLite writer must expose the full queue and producer backpressure.");
            }
            finally
            {
                transaction.Rollback();
                await producer;
            }
            writer.Close("Queue diagnostics test");
            Require(writer.Failure is null && writer.Status.StoredEvents == 24_000 &&
                writer.Diagnostics is { Depth: 0, BackpressureMilliseconds: > 0, Batches: > 0,
                    EncodingMilliseconds: > 0, TransactionMilliseconds: > 0 },
                "Writer diagnostics must distinguish backpressure from encoding and database work without losing events.");
        }
        finally
        {
            foreach (var file in new[] { path, path + "-wal", path + "-shm" }) File.Delete(file);
        }
        Console.WriteLine("Passed: queue saturation, writer backpressure, loss accounting and finalized diagnostics.");
    }

    // This replays only the events that survived capture. It diagnoses profiler throughput, not app performance,
    // and cannot reconstruct dropped events, CPU-stack writes, ETW delivery bursts or competing live workloads.
    internal static async Task DiagnoseAsync(string source, string directory)
    {
        if (Directory.Exists(directory) || File.Exists(directory))
            throw new IOException("Queue diagnostics require a new output directory.");
        var sourceHash = Hash(source);
        var info = SessionFile.ReadInfo(source);
        long frequency;
        long startQpc;
        using (var connection = new SqliteConnection(new SqliteConnectionStringBuilder
        {
            DataSource = source, Mode = SqliteOpenMode.ReadOnly, Pooling = false
        }.ToString()))
        {
            connection.Open();
            using var command = connection.CreateCommand();
            command.CommandText = "SELECT QpcFrequency, StartQpc FROM SessionInfo";
            using var reader = command.ExecuteReader();
            if (!reader.Read() || reader.IsDBNull(0) || reader.IsDBNull(1) || reader.GetInt64(0) <= 0)
                throw new InvalidDataException("A timed queue replay requires the recording's QPC clock.");
            frequency = reader.GetInt64(0);
            startQpc = reader.GetInt64(1);
        }
        var events = new List<ProfilerEvent>();
        SessionFile.Read(source, item =>
        {
            if (events.Count >= 1_000_000) throw new InvalidDataException("Queue diagnostics are limited to one million events.");
            events.Add(item);
        });
        if (events.Count == 0) throw new InvalidDataException("The recording contains no events.");
        Directory.CreateDirectory(directory);
        var results = new List<object>();
        foreach (var mode in new[] { "counter-only", "writer-lossless", "writer-paced-live" })
        {
            var samples = new List<object>();
            using var writer = mode == "counter-only" ? null : SessionWriter.Create(
                Path.Combine(directory, mode + ".sqlite"),
                new(info.ProcessId, info.ProcessName, null, frequency, startQpc, SessionCoverage.Trace));
            var counter = new EventCounter();
            IEventProcessor[] processors = writer is null ? [counter] : [counter, writer];
            using var pipeline = mode == "writer-paced-live" ? new EventPipeline(processors) : EventPipeline.ForTrace(processors);
            var clock = Stopwatch.StartNew();
            var producer = Task.Run(() =>
            {
                var started = Stopwatch.GetTimestamp();
                foreach (var item in events)
                {
                    if (mode == "writer-paced-live")
                    {
                        var due = (item.TimestampQpc - events[0].TimestampQpc) * 1000.0 / frequency;
                        while (due - Stopwatch.GetElapsedTime(started).TotalMilliseconds is var remaining && remaining > 0)
                            Thread.Sleep((int)Math.Clamp(remaining, 1, 10));
                        pipeline.Post(item);
                    }
                    else if (!pipeline.PostBlocking(item))
                        throw new InvalidOperationException("The diagnostic processing pipeline failed.", pipeline.Failure);
                }
                pipeline.Dispose();
                writer?.Close("Offline queue diagnostic", droppedByProfiler: pipeline.DroppedEvents);
            });
            while (!producer.IsCompleted)
            {
                samples.Add(new { seconds = clock.Elapsed.TotalSeconds, pipeline = pipeline.Diagnostics, writer = writer?.Status });
                await Task.WhenAny(producer, Task.Delay(100));
            }
            await producer;
            clock.Stop();
            if (pipeline.Failure is not null || writer?.Failure is not null)
                throw new InvalidOperationException("Queue replay failed.", pipeline.Failure ?? writer?.Failure);
            var result = new
            {
                mode, seconds = clock.Elapsed.TotalSeconds, pipeline = pipeline.Diagnostics, writer = writer?.Status, samples
            };
            results.Add(result);
            Console.WriteLine(JsonSerializer.Serialize(new { mode, result.seconds, result.pipeline, result.writer }));
        }
        if (Hash(source) != sourceHash) throw new IOException("The source recording changed during diagnosis.");
        var report = new
        {
            kind = "offline-profiler-queue-diagnostic",
            source, sourceSha256 = sourceHash, sourceEvents = events.Count,
            sourceDroppedEvents = info.DroppedByProfiler, sourceLostEvents = info.LostByEtw,
            sourceEventSpanSeconds = (events[^1].TimestampQpc - events[0].TimestampQpc) / (double)frequency,
            limitations = new[]
            {
                "Not a valid app baseline or a replacement for the failed capture.",
                "Only retained events are replayed; original ETW delivery timing and CPU-stack writes are not reproduced.",
                "Paced replay uses event timestamps with millisecond scheduling, not original ETW callback timing."
            },
            results
        };
        var reportPath = Path.Combine(directory, "queue-diagnostics.json");
        File.WriteAllText(reportPath, JsonSerializer.Serialize(report, new JsonSerializerOptions { WriteIndented = true }));
        Console.WriteLine(reportPath);
    }

    private static string Hash(string path)
    {
        using var stream = File.OpenRead(path);
        return Convert.ToHexString(SHA256.HashData(stream));
    }

    private static ProfilerEvent Event() => new(1, 0, 1, 1, XamlProvider.Xaml, true, "QueueTest", 0, []);

    private static void Require(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }

    private sealed class GateProcessor(ManualResetEventSlim entered, ManualResetEventSlim release) : IEventProcessor
    {
        public void Process(long sequence, ProfilerEvent profilerEvent)
        {
            entered.Set();
            if (!release.Wait(TimeSpan.FromSeconds(10))) throw new TimeoutException("Queue test gate timed out.");
        }
    }
}

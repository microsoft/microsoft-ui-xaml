using System.Text.Json;
using WinUIProfiler.Cli;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;

internal static class LiveCliTests
{
    internal static async Task RunAsync()
    {
        string[] valid = ["live", "--pid", "123", "--trust-local-binaries", "--json"];
        foreach (var invalid in new[]
        {
            new[] { "live", "--pid", "123" },
            valid.Concat(["--exe", @"C:\app.exe"]).ToArray(),
            valid.Concat(["--duration", "NaN"]).ToArray(),
            valid.Concat(["--snapshot-interval", "0"]).ToArray(),
            valid.Concat(["--private-runtime", @"C:\runtime"]).ToArray(),
            valid.Concat(["--creation-time", "-1"]).ToArray(),
            valid.Concat(["--unknown"]).ToArray(),
            valid.Concat(["--json"]).ToArray(),
            new[] { "snapshot", "capture", "--pid", "123", "--trust-local-binaries" }
        })
        {
            var output = new StringWriter();
            var errors = new StringWriter();
            var result = await LiveCommands.RunAsync(invalid, new StringReader(""), output, errors, CancellationToken.None);
            Check(result == 2 && errors.ToString().Length > 0, "Invalid live arguments must fail before native activity.");
        }

        var parsed = LiveCommands.Parse(valid.Concat(["--creation-time", "123456", "--duration", "0.5"]).ToArray());
        Check(parsed.Connection.ExpectedCreationTimeFileTime == 123456 && parsed.Duration == 0.5,
            "Creation FILETIME and fractional duration must be preserved.");
        var launched = LiveCommands.Parse(["live", "--exe", Path.Combine(AppContext.BaseDirectory, "app.exe"),
            "--trust-local-binaries", "--arguments", "--app-flag value"]);
        Check(launched.Connection.Arguments == "--app-flag value" && launched.Connection.IsolateLaunchedStandardHandles,
            "App arguments must remain opaque and CLI-owned launches must isolate their standard handles.");
        var settings = LiveCommands.Parse(valid.Concat(["--interactive"]).ToArray());
        var fake = new FakeConnection();
        var text = new StringWriter();
        var connects = 0;
        var code = await LiveCommands.RunSessionAsync(settings, new StringReader("status\nsnapshot\nstop\n"),
            text, new StringWriter(), CancellationToken.None, (_, _, _) =>
            {
                connects++;
                return Task.FromResult<LiveCommands.ILiveConnection>(fake);
            });
        Check(code == 0 && connects == 1 && fake.Captures == 2 && fake.Disposed,
            "Interactive status/snapshot/stop must reuse one connection and dispose it.");
        var records = text.ToString().Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries)
            .Select(line => JsonDocument.Parse(line)).ToArray();
        try
        {
            Check(records.Last().RootElement.GetProperty("type").GetString() == "summary" &&
                records.Count(record => record.RootElement.GetProperty("type").GetString() == "snapshot") == 2 &&
                records.Last().RootElement.GetProperty("data").GetProperty("snapshotPaths").GetArrayLength() == 2,
                "Machine output must be NDJSON with both snapshots and a final summary.");
        }
        finally { foreach (var record in records) record.Dispose(); }

        fake = new FakeConnection { EtwError = "ETW permission denied" };
        text = new StringWriter();
        code = await LiveCommands.RunSessionAsync(settings, new StringReader("stop\n"), text, new StringWriter(),
            CancellationToken.None, (_, _, _) => Task.FromResult<LiveCommands.ILiveConnection>(fake));
        Check(code == 3 && text.ToString().Contains("unavailable") && fake.Disposed,
            "Partial ETW failure must remain visible and return an incomplete result.");

        fake = new FakeConnection { SnapshotError = new IOException("Native capture failed") };
        code = await LiveCommands.RunSessionAsync(settings, new StringReader("stop\n"), new StringWriter(), new StringWriter(),
            CancellationToken.None, (_, _, _) => Task.FromResult<LiveCommands.ILiveConnection>(fake));
        Check(code == 3 && fake.Disposed, "Native capture failure cannot become a successful fallback.");

        fake = new FakeConnection { SnapshotError = new InvalidDataException("Malformed native snapshot") };
        code = await LiveCommands.RunSessionAsync(settings, new StringReader("stop\n"), new StringWriter(), new StringWriter(),
            CancellationToken.None, (_, _, _) => Task.FromResult<LiveCommands.ILiveConnection>(fake));
        Check(code == 3 && fake.Disposed, "InvalidDataException must be reported as an operational native-protocol failure.");

        using var cancel = new CancellationTokenSource();
        fake = new FakeConnection { AfterCapture = () => cancel.Cancel() };
        text = new StringWriter();
        code = await LiveCommands.RunSessionAsync(settings, new StringReader(""), text, new StringWriter(), cancel.Token,
            (_, _, _) => Task.FromResult<LiveCommands.ILiveConnection>(fake));
        Check(code == 4 && fake.Disposed && text.ToString().Contains("recording.sqlite"),
            "Cancellation must dispose the connection and preserve the final recording path.");

        fake = new FakeConnection();
        code = await LiveCommands.RunSessionAsync(settings with { Interactive = false, Duration = 0.02 },
            new StringReader(""), new StringWriter(), new StringWriter(), CancellationToken.None,
            (_, _, _) => Task.FromResult<LiveCommands.ILiveConnection>(fake)).WaitAsync(TimeSpan.FromSeconds(5));
        Check(code == 0 && fake.Disposed && fake.Captures == 1, "A duration limit must stop cleanly after the initial snapshot.");

        fake = new FakeConnection();
        var exactPath = Path.Combine(AppContext.BaseDirectory, "requested-snapshot.json");
        code = await LiveCommands.RunSessionAsync(settings with { Capture = true, OutputPath = exactPath, Interactive = false },
            new StringReader(""), new StringWriter(), new StringWriter(), CancellationToken.None,
            (_, _, _) => Task.FromResult<LiveCommands.ILiveConnection>(fake));
        Check(code == 0 && fake.ExactPath == exactPath && fake.Captures == 1 && fake.Disposed,
            "One-shot capture must request the caller's exact output path through the same authenticated session.");

        foreach (var help in new[] { new[] { "live", "--help" }, new[] { "targets", "--help" },
            new[] { "snapshot", "capture", "--help" } })
            Check(await LiveCommands.RunAsync(help, new StringReader(""), new StringWriter(), new StringWriter(),
                CancellationToken.None) == 0, "Live command families must expose help without requiring Windows or target consent.");
        Console.WriteLine("Passed: live CLI validation, interactive reuse, NDJSON, partial failures and cancellation.");
    }

    private static void Check(bool value, string message)
    {
        if (!value) throw new InvalidOperationException(message);
    }

    private sealed class FakeConnection : LiveCommands.ILiveConnection
    {
        public string State => "Connected";
        public string Detail => "";
        public SessionScope? Scope { get; } = new(Guid.NewGuid(), new(123, 123456, Guid.NewGuid()));
        public string RuntimePath => @"C:\runtime\Microsoft.UI.Xaml.dll";
        public DateTime? LastHelloUtc => DateTime.UtcNow;
        public Task Completion { get; } = new TaskCompletionSource().Task;
        public MetricHistory? History { get; }
        public EtwStatus? Etw => new(false, 0, 0, 0, 0, 0);
        public string? EtwError { get; init; }
        public RecordingStatus? Recording => new(@"C:\recording.sqlite", 0, 0, 1000000, null);
        public string? RecordingError => null;
        public Exception? SnapshotError { get; init; }
        public Action? AfterCapture { get; init; }
        public int Captures { get; private set; }
        public bool Disposed { get; private set; }
        public string? ExactPath { get; private set; }
        internal FakeConnection() => History = new(Scope!.Process, 1000000);
        public Task<SavedTreeSnapshot> TakeSnapshotAsync(string directory, CancellationToken token)
        {
            token.ThrowIfCancellationRequested();
            if (SnapshotError is not null) return Task.FromException<SavedTreeSnapshot>(SnapshotError);
            Captures++;
            AfterCapture?.Invoke();
            return Task.FromResult(new SavedTreeSnapshot(Path.Combine(directory, "snapshot.json"), Guid.NewGuid(), 1, 1, 1, 1));
        }
        public Task<SavedTreeSnapshot> TakeSnapshotToFileAsync(string path, CancellationToken token)
        {
            ExactPath = path;
            return TakeSnapshotAsync(Path.GetDirectoryName(path)!, token);
        }
        public ValueTask DisposeAsync()
        {
            Disposed = true;
            return ValueTask.CompletedTask;
        }
    }
}

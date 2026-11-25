using System.Diagnostics;
using System.Text.Json;
using WinUIProfiler.Cli;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Launch;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;

internal static class CliLiveTests
{
    internal static async Task RunAsync()
    {
        var failures = new List<Exception>();
        await Check("packaged working directory", () =>
        {
            try
            {
                LiveCommands.Parse(["live", "--aumid", "Test!App", "--working-directory", Path.GetTempPath(),
                    "--trust-local-binaries"]);
            }
            catch (ArgumentException) { return Task.CompletedTask; }
            throw new InvalidOperationException("Packaged activation must reject an ignored working directory.");
        });
        await Check("default markers", () =>
        {
            var path = MarkerConfig.UserPath;
            var original = File.Exists(path) ? File.ReadAllBytes(path) : null;
            var previousError = Console.Error;
            using var error = new StringWriter();
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(path)!);
                File.WriteAllText(path, "{invalid");
                Console.SetError(error);
                var options = LiveCommands.Parse(["live", "--exe", Path.Combine(Path.GetTempPath(), "target.exe"),
                    "--trust-local-binaries"]);
                Require(options.Connection.MarkerProviders!.Count == MarkerConfig.Default.Providers.Count &&
                    error.ToString().Contains("default marker configuration"),
                    "Invalid implicit markers must warn and use the default providers.");
                try
                {
                    LiveCommands.Parse(["live", "--exe", Path.Combine(Path.GetTempPath(), "target.exe"),
                        "--trust-local-binaries", "--markers", path]);
                }
                catch (MarkerConfigException) { return Task.CompletedTask; }
                throw new InvalidOperationException("Invalid explicit markers must remain an error.");
            }
            finally
            {
                Console.SetError(previousError);
                if (original is null) File.Delete(path);
                else File.WriteAllBytes(path, original);
            }
        });
        await Check("initial snapshot waits, manual snapshot does not", async () =>
        {
            var connection = new Connection();
            var waiting = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
            var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
            connection.InitialSnapshotDelay = async token =>
            {
                waiting.TrySetResult();
                using var linked = CancellationTokenSource.CreateLinkedTokenSource(token, connection.Stopped.Token);
                await release.Task.WaitAsync(linked.Token);
            };
            var run = Run(connection, Options() with { Interactive = true }, 0, 2,
                input: new StringReader("snapshot\nstop\n"));
            try
            {
                await waiting.Task.WaitAsync(TimeSpan.FromSeconds(3));
                Require(connection.CaptureCount == 0 && !run.IsCompleted,
                    "The automatic initial snapshot must not reach capture before its post-connection delay completes.");
            }
            finally
            {
                release.TrySetResult();
                await run;
            }
            Require(connection.InitialSnapshotCount == 1,
                "A subsequent manual snapshot must not repeat the initial post-connection delay.");
        });
        foreach (var external in new[] { false, true })
            await Check($"initial post-connection delay {(external ? "cancellation" : "duration")}", async () =>
            {
                using var stop = new CancellationTokenSource();
                var connection = new Connection();
                var waiting = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
                connection.InitialSnapshotDelay = async token =>
                {
                    waiting.TrySetResult();
                    using var linked = CancellationTokenSource.CreateLinkedTokenSource(token, connection.Stopped.Token);
                    await Task.Delay(Timeout.Infinite, linked.Token);
                };
                var run = Run(connection, Options() with { Duration = external ? null : 0.1 }, external ? 4 : 3, 0, stop.Token);
                try
                {
                    await waiting.Task.WaitAsync(TimeSpan.FromSeconds(3));
                    if (external) stop.Cancel();
                    await run;
                    Require(connection.CaptureCount == 0,
                        "Stopping during the post-connection delay must not request a target snapshot.");
                }
                finally
                {
                    stop.Cancel();
                    await run;
                }
            });
        await Check("explicit snapshot bypasses post-connection delay", async () =>
        {
            var connection = new Connection
            {
                InitialSnapshotDelay = _ => throw new InvalidOperationException("An explicit capture must not wait after connecting.")
            };
            await Run(connection, Options() with { Capture = true, OutputPath = "capture.json" }, 0, 1);
            Require(connection.InitialSnapshotCount == 0, "An explicit snapshot must use the immediate capture path.");
        });
        await Check("target exit", async () =>
        {
            var connection = new Connection();
            connection.Capture = (_, _) =>
            {
                connection.Ended.TrySetResult();
                return Task.FromResult(Saved());
            };
            await Run(connection, Options(), 0, 1);
        });
        await Check("interactive commands", async () =>
        {
            await Run(new Connection(), Options() with { Interactive = true }, 0, 2,
                input: new StringReader("status\nsnapshot\nstop\n"));
        });
        await Check("external cancellation", async () =>
        {
            using var stop = new CancellationTokenSource();
            var connection = new Connection();
            connection.Capture = (_, _) =>
            {
                stop.Cancel();
                return Task.FromResult(Saved());
            };
            await Run(connection, Options(), 4, 1, stop.Token);
        });
        await Check("duration during initial snapshot", async () =>
        {
            var connection = new Connection();
            connection.Capture = async (_, token) =>
            {
                using var linked = CancellationTokenSource.CreateLinkedTokenSource(token, connection.Stopped.Token);
                await Task.Delay(Timeout.Infinite, linked.Token);
                return Saved();
            };
            await Run(connection, Options() with { Duration = 0.05 }, 3, 0);
        });
        await Check("duration during later snapshot", async () =>
        {
            var connection = new Connection();
            connection.Capture = async (count, token) =>
            {
                if (count == 1) return Saved();
                using var linked = CancellationTokenSource.CreateLinkedTokenSource(token, connection.Stopped.Token);
                try { await Task.Delay(Timeout.Infinite, linked.Token); }
                catch (OperationCanceledException)
                {
                    if (!connection.Disposing) connection.DisposeError = new IOException("Heartbeat used a canceled TAP pipe.");
                    throw;
                }
                return Saved();
            };
            await Run(connection, Options() with { Duration = 0.3, SnapshotInterval = 0.001 }, 0, 1);
        });
        foreach (var unavailable in new[] { "etw", "recording", "partial", "loss" })
            await Check(unavailable + " is incomplete", async () =>
            {
                var connection = new Connection();
                switch (unavailable)
                {
                    case "etw": connection.EtwError = "ETW unavailable"; break;
                    case "recording": connection.Recording = null; break;
                    case "partial": connection.Recording = connection.Recording! with { CompleteFromSeconds = 1 }; break;
                    case "loss": connection.Etw = connection.Etw! with { LostEvents = 1 }; break;
                }
                await Run(connection, Options() with { Interactive = true }, 3, 1, input: new StringReader("stop\n"));
            });
        await Check("sampling failure", async () =>
        {
            var connection = new Connection { DisposeError = new IOException("Sampling ended unexpectedly.") };
            connection.Capture = (_, _) =>
            {
                connection.Ended.TrySetResult();
                return Task.FromResult(Saved());
            };
            await Run(connection, Options(), 3, 1);
        });
        await Check("disposal failure", async () =>
        {
            await Run(new Connection { DisposeError = new IOException("Could not close recording.") },
                Options() with { Interactive = true }, 3, 1, input: new StringReader("stop\n"));
        });
        await Check("startup output failure", async () =>
        {
            var connection = new Connection();
            var connected = new TaskCompletionSource<LiveCommands.ILiveConnection>(TaskCreationOptions.RunContinuationsAsynchronously);
            using var output = new CallbackWriter(() =>
            {
                connected.TrySetResult(connection);
                throw new IOException("Broken output pipe.");
            });
            using var error = new StringWriter();
            try
            {
                await LiveCommands.RunSessionAsync(Options(), TextReader.Null, output, error, default,
                    (_, progress, _) =>
                    {
                        progress.Report(new("Connecting", "test"));
                        return connected.Task;
                    }).WaitAsync(TimeSpan.FromSeconds(10));
            }
            catch (IOException) { }
            Require(connection.DisposeCount == 1, "A connection established during a failed progress write must be disposed.");
        });
        foreach (var startup in new[] { false, true })
        foreach (var external in new[] { false, true })
            await Check($"blocked {(startup ? "startup" : "snapshot")} output {(external ? "cancellation" : "duration")}", async () =>
            {
                var connection = new Connection();
                using var stop = new CancellationTokenSource();
                var entered = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
                var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
                var connected = new TaskCompletionSource<LiveCommands.ILiveConnection>(TaskCreationOptions.RunContinuationsAsynchronously);
                using var output = new CallbackWriter(() =>
                {
                    connected.TrySetResult(connection);
                    entered.TrySetResult();
                    release.Task.GetAwaiter().GetResult();
                });
                using var error = new StringWriter();
                var run = Task.Run(() => LiveCommands.RunSessionAsync(Options() with { Duration = external ? null : 0.1 },
                    TextReader.Null, output, error, stop.Token, (_, progress, _) =>
                    {
                        if (!startup) return Task.FromResult<LiveCommands.ILiveConnection>(connection);
                        progress.Report(new("Connecting", "test"));
                        return connected.Task;
                    }));
                try
                {
                    await entered.Task.WaitAsync(TimeSpan.FromSeconds(3));
                    if (external) stop.Cancel();
                    await connection.Disposed.Task.WaitAsync(TimeSpan.FromSeconds(3));
                }
                finally
                {
                    release.TrySetResult();
                    await run.WaitAsync(TimeSpan.FromSeconds(10));
                }
                Require(connection.DisposeCount == 1, "Backpressure must not keep the connection alive.");
                using var summary = Summary(output.ToString());
                Require(summary.RootElement.GetProperty("exitCode").GetInt32() == (external ? 4 : startup ? 3 : 0),
                    "After stdout drains, its final summary must preserve the stop reason.");
            });
        await Check("capture output timeout", async () =>
        {
            var connection = new Connection();
            var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
            using var output = new CallbackWriter(() => release.Task.GetAwaiter().GetResult());
            using var error = new StringWriter();
            var run = LiveCommands.RunSessionAsync(Options() with { Capture = true, OutputPath = "capture.json" },
                TextReader.Null, output, error, default, (_, _, _) => Task.FromResult<LiveCommands.ILiveConnection>(connection));
            try { await connection.Disposed.Task.WaitAsync(TimeSpan.FromSeconds(7)); }
            finally { release.TrySetResult(); await run.WaitAsync(TimeSpan.FromSeconds(10)); }
            Require(await run == 3 && connection.DisposeCount == 1,
                "A one-shot capture needs bounded output and disposal without a duration or external cancellation.");
        });
        await Check("late physical output failure", async () =>
        {
            var connection = new Connection();
            var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
            var writes = 0;
            using var output = new CallbackWriter(() =>
            {
                if (Interlocked.Increment(ref writes) != 1) return;
                release.Task.GetAwaiter().GetResult();
                throw new IOException("Delayed physical write failed.");
            });
            using var error = new StringWriter();
            var run = LiveCommands.RunSessionAsync(Options() with { Duration = 0.1 }, TextReader.Null,
                output, error, default, (_, _, _) => Task.FromResult<LiveCommands.ILiveConnection>(connection));
            try { await connection.Disposed.Task.WaitAsync(TimeSpan.FromSeconds(3)); }
            finally { release.TrySetResult(); await run.WaitAsync(TimeSpan.FromSeconds(10)); }
            Require(await run == 3, "A physical write failure after cancellation must not be reported as success.");
        });
        if (failures.Count > 0) throw new AggregateException(failures);

        async Task Check(string name, Func<Task> test)
        {
            try { await test(); Console.WriteLine($"Passed: live CLI {name}."); }
            catch (Exception error) { failures.Add(new InvalidOperationException(name, error)); Console.WriteLine($"FAILED: live CLI {name}: {error.Message}"); }
        }
    }

    internal static async Task RunSnapshotDelayIntegrationAsync(string executable)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Snapshot timing integration requires Windows.");
        var applicationDirectory = Path.GetDirectoryName(executable)!;
        var directory = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-SnapshotDelay-{Guid.NewGuid():N}");
        using var target = Process.Start(new ProcessStartInfo(executable)
        {
            UseShellExecute = false,
            WorkingDirectory = applicationDirectory
        }) ?? throw new IOException("The snapshot timing test target did not start.");
        try
        {
            using var timeout = new CancellationTokenSource(TimeSpan.FromMinutes(2));
            var options = new TargetConnectionOptions(TargetLaunchKind.Attach,
                target.Id.ToString(System.Globalization.CultureInfo.InvariantCulture), "", null,
                Path.Combine(applicationDirectory, "ConnectionPayloads"), true,
                ExpectedCreationTimeFileTime: target.StartTime.ToFileTimeUtc());
            foreach (var scenario in new[] { "newly started target", "already-running target" })
            {
                await using var connection = await TargetConnection.ConnectAsync(options, null, timeout.Token);
                var connected = Stopwatch.GetTimestamp();
                var saved = await connection.TakeInitialSnapshotAsync(directory, timeout.Token);
                using var snapshot = JsonDocument.Parse(await File.ReadAllTextAsync(saved.Path, timeout.Token));
                var captureStarted = snapshot.RootElement.GetProperty("startQpc").GetInt64();
                var seconds = (captureStarted - connected) / (double)Stopwatch.Frequency;
                Require(seconds >= 1.95,
                    $"The {scenario}'s initial snapshot started only {seconds:F3}s after connection; expected a full two-second wait.");
                Console.WriteLine($"Passed: {scenario} initial snapshot started {seconds:F3}s after connection.");
            }
        }
        finally
        {
            try
            {
                target.Refresh();
                if (!target.HasExited)
                {
                    if (!target.CloseMainWindow()) throw new IOException($"Could not close the test target (PID {target.Id}).");
                    using var shutdown = new CancellationTokenSource(TimeSpan.FromSeconds(15));
                    await target.WaitForExitAsync(shutdown.Token);
                }
            }
            finally
            {
                if (Directory.Exists(directory)) Directory.Delete(directory, recursive: true);
            }
        }
    }

    private static LiveCommands.Options Options() => new(false, true, false, null, null, null, Path.GetTempPath(),
        new(TargetLaunchKind.Unpackaged, Path.Combine(Path.GetTempPath(), "target.exe"), "", null, Path.GetTempPath(), true));

    private static SavedTreeSnapshot Saved() => new("capture.json", Guid.NewGuid(), 1, 1, 1, 1);

    private static async Task Run(Connection connection, LiveCommands.Options options, int code, int snapshots,
        CancellationToken token = default, TextReader? input = null)
    {
        using var output = new StringWriter();
        using var error = new StringWriter();
        var result = await LiveCommands.RunSessionAsync(options, input ?? TextReader.Null, output, error, token,
            (_, _, _) => Task.FromResult<LiveCommands.ILiveConnection>(connection)).WaitAsync(TimeSpan.FromSeconds(10));
        Require(connection.DisposeCount == 1, "Every established session must be disposed exactly once.");
        using var summary = Summary(output.ToString());
        var data = summary.RootElement;
        Require(result == code && data.GetProperty("exitCode").GetInt32() == code &&
            data.GetProperty("snapshots").GetInt32() == snapshots && !data.GetProperty("targetTerminated").GetBoolean(),
            $"Expected exit {code}, {snapshots} snapshots and an untouched target; got {result}: {output} {error}");
    }

    private static JsonDocument Summary(string output)
    {
        var lines = output.Split('\n', StringSplitOptions.RemoveEmptyEntries);
        foreach (var line in lines) { using var valid = JsonDocument.Parse(line); }
        using var last = JsonDocument.Parse(lines[^1]);
        Require(last.RootElement.GetProperty("type").GetString() == "summary", "The last NDJSON record must be a summary.");
        return JsonDocument.Parse(last.RootElement.GetProperty("data").GetRawText());
    }

    private static void Require(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }

    private sealed class Connection : LiveCommands.ILiveConnection
    {
        public string State => "Connected";
        public string Detail => "";
        public SessionScope? Scope => null;
        public string RuntimePath => "runtime.dll";
        public DateTime? LastHelloUtc => null;
        public TaskCompletionSource Ended { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);
        public Task Completion => Ended.Task;
        public MetricHistory? History { get; } = new(new(42, 100, Guid.NewGuid()), 1000);
        public EtwStatus? Etw { get; set; } = new(true, 1, 0, 0, 0, 0);
        public string? EtwError { get; set; }
        public RecordingStatus? Recording { get; set; } = new("session.sqlite", 1, 100, 1000, null);
        public string? RecordingError => null;
        public CancellationTokenSource Stopped { get; } = new();
        public TaskCompletionSource Disposed { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);
        public Exception? DisposeError { get; set; }
        public bool Disposing { get; private set; }
        public int DisposeCount { get; private set; }
        private int captures;
        public int CaptureCount => captures;
        public int InitialSnapshotCount { get; private set; }
        public Func<CancellationToken, Task> InitialSnapshotDelay { get; set; } = _ => Task.CompletedTask;
        public Func<int, CancellationToken, Task<SavedTreeSnapshot>> Capture { get; set; } = (_, _) => Task.FromResult(Saved());
        public async Task<SavedTreeSnapshot> TakeInitialSnapshotAsync(string directory, CancellationToken token)
        {
            InitialSnapshotCount++;
            await InitialSnapshotDelay(token);
            return await TakeSnapshotAsync(directory, token);
        }
        public Task<SavedTreeSnapshot> TakeSnapshotAsync(string directory, CancellationToken token) => Capture(++captures, token);
        public Task<SavedTreeSnapshot> TakeSnapshotToFileAsync(string path, CancellationToken token) => Capture(++captures, token);
        public ValueTask DisposeAsync()
        {
            DisposeCount++;
            Disposing = true;
            Stopped.Cancel();
            Disposed.TrySetResult();
            return DisposeError is { } error ? ValueTask.FromException(error) : ValueTask.CompletedTask;
        }
    }

    private sealed class CallbackWriter(Action callback) : StringWriter
    {
        public override Task WriteLineAsync(ReadOnlyMemory<char> buffer, CancellationToken cancellationToken = default)
        {
            callback();
            return base.WriteLineAsync(buffer, cancellationToken);
        }
    }
}

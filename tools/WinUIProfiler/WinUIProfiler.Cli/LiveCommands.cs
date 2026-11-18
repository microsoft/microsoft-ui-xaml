using System.Collections.Concurrent;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.ExceptionServices;
using System.Runtime.Versioning;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Launch;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.Cli;

internal static class LiveCommands
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        Converters = { new JsonStringEnumConverter(JsonNamingPolicy.CamelCase) }
    };

    internal sealed record Options(bool Capture, bool Json, bool Interactive, double? Duration, double? SnapshotInterval,
        string? OutputPath, string SnapshotDirectory, TargetConnectionOptions Connection);

    internal static async Task<int> RunAsync(string[] args, TextReader input, TextWriter output, TextWriter error,
        CancellationToken cancellationToken)
    {
        var json = args.Contains("--json", StringComparer.Ordinal);
        try
        {
            if (args is ["live", "--help"] or ["targets", "--help"] or ["targets", "processes", "--help"] or
                ["targets", "apps", "--help"] or ["snapshot", "capture", "--help"])
            {
                await output.WriteLineAsync(
                    "live (--pid PID | --exe FULLPATH | --aumid AUMID) --trust-local-binaries [options]\n" +
                    "snapshot capture <same target options> --out NEW.json [--json]\n" +
                    "targets processes|apps [--json]\n" +
                    "Target options: --arguments TEXT --working-directory PATH --payload-directory PATH\n" +
                    "  --creation-time FILETIME --endpoint NUMBER --session-directory PATH --markers JSON\n" +
                    "Private runtime: --private-runtime PATH --runtime-source PATH --launch-directory PATH\n" +
                    "  [--redirect-controls] [--launcher-directory PATH]\n" +
                    "Live options: --duration SECONDS --snapshot-interval SECONDS --snapshot-directory PATH\n" +
                    "  --interactive (stdin: status, snapshot, stop) --json (NDJSON)\n" +
                    "Live captures an initial snapshot automatically. Stopping never terminates the target.\n" +
                    "Exit codes: 0 complete, 2 usage, 3 failed/incomplete, 4 canceled.").ConfigureAwait(false);
                return 0;
            }
            if (args is ["targets", ..])
            {
                var options = CommandArguments.Parse(args.Skip(1), [], ["--json"]);
                options.RequirePositionals(1);
                var kind = options.Positionals[0];
                if (kind is not ("processes" or "apps")) throw new ArgumentException("Use targets processes or targets apps.");
                if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Live target discovery requires Windows.");
                if (kind == "processes")
                {
                    var candidates = TargetProcessDiscovery.Discover(cancellationToken);
                    if (options.Has("--json"))
                        await WriteAsync(output, true, "targets", candidates, cancellationToken).ConfigureAwait(false);
                    else
                    {
                        await output.WriteLineAsync("PID\tCREATION FILETIME\tNAME\tERROR").ConfigureAwait(false);
                        foreach (var candidate in candidates)
                            await output.WriteLineAsync($"{candidate.ProcessId}\t{candidate.CreationTimeFileTime}\t{candidate.Name}\t{candidate.Error}").ConfigureAwait(false);
                    }
                }
                else
                {
                    var candidates = await DiscoverAppsAsync(cancellationToken).ConfigureAwait(false);
                    if (options.Has("--json"))
                        await WriteAsync(output, true, "targets", candidates, cancellationToken).ConfigureAwait(false);
                    else
                    {
                        await output.WriteLineAsync("AUMID\tNAME").ConfigureAwait(false);
                        foreach (var candidate in candidates)
                            await output.WriteLineAsync($"{candidate.AppId}\t{candidate.Name}").ConfigureAwait(false);
                    }
                }
                return 0;
            }

            var parsed = Parse(args, error);
            if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Live profiling requires Windows.");
            return await RunSessionAsync(parsed, input, output, error, cancellationToken,
                async (settings, progress, token) =>
                {
                    if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Live profiling requires Windows.");
                    return new LiveConnection(await TargetConnection.ConnectAsync(settings, progress, token).ConfigureAwait(false));
                }).ConfigureAwait(false);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            await error.WriteLineAsync("Live operation canceled. The target was not terminated.").ConfigureAwait(false);
            await WriteAsync(output, json, "summary", new { state = "canceled", exitCode = 4 }, CancellationToken.None).ConfigureAwait(false);
            return 4;
        }
        catch (ArgumentException failure)
        {
            await error.WriteLineAsync(failure.Message).ConfigureAwait(false);
            return 2;
        }
        catch (Exception failure) when (failure is not OutOfMemoryException)
        {
            await error.WriteLineAsync(failure.Message).ConfigureAwait(false);
            await WriteAsync(output, json, "summary", new
            {
                state = "failed", exitCode = 3, error = failure.Message,
                recordingPath = failure.Data["RecordingPath"], etwError = failure.Data["EtwError"],
                recordingError = failure.Data["RecordingError"]
            }, CancellationToken.None).ConfigureAwait(false);
            return 3;
        }
    }

    internal static Options Parse(string[] args, TextWriter? error = null)
    {
        var capture = args.Length >= 2 && args[0] == "snapshot" && args[1] == "capture";
        if (!capture && (args.Length == 0 || args[0] != "live"))
            throw new ArgumentException("Use live, targets processes/apps, or snapshot capture.");
        var tokens = new List<string>();
        string? arguments = null;
        var hasArguments = false;
        for (var index = capture ? 2 : 1; index < args.Length; index++)
        {
            var name = args[index];
            if (name == "--arguments" || name.StartsWith("--arguments=", StringComparison.Ordinal))
            {
                if (hasArguments) throw new ArgumentException("--arguments was specified more than once.");
                hasArguments = true;
                if (name == "--arguments")
                {
                    if (++index == args.Length) throw new ArgumentException("--arguments needs a value.");
                    arguments = args[index];
                }
                else arguments = name["--arguments=".Length..];
            }
            else tokens.Add(name);
        }
        var options = CommandArguments.Parse(tokens,
            ["--pid", "--exe", "--aumid", "--working-directory", "--payload-directory",
                "--session-directory", "--snapshot-directory", "--markers", "--private-runtime", "--runtime-source",
                "--launch-directory", "--launcher-directory", "--duration", "--snapshot-interval", "--out",
                "--creation-time", "--endpoint"],
            ["--trust-local-binaries", "--json", "--interactive", "--redirect-controls"]);
        options.RequirePositionals(0);
        var selected = new[] { "--pid", "--exe", "--aumid" }.Where(options.Has).ToArray();
        if (selected.Length != 1) throw new ArgumentException("Specify exactly one of --pid, --exe or --aumid.");
        if (!options.Has("--trust-local-binaries"))
            throw new ArgumentException("Connecting requires explicit --trust-local-binaries consent.");
        var kind = selected[0] switch
        {
            "--pid" => TargetLaunchKind.Attach,
            "--exe" => TargetLaunchKind.Unpackaged,
            _ => TargetLaunchKind.Packaged
        };
        var target = options.Required(selected[0]);
        if (string.IsNullOrWhiteSpace(target)) throw new ArgumentException("The selected target cannot be empty.");
        if (kind == TargetLaunchKind.Attach && (!int.TryParse(target, NumberStyles.None, CultureInfo.InvariantCulture, out var pid) ||
            pid <= 0 || pid == Environment.ProcessId)) throw new ArgumentException("--pid needs another process's positive PID.");
        if (kind == TargetLaunchKind.Unpackaged && !Path.IsPathFullyQualified(target))
            throw new ArgumentException("--exe requires a full executable path.");
        if (kind == TargetLaunchKind.Attach && (hasArguments || options.Has("--working-directory")))
            throw new ArgumentException("--arguments and --working-directory require a launched target.");
        if (kind == TargetLaunchKind.Packaged && options.Has("--working-directory"))
            throw new ArgumentException("--working-directory requires an unpackaged target.");
        var duration = Seconds(options.Get("--duration"), "--duration");
        var interval = Seconds(options.Get("--snapshot-interval"), "--snapshot-interval");
        if (capture && (duration.HasValue || interval.HasValue || options.Has("--interactive") || options.Has("--snapshot-directory")))
            throw new ArgumentException("snapshot capture does not accept live session options.");
        var outputPath = capture ? Path.GetFullPath(options.Required("--out")) : null;
        if (!capture && options.Has("--out")) throw new ArgumentException("--out is only valid for snapshot capture.");
        if (outputPath is not null && (!string.Equals(Path.GetExtension(outputPath), ".json", StringComparison.OrdinalIgnoreCase) ||
            File.Exists(outputPath) || Directory.Exists(outputPath)))
            throw new ArgumentException("--out must name a new JSON file.");
        var launch = new List<string>();
        if (kind != TargetLaunchKind.Attach) launch.AddRange([kind == TargetLaunchKind.Packaged ? "--launch-aumid" : "--launch-exe", target]);
        foreach (var name in new[] { "--private-runtime", "--runtime-source", "--launch-directory" })
            if (options.Get(name) is { } value) launch.AddRange([name, value]);
        if (options.Has("--redirect-controls")) launch.Add("--redirect-controls");
        if (hasArguments && kind != TargetLaunchKind.Attach) launch.AddRange(["--arguments", arguments!]);
        var commandLine = ProfilerCommandLine.Parse(launch);
        if (options.Has("--launcher-directory") && commandLine.PrivateRuntime is null)
            throw new ArgumentException("--launcher-directory requires --private-runtime.");
        long? creation = null;
        if (options.Get("--creation-time") is { } text)
        {
            if (!long.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out var value) || value <= 0 || kind != TargetLaunchKind.Attach)
                throw new ArgumentException("--creation-time requires a positive process FILETIME and --pid.");
            creation = value;
        }
        var connection = new TargetConnectionOptions(kind, target, arguments ?? "",
            FullPath(options.Get("--working-directory")), Path.GetFullPath(options.Get("--payload-directory") ??
                Path.Combine(AppContext.BaseDirectory, "ConnectionPayloads")), true,
            checked((ushort)options.Integer("--endpoint", 1, 1, ushort.MaxValue)), creation,
            FullPath(options.Get("--session-directory")) ??
                Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WinUIProfiler", "Sessions"),
            CliApplication.LoadMarkers(options.Get("--markers"), error ?? Console.Error).Providers,
            commandLine.GetPrivateRuntime(Path.GetFullPath(options.Get("--launcher-directory") ??
                Path.Combine(AppContext.BaseDirectory, "NativeLauncher"))), IsolateLaunchedStandardHandles: true);
        if (OperatingSystem.IsWindows()) TargetConnection.ValidateOptions(connection);
        return new(capture, options.Has("--json"), options.Has("--interactive"), duration, interval, outputPath,
            Path.GetFullPath(options.Get("--snapshot-directory") ?? options.Get("--session-directory") ??
                Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WinUIProfiler", "Snapshots")), connection);
    }

    private static string? FullPath(string? path) => path is null ? null : Path.GetFullPath(path);

    private static double? Seconds(string? value, string name)
    {
        if (value is null) return null;
        if (!double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out var seconds) ||
            !double.IsFinite(seconds) || seconds < 0.001 || seconds > (uint.MaxValue - 1) / 1000d)
            throw new ArgumentException($"{name} requires seconds between 0.001 and {(uint.MaxValue - 1) / 1000d}.");
        return seconds;
    }

    internal static async Task<int> RunSessionAsync(Options options, TextReader input, TextWriter output, TextWriter error,
        CancellationToken cancellationToken,
        Func<TargetConnectionOptions, IProgress<ConnectionProgress>, CancellationToken, Task<ILiveConnection>> connect)
    {
        output = new SessionOutput(output);
        error = new SessionOutput(error);
        var progress = new ProgressQueue();
        ILiveConnection? connection = null;
        Task<ILiveConnection>? connecting = null;
        Task? startupProgress = null;
        Task? shutdown = null;
        var shutdownGate = new object();
        CancellationTokenRegistration shutdownRegistration = default;
        Exception? failure = null;
        var canceled = false;
        var snapshots = 0;
        var snapshotPaths = new List<string>();
        using var sessionStop = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        try
        {
            connecting = connect(options.Connection, progress, sessionStop.Token);
            while (!connecting.IsCompleted)
            {
                startupProgress = DrainProgressAsync(progress, options.Json, output, error, sessionStop.Token);
                await Task.WhenAny(connecting, startupProgress).ConfigureAwait(false);
                if (!startupProgress.IsCompleted) break;
                await startupProgress.ConfigureAwait(false);
                startupProgress = null;
                await Task.WhenAny(connecting, Task.Delay(100, CancellationToken.None)).ConfigureAwait(false);
            }
            connection = await connecting.ConfigureAwait(false);
            shutdownRegistration = sessionStop.Token.Register(() => { _ = ShutdownAsync(); });
            if (options.Duration is { } seconds) sessionStop.CancelAfter(TimeSpan.FromSeconds(seconds));
            if (startupProgress is not null) await startupProgress.ConfigureAwait(false);
            await DrainProgressAsync(progress, options.Json, output, error, sessionStop.Token).ConfigureAwait(false);
            await CaptureAsync().ConfigureAwait(false);
            if (!options.Capture)
            {
                var watch = Stopwatch.StartNew();
                var nextSnapshot = options.SnapshotInterval ?? double.PositiveInfinity;
                Task<string?>? command = options.Interactive ? ReadCommandAsync(input, sessionStop.Token) : null;
                long lastQpc = -1;
                while (!sessionStop.IsCancellationRequested && !connection.Completion.IsCompleted)
                {
                    await DrainProgressAsync(progress, options.Json, output, error, sessionStop.Token).ConfigureAwait(false);
                    // Copy history on the host loop, never write output from the native acquisition callback.
                    var series = connection.History!.Snapshot();
                    var latest = series[0].Points.LastOrDefault()?.TimestampQpc;
                    if (latest is { } timestamp && timestamp != lastQpc)
                    {
                        lastQpc = timestamp;
                        await WriteAsync(output, options.Json, "metrics", new
                        {
                            process = connection.Scope!.Process, timestampQpc = timestamp,
                            totalSamples = connection.History.TotalSamples,
                            values = series.Select(item => new { metric = item.Metric, unit = item.IsPercent ? "percent" : "MiB", sample = item.Points.LastOrDefault() })
                        }, sessionStop.Token).ConfigureAwait(false);
                    }
                    if (command?.IsCompleted == true)
                    {
                        var line = await command.ConfigureAwait(false);
                        if (line is null || line.Trim().Equals("stop", StringComparison.OrdinalIgnoreCase)) break;
                        switch (line.Trim().ToLowerInvariant())
                        {
                            case "status":
                                await WriteStatusAsync().ConfigureAwait(false);
                                break;
                            case "snapshot":
                                await CaptureAsync().ConfigureAwait(false);
                                break;
                            default:
                                await error.WriteLineAsync("Commands: status, snapshot, stop.".AsMemory(), sessionStop.Token).ConfigureAwait(false);
                                break;
                        }
                        command = ReadCommandAsync(input, sessionStop.Token);
                    }
                    if (watch.Elapsed.TotalSeconds >= nextSnapshot)
                    {
                        await CaptureAsync().ConfigureAwait(false);
                        nextSnapshot = watch.Elapsed.TotalSeconds + options.SnapshotInterval!.Value;
                    }
                    await Task.WhenAny(connection.Completion, (Task?)command ?? Task.Delay(100, sessionStop.Token),
                        Task.Delay(100, sessionStop.Token)).ConfigureAwait(false);
                }
            }
            async Task CaptureAsync()
            {
                sessionStop.Token.ThrowIfCancellationRequested();
                // Stop the connection first: canceling TAP alone can release a queued heartbeat onto a closed pipe.
                var saved = options.Capture
                    ? await connection.TakeSnapshotToFileAsync(options.OutputPath!, CancellationToken.None).ConfigureAwait(false)
                    : await connection.TakeSnapshotAsync(options.SnapshotDirectory, CancellationToken.None).ConfigureAwait(false);
                snapshots++;
                snapshotPaths.Add(saved.Path);
                await WriteAsync(output, options.Json, "snapshot", saved, sessionStop.Token).ConfigureAwait(false);
                await WriteStatusAsync().ConfigureAwait(false);
            }
            Task WriteStatusAsync() => WriteAsync(output, options.Json, "status", new
            {
                connection.State, connection.Detail, connection.Scope, connection.RuntimePath, connection.LastHelloUtc,
                etwState = EtwState(connection),
                etw = connection.Etw, etwError = connection.EtwError,
                recordingState = RecordingState(connection),
                recording = connection.Recording, recordingError = connection.RecordingError, snapshotPaths
            }, sessionStop.Token);
        }
        catch (OperationCanceledException) when (sessionStop.IsCancellationRequested)
        {
            canceled = cancellationToken.IsCancellationRequested;
        }
        catch (Exception exception) when (exception is not OutOfMemoryException)
        {
            failure = exception;
        }
        finally
        {
            await sessionStop.CancelAsync().ConfigureAwait(false);
            shutdownRegistration.Dispose();
            if (connection is null && connecting is not null)
            {
                try { connection = await connecting.ConfigureAwait(false); }
                catch (OperationCanceledException) when (sessionStop.IsCancellationRequested) { }
                catch (Exception exception) when (exception is not OutOfMemoryException)
                {
                    if (!ReferenceEquals(failure, exception))
                        failure = failure is null ? exception : new AggregateException(failure, exception);
                }
            }
            if (connection is not null)
            {
                try { await ShutdownAsync().ConfigureAwait(false); }
                catch (Exception exception) when (exception is not OutOfMemoryException)
                {
                    failure = failure is null ? exception : new AggregateException(failure, exception);
                }
            }
            if (startupProgress is not null)
            {
                try { await startupProgress.ConfigureAwait(false); }
                catch (OperationCanceledException) when (sessionStop.IsCancellationRequested) { }
                catch (Exception exception) when (exception is not OutOfMemoryException)
                {
                    if (!ReferenceEquals(failure, exception))
                        failure = failure is null ? exception : new AggregateException(failure, exception);
                }
            }
        }
        var etwError = connection?.EtwError ?? failure?.Data["EtwError"] as string;
        var recordingError = connection?.RecordingError ?? failure?.Data["RecordingError"] as string;
        var incomplete = failure is not null || etwError is not null || recordingError is not null ||
            connection?.Recording is null || connection?.Recording?.CompleteFromSeconds is not null || snapshots == 0 ||
            connection?.Etw is { LostEvents: > 0 } or { DroppedEvents: > 0 };
        canceled |= cancellationToken.IsCancellationRequested;
        var exitCode = canceled ? 4 : incomplete ? 3 : 0;
        using var finalOutput = new CancellationTokenSource(TimeSpan.FromSeconds(5));
        try
        {
            await DrainProgressAsync(progress, options.Json, output, error, finalOutput.Token).ConfigureAwait(false);
            foreach (var message in new[] { failure?.Message, etwError, recordingError }.OfType<string>())
                await error.WriteLineAsync(message.AsMemory(), finalOutput.Token).ConfigureAwait(false);
            await WriteAsync(output, options.Json, "summary", new
            {
                state = canceled ? "canceled" : incomplete ? "incomplete" : "completed", exitCode, snapshots, snapshotPaths,
                process = connection?.Scope?.Process, samples = connection?.History?.TotalSamples ?? 0,
                recordingPath = connection?.Recording?.Path ?? failure?.Data["RecordingPath"] as string,
                recording = connection?.Recording,
                etwState = EtwState(connection),
                etw = connection?.Etw, etwError,
                recordingState = RecordingState(connection),
                recordingError, error = failure?.Message, targetTerminated = false
            }, finalOutput.Token).ConfigureAwait(false);
        }
        catch (Exception exception) when (exception is not OutOfMemoryException)
        {
            Trace.TraceError($"Final live CLI output failed: {exception}");
            using var diagnostics = new CancellationTokenSource(TimeSpan.FromSeconds(1));
            try
            {
                await error.WriteLineAsync("Could not write the final live summary; the connection has been stopped.".AsMemory(),
                    diagnostics.Token).ConfigureAwait(false);
            }
            catch (Exception diagnosticError) when (diagnosticError is not OutOfMemoryException)
            {
                Trace.TraceError($"Live CLI diagnostic output failed: {diagnosticError}");
            }
            return canceled ? 4 : 3;
        }
        return exitCode;

        Task ShutdownAsync()
        {
            lock (shutdownGate) return shutdown ??= DisposeConnectionAsync();
        }

        async Task DisposeConnectionAsync() => await connection!.DisposeAsync().ConfigureAwait(false);
    }

    private static async Task DrainProgressAsync(ProgressQueue progress, bool json, TextWriter output, TextWriter error,
        CancellationToken token)
    {
        while (progress.Items.TryDequeue(out var item))
        {
            await error.WriteLineAsync($"{item.State}: {item.Detail}".AsMemory(), token).ConfigureAwait(false);
            if (json) await WriteAsync(output, true, "status", item, token).ConfigureAwait(false);
        }
    }

    private static Task WriteAsync(TextWriter output, bool json, string type, object data, CancellationToken token) =>
        output.WriteLineAsync((json
            ? JsonSerializer.Serialize(new { type, data }, JsonOptions)
            : $"{type}: {JsonSerializer.Serialize(data, JsonOptions)}").AsMemory(), token);

    // Console writers can block synchronously before returning a Task. Serialize writes off the acquisition loop,
    // and let cancellation stop the connection even when a pipe reader stops draining stdout or stderr.
    private sealed class SessionOutput(TextWriter inner) : TextWriter
    {
        private readonly SemaphoreSlim writes = new(1, 1);
        private ExceptionDispatchInfo? failure;
        public override Encoding Encoding => inner.Encoding;

        public override async Task WriteLineAsync(ReadOnlyMemory<char> buffer, CancellationToken cancellationToken = default)
        {
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(5));
            using var stop = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, timeout.Token);
            try
            {
                await writes.WaitAsync(stop.Token).ConfigureAwait(false);
                if (failure is { } previous)
                {
                    writes.Release();
                    previous.Throw();
                }
                var write = Task.Run(async () =>
                {
                    try { await inner.WriteLineAsync(buffer, CancellationToken.None).ConfigureAwait(false); }
                    catch (Exception exception) when (exception is not OutOfMemoryException)
                    {
                        failure = ExceptionDispatchInfo.Capture(exception);
                    }
                    finally { writes.Release(); }
                }, CancellationToken.None);
                await write.WaitAsync(stop.Token).ConfigureAwait(false);
                failure?.Throw();
            }
            catch (OperationCanceledException exception) when (!cancellationToken.IsCancellationRequested && timeout.IsCancellationRequested)
            {
                throw new IOException("Timed out writing live CLI output; the reader may have stopped draining the pipe.", exception);
            }
        }
    }

    private static string EtwState(ILiveConnection? connection) =>
        connection?.EtwError is not null || connection?.Etw is null ? "unavailable" :
        connection.Etw is { LostEvents: > 0 } or { DroppedEvents: > 0 } ? "partial" : "available";

    private static string RecordingState(ILiveConnection? connection) =>
        connection?.RecordingError is not null || connection?.Recording is null ? "unavailable" :
        connection.Recording.CompleteFromSeconds is not null ? "partial" : "available";

    // Console.In's synchronized reader may implement ReadLineAsync synchronously. Keep it off the host loop.
    private static Task<string?> ReadCommandAsync(TextReader input, CancellationToken token) =>
        Task.Run(() => input.ReadLineAsync(token).AsTask(), CancellationToken.None).WaitAsync(token);

    private sealed class ProgressQueue : IProgress<ConnectionProgress>
    {
        internal ConcurrentQueue<ConnectionProgress> Items { get; } = new();
        public void Report(ConnectionProgress value) => Items.Enqueue(value);
    }

    internal interface ILiveConnection : IAsyncDisposable
    {
        string State { get; }
        string Detail { get; }
        SessionScope? Scope { get; }
        string RuntimePath { get; }
        DateTime? LastHelloUtc { get; }
        Task Completion { get; }
        MetricHistory? History { get; }
        EtwStatus? Etw { get; }
        string? EtwError { get; }
        RecordingStatus? Recording { get; }
        string? RecordingError { get; }
        Task<SavedTreeSnapshot> TakeSnapshotAsync(string directory, CancellationToken token);
        Task<SavedTreeSnapshot> TakeSnapshotToFileAsync(string path, CancellationToken token);
    }

    [SupportedOSPlatform("windows")]
    private sealed class LiveConnection(TargetConnection connection) : ILiveConnection
    {
        public string State => connection.State;
        public string Detail => connection.Detail;
        public SessionScope? Scope => connection.Scope;
        public string RuntimePath => connection.RuntimePath;
        public DateTime? LastHelloUtc => connection.LastHelloUtc;
        public Task Completion => connection.Completion;
        public MetricHistory? History => connection.History;
        public EtwStatus? Etw => connection.Etw;
        public string? EtwError => connection.EtwError;
        public RecordingStatus? Recording => connection.Recording;
        public string? RecordingError => connection.RecordingError;
        public Task<SavedTreeSnapshot> TakeSnapshotAsync(string directory, CancellationToken token) => connection.TakeSnapshotAsync(directory, token);
        public Task<SavedTreeSnapshot> TakeSnapshotToFileAsync(string path, CancellationToken token) => connection.TakeSnapshotToFileAsync(path, token);
        public ValueTask DisposeAsync() => connection.DisposeAsync();
    }

    internal sealed record AppCandidate(string Name, string AppId);

    private static async Task<IReadOnlyList<AppCandidate>> DiscoverAppsAsync(CancellationToken token)
    {
        var executable = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),
            @"WindowsPowerShell\v1.0\powershell.exe");
        var start = new ProcessStartInfo(executable)
        {
            UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true,
            StandardOutputEncoding = Encoding.UTF8, StandardErrorEncoding = Encoding.UTF8
        };
        // The inbox StartLayout module queries the current user's registered app list without loading WinRT into Core.
        foreach (var argument in new[] { "-NoLogo", "-NoProfile", "-NonInteractive", "-Command",
            "$ErrorActionPreference='Stop'; [Console]::OutputEncoding=[Text.UTF8Encoding]::new($false); " +
            "ConvertTo-Json -InputObject @(Get-StartApps | Sort-Object Name | Select-Object Name,AppID) -Compress" })
            start.ArgumentList.Add(argument);
        using var child = Process.Start(start) ?? throw new IOException("Windows app discovery did not start.");
        var textTask = child.StandardOutput.ReadToEndAsync(token);
        var errorsTask = child.StandardError.ReadToEndAsync(token);
        try
        {
            await child.WaitForExitAsync(token).ConfigureAwait(false);
            var text = await textTask.ConfigureAwait(false);
            var errors = await errorsTask.ConfigureAwait(false);
            if (child.ExitCode != 0) throw new IOException($"Windows app discovery failed: {errors.Trim()}");
            using var document = JsonDocument.Parse(text);
            return document.RootElement.EnumerateArray().Select(item => new AppCandidate(
                item.GetProperty("Name").GetString()!, item.GetProperty("AppID").GetString()!))
                .Where(item => item.AppId.Contains('!')).ToArray();
        }
        finally
        {
            // This is our discovery helper, never the profiled target.
            if (!child.HasExited)
            {
                try { child.Kill(); }
                catch (InvalidOperationException) when (child.HasExited) { }
            }
        }
    }
}

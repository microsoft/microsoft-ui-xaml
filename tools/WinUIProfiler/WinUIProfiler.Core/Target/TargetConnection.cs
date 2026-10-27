using System.Diagnostics;
using System.Globalization;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.Engine.Target;

public enum TargetLaunchKind
{
    Unpackaged, Packaged, Attach
}
public sealed record TargetConnectionOptions(TargetLaunchKind Kind,
string Target,
string Arguments,
    string? WorkingDirectory,
string PayloadDirectory,
bool TrustLocalBinaries,
    ushort EndpointNumber = 1,
long? ExpectedCreationTimeFileTime = null);
public sealed record ConnectionProgress(string State, string Detail);
// FromProcessStart is true when the session was listening before the target was created, so its startup is
// included; otherwise the counts start when the profiler connected.
public sealed record EtwStatus(bool FromProcessStart, long XamlEvents, long ProfilerEvents, int LostEvents, long DroppedEvents);

[SupportedOSPlatform("windows")]
public sealed class TargetConnection : IAsyncDisposable
{
    private readonly CancellationTokenSource stop = new();
    private readonly object disposalGate = new();
    private readonly SemaphoreSlim targetCommands = new(1, 1);
    private readonly IProgress<ConnectionProgress>? progress;
    private Process? target;
    private TapClient? tap;
    private ProcessMetricSession? sampling;
    // Kept after cleanup so the final counts stay visible.
    private LiveEtwSource? etw;
    private EventPipeline? pipeline;
    private EventCounter? eventCounter;
    private bool etwFromProcessStart;
    private long etwListeningSinceFileTime;
    private string? etwStartError;
    private Task? lifetime;
    private Task? disposal;
    private volatile string state = "Preparing";
    private volatile string detail = "";
    private long lastHelloTicks;
    public event EventHandler? Changed;
    public SessionScope? Scope
    {
        get; private set;
    }
    public MetricHistory? History
    {
        get; private set;
    }
    public string RuntimePath { get; private set; } = "";
    public string State => state;
    public string Detail => detail;
    public Exception? Failure
    {
        get; private set;
    }
    public DateTime? LastHelloUtc => Interlocked.Read(ref lastHelloTicks) is var ticks && ticks > 0
        ? new DateTime(ticks, DateTimeKind.Utc) : null;
    public Task Completion => lifetime ?? Task.CompletedTask;
    public EtwStatus? Etw => eventCounter is { } counter && etw is { } source && pipeline is { } events
        ? new(etwFromProcessStart, counter.XamlEvents, counter.ProfilerEvents, source.EventsLost, events.DroppedEvents) : null;
    // Set when the ETW session could not start or stopped with an error; process metrics keep working.
    public string? EtwError
    {
        get
        {
            if (etwStartError is not null)
            {
                return etwStartError;
            }

            var failure = etw?.Failure ?? pipeline?.Failure;
            return failure is null ? null : $"ETW reading stopped: {failure.Message}";
        }
    }
    private TargetConnection(IProgress<ConnectionProgress>? progress) => this.progress = progress;

    public static void ValidateOptions(TargetConnectionOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        Require.Enum(options.Kind);
        ArgumentException.ThrowIfNullOrWhiteSpace(options.Target);
        ArgumentNullException.ThrowIfNull(options.Arguments);
        if (!options.TrustLocalBinaries)
        {
            throw new UnauthorizedAccessException("Explicitly trust the local injector and TAP before connecting.");
        }

        if (!Path.IsPathFullyQualified(options.PayloadDirectory))
        {
            throw new ArgumentException("The native payload directory must be absolute.");
        }

        if (options.EndpointNumber == 0)
        {
            throw new ArgumentOutOfRangeException(nameof(options.EndpointNumber));
        }

        if (options.ExpectedCreationTimeFileTime is { } creation &&
                    (options.Kind != TargetLaunchKind.Attach ||
        creation <= 0))
        {
            throw new ArgumentException("A selected creation time requires running-process attach.");
        }

        if (options.Kind == TargetLaunchKind.Attach &&
                    (!int.TryParse(options.Target, NumberStyles.None, CultureInfo.InvariantCulture, out var pid) ||
                        pid <= 0 ||
        pid == Environment.ProcessId))
        {
            throw new ArgumentException("Select another running process using its positive PID.");
        }
    }

    public static async Task<TargetConnection> ConnectAsync(TargetConnectionOptions options,
        IProgress<ConnectionProgress>? progress, CancellationToken token)
    {
        ValidateOptions(options);
        var connection = new TargetConnection(progress);
        try
        {
            await connection.ConnectCoreAsync(options, token).ConfigureAwait(false);
            return connection;
        }
        catch (Exception error)
        {
            var failure = error is OperationCanceledException && !token.IsCancellationRequested
                ? new TimeoutException("Connecting to the target exceeded the 45-second deadline.", error) : error;
            connection.Failure = failure;
            await connection.CleanupAsync().ConfigureAwait(false);
            connection.stop.Dispose();
            if (connection.Failure != failure)
            {
                throw new AggregateException("Connection failed and cleanup reported an error.", failure, connection.Failure!);
            }

            if (failure != error)
            {
                throw failure;
            }

            throw;
        }
    }

    private async Task ConnectCoreAsync(TargetConnectionOptions options, CancellationToken cancellationToken)
    {
        using var deadline = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        deadline.CancelAfter(TimeSpan.FromSeconds(45));
        var token = deadline.Token;
        token.ThrowIfCancellationRequested();
        Report("Launching", options.Target);
        var launchExecutables = GetLaunchExecutables(options);
        if (launchExecutables.Count > 0)
        {
            PrepareEtw(launchExecutables);
        }

        target = options.Kind switch
        {
            TargetLaunchKind.Unpackaged => TargetLauncher.LaunchUnpackaged(options.Target, options.Arguments, options.WorkingDirectory),
            TargetLaunchKind.Packaged => TargetLauncher.LaunchPackaged(options.Target, options.Arguments),
            TargetLaunchKind.Attach => Process.GetProcessById(int.Parse(options.Target, CultureInfo.InvariantCulture)),
            _ => throw new ArgumentOutOfRangeException(nameof(options))
        };
        Scope = new(Guid.NewGuid(), Identify(target));
        if (options.ExpectedCreationTimeFileTime is { } selected && selected != Scope.Process.CreationTimeFileTime)
        {
            throw new InvalidOperationException("The selected PID was reused. Refresh the process list and select again.");
        }

        StartEtw(target, Scope.Process.CreationTimeFileTime, launchExecutables);
        var payloads = ConnectionPayloadCatalog.Resolve(options.PayloadDirectory, InjectorResolver.GetArchitecture(Scope.Process));
        Report("Waiting for XAML", $"PID {target.Id}; waiting for the WinUI diagnostics endpoint.");
        string? runtime = null;
        string? udk = null;
        while (runtime is null || udk is null || !IsDiagnosticsReady(Scope.Process.ProcessId))
        {
            token.ThrowIfCancellationRequested();
            if (target.HasExited)
            {
                throw new IOException("The target exited before WinUI diagnostics became ready.");
            }

            target.Refresh();
            foreach (ProcessModule module in target.Modules)
            {
                if (module.ModuleName.Equals("Microsoft.UI.Xaml.dll", StringComparison.OrdinalIgnoreCase))
                {
                    runtime = module.FileName;
                }

                if (module.ModuleName.Equals("Microsoft.Internal.FrameworkUdk.dll", StringComparison.OrdinalIgnoreCase))
                {
                    udk = module.FileName;
                }
            }
            if (runtime is null || udk is null || !IsDiagnosticsReady(Scope.Process.ProcessId))
            {
                await Task.Delay(100, token).ConfigureAwait(false);
            }
        }
        RuntimePath = runtime;
        var resolved = InjectorResolver.Resolve(Scope.Process, new(payloads.Helper, payloads.Tap, runtime, udk), true);
        var pipe = TapClient.CreatePipeName();
        var nonce = RandomNumberGenerator.GetBytes(TapProtocol.NonceBytes);
        using var current = Process.GetCurrentProcess();
        var bootstrap = InjectorResolver.CreateBootstrap(pipe, Scope, Identify(current), nonce);
        Report("Injecting TAP", $"PID {target.Id}; endpoint {options.EndpointNumber}.");
        await InjectAsync(resolved.Payloads, options.EndpointNumber, bootstrap, token).ConfigureAwait(false);
        Report("Authenticating pipe", "Verifying target identity, session and nonce.");
        tap = await TapClient.ConnectAsync(pipe, Scope, nonce, TimeSpan.FromSeconds(10), token).ConfigureAwait(false);
        Report("Verifying hello", "Sending hello and waiting for the native TAP hello reply.");
        if (await tap.HelloAsync(TimeSpan.FromSeconds(5), token).ConfigureAwait(false) != "hello")
        {
            throw new InvalidDataException("The target TAP did not reply hello.");
        }

        Interlocked.Exchange(ref lastHelloTicks, DateTime.UtcNow.Ticks);
        Changed?.Invoke(this, EventArgs.Empty);
        History = new(Scope.Process, Stopwatch.Frequency);
        sampling = new(Path.GetFullPath(Path.Combine(options.PayloadDirectory, "..", "WinUIProfiler.Native.dll")),
            Scope.Process,
History);
        sampling.Start();
        var first = await Task.WhenAny(sampling.FirstSample, sampling.Completion).WaitAsync(token).ConfigureAwait(false);
        await first.ConfigureAwait(false);
        token.ThrowIfCancellationRequested();
        Report("Connected", $"hello received from TAP in PID {target.Id}; process sampling is active.");
        lifetime = MaintainAsync();
    }

    // The executable names a launch will start, so ETW can listen before the process exists. Empty for attach,
    // or when a packaged application's executable cannot be determined; ETW then starts after the launch.
    private static IReadOnlyList<string> GetLaunchExecutables(TargetConnectionOptions options)
    {
        try
        {
            return options.Kind switch
            {
                TargetLaunchKind.Unpackaged => Path.GetFileName(options.Target) is { Length: > 0 } name ? [name] : [],
                TargetLaunchKind.Packaged => TargetLauncher.GetPackagedExecutableNames(options.Target),
                _ => []
            };
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            Trace.TraceWarning($"The launched executable is unknown, so ETW starts after the launch: {error}");
            return [];
        }
    }

    private void PrepareEtw(IReadOnlyList<string> launchExecutables)
    {
        try
        {
            etwListeningSinceFileTime = DateTime.UtcNow.ToFileTimeUtc();
            etw = LiveEtwSource.StartBeforeLaunch(launchExecutables);
        }
        catch (Exception error)
        {
            SetEtwStartError(error);
        }
    }

    private void StartEtw(Process process, long? creationTimeFileTime, IReadOnlyList<string> launchExecutables)
    {
        if (etwStartError is not null)
        {
            return;
        }

        var counter = new EventCounter();
        var events = new EventPipeline([counter]);
        try
        {
            if (etw is null)
            {
                etw = LiveEtwSource.Start(process.Id, events.Post);
            }
            else
            {
                // The session has listened since before the launch. It saw this process's start only if the process
                // was created after that (activating a packaged app that is already running returns the existing
                // process) and runs one of the executables the session was filtering on.
                var createdAfterListening = creationTimeFileTime >= etwListeningSinceFileTime;
                var runsExpectedExecutable = launchExecutables.Any(name =>
                    Path.GetFileNameWithoutExtension(name).Equals(process.ProcessName, StringComparison.OrdinalIgnoreCase));
                etwFromProcessStart = createdAfterListening && runsExpectedExecutable;
                if (!etwFromProcessStart)
                {
                    Trace.TraceWarning(createdAfterListening
                        ? $"Launched {process.ProcessName}, expected {string.Join(", ", launchExecutables)}; ETW starts now."
                        : "The activated application was already running; ETW starts now.");
                }

                etw.StartReading(process.Id, events.Post);
            }

            pipeline = events;
            eventCounter = counter;
        }
        catch (Exception error)
        {
            events.Dispose();
            etw?.Dispose();
            etw = null;
            etwFromProcessStart = false;
            SetEtwStartError(error);
        }
    }

    private void SetEtwStartError(Exception error)
    {
        etwStartError = error is UnauthorizedAccessException
            ? "Reading ETW needs an elevated profiler or membership in the local Performance Log Users group."
            : $"The ETW session could not start: {error.Message}";
        Trace.TraceWarning(error.ToString());
    }

    public static ProcessInstanceKey Identify(Process process) =>
        new(checked((uint)process.Id), process.StartTime.ToFileTimeUtc(), Guid.NewGuid());

    public async Task<SavedTreeSnapshot> TakeSnapshotAsync(string directory, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(directory);
        if (!Path.IsPathFullyQualified(directory))
        {
            throw new ArgumentException("Snapshot output directory must be absolute.", nameof(directory));
        }

        var client = tap;
        if (State != "Connected" || client is null)
        {
            throw new InvalidOperationException("Connect to a target before taking a tree snapshot.");
        }

        using var cancellation = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, stop.Token);
        var token = cancellation.Token;
        TreeSnapshotDocument snapshot;
        await targetCommands.WaitAsync(token).ConfigureAwait(false);
        try
        {
            snapshot = await client.TakeSnapshotAsync(TimeSpan.FromSeconds(30), token).ConfigureAwait(false);
        }
        finally
        {
            targetCommands.Release();
        }

        token.ThrowIfCancellationRequested();
        Directory.CreateDirectory(directory);
        var path = Path.Combine(directory, $"tree-{snapshot.ProcessId}-{DateTime.UtcNow:yyyyMMdd-HHmmss}-{snapshot.CaptureId:N}.json");
        var temporary = path + ".partial";
        var created = false;
        var committed = false;
        try
        {
            await using (var output = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None,
                64 * 1024, FileOptions.Asynchronous))
            {
                created = true;
                await TreeSnapshotCodec.WriteAsync(output, snapshot, token).ConfigureAwait(false);
            }
            token.ThrowIfCancellationRequested();
            File.Move(temporary, path);
            committed = true;
            return new(path, snapshot.CaptureId, snapshot.Roots.Count, snapshot.XamlNodes.Count,
                snapshot.CompositionNodes.Count, snapshot.Correlations.Count);
        }
        finally
        {
            if (created && !committed)
            {
                try { File.Delete(temporary); }
                catch (Exception error) when (error is IOException or UnauthorizedAccessException)
                {
                    Trace.TraceError($"Removing incomplete tree snapshot '{temporary}' failed: {error}");
                }
            }
        }
    }

    private async Task MaintainAsync()
    {
        var targetExited = false;
        try
        {
            var exited = target!.WaitForExitAsync(stop.Token);
            while (!stop.IsCancellationRequested)
            {
                var delay = Task.Delay(TimeSpan.FromSeconds(5), stop.Token);
                var completed = await Task.WhenAny(delay, exited, sampling!.Completion).ConfigureAwait(false);
                if (completed == exited || completed == sampling.Completion && target.HasExited)
                {
                    targetExited = true;
                    break;
                }
                if (completed == sampling.Completion)
                {
                    await sampling.Completion.ConfigureAwait(false);
                    throw new IOException("Process sampling ended unexpectedly.");
                }
                await delay.ConfigureAwait(false);
                // A bounded tree capture must not consume the heartbeat's own timeout.
                await targetCommands.WaitAsync(stop.Token).ConfigureAwait(false);
                try
                {
                    await tap!.HelloAsync(TimeSpan.FromSeconds(5), stop.Token).ConfigureAwait(false);
                }
                finally
                {
                    targetCommands.Release();
                }
                Interlocked.Exchange(ref lastHelloTicks, DateTime.UtcNow.Ticks);
                Changed?.Invoke(this, EventArgs.Empty);
            }
        }
        catch (OperationCanceledException) when (stop.IsCancellationRequested) { }
        catch (Exception error)
        {
            if (target!.HasExited)
            {
                targetExited = true;
            }
            else
            {
                Failure = error;
                Trace.TraceError(error.ToString());

            }
        }
        finally
        {
            await stop.CancelAsync().ConfigureAwait(false);
            await CleanupAsync(targetExited).ConfigureAwait(false);
            Report(Failure is null ? "Disconnected" : "Failed",
                Failure?.Message ?? (targetExited ? "Target exited; process sampling stopped." : "Disconnected; the target was not terminated."));
        }
    }

    private async Task CleanupAsync(bool targetExited = false)
    {
        var errors = new List<Exception>();
        try
        {
            // Stop the source before the pipeline so nothing is posted to a finished queue.
            etw?.Dispose();
            pipeline?.Dispose();
        }
        catch (Exception error)
        {
            errors.Add(error);
        }

        if (sampling is not null)
        {
            try
            {
                await sampling.DisposeAsync().ConfigureAwait(false);

            }
            catch (IOException error) when (targetExited)
            {
                Trace.TraceInformation(error.Message);

            }
            catch (Exception error)
            {
                errors.Add(error);

            }
            sampling = null;
        }
        if (tap is not null)
        {
            try
            {
                if (!targetExited && Failure is null && !tap.IsClosed)
                {
                    await tap.DisconnectAsync(CancellationToken.None).ConfigureAwait(false);
                }
            }
            catch (Exception error)
            {
                errors.Add(error);

            }
            finally
            {
                try
                {
                    await tap.DisposeAsync().ConfigureAwait(false);

                }
                catch (Exception error)
                {
                    errors.Add(error);

                }
                tap = null;
            }
        }
        target?.Dispose();
        if (errors.Count > 0)
        {
            if (Failure is not null)
            {
                errors.Insert(0, Failure);
            }

            Failure = new AggregateException("Connection cleanup failed.", errors);
            Trace.TraceError(Failure.ToString());
        }
    }

    public ValueTask DisposeAsync()
    {
        lock (disposalGate)
        {
            return new(disposal ??= DisposeCoreAsync());
        }
    }

    private async Task DisposeCoreAsync()
    {
        try
        {
            await stop.CancelAsync().ConfigureAwait(false);
            if (lifetime is not null)
            {
                await lifetime.ConfigureAwait(false);
            }

            if (Failure is not null)
            {
                throw new IOException("Target connection ended with an error.", Failure);
            }
        }
        finally
        {
            stop.Dispose();

        }
    }

    private void Report(string value, string detail)
    {
        this.detail = detail;
        state = value;
        progress?.Report(new(value, detail));
        Changed?.Invoke(this, EventArgs.Empty);
    }

    private static bool IsDiagnosticsReady(uint pid)
    {
        if (!EventWaitHandle.TryOpenExisting("WinUIDebuggable" + pid.ToString(CultureInfo.InvariantCulture), out var ready))
        {
            return false;
        }

        using (ready)
        {
            return ready.WaitOne(0);
        }
    }

    private static async Task InjectAsync(InjectorPayloads payloads, ushort endpoint, string bootstrap, CancellationToken token)
    {
        var start = new ProcessStartInfo(payloads.Helper)
        {
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        foreach (var argument in new[] { payloads.Runtime, payloads.FrameworkUdk, payloads.Tap,
            endpoint.ToString(CultureInfo.InvariantCulture), "--trust-local-binaries" })
        {
            start.ArgumentList.Add(argument);
        }

        using var helper = Process.Start(start) ?? throw new IOException("The injector did not start.");
        using var outputLimit = CancellationTokenSource.CreateLinkedTokenSource(token);
        var output = ReadBoundedAsync(helper.StandardOutput, outputLimit.Token);
        var errors = ReadBoundedAsync(helper.StandardError, outputLimit.Token);
        try
        {
            await helper.StandardInput.WriteLineAsync(bootstrap.AsMemory(), token).ConfigureAwait(false);
            helper.StandardInput.Close();
            var exit = helper.WaitForExitAsync(token);
            var pending = new List<Task> { exit, output, errors };
            while (pending.Count > 0)
            {
                var completed = await Task.WhenAny(pending).ConfigureAwait(false);
                await completed.ConfigureAwait(false);
                pending.Remove(completed);
            }
            var text = await output.ConfigureAwait(false);
            var error = await errors.ConfigureAwait(false);
            if (helper.ExitCode != 0)
            {
                throw new IOException($"TAP injection failed (0x{unchecked((uint)helper.ExitCode):X8}). {error}");
            }

            using var result = JsonDocument.Parse(text);
            if (result.RootElement.GetProperty("schemaVersion").GetInt32() != 1 ||
                result.RootElement.GetProperty("hresult").GetUInt32() != 0 ||
                !result.RootElement.GetProperty("injectionAccepted").GetBoolean())
            {
                throw new InvalidDataException("Injector returned an invalid acceptance result.");
            }
        }
        finally
        {
            outputLimit.Cancel();
            if (!helper.HasExited)
            {
                helper.Kill(entireProcessTree: true);
            }

            await helper.WaitForExitAsync().ConfigureAwait(false);
            try
            {
                await Task.WhenAll(output, errors).ConfigureAwait(false);

            }
            catch (Exception error) when (error is OperationCanceledException or IOException or InvalidDataException)
            {
                // The loop propagates the primary error; join both readers before releasing their streams.
                Trace.TraceError(error.ToString());
            }
        }
    }

    private static async Task<string> ReadBoundedAsync(StreamReader reader, CancellationToken token)
    {
        var text = new StringBuilder();
        var buffer = new char[4096];
        int count;
        while ((count = await reader.ReadAsync(buffer.AsMemory(), token).ConfigureAwait(false)) != 0)
        {
            if (text.Length + count > 32768)
            {
                throw new InvalidDataException("Injector output exceeds its bound.");
            }

            text.Append(buffer, 0, count);
        }
        return text.ToString();
    }
}

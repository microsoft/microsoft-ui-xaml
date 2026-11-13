using System.Diagnostics;
using System.Globalization;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;

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
long? ExpectedCreationTimeFileTime = null,
    string? SessionDirectory = null,
    IReadOnlyList<MarkerProviderSettings>? MarkerProviders = null,
    PrivateRuntimeLaunch? PrivateRuntime = null);
public sealed record ConnectionProgress(string State, string Detail);
// FromProcessStart is true when the session was listening before the target was created, so its startup is
// included; otherwise the counts start when the profiler connected.
public sealed record EtwStatus(bool FromProcessStart, long XamlEvents, long ProfilerEvents, long GpuMemoryEvents, int LostEvents,
    long DroppedEvents);

[SupportedOSPlatform("windows")]
public sealed class TargetConnection : IAsyncDisposable
{
    private readonly CancellationTokenSource stop = new();
    private readonly object disposalGate = new();
    private readonly SemaphoreSlim targetCommands = new(1, 1);
    private readonly IProgress<ConnectionProgress>? progress;
    private Process? target;
    private NativeLaunchSession? nativeLaunch;
    private TapClient? tap;
    private ProcessMetricSession? sampling;
    // Kept after cleanup so the final counts stay visible.
    private LiveEtwSource? etw;
    private ProcessRecordingGroup? recordings;
    private bool etwFromProcessStart;
    private long etwListeningSinceFileTime;
    private long etwListeningSinceQpc;
    private string? etwStartError;
    // Samples the recorded processes' call stacks; null when the session isn't recorded or sampling couldn't start.
    private KernelSampleSession? cpu;
    private string? cpuSamplingError;
    private string? startupCatalogError;
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
    public EtwStatus? Etw => recordings?.Primary is { } primary && etw is { } source
        ? new(etwFromProcessStart, primary.XamlEvents, primary.ProfilerEvents, primary.GpuMemoryEvents, source.EventsLost,
            primary.DroppedEvents) : null;
    // Set when the ETW session could not start or stopped with an error; process metrics keep working.
    public string? EtwError
    {
        get
        {
            if (etwStartError is not null)
            {
                return etwStartError;
            }

            var failure = etw?.Failure ?? recordings?.Failure;
            return failure is null ? null : $"ETW reading stopped: {failure.Message}";
        }
    }
    // The session file the events are saved to; null when this connection does not record. Kept after cleanup.
    public RecordingStatus? Recording => recordings?.Primary.Recording;
    // Every session file this connection records, the target's first: a startup handoff records each of its processes.
    public IReadOnlyList<string> RecordingPaths => recordings?.RecordingPaths ?? [];
    public string? RecordingError => recordings?.RecordingError ?? startupCatalogError;
    // Why a recorded session has no CPU samples, or why some are missing; the events are recorded either way.
    public string? CpuSamplingError => cpuSamplingError ?? cpu?.Note;
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

        if (options.PrivateRuntime is { } runtime)
        {
            if (options.Kind == TargetLaunchKind.Attach)
            {
                throw new ArgumentException("Private WinUI DLLs can be loaded only into an app the profiler launches.");
            }

            if (!string.IsNullOrEmpty(options.WorkingDirectory))
            {
                throw new ArgumentException("An app launched with private WinUI DLLs starts in its own folder.");
            }

            if (options.Kind == TargetLaunchKind.Packaged && !string.IsNullOrWhiteSpace(options.Arguments))
            {
                throw new ArgumentException("A packaged app launched with private WinUI DLLs is activated without arguments.");
            }

            if ((options.Kind == TargetLaunchKind.Unpackaged && !Path.IsPathFullyQualified(options.Target)) ||
                !new[] { runtime.RuntimeDirectory, runtime.SourceDirectory, runtime.LaunchDirectory, runtime.LauncherDirectory }
                    .All(path => !string.IsNullOrWhiteSpace(path) && Path.IsPathFullyQualified(path)))
            {
                throw new ArgumentException("The target and the private WinUI folders must be full paths.");
            }
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
        var markers = options.MarkerProviders ?? [];
        if (launchExecutables.Count > 0)
        {
            PrepareEtw(launchExecutables, GetLauncherExecutables(options), markers);
        }

        // Before the launch too, so the startup's samples are kept until the recordings exist.
        if (options.SessionDirectory is not null && etwStartError is null)
        {
            PrepareCpuSampling();
        }

        if (options.PrivateRuntime is { } privateRuntime)
        {
            nativeLaunch = await NativeLauncher.LaunchAsync(options.Kind, options.Target, options.Arguments, privateRuntime, token)
                .ConfigureAwait(false);
            target = nativeLaunch.Process;
        }
        else
        {
            target = options.Kind switch
            {
                TargetLaunchKind.Unpackaged => TargetLauncher.LaunchUnpackaged(options.Target, options.Arguments, options.WorkingDirectory),
                TargetLaunchKind.Packaged => TargetLauncher.LaunchPackaged(options.Target, options.Arguments),
                TargetLaunchKind.Attach => Process.GetProcessById(int.Parse(options.Target, CultureInfo.InvariantCulture)),
                _ => throw new ArgumentOutOfRangeException(nameof(options))
            };
        }
        Scope = new(Guid.NewGuid(), Identify(target));
        if (options.ExpectedCreationTimeFileTime is { } selected && selected != Scope.Process.CreationTimeFileTime)
        {
            throw new InvalidOperationException("The selected PID was reused. Refresh the process list and select again.");
        }

        StartEtw(target, Scope.Process.CreationTimeFileTime, launchExecutables, options.SessionDirectory, markers);
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
        if (options.PrivateRuntime is { } loadedRuntime)
        {
            NativeLauncher.VerifyModules(target, loadedRuntime);
        }

        RuntimePath = runtime;
        var resolved = InjectorResolver.Resolve(Scope.Process, new(payloads.Helper, payloads.Tap, runtime, udk), true);
        var pipe = TapClient.CreatePipeName();
        var nonce = RandomNumberGenerator.GetBytes(TapProtocol.NonceBytes);
        using var current = Process.GetCurrentProcess();
        var bootstrap = InjectorResolver.CreateBootstrap(pipe, Scope, Identify(current), nonce);
        Report("Injecting TAP", $"PID {target.Id}; endpoint {options.EndpointNumber}.");
        // InitializeXamlDiagnosticsEx drops the request and returns ERROR_NOT_FOUND when the target's UI thread doesn't
        // take it within about a second, as in a busy startup. Retry with the same bootstrap until the deadline, unless
        // the TAP's pipe shows that the target took a request after all.
        while (!await InjectAsync(resolved.Payloads, options.EndpointNumber, bootstrap, token).ConfigureAwait(false) &&
            !TapClient.PipeExists(pipe))
        {
            if (target.HasExited)
            {
                throw new IOException("The target exited before the TAP was injected.");
            }

            Report("Injecting TAP", $"PID {target.Id}; endpoint {options.EndpointNumber}; the target's UI thread is busy, retrying.");
            await Task.Delay(250, token).ConfigureAwait(false);
        }
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

    // The executables, other than the profiler's, of a process that can create the launched one, so ETW has its launcher
    // events. With private WinUI DLLs, ProfilerLauncher.exe activates a packaged app through a COM surrogate,
    // dllhost.exe, which creates the app's process.
    private static IReadOnlyList<string> GetLauncherExecutables(TargetConnectionOptions options) =>
        options is { PrivateRuntime: not null, Kind: TargetLaunchKind.Packaged } ? ["dllhost.exe"] : [];

    private void PrepareEtw(IReadOnlyList<string> launchExecutables, IReadOnlyList<string> launcherExecutables,
        IReadOnlyList<MarkerProviderSettings> markers)
    {
        try
        {
            etwListeningSinceFileTime = DateTime.UtcNow.ToFileTimeUtc();
            etwListeningSinceQpc = Stopwatch.GetTimestamp();
            etw = LiveEtwSource.StartBeforeLaunch(launchExecutables, launcherExecutables, markers);
        }
        catch (Exception error)
        {
            SetEtwStartError(error);
        }
    }

    private void StartEtw(Process process, long? creationTimeFileTime, IReadOnlyList<string> launchExecutables,
        string? sessionDirectory, IReadOnlyList<MarkerProviderSettings> markers)
    {
        if (etwStartError is not null)
        {
            // Without the events there are no recordings to store the samples in.
            cpu?.Dispose();
            cpu = null;
            return;
        }

        (int ProcessId, string? Name)? launcher = null;
        try
        {
            if (etw is not null)
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
                else
                {
                    // The launcher is the process that created the launch, which is not always the profiler.
                    launcher = FindLauncher(process);
                    if (launcher is null)
                    {
                        Trace.TraceWarning("The process that created the target is unknown, so the launch has no launcher events.");
                    }
                }
            }

            IReadOnlyList<SessionStart> sources = nativeLaunch?.Result.Processes is { Count: > 0 } group
                ? group.Select(entry => CreateSessionStart(entry.ProcessId, process.ProcessName, entry.CreationTimeFileTime, launcher)).ToArray()
                : [CreateSessionStart(process.Id, process.ProcessName, creationTimeFileTime, launcher)];
            recordings = new(sources, process.Id, launcher?.ProcessId, sessionDirectory);
            var identities = sources.ToDictionary(source => source.ProcessId, source => source.ProcessCreatedUtc?.ToFileTimeUtc());
            if (etw is null)
            {
                etwListeningSinceQpc = Stopwatch.GetTimestamp();
                etw = LiveEtwSource.Start(identities, markers, recordings.Post);
            }
            else
            {
                // The launcher's events from the launcher providers are the launch's too.
                etw.StartReading(identities, launcher?.ProcessId, recordings.Post);
            }
            StartCpuSampling(recordings);
            SaveStartupRecordings("Recording");
        }
        catch (Exception error)
        {
            etw?.Dispose();
            etw = null;
            cpu?.Dispose();
            cpu = null;
            recordings?.Close("ETW could not start");
            etwFromProcessStart = false;
            SetEtwStartError(error);
            SaveStartupRecordings("ETW could not start");
        }
    }

    // The process that created the launch. In a startup handoff, the target's parent is the bootstrap, another process of
    // the launch, so the launcher is the parent of the launch's first process. A process of the launch is never the
    // launcher, which ProcessRecordingGroup would otherwise route the launcher's events to.
    private (int ProcessId, string? Name)? FindLauncher(Process process)
    {
        if (nativeLaunch?.Result is not { ActivationProcessId: { } root } result)
        {
            return ProcessParent.Find(process);
        }

        var launcher = result.Processes.FirstOrDefault(entry => entry.ProcessId == root) is { } activation
            ? ProcessParent.Find(activation.ProcessId, activation.CreationTimeFileTime) : null;
        return launcher is { } found && result.Processes.All(entry => entry.ProcessId != found.ProcessId) ? launcher : null;
    }

    // Starts sampling the processors' call stacks for a recorded session, as WPR's CPU profile does. The kernel's
    // sampled profile needs an elevated profiler; otherwise the session records events only.
    private void PrepareCpuSampling()
    {
        try
        {
            cpu = KernelSampleSession.Start();
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            cpuSamplingError = error is UnauthorizedAccessException
                ? "CPU sampling needs an elevated (administrator) profiler; only events are recorded."
                : $"CPU sampling could not start: {error.Message}";
            Trace.TraceWarning(error.ToString());
        }
    }

    // Gives each recording its process's CPU samples, from those the session kept since it started.
    private void StartCpuSampling(ProcessRecordingGroup group)
    {
        if (!group.IsWriting)
        {
            cpu?.Dispose();
            cpu = null;
            return;
        }

        if (cpu is null && cpuSamplingError is null)
        {
            PrepareCpuSampling();
        }

        if (cpu is null)
        {
            return;
        }

        try
        {
            cpu.StartReading(group.IsRecorded, group.PostSample);
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            cpuSamplingError = $"CPU sampling could not start: {error.Message}";
            Trace.TraceWarning(error.ToString());
            cpu.Dispose();
            cpu = null;
        }
    }

    private void SaveStartupRecordings(string status)
    {
        if (nativeLaunch is null || recordings is null) return;
        try
        {
            var path = Path.Combine(nativeLaunch.Directory, "recordings.json");
            var text = JsonSerializer.Serialize(new
            {
                schemaVersion = 1,
                primaryProcessId = nativeLaunch.Result.ProcessId,
                activationProcessId = nativeLaunch.Result.ActivationProcessId,
                status,
                recordings = recordings.Processes.Select(recording => new
                {
                    pid = recording.Source.ProcessId,
                    creationTimeFileTime = recording.Source.ProcessCreatedUtc?.ToFileTimeUtc(),
                    isPrimary = recording.IsPrimary,
                    sessionFile = recording.Recording?.Path,
                    coverage = recording.Source.Coverage.ToString(),
                    storedEvents = recording.Recording?.StoredEvents,
                    xamlEvents = recording.XamlEvents,
                    profilerEvents = recording.ProfilerEvents,
                    gpuMemoryEvents = recording.GpuMemoryEvents,
                    droppedEvents = recording.DroppedEvents,
                    error = recording.Error
                })
            }, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(path + ".tmp", text);
            File.Move(path + ".tmp", path, overwrite: true);
            startupCatalogError = null;
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            startupCatalogError = $"The startup recording inventory could not be saved: {error.Message}";
            Trace.TraceWarning(error.ToString());
        }
    }

    // Time zero of the recording is when ETW started listening. For a launch, launcher is the process that created it.
    private SessionStart CreateSessionStart(int processId, string processName, long? creationTimeFileTime,
        (int ProcessId, string? Name)? launcher) =>
        new(processId, processName, creationTimeFileTime is { } created ? DateTime.FromFileTimeUtc(created) : null,
            Stopwatch.Frequency, etwListeningSinceQpc == 0 ? Stopwatch.GetTimestamp() : etwListeningSinceQpc,
            etwFromProcessStart && creationTimeFileTime >= etwListeningSinceFileTime
                ? SessionCoverage.ProcessStart : SessionCoverage.Connection,
            LauncherProcessId: launcher?.ProcessId, LauncherName: launcher?.Name);

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
                    await exited.ConfigureAwait(false);
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
            // Stop the source before the pipeline so nothing is posted to a finished queue, and the pipeline before the
            // recorder so every processed event is stored.
            etw?.Dispose();
        }
        catch (Exception error)
        {
            errors.Add(error);
        }

        try
        {
            // The samples still waiting are given to the recordings before they close and store what was sampled.
            var summary = cpu?.Stop();
            if (cpu is not null || cpuSamplingError is not null)
            {
                recordings?.SetCpuSampling(summary, CpuSamplingError);
            }
        }
        catch (Exception error)
        {
            errors.Add(error);
        }

        try
        {
            recordings?.Close(targetExited ? "Target exited" : Failure is not null ? "Connection failed" : "Disconnected",
                etw?.EventsLost);
        }
        catch (Exception error)
        {
            errors.Add(error);
        }

        SaveStartupRecordings(targetExited ? "Target exited" : Failure is not null ? "Connection failed" : "Disconnected");
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
        if (nativeLaunch is not null) nativeLaunch.Dispose();
        else target?.Dispose();
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

    // False when the target's UI thread didn't take the request (HRESULT_FROM_WIN32(ERROR_NOT_FOUND)).
    private static async Task<bool> InjectAsync(InjectorPayloads payloads, ushort endpoint, string bootstrap, CancellationToken token)
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
            if (helper.ExitCode == unchecked((int)0x80070490))
            {
                return false;
            }

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

            return true;
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

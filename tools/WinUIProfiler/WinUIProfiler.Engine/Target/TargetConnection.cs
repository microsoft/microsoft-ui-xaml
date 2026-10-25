using System.Diagnostics;
using System.Globalization;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Target;

public enum TargetLaunchKind { Unpackaged, Packaged, Attach }
public sealed record TargetConnectionOptions(TargetLaunchKind Kind, string Target, string Arguments,
    string? WorkingDirectory, string SessionDirectory, string PayloadDirectory, bool TrustLocalBinaries,
    ushort EndpointNumber = 1, bool DetailedEvents = false, long? ExpectedCreationTimeFileTime = null);
public sealed record ConnectionProgress(string State, string Detail);

[SupportedOSPlatform("windows")]
public sealed class TargetConnection : IAsyncDisposable
{
    private readonly BufferedRuntimeEvents events;
    private readonly CancellationTokenSource stop = new();
    private readonly object disposalGate = new();
    private readonly SemaphoreSlim commands = new(1, 1);
    private readonly SemaphoreSlim metricReaders = new(1, 1);
    private readonly IProgress<ConnectionProgress>? progress;
    private Process? target;
    private SessionStore? store;
    private ProcessingWorker? worker;
    private IDisposable? acquisition;
    private TapClient? tap;
    private NativeMetricSession? sampling;
    private QueryPage<MetricSample>? finalMetrics;
    private Task? lifetime;
    private Task? disposal;
    private bool disposed;
    private long committed;
    private long lastHelloTicks;
    private volatile string state = "Preparing";
    public SessionScope Scope { get; private set; }
    public string DatabasePath { get; private set; } = "";
    public string RuntimePath { get; private set; } = "";
    public bool HasRuntimeBridge { get; private set; }
    public string State => state;
    public Exception? Failure { get; private set; }
    public long ReceivedEvents => events.Received;
    public long ReceivedRuntimeEvents => events.RuntimeReceived;
    public long CommittedEvents => Interlocked.Read(ref committed);
    public int ListenerThreadId => events.ListenerThreadId;
    public int DecoderThreadId => events.DecoderThreadId;
    public int ProcessingThreadId => worker?.OwnerThreadId ?? 0;
    public (int Items, long Bytes) QueueUsage => events.QueueUsage;
    public DateTime? LastHelloUtc => Interlocked.Read(ref lastHelloTicks) is var ticks && ticks != 0
        ? new DateTime(ticks, DateTimeKind.Utc) : null;
    public Task Completion => lifetime ?? Task.CompletedTask;

    private TargetConnection(IProgress<ConnectionProgress>? progress, bool detailed)
    {
        this.progress = progress;
        events = new(detailed);
    }

    public static async Task<TargetConnection> ConnectAsync(TargetConnectionOptions options,
        IProgress<ConnectionProgress>? progress, CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(options);
        Require.Enum(options.Kind);
        ArgumentException.ThrowIfNullOrWhiteSpace(options.Target);
        ArgumentNullException.ThrowIfNull(options.Arguments);
        if (!options.TrustLocalBinaries)
            throw new UnauthorizedAccessException("Explicitly trust the local runtime, injector and TAP before connecting.");
        if (options.EndpointNumber == 0) throw new ArgumentOutOfRangeException(nameof(options.EndpointNumber));
        if (options.ExpectedCreationTimeFileTime is { } expected &&
            (options.Kind != TargetLaunchKind.Attach || expected <= 0))
            throw new ArgumentException("A selected process creation time requires running-process attach.", nameof(options));
        if (options.Kind == TargetLaunchKind.Attach &&
            (!int.TryParse(options.Target, NumberStyles.None, CultureInfo.InvariantCulture, out var pid) ||
                pid <= 0 || pid == Environment.ProcessId))
            throw new ArgumentException("Select another running process using its positive PID.", nameof(options));
        if (!Path.IsPathFullyQualified(options.SessionDirectory) || !Path.IsPathFullyQualified(options.PayloadDirectory))
            throw new ArgumentException("Session and native payload directories must be absolute local paths.", nameof(options));
        cancellationToken.ThrowIfCancellationRequested();
        var connection = new TargetConnection(progress, options.DetailedEvents);
        try
        {
            await connection.ConnectCoreAsync(options, cancellationToken).ConfigureAwait(false);
            return connection;
        }
        catch (Exception error)
        {
            var cancelled = error is OperationCanceledException && cancellationToken.IsCancellationRequested;
            connection.Failure = cancelled ? null : connection.worker?.StopRequested.IsCancellationRequested == true ?
                new IOException("Event processing stopped while preparing the connection.", error) : error is OperationCanceledException ?
                new TimeoutException("The 45-second launch/connection deadline expired.", error) : error;
            await connection.CleanupAsync(cancelled).ConfigureAwait(false);
            connection.stop.Dispose();
            if (cancelled && connection.Failure is null) throw;
            throw new InvalidOperationException($"Target connection failed. {connection.Failure?.Message} " +
                (connection.DatabasePath.Length > 0 ? $"Session: {connection.DatabasePath}" : ""), connection.Failure);
        }
    }

    private async Task ConnectCoreAsync(TargetConnectionOptions options, CancellationToken cancellationToken)
    {
        using var deadline = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        deadline.CancelAfter(TimeSpan.FromSeconds(45));
        var token = deadline.Token;
        Report("Listening", "Starting the dedicated ETW listener before launch/attach.");
        await events.StartAsync().WaitAsync(token).ConfigureAwait(false);
        token.ThrowIfCancellationRequested();
        var activationStarted = DateTime.UtcNow.ToFileTimeUtc();
        Report("Launching", options.Kind == TargetLaunchKind.Attach ? "Selecting the running target." : options.Target);
        target = options.Kind switch
        {
            TargetLaunchKind.Unpackaged => TargetLauncher.LaunchUnpackaged(options.Target, options.Arguments, options.WorkingDirectory),
            TargetLaunchKind.Packaged => TargetLauncher.LaunchPackaged(options.Target, options.Arguments),
            TargetLaunchKind.Attach => Process.GetProcessById(int.Parse(options.Target, CultureInfo.InvariantCulture)),
            _ => throw new ArgumentOutOfRangeException(nameof(options))
        };
        Scope = new(Guid.NewGuid(), LiveEtwSource.Identify(target));
        if (options.ExpectedCreationTimeFileTime is { } selectedCreation &&
            selectedCreation != Scope.Process.CreationTimeFileTime)
            throw new InvalidOperationException("The selected process exited or its PID was reused. Refresh and select the process again.");
        var architecture = InjectorResolver.GetArchitecture(Scope.Process);
        var payloads = ConnectionPayloadCatalog.Resolve(options.PayloadDirectory, architecture);
        Directory.CreateDirectory(options.SessionDirectory);
        DatabasePath = Path.GetFullPath(Path.Combine(options.SessionDirectory, $"{Scope.SessionId:N}.sqlite"));
        var mode = options.Kind == TargetLaunchKind.Attach ||
            options.Kind == TargetLaunchKind.Packaged && Scope.Process.CreationTimeFileTime <= activationStarted ?
            SessionMode.LiveAttach : SessionMode.LiveLaunch;
        var info = new SessionInfo(mode, SessionStatus.Recording, Stopwatch.Frequency, null, [], null);
        store = SessionStore.Create(DatabasePath, Scope, info, new(MeasurementKind.Measured, CoverageState.Partial,
            "Live observations only; complete pre-existing state and exact process-creation QPC are unavailable."));
        // Keep runtime observations source-only; metrics do not require a tree baseline.
        worker = new(store, info, processors: [new MetricProcessor(processOnly: true)]);
        acquisition = worker.RegisterAcquisition();
        using var processingStop = worker.StopRequested.Register(deadline.Cancel);
        events.Bind(Scope, worker);
        Report("Waiting for XAML", $"PID {Scope.Process.ProcessId}; waiting for the loaded WinUI diagnostics runtime.");
        string? runtime = null, udk = null;
        while (runtime is null || udk is null || !IsDiagnosticsReady(Scope.Process.ProcessId))
        {
            token.ThrowIfCancellationRequested();
            if (events.Failure.IsCompletedSuccessfully) throw await events.Failure.ConfigureAwait(false);
            if (target.HasExited) throw new IOException("The target exited before XAML diagnostics became ready.");
            target.Refresh();
            foreach (ProcessModule module in target.Modules)
            {
                if (module.ModuleName.Equals("Microsoft.UI.Xaml.dll", StringComparison.OrdinalIgnoreCase)) runtime = module.FileName;
                if (module.ModuleName.Equals("Microsoft.Internal.FrameworkUdk.dll", StringComparison.OrdinalIgnoreCase)) udk = module.FileName;
            }
            if (runtime is null || udk is null || !IsDiagnosticsReady(Scope.Process.ProcessId))
                await Task.Delay(100, token).ConfigureAwait(false);
        }
        RuntimePath = runtime;
        var resolved = InjectorResolver.Resolve(Scope.Process, new(payloads.Helper, payloads.Tap, runtime, udk), true);
        var pipe = TapClient.CreatePipeName();
        var nonce = RandomNumberGenerator.GetBytes(TapProtocol.NonceBytes);
        using var current = Process.GetCurrentProcess();
        var bootstrap = InjectorResolver.CreateBootstrap(pipe, Scope, LiveEtwSource.Identify(current), nonce);
        Report("Injecting TAP", $"PID {Scope.Process.ProcessId}; {architecture}; endpoint {options.EndpointNumber}.");
        await InjectAsync(resolved.Payloads, options.EndpointNumber, bootstrap, token).ConfigureAwait(false);
        Report("Authenticating pipe", "Verifying the target process, session and nonce.");
        tap = await TapClient.ConnectAsync(pipe, Scope, nonce, TimeSpan.FromSeconds(10), token).ConfigureAwait(false);
        var capabilities = await tap.SendAsync(TapMessageType.Capabilities, new CapabilitiesRequestBody(1),
            TimeSpan.FromSeconds(5), token).ConfigureAwait(false);
        if (capabilities.Body is not CapabilitiesBody runtimeCapabilities)
            throw new InvalidDataException("The target did not return runtime capabilities.");
        HasRuntimeBridge = runtimeCapabilities.Epoch.HasValue;
        Report("Verifying hello", "Profiler sends hello; waiting for hello from the target TAP.");
        if (await tap.HelloAsync(TimeSpan.FromSeconds(5), token).ConfigureAwait(false) != "hello")
            throw new InvalidDataException("The target TAP did not reply hello.");
        Interlocked.Exchange(ref lastHelloTicks, DateTime.UtcNow.Ticks);
        if (events.Failure.IsCompletedSuccessfully) throw await events.Failure.ConfigureAwait(false);
        token.ThrowIfCancellationRequested();
        var emitter = new CollectorMetricEmitter(Scope);
        var firstSample = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var nativePath = Path.GetFullPath(Path.Combine(options.PayloadDirectory, "..", "WinUIProfiler.Native.dll"));
        sampling = new(nativePath, Scope.Process, TimeSpan.FromSeconds(1), batch =>
        {
            if (!emitter.TryEmit(batch)) return false;
            firstSample.TrySetResult();
            return true;
        }, gpuMemoryAdapters: []);
        if (!emitter.TryEmit(new ProcessPayload(ProcessOperation.Attach, null,
            new(MeasurementKind.Measured, CoverageState.Partial,
                "Process metrics attached after TAP hello; startup and GPU-memory byte gauges are outside this capture."))))
            throw new IOException("Collector lifecycle publication was rejected.");
        sampling.Start();
        var started = await Task.WhenAny(firstSample.Task, sampling.Completion).WaitAsync(token).ConfigureAwait(false);
        await started.ConfigureAwait(false);
        Report("Connected", $"hello received from target PID {Scope.Process.ProcessId}; " +
            $"XAML {(options.DetailedEvents ? "detailed" : "summary")} ETW and process metrics active." +
            (HasRuntimeBridge ? "" : " Private producer captures are unavailable; TAP tree snapshots are separate."));
        lifetime = MaintainAsync();
    }

    private async Task MaintainAsync()
    {
        try
        {
            var exited = target!.WaitForExitAsync(stop.Token);
            var processingStopped = Task.Delay(Timeout.InfiniteTimeSpan, worker!.StopRequested);
            while (!stop.IsCancellationRequested)
            {
                var delay = Task.Delay(TimeSpan.FromSeconds(5), stop.Token);
                var completed = await Task.WhenAny(delay, exited, events.Failure, processingStopped,
                    sampling!.Completion).ConfigureAwait(false);
                if (completed == sampling.Completion)
                {
                    await ObserveSamplingAsync(sampling.Completion).ConfigureAwait(false);
                    if (sampling.TerminationStatus != MetricSampleStatus.TargetMissing)
                        throw new IOException("Native metric polling ended without a stop request.");
                    Report("Target exited", "Native process handle signaled exit; draining accepted observations.");
                    break;
                }
                if (completed == exited)
                {
                    await exited.ConfigureAwait(false);
                    Report("Target exited", "Stopping the connection and draining accepted events.");
                    break;
                }
                if (completed == events.Failure) throw await events.Failure.ConfigureAwait(false);
                if (completed == processingStopped)
                    throw new IOException("Event processing stopped; the recording is incomplete.");
                await delay.ConfigureAwait(false);
                events.PollLoss();
                await commands.WaitAsync(stop.Token).ConfigureAwait(false);
                try
                {
                    await tap!.HelloAsync(TimeSpan.FromSeconds(5), stop.Token).ConfigureAwait(false);
                    Interlocked.Exchange(ref lastHelloTicks, DateTime.UtcNow.Ticks);
                }
                finally { commands.Release(); }
                Interlocked.Exchange(ref committed, store!.Watermark.IngestOrdinal);
            }
        }
        catch (OperationCanceledException) when (stop.IsCancellationRequested) { }
        catch (Exception error) { Failure = error; }
        finally
        {
            await stop.CancelAsync().ConfigureAwait(false);
            await commands.WaitAsync().ConfigureAwait(false);
            try { await CleanupAsync().ConfigureAwait(false); }
            finally { commands.Release(); }
        }
    }

    public Task<QueryPage<MetricSample>> ReadRecentMetricsAsync(CancellationToken cancellationToken = default) =>
        Task.Run(async () =>
        {
            await metricReaders.WaitAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                cancellationToken.ThrowIfCancellationRequested();
                return finalMetrics ?? await ReadMetricsAsync(cancellationToken).ConfigureAwait(false);
            }
            finally { metricReaders.Release(); }
        }, cancellationToken);

    private async Task<QueryPage<MetricSample>> ReadMetricsAsync(CancellationToken cancellationToken)
    {
        var selected = store ?? throw new InvalidOperationException("The metric session has not been created.");
        var session = await selected.Queries.GetSession(new(Scope), cancellationToken).ConfigureAwait(false);
        var end = checked(session.Context.Watermark.TimestampQpc + 1);
        var start = Math.Max(0, end - checked(session.Value.QpcFrequency * 120));
        var page = await selected.Queries.GetMetrics(new(Scope, new(start, end),
            new(1024, AtWatermark: session.Context.Watermark)), cancellationToken).ConfigureAwait(false);
        if (page.Page.HasMore)
            throw new InvalidDataException("The live process metric window exceeded its bounded sample budget.");
        return page;
    }

    private async Task ObserveSamplingAsync(Task completion)
    {
        try { await completion.ConfigureAwait(false); }
        catch (InvalidOperationException error) when (sampling?.TerminationStatus == MetricSampleStatus.TargetMissing &&
            ReferenceEquals(error, sampling.Completion.Exception?.InnerException)) { }
    }

    public async Task<SavedTreeSnapshot> TakeSnapshotAsync(CancellationToken cancellationToken = default)
    {
        using var cancelled = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, stop.Token);
        await commands.WaitAsync(cancelled.Token).ConfigureAwait(false);
        try
        {
            cancelled.Token.ThrowIfCancellationRequested();
            if (tap is null || tap.IsClosed || Completion.IsCompleted)
                throw new InvalidOperationException("An authenticated live TAP connection is required for a tree snapshot.");
            var document = await tap.TakeSnapshotAsync(TimeSpan.FromSeconds(45), cancelled.Token).ConfigureAwait(false);
            var bytes = JsonSerializer.SerializeToUtf8Bytes(document, WireJson.Options);
            _ = TreeSnapshotCodec.Decode(bytes, Scope.Process, document.CaptureId);
            var path = Path.ChangeExtension(DatabasePath, $"{document.CaptureId:N}.trees.json");
            var temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
            var created = false;
            try
            {
                await using (var file = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None,
                    64 * 1024, FileOptions.Asynchronous | FileOptions.WriteThrough))
                {
                    created = true;
                    await file.WriteAsync(bytes, cancelled.Token).ConfigureAwait(false);
                    await file.FlushAsync(cancelled.Token).ConfigureAwait(false);
                    file.Flush(flushToDisk: true);
                }
                cancelled.Token.ThrowIfCancellationRequested();
                File.Move(temporary, path);
            }
            finally { if (created && File.Exists(temporary)) File.Delete(temporary); }
            return new(path, document.CaptureId, document.Roots.Count, document.XamlNodes.Count,
                document.CompositionNodes.Count, document.Correlations.Count);
        }
        finally { commands.Release(); }
    }

    private async Task CleanupAsync(bool cancelled = false)
    {
        var errors = new List<Exception>();
        if (Failure is not null) errors.Add(Failure);
        if (sampling is not null)
        {
            try { await ObserveSamplingAsync(sampling.StopAsync()).ConfigureAwait(false); }
            catch (Exception error) { errors.Add(error); }
            try { await ObserveSamplingAsync(sampling.DisposeAsync().AsTask()).ConfigureAwait(false); }
            catch (Exception error) { errors.Add(error); }
        }
        if (tap is not null)
        {
            try
            {
                if (!cancelled && !tap.IsClosed && target is not null && !target.HasExited && Failure is null)
                    await tap.SendAsync(TapMessageType.Disconnect, new DisconnectBody("Profiler disconnected."),
                        TimeSpan.FromSeconds(5), CancellationToken.None).ConfigureAwait(false);
            }
            catch (Exception error) { errors.Add(error); }
            try { await tap.DisposeAsync().ConfigureAwait(false); }
            catch (Exception error) { errors.Add(error); }
        }
        try { await events.DisposeAsync().ConfigureAwait(false); }
        catch (Exception error) { errors.Add(error); }
        if (worker is not null)
        {
            try
            {
                if (errors.Count > 0 && !worker.StopRequested.IsCancellationRequested)
                    worker.Report(new(IngestionIssue.SourceFailure, errors[0].Message[..Math.Min(errors[0].Message.Length, 768)]));
            }
            catch (Exception error) { errors.Add(error); }
            acquisition?.Dispose();
            if (!worker.StopRequested.IsCancellationRequested)
                worker.Complete(errors.Count > 0 ? SourceCompletion.Failed :
                    cancelled ? SourceCompletion.Cancelled : SourceCompletion.UserStop);
            try
            {
                var result = await worker.Completion.ConfigureAwait(false);
                Interlocked.Exchange(ref committed, result.DurableBoundary.IngestOrdinal);
                if (result.Status is SessionStatus.Failed or SessionStatus.StoppedAtLimit)
                    errors.Add(new IOException(result.StopReason ?? "Recording failed."));
            }
            catch (Exception error) { errors.Add(error); }
            try { await worker.DisposeAsync().ConfigureAwait(false); }
            catch (Exception error) { errors.Add(error); }
        }
        await metricReaders.WaitAsync().ConfigureAwait(false);
        try
        {
            if (store is not null)
            {
                try { finalMetrics = await ReadMetricsAsync(CancellationToken.None).ConfigureAwait(false); }
                catch (Exception error) { errors.Add(error); }
                try { store.Dispose(); }
                catch (Exception error) { errors.Add(error); }
                store = null;
            }
        }
        finally { metricReaders.Release(); }
        target?.Dispose();
        Failure = errors.Count switch { 0 => null, 1 => errors[0], _ => new AggregateException(errors) };
        Report(Failure is null ? "Disconnected" : "Failed", Failure?.Message ?? "Accepted events saved; target was not terminated.");
    }

    private static async Task InjectAsync(InjectorPayloads payloads, ushort endpoint, string bootstrap,
        CancellationToken cancellationToken)
    {
        var start = new ProcessStartInfo(payloads.Helper)
        {
            UseShellExecute = false, CreateNoWindow = true, RedirectStandardInput = true,
            RedirectStandardOutput = true, RedirectStandardError = true
        };
        foreach (var argument in new[] { payloads.Runtime, payloads.FrameworkUdk, payloads.Tap,
            endpoint.ToString(CultureInfo.InvariantCulture), "--trust-local-binaries" })
            start.ArgumentList.Add(argument);
        using var helper = Process.Start(start) ?? throw new IOException("The injector helper did not start.");
        try
        {
            var output = ReadBoundedAsync(helper.StandardOutput, cancellationToken);
            var errors = ReadBoundedAsync(helper.StandardError, cancellationToken);
            await helper.StandardInput.WriteLineAsync(bootstrap.AsMemory(), cancellationToken).ConfigureAwait(false);
            helper.StandardInput.Close();
            await helper.WaitForExitAsync(cancellationToken).ConfigureAwait(false);
            var text = await output.ConfigureAwait(false);
            var errorText = await errors.ConfigureAwait(false);
            if (helper.ExitCode != 0)
                throw new IOException($"TAP injection failed (0x{unchecked((uint)helper.ExitCode):X8}). {errorText}");
            using var result = JsonDocument.Parse(text);
            if (result.RootElement.GetProperty("schemaVersion").GetInt32() != 1 ||
                result.RootElement.GetProperty("hresult").GetUInt32() != 0 ||
                !result.RootElement.GetProperty("injectionAccepted").GetBoolean())
                throw new InvalidDataException("Injector returned an invalid acceptance result.");
        }

            finally
        {
            if (!helper.HasExited) helper.Kill(entireProcessTree: true);
            await helper.WaitForExitAsync().ConfigureAwait(false);
        }
    }

    private static async Task<string> ReadBoundedAsync(StreamReader reader, CancellationToken cancellationToken)
    {
        var value = new System.Text.StringBuilder();
        var buffer = new char[4096];
        int count;
        while ((count = await reader.ReadAsync(buffer.AsMemory(), cancellationToken).ConfigureAwait(false)) != 0)
        {
            if (value.Length + count > 32768) throw new InvalidDataException("Injector output exceeds its bound.");
            value.Append(buffer, 0, count);
        }
        return value.ToString();
    }

    private static bool IsDiagnosticsReady(uint pid)
    {
        // WinUI signals this only after registering its diagnostics message port.
        if (!EventWaitHandle.TryOpenExisting("WinUIDebuggable" + pid.ToString(CultureInfo.InvariantCulture), out var ready))
            return false;
        using (ready) return ready.WaitOne(0);
    }

    private void Report(string value, string detail)
    {
        state = value;
        progress?.Report(new(value, detail));
    }

    public async Task StopAsync()
    {
        Task cancellation;
        lock (disposalGate)
        {
            if (disposed) return;
            cancellation = stop.CancelAsync();
        }
        await cancellation.ConfigureAwait(false);
        if (lifetime is not null) await lifetime.ConfigureAwait(false);
        if (Failure is not null) throw new IOException("Target connection ended with an error.", Failure);
    }

    public ValueTask DisposeAsync()
    {
        lock (disposalGate) return new(disposal ??= DisposeCoreAsync());
    }

    private async Task DisposeCoreAsync()
    {
        try { await StopAsync().ConfigureAwait(false); }
        finally { lock (disposalGate) { disposed = true; stop.Dispose(); } }
    }
}

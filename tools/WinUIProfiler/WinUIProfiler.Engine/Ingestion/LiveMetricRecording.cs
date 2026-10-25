using System.Diagnostics;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Ingestion;

public sealed class LiveMetricRecording : IAsyncDisposable
{
    private readonly ProcessingWorker worker;
    private readonly SessionScope scope;
    private readonly Func<SessionScope, long, LiveEtwSource> createSource;
    private readonly Func<Func<NativeMetricBatch, bool>, NativeMetricSession> createSampling;
    private readonly long frequency;
    private readonly object gate = new();
    private readonly CancellationTokenSource stop = new();
    private readonly TaskCompletionSource ready = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private Task? run;
    private Task? disposal;
    private bool stopped;
    private bool disposed;

    public LiveMetricRecording(SessionStore store, SessionInfo info, string nativeLibraryPath,
        TimeSpan interval, ProcessingOptions? options = null, IReadOnlyList<IEventProcessor>? processors = null,
        IReadOnlyList<PhysicalAdapterKey>? gpuMemoryAdapters = null)
        : this(store, info, nativeLibraryPath, interval, options, processors, null, null, gpuMemoryAdapters) { }

    internal LiveMetricRecording(SessionStore store, SessionInfo info, string nativeLibraryPath,
        TimeSpan interval, ProcessingOptions? options, IReadOnlyList<IEventProcessor>? processors,
        Func<SessionScope, long, LiveEtwSource>? sourceFactory,
        Func<Func<NativeMetricBatch, bool>, NativeMetricSession>? samplingFactory,
        IReadOnlyList<PhysicalAdapterKey>? gpuMemoryAdapters = null)
    {
        if (info.Mode == SessionMode.Offline)
            throw new ArgumentException("Live metric recording cannot own an offline session.", nameof(info));
        ArgumentException.ThrowIfNullOrWhiteSpace(nativeLibraryPath);
        scope = store.Scope;
        frequency = info.QpcFrequency;
        var selectedAdapters = gpuMemoryAdapters is null ? null : NativeMetricSession.SnapshotGpuMemoryAdapters(gpuMemoryAdapters);
        createSource = sourceFactory ?? ((selected, clock) => new(selected, clock));
        createSampling = samplingFactory ?? (publish => new(nativeLibraryPath, scope.Process, interval, publish, selectedAdapters));
        worker = new(store, info, options, processors);
    }

    public Task Started => ready.Task;
    public Task<ProcessingResult> Completion => worker.Completion;
    public void Start(CancellationToken cancellationToken = default)
    {
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(disposal is not null || disposed, this);
            if (run is not null || stopped) throw new InvalidOperationException("A recording can only be started once.");
            var lifetime = worker.RegisterAcquisition();
            run = Task.Run(() => AcquireAsync(lifetime, cancellationToken));
        }
    }

    private async Task AcquireAsync(IDisposable lifetime, CancellationToken cancellationToken)
    {
        using var acquisition = lifetime;
        using var linked = CancellationTokenSource.CreateLinkedTokenSource(
            cancellationToken, stop.Token, worker.StopRequested);
        var failures = new HashSet<Exception>(ReferenceEqualityComparer.Instance);
        var begin = Stopwatch.GetTimestamp();
        NativeMetricSession? sampling = null;
        LiveEtwSource? source = null;
        CollectorMetricEmitter? emitter = null;
        Task? transport = null;
        var targetExited = false;
        void Fail(Exception error)
        {
            if (!failures.Add(error)) return;
            ready.TrySetException(error);
            worker.Report(new(IngestionIssue.SourceFailure, "Live metric recording failed: " + error.GetType().Name));
        }
        void RequirePublication(bool accepted)
        {
            if (!accepted) throw new InvalidOperationException("Collector rejected recording lifecycle publication.");
        }
        async Task ObserveSamplingAsync(Task completion)
        {
            try { await completion.ConfigureAwait(false); }
            catch (InvalidOperationException error) when (sampling?.TerminationStatus == MetricSampleStatus.TargetMissing &&
                ReferenceEquals(error, sampling.Completion.Exception?.InnerException))
            { targetExited = true; }
        }
        try
        {
            linked.Token.ThrowIfCancellationRequested();
            emitter = new(scope);
            sampling = createSampling(batch =>
            {
                if (!emitter.TryEmit(batch)) return false;
                ready.TrySetResult();
                return true;
            });
            linked.Token.ThrowIfCancellationRequested();
            source = createSource(scope, frequency);
            transport = source.RunComponentAsync(worker);
            await source.Started.ConfigureAwait(false);
            linked.Token.ThrowIfCancellationRequested();
            RequirePublication(emitter.TryEmit(new ProcessPayload(ProcessOperation.Attach, null,
                new(MeasurementKind.Measured, CoverageState.Partial,
                    "Collector attached; process creation QPC is unknown. " + sampling.GpuMemoryCoverage))));
            sampling.Start();
            var cancelled = Task.Delay(Timeout.InfiniteTimeSpan, linked.Token);
            var completed = await Task.WhenAny(cancelled, sampling.Completion, transport, source.ControlFailure).ConfigureAwait(false);
            if (completed == sampling.Completion)
            {
                await ObserveSamplingAsync(sampling.Completion).ConfigureAwait(false);
                if (!targetExited) throw new IOException("Native metric polling ended without a stop request.");
            }
            else if (completed == transport)
            {
                await transport.ConfigureAwait(false);
                throw new IOException("Live ETW transport ended before native polling was stopped.");
            }
            else if (completed == source.ControlFailure)
                Fail(await source.ControlFailure.ConfigureAwait(false));
        }
        catch (OperationCanceledException) when (linked.IsCancellationRequested)
        { ready.TrySetCanceled(linked.Token); }
        catch (Exception error) { Fail(error); }
        finally
        {
            if (sampling is not null)
            {
                try { await ObserveSamplingAsync(sampling.StopAsync()).ConfigureAwait(false); }
                catch (Exception error) { Fail(error); }
                try { await ObserveSamplingAsync(sampling.DisposeAsync().AsTask()).ConfigureAwait(false); }
                catch (Exception error) { Fail(error); }
            }
            if (source?.ControlFailure.IsCompletedSuccessfully == true)
                Fail(await source.ControlFailure.ConfigureAwait(false));
            if (emitter is not null && source?.Started.IsCompletedSuccessfully == true &&
                transport?.IsCompleted == false)
            {
                try
                {
                    if (targetExited)
                        RequirePublication(emitter.TryEmit(new ProcessPayload(ProcessOperation.Exit, null,
                            new(MeasurementKind.Measured, CoverageState.Partial, "Native selected-process handle signaled exit."))));
                    if (failures.Count > 0)
                        RequirePublication(emitter.TryEmit(new LossPayload(emitter.Stream.ProducerEpoch,
                            new(begin, Stopwatch.GetTimestamp()), null, "Native/live acquisition failed; metric coverage is incomplete.",
                            [new(emitter.Stream, scope.Process, 2, 1)])));
                }
                catch (Exception error) { Fail(error); }
            }
            if (source is not null)
            {
                try { await source.StopAsync().ConfigureAwait(false); }
                catch (Exception error) { Fail(error); }
                try { await source.DisposeAsync().ConfigureAwait(false); }
                catch (Exception error) { Fail(error); }
            }
            if (!ready.Task.IsCompleted)
            {
                if (linked.IsCancellationRequested) ready.TrySetCanceled(linked.Token);
                else Fail(new IOException("Recording ended before its first native metric publication."));
            }
            worker.Complete(failures.Count > 0 ? SourceCompletion.Failed :
                cancellationToken.IsCancellationRequested ? SourceCompletion.Cancelled : SourceCompletion.UserStop);
        }
    }

    public async Task<ProcessingResult> StopAsync()
    {
        Task? active;
        Task cancellation;
        lock (gate)
        {
            stopped = true;
            active = run;
            cancellation = disposed ? Task.CompletedTask : stop.CancelAsync();
            if (active is null)
            {
                ready.TrySetCanceled(new CancellationToken(true));
                worker.Complete(SourceCompletion.UserStop);
            }
        }
        await cancellation.ConfigureAwait(false);
        if (active is not null) await active.ConfigureAwait(false);
        return await worker.Completion.ConfigureAwait(false);
    }

    public ValueTask DisposeAsync()
    {
        lock (gate) return new(disposal ??= DisposeCoreAsync());
    }

    private async Task DisposeCoreAsync()
    {
        try { await StopAsync().ConfigureAwait(false); }
        finally
        {
            try { await worker.DisposeAsync().ConfigureAwait(false); }
            finally
            {
                lock (gate)
                {
                    disposed = true;
                    stop.Dispose();
                }
            }
        }
    }
}

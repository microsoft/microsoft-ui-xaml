using System.Diagnostics;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.Engine.Ingestion;

public sealed class LiveEtwSource : IAsyncDisposable
{
    private readonly SessionScope scope;
    private readonly long sessionFrequency;
    private readonly CancellationTokenSource stop = new();
    private readonly TaskCompletionSource ready = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private readonly TaskCompletionSource<Exception> controlFailed = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private readonly Func<string, (long Events, long Buffers)> queryStatistics;
    private readonly Func<string, (long Events, long Buffers)> stopSession;
    private Task? run;
    public string SessionName { get; } = "WinUIProfiler-" + Guid.NewGuid().ToString("N");
    public Task Started => ready.Task;
    internal Task<Exception> ControlFailure => controlFailed.Task;
    public LiveEtwSource(SessionScope scope, long sessionFrequency) :
        this(scope, sessionFrequency, EtwSessionStatistics.Query, EtwSessionStatistics.Stop) { }

    internal LiveEtwSource(SessionScope scope, long sessionFrequency,
        Func<string, (long Events, long Buffers)> queryStatistics,
        Func<string, (long Events, long Buffers)> stopSession)
    {
        scope.Validate(); Require.Positive(sessionFrequency);
        if (scope.Process.CreationTimeFileTime is null) throw new ArgumentException("Live selection requires actual process creation evidence.");
        this.scope = scope; this.sessionFrequency = sessionFrequency;
        this.queryStatistics = queryStatistics;
        this.stopSession = stopSession;
    }
    public static ProcessInstanceKey Identify(Process process) =>
        new(checked((uint)process.Id), process.StartTime.ToFileTimeUtc(), Guid.NewGuid());

    public Task RunAsync(ProcessingWorker worker, CancellationToken cancellationToken = default) =>
        RunAsync(worker, cancellationToken, true);

    internal Task RunComponentAsync(ProcessingWorker worker) =>
        RunAsync(worker, CancellationToken.None, false);

    private Task RunAsync(ProcessingWorker worker, CancellationToken cancellationToken, bool ownsLifetime)
    {
        lock (ready)
        {
            if (run is not null) throw new InvalidOperationException("A live source owns only one transport lifetime.");
            var lifetime = ownsLifetime ? worker.RegisterAcquisition() : null;
            run = Task.Factory.StartNew(() =>
            {
                using var acquisition = lifetime;
                Acquire(worker, cancellationToken, ownsLifetime);
            }, CancellationToken.None, TaskCreationOptions.LongRunning, TaskScheduler.Default);
            return run;
        }
    }
    private void Acquire(ProcessingWorker worker, CancellationToken cancellationToken, bool ownsLifetime)
    {
        try
        {
            using var session = new TraceEventSession(SessionName, TraceEventSessionOptions.Create | TraceEventSessionOptions.NoRestartOnCreate)
            { BufferSizeMB = 16, BufferQuantumKB = 64, EnableProviderTimeoutMSec = 10_000 };
            var source = session.Source;
            var loss = new EtwLossTracker(worker);
            var decoder = new EventDecoder(scope, new(Stopwatch.Frequency, sessionFrequency));
            worker.RegisterSourceClock(decoder.SourceClock);
            EtwAcquisition.Subscribe(source, decoder, worker);
            session.EnableProvider(EventCatalog.RuntimeProviderId, TraceEventLevel.Informational, 0xff);
            session.EnableProvider(EventCatalog.CollectorProviderId, TraceEventLevel.Informational, 0x101);
            using var linked = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, stop.Token,
                ownsLifetime ? worker.StopRequested : CancellationToken.None);
            var sessionGate = new object();
            var stopping = false;
            Exception? controlFailure = null;
            void StopOwned()
            {
                var counts = stopSession(SessionName);
                session.StopOnDispose = false;
                loss.Update(counts.Events, counts.Buffers);
            }
            void Poll()
            {
                lock (sessionGate)
                {
                    if (stopping || controlFailure is not null) return;
                    try { var counts = queryStatistics(SessionName); loss.Update(counts.Events, counts.Buffers); }
                    catch (Exception error)
                    {
                        controlFailure = error;
                        worker.Report(new(IngestionIssue.SourceFailure, "Owned ETW loss query failed: " + error.GetType().Name));
                        controlFailed.TrySetResult(error);
                        if (ownsLifetime) _ = stop.CancelAsync();
                    }
                }
            }
            using var timer = new Timer(_ => Poll(), null, 500, 500);
            using var registration = linked.Token.Register(() => ThreadPool.QueueUserWorkItem(_ =>
            {
                lock (sessionGate)
                {
                    if (stopping) return;
                    Poll(); stopping = true;
                    try { StopOwned(); }
                    catch (Exception error) { controlFailure = error; source.StopProcessing(); }
                }
            }));
            ready.TrySetResult();
            try { source.Process(); }
            finally
            {
                lock (sessionGate)
                {
                    if (!stopping) { Poll(); stopping = true; StopOwned(); }
                }
            }
            if (controlFailure is not null) throw new IOException("Owned ETW control/loss monitoring failed.", controlFailure);
            if (ownsLifetime)
                worker.Complete(cancellationToken.IsCancellationRequested ? SourceCompletion.Cancelled : SourceCompletion.UserStop);
        }
        catch (Exception error)
        {
            ready.TrySetException(error);
            worker.Report(new(IngestionIssue.SourceFailure, "Live ETW acquisition failed: " + error.GetType().Name));
            if (ownsLifetime) worker.Complete(SourceCompletion.Failed);
            throw;
        }
    }
    public async Task StopAsync()
    {
        await stop.CancelAsync().ConfigureAwait(false);
        Task? active;
        lock (ready) active = run;
        if (active is not null) await active.ConfigureAwait(false);
    }
    public async ValueTask DisposeAsync()
    {
        try { await StopAsync().ConfigureAwait(false); }
        finally { stop.Dispose(); }
    }
}

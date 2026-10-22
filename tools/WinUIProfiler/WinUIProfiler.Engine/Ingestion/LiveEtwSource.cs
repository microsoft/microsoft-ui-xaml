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
    private Task? run;
    public string SessionName { get; } = "WinUIProfiler-" + Guid.NewGuid().ToString("N");
    public Task Started => ready.Task;
    public LiveEtwSource(SessionScope scope, long sessionFrequency)
    {
        scope.Validate(); Require.Positive(sessionFrequency);
        if (scope.Process.CreationTimeFileTime is null) throw new ArgumentException("Live selection requires actual process creation evidence.");
        this.scope = scope; this.sessionFrequency = sessionFrequency;
    }
    public static ProcessInstanceKey Identify(Process process) =>
        new(checked((uint)process.Id), process.StartTime.ToFileTimeUtc(), Guid.NewGuid());

    public Task RunAsync(ProcessingWorker worker, CancellationToken cancellationToken = default)
    {
        lock (ready)
        {
            if (run is not null) throw new InvalidOperationException("A live source owns only one transport lifetime.");
            var lifetime = worker.RegisterAcquisition();
            run = Task.Factory.StartNew(() =>
            {
                using var acquisition = lifetime;
                Acquire(worker, cancellationToken);
            }, CancellationToken.None, TaskCreationOptions.LongRunning, TaskScheduler.Default);
            return run;
        }
    }
    private void Acquire(ProcessingWorker worker, CancellationToken cancellationToken)
    {
        try
        {
            using var session = new TraceEventSession(SessionName, TraceEventSessionOptions.Create | TraceEventSessionOptions.NoRestartOnCreate)
            { BufferSizeMB = 16, BufferQuantumKB = 64 };
            var source = session.Source;
            var loss = new EtwLossTracker(worker);
            var decoder = new EventDecoder(scope, new(Stopwatch.Frequency, sessionFrequency));
            EtwAcquisition.Subscribe(source, decoder, worker);
            session.EnableProvider(EventCatalog.RuntimeProviderId, TraceEventLevel.Informational, 0xff);
            session.EnableProvider(EventCatalog.CollectorProviderId, TraceEventLevel.Informational, 0x101);
            using var linked = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, stop.Token, worker.StopRequested);
            var sessionGate = new object();
            var stopping = false;
            Exception? controlFailure = null;
            void StopOwned()
            {
                var counts = EtwSessionStatistics.Stop(SessionName);
                session.StopOnDispose = false;
                loss.Update(counts.Events, counts.Buffers);
            }
            void Poll()
            {
                lock (sessionGate)
                {
                    if (stopping) return;
                    try { var counts = EtwSessionStatistics.Query(SessionName); loss.Update(counts.Events, counts.Buffers); }
                    catch (Exception error)
                    {
                        controlFailure = error;
                        worker.Report(new(IngestionIssue.SourceFailure, "Owned ETW loss query failed: " + error.GetType().Name));
                        _ = stop.CancelAsync();
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
            worker.Complete(cancellationToken.IsCancellationRequested ? SourceCompletion.Cancelled : SourceCompletion.UserStop);
        }
        catch (Exception error)
        {
            ready.TrySetException(error);
            worker.Report(new(IngestionIssue.SourceFailure, "Live ETW acquisition failed: " + error.GetType().Name));
            worker.Complete(SourceCompletion.Failed);
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

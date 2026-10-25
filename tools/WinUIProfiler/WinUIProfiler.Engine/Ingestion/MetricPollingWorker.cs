namespace WinUIProfiler.Engine.Ingestion;

internal sealed class MetricPollingWorker : IAsyncDisposable
{
    private readonly object gate = new();
    private readonly TimeSpan interval;
    private readonly Action capture;
    private readonly CancellationTokenSource stop = new();
    private Task completion = Task.CompletedTask;
    private bool started;
    private bool stopped;
    private bool disposed;

    internal MetricPollingWorker(TimeSpan interval, Action capture)
    {
        if (interval < TimeSpan.FromMilliseconds(1) || interval.TotalMilliseconds > int.MaxValue)
            throw new ArgumentOutOfRangeException(nameof(interval), "Polling delay must be between 1 and Int32.MaxValue milliseconds.");
        ArgumentNullException.ThrowIfNull(capture);
        this.interval = interval;
        this.capture = capture;
    }

    internal Task Completion
    {
        get { lock (gate) return completion; }
    }

    internal void Start()
    {
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(disposed, this);
            if (started || stopped) throw new InvalidOperationException("Metric polling can only be started once.");
            started = true;
            completion = Task.Factory.StartNew(Run, CancellationToken.None,
                TaskCreationOptions.LongRunning, TaskScheduler.Default);
        }
    }

    private void Run()
    {
        while (!stop.IsCancellationRequested)
        {
            capture();
            if (stop.Token.WaitHandle.WaitOne(interval)) break;
        }
    }

    internal Task StopAsync()
    {
        lock (gate)
        {
            stopped = true;
            if (!disposed) stop.Cancel();
            return completion;
        }
    }

    public async ValueTask DisposeAsync()
    {
        try { await StopAsync().ConfigureAwait(false); }
        finally
        {
            lock (gate)
            {
                if (!disposed)
                {
                    disposed = true;
                    stop.Dispose();
                }
            }
        }
    }
}

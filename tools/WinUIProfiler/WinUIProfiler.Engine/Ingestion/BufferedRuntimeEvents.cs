using System.Diagnostics;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed class BufferedRuntimeEvents : IAsyncDisposable
{
    internal const int MaximumBufferedEvents = 16384;
    internal const long MaximumBufferedBytes = 64 * 1024 * 1024;
    private readonly BoundedQueue<NativeRecord> incoming = new(MaximumBufferedEvents, MaximumBufferedBytes);
    private readonly TaskCompletionSource ready = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private readonly TaskCompletionSource ended = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private readonly TaskCompletionSource<Exception> failure = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private readonly object gate = new();
    private readonly Thread listener;
    private TraceEventSession? session;
    private ProcessingWorker? worker;
    private EtwLossTracker? loss;
    private Task? decoding;
    private Task? disposal;
    private int selectedPid;
    private int stopping;
    private long received;
    private long runtimeReceived;
    internal ulong SelectedKeywords { get; }
    internal ulong StandardXamlKeywords { get; }
    internal TraceEventLevel StandardXamlLevel { get; }
    public string SessionName { get; } = "WinUIProfiler-Connect-" + Guid.NewGuid().ToString("N");
    public Task<Exception> Failure => failure.Task;
    public long Received => Interlocked.Read(ref received);
    public long RuntimeReceived => Interlocked.Read(ref runtimeReceived);
    public int ListenerThreadId { get; private set; }
    public int DecoderThreadId { get; private set; }
    public (int Items, long Bytes) QueueUsage => incoming.Usage;

    public BufferedRuntimeEvents(bool detailed = false)
    {
        CollectorMetricEmitter.EnsureRegistered();
        SelectedKeywords = detailed ? 0x3fUL : 0x11UL;
        StandardXamlKeywords = detailed ? ulong.MaxValue : 0x1044UL;
        StandardXamlLevel = detailed ? TraceEventLevel.Verbose : TraceEventLevel.Informational;
        listener = new(Listen) { IsBackground = true, Name = $"WinUIProfiler ETW listener {SessionName}" };
    }

    public Task StartAsync()
    {
        listener.Start();
        return ready.Task;
    }

    public void Bind(SessionScope scope, ProcessingWorker owner)
    {
        if (decoding is not null) throw new InvalidOperationException("ETW selection can only be bound once.");
        if (!ready.Task.IsCompletedSuccessfully) throw new InvalidOperationException("ETW must be ready before target selection.");
        worker = owner;
        loss = new(owner);
        Volatile.Write(ref selectedPid, checked((int)scope.Process.ProcessId));
        var decoder = new EventDecoder(scope, new(Stopwatch.Frequency, Stopwatch.Frequency));
        owner.RegisterSourceClock(decoder.SourceClock);
        decoding = Task.Factory.StartNew(() =>
        {
            DecoderThreadId = Environment.CurrentManagedThreadId;
            try
            {
                while (incoming.Take(out var record, out _))
                {
                    try
                    {
                        if (decoder.Decode(record) is { } observation)
                        {
                            if (!owner.TryEnqueue(observation))
                            {
                                Fail(new IOException("Target event processing stopped accepting events; the uncommitted tail is incomplete."));
                                break;
                            }
                            Interlocked.Increment(ref received);
                            if (record.ProviderId != EventCatalog.CollectorProviderId)
                                Interlocked.Increment(ref runtimeReceived);
                        }
                    }
                    catch (Exception error) when (error is ArgumentException or InvalidDataException or
                        NotSupportedException or OverflowException or System.Text.Json.JsonException)
                    {
                        owner.Report(new(error is NotSupportedException ? IngestionIssue.UnknownSchema : IngestionIssue.DecodeFailure,
                            error.Message[..Math.Min(error.Message.Length, 768)], record.ProviderId, record.Name,
                            record.ProducerProcessId, record.TimestampQpc, Convert.ToHexString(record.PayloadSample.Span)));
                    }
                }
            }
            catch (Exception error) { Fail(error); }
        }, CancellationToken.None, TaskCreationOptions.LongRunning, TaskScheduler.Default);
    }

    public void PollLoss()
    {
        lock (gate)
        {
            if (session is null || Volatile.Read(ref stopping) != 0) return;
            var counts = EtwSessionStatistics.Query(SessionName);
            loss?.Update(counts.Events, counts.Buffers);
        }
    }

    private void Listen()
    {
        ListenerThreadId = Environment.CurrentManagedThreadId;
        try
        {
            using var owned = new TraceEventSession(SessionName,
                TraceEventSessionOptions.Create | TraceEventSessionOptions.NoRestartOnCreate)
            { BufferSizeMB = 16, BufferQuantumKB = 64, EnableProviderTimeoutMSec = 10_000 };
            lock (gate) session = owned;
            void Receive(TraceEvent data)
            {
                var pid = Volatile.Read(ref selectedPid);
                if (failure.Task.IsCompleted || !AcceptsHeader(data.ProviderGuid, data.ProcessID, pid)) return;
                try
                {
                    var captured = NativeRecord.Capture(data);
                    if (!incoming.TryAdd(captured, captured.RetainedBytes))
                    {
                        var usage = incoming.Usage;
                        var reason = $"ETW listener queue limit reached: {usage.Items} items, {usage.Bytes} bytes; " +
                            $"incoming {captured.RetainedBytes} bytes, event {captured.Name}.";
                        worker?.Report(new(IngestionIssue.QueueOverflow,
                            reason));
                        Fail(new IOException(reason));
                    }
                }
                catch (Exception error) when (error is ArgumentException or InvalidDataException or
                    NotSupportedException or OverflowException)
                {
                    worker?.Report(new(IngestionIssue.DecodeFailure, error.Message[..Math.Min(error.Message.Length, 768)]));
                    Fail(error);
                }
            }
            owned.Source.Dynamic.All += Receive;
            owned.Source.UnhandledEvents += Receive;
            // Allocator and capture-detail keywords are intentionally not enabled.
            owned.EnableProvider(EventCatalog.RuntimeProviderId, TraceEventLevel.Informational, SelectedKeywords);
            owned.EnableProvider(EventCatalog.StandardXamlProviderId, StandardXamlLevel, StandardXamlKeywords);
            owned.EnableProvider(EventCatalog.CollectorProviderId, TraceEventLevel.Informational, 0x101);
            ready.TrySetResult();
            owned.Source.Process();
            if (Volatile.Read(ref stopping) == 0) Fail(new IOException("Live ETW ended without a stop request."));
        }
        catch (Exception error)
        {
            ready.TrySetException(error);
            Fail(error);
        }
        finally
        {
            lock (gate) session = null;
            incoming.Complete();
            ended.TrySetResult();
        }
    }

    private void Fail(Exception error) => failure.TrySetResult(error);

    internal static bool AcceptsHeader(Guid provider, int producerPid, int targetPid) =>
        provider == EventCatalog.CollectorProviderId
            ? targetPid != 0 && producerPid == Environment.ProcessId
            : (provider == EventCatalog.RuntimeProviderId || provider == EventCatalog.StandardXamlProviderId) &&
                producerPid != Environment.ProcessId && (targetPid == 0 || producerPid == targetPid);

    public ValueTask DisposeAsync()
    {
        lock (gate) return new(disposal ??= StopAsync());
    }

    private async Task StopAsync()
    {
        Interlocked.Exchange(ref stopping, 1);
        try
        {
            await ready.Task.ConfigureAwait(false);
            lock (gate)
            {
                if (session is not null)
                {
                    var counts = EtwSessionStatistics.Stop(SessionName);
                    session.StopOnDispose = false;
                    loss?.Update(counts.Events, counts.Buffers);
                }
            }
        }
        catch (Exception error) { Fail(error); }
        await ended.Task.ConfigureAwait(false);
        if (decoding is not null) await decoding.ConfigureAwait(false);
        if (failure.Task.IsCompletedSuccessfully)
            throw new IOException("Live target event capture failed.", failure.Task.Result);
    }
}

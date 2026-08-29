using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using Microsoft.UI.Dispatching;
using System.Collections.Concurrent;
using XamlProfiler.Models;

namespace XamlProfiler.Services;

/// <summary>
/// Real-time ETW listener that subscribes to the XamlProfilerTracing provider
/// and dispatches tree mutations to the ProfilerTreeStore on the UI thread.
/// </summary>
public sealed class EtwListenerService : IDisposable
{
    // Must match the GUID in XamlProfilerTracing.h:
    // {A1B2C3D4-E5F6-4A5B-9C8D-7E6F5A4B3C2D}
    private static readonly Guid XamlProfilerProviderGuid =
        new("A1B2C3D4-E5F6-4A5B-9C8D-7E6F5A4B3C2D");

    private const string SessionName = "XamlProfilerSession";

    // Batching: events arrive on the ETW pump thread far faster than the UI
    // can absorb them one-by-one. We queue them here and drain in periodic
    // bursts so each dispatcher hop applies many mutations under one layout
    // invalidation, keeping the UI thread responsive.
    private const int FlushIntervalMs = 50;
    private const int MaxBatchSize = 500;

    private TraceEventSession? _session;
    private ETWTraceEventSource? _source;
    private Thread? _processingThread;
    private readonly ProfilerTreeStore _store;
    private readonly ProfilerEventRouter _router;
    private readonly DispatcherQueue _dispatcherQueue;
    // volatile: written from the UI thread (SetTargetProcess) and read on the ETW pump
    // thread (OnEvent). 0 means "capture all processes".
    private volatile int _targetProcessId;
    private readonly ConcurrentQueue<(string name, object?[] payload)> _pending = new();
    private System.Threading.Timer? _flushTimer;
    private int _flushScheduled;   // 0 = no flush in flight, 1 = one queued on dispatcher
    private bool _isRunning;

    public EtwListenerService(ProfilerTreeStore store, DispatcherQueue dispatcherQueue, int targetProcessId = 0)
    {
        _store = store;
        _router = new ProfilerEventRouter(store);
        _dispatcherQueue = dispatcherQueue;
        _targetProcessId = targetProcessId;
    }

    public event Action<string>? StatusChanged;
    public event Action<int>? EventCountChanged;

    /// <summary>
    /// Narrows (or clears) the process filter after the session is already running.
    /// Pass the target PID once it is known; pass 0 to capture all processes.
    /// Used to start capturing in all-process mode BEFORE the target is launched
    /// (so boot events aren't missed), then focus on the target the moment its PID
    /// is resolved. Only affects events processed after this call — events already
    /// enqueued are kept.
    /// </summary>
    public void SetTargetProcess(int processId) => _targetProcessId = processId;

    public void Start()
    {
        if (_isRunning) return;

        try
        {
            // Kill any lingering session from a previous crash
            try { TraceEventSession.GetActiveSession(SessionName)?.Stop(); } catch { }

            _session = new TraceEventSession(SessionName)
            {
                StopOnDispose = true
            };

            _session.EnableProvider(XamlProfilerProviderGuid, TraceEventLevel.Verbose);

            var pidInfo = _targetProcessId != 0 ? $" (PID: {_targetProcessId})" : " (all processes)";
            StatusChanged?.Invoke($"Session started{pidInfo} — waiting for events...");

            _source = _session.Source;
            _source.Dynamic.All += OnEvent;

            _processingThread = new Thread(() =>
            {
                try
                {
                    _source.Process();
                }
                catch (Exception ex)
                {
                    _dispatcherQueue.TryEnqueue(() =>
                        StatusChanged?.Invoke($"ETW processing error: {ex.Message}"));
                }
            })
            {
                IsBackground = true,
                Name = "ETW-Profiler-Listener"
            };

            _isRunning = true;
            _processingThread.Start();

            // Start periodic flush — drains the queue into the UI in batches.
            _flushTimer = new System.Threading.Timer(_ => ScheduleFlush(),
                null, FlushIntervalMs, FlushIntervalMs);
        }
        catch (UnauthorizedAccessException)
        {
            StatusChanged?.Invoke("ERROR: Run as Administrator to capture ETW events.");
        }
        catch (Exception ex)
        {
            StatusChanged?.Invoke($"ERROR: {ex.Message}");
        }
    }

    public void Stop()
    {
        if (!_isRunning) return;
        _isRunning = false;

        _flushTimer?.Dispose();
        _flushTimer = null;

        _session?.Stop();
        _processingThread?.Join(2000);

        // Final drain so events captured right before stop still reach the UI.
        ScheduleFlush();

        StatusChanged?.Invoke("Session stopped.");
    }

    private void OnEvent(TraceEvent evt)
    {
        // Filter to target process only (if specified)
        if (_targetProcessId != 0 && evt.ProcessID != _targetProcessId)
            return;

        // IMPORTANT: The TraceEvent instance is reused by the ETW parser and its
        // underlying payload buffer becomes invalid as soon as this callback returns.
        // Capture EventName and all payload values synchronously HERE on the ETW thread,
        // then marshal the already-copied primitives to the UI thread.
        // Reading evt.PayloadValue / evt.EventName from the dispatcher lambda would
        // dereference recycled memory => NullReferenceException / FormatException.

        string eventName;
        object?[] payload;
        try
        {
            eventName = evt.EventName;
            int count = evt.PayloadNames?.Length ?? 0;
            payload = new object?[count];
            for (int i = 0; i < count; i++)
            {
                try { payload[i] = evt.PayloadValue(i); }
                catch { payload[i] = null; }
            }
        }
        catch
        {
            // If we can't even read the metadata, drop the event.
            return;
        }

        // Just enqueue — the flush timer will drain in batches on the UI thread.
        _pending.Enqueue((eventName, payload));
    }

    /// <summary>
    /// Queues at most one pending drain on the UI thread at a time. If a drain
    /// is already in flight, the next timer tick will pick up any new events.
    /// </summary>
    private void ScheduleFlush()
    {
        if (_pending.IsEmpty) return;
        if (Interlocked.Exchange(ref _flushScheduled, 1) == 1) return;

        _dispatcherQueue.TryEnqueue(() =>
        {
            try { FlushPending(); }
            finally { Interlocked.Exchange(ref _flushScheduled, 0); }
        });
    }

    private void FlushPending()
    {
        int applied = 0;
        while (applied < MaxBatchSize && _pending.TryDequeue(out var item))
        {
            try
            {
                _store.EventCount++;
                _router.Dispatch(new EtwEvent(item.name, item.payload));
            }
            catch
            {
                // Robustness — one bad event must not poison the batch.
            }
            applied++;
        }

        if (applied > 0)
        {
            EventCountChanged?.Invoke(_store.EventCount);
        }
    }

    public void Dispose()
    {
        Stop();
        _source?.Dispose();
        _session?.Dispose();
    }
}

using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using Microsoft.UI.Dispatching;
using System.Collections.Concurrent;
using XamlProfiler.Models;

namespace XamlProfiler.Services;

/// <summary>
/// Real-time ETW listener that subscribes to the XamlProfilerTracing provider
/// and dispatches tree mutations to the ProfilerTreeStore on the UI thread.
/// The session enables only the tree keyword groups the views consume, and when the
/// target is known up front it is scoped to that executable (or PID), so other WinUI
/// processes on the machine don't turn the instrumentation on.
/// </summary>
public sealed class EtwListenerService : IDisposable
{
    // Must match the provider defined in XamlProfilerTracing.cpp ("Microsoft-Windows-XAML-Profiler"),
    // whose GUID is the EventSource name hash of the provider name.
    private static readonly Guid XamlProfilerProviderGuid =
        new("26f28a5c-7d30-54c5-40f4-fff6d3beda7d");

    // XamlProfilerKeywords (XamlProfilerTracing.h): XamlTree (0x01) | CompTree (0x02) | WucTree (0x04).
    // The router only consumes the tree events, so the surface, texture, heap-snapshot and
    // GPU-snapshot groups (and the work behind them) stay off in the target.
    private const ulong TreeKeywords = 0x01 | 0x02 | 0x04;

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
    // File name (with extension, no path) of the process to launch; null when unknown.
    private readonly string? _targetExecutableName;
    private readonly ConcurrentQueue<(string name, object?[] payload)> _pending = new();
    private System.Threading.Timer? _flushTimer;
    private int _flushScheduled;   // 0 = no flush in flight, 1 = one queued on dispatcher
    private bool _isRunning;

    /// <param name="targetProcessId">An already-running target process (attach). Scopes the
    /// session to that PID. 0 when the target isn't running yet.</param>
    /// <param name="targetExecutableName">File name of a target that will be launched after
    /// <see cref="Start"/> (for example "MyApp.exe"). Scopes the session to processes running
    /// that executable. Takes precedence over <paramref name="targetProcessId"/>.</param>
    public EtwListenerService(
        ProfilerTreeStore store,
        DispatcherQueue dispatcherQueue,
        int targetProcessId = 0,
        string? targetExecutableName = null)
    {
        _store = store;
        _router = new ProfilerEventRouter(store);
        _dispatcherQueue = dispatcherQueue;
        _targetProcessId = targetProcessId;
        _targetExecutableName = string.IsNullOrWhiteSpace(targetExecutableName) ? null : targetExecutableName;
    }

    public event Action<string>? StatusChanged;
    public event Action<int>? EventCountChanged;

    /// <summary>
    /// Narrows (or clears) the process filter after the session is already running.
    /// Pass the target PID once it is known; pass 0 to capture all processes.
    /// Used to start capturing BEFORE the target is launched (so boot events aren't
    /// missed), then focus on the target the moment its PID is resolved. Only affects
    /// events processed after this call — events already enqueued are kept.
    /// </summary>
    public void SetTargetProcess(int processId) => _targetProcessId = processId;

    /// <summary>
    /// Re-enables the provider in the running session for one process, or for all processes
    /// when <paramref name="processId"/> is 0, replacing the executable-name scope given at
    /// construction, and narrows the consumer-side filter to match. Used when the process that
    /// hosts XAML doesn't run the executable the session was scoped to, so ETW never enabled the
    /// provider in it. That process's events from before this call were not captured.
    /// Returns false if the provider couldn't be re-enabled.
    /// </summary>
    public bool RescopeToProcess(int processId)
    {
        _targetProcessId = processId;
        if (_session == null || !_isRunning) return false;

        var options = new TraceEventProviderOptions();
        if (processId != 0)
        {
            options.ProcessIDFilter = new List<int> { processId };
        }

        try
        {
            _session.EnableProvider(XamlProfilerProviderGuid, TraceEventLevel.Verbose, TreeKeywords, options);
            return true;
        }
        catch (Exception ex)
        {
            StatusChanged?.Invoke($"ERROR: {ex.Message}");
            return false;
        }
    }

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

            // Scope filter, applied by ETW in the target processes: only processes that match
            // enable the provider. The consumer-side PID check in OnEvent still applies.
            var options = new TraceEventProviderOptions();
            string scopeInfo;
            if (_targetExecutableName != null)
            {
                options.ProcessNameFilter = new List<string> { _targetExecutableName };
                scopeInfo = $" ({_targetExecutableName})";
            }
            else if (_targetProcessId != 0)
            {
                options.ProcessIDFilter = new List<int> { _targetProcessId };
                scopeInfo = $" (PID: {_targetProcessId})";
            }
            else
            {
                scopeInfo = " (all processes)";
            }

            _session.EnableProvider(XamlProfilerProviderGuid, TraceEventLevel.Verbose, TreeKeywords, options);

            StatusChanged?.Invoke($"Session started{scopeInfo} — waiting for events...");

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

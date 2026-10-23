using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using Microsoft.UI.Dispatching;
using System.Collections.Concurrent;
using System.Diagnostics;
using XamlProfiler.Models;

namespace XamlProfiler.Services;

/// <summary>
/// Real-time ETW listener that subscribes to the XamlProfilerTracing provider (plus the two
/// Microsoft-Windows-XAML events that carry x:Name, see <see cref="ProfilerProviders"/>)
/// and dispatches tree mutations to the ProfilerTreeStore on the UI thread.
/// The session enables only the tree keyword groups the views consume, and when the
/// target is known up front it is scoped to that executable (or PID), so other WinUI
/// processes on the machine don't turn the instrumentation on. Events are put back in
/// timestamp order (<see cref="EventReorderBuffer{T}"/>) before they reach the router, so
/// the trees lag the target by about <see cref="ReorderHoldBack"/>.
/// </summary>
public sealed class EtwListenerService : IDisposable
{
    private const string SessionName = "XamlProfilerSession";

    // Batching: events arrive on the ETW pump thread far faster than the UI
    // can absorb them one-by-one. We queue them here and drain in periodic
    // bursts so each dispatcher hop applies many mutations under one layout
    // invalidation, keeping the UI thread responsive.
    private const int FlushIntervalMs = 50;
    private const int MaxBatchSize = 500;

    // How long each event is held for reordering. ETW flushes a real-time session's partly
    // filled buffers about once a second, so that's how late an event can arrive.
    private static readonly TimeSpan ReorderHoldBack = TimeSpan.FromSeconds(1.5);

    private TraceEventSession? _session;
    private ETWTraceEventSource? _source;
    private Thread? _processingThread;
    private readonly ProfilerTreeStore _store;
    private readonly ProfilerEventRouter _router;
    private readonly DispatcherQueue _dispatcherQueue;
    // volatile: written from the UI thread (SetTargetProcess) and read on the ETW pump
    // thread (OnEvent) and when held events are released. 0 means "capture all processes".
    private volatile int _targetProcessId;
    // The profiler is a WinUI app too, so a session that isn't scoped to the target also enables
    // the providers here. Its own events are always dropped.
    private static readonly int s_ownProcessId = Environment.ProcessId;
    // File name (with extension, no path) of the process to launch; null when unknown.
    private readonly string? _targetExecutableName;
    // Captured on the ETW pump thread: the event, the process that wrote it, its ETW timestamp
    // (100 ns ticks) and when it arrived (Stopwatch ticks).
    private readonly ConcurrentQueue<(EtwEvent Event, int ProcessId, long Timestamp, long Arrival)> _pending = new();
    // UI thread only.
    private readonly EventReorderBuffer<(EtwEvent Event, int ProcessId)> _reorder =
        new((long)(ReorderHoldBack.TotalSeconds * Stopwatch.Frequency));
    private readonly List<(EtwEvent Event, int ProcessId)> _released = new();
    // Events held in _reorder, published for ScheduleFlush on the timer thread.
    private volatile int _held;
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
    /// missed), then focus on the target the moment its PID is resolved. Events already
    /// applied are kept; events from other processes that are still held for reordering
    /// are dropped. Filters in the consumer only: ETW still enables the providers in every
    /// process the session is scoped to (see <see cref="RescopeToProcess"/>).
    /// </summary>
    public void SetTargetProcess(int processId) => _targetProcessId = processId;

    /// <summary>
    /// Re-enables the providers in the running session for one process, or for all processes
    /// when <paramref name="processId"/> is 0, replacing the scope given at construction, and
    /// narrows the consumer-side filter to match. Used when the process that hosts XAML doesn't
    /// run the executable the session was scoped to, so ETW never enabled the providers in it
    /// (that process's events from before this call were not captured), and to scope a session
    /// that wasn't scoped once the target's PID is known.
    /// Returns false if the providers couldn't be re-enabled.
    /// </summary>
    public bool RescopeToProcess(int processId)
    {
        _targetProcessId = processId;
        if (_session == null || !_isRunning) return false;

        try
        {
            ProfilerProviders.Enable(_session, null, processId);
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
            // enable the providers. The consumer-side PID check in OnEvent still applies.
            string scopeInfo;
            if (_targetExecutableName != null)
            {
                scopeInfo = $" ({_targetExecutableName})";
            }
            else if (_targetProcessId != 0)
            {
                scopeInfo = $" (PID: {_targetProcessId})";
            }
            else
            {
                scopeInfo = " (all processes)";
            }

            ProfilerProviders.Enable(_session, _targetExecutableName, _targetProcessId);

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

    /// <summary>Stops the session and applies every event still queued or held. Call on the UI thread.</summary>
    public void Stop()
    {
        if (!_isRunning) return;
        _isRunning = false;

        _flushTimer?.Dispose();
        _flushTimer = null;

        _session?.Stop();
        _processingThread?.Join(2000);

        // Final drain so events captured right before stop still reach the UI, including
        // those still held for reordering.
        FlushPending(drainAll: true);

        StatusChanged?.Invoke("Session stopped.");
    }

    private void OnEvent(TraceEvent evt)
    {
        // Filter to target process only (if specified), and never route the profiler's own events.
        int processId = evt.ProcessID;
        if ((_targetProcessId != 0 && processId != _targetProcessId) || processId == s_ownProcessId)
            return;

        // IMPORTANT: The TraceEvent instance is reused by the ETW parser and its
        // underlying payload buffer becomes invalid as soon as this callback returns.
        // Capture EventName and all payload values synchronously HERE on the ETW thread,
        // then marshal the already-copied primitives to the UI thread.
        // Reading evt.PayloadValue / evt.EventName from the dispatcher lambda would
        // dereference recycled memory => NullReferenceException / FormatException.

        EtwEvent captured;
        long timestamp;
        try
        {
            if (!ProfilerProviders.TryCapture(evt, out captured)) return;
            // 100 ns ticks since the session started, which keeps the QPC resolution.
            timestamp = (long)(evt.TimeStampRelativeMSec * 10_000);
        }
        catch
        {
            // If we can't even read the metadata, drop the event.
            return;
        }

        // Just enqueue — the flush timer will drain in batches on the UI thread.
        _pending.Enqueue((captured, processId, timestamp, Stopwatch.GetTimestamp()));
    }

    /// <summary>
    /// Queues at most one pending drain on the UI thread at a time. If a drain
    /// is already in flight, the next timer tick will pick up any new events.
    /// </summary>
    private void ScheduleFlush()
    {
        if (_pending.IsEmpty && _held == 0) return;
        if (Interlocked.Exchange(ref _flushScheduled, 1) == 1) return;

        _dispatcherQueue.TryEnqueue(() =>
        {
            try { FlushPending(); }
            finally { Interlocked.Exchange(ref _flushScheduled, 0); }
        });
    }

    // UI thread. Moves newly captured events into the reorder buffer, then applies those whose
    // hold-back has ended (at most MaxBatchSize), or every held event when draining.
    private void FlushPending(bool drainAll = false)
    {
        while (_pending.TryDequeue(out var item))
        {
            _reorder.Add((item.Event, item.ProcessId), item.Timestamp, item.Arrival);
        }

        _released.Clear();
        if (drainAll)
        {
            _reorder.ReleaseAll(_released);
        }
        else
        {
            _reorder.Release(Stopwatch.GetTimestamp(), MaxBatchSize, _released);
        }
        _held = _reorder.Count;

        // The target can be narrowed after an event was captured (a session that started out
        // listening to all processes), so check each event's process again.
        int targetProcessId = _targetProcessId;
        int applied = 0;
        foreach (var (e, processId) in _released)
        {
            if (targetProcessId != 0 && processId != targetProcessId) continue;
            try
            {
                _store.EventCount++;
                applied++;
                _router.Dispatch(e);
            }
            catch
            {
                // Robustness — one bad event must not poison the batch.
            }
        }

        if (applied > 0)
        {
            EventCountChanged?.Invoke(_store.EventCount);
        }
        _released.Clear();
    }

    public void Dispose()
    {
        Stop();
        _source?.Dispose();
        _session?.Dispose();
    }
}

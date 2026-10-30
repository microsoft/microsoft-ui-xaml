// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.Diagnostics.Tracing;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Ingestion;

public enum TraceReplayState
{
    Reading,
    Stopping,
    // The trace was read to its end and every event of the process was stored; as in a live session, the session file's
    // size limit may have deleted the oldest since (RecordingStatus.CompleteFromSeconds).
    Finished,
    // Stop ended the reading; the session file holds the process's events read before it.
    Stopped,
    // The trace could not be read to its end, as when the file is damaged or cut off, or an event could not be processed
    // or stored. The session file holds the events stored before.
    Failed
}

// ReadMilliseconds is the trace time of the last event read, of any process, and DurationMilliseconds the trace's length
// (0 when its header has none), so Progress is how much of the file has been read. OutOfOrderEvents counts the process's
// events whose timestamp is earlier than the event before them; they keep the file's order.
public sealed record TraceReplayStatus(TraceReplayState State, long XamlEvents, long ProfilerEvents, long GpuMemoryEvents,
    int LostEvents, long OutOfOrderEvents, double ReadMilliseconds, double DurationMilliseconds)
{
    public double? Progress => DurationMilliseconds > 0 ? Math.Clamp(ReadMilliseconds / DurationMilliseconds, 0, 1) : null;
}

// Reads one process's events from a recorded trace (.etl) into a new session file, through the pipeline and processors
// a live session uses. A reader thread reads the file in its order, which is time order, and posts the process's events
// in batches. A batch waits while the pipeline is full instead of being dropped, so every event of the process is stored,
// in the file's order, unless the session file's size limit deletes the oldest. Stop ends the reading early.
public sealed class TraceReplay : IAsyncDisposable
{
    public const int BatchSize = 4_096;
    // A process name read from the trace can be as long as any file name; the session file's name has to fit the rest.
    private const int MaxNameLength = 64;
    private static readonly TimeSpan ChangedInterval = TimeSpan.FromMilliseconds(250);

    private readonly ETWTraceEventSource source;
    private readonly SessionWriter writer;
    private readonly EventCounter counter = new();
    private readonly EventPipeline pipeline;
    private readonly TaskCompletionSource completion = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private readonly Timer changed;
    private readonly double durationMilliseconds;

    // Used only on the reader thread.
    private readonly ProfilerEvent[] batch = new ProfilerEvent[BatchSize];
    private int count;
    private long lastQpc = long.MinValue;
    private bool halted;

    private int state;
    private int lostEvents;
    private long outOfOrderEvents;
    private double readMilliseconds;
    private volatile Exception? failure;

    private TraceReplay(string etlPath, int processId, string? processName, ETWTraceEventSource source, SessionWriter writer)
    {
        EtlPath = etlPath;
        ProcessId = processId;
        ProcessName = processName;
        this.source = source;
        this.writer = writer;
        lostEvents = source.EventsLost;
        var duration = source.SessionDuration.TotalMilliseconds;
        durationMilliseconds = double.IsFinite(duration) && duration > 0 ? duration : 0;
        // Queues as many events as a live session's pipeline holds, so the reader runs through a launch's burst of events
        // while the processors catch up and doesn't wait for them. With 4 batches, a WinUI 3 Gallery launch took 12% longer.
        pipeline = EventPipeline.ForTrace([counter, writer], EventPipeline.DefaultCapacity / BatchSize);
        try
        {
            source.AllEvents += OnAnyEvent;
            XamlEventSubscription.Match(source, processId, OnEvent);
            changed = new(_ => OnInterval(), null, ChangedInterval, ChangedInterval);
            new Thread(Read) { IsBackground = true, Name = "WinUIProfiler trace reader" }.Start();
        }
        catch
        {
            // Start closes the session file and the trace.
            changed?.Dispose();
            pipeline.Dispose();
            throw;
        }
    }

    // Raised every 250 ms while reading, and when the reading ends; may be raised on any thread.
    public event EventHandler? Changed;

    public string EtlPath { get; }

    public int ProcessId { get; }

    public string? ProcessName { get; }

    public TraceReplayState State => (TraceReplayState)Volatile.Read(ref state);

    private bool StopRequested => State == TraceReplayState.Stopping;

    // Why the reading failed: the trace could not be read, or an event could not be processed or stored.
    public Exception? Failure => failure;

    // Completes, never faulted, once the session file is closed.
    public Task Completion => completion.Task;

    // The counts are final once State is.
    public TraceReplayStatus Status => new(State, counter.XamlEvents, counter.ProfilerEvents, counter.GpuMemoryEvents,
        Volatile.Read(ref lostEvents), Interlocked.Read(ref outOfOrderEvents), Volatile.Read(ref readMilliseconds),
        durationMilliseconds);

    public RecordingStatus Recording => writer.Status;

    public string? RecordingError => writer.Failure is { } error ? $"Recording stopped: {error.Message}" : null;

    // Reads processId's events from the trace at etlPath into a new session file in sessionDirectory, named like a live
    // session's file after processName, or after the trace when the name is not known.
    public static TraceReplay Start(string etlPath, int processId, string? processName, string sessionDirectory,
        long limitBytes = SessionWriter.DefaultLimitBytes)
    {
        ArgumentException.ThrowIfNullOrEmpty(etlPath);
        ArgumentException.ThrowIfNullOrEmpty(sessionDirectory);
        var path = Path.GetFullPath(etlPath);
        if (!File.Exists(path))
        {
            throw new FileNotFoundException("The .etl file was not found.", path);
        }

        // Readers of the session file time its events with the frequency it stores, or this machine's while it has none,
        // so the trace's is stored before any event. It is read before the trace is opened for reading, so the file is
        // never open twice at once.
        var frequency = OfflineEtlSource.ReadQpcFrequency(path);
        // Opened before the session file is created, so a file that is not a trace leaves no session file.
        var source = new ETWTraceEventSource(path);
        SessionWriter? writer = null;
        try
        {
            writer = SessionWriter.Create(SessionPath(sessionDirectory, processName ?? Path.GetFileNameWithoutExtension(path), processId),
                new(processId, processName, null, frequency, null, SessionCoverage.Trace, path), limitBytes);
            return new(path, processId, processName, source, writer);
        }
        catch
        {
            try
            {
                writer?.Close("Opening the trace failed");
            }
            catch (Exception error)
            {
                Trace.TraceError($"Closing the session file of a trace that could not be read failed: {error}");
            }

            source.Dispose();
            throw;
        }
    }

    private static string SessionPath(string directory, string name, int processId)
    {
        var length = Math.Min(name.Length, MaxNameLength);
        if (length < name.Length && char.IsHighSurrogate(name[length - 1]))
        {
            length--;
        }

        var invalid = Path.GetInvalidFileNameChars();
        var fileName = string.Concat(name[..length].Select(character => Array.IndexOf(invalid, character) < 0 ? character : '_'));
        var stem = Path.Combine(directory, $"{DateTime.Now:yyyyMMdd-HHmmss}-{fileName}-{processId}");
        var path = stem + ".sqlite";
        for (var number = 2; File.Exists(path); number++)
        {
            path = $"{stem}-{number}.sqlite";
        }

        return path;
    }

    // Ends the reading at the next event; the session file keeps the process's events read before it. Completes once the
    // file is closed.
    public Task StopAsync()
    {
        if (Interlocked.CompareExchange(ref state, (int)TraceReplayState.Stopping, (int)TraceReplayState.Reading) ==
            (int)TraceReplayState.Reading)
        {
            RaiseChanged();
        }

        return completion.Task;
    }

    public async ValueTask DisposeAsync() => await StopAsync().ConfigureAwait(false);

    private void Read()
    {
        var readAll = false;
        try
        {
            try
            {
                source.Process();
            }
            catch (Exception error)
            {
                // A damaged or cut-off file; the events read before the damage are still stored.
                failure ??= error;
            }

            // The events read since the last full batch, also when Stop or damage ended the reading.
            var flushed = count == 0 || Flush();
            readAll = failure is null && !halted && flushed;
            Volatile.Write(ref lostEvents, source.EventsLost);
        }
        catch (Exception error)
        {
            failure ??= error;
        }
        finally
        {
            Finish(readAll);
        }
    }

    // TraceEvent calls these on the reader thread, from ProcessTrace, so they must not throw.
    private void OnAnyEvent(TraceEvent data)
    {
        if (halted)
        {
            return;
        }

        Volatile.Write(ref readMilliseconds, data.TimeStampRelativeMSec);
        if (StopRequested)
        {
            Halt();
        }
    }

    private void OnEvent(TraceEvent data, XamlProvider provider, int processId)
    {
        if (halted)
        {
            return;
        }

        try
        {
            var profilerEvent = XamlEventSubscription.Capture(data, provider, processId);
            if (profilerEvent.TimestampQpc < lastQpc)
            {
                Interlocked.Increment(ref outOfOrderEvents);
            }

            lastQpc = profilerEvent.TimestampQpc;
            batch[count++] = profilerEvent;
            if (count == batch.Length && !Flush())
            {
                Halt();
            }
        }
        catch (Exception error)
        {
            failure ??= error;
            Halt();
        }
    }

    // Queues the batch, waiting while the pipeline is full, also after Stop: the batches already queued are processed
    // before the file is closed anyway. False when an event could not be processed or stored, so the reading must end.
    private bool Flush()
    {
        try
        {
            return writer.Failure is null && pipeline.PostBatch(batch.AsSpan(0, count));
        }
        finally
        {
            Array.Clear(batch, 0, count);
            count = 0;
        }
    }

    // TraceEvent skips the rest of the events it has read and returns from Process.
    private void Halt()
    {
        halted = true;
        source.StopProcessing();
    }

    private void Finish(bool readAll)
    {
        try
        {
            changed.Dispose();
            source.Dispose();
        }
        catch (Exception error)
        {
            failure ??= error;
        }

        try
        {
            // Processes the batches already queued, so no event posted is discarded.
            pipeline.Dispose();
        }
        catch (Exception error)
        {
            failure ??= error;
        }

        failure ??= pipeline.Failure ?? writer.Failure;
        var final = failure is not null ? TraceReplayState.Failed : readAll ? TraceReplayState.Finished : TraceReplayState.Stopped;
        try
        {
            writer.Close(final switch
            {
                TraceReplayState.Finished => "End of trace",
                TraceReplayState.Stopped => "Stopped",
                _ => "Reading the trace failed"
            }, Volatile.Read(ref lostEvents), 0);
        }
        catch (Exception error)
        {
            failure ??= error;
        }

        failure ??= writer.Failure;
        if (failure is not null)
        {
            final = TraceReplayState.Failed;
        }
        else if (final == TraceReplayState.Finished)
        {
            Volatile.Write(ref readMilliseconds, Math.Max(Volatile.Read(ref readMilliseconds), durationMilliseconds));
        }

        Volatile.Write(ref state, (int)final);
        try
        {
            RaiseChanged();
        }
        finally
        {
            completion.TrySetResult();
        }
    }

    private void OnInterval()
    {
        if (State is TraceReplayState.Reading or TraceReplayState.Stopping)
        {
            RaiseChanged();
        }
    }

    // Changed is raised on the timer and reader threads, where an exception would end the process.
    private void RaiseChanged()
    {
        try
        {
            Changed?.Invoke(this, EventArgs.Empty);
        }
        catch (Exception error)
        {
            Trace.TraceError($"A trace reading Changed handler failed: {error}");
        }
    }
}

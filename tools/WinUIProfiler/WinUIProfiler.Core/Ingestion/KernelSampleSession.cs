// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.Diagnostics.Tracing.Parsers;
using Microsoft.Diagnostics.Tracing.Session;

namespace WinUIProfiler.Engine.Ingestion;

// A real-time kernel session that samples the processors' call stacks every millisecond while the profiler is connected,
// as WPR's CPU profile does, and gives StackSampleCollector's complete samples of the target processes to a sink.
// Starting it needs an elevated profiler, as the kernel's sampled profile does; it throws UnauthorizedAccessException
// otherwise. The session's buffers keep its events until StartReading, so a launch's samples from before its
// recordings exist are kept.
internal sealed class KernelSampleSession : IDisposable
{
    // The kernel delivers each processor's buffer on its own, so its events can be this far out of time order.
    private static readonly TimeSpan ReorderWindow = TimeSpan.FromSeconds(2);

    private readonly object gate = new();
    private readonly TraceEventSession session;
    private readonly Thread reader;
    private StackSampleCollector? collector;
    private CpuSamplingSummary? summary;
    private int eventsLost;
    private bool reading;
    private bool stopped;

    private KernelSampleSession(TraceEventSession session)
    {
        this.session = session;
        reader = new(Read) { IsBackground = true, Name = "WinUIProfiler CPU sample reader" };
    }

    // Why reading stopped before Stop, when it did.
    public Exception? Failure { get; private set; }

    // Why the samples are incomplete, stored with them once the session stopped; null when they aren't known to be.
    public string? Note => StackSampleCollector.JoinNotes(
        Failure is { } failure
            ? $"CPU sampling stopped early: {failure.Message}"
            : eventsLost > 0
                ? $"The kernel's ETW session lost {eventsLost} events, so some CPU samples or their stacks are missing."
                : null,
        summary is { } sampling ? StackSampleCollector.UnattributedNote(sampling) : null);

    public static KernelSampleSession Start()
    {
        if (TraceEventSession.IsElevated() == false)
        {
            throw new UnauthorizedAccessException("CPU sampling needs an elevated (administrator) profiler.");
        }

        LiveEtwSource.StopAbandonedSessions();
        // A kernel session of its own name, which Windows 8 and later allow, so the NT Kernel Logger stays free for WPR.
        var session = new TraceEventSession($"{LiveEtwSource.SessionPrefix}{Environment.ProcessId}-cpu{Guid.NewGuid():N}")
        {
            StopOnDispose = true
        };
        try
        {
            // The kernel provider is enabled before the source is opened, which would otherwise start an ordinary
            // session. Stack compression stays off: the collector doesn't read stack keys.
            session.EnableKernelProvider(
                KernelTraceEventParser.Keywords.Process | KernelTraceEventParser.Keywords.Thread |
                KernelTraceEventParser.Keywords.ImageLoad | KernelTraceEventParser.Keywords.Profile,
                KernelTraceEventParser.Keywords.Profile);
            _ = session.Source;
            return new(session);
        }
        catch
        {
            session.Dispose();
            throw;
        }
    }

    // Reads the session on its own thread, from the events it kept. IsTarget (given a process ID and the event's time as a
    // UTC FILETIME) and sink are called on that thread, and sink in Stop for the samples still waiting; both must be safe
    // to call there.
    public void StartReading(Func<int, long, bool> isTarget, Action<CompletedSample> sink)
    {
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(stopped, this);
            if (reading)
            {
                throw new InvalidOperationException("The CPU sample session is already reading.");
            }

            collector = new(session.Source, isTarget, sink, ReorderWindow);
            reading = true;
            reader.Start();
        }
    }

    // Stops the sampling, gives the sink the samples still waiting, and returns what the session recorded; null when it
    // never read.
    public CpuSamplingSummary? Stop()
    {
        lock (gate)
        {
            if (stopped)
            {
                return summary;
            }

            stopped = true;
            try
            {
                eventsLost = session.EventsLost;
                // Stop flushes the kernel's last buffers to the reader, which then returns from Process.
                session.Stop();
            }
            catch (Exception error) when (error is not OutOfMemoryException)
            {
                Trace.TraceWarning($"Could not stop the CPU sample session: {error}");
                // Closing the source ends Process.
                session.Dispose();
            }

            try
            {
                if (reading)
                {
                    reader.Join();
                }
            }
            finally
            {
                session.Dispose();
            }

            if (collector is not null)
            {
                try
                {
                    collector.Flush();
                }
                catch (Exception error) when (error is not OutOfMemoryException)
                {
                    Trace.TraceError($"CPU samples could not be flushed: {error}");
                    Failure ??= error;
                }

                // The kernel doesn't always write the interval to a real-time session; it's the one the session set.
                var collected = collector.Summary;
                summary = collected with { IntervalMilliseconds = collected.IntervalMilliseconds ?? session.CpuSampleIntervalMSec };
            }

            return summary;
        }
    }

    private void Read()
    {
        try
        {
            session.Source.Process();
        }
        catch (Exception error)
        {
            Trace.TraceError($"CPU sample reading stopped: {error}");
            Failure = error;
        }
    }

    public void Dispose() => Stop();
}

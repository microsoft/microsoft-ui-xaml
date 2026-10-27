// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Ingestion;

// A real-time ETW session that reads one process's XAML TraceLogging events while the profiler is connected.
public sealed class LiveEtwSource : IDisposable
{
    // The TraceLogging events read here have no keyword, so ETW always delivers them. No manifest event uses
    // this bit, so asking only for it stops the runtime from writing manifest events we would skip anyway.
    private const ulong TraceLoggingOnlyKeyword = 0x100000;
    private const string SessionPrefix = "WinUIProfiler-";

    private readonly object gate = new();
    private readonly TraceEventSession session;
    private readonly Thread reader;
    private int eventsLost;
    private bool reading;
    private bool disposed;

    private LiveEtwSource(TraceEventSession session)
    {
        this.session = session;
        reader = new(Read) { IsBackground = true, Name = "WinUIProfiler ETW reader" };
    }

    public Exception? Failure { get; private set; }

    // Events ETW dropped because its buffers were full. Still readable after Dispose.
    public int EventsLost
    {
        get
        {
            lock (gate)
            {
                if (!disposed)
                {
                    eventsLost = session.EventsLost;
                }

                return eventsLost;
            }
        }
    }

    // For a process that is already running: its events from now on.
    public static LiveEtwSource Start(int processId, Action<ProfilerEvent> sink)
    {
        ArgumentNullException.ThrowIfNull(sink);
        var source = Create(ForProcess(processId));
        try
        {
            lock (source.gate)
            {
                source.BeginReading(processId, sink);
            }

            return source;
        }
        catch
        {
            source.Dispose();
            throw;
        }
    }

    // For a launch: call before the process is created, then call StartReading with its ID. The process has
    // no ID yet, so until then ETW filters on the executable name and keeps the events in the session's
    // buffers. The launched process is not delayed or suspended.
    public static LiveEtwSource StartBeforeLaunch(IReadOnlyCollection<string> executableNames)
    {
        ArgumentNullException.ThrowIfNull(executableNames);
        ArgumentOutOfRangeException.ThrowIfZero(executableNames.Count);
        return Create(new TraceEventProviderOptions { ProcessNameFilter = [.. executableNames] });
    }

    // Narrows the session to the launched process and delivers its events, including those buffered since
    // the session started. The same session keeps running, so nothing is lost while the filter changes.
    public void StartReading(int processId, Action<ProfilerEvent> sink)
    {
        ArgumentNullException.ThrowIfNull(sink);
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(disposed, this);
            if (reading)
            {
                throw new InvalidOperationException("The session is already reading.");
            }

            // Other processes with the same executable name stop writing to the session.
            EnableProviders(session, ForProcess(processId));
            BeginReading(processId, sink);
        }
    }

    private static TraceEventProviderOptions ForProcess(int processId) => new() { ProcessIDFilter = [processId] };

    // Needs an elevated process or membership in the local Performance Log Users group;
    // otherwise starting the session throws UnauthorizedAccessException.
    private static LiveEtwSource Create(TraceEventProviderOptions options)
    {
        StopAbandonedSessions();
        var session = new TraceEventSession($"{SessionPrefix}{Environment.ProcessId}-{Guid.NewGuid():N}") { StopOnDispose = true };
        try
        {
            // Open the consumer side before enabling, so events written before reading starts are kept.
            _ = session.Source;
            EnableProviders(session, options);
            return new(session);
        }
        catch
        {
            session.Dispose();
            throw;
        }
    }

    private static void EnableProviders(TraceEventSession session, TraceEventProviderOptions options)
    {
        session.EnableProvider(XamlProviders.Xaml, TraceEventLevel.Verbose, TraceLoggingOnlyKeyword, options);
        session.EnableProvider(XamlProviders.XamlProfiler, TraceEventLevel.Verbose, ulong.MaxValue, options);
    }

    // A session outlives a profiler that crashed or was killed, and keeps the providers enabled in the processes
    // it filters on (for a launch, every new process with that executable name). Sessions are named
    // WinUIProfiler-<profiler process ID>-<unique ID>; stop those whose profiler process no longer exists.
    private static void StopAbandonedSessions()
    {
        foreach (var name in TraceEventSession.GetActiveSessionNames())
        {
            if (!name.StartsWith(SessionPrefix, StringComparison.Ordinal) ||
                name[SessionPrefix.Length..].Split('-') is not [var owner, _] ||
                !int.TryParse(owner, out var ownerId) || IsRunning(ownerId))
            {
                continue;
            }

            try
            {
                TraceEventSession.GetActiveSession(name)?.Stop();
            }
            catch (Exception error) when (error is not OutOfMemoryException)
            {
                Trace.TraceWarning($"Could not stop the abandoned ETW session {name}: {error.Message}");
            }
        }
    }

    // A process ID that was reused counts as running, so the session is left alone.
    private static bool IsRunning(int processId)
    {
        try
        {
            using var process = Process.GetProcessById(processId);
            return true;
        }
        catch (ArgumentException)
        {
            return false;
        }
    }

    // The process ID check also skips events other processes with the same executable name wrote before the
    // session was narrowed to one process.
    private void BeginReading(int processId, Action<ProfilerEvent> sink)
    {
        XamlEventSubscription.Subscribe(session.Source, processId, sink);
        reader.Start();
        reading = true;
    }

    private void Read()
    {
        try
        {
            session.Source.Process();
        }
        catch (Exception error)
        {
            Trace.TraceError($"ETW reading stopped: {error}");
            Failure = error;
        }
    }

    public void Dispose()
    {
        lock (gate)
        {
            if (disposed)
            {
                return;
            }

            eventsLost = session.EventsLost;
            disposed = true;
            // Stopping the session makes Process() return on the reader thread.
            session.Dispose();
        }

        if (reading)
        {
            reader.Join();
        }
    }
}

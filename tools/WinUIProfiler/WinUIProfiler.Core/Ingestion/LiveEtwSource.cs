// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.Engine.Ingestion;

// A real-time ETW session that reads selected processes' events from the XAML providers and the marker providers, and the
// graphics resource/device events and the kernel's GPU memory, allocation and window focus events for them, while
// the profiler is connected. For a launch, the launcher providers' (MarkerProviderSettings.Launcher) events of the
// process that created the target, such as the profiler, are read too.
public sealed class LiveEtwSource : IDisposable
{
    // Microsoft-Windows-DxgKrnl keyword of the per-process GPU memory usage and budget events.
    private const ulong GpuMemoryKeyword = 0x4;
    // Microsoft-Windows-DxgKrnl Resource: device and adapter allocation lifetimes and rundown.
    private const ulong GpuResourceKeyword = 0x40;
    // Microsoft-Windows-Direct3D11 Devices (0x2) and Resources (0x4).
    private const ulong Direct3D11Keywords = 0x6;
    // Microsoft-Windows-Win32k Focus keyword, which only its focus change event has.
    private const ulong FocusKeyword = 0x2000;
    private const string SessionPrefix = "WinUIProfiler-";
    // Win32kTraceLogging, a provider of the default marker configuration.
    private static readonly Guid Win32kTraceLogging = new("487d6e37-1b9d-46d3-a8fd-54ce8bdf8a53");

    private readonly object gate = new();
    private readonly TraceEventSession session;
    private readonly IReadOnlyList<MarkerProviderSettings> markers;
    private readonly Thread reader;
    private int eventsLost;
    private bool direct3DEnabled;
    private bool reading;
    private bool disposed;

    private LiveEtwSource(TraceEventSession session, IReadOnlyList<MarkerProviderSettings> markers, bool direct3DEnabled)
    {
        this.session = session;
        this.markers = markers;
        this.direct3DEnabled = direct3DEnabled;
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

    // For a process that is already running: its events from now on. Markers are the marker providers to read too.
    public static LiveEtwSource Start(int processId, IReadOnlyCollection<MarkerProviderSettings> markers,
        Action<ProfilerEvent> sink) =>
        Start(new Dictionary<int, long?> { [processId] = null }, markers, sink);

    public static LiveEtwSource Start(IReadOnlyDictionary<int, long?> processes,
        IReadOnlyCollection<MarkerProviderSettings> markers, Action<ProfilerEvent> sink)
    {
        var selected = ValidateProcesses(processes);
        ArgumentNullException.ThrowIfNull(sink);
        var options = new TraceEventProviderOptions { ProcessIDFilter = [.. selected.Keys] };
        var source = Create(options, options, markers);
        try
        {
            XamlEventSubscription.SubscribeProcesses(source.session.Source, selected, source.markers, sink, null);
            source.CaptureGraphicsState(options);
            source.reader.Start();
            source.reading = true;
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
    // buffers. The launched process is not delayed or suspended. The launcher providers are enabled for the
    // profiler's executable and launcherExecutableNames too, so the session has the events the process that creates
    // the target writes as it launches it.
    public static LiveEtwSource StartBeforeLaunch(IReadOnlyCollection<string> executableNames,
        IReadOnlyCollection<string> launcherExecutableNames, IReadOnlyCollection<MarkerProviderSettings> markers)
    {
        ArgumentNullException.ThrowIfNull(executableNames);
        ArgumentNullException.ThrowIfNull(launcherExecutableNames);
        ArgumentOutOfRangeException.ThrowIfZero(executableNames.Count);
        var launchers = Path.GetFileName(Environment.ProcessPath) is { Length: > 0 } profiler
            ? launcherExecutableNames.Append(profiler)
            : launcherExecutableNames;
        List<string> launcherNames = [.. executableNames.Concat(launchers).Distinct(StringComparer.OrdinalIgnoreCase)];
        return Create(new TraceEventProviderOptions { ProcessNameFilter = [.. executableNames] },
            new TraceEventProviderOptions { ProcessNameFilter = launcherNames }, markers);
    }

    // Narrows the session to the launched processes and delivers their events, including those buffered since
    // the session started, and the events of launcherProcessId, the process that created them, from the launcher
    // providers: those written before the session was narrowed. The same session keeps running, so nothing is lost
    // while the filter changes.
    public void StartReading(int processId, int? launcherProcessId, Action<ProfilerEvent> sink) =>
        StartReading(new Dictionary<int, long?> { [processId] = null }, launcherProcessId, sink);

    // The native startup group owns handles for these exact process instances for the duration of the capture.
    // Keep every validated member enabled, rather than replacing the activation PID with only the UI child's PID.
    public void StartReading(IReadOnlyDictionary<int, long?> processes, int? launcherProcessId, Action<ProfilerEvent> sink)
    {
        var selected = ValidateProcesses(processes);
        ArgumentNullException.ThrowIfNull(sink);
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(disposed, this);
            if (reading)
            {
                throw new InvalidOperationException("The session is already reading.");
            }

            // Other processes with the same executable name, and the launchers, stop writing to the session.
            var options = new TraceEventProviderOptions { ProcessIDFilter = [.. selected.Keys] };
            direct3DEnabled = EnableProviders(session, options, options, markers, direct3DEnabled);
            XamlEventSubscription.SubscribeProcesses(session.Source, selected, markers, sink, launcherProcessId);
            CaptureGraphicsState(options);
            reader.Start();
            reading = true;
        }
    }

    private static Dictionary<int, long?> ValidateProcesses(IReadOnlyDictionary<int, long?> processes)
    {
        ArgumentNullException.ThrowIfNull(processes);
        if (processes.Count is < 1 or > NativeLaunchProtocol.MaxStartupProcesses ||
            processes.Any(entry => entry.Key <= 0 || entry.Value is <= 0))
        {
            throw new ArgumentException("An ETW startup group needs one to eight valid process identities.", nameof(processes));
        }
        return processes.ToDictionary();
    }

    // Needs an elevated process or membership in the local Performance Log Users group;
    // otherwise starting the session throws UnauthorizedAccessException.
    private static LiveEtwSource Create(TraceEventProviderOptions options, TraceEventProviderOptions launcherOptions,
        IReadOnlyCollection<MarkerProviderSettings> markers)
    {
        ArgumentNullException.ThrowIfNull(markers);
        IReadOnlyList<MarkerProviderSettings> enabled =
        [
            .. markers.Where(marker => !XamlProviders.IsBuiltIn(marker.Provider.Id))
                .DistinctBy(marker => marker.Provider.Id)
        ];
        StopAbandonedSessions();
        var session = new TraceEventSession($"{SessionPrefix}{Environment.ProcessId}-{Guid.NewGuid():N}") { StopOnDispose = true };
        try
        {
            // Open the consumer side before enabling, so events written before reading starts are kept.
            _ = session.Source;
            var direct3DEnabled = EnableProviders(session, options, launcherOptions, enabled);
            EnableGpuMemory(session);
            EnableFocus(session);
            return new(session, enabled, direct3DEnabled);
        }
        catch
        {
            session.Dispose();
            throw;
        }
    }

    // Every keyword at Verbose: all the providers' events, including the telemetry events, which have keyword bits
    // (such as MICROSOFT_KEYWORD_MEASURES), and the TraceLogging events the runtime writes only while the matching
    // manifest event is enabled, such as the start of each FrameworkElement_ApplyTemplate. Not included:
    // Microsoft-Windows-XAML's few events at its own levels above Verbose, written for every API call or parsed
    // object.
    // While Microsoft-Windows-XAML-Diagnostics is enabled, the runtime keeps the XAML source position of the objects
    // it parses (which ElementCreatedWithSource and ElementEnteredTree carry),
    // and keeps setters and resource references for XAML diagnostics, at some cost in memory and CPU time. Only XAML
    // parsed while it is enabled has positions, so a launch has them from the start.
    // The marker providers get the level and keywords their configuration names, and the same process filter, or for a
    // launcher provider launcherOptions. Except Win32kTraceLogging, which win32k registers in the kernel: ETW doesn't
    // enable a provider registered in the kernel for a session that filters by process (see EVENT_FILTER_DESCRIPTOR),
    // so it is enabled for every process, and XamlEventSubscription keeps the target's events, and for a launcher
    // provider the launcher's. ETW enables a provider that has not registered yet when it does, so an app's provider
    // needs no special order.
    private static bool EnableProviders(TraceEventSession session, TraceEventProviderOptions options,
        TraceEventProviderOptions launcherOptions, IReadOnlyList<MarkerProviderSettings> markers, bool direct3DEnabled = false)
    {
        foreach (var provider in XamlProviders.XamlIds)
        {
            session.EnableProvider(provider, TraceEventLevel.Verbose, ulong.MaxValue, options);
        }

        var enabled = EnableDirect3D(
            () => session.EnableProvider(XamlProviders.Direct3D11, TraceEventLevel.Verbose, Direct3D11Keywords, options), direct3DEnabled);

        foreach (var marker in markers)
        {
            session.EnableProvider(marker.Provider.Id, (TraceEventLevel)marker.Level, marker.Keywords,
                marker.Provider.Id == Win32kTraceLogging ? null : marker.Launcher ? launcherOptions : options);
        }
        return enabled;
    }

    internal static bool EnableDirect3D(Action enable, bool previouslyEnabled = false)
    {
        try
        {
            enable();
            return true;
        }
        catch (Exception error) when (error is not OutOfMemoryException && !previouslyEnabled)
        {
            Trace.TraceWarning($"Could not enable Direct3D11 device and resource events: {error.Message}");
            return false;
        }
    }

    // The System process writes some of the target's GPU memory events (such as those written when it exits), so
    // the provider is enabled for every process and XamlEventSubscription keeps the target's by their ProcessId
    // or hProcessId field. A kernel provider writes only to sessions in nonpaged memory, which TraceEventSession uses.
    // Best effort: without these events the session still reads the XAML events.
    private static void EnableGpuMemory(TraceEventSession session)
    {
        try
        {
            session.EnableProvider(XamlProviders.DxgKrnl, TraceEventLevel.Verbose, GpuMemoryKeyword | GpuResourceKeyword);
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            Trace.TraceWarning($"Could not enable the kernel's GPU memory and allocation events: {error.Message}");
        }
    }

    // Window focus changes start regions of interest (InterestingRows). Like DxgKrnl, Win32k writes from the
    // kernel for every process, once per focus change; XamlEventSubscription keeps the target's. Best effort, as above.
    private static void EnableFocus(TraceEventSession session)
    {
        try
        {
            session.EnableProvider(XamlProviders.Win32k, TraceEventLevel.Informational, FocusKeyword);
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            Trace.TraceWarning($"Could not enable the kernel's window focus events: {error.Message}");
        }
    }

    // The kernel writes a process's current GPU memory usage and budget when the process opens a GPU adapter, and
    // for every process when a session requests capture state. Request it, so a process that opened its adapter
    // before the session started also gets its current values. D3D11 devices/resources and kernel allocations have
    // rundown events too. They identify existing objects, not their original creation times.
    private void CaptureGraphicsState(TraceEventProviderOptions options) =>
        CaptureGraphicsState((provider, keywords) => session.CaptureState(provider, keywords),
            () => session.EnableProvider(XamlProviders.Direct3D11, TraceEventLevel.Verbose, Direct3D11Keywords, options), direct3DEnabled);

    internal static void CaptureGraphicsState(Action<Guid, ulong> request, Action restoreDirect3D, bool direct3DEnabled = true)
    {
        foreach (var (provider, keywords) in new[]
        {
            (XamlProviders.Direct3D11, Direct3D11Keywords),
            (XamlProviders.DxgKrnl, GpuMemoryKeyword | GpuResourceKeyword)
        })
        {
            if (provider == XamlProviders.Direct3D11 && !direct3DEnabled) continue;
            try
            {
                request(provider, keywords);
            }
            catch (Exception error) when (error is not OutOfMemoryException)
            {
                Trace.TraceWarning($"Could not request graphics state from {provider}: {error.Message}");
            }
            finally
            {
                // CaptureState re-enables its provider without filters, even when the subsequent state request fails.
                if (provider == XamlProviders.Direct3D11) restoreDirect3D();
            }
        }
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
            // Stop flushes ETW's final buffers. Disposing its source immediately would cancel Process() before
            // those buffers are delivered, losing the last events of a short-lived startup process.
            try
            {
                session.Stop();
            }
            catch
            {
                session.Dispose();
                throw;
            }
        }

        try
        {
            if (reading) reader.Join();
        }
        finally { session.Dispose(); }
    }
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Tracing;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Ingestion;

// Delivers one process's events from the XAML providers and the marker providers, and the kernel's GPU memory and
// window focus events for it, from a TraceEvent source (an .etl file or a live session), and the launcher providers'
// events of the process that launched it, when that is known. TraceLogging events carry their own field names and
// types. Manifest events are decoded with the manifest Windows registers for their provider: system XAML's for
// Microsoft-Windows-XAML and Microsoft-Windows-XAML-Diagnostics, whose events WinUI shares, and the kernel's for
// DxgKrnl and Win32k. An event that manifest does not describe is not delivered. WinUI's FrameInfo is the one shared
// event whose fields differ, so it is decoded here. A marker provider's events are decoded the same ways, or with the
// manifest an EventSource writes into the trace.
internal static class XamlEventSubscription
{
    // WINEVENT_CHANNEL_TRACELOGGING (winmeta.h). Manifest events on the same providers use other channels.
    private const int TraceLoggingChannel = 11;

    // The ID of the events an EventSource writes its manifest in; they describe its other events and are not delivered.
    private const int EventSourceManifest = 0xFFFE;

    // Microsoft-Windows-DxgKrnl VidMmProcessBudgetChange and VidMmProcessUsageChange: a process's GPU memory budget
    // and usage on one adapter and memory segment group, the values DXGI's QueryVideoMemoryInfo returns.
    private const int VidMmProcessBudgetChange = 366;
    private const int VidMmProcessUsageChange = 367;

    // Microsoft-Windows-Win32k's one event with the Focus keyword (0x2000): a window focus change, with the thread
    // that lost focus and the one that got it.
    internal const int FocusChange = 2;

    // Microsoft-Windows-XAML FrameInfo, version 0. WinUI writes one 4-byte Boolean, Value, which is whether the frame
    // drew (TraceFrameInfo in xcpcore.cpp). System XAML's manifest describes four UInt32 fields instead.
    private const int FrameInfo = 64;

    // LauncherProcessId is the process that launched the target, whose events from the launcher providers
    // (MarkerProviderSettings.Launcher) are delivered too.
    internal static void Subscribe(TraceEventDispatcher source, int? processId, IReadOnlyCollection<MarkerProviderSettings> markers,
        Action<ProfilerEvent> sink, int? launcherProcessId = null)
    {
        ArgumentNullException.ThrowIfNull(sink);
        Match(source, processId, markers, (data, provider, marker, owner) => sink(Capture(data, provider, marker, owner)),
            launcherProcessId is not { } launcher ? null : (data, marker) =>
            {
                if (data.ProcessID == launcher)
                {
                    sink(Capture(data, XamlProvider.Marker, marker, launcher));
                }
            });
    }

    internal static void SubscribeProcesses(TraceEventDispatcher source, IReadOnlyDictionary<int, long?> processes,
        IReadOnlyCollection<MarkerProviderSettings> markers, Action<ProfilerEvent> sink, int? launcherProcessId)
    {
        var launcherProviders = markers.Where(marker => marker.Launcher).Select(marker => marker.Provider.Id).ToHashSet();
        Match(source, null, markers, (data, provider, marker, owner) =>
        {
            if (MatchesProcess(processes, owner, data.TimeStamp.ToFileTimeUtc()) ||
                owner == launcherProcessId && marker is not null && launcherProviders.Contains(marker.Id))
            {
                sink(Capture(data, provider, marker, owner));
            }
        });
    }

    internal static bool MatchesProcess(IReadOnlyDictionary<int, long?> processes, int processId, long eventFileTime) =>
        processes.TryGetValue(processId, out var created) && (created is null || eventFileTime >= created);

    // Calls match for each event Subscribe delivers, with its provider (and for a marker event, the marker provider) and
    // the process it belongs to, without copying it. TraceEvent reuses the event object after the call. A marker provider
    // that is a XAML provider, DxgKrnl or Win32k is not read again. For one process, otherProcess, if given, gets the
    // events of the launcher providers (MarkerProviderSettings.Launcher) from every other process.
    internal static void Match(TraceEventDispatcher source, int? processId, IReadOnlyCollection<MarkerProviderSettings> markers,
        Action<TraceEvent, XamlProvider, MarkerProvider?, int> match, Action<TraceEvent, MarkerProvider>? otherProcess = null)
    {
        ArgumentNullException.ThrowIfNull(source);
        ArgumentNullException.ThrowIfNull(markers);
        ArgumentNullException.ThrowIfNull(match);

        var markerIds = new Dictionary<Guid, MarkerProviderSettings>();
        foreach (var marker in markers.Where(marker => !XamlProviders.IsBuiltIn(marker.Provider.Id)))
        {
            markerIds.TryAdd(marker.Provider.Id, marker);
        }

        IgnoreOtherProviders(source, markerIds);
        source.Dynamic.All += data =>
        {
            if (data.ProviderGuid == XamlProviders.DxgKrnl)
            {
                // The System process writes some of a process's rows, such as those written when it exits, so the
                // ProcessId field names the process, not the event header.
                if ((int)data.ID is VidMmProcessBudgetChange or VidMmProcessUsageChange &&
                    PayloadProcessId(data) is { } owner && (processId is null || owner == processId))
                {
                    match(data, XamlProvider.DxgKrnl, null, owner);
                }

                return;
            }

            if (data.ProviderGuid == XamlProviders.Win32k)
            {
                // Matched to the process by the event header's process ID.
                if ((int)data.ID == FocusChange && (processId is null || data.ProcessID == processId))
                {
                    match(data, XamlProvider.Win32k, null, data.ProcessID);
                }

                return;
            }

            MarkerProviderSettings? marker = null;
            if (!XamlProviders.TryGet(data.ProviderGuid, out var provider))
            {
                if (!markerIds.TryGetValue(data.ProviderGuid, out marker) || (int)data.ID == EventSourceManifest)
                {
                    return;
                }

                provider = XamlProvider.Marker;
            }

            if (processId is { } targetProcessId && data.ProcessID != targetProcessId)
            {
                if (marker is { Launcher: true } && otherProcess is not null)
                {
                    otherProcess(data, marker.Provider);
                }

                return;
            }

            match(data, provider, marker?.Provider, data.ProcessID);
        };
    }

    // The dynamic parser asks TDH to describe each type of event that no template handles, once per type, and TDH asks
    // WMI about a classic event, such as the kernel's in a WPR trace: seconds the first time a process reads such a
    // trace. Every later event of a type TDH described is then decoded and passed to Dynamic.All. Instead, this handler
    // gives each type from another provider a template that does nothing, so the parser never sees it. It relies on how
    // TraceEvent 3.2.8 (pinned in Directory.Packages.props) works; check these when updating it:
    // - Handlers for unhandled events run in the order they were registered, so this one must be registered before
    //   anything uses source.Dynamic, which registers the parser's.
    // - A classic event's template is looked up by its task GUID and its ID plus opcode: the key of a template whose
    //   provider is that GUID and whose ID is that sum.
    // - AllEvents still sees the events of a template that does nothing; TraceReplay and ListProcesses count on it.
    // If either of the first two changes, each type goes to TDH as it did without this handler.
    private static void IgnoreOtherProviders(TraceEventDispatcher source, IReadOnlyDictionary<Guid, MarkerProviderSettings> markers)
    {
        var services = (ITraceParserServices)source;
        var ignored = new HashSet<(Guid, ushort)>();
        services.RegisterUnhandledEvent(unknown =>
        {
            // This runs inside ETW's callback, which an exception must not leave; returning false leaves the type to TDH.
            try
            {
                var classic = unknown.IsClassicProvider;
                var provider = classic ? unknown.TaskGuid : unknown.ProviderGuid;
                if (XamlProviders.IsBuiltIn(provider) || markers.ContainsKey(provider))
                {
                    return false;
                }

                var id = (ushort)((int)unknown.ID + (classic ? (int)unknown.Opcode : 0));
                if (id == (ushort)TraceEventID.Illegal || !ignored.Add((provider, id)))
                {
                    return false;
                }

                services.RegisterEventTemplate(new IgnoredEvent(provider, id));
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        });
    }

    private sealed class IgnoredEvent(Guid provider, ushort id)
        : TraceEvent(id, 0, null, Guid.Empty, 0, null, provider, null)
    {
        public override string[] PayloadNames => [];

        public override object? PayloadValue(int index) => null;

        protected override Delegate? Target
        {
            get => null;
            set { }
        }
    }

    private static int? PayloadProcessId(TraceEvent data)
    {
        try
        {
            var index = data.PayloadIndex("ProcessId");
            return index < 0 ? null : data.PayloadValue(index) switch
            {
                int value => value,
                uint value => (int)value,
                long value => (int)value,
                ulong value => (int)value,
                _ => null
            };
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            return null;
        }
    }

    internal static ProfilerEvent Capture(TraceEvent data, XamlProvider provider, MarkerProvider? marker, int processId)
    {
#pragma warning disable CS0618 // The raw QPC timestamp is the clock shared by live sessions and .etl files.
        var timestampQpc = data.TimeStampQPC;
#pragma warning restore CS0618
        return new(timestampQpc, data.TimeStampRelativeMSec, processId, data.ThreadID, provider,
            (int)data.Channel == TraceLoggingChannel, data.EventName, (int)data.Opcode, CopyFields(data, provider), marker);
    }

    private static EventField[] CopyFields(TraceEvent data, XamlProvider provider)
    {
        if (provider == XamlProvider.Xaml && (int)data.ID == FrameInfo && data.Version == 0 &&
            data.EventDataLength == sizeof(int))
        {
            return [new("Value", BitConverter.ToInt32(data.EventData()) != 0)];
        }

        var names = data.PayloadNames;
        var fields = new EventField[names.Length];
        for (var i = 0; i < names.Length; i++)
        {
            object? value;
            try
            {
                value = data.PayloadValue(i);
            }
            catch (Exception error) when (error is not OutOfMemoryException)
            {
                value = null;
            }

            fields[i] = new(names[i], value);
        }

        return fields;
    }
}
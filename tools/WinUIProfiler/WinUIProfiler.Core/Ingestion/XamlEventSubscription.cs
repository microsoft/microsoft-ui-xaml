// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Tracing;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Ingestion;

// Delivers one process's XAML TraceLogging events, and the kernel's GPU memory events for it, from a TraceEvent
// source (an .etl file or a live session). TraceLogging events carry their own field names and types; the kernel's
// events are decoded with its installed manifest. Manifest events on the XAML providers are not read.
internal static class XamlEventSubscription
{
    // WINEVENT_CHANNEL_TRACELOGGING (winmeta.h). Manifest events on the same providers use other channels.
    private const int TraceLoggingChannel = 11;

    // Microsoft-Windows-DxgKrnl VidMmProcessBudgetChange and VidMmProcessUsageChange: a process's GPU memory budget
    // and usage on one adapter and memory segment group, the values DXGI's QueryVideoMemoryInfo returns.
    private const int VidMmProcessBudgetChange = 366;
    private const int VidMmProcessUsageChange = 367;

    internal static void Subscribe(TraceEventDispatcher source, int? processId, Action<ProfilerEvent> sink)
    {
        ArgumentNullException.ThrowIfNull(source);
        ArgumentNullException.ThrowIfNull(sink);

        source.Dynamic.All += data =>
        {
            if (data.ProviderGuid == XamlProviders.DxgKrnl)
            {
                // The System process writes some of a process's rows, such as those written when it exits, so the
                // ProcessId field names the process, not the event header.
                if ((int)data.ID is VidMmProcessBudgetChange or VidMmProcessUsageChange &&
                    PayloadProcessId(data) is { } owner && (processId is null || owner == processId))
                {
                    sink(Capture(data, XamlProvider.DxgKrnl, owner));
                }

                return;
            }

            // The dynamic parser also decodes manifest events when a manifest is registered, so filter by channel.
            if ((int)data.Channel != TraceLoggingChannel || !XamlProviders.TryGet(data.ProviderGuid, out var provider))
            {
                return;
            }

            if (processId is { } targetProcessId && data.ProcessID != targetProcessId)
            {
                return;
            }

            sink(Capture(data, provider, data.ProcessID));
        };
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

    private static ProfilerEvent Capture(TraceEvent data, XamlProvider provider, int processId)
    {
#pragma warning disable CS0618 // The raw QPC timestamp is the clock shared by live sessions and .etl files.
        var timestampQpc = data.TimeStampQPC;
#pragma warning restore CS0618
        return new(timestampQpc, data.TimeStampRelativeMSec, processId, data.ThreadID, provider, data.EventName,
            (int)data.Opcode, CopyFields(data));
    }

    private static EventField[] CopyFields(TraceEvent data)
    {
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
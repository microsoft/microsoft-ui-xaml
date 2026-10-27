// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Tracing;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Ingestion;

// Delivers one process's XAML TraceLogging events from a TraceEvent source (an .etl file now, a live session
// later). TraceLogging events carry their own field names and types; manifest events are not read.
internal static class XamlEventSubscription
{
    // WINEVENT_CHANNEL_TRACELOGGING (winmeta.h). Manifest events on the same providers use other channels.
    private const int TraceLoggingChannel = 11;

    internal static void Subscribe(TraceEventDispatcher source, int? processId, Action<ProfilerEvent> sink)
    {
        ArgumentNullException.ThrowIfNull(source);
        ArgumentNullException.ThrowIfNull(sink);

        // The dynamic parser also decodes manifest events when a manifest is registered, so filter by channel.
        source.Dynamic.All += data =>
        {
            if ((int)data.Channel != TraceLoggingChannel || !XamlProviders.TryGet(data.ProviderGuid, out var provider))
            {
                return;
            }

            if (processId is { } targetProcessId && data.ProcessID != targetProcessId)
            {
                return;
            }

            sink(Capture(data, provider));
        };
    }

    private static ProfilerEvent Capture(TraceEvent data, XamlProvider provider)
    {
#pragma warning disable CS0618 // The raw QPC timestamp is the clock shared by live sessions and .etl files.
        var timestampQpc = data.TimeStampQPC;
#pragma warning restore CS0618
        return new(timestampQpc, data.TimeStampRelativeMSec, data.ProcessID, data.ThreadID, provider, data.EventName,
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
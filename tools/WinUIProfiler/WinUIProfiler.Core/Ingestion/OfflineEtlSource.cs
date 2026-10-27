// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Tracing;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Ingestion;

public sealed record EtlReadResult(long EventsDelivered, int EventsLost, TimeSpan SessionDuration);

// Reads a recorded .etl through the same subscription a live session will use.
public static class OfflineEtlSource
{
    // A null processId reads every process.
    public static EtlReadResult Read(string path, int? processId, Action<ProfilerEvent> sink,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(path);
        ArgumentNullException.ThrowIfNull(sink);
        if (!File.Exists(path))
        {
            throw new FileNotFoundException("The .etl file was not found.", path);
        }

        using var source = new ETWTraceEventSource(path);
        long delivered = 0;
        XamlEventSubscription.Subscribe(source, processId, profilerEvent =>
        {
            delivered++;
            sink(profilerEvent);
        });
        using (cancellationToken.Register(source.StopProcessing))
        {
            source.Process();
        }

        cancellationToken.ThrowIfCancellationRequested();
        return new(delivered, source.EventsLost, source.SessionDuration);
    }
}

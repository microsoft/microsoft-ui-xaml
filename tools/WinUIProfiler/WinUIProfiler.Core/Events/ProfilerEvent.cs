// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Events;

public readonly record struct EventField(string Name, object? Value);

// A copy of one event from a XAML, graphics, focus or marker provider. TraceEvent reuses its event objects, so the data
// is copied out. For a DxgKrnl event, ProcessId is the process the memory belongs to (its ProcessId or hProcessId field),
// and ThreadId is the thread that wrote the event. TraceLogging is
// whether the event describes its own fields rather than a manifest describing them: Microsoft-Windows-XAML writes both
// kinds, and a field name such as ElementId doesn't hold the same kind of object in both. Marker is the provider of a
// marker event (XamlProvider.Marker), and null for the others. EventId and Version are the original ETW identity,
// null when unknown, such as when reading a session file written before they were stored.
public sealed record ProfilerEvent(
    long TimestampQpc,
    double RelativeMilliseconds,
    int ProcessId,
    int ThreadId,
    XamlProvider Provider,
    bool TraceLogging,
    string Name,
    int Opcode,
    IReadOnlyList<EventField> Fields,
    MarkerProvider? Marker = null,
    int? EventId = null,
    int? Version = null)
{
    public Guid ProviderId => Marker?.Id ?? XamlProviders.IdOf(Provider);

    public string ProviderName => Marker?.Name ?? XamlProviders.NameOf(Provider);
}
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Events;

public readonly record struct EventField(string Name, object? Value);

// A copy of one event from a XAML provider, or of one of the kernel's GPU memory events for the process. TraceEvent
// reuses its event objects, so the data is copied out. For a GPU memory event, ProcessId is the process the
// memory belongs to (its ProcessId field), and ThreadId is the thread that wrote the event.
public sealed record ProfilerEvent(
    long TimestampQpc,
    double RelativeMilliseconds,
    int ProcessId,
    int ThreadId,
    XamlProvider Provider,
    string Name,
    int Opcode,
    IReadOnlyList<EventField> Fields);
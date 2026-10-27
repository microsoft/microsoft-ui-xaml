// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Events;

public readonly record struct EventField(string Name, object? Value);

// A copy of one XAML TraceLogging event. TraceEvent reuses its event objects, so the data is copied out.
public sealed record ProfilerEvent(
    long TimestampQpc,
    double RelativeMilliseconds,
    int ProcessId,
    int ThreadId,
    XamlProvider Provider,
    string Name,
    int Opcode,
    IReadOnlyList<EventField> Fields);
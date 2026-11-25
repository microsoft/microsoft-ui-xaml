// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.App.ViewModels;

// An event in the selected object's history in the Objects tab, with the fields that hold the object's address and the
// frame the event belongs to.
public sealed class ObjectEventRow(ProfilerEvent profilerEvent, IReadOnlyList<string> fields, long? frame)
{
    public string Time { get; } = profilerEvent.RelativeMilliseconds.ToString("N3", CultureInfo.CurrentCulture);
    public string Frame { get; } = frame?.ToString("N0", CultureInfo.CurrentCulture) ?? "-";
    public string Name => profilerEvent.Name;
    public string Fields { get; } = EventFormat.Fields(profilerEvent);

    // One field per line, for the details pane.
    public string Details => $"{profilerEvent.Name} | {Time} ms | thread {profilerEvent.ThreadId} | frame {Frame}\n" +
        $"Mentioned as: {string.Join(", ", fields)}\n" + EventFormat.Lines(profilerEvent);

    public override string ToString() => $"{Time} ms | frame {Frame} | {Name} | {Fields}";
}

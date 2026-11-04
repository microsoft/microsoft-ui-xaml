// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

// An event of the selected frame in the Frames tab.
public sealed class FrameEventRow(ProfilerEvent profilerEvent, string bucket, IReadOnlyList<ObjectMention> mentions)
{
    public string Time { get; } = profilerEvent.RelativeMilliseconds.ToString("N3", CultureInfo.CurrentCulture);
    public string Thread { get; } = profilerEvent.ThreadId.ToString(CultureInfo.CurrentCulture);
    public string Name => profilerEvent.Name;
    public string Bucket { get; } = bucket;
    public string Fields { get; } = EventFormat.Fields(profilerEvent);

    // The UI elements and visuals the event mentions, which the details pane links to.
    public IReadOnlyList<ObjectLink> Objects { get; } = [.. mentions.Select(mention => new ObjectLink(mention))];

    // One field per line, for the details pane.
    public string Details => $"{profilerEvent.Name} | {Bucket} | {Time} ms | thread {Thread}\n" + EventFormat.Lines(profilerEvent);

    public override string ToString() => $"{Time} ms | {Name} | {Fields}";
}

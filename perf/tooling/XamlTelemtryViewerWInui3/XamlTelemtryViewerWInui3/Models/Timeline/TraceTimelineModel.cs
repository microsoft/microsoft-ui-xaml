using System;
using System.Collections.Generic;

namespace XamlTelemtryViewerWInui3.Models.Timeline;

/// <summary>
/// Snapshot of the timeline view's bindable state for a particular
/// (trace, selected-process) pair. Built by <see cref="Services.TraceTimelineBuilder"/>.
/// </summary>
public sealed class TraceTimelineModel
{
    public required DateTime TraceStart { get; init; }
    public required DateTime TraceEnd { get; init; }

    /// <summary>Start of the selected process; equal to <see cref="TraceStart"/> if unknown.</summary>
    public required DateTime ProcessStart { get; init; }

    /// <summary>End of the selected process; equal to <see cref="TraceEnd"/> if unknown.</summary>
    public required DateTime ProcessEnd { get; init; }

    public required ProcessInfo SelectedProcess { get; init; }

    public required IReadOnlyList<TimelineItem> TrackOneItems { get; init; }
    public required IReadOnlyList<TimelineItem> TrackTwoItems { get; init; }

    public TimeSpan TraceDuration => TraceEnd - TraceStart;
    public TimeSpan ProcessDuration => ProcessEnd - ProcessStart;
}


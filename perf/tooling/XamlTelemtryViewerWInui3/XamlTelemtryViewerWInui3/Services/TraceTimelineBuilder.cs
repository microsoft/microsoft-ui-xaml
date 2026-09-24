using System;
using System.Collections.Generic;
using XamlTelemtryViewerWInui3.Models;
using XamlTelemtryViewerWInui3.Models.Timeline;

namespace XamlTelemtryViewerWInui3.Services;

/// <summary>
/// Viewer-side wrapper that assembles a <see cref="TraceTimelineModel"/> from the
/// shared <see cref="XamlTimeline.TimelineBuilder"/> Track 1 detection plus the
/// viewer-specific process/trace bounds. Track 2 starts empty and is built on
/// demand in TimelineWindow when the user drills into a Track 1 item.
/// </summary>
public sealed class TraceTimelineBuilder
{
    private readonly TimelineBuilder _core = new();  // XamlTimeline.TimelineBuilder (via global using)

    public TraceTimelineModel Build(
        TraceData trace,
        IReadOnlyList<PhaseDefinition> definitions,
        ProcessInfo selectedProcess)
    {
        ArgumentNullException.ThrowIfNull(trace);
        ArgumentNullException.ThrowIfNull(definitions);
        ArgumentNullException.ThrowIfNull(selectedProcess);

        var processStart = selectedProcess.CreateTime ?? trace.TraceStart;
        var processEnd = selectedProcess.ExitTime ?? trace.TraceEnd;

        var trackOne = _core.BuildTrackOne(
            trace.Events,
            definitions,
            selectedProcess.Id,
            selectedProcess.Name);

        return new TraceTimelineModel
        {
            TraceStart = trace.TraceStart,
            TraceEnd = trace.TraceEnd,
            ProcessStart = processStart,
            ProcessEnd = processEnd,
            SelectedProcess = selectedProcess,
            TrackOneItems = trackOne,
            TrackTwoItems = new List<TimelineItem>(),  // Always empty; built dynamically in TimelineWindow
        };
    }
}

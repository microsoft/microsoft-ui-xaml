using System;
using System.Collections.ObjectModel;
using CommunityToolkit.Mvvm.ComponentModel;
using XamlTelemtryViewerWInui3.Models;

namespace XamlTelemtryViewerWInui3.Models.Timeline;

/// <summary>
/// A single Track 1 lane in the timeline: the Track 1 model for one trace''s selected
/// process, its own process picker list, and a short label. When several traces are
/// loaded at once, one lane is stacked per trace, all sharing a common time scale.
/// Each lane picks its process independently via <see cref="SelectedProcess"/>.
/// </summary>
public sealed partial class TimelineLane : ObservableObject
{
    public required string TraceLabel { get; init; }

    public required TraceData Trace { get; init; }

    public ObservableCollection<ProcessInfo> Processes { get; } = new();

    [ObservableProperty]
    public partial ProcessInfo? SelectedProcess { get; set; }

    /// <summary>Track 1 model for this trace''s selected process; null until built.</summary>
    [ObservableProperty]
    public partial TraceTimelineModel? Model { get; set; }

    /// <summary>Label shown above the lane (trace + selected process).</summary>
    public string Label => SelectedProcess is null
        ? TraceLabel
        : $"{TraceLabel} - {SelectedProcess.Name} ({SelectedProcess.Id})";

    /// <summary>Set by the view model so a process change rebuilds just this lane.</summary>
    public Action<TimelineLane>? OnProcessChanged { get; set; }

    partial void OnSelectedProcessChanged(ProcessInfo? value)
    {
        OnProcessChanged?.Invoke(this);
        OnPropertyChanged(nameof(Label));
    }
}

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;
using XamlTelemtryViewerWInui3.Models;
using XamlTelemtryViewerWInui3.Models.Timeline;
using XamlTelemtryViewerWInui3.Services;

namespace XamlTelemtryViewerWInui3.ViewModels;

public sealed partial class TimelineViewModel : ObservableObject
{
    private static readonly Guid XamlProviderGuid = new("531a35ab-63ce-4bcf-aa98-f88c7a89e455");

    private readonly TraceTimelineBuilder _builder = new();
    private readonly IReadOnlyList<TraceData> _traces;
    private readonly IReadOnlyList<PhaseDefinition> _definitions;
    private bool _suppressRebuild;

    [ObservableProperty]
    public partial ProcessInfo? SelectedProcess { get; set; }

    [ObservableProperty]
    public partial TraceTimelineModel? Model { get; set; }

    [ObservableProperty]
    public partial string StatusMessage { get; set; } = string.Empty;

    [ObservableProperty]
    public partial bool IsTrack2Visible { get; set; } = false;

    [ObservableProperty]
    public partial bool IsActionButtonsVisible { get; set; } = false;

    [ObservableProperty]
    public partial bool ShowAllProviders { get; set; } = false;

    public TimelineViewModel(IReadOnlyList<TraceData> traces, IReadOnlyList<PhaseDefinition> definitions)
    {
        _traces = traces ?? throw new ArgumentNullException(nameof(traces));
        _definitions = definitions ?? throw new ArgumentNullException(nameof(definitions));
        if (_traces.Count == 0) throw new ArgumentException("At least one trace is required.", nameof(traces));

        // Primary trace also feeds Track 2/3 drill-down (Model = lane 0).
        var primary = _traces[0];
        Processes = new ObservableCollection<ProcessInfo>(primary.Processes);
        _suppressRebuild = true;
        SelectedProcess = PickDefaultProcess(primary);
        _suppressRebuild = false;

        BuildAllLanes();
    }

    public ObservableCollection<ProcessInfo> Processes { get; }

    /// <summary>One Track 1 lane per loaded trace, stacked top-to-bottom on a shared scale.</summary>
    public ObservableCollection<TimelineLane> Lanes { get; } = new();

    /// <summary>Raised whenever a lane's process changes so the window can redraw all lanes.</summary>
    public event Action? LanesChanged;

    partial void OnSelectedProcessChanged(ProcessInfo? value)
    {
        if (_suppressRebuild || Lanes.Count == 0)
        {
            return;
        }

        // Keep the single picker in sync with lane 0's picker.
        if (!ReferenceEquals(Lanes[0].SelectedProcess, value))
        {
            Lanes[0].SelectedProcess = value;
        }
    }

    private void BuildAllLanes()
    {
        Lanes.Clear();
        foreach (var trace in _traces)
        {
            var lane = new TimelineLane { TraceLabel = trace.Label, Trace = trace };
            foreach (var p in trace.Processes) lane.Processes.Add(p);
            lane.SelectedProcess = PickDefaultProcess(trace);
            lane.Model = lane.SelectedProcess is null ? null : _builder.Build(trace, _definitions, lane.SelectedProcess);
            lane.OnProcessChanged = RebuildLane;  // wire after initial build
            Lanes.Add(lane);
        }

        Model = Lanes.Count > 0 ? Lanes[0].Model : null;
        ReportStatus();
    }

    private void RebuildLane(TimelineLane lane)
    {
        lane.Model = lane.SelectedProcess is null
            ? null
            : _builder.Build(lane.Trace, _definitions, lane.SelectedProcess);

        if (Lanes.Count > 0 && ReferenceEquals(lane, Lanes[0]))
        {
            _suppressRebuild = true;
            SelectedProcess = lane.SelectedProcess;  // keep header picker in sync
            _suppressRebuild = false;
            Model = lane.Model;
        }

        ReportStatus();
        LanesChanged?.Invoke();
    }

    private void ReportStatus()
    {
        var total = Lanes.Sum(l => l.Model?.TrackOneItems.Count ?? 0);
        var perLane = string.Join(" | ", Lanes.Select(l => $"{l.TraceLabel}:{l.Model?.TrackOneItems.Count ?? 0}"));
        StatusMessage = _traces.Count == 1
            ? $"{Lanes[0].Label} — track 1: {total} item(s)."
            : $"{Lanes.Count} traces — {total} total. [{perLane}]";
    }

    /// <summary>
    /// Picks the process most likely to be the WinUI app being analyzed:
    /// the one with the highest number of XAML-provider events. Falls back
    /// to the process with the most events overall, or the first.
    /// </summary>
    private static ProcessInfo? PickDefaultProcess(TraceData trace)
    {
        if (trace.Processes.Count == 0)
        {
            return null;
        }

        var xamlCounts = trace.Events
            .Where(e => e.ProviderGuid == XamlProviderGuid)
            .GroupBy(e => e.ProcessId)
            .ToDictionary(g => g.Key, g => g.Count());

        var best = trace.Processes
            .Select(p => (Process: p, XamlEvents: xamlCounts.GetValueOrDefault(p.Id, 0)))
            .Where(x => x.XamlEvents > 0)
            .OrderByDescending(x => x.XamlEvents)
            .FirstOrDefault();

        return best.Process ?? trace.Processes[0];
    }
}

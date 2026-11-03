// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Immutable;
using System.Globalization;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Launch;

// What a launch timeline's times are measured from.
public enum LaunchOrigin
{
    // The process's creation.
    ProcessCreated,
    // The start of the recording, because the process's creation is not known: for a recorded trace, the process's first
    // event in it.
    RecordingStart,
    // The first milestone of the process's launcher, which it reached before the process was created
    // (LaunchTimeline.OriginLabel).
    LauncherMilestone
}

// Why a launch timeline may be incomplete.
[Flags]
public enum LaunchNotes
{
    None = 0,
    // The profiler connected to a running process, so the milestones before the recording started were not recorded. The
    // first frame drawn is not reached: the first frame the recording saw need not be the process's first.
    Attached = 1,
    // The process's creation is not known, as in a recorded trace without the kernel's process events.
    NoProcessStart = 2,
    // The session file deleted its oldest events to stay within its size limit before they were read, so milestones among
    // them may be missing or show a later event. The first frame drawn is then not reached unless it was found before, as
    // the Frames tab shows no launch frames for such a file.
    Trimmed = 4
}

// A milestone the session reached: Label, and Definition, the milestone of the configuration it is (for one with
// LabelFrom, one of its values); when, in milliseconds from the timeline's origin and since the milestone before it; how
// many events matched (1 for a built-in milestone), and the thread of the first, null for the process's creation. For a
// launcher milestone (MilestoneSource.Launcher), the events are the launcher's within MarkerProviderSettings.LauncherWindow
// of the process's creation (in a recorded trace, only those nearer it than the launcher's creation of any other process),
// and the one nearest the creation marks it.
public sealed record LaunchMilestone(string Label, MilestoneDefinition Definition, double Milliseconds,
    double? SincePreviousMilliseconds, long Count, int? ThreadId);

// A session's launch timeline: the milestones reached in time order, and those not reached in the configuration's order,
// without those of a "process" when the session's process name is not known.
// RecordingStartMilliseconds is when the recording started, from the origin; null for a recorded trace, which was
// recording before its process's first event. OriginLabel is the label of the launcher milestone the times are from, for
// LaunchOrigin.LauncherMilestone; the session names the launcher (SessionFileInfo.LauncherProcessId).
public sealed record LaunchTimeline(SessionFileInfo Session, LaunchOrigin Origin, double? RecordingStartMilliseconds,
    IReadOnlyList<LaunchMilestone> Milestones, IReadOnlyList<MilestoneDefinition> NotReached, LaunchNotes Notes,
    string? OriginLabel = null);

// Finds a session file's launch milestones. Each Read reads only the events stored since the one before, so calling it
// again follows a session that is still being recorded; a Read that is cancelled or fails can be repeated. A reader keeps
// the milestones it found when the session file later deletes their events to stay within its size limit; a new reader
// of the file may not find them, and notes it (LaunchNotes.Trimmed). Not thread-safe.
public sealed class LaunchTimelineReader
{
    private static readonly double LauncherWindow = MarkerProviderSettings.LauncherWindow.TotalMilliseconds;

    private readonly string path;
    // The first event of each of the process's milestones found so far, by the index of its definition and its label.
    // Times here and in launcherEvents are kept on the events' clock and converted on each Read, as a recorded trace's
    // frequency can become known after the events were read.
    private Dictionary<(int Definition, string Label), Reached> reached = [];
    // Each launcher milestone's events found so far, by the same key. The one nearest the process's creation marks it,
    // and a recorded trace's session file may have had none of its events when the creation was read.
    private Dictionary<(int Definition, string Label), ImmutableList<LauncherEvent>> launcherEvents = [];
    // The first frame drawn, once found. Its time is kept in milliseconds: LaunchSummary has no rows until the file's
    // start and frequency are known, and neither changes after.
    private DrawnFrame? firstFrame;
    private long? lastSeq;
    private bool trimmed;

    public LaunchTimelineReader(string path, MarkerConfig config)
    {
        ArgumentException.ThrowIfNullOrEmpty(path);
        ArgumentNullException.ThrowIfNull(config);
        this.path = path;
        Config = config;
    }

    public MarkerConfig Config { get; }

    public LaunchTimeline Read(CancellationToken cancellationToken = default)
    {
        // One snapshot, so the session's info, its events and frames, and the clock they are timed by agree.
        using var snapshot = SessionFile.OpenSnapshot(path);
        var session = snapshot.Info;
        var definitions = Config.Milestones
            .Select((definition, index) => (Definition: definition, Index: index))
            .Where(entry => entry.Definition.AppliesTo(session.ProcessName))
            .ToArray();
        var eventTypes = definitions
            .Where(entry => entry.Definition.Provider is not null)
            .Select(entry => (entry.Definition.Provider!.Id, entry.Definition.Event!))
            .Distinct()
            .ToArray();
        // Kept only once the read completes, so a read that is cancelled or fails can be repeated.
        var found = new Dictionary<(int Definition, string Label), Reached>(reached);
        var foundLauncher = new Dictionary<(int Definition, string Label), ImmutableList<LauncherEvent>>(launcherEvents);
        // The events are matched to the process or its launcher with the info of their own snapshot, which names the
        // launcher by the time its events are stored.
        var readSeq = snapshot.ReadEvents(eventTypes, lastSeq,
            profilerEvent => Add(found, foundLauncher, profilerEvent, definitions, session), cancellationToken);
        var attached = session.Coverage == SessionCoverage.Connection;
        var firstFrameWanted = !attached && definitions.Any(entry => entry.Definition.BuiltIn == BuiltInMilestone.FirstFrameDrawn);
        // The file deleted its oldest events by the time of this read: events not read yet were among them, or, while the
        // first frame drawn is not found, its frames may have been.
        if (session.CompleteFromSeq is { } completeFrom && (completeFrom > (lastSeq ?? 0) + 1 || (firstFrameWanted && firstFrame is null)))
        {
            trimmed = true;
        }

        reached = found;
        launcherEvents = foundLauncher;
        lastSeq = readSeq ?? lastSeq;
        if (firstFrameWanted && firstFrame is null && !trimmed)
        {
            firstFrame = snapshot.ReadFirstDrawnFrame();
        }

        // In milliseconds from the start of the recording until the origin is known.
        var created = session.ProcessStartMilliseconds;
        var milestones = new List<LaunchMilestone>();
        var notReached = new List<MilestoneDefinition>();
        foreach (var (definition, index) in definitions)
        {
            var count = milestones.Count;
            switch (definition.BuiltIn)
            {
                case BuiltInMilestone.ProcessCreated when created is { } creation:
                    milestones.Add(new(definition.Label!, definition, creation, null, 1, null));
                    break;
                case BuiltInMilestone.FirstFrameDrawn when firstFrame is { } frame:
                    milestones.Add(new(definition.Label!, definition, frame.EndMilliseconds, null, 1, frame.ThreadId));
                    break;
                case null when definition.From == MilestoneSource.Launcher:
                    if (created is { } launched)
                    {
                        foreach (var ((_, label), events) in launcherEvents.Where(entry => entry.Key.Definition == index))
                        {
                            var near = events
                                .Select(launcherEvent => (Milliseconds: snapshot.Milliseconds(launcherEvent.Qpc), Event: launcherEvent))
                                .Where(timed => Math.Abs(timed.Milliseconds - launched) <= LauncherWindow)
                                .ToArray();
                            if (near.Length > 0)
                            {
                                var nearest = near.MinBy(timed => Math.Abs(timed.Milliseconds - launched));
                                milestones.Add(new(label, definition, nearest.Milliseconds, null, near.Length, nearest.Event.ThreadId));
                            }
                        }
                    }

                    break;
                case null:
                    milestones.AddRange(reached
                        .Where(entry => entry.Key.Definition == index)
                        .Select(entry => new LaunchMilestone(entry.Key.Label, definition, snapshot.Milliseconds(entry.Value.Qpc), null,
                            entry.Value.Count, entry.Value.ThreadId)));
                    break;
            }

            // Another app's milestone isn't missing when the process's name, and so its app, isn't known.
            if (milestones.Count == count && (definition.Process is null || session.ProcessName is not null))
            {
                notReached.Add(definition);
            }
        }

        // The launcher's milestones can come before the process's creation; the times are from the earliest.
        var origin = created ?? 0;
        string? originLabel = null;
        foreach (var milestone in milestones)
        {
            if (milestone.Definition.From == MilestoneSource.Launcher && milestone.Milliseconds < origin)
            {
                origin = milestone.Milliseconds;
                originLabel = milestone.Label;
            }
        }

        // OrderBy is stable, so milestones at the same time keep the configuration's order.
        var ordered = new List<LaunchMilestone>(milestones.Count);
        foreach (var milestone in milestones.OrderBy(milestone => milestone.Milliseconds))
        {
            var milliseconds = milestone.Milliseconds - origin;
            ordered.Add(milestone with
            {
                Milliseconds = milliseconds,
                SincePreviousMilliseconds = ordered.Count == 0 ? null : milliseconds - ordered[^1].Milliseconds
            });
        }

        var notes = LaunchNotes.None;
        if (attached)
        {
            notes |= LaunchNotes.Attached;
        }

        if (created is null)
        {
            notes |= LaunchNotes.NoProcessStart;
        }

        if (trimmed)
        {
            notes |= LaunchNotes.Trimmed;
        }

        var originKind = created is null ? LaunchOrigin.RecordingStart
            : originLabel is null ? LaunchOrigin.ProcessCreated
            : LaunchOrigin.LauncherMilestone;
        return new(session, originKind, session.Coverage == SessionCoverage.Trace ? null : -origin, ordered, notReached, notes,
            originLabel);
    }

    // A launcher milestone matches only the launcher's events, and another only the process's.
    private static void Add(Dictionary<(int Definition, string Label), Reached> found,
        Dictionary<(int Definition, string Label), ImmutableList<LauncherEvent>> foundLauncher, ProfilerEvent profilerEvent,
        (MilestoneDefinition Definition, int Index)[] definitions, SessionFileInfo session)
    {
        MilestoneSource source;
        if (profilerEvent.ProcessId == session.ProcessId)
        {
            source = MilestoneSource.App;
        }
        else if (profilerEvent.ProcessId == session.LauncherProcessId)
        {
            source = MilestoneSource.Launcher;
        }
        else
        {
            return;
        }

        foreach (var (definition, index) in definitions)
        {
            if (definition.From != source || !definition.Matches(profilerEvent))
            {
                continue;
            }

            var label = definition.Label;
            if (definition.LabelFrom is { } field)
            {
                label = Convert.ToString(MilestoneDefinition.FieldValue(profilerEvent, field), CultureInfo.InvariantCulture);
                if (string.IsNullOrWhiteSpace(label))
                {
                    continue;
                }
            }

            var key = (index, label!);
            var qpc = profilerEvent.TimestampQpc;
            if (source == MilestoneSource.Launcher)
            {
                foundLauncher[key] = (foundLauncher.GetValueOrDefault(key) ?? []).Add(new(qpc, profilerEvent.ThreadId));
                continue;
            }

            // Events are stored in the order they arrived, which can differ a little from time order.
            found[key] = found.TryGetValue(key, out var first)
                ? qpc < first.Qpc ? new(qpc, profilerEvent.ThreadId, first.Count + 1) : first with { Count = first.Count + 1 }
                : new(qpc, profilerEvent.ThreadId, 1);
        }
    }

    private readonly record struct Reached(long Qpc, int ThreadId, long Count);

    private readonly record struct LauncherEvent(long Qpc, int ThreadId);
}

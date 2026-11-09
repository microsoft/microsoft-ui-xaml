// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Globalization;
using ModelContextProtocol.Server;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Sessions;

namespace WinUIProfiler.Mcp.Tools;

/// <summary>
/// How a session's launch went, from what the app's frame processor stored: the first ticks the first UI thread drew
/// (the LaunchSummary view), and the first tick, region of interest and interesting row of each kind (Ticks and
/// InterestingRows). Never the events' field values.
/// </summary>
[McpServerToolType]
internal static class LaunchTools
{
    internal static readonly string[] Views = ["milestones", "frames"];

    [McpServerTool(Name = "launch", ReadOnly = true, Idempotent = true, OpenWorld = false), Description(
        "Answers questions about how a session's launch went, from what the WinUI Profiler app's frame processor stored. " +
        "Views: \"milestones\" (the default): in time order, when the process was created (for a launch recorded from " +
        "process start), when the recording started, the first UI-thread tick, the first of each PerfXamlEvent operation " +
        "(such as WXM::InitializeForCurrentThread, WinUI initializing on a thread) with its duration and count, graphics " +
        "device creation, the first UpdateLayout pass in a region of interest, the first navigation and window focus " +
        "change, the first region of interest and the first frame drawn. \"frames\": when each of the first ten ticks that " +
        "drew on the first UI thread ended, from the process's creation for a launch recorded from process start, else " +
        "from the start of the recording; with the time since the previous one and each tick's own duration. Times are in " +
        "ms (see the session's timeBase); msFromProcessStart is given for a session recorded from process start, when it " +
        "knows when the process was created.")]
    public static Task<object> Launch(
        [Description("\"milestones\" or \"frames\". Omit it for the milestones.")] string? view = null,
        [Description(SessionCatalog.SessionPathDescription)] string? sessionPath = null,
        CancellationToken cancellationToken = default)
    {
        var chosen = ToolArguments.View(view, Views, "launch");
        return SessionCatalog.RunAsync(sessionPath, path => chosen == "frames" ? Frames(path) : Milestones(path), cancellationToken);
    }

    internal const string NoMilestones = "The session has none of the launch's milestones: no tick started on a UI " +
        "thread, no frame was drawn, and it recorded no region of interest, navigation, PerfXamlEvent operation, window " +
        "focus change or graphics device creation.";

    internal const string LaunchDeleted =
        "The oldest events, with the launch, were deleted to keep the file within its size limit.";

    /// <summary>Why a session has no launch frames.</summary>
    internal static string NoLaunch(SessionFileInfo info, int ticks) =>
        info.CompleteFromSeq is not null
            ? "The oldest events, with the launch's first frames, were deleted to keep the file within its size limit."
            : ticks == 0
                ? FrameTools.NoTicks
                : "No tick on the first UI thread (the thread of the session's first tick) is known to have drawn: a tick " +
                    "drew when the thread's next tick has a higher frame number.";

    private static ToolAnswer Frames(string path)
    {
        var info = SessionFile.ReadInfo(path);
        var launch = SessionFile.ReadLaunchSummary(path);
        var ticks = SessionFile.ReadFrames(path);
        var launchTicks = LaunchTicks(launch, ticks);
        var notes = Notes(info, launch, ticks.Count, launchTicks);
        var frames = new List<object>();
        for (var index = 0; index < launch.Count; index++)
        {
            var frame = launch[index];
            var tick = launchTicks?[index];
            frames.Add(new
            {
                position = index + 1,
                frameNumber = frame.FrameNumber,
                ms = EventJson.Ms(frame.Milliseconds),
                sincePreviousMs = index == 0 ? null : EventJson.Ms(frame.Milliseconds - launch[index - 1].Milliseconds),
                tickId = tick?.FrameId,
                threadId = tick?.ThreadId,
                tickStartMs = EventJson.Ms(tick?.StartMilliseconds),
                tickEndMs = EventJson.Ms(tick?.EndMilliseconds),
                tickMs = EventJson.Ms(tick?.TickMilliseconds),
                slow = tick?.Slow,
            });
        }

        var launchFrames = EventJson.Count(launch.Count, "launch frame", "launch frames");
        var computed = launch.Count == 0
            ? "The session has no launch frames, so there are none to list."
            : launchTicks is null
                ? $"Listed the {launchFrames}, each with the time since the previous one. Their ticks aren't given: the " +
                    $"{EventJson.Count(ticks.Count, "tick", "ticks")} read after them don't start with the same frames, so the " +
                    "session changed between the reads."
                : string.Create(CultureInfo.InvariantCulture,
                    $"Matched the {launchFrames} to the first {launch.Count} ticks that drew and ended on the first UI thread " +
                    $"(the thread of the first of the {EventJson.Count(ticks.Count, "tick", "ticks")}), by frame number, and " +
                    $"gave each the time since the previous one.");
        return new(new
        {
            session = SessionCatalog.Header(path, info),
            view = "frames",
            measuredFrom = MeasuredFrom(launch),
            frames,
            notes,
        }, computed);
    }

    private static ToolAnswer Milestones(string path)
    {
        var info = SessionFile.ReadInfo(path);
        var launch = SessionFile.ReadLaunchSummary(path);
        var ticks = SessionFile.ReadFrames(path);
        var rows = SessionFile.ReadInterestingRows(path);
        var milestones = BuildMilestones(info, launch, ticks, rows);
        var notes = Notes(info, launch, ticks.Count, LaunchTicks(launch, ticks));
        if (!rows.Any(row => row.Type is InterestingRowType.UpdateLayout))
        {
            notes.Add("UpdateLayout passes are listed only inside regions of interest, and the session has none there.");
        }

        if (info.CompleteFromSeq is null && !milestones.Any(item => IsObserved(item.Milestone)))
        {
            notes.Add(NoMilestones);
        }

        var recorded = new List<string>();
        if (milestones.Any(item => item.Milestone == "ProcessCreated"))
        {
            recorded.Add("when the process was created");
        }

        if (milestones.Any(item => item.Milestone == "RecordingStarted"))
        {
            recorded.Add("when the recording started");
        }

        var computed = $"Took the first of the {EventJson.Count(ticks.Count, "tick", "ticks")} to start and the first of the " +
            $"{EventJson.Count(launch.Count, "launch frame", "launch frames")}; then, from the " +
            $"{EventJson.Count(rows.Count, "interesting row", "interesting rows")}, the first region, operation of each name, and " +
            "CreateGraphicsDevice, UpdateLayout, Navigating, Navigated and FocusChange row, with how many of each there are" +
            (recorded.Count == 0 ? "" : $"; added {string.Join(" and ", recorded)}, from the session record") +
            $"; and sorted the {EventJson.Count(milestones.Count, "milestone", "milestones")} by time.";
        return new(new
        {
            session = SessionCatalog.Header(path, info),
            view = "milestones",
            firstFrame = launch.Count == 0 ? null : new
            {
                frameNumber = launch[0].FrameNumber,
                ms = EventJson.Ms(launch[0].Milliseconds),
                measuredFrom = MeasuredFrom(launch),
            },
            milestones = milestones.OrderBy(item => item.Ms).ThenBy(item => item.Order).Select(item => item.Item).ToList(),
            notes,
        }, computed);
    }

    /// <summary>
    /// How many of the launch's milestones a session's processed data holds, other than when the process was created
    /// and when the recording started, which every session recorded live has. A launch that never drew a frame can
    /// still have some.
    /// </summary>
    internal static int ObservedMilestones(SessionFileInfo info, IReadOnlyList<LaunchFrame> launch,
        IReadOnlyList<SessionFrame> ticks, IReadOnlyList<InterestingRow> rows) =>
        BuildMilestones(info, launch, ticks, rows).Count(item => IsObserved(item.Milestone));

    private static bool IsObserved(string milestone) => milestone is not ("ProcessCreated" or "RecordingStarted");

    /// <summary>The launch's milestones, in the order they're found; Order breaks ties between equal times.</summary>
    private static List<(string Milestone, double Ms, int Order, object Item)> BuildMilestones(SessionFileInfo info,
        IReadOnlyList<LaunchFrame> launch, IReadOnlyList<SessionFrame> ticks, IReadOnlyList<InterestingRow> rows)
    {
        var launchTicks = LaunchTicks(launch, ticks);
        var processStart = info.Coverage == SessionCoverage.ProcessStart ? info.ProcessStartMilliseconds : null;
        var milestones = new List<(string Milestone, double Ms, int Order, object Item)>();

        void Add(string milestone, double ms, int? threadId = null, double? durationMs = null, string? name = null,
            string? trigger = null, long? count = null, long? tickId = null, long? frameNumber = null, long? regionId = null,
            long? endsWithTickId = null) =>
            milestones.Add((milestone, ms, milestones.Count, new
            {
                milestone,
                ms = EventJson.Ms(ms),
                msFromProcessStart = EventJson.Ms(ms - processStart),
                threadId,
                durationMs = EventJson.Ms(durationMs),
                name,
                trigger,
                count,
                tickId,
                frameNumber,
                regionId,
                endsWithTickId,
            }));

        if (processStart is { } created)
        {
            Add("ProcessCreated", created);
        }

        if (!SessionCatalog.IsTrace(info))
        {
            Add("RecordingStarted", 0);
        }

        if (ticks.Where(tick => tick.StartMilliseconds is not null).MinBy(tick => tick.StartMilliseconds) is { } firstTick)
        {
            Add("FirstTickStarted", firstTick.StartMilliseconds!.Value, firstTick.ThreadId, firstTick.TickMilliseconds,
                tickId: firstTick.FrameId, frameNumber: firstTick.FrameNumber);
        }

        if (launch.Count > 0)
        {
            var first = launch[0];
            var tick = launchTicks?[0];
            var ended = tick?.EndMilliseconds
                ?? (first.FromProcessStart ? first.Milliseconds + info.ProcessStartMilliseconds.GetValueOrDefault() : first.Milliseconds);
            Add("FirstFrameDrawn", ended, tick?.ThreadId, tick?.TickMilliseconds, tickId: tick?.FrameId, frameNumber: first.FrameNumber);
        }

        var regions = rows.Where(row => row.Type == InterestingRowType.Region).ToList();
        if (regions.Count > 0)
        {
            var region = regions[0];
            Add("FirstRegion", region.StartMilliseconds, region.ThreadId, region.DurationMilliseconds, trigger: region.Trigger,
                count: regions.Count, regionId: FrameTools.RegionId(region), endsWithTickId: region.TickId);
        }

        foreach (var operation in rows.Where(row => row.Type == InterestingRowType.Operation).GroupBy(row => row.Name, StringComparer.Ordinal))
        {
            var row = operation.First();
            Add("Operation", row.StartMilliseconds, row.ThreadId, row.DurationMilliseconds, row.Name, count: operation.Count(),
                regionId: row.RegionId);
        }

        foreach (var (type, milestone) in new[]
        {
            (InterestingRowType.CreateGraphicsDevice, "CreateGraphicsDevice"),
            (InterestingRowType.UpdateLayout, "FirstUpdateLayoutInRegion"),
            (InterestingRowType.Navigating, "FirstNavigating"),
            (InterestingRowType.Navigated, "FirstNavigated"),
            (InterestingRowType.FocusChange, "FirstFocusChange"),
        })
        {
            var ofType = rows.Where(row => row.Type == type).ToList();
            if (ofType.Count > 0)
            {
                var row = ofType[0];
                Add(milestone, row.StartMilliseconds, row.ThreadId, row.DurationMilliseconds, count: ofType.Count,
                    regionId: row.RegionId);
            }
        }

        return milestones;
    }

    private static string? MeasuredFrom(IReadOnlyList<LaunchFrame> launch) =>
        launch.Count == 0 ? null : launch[0].FromProcessStart ? "ProcessStart" : "RecordingStart";

    /// <summary>
    /// The ticks of the launch frames, or null when the ticks read after them don't start with the same frames: the
    /// LaunchSummary view doesn't give its rows' ticks, and a session still being recorded can change between the reads.
    /// The view keeps the first UI thread's (the thread of the first tick's) ticks that drew and ended, in order.
    /// </summary>
    private static List<SessionFrame>? LaunchTicks(IReadOnlyList<LaunchFrame> launch, IReadOnlyList<SessionFrame> ticks)
    {
        if (launch.Count == 0 || ticks.Count == 0)
        {
            return launch.Count == 0 ? [] : null;
        }

        var thread = ticks[0].ThreadId;
        var drawn = ticks.Where(tick => tick.ThreadId == thread && tick.Drawn == true && tick.EndMilliseconds is not null)
            .Take(launch.Count).ToList();
        return drawn.Count == launch.Count && drawn.Zip(launch).All(pair => pair.First.FrameNumber == pair.Second.FrameNumber)
            ? drawn
            : null;
    }

    private static List<string> Notes(SessionFileInfo info, IReadOnlyList<LaunchFrame> launch, int ticks,
        List<SessionFrame>? launchTicks)
    {
        var notes = new List<string>(SessionCatalog.Notes(info));
        if (launch.Count == 0)
        {
            notes.Add(NoLaunch(info, ticks));
        }
        else if (launchTicks is null)
        {
            notes.Add("The session changed while it was read, so the launch frames' ticks aren't given; call again.");
        }

        if (info.Coverage == SessionCoverage.Connection && launch.Count > 0)
        {
            notes.Add("These are the first frames drawn after the profiler connected, not the launch's.");
        }

        if (SessionCatalog.IsTrace(info))
        {
            notes.Add("A trace doesn't say when the process was created, so launch times count from the process's first " +
                "event in the trace, not from its creation.");
        }
        else if (info.Coverage == SessionCoverage.ProcessStart && info.ProcessStartMilliseconds is null)
        {
            notes.Add("When the process was created isn't known, so launch times count from the start of the recording.");
        }

        return notes;
    }
}

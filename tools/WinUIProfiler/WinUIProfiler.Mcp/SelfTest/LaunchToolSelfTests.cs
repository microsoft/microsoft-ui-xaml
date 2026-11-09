// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Tools;
using static WinUIProfiler.Mcp.SelfTest.SessionFixtures;
using static WinUIProfiler.Mcp.SelfTest.ToolResults;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>The launch tool's views on the fixtures.</summary>
internal static class LaunchToolSelfTests
{
    public static IEnumerable<SelfTestCase> Tests =>
    [
        new("launch.frames", Frames),
        new("launch.milestones", Milestones),
        new("launch.undrawn", Undrawn),
        new("launch.coverage", Coverage),
        new("launch.arguments", Arguments),
    ];

    private static void Frames(SelfTestContext context)
    {
        var main = WriteMain(context);
        var result = Run(() => LaunchTools.Launch(view: "frames", sessionPath: main));
        Expect.Equal("frames", result.Text("view"), "The view");
        Expect.Equal("ProcessStart", result.Text("measuredFrom"), "Launch times count from the process's creation.");
        var frames = result.Items("frames");
        Expect.Equal("1,2", string.Join(",", frames.Select(frame => frame.Long("frameNumber"))), "The frames drawn");
        var firstMs = frames[0].Double("ms") ?? 0;
        Expect.True(firstMs >= 54 && firstMs < 1000, $"Frame 1 ends 4 ms into a recording that started 50 ms or more after the " +
            $"process was created; it was {firstMs} ms.");
        Expect.True(frames[0].IsNull("sincePreviousMs"), "The first frame has no previous one.");
        Expect.Equal(16.0, frames[1].Double("sincePreviousMs"), "Time between the two frames");
        Expect.Equal(3.0, frames[0].Double("tickMs"), "Frame 1's tick");
        Expect.Equal(4.0, frames[1].Double("tickMs"), "Frame 2's tick");
        Expect.Equal(1.0, frames[0].Double("tickStartMs"), "When frame 1's tick started");
        Expect.Equal((long)UiThread, frames[0].Long("threadId"), "Frame 1's thread");
        Expect.True(!frames[0].IsNull("tickId"), "Frame 1's tick is given.");
    }

    private static void Milestones(SelfTestContext context)
    {
        var main = WriteMain(context);
        var result = Run(() => LaunchTools.Launch(sessionPath: main));
        Expect.Equal("milestones", result.Text("view"), "The default view");
        var milestones = result.Items("milestones");
        Expect.Equal("ProcessCreated,RecordingStarted,FirstTickStarted,FirstFrameDrawn",
            string.Join(",", milestones.Select(item => item.Text("milestone"))), "The milestones, in time order");
        var firstTick = milestones.Single(item => item.Text("milestone") == "FirstTickStarted");
        Expect.Equal(1.0, firstTick.Double("ms"), "When the first tick started");
        Expect.Equal(3.0, firstTick.Double("durationMs"), "The first tick's duration");
        var drawn = milestones.Single(item => item.Text("milestone") == "FirstFrameDrawn");
        Expect.Equal(4.0, drawn.Double("ms"), "When the first frame was drawn");
        Expect.Equal(firstTick.Long("tickId"), drawn.Long("tickId"), "The first tick drew the first frame.");
        Expect.True((drawn.Double("msFromProcessStart") ?? 0) >= 54, "The first frame's time from the process's creation");
        Expect.Equal(1L, result.Property("firstFrame").Long("frameNumber"), "The first frame");
        HasNote(result, "UpdateLayout passes are listed only inside regions of interest");
        LacksNote(result, LaunchTools.NoMilestones);
        ExpectSummaryAgrees(main, "A session that drew");
    }

    private static void Undrawn(SelfTestContext context)
    {
        // The recording ends after one tick, so no tick is known to have drawn: a tick drew when the thread's next tick
        // has a higher frame number. A navigation in it opens a region of interest with a layout pass inside.
        var path = Write(Path.Combine(context.TempDirectory, "undrawn.sqlite"),
        [
            FrameStart(1.0, 1),
            Tl(2.0, UiThread, XamlProvider.Xaml, "FrameNavigating"),
            Manifest(3.0, UiThread, XamlProvider.Xaml, "Layout/Start", 1),
            Manifest(4.0, UiThread, XamlProvider.Xaml, "Layout/Stop", 2),
            FrameEnd(5.0, 1),
        ]);
        var result = Run(() => LaunchTools.Launch(sessionPath: path));
        var milestones = result.Items("milestones");
        Expect.Equal("ProcessCreated,RecordingStarted,FirstTickStarted,FirstRegion,FirstNavigating,FirstUpdateLayoutInRegion",
            string.Join(",", milestones.Select(item => item.Text("milestone"))), "The milestones of a launch that never drew");
        var region = milestones.Single(item => item.Text("milestone") == "FirstRegion");
        Expect.Equal(1.0, region.Double("ms"), "A trigger during a tick starts the region at the tick's start.");
        Contains(region.Text("trigger"), "What opened the region", "Navigating");
        Expect.Equal(3.0, milestones.Single(item => item.Text("milestone") == "FirstUpdateLayoutInRegion").Double("ms"),
            "When the layout pass started");
        Expect.True(result.IsNull("firstFrame"), "No frame was drawn.");
        HasNote(result, "No tick on the first UI thread (the thread of the session's first tick) is known to have drawn");
        LacksNote(result, LaunchTools.NoMilestones);
        Expect.Equal(0, Run(() => LaunchTools.Launch(view: "frames", sessionPath: path)).Items("frames").Count,
            "Launch frames of a launch that never drew");

        var summary = Run(() => SessionTools.Sessions(sessionPath: path));
        Expect.True(summary.IsNull("firstFrame"), "The summary has no first frame.");
        Expect.True(SessionToolSelfTests.Analysis(summary, "launch").Bool("available"),
            "The summary offers the milestones of a launch that never drew.");
        var launchFrames = SessionToolSelfTests.Analysis(summary, "launchFrames");
        Expect.True(!launchFrames.Bool("available"), "The summary has no launch frames.");
        Contains(launchFrames.Text("why"), "Why there are no launch frames", "is known to have drawn");
        ExpectSummaryAgrees(path, "A launch that never drew");
    }

    /// <summary>
    /// Checks that the summary calls a session's launch available exactly when the milestones view finds one of the
    /// launch's milestones, beyond when the process was created and the recording started.
    /// </summary>
    private static void ExpectSummaryAgrees(string path, string what)
    {
        var observed = Run(() => LaunchTools.Launch(sessionPath: path)).Items("milestones")
            .Any(item => item.Text("milestone") is not ("ProcessCreated" or "RecordingStarted"));
        var available = SessionToolSelfTests.Analysis(Run(() => SessionTools.Sessions(sessionPath: path)), "launch")
            .Bool("available");
        Expect.Equal(observed, available, $"{what}: the summary's launch analysis is available when the milestones view " +
            "finds a milestone");
    }

    private static void Coverage(SelfTestContext context)
    {
        var etl = Path.Combine(context.TempDirectory, "fixture.etl");
        var trace = Write(Path.Combine(context.TempDirectory, "trace.sqlite"), MainEvents, coverage: SessionCoverage.Trace,
            etlPath: etl);
        var fromTrace = Run(() => LaunchTools.Launch(sessionPath: trace));
        Expect.Equal("FirstTickStarted,FirstFrameDrawn",
            string.Join(",", fromTrace.Items("milestones").Select(item => item.Text("milestone"))),
            "The milestones of a trace without the kernel's process events: it doesn't say when the process was created, " +
            "and a trace has no recording start.");
        HasNote(fromTrace, "The trace doesn't say when the process was created");
        Contains(fromTrace.Property("session").Text("timeBase"), "The time base of a trace that doesn't say when the process " +
            "was created", "the first of the process's events in the trace; the trace doesn't say when the process was created.");
        Expect.Equal("RecordingStart", Run(() => LaunchTools.Launch(view: "frames", sessionPath: trace)).Text("measuredFrom"),
            "A trace's launch times");
        ExpectSummaryAgrees(trace, "A trace");

        // A trace with the kernel's process events says when the process was created: here, 30 ms before its first event.
        var started = Write(Path.Combine(context.TempDirectory, "trace-started.sqlite"), MainEvents,
            coverage: SessionCoverage.Trace, etlPath: etl, processStartMs: -30);
        var fromStart = Run(() => LaunchTools.Launch(sessionPath: started));
        var milestones = fromStart.Items("milestones");
        Expect.Equal("ProcessCreated,FirstTickStarted,FirstFrameDrawn",
            string.Join(",", milestones.Select(item => item.Text("milestone"))),
            "The milestones of a trace with the kernel's process events");
        var created = milestones.Single(item => item.Text("milestone") == "ProcessCreated");
        Expect.Equal(-30.0, created.Double("ms"), "When the process was created, from the trace's first event");
        Expect.Equal(0.0, created.Double("msFromProcessStart"), "When the process was created, from its creation");
        var drawn = milestones.Single(item => item.Text("milestone") == "FirstFrameDrawn");
        Expect.Equal(4.0, drawn.Double("ms"), "When the first frame was drawn, from the trace's first event");
        Expect.Equal(34.0, drawn.Double("msFromProcessStart"), "When the first frame was drawn, from the process's creation");
        LacksNote(fromStart, "say when the process was created");
        Contains(fromStart.Property("session").Text("timeBase"), "The time base of a trace that says when the process was " +
            "created", "the first of the process's events in the trace; the process was created 30 ms before it.");
        var startedFrames = Run(() => LaunchTools.Launch(view: "frames", sessionPath: started));
        Expect.Equal("ProcessStart", startedFrames.Text("measuredFrom"), "Launch times of a trace that says when the process " +
            "was created");
        Expect.Equal(34.0, startedFrames.Items("frames")[0].Double("ms"), "Frame 1's end, from the process's creation");
        var startedSummary = Run(() => SessionTools.Sessions(sessionPath: started));
        Expect.Equal(-30.0, startedSummary.Double("processStartMs"), "The summary's process creation");
        Expect.Equal(34.0, startedSummary.Property("firstFrame").Double("ms"), "The summary's first frame");
        ExpectSummaryAgrees(started, "A trace that says when the process was created");

        var connection = Write(Path.Combine(context.TempDirectory, "connection.sqlite"), MainEvents,
            coverage: SessionCoverage.Connection);
        var connected = Run(() => LaunchTools.Launch(view: "frames", sessionPath: connection));
        Expect.Equal("RecordingStart", connected.Text("measuredFrom"), "Launch times of a session from the connection");
        HasNote(connected, "These are the first frames drawn after the profiler connected, not the launch's.");
        Contains(connected.Property("session").Text("timeBase"), "The time base of a session from the connection",
            "the process was created");
        Expect.True(Run(() => LaunchTools.Launch(sessionPath: connection)).Items("milestones")
                .All(item => item.Text("milestone") != "ProcessCreated" && item.IsNull("msFromProcessStart")),
            "A session from the connection knows when the process was created, but its launch doesn't count from it.");
        ExpectSummaryAgrees(connection, "A session from the connection");

        var empty = Write(Path.Combine(context.TempDirectory, "empty.sqlite"), []);
        var none = Run(() => LaunchTools.Launch(view: "frames", sessionPath: empty));
        Expect.Equal(0, none.Items("frames").Count, "Frames in an empty session");
        Expect.True(none.IsNull("measuredFrom"), "An empty session's launch times count from nothing.");
        HasNote(none, FrameTools.NoTicks);
        var noMilestones = Run(() => LaunchTools.Launch(sessionPath: empty));
        Expect.True(noMilestones.IsNull("firstFrame"), "An empty session has no first frame.");
        HasNote(noMilestones, LaunchTools.NoMilestones);
        ExpectSummaryAgrees(empty, "An empty session");
    }

    private static void Arguments(SelfTestContext context)
    {
        var main = WriteMain(context);
        Contains(Error(() => LaunchTools.Launch(view: "phases", sessionPath: main)), "An unknown view", "'phases'", "milestones",
            "frames");
    }
}

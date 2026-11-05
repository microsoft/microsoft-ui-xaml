// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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
    }

    private static void Coverage(SelfTestContext context)
    {
        var trace = Write(Path.Combine(context.TempDirectory, "trace.sqlite"), MainEvents, coverage: SessionCoverage.Trace,
            etlPath: Path.Combine(context.TempDirectory, "fixture.etl"));
        var fromTrace = Run(() => LaunchTools.Launch(sessionPath: trace));
        Expect.Equal("FirstTickStarted,FirstFrameDrawn",
            string.Join(",", fromTrace.Items("milestones").Select(item => item.Text("milestone"))),
            "A trace's milestones: it doesn't say when the process was created or when a recording started.");
        HasNote(fromTrace, "A trace doesn't say when the process was created");
        Expect.Equal("RecordingStart", Run(() => LaunchTools.Launch(view: "frames", sessionPath: trace)).Text("measuredFrom"),
            "A trace's launch times");

        var connection = Write(Path.Combine(context.TempDirectory, "connection.sqlite"), MainEvents,
            coverage: SessionCoverage.Connection);
        var connected = Run(() => LaunchTools.Launch(view: "frames", sessionPath: connection));
        Expect.Equal("RecordingStart", connected.Text("measuredFrom"), "Launch times of a session from the connection");
        HasNote(connected, "These are the first frames drawn after the profiler connected, not the launch's.");

        var empty = Write(Path.Combine(context.TempDirectory, "empty.sqlite"), []);
        var none = Run(() => LaunchTools.Launch(view: "frames", sessionPath: empty));
        Expect.Equal(0, none.Items("frames").Count, "Frames in an empty session");
        Expect.True(none.IsNull("measuredFrom"), "An empty session's launch times count from nothing.");
        HasNote(none, FrameTools.NoTicks);
        Expect.True(Run(() => LaunchTools.Launch(sessionPath: empty)).IsNull("firstFrame"), "An empty session has no first frame.");
    }

    private static void Arguments(SelfTestContext context)
    {
        var main = WriteMain(context);
        Contains(Error(() => LaunchTools.Launch(view: "phases", sessionPath: main)), "An unknown view", "'phases'", "milestones",
            "frames");
    }
}

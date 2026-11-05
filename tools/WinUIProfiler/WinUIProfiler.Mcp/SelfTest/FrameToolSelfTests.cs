// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;
using WinUIProfiler.Mcp.Tools;
using static WinUIProfiler.Mcp.SelfTest.SessionFixtures;
using static WinUIProfiler.Mcp.SelfTest.ToolResults;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>The frames tool's views on the fixtures.</summary>
internal static class FrameToolSelfTests
{
    /// <summary>
    /// A navigation in frame 1 opens a region of interest that ends with that tick, and a layout pass inside it is an
    /// interesting row; frame 3 is still open when the recording ends.
    /// </summary>
    private static IReadOnlyList<FixtureEvent> RegionEvents { get; } =
    [
        FrameStart(1.0, 1),
        Tl(2.0, UiThread, XamlProvider.Xaml, "FrameNavigating"),
        Manifest(3.0, UiThread, XamlProvider.Xaml, "Layout/Start", 1),
        Manifest(4.0, UiThread, XamlProvider.Xaml, "Layout/Stop", 2),
        FrameEnd(5.0, 1),
        FrameStart(20.0, 2),
        FrameEnd(22.0, 2),
        FrameStart(30.0, 3),
    ];

    public static IEnumerable<SelfTestCase> Tests =>
    [
        new("frames.summary", Summary),
        new("frames.slowest", Slowest),
        new("frames.regions", Regions),
        new("frames.tick", Tick),
        new("frames.arguments", Arguments),
    ];

    private static void Summary(SelfTestContext context)
    {
        var main = WriteMain(context);
        var result = Run(() => FrameTools.Frames(sessionPath: main));
        Expect.Equal("summary", result.Text("view"), "The default view");
        Expect.Equal(3L, result.Long("ticks"), "Ticks");
        var threads = result.Items("uiThreads");
        Expect.Equal(1, threads.Count, "UI threads");
        var thread = threads[0];
        Expect.Equal((long)UiThread, thread.Long("threadId"), "The UI thread");
        Expect.Equal(3L, thread.Long("ticks"), "The thread's ticks");
        Expect.Equal(2L, thread.Long("drew"), "Ticks that drew");
        Expect.Equal(0L, thread.Long("didNotDraw"), "Ticks that didn't draw");
        Expect.Equal(1L, thread.Long("drewUnknown"), "Ticks not known to have drawn");
        Expect.Equal(0L, thread.Long("slow"), "Slow ticks");
        var tickMs = thread.Property("tickMs");
        Expect.Equal(2L, tickMs.Long("count"), "Ticks with a duration");
        Expect.Equal(7.0, tickMs.Double("totalMs"), "Time in ticks");
        Expect.Equal(4.0, tickMs.Double("maxMs"), "The longest tick");
        Expect.Equal(13L, thread.Property("events").Long("total"), "Events in the thread's ticks");
        Expect.Equal(0L, result.Property("regions").Long("count"), "Regions");
        Expect.Equal(1L, result.Property("firstFrame").Long("frameNumber"), "The first frame");
        HasNote(result, "1 tick hadn't ended when the recording stopped, so its duration isn't known.");
        HasNote(result, FrameTools.NoRegions);

        var empty = Write(Path.Combine(context.TempDirectory, "empty.sqlite"), []);
        var none = Run(() => FrameTools.Frames(sessionPath: empty));
        Expect.Equal(0L, none.Long("ticks"), "Ticks in an empty session");
        HasNote(none, FrameTools.NoTicks);
    }

    private static void Slowest(SelfTestContext context)
    {
        var main = WriteMain(context);
        var result = Run(() => FrameTools.Frames(sessionPath: main, view: "slowest"));
        Expect.Equal("duration", result.Text("orderBy"), "The default order");
        Expect.Equal(2L, result.Long("total"), "Ticks with a duration");
        var ticks = result.Items("ticks");
        Expect.Equal("2:4,1:3", string.Join(",", ticks.Select(tick => $"{tick.Long("frameNumber")}:{tick.Double("tickMs")}")),
            "The ticks, longest first");
        Expect.True(ticks.All(tick => tick.Bool("drew")), "Both ticks drew.");
        HasNote(result, "1 tick isn't ranked because its duration isn't known");

        var byEvents = Run(() => FrameTools.Frames(sessionPath: main, view: "slowest", orderBy: "events"));
        Expect.Equal(1L, byEvents.Items("ticks")[0].Long("frameNumber"), "The tick with the most events");
        var page = Run(() => FrameTools.Frames(sessionPath: main, view: "slowest", offset: 1, limit: 1));
        Expect.Equal(1L, page.Long("returned"), "Ticks on a page");
        Expect.Equal(1L, page.Items("ticks")[0].Long("frameNumber"), "The second slowest tick");

        var open = Write(Path.Combine(context.TempDirectory, "open.sqlite"),
        [
            FrameStart(1.0, 1),
            Tl(2.0, UiThread + 1, XamlProvider.Xaml, "CoreServices_Frame", F("IsStart", true), F("FrameNumber", 7u)),
        ]);
        HasNote(Run(() => FrameTools.Frames(sessionPath: open, view: "slowest")),
            "2 ticks aren't ranked because their durations aren't known");
        HasNote(Run(() => FrameTools.Frames(sessionPath: open)),
            "2 ticks hadn't ended when the recording stopped, so their durations aren't known.");
    }

    private static void Regions(SelfTestContext context)
    {
        var path = Write(Path.Combine(context.TempDirectory, "regions.sqlite"), RegionEvents);
        var summary = Run(() => FrameTools.Frames(sessionPath: path));
        Expect.Equal(1L, summary.Property("regions").Long("count"), "Regions in the summary");

        var result = Run(() => FrameTools.Frames(sessionPath: path, view: "regions"));
        Expect.Equal(1L, result.Long("total"), "Regions");
        var region = result.Items("regions")[0];
        Contains(region.Text("trigger"), "The region's trigger", "Navigating");
        Expect.Equal((long)UiThread, region.Long("threadId"), "The region's thread");
        Expect.Equal(5.0, region.Double("stopMs"), "The region ends with its tick");

        var rows = Run(() => FrameTools.Frames(sessionPath: path, regionId: region.Long("regionId")));
        Expect.Equal("rows", rows.Text("view"), "The view with a regionId");
        var types = rows.Items("rows").Select(row => row.Text("type")).ToList();
        Expect.True(types.Contains("UpdateLayout"), $"The region's rows include its layout pass; they were {string.Join(",", types)}.");
        Expect.True(rows.Items("rows").All(row => row.Long("regionId") == region.Long("regionId")), "Every row is in the region.");

        var layouts = Run(() => FrameTools.Frames(sessionPath: path, type: "updatelayout"));
        Expect.True(layouts.Items("rows").All(row => row.Text("type") == "UpdateLayout"), "The rows view by type");
        Expect.True(layouts.Long("total") >= 1, "The session has a layout row.");

        Expect.Equal($"There's no region 99 in {path}. The regions view lists the regions with their regionId.",
            Error(() => FrameTools.Frames(sessionPath: path, regionId: 99)), "An unknown region");
    }

    private static void Tick(SelfTestContext context)
    {
        var main = WriteMain(context);
        var first = Run(() => FrameTools.Frames(sessionPath: main, view: "slowest")).Items("ticks")
            .First(tick => tick.Long("frameNumber") == 1).Long("tickId");
        var result = Run(() => FrameTools.Frames(sessionPath: main, tickId: first));
        Expect.Equal("tick", result.Text("view"), "The view with a tickId");
        var tick = result.Property("tick");
        Expect.Equal(1L, tick.Long("frameNumber"), "The tick's frame");
        Expect.Equal(1.0, tick.Double("startMs"), "When the tick started");
        Expect.Equal(4.0, tick.Double("endMs"), "When the tick ended");
        Expect.Equal(7L, tick.Long("events"), "The tick's events");
        Expect.Equal(4L, tick.Long("xaml"), "The tick's XAML events");
        Expect.Equal(1L, tick.Long("composition"), "The tick's Composition events");
        Expect.Equal(7L, result.Long("total"), "Events listed");
        var objectKinds = result.Items("objects").Select(item => item.Text("kind")).Distinct().Order(StringComparer.Ordinal);
        Expect.Equal("Element,Visual", string.Join(",", objectKinds), "The kinds of objects the tick's events mention");
        var mentions = result.Items("page").SelectMany(item => item.Items("mentions")).Select(item => item.Text("name")).OfType<string>();
        Expect.True(mentions.Contains("OkButton"), "The tick's events mention OkButton.");

        var composition = Run(() => FrameTools.Frames(sessionPath: main, tickId: first, bucket: "composition"));
        Expect.Equal(1L, composition.Long("matching"), "Composition events in the tick");
        Expect.True(composition.Items("page").All(item => item.Text("bucket") == "Composition"), "Only Composition events are listed.");
        var page = Run(() => FrameTools.Frames(sessionPath: main, tickId: first, offset: 2, limit: 2));
        Expect.Equal(2L, page.Long("returned"), "Events on a page");
        Expect.Equal(4L, page.Long("nextOffset"), "The next page");

        Expect.Equal($"There's no tick 999 in {main}. The slowest, regions and rows views give tick IDs.",
            Error(() => FrameTools.Frames(sessionPath: main, tickId: 999)), "An unknown tick");
    }

    private static void Arguments(SelfTestContext context)
    {
        var main = WriteMain(context);
        Expect.Equal("The tick view needs tickId. The slowest, regions and rows views give tick IDs.",
            Error(() => FrameTools.Frames(sessionPath: main, view: "tick")), "The tick view without a tickId");
        Contains(Error(() => FrameTools.Frames(sessionPath: main, view: "rows", type: "Region")), "The Region row type",
            "The regions view lists the regions.");
        Contains(Error(() => FrameTools.Frames(sessionPath: main, view: "rows", type: "Paint")), "An unknown row type",
            "'Paint' isn't a valid row type.");
        Contains(Error(() => FrameTools.Frames(sessionPath: main, tickId: 1, bucket: "Gpu")), "An unknown bucket",
            "'Gpu' isn't a valid bucket.");
        Contains(Error(() => FrameTools.Frames(sessionPath: main, view: "summary", tickId: 1)), "tickId in the summary view",
            "doesn't read tickId");
        Contains(Error(() => FrameTools.Frames(sessionPath: main, view: "frames")), "An unknown view", "isn't a view of frames.");
        Expect.Equal("limit must be from 1 to 500; it was 501.",
            Error(() => FrameTools.Frames(sessionPath: main, view: "slowest", limit: 501)), "A limit over 500");
    }
}

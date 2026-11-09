// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
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
    internal static IReadOnlyList<FixtureEvent> RegionEvents { get; } =
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

    /// <summary>
    /// One busy tick (172 events): 51 elements, each entering and applying its template, of 25 types (Type24 has three,
    /// the others two); and Probe00 to Probe33, twice each. 37 event names in all.
    /// </summary>
    private static IReadOnlyList<FixtureEvent> BusyTickEvents { get; } = BusyTick();

    /// <summary>
    /// Two ticks at the tick view's caps: frame 1 has 31 event names and 21 object types, one over each cap; frame 2 has
    /// 30 names and 20 types, exactly at the caps.
    /// </summary>
    private static IReadOnlyList<FixtureEvent> CapEdgeEvents { get; } = [.. CapEdgeTick(1, 1.0, 21, 28), .. CapEdgeTick(2, 3.0, 20, 27)];

    /// <summary>
    /// Two ticks, each inserting children into a visual and then clearing them, so that the clearing mentions the parent
    /// and every child: frame 1 clears 25 children (26 mentions, over the tick view's cap of 10 per event), frame 2
    /// clears 9 (10 mentions, at the cap).
    /// </summary>
    private static IReadOnlyList<FixtureEvent> ClearedEvents { get; } =
        [.. ClearedTick(1, 1.0, 0xE000, 25), .. ClearedTick(2, 3.0, 0xE100, 9)];

    public static IEnumerable<SelfTestCase> Tests =>
    [
        new("frames.summary", Summary),
        new("frames.slowest", Slowest),
        new("frames.regions", Regions),
        new("frames.tick", Tick),
        new("frames.tick.caps", TickCaps),
        new("frames.tick.mentions", TickMentions),
        new("frames.arguments", Arguments),
    ];

    private static List<FixtureEvent> BusyTick()
    {
        var events = new List<FixtureEvent> { FrameStart(1.0, 1) };
        for (var i = 0; i < 51; i++)
        {
            var address = 0x10000L + (i * 0x100L);
            var typeIndex = Math.Min(i / 2, 24);
            var ms = 1.01 + (i * 0.005);
            events.Add(Enter(ms, address, string.Create(CultureInfo.InvariantCulture, $"Element{i:00}"),
                string.Create(CultureInfo.InvariantCulture, $"Fixture.Type{typeIndex:00}"), 1000 + typeIndex));
            events.Add(Tl(ms + 0.002, UiThread, XamlProvider.Xaml, "FrameworkElement_ApplyTemplate", F("ObjectPointer", address)));
        }

        for (var i = 0; i < 68; i++)
        {
            events.Add(Tl(1.5 + (i * 0.005), UiThread, XamlProvider.Xaml,
                string.Create(CultureInfo.InvariantCulture, $"Probe{i % 34:00}")));
        }

        events.Add(FrameEnd(2.0, 1));
        events.Add(FrameStart(20.0, 2));
        return events;
    }

    /// <summary>
    /// A tick with one element of each of <paramref name="types"/> types, each entering and applying its template, and
    /// Probe00 onwards once each.
    /// </summary>
    private static List<FixtureEvent> CapEdgeTick(uint frame, double start, int types, int probes)
    {
        var events = new List<FixtureEvent> { FrameStart(start, frame) };
        for (var i = 0; i < types; i++)
        {
            var typeIndex = ((int)frame * 100) + i;
            var address = 0x100000L + (typeIndex * 0x100L);
            var ms = start + 0.01 + (i * 0.005);
            events.Add(Enter(ms, address, string.Create(CultureInfo.InvariantCulture, $"Edge{typeIndex}"),
                string.Create(CultureInfo.InvariantCulture, $"Fixture.Edge{typeIndex}"), 2000 + typeIndex));
            events.Add(Tl(ms + 0.002, UiThread, XamlProvider.Xaml, "FrameworkElement_ApplyTemplate", F("ObjectPointer", address)));
        }

        for (var i = 0; i < probes; i++)
        {
            events.Add(Tl(start + 0.5 + (i * 0.005), UiThread, XamlProvider.Xaml,
                string.Create(CultureInfo.InvariantCulture, $"Probe{i:00}")));
        }

        events.Add(FrameEnd(start + 1.0, frame));
        return events;
    }

    /// <summary>
    /// A tick that inserts <paramref name="children"/> visuals into <paramref name="parent"/>, then clears its children.
    /// Each child gets its object ID before the parent, which the first insertion only names.
    /// </summary>
    private static List<FixtureEvent> ClearedTick(uint frame, double start, long parent, int children)
    {
        var events = new List<FixtureEvent> { FrameStart(start, frame) };
        for (var i = 0; i < children; i++)
        {
            events.Add(VisualInserted(start + 0.01 + (i * 0.001), parent, parent * 0x100 + i + 1));
        }

        events.Add(Tl(start + 0.5, UiThread, XamlProvider.XamlProfiler, "WucVisualChildrenCleared", F("ParentVisualId", parent)));
        events.Add(FrameEnd(start + 1.0, frame));
        return events;
    }

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
        var untyped = result.Items("objects").Where(item => item.Text("type") is null).ToList();
        Expect.Equal("Visual:1", string.Join(",", untyped.Select(item => $"{item.Text("kind")}:{item.Long("objects")}")),
            "The tick's objects without a type: the parent visual");
        Expect.True(untyped.All(item => item.Bool("typeUnknown")), "A row of objects without a type says so.");
        Expect.True(result.Items("objects").Where(item => item.Text("type") is not null).All(item => item.IsNull("typeUnknown")),
            "A row of typed objects doesn't have typeUnknown.");
        HasNote(result, "1 object has no type: none of the events that mention it gave it. An element's type comes from " +
            "DependencyObject_Enter, else from a template, Loaded or render event's class name; a visual's from its insertion.");
        var mentions = result.Items("page").SelectMany(item => item.Items("mentions")).Select(item => item.Text("name")).OfType<string>();
        Expect.True(mentions.Contains("OkButton"), "The tick's events mention OkButton.");

        var composition = Run(() => FrameTools.Frames(sessionPath: main, tickId: first, bucket: "composition"));
        Expect.Equal(1L, composition.Long("matching"), "Composition events in the tick");
        Expect.Equal(7L, composition.Long("total"), "The tick's events, with a bucket");
        Expect.True(composition.Items("page").All(item => item.Text("bucket") == "Composition"), "Only Composition events are listed.");
        Expect.Equal("WucVisualChildInserted", string.Join(",", composition.Items("events").Select(item => item.Text("name"))),
            "The names of the tick's Composition events");
        Expect.Equal("Visual", string.Join(",", composition.Items("objects").Select(item => item.Text("kind")).Distinct()),
            "The kinds of objects the tick's Composition events mention");
        HasNote(composition, "1 object has no type");
        var page = Run(() => FrameTools.Frames(sessionPath: main, tickId: first, offset: 2, limit: 2));
        Expect.Equal(2L, page.Long("returned"), "Events on a page");
        Expect.Equal(4L, page.Long("nextOffset"), "The next page");

        Expect.Equal($"There's no tick 999 in {main}. The slowest, regions and rows views give tick IDs.",
            Error(() => FrameTools.Frames(sessionPath: main, tickId: 999)), "An unknown tick");
    }

    private static void TickCaps(SelfTestContext context)
    {
        var path = Write(Path.Combine(context.TempDirectory, "busy.sqlite"), BusyTickEvents);
        var busy = Run(() => FrameTools.Frames(sessionPath: path, view: "slowest")).Items("ticks")
            .First(tick => tick.Long("frameNumber") == 1).Long("tickId");
        var result = Run(() => FrameTools.Frames(sessionPath: path, tickId: busy));
        Expect.Equal(172L, result.Long("total"), "The busy tick's events");
        Expect.Equal(37L, result.Long("eventNames"), "Event names in the tick");
        var names = result.Items("events").Select(item => item.Text("name")).ToList();
        Expect.Equal(30, names.Count, "Event names listed");
        Expect.Equal("DependencyObject_Enter,FrameworkElement_ApplyTemplate,CoreServices_Frame,Probe00",
            string.Join(",", names.Take(4)), "The names with the most events come first");
        Expect.Equal("Probe26", names[^1], "The last name listed");
        HasNote(result, "Only the 30 of 37 event names with the most events are listed; the other 7 names have 14 events. " +
            "Pass bucket (Xaml, Composition or Other) to list one bucket's names.");
        Expect.Equal(25L, result.Long("objectTypes"), "Types of the objects the tick's events mention");
        var types = result.Items("objects");
        Expect.Equal(20, types.Count, "Types listed");
        Expect.Equal("Fixture.Type24:3", $"{types[0].Text("type")}:{types[0].Long("objects")}",
            "The type with the most objects comes first");
        Expect.Equal("Fixture.Type18", types[^1].Text("type"), "The last type listed");
        HasNote(result, "Only the 20 of 25 types with the most objects are listed; the other 5 types have 10 objects.");
        Expect.True(types.All(item => item.IsNull("typeUnknown")), "Every object in the busy tick has a type.");
        LacksNote(result, "no type");
        Expect.Equal(20L, result.Long("returned"), "Events on the default page");
        Expect.Equal(20L, result.Long("nextOffset"), "The next page");

        var other = Run(() => FrameTools.Frames(sessionPath: path, tickId: busy, bucket: "Other"));
        Expect.Equal(172L, other.Long("total"), "The busy tick's events, with a bucket");
        Expect.Equal(70L, other.Long("matching"), "The tick's Other events");
        Expect.Equal(35L, other.Long("eventNames"), "Names of the tick's Other events");
        Expect.Equal(30, other.Items("events").Count, "Names of Other events listed");
        Expect.True(other.Items("events").All(item => item.Text("bucket") == "Other"), "Only Other events' names are listed.");
        HasNote(other, "Only the 30 of 35 event names with the most events are listed; the other 5 names have 10 events.");
        LacksNote(other, "Pass bucket");
        Expect.Equal(0L, other.Long("objectTypes"), "Types of the objects the tick's Other events mention");
        LacksNote(other, "types with the most objects");

        var xaml = Run(() => FrameTools.Frames(sessionPath: path, tickId: busy, bucket: "Xaml"));
        Expect.Equal(102L, xaml.Long("matching"), "The tick's Xaml events");
        Expect.Equal(2L, xaml.Long("eventNames"), "Names of the tick's Xaml events");
        LacksNote(xaml, "event names with the most events");
        Expect.Equal(25L, xaml.Long("objectTypes"), "Types of the objects the tick's Xaml events mention");
        Expect.Equal(20, xaml.Items("objects").Count, "Types listed for the tick's Xaml events");

        var edges = Write(Path.Combine(context.TempDirectory, "cap-edges.sqlite"), CapEdgeEvents);
        var edgeTicks = Run(() => FrameTools.Frames(sessionPath: edges, view: "slowest")).Items("ticks")
            .ToDictionary(tick => tick.Long("frameNumber"), tick => tick.Long("tickId"));
        var over = Run(() => FrameTools.Frames(sessionPath: edges, tickId: edgeTicks[1]));
        Expect.Equal(31L, over.Long("eventNames"), "Event names in a tick one over the cap");
        Expect.Equal(30, over.Items("events").Count, "Event names listed for a tick one over the cap");
        HasNote(over, "Only the 30 of 31 event names with the most events are listed; the other 1 name has 1 event.");
        Expect.Equal(21L, over.Long("objectTypes"), "Types in a tick one over the cap");
        Expect.Equal(20, over.Items("objects").Count, "Types listed for a tick one over the cap");
        HasNote(over, "Only the 20 of 21 types with the most objects are listed; the other 1 type has 1 object.");

        var at = Run(() => FrameTools.Frames(sessionPath: edges, tickId: edgeTicks[2]));
        Expect.Equal(30L, at.Long("eventNames"), "Event names in a tick at the cap");
        Expect.Equal(30, at.Items("events").Count, "Event names listed for a tick at the cap");
        LacksNote(at, "event names with the most events");
        Expect.Equal(20L, at.Long("objectTypes"), "Types in a tick at the cap");
        Expect.Equal(20, at.Items("objects").Count, "Types listed for a tick at the cap");
        LacksNote(at, "types with the most objects");
    }

    private static void TickMentions(SelfTestContext context)
    {
        var path = Write(Path.Combine(context.TempDirectory, "cleared.sqlite"), ClearedEvents);
        var ticks = Run(() => FrameTools.Frames(sessionPath: path, view: "slowest")).Items("ticks")
            .ToDictionary(tick => tick.Long("frameNumber"), tick => tick.Long("tickId"));

        // Event paging can't bound one event: the page holds only the clearing, after the tick's start and 25 insertions.
        var over = Run(() => FrameTools.Frames(sessionPath: path, tickId: ticks[1], offset: 26, limit: 1));
        var clearing = over.Items("page").Single();
        Expect.Equal("WucVisualChildrenCleared", clearing.Text("name"), "The event on the page");
        var mentions = clearing.Items("mentions");
        Expect.Equal(FrameTools.EventMentions, mentions.Count, "Mentions listed for an event with 26");
        Expect.Equal("ParentVisualId", mentions[0].Text("field"), "The parent, in the event's own field, comes first");
        Expect.True(mentions.Skip(1).All(mention => mention.Text("field") == "child of ParentVisualId"),
            "The other mentions listed are cleared children.");
        Expect.Equal(16L, clearing.Long("omittedMentions"), "Mentions left out");
        HasNote(over, "1 event on this page mentions more than 10 objects (clearing a visual's children mentions every " +
            "child). Each such event lists 10 of them, those in its own fields first, and its omittedMentions says how " +
            "many more it mentions.");
        Expect.Equal(26L, over.Items("objects").Sum(item => item.Long("objects")), "Objects counted by type, left out or not");

        var insertion = Run(() => FrameTools.Frames(sessionPath: path, tickId: ticks[1], offset: 1, limit: 1)).Items("page").Single();
        Expect.Equal(2, insertion.Items("mentions").Count, "Mentions listed for an insertion");
        Expect.True(insertion.IsNull("omittedMentions"), "An event under the cap has no omittedMentions.");

        var at = Run(() => FrameTools.Frames(sessionPath: path, tickId: ticks[2]));
        var atCap = at.Items("page").Single(item => item.Text("name") == "WucVisualChildrenCleared");
        Expect.Equal(10, atCap.Items("mentions").Count, "Mentions listed for an event with 10");
        Expect.True(atCap.IsNull("omittedMentions"), "An event at the cap has no omittedMentions.");
        LacksNote(at, "more than 10 objects");
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

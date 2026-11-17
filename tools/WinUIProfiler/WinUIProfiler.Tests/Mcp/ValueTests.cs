// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text.Json;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Mcp.Sessions;
using WinUIProfiler.Mcp.Tools;
using static WinUIProfiler.Tests.Mcp.SessionFixtures;
using static WinUIProfiler.Tests.Mcp.ToolResults;

namespace WinUIProfiler.Tests.Mcp;

/// <summary>How the tools show times, addresses and providers, and how they check their arguments.</summary>
internal static class ValueTests
{
    private static readonly string[] Views = ["first", "second view", "third"];

    public static IEnumerable<McpTestCase> Tests =>
    [
        new("values.format", _ => Format()),
        new("values.arguments", _ => Arguments()),
        new("values.providers", Providers),
    ];

    private static void Format()
    {
        Expect.Equal<double?>(1.235, EventJson.Ms(1.23456), "Milliseconds are rounded to microseconds");
        Expect.Equal<double?>(0.063, EventJson.Ms(0.0625), "Half a microsecond is rounded up");
        Expect.Equal(-91.3, EventJson.Round(-91.25, 1), "Halves are rounded away from zero");
        Expect.Equal<double?>(null, EventJson.Ms(double.NaN), "A time that isn't finite");
        Expect.Equal<double?>(null, EventJson.Ms((double?)null), "No time");
        Expect.Equal("0x1000", EventJson.Hex(4096), "An address");
        Expect.Equal("2026-01-02T03:04:05.0000000Z",
            EventJson.Utc(new DateTime(2026, 1, 2, 3, 4, 5, DateTimeKind.Unspecified)), "A stored time is UTC");
        Expect.Equal<string?>(null, EventJson.Utc((DateTime?)null), "No UTC time");
    }

    private static void Arguments()
    {
        Expect.Equal("limit must be from 1 to 500; it was 0.", Error(() => ToolArguments.Range(0, 1, 500, "limit")), "Range");
        Expect.Equal(500, ToolArguments.Range(500, 1, 500, "limit"), "The largest limit");
        Expect.Equal("offset can't be negative; it was -1.", Error(() => ToolArguments.NotNegative(-1, "offset")), "NotNegative");
        Expect.Equal(0, ToolArguments.NotNegative(0, "offset"), "A zero offset");

        Expect.Equal<string?>(null, ToolArguments.Text("   "), "Blank text");
        Expect.Equal<string?>("a b", ToolArguments.Text("  a b "), "Text is trimmed");

        Expect.Equal("first", ToolArguments.View(null, Views, "tool"), "No view is the first view");
        Expect.Equal("first", ToolArguments.View(" ", Views, "tool"), "A blank view is the first view");
        Expect.Equal("second view", ToolArguments.View(" Second View ", Views, "tool"), "A view, ignoring case");
        Expect.Equal("second view", ToolArguments.View("secondview", Views, "tool"), "A view, ignoring spaces");
        Expect.Equal("'fourth' isn't a view of tool. Use one of: first, second view, third.",
            Error(() => ToolArguments.View(" fourth ", Views, "tool")), "An unknown view");

        Expect.Equal<string?>(null, ToolArguments.Choice("", Views, "order"), "No choice");
        Expect.Equal<string?>("third", ToolArguments.Choice("THIRD", Views, "order"), "A choice, ignoring case");
        Expect.Equal("'last' isn't a valid order. Use one of: first, second view, third.",
            Error(() => ToolArguments.Choice("last", Views, "order")), "An unknown choice");

        ToolArguments.OnlyFor(false, "limit", "tool", "first", "third");
        ToolArguments.OnlyFor(true, "limit", "tool", "third", "third");
        Expect.Equal("The first view of tool doesn't read limit; only the third view does.",
            Error(() => ToolArguments.OnlyFor(true, "limit", "tool", "first", "third")), "An argument one other view reads");
        Expect.Equal("The first view of tool doesn't read limit; only the second view and third views do.",
            Error(() => ToolArguments.OnlyFor(true, "limit", "tool", "first", "second view", "third")),
            "An argument two other views read");

        Expect.Equal<long?>(20, ToolArguments.NextOffset(0, 20, 50), "The next page");
        Expect.Equal<long?>(null, ToolArguments.NextOffset(40, 10, 50), "The last page");
        Expect.Equal<long?>(null, ToolArguments.NextOffset(60, 0, 50), "Past the last page");

        Expect.Equal(4096L, ToolArguments.Address("0x1000"), "A hex address");
        Expect.Equal(4096L, ToolArguments.Address(" 0X1000 "), "A hex address with spaces and 0X");
        Expect.Equal(4096L, ToolArguments.Address(" 4096 "), "A decimal address");
        foreach (var bad in new[] { "-1", "0xZZ", "zz", "0x", "1.5" })
        {
            Contains(Error(() => ToolArguments.Address(bad)), "A bad address's error", $"'{bad}' isn't an address.");
        }
    }

    /// <summary>
    /// The tick and history views name a XAML provider by its ETW name, and a marker provider by the name its marker
    /// configuration gives it, as the app does.
    /// </summary>
    private static void Providers(McpTestContext context)
    {
        var marker = new MarkerProvider("Contoso-Startup", new Guid("6F3E1A52-0D8C-4C4B-9C1E-2B7A9E5D4F10"));
        // The main fixture, with a marker event in frame 2, before the frame's end.
        var path = Write(Path.Combine(context.TempDirectory, "markers.sqlite"),
            [.. MainEvents.Take(14), MarkerEvent(19.0, UiThread, marker, "PageReady"), .. MainEvents.Skip(14)]);
        var tickId = Run(() => FrameTools.Frames(sessionPath: path, view: "slowest")).Items("ticks")
            .First(tick => tick.Long("frameNumber") == 2).Long("tickId");
        var tick = Run(() => FrameTools.Frames(sessionPath: path, tickId: tickId));
        static string Named(JsonElement item) => $"{item.Text("provider")}:{item.Text("name")}:{item.Text("bucket")}";
        Expect.Equal("Contoso-Startup:PageReady:Other", Named(tick.Items("events").Single(item => item.Text("name") == "PageReady")),
            "The marker event's name in the tick view");
        Expect.Equal("Contoso-Startup:PageReady:Other", Named(tick.Items("page").Single(item => item.Text("name") == "PageReady")),
            "The marker event in the tick view's page");
        Expect.Equal("Microsoft-Windows-XAML:Layout/Start:Other",
            Named(tick.Items("events").Single(item => item.Text("name") == "Layout/Start")), "A XAML event's name in the tick view");

        var history = Run(() => ObjectTools.Objects(address: "0x1000", sessionPath: path));
        Expect.Equal("Microsoft-Windows-XAML-Profiler:ElementEnteredTree,Microsoft-Windows-XAML:DependencyObject_Enter",
            string.Join(",", history.Items("events").Select(item => $"{item.Text("provider")}:{item.Text("name")}")),
            "The providers and names of Root's events");
        Expect.Equal("Microsoft-Windows-XAML,Microsoft-Windows-XAML-Profiler,Microsoft-Windows-XAML-Profiler",
            string.Join(",", history.Items("page").Select(item => item.Text("provider"))), "The providers of Root's events, in order");
    }
}

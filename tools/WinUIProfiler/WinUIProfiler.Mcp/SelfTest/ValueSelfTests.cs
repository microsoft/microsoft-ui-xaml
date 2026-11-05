// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;
using WinUIProfiler.Mcp.Sessions;
using static WinUIProfiler.Mcp.SelfTest.ToolResults;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>How the tools show times, addresses and providers, and how they check their arguments.</summary>
internal static class ValueSelfTests
{
    private static readonly string[] Views = ["first", "second view", "third"];

    public static IEnumerable<SelfTestCase> Tests =>
    [
        new("values.format", _ => Format()),
        new("values.arguments", _ => Arguments()),
        new("values.providers", _ => Providers()),
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

    private static void Providers()
    {
        Expect.Equal(Enum.GetValues<XamlProvider>().Length, EventJson.Providers.Count, "Every provider has a name");
        Expect.True(EventJson.Providers.Select(info => info.Provider).SequenceEqual(Enum.GetValues<XamlProvider>()),
            "The providers are in the order of XamlProvider.");
        Expect.True(EventJson.Providers.All(info => info.Guid != Guid.Empty && info.Name.Length > 0),
            "Every provider has its GUID and name.");
        Expect.Equal("Microsoft-Windows-XAML-Profiler", EventJson.Provider(XamlProvider.XamlProfiler).Name, "A provider's name");
    }
}

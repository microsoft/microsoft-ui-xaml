// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Text.Json;
using WinUIProfiler.Mcp.Tools;
using static WinUIProfiler.Tests.Mcp.SessionFixtures;
using static WinUIProfiler.Tests.Mcp.ToolResults;

namespace WinUIProfiler.Tests.Mcp;

/// <summary>The objects tool's types, list and history views on the fixtures.</summary>
internal static class ObjectToolTests
{
    private const string TextBlockType = "Microsoft.UI.Xaml.Controls.TextBlock";
    private const string BorderType = "Microsoft.UI.Xaml.Controls.Border";

    // OkButton's address as the tools show it, with other case and spaces, and in decimal.
    private static readonly string[] OkButtonAddresses = ["0x2000", " 0X2000 ", "8192"];

    public static IEnumerable<McpTestCase> Tests =>
    [
        new("objects.types", Types),
        new("objects.list", List),
        new("objects.history", History),
        new("objects.reuse", Reuse),
        new("objects.arguments", Arguments),
    ];

    private static void Types(McpTestContext context)
    {
        var main = WriteMain(context);
        var result = Run(() => ObjectTools.Objects(sessionPath: main));
        Expect.Equal("types", result.Text("view"), "The default view");
        Expect.Equal(main, result.Property("session").Text("sessionPath"), "The session path");
        var totals = result.Items("totals");
        Expect.Equal("Element:2/2,Visual:2/1", string.Join(",", totals.Select(item =>
            $"{item.Text("kind")}:{item.Long("objects")}/{item.Long("types")}")), "Objects and types of each kind");
        Expect.Equal(4L, result.Long("total"), "Types");
        var types = result.Items("types");
        Expect.Equal(4, types.Count, "Types returned");
        Expect.True(types.Any(item => item.Text("type") == GridType && item.Long("objects") == 1 && item.Long("events") == 3),
            "Root's Grid type has one object and three events.");
        Expect.True(types.Any(item => item.Text("kind") == "Visual" && item.Bool("typeUnknown")), "One visual's type isn't known.");
        HasNote(result, "1 object has no type: none of the events that mention it gave it.");
        var twoParents = Write(Path.Combine(context.TempDirectory, "untyped.sqlite"),
            [FrameStart(1.0, 1), VisualInserted(2.0, 0xA100, 0xB100), VisualInserted(2.5, 0xA200, 0xB200), FrameEnd(4.0, 1)]);
        HasNote(Run(() => ObjectTools.Objects(sessionPath: twoParents)),
            "2 objects have no type: none of the events that mention them gave it.");

        var visuals = Run(() => ObjectTools.Objects(sessionPath: main, kind: " visual "));
        Expect.Equal("Visual", visuals.Text("kind"), "The kind filter");
        Expect.True(visuals.Items("types").All(item => item.Text("kind") == "Visual"), "Only visuals' types are listed.");

        var page = Run(() => ObjectTools.Objects(sessionPath: main, offset: 1, limit: 2));
        Expect.Equal(2L, page.Long("returned"), "Types on a page");
        Expect.Equal(3L, page.Long("nextOffset"), "The next page");

        var framesOnly = Write(Path.Combine(context.TempDirectory, "frames.sqlite"), [FrameStart(1.0, 1), FrameEnd(4.0, 1)]);
        var none = Run(() => ObjectTools.Objects(sessionPath: framesOnly));
        Expect.Equal(0L, none.Long("total"), "Types in a session without element or visual events");
        HasNote(none, ObjectTools.NoObjects);
    }

    private static void List(McpTestContext context)
    {
        var main = WriteMain(context);
        var result = Run(() => ObjectTools.Objects(sessionPath: main, view: "list"));
        Expect.Equal("list", result.Text("view"), "The view");
        Expect.Equal("time", result.Text("orderBy"), "The default order");
        Expect.Equal(4L, result.Long("total"), "Objects");
        Expect.True(result.IsNull("nextOffset"), "Every object is on the first page.");
        var objects = result.Items("objects");
        Expect.Equal("Element 0x1000 Root Grid 3@1.5-2.1,Element 0x2000 OkButton Button 2@2-2.1",
            string.Join(",", objects.Take(2).Select(Describe)), "The elements, first mentioned first");
        Expect.Equal("Visual 0xA000 - - 1@3-3,Visual 0xB000 - ContainerVisual 1@3-3",
            string.Join(",", objects.Skip(2).Select(Describe).Order(StringComparer.Ordinal)), "The visuals");
        Expect.Equal(4, objects.Select(item => item.Long("objectId")).Distinct().Count(), "Distinct objectIds");

        Expect.Equal("Root,OkButton", Names(Run(() => ObjectTools.Objects(sessionPath: main, view: "list", orderBy: " EVENTS ")), 2),
            "The objects with the most events, first");
        Expect.Equal("OkButton", Names(Run(() => ObjectTools.Objects(sessionPath: main, type: "button"))),
            "The list view by a type's short name");
        Expect.Equal("OkButton", Names(Run(() => ObjectTools.Objects(sessionPath: main, type: ButtonType))),
            "The list view by a type's full name");
        Expect.Equal("Root", Names(Run(() => ObjectTools.Objects(sessionPath: main, name: "ROO"))), "The list view by part of a name");
        Expect.Equal("0xA000,0xB000", string.Join(",", Run(() => ObjectTools.Objects(sessionPath: main, view: "list", kind: "Visual"))
            .Items("objects").Select(item => item.Text("address")).Order(StringComparer.Ordinal)), "The list view of visuals");

        var missing = Run(() => ObjectTools.Objects(sessionPath: main, type: "ListView"));
        Expect.Equal(0L, missing.Long("total"), "Objects of a type the session doesn't have");
        HasNote(missing, "No object has the type ListView.");
        var page = Run(() => ObjectTools.Objects(sessionPath: main, view: "list", offset: 1, limit: 1));
        Expect.Equal("OkButton", Names(page), "The second page of one");
        Expect.Equal(2L, page.Long("nextOffset"), "The next page");
    }

    private static void History(McpTestContext context)
    {
        var main = WriteMain(context);
        foreach (var address in OkButtonAddresses)
        {
            var result = Run(() => ObjectTools.Objects(sessionPath: main, address: address));
            Expect.Equal("OkButton", result.Property("object").Text("name"), $"The object at {address}");
        }

        var root = Run(() => ObjectTools.Objects(sessionPath: main, address: "0x1000"));
        Expect.Equal("history", root.Text("view"), "The view with an address");
        Expect.Equal(3L, root.Long("total"), "Root's events");
        Expect.Equal("DependencyObject_Enter[ObjectPointer]@1.5#1,ElementEnteredTree[ElementId]@1.6#1," +
            "ElementEnteredTree[ParentId]@2.1#1", string.Join(",", root.Items("page").Select(Mention)), "Root's history");
        Expect.True(root.Items("events").Any(item => item.Text("name") == "ElementEnteredTree" && item.Long("count") == 2),
            "Root is mentioned by two ElementEnteredTree events.");

        var byId = Run(() => ObjectTools.Objects(sessionPath: main, objectId: root.Property("object").Long("objectId")));
        Expect.Equal("Root", byId.Property("object").Text("name"), "The object by its objectId");
        var page = Run(() => ObjectTools.Objects(sessionPath: main, address: "0x1000", offset: 1, limit: 1));
        Expect.Equal(1L, page.Long("returned"), "Events on a page");
        Expect.Equal(2L, page.Long("nextOffset"), "The next page");

        Expect.Equal($"No object in {main} has objectId 999. The list view lists the objects.",
            Error(() => ObjectTools.Objects(sessionPath: main, objectId: 999)), "An unknown objectId");
        Expect.Equal($"No object in {main} has the address 0x9999. The list view lists the objects.",
            Error(() => ObjectTools.Objects(sessionPath: main, address: "0x9999")), "An unknown address");
    }

    private static void Reuse(McpTestContext context)
    {
        // A TextBlock leaves the tree and is freed; a Border then enters at the same address.
        var path = Write(Path.Combine(context.TempDirectory, "reuse.sqlite"),
        [
            FrameStart(1.0, 1),
            Enter(1.5, 0x3000, "Label", TextBlockType, 102),
            EnteredTree(1.6, 0x3000, 0, "Label", 0x7200, 200, "", 0, 0),
            LeftTree(2.0, 0x3000, 0),
            Enter(3.0, 0x3000, "Frame", BorderType, 103),
            EnteredTree(3.1, 0x3000, 0, "Frame", 0x7300, 250, "", 0, 0),
            FrameEnd(4.0, 1),
        ]);
        var objects = Run(() => ObjectTools.Objects(sessionPath: path, view: "list")).Items("objects");
        Expect.Equal("Element 0x3000 Label TextBlock 3@1.5-2,Element 0x3000 Frame Border 2@3-3.1",
            string.Join(",", objects.Select(Describe)), "Two elements at one address, one after the other");

        var both = Run(() => ObjectTools.Objects(sessionPath: path, address: "0x3000"));
        Expect.True(both.Bool("ambiguous"), "An address two objects had is ambiguous.");
        Expect.Equal("2 objects had the address 0x3000 during the session: the runtime reuses an address once its object is " +
            "freed. Call the history view again with one's objectId.", both.Text("message"), "The message");
        Expect.Equal(2, both.Items("objects").Count, "The objects it could be");
        Expect.Equal(2L, both.Long("total"), "How many objects had the address");
        Expect.True(both.IsNull("nextOffset"), "One page holds both objects.");

        // It read the session record, then the objects at the address, a read that reads the session record again.
        var ran = QueryTests.CheckQueries(both, "The history view of an address two objects had");
        Expect.Equal(2, ran.Count, "Statements the history view of an address two objects had ran");
        Expect.Equal(2L, ran[0].Long("runs"), "Runs of the session record's statement");

        var first = Run(() => ObjectTools.Objects(sessionPath: path, address: "0x3000", limit: 1));
        Expect.Equal(1L, first.Long("returned"), "Objects on a page of one");
        Expect.Equal(2L, first.Long("total"), "How many objects had the address, on a page of one");
        Expect.Equal(1L, first.Long("nextOffset"), "Where the next page of objects starts");
        Expect.Equal("Label", first.Items("objects").Single().Text("name"), "The first object on the address");
        var second = Run(() => ObjectTools.Objects(sessionPath: path, address: "0x3000", offset: 1));
        Expect.Equal(1L, second.Long("returned"), "Objects on the page after the first");
        Expect.Equal("Frame", second.Items("objects").Single().Text("name"), "The next object on the address");
        Expect.True(second.IsNull("nextOffset"), "The second page is the last.");

        var label = Run(() => ObjectTools.Objects(sessionPath: path, objectId: objects[0].Long("objectId")));
        Expect.Equal("DependencyObject_Enter[ObjectPointer]@1.5#1,ElementEnteredTree[ElementId]@1.6#1,ElementLeftTree[ElementId]@2#1",
            string.Join(",", label.Items("page").Select(Mention)), "The TextBlock's events, up to when it left the tree");
        var border = Run(() => ObjectTools.Objects(sessionPath: path, objectId: objects[1].Long("objectId")));
        Expect.Equal("DependencyObject_Enter[ObjectPointer]@3#1,ElementEnteredTree[ElementId]@3.1#1",
            string.Join(",", border.Items("page").Select(Mention)), "The Border's events");
    }

    private static void Arguments(McpTestContext context)
    {
        var main = WriteMain(context);
        Expect.Equal("'x' isn't a valid kind. Use one of: Element, Visual.",
            Error(() => ObjectTools.Objects(sessionPath: main, kind: "x")), "An unknown kind");
        Expect.Equal("'shapes' isn't a view of objects. Use one of: types, list, history.",
            Error(() => ObjectTools.Objects(sessionPath: main, view: "shapes")), "An unknown view");
        Expect.Equal("The history view needs one of objectId and address, not both.",
            Error(() => ObjectTools.Objects(sessionPath: main, objectId: 1, address: "0x1000")), "Both objectId and address");
        Expect.Equal("The history view needs objectId or address. The list view gives both.",
            Error(() => ObjectTools.Objects(sessionPath: main, view: "history")), "Neither objectId nor address");
        Contains(Error(() => ObjectTools.Objects(sessionPath: main, address: "zz")), "A bad address", "'zz' isn't an address.");
        Contains(Error(() => ObjectTools.Objects(sessionPath: main, view: "types", type: "Button")), "type in the types view",
            "doesn't read type");
        Contains(Error(() => ObjectTools.Objects(sessionPath: main, view: "list", objectId: 1)), "objectId in the list view",
            "doesn't read objectId");
        Expect.Equal("limit must be from 1 to 500; it was 0.", Error(() => ObjectTools.Objects(sessionPath: main, limit: 0)), "limit 0");
        Expect.Equal("offset can't be negative; it was -1.", Error(() => ObjectTools.Objects(sessionPath: main, offset: -1)),
            "A negative offset");
    }

    private static string Names(JsonElement result) => Names(result, int.MaxValue);

    private static string Names(JsonElement result, int count) => string.Join(",",
        result.Items("objects").Take(count).Select(item => item.Text("name") ?? item.Text("address")));

    /// <summary>An object row as "kind address name type events@first-last", with the type's short name.</summary>
    private static string Describe(JsonElement item)
    {
        var type = item.Text("type");
        var shortType = type is null ? "-" : type[(type.LastIndexOf('.') + 1)..];
        return string.Create(CultureInfo.InvariantCulture,
            $"{item.Text("kind")} {item.Text("address")} {item.Text("name") ?? "-"} {shortType} " +
            $"{item.Long("events")}@{item.Double("firstMs")}-{item.Double("lastMs")}");
    }

    /// <summary>A history event as "name[fields that hold the address]@ms#frame".</summary>
    private static string Mention(JsonElement item)
    {
        var frame = item.IsNull("frameNumber") ? "-" : item.Long("frameNumber").ToString(CultureInfo.InvariantCulture);
        return string.Create(CultureInfo.InvariantCulture,
            $"{item.Text("name")}[{string.Join("+", item.Strings("mentionedIn"))}]@{item.Double("ms")}#{frame}");
    }
}

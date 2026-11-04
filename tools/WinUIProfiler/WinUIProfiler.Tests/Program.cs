// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using XamlTimeline;

const long element = 0x1234;
var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-EventCompatibility-{Guid.NewGuid():N}.sqlite");
var visualPath = Path.ChangeExtension(path, "visuals.sqlite");
try
{
    using (var writer = SessionWriter.Create(path, new(1, "CompatibilityTests", null, 1_000_000, 0, SessionCoverage.Trace)))
    {
        ProfilerEvent Event(string name, bool traceLogging, IReadOnlyList<EventField> fields,
            XamlProvider provider = XamlProvider.Xaml, int opcode = 0) =>
            new(1, 0.001, 1, 1, provider, traceLogging, name, opcode, fields);

        EventField[] observation =
        [
            new("IsEnabled", false), new("ElementId", element), new("Name", "Items"),
            new("ClassType", "ListView"), new("PropertyType", "ItemsPanel")
        ];
        writer.Process(1, Event("GenerateContainer/Start", true, [], opcode: 1));
        writer.Process(2, Event("GenerateContainer/Stop", true, [new("ElementId", element)], opcode: 2));
        writer.Process(3, Event("VirtualizationIsEnabledByLayout", true,
        [
            new("ElementId", element), new("IsEnabled", false), new("Name", "Items"),
            new("ClassType", "ListView"), new("PropertyType", "ItemsPanel")
        ]));
        writer.Process(4, Event("VirtualizationIsEnabledByLayout", false, [new("IsEnabled", false)]));
        writer.Process(5, Event("BeginStoryboard/Start", false, [new("ElementId", element)]));
        writer.Process(6, Event("VirtualizationIsEnabledByLayout", false, observation,
            provider: XamlProvider.XamlDiagnostics));
        writer.Process(7, Event("VirtualizationIsEnabledByLayout", false, observation));
    }

    var objects = SessionFile.ReadObjects(path);
    if (objects.Count != 1 || objects[0].Address != element || objects[0].Events != 3)
    {
        throw new InvalidOperationException("The migrated activity and new/legacy virtualization observations must share one element.");
    }

    var mentions = new List<string>();
    SessionFile.ReadTimeline(path, objects[0].ObjectId, 0, -1,
        (profilerEvent, _, _) => mentions.Add(profilerEvent.Name));
    if (!mentions.SequenceEqual(new[]
        { "GenerateContainer/Stop", "VirtualizationIsEnabledByLayout", "VirtualizationIsEnabledByLayout" }))
    {
        throw new InvalidOperationException("Unrelated manifest events must not be added to the element's timeline.");
    }

    Console.WriteLine("Passed: TraceLogging activity identity and new/legacy virtualization observations share one timeline.");
    Console.WriteLine("Passed: version-0 observations, storyboard IDs and other providers are not indexed as that element.");

    const long parentVisual = 0x2000;
    const long childVisual = 0x3000;
    const long target = 0x4000;
    const long legacyVisual = 0x5000;
    const long linkedElement = 0x6000;
    const long childElement = 0x7000;
    const long owner = 0x9000;
    using (var writer = SessionWriter.Create(visualPath, new(1, "VisualIdentityTests", null, 1_000_000, 0, SessionCoverage.Trace)))
    {
        ProfilerEvent VisualEvent(string name, params EventField[] fields) =>
            new(1, 0.001, 1, 1, XamlProvider.XamlProfiler, true, name, 0, fields);

        writer.Process(1, VisualEvent("WucVisualRootSet", new("VisualId", parentVisual), new("TargetId", target)));
        writer.Process(2, VisualEvent("WucVisualChildInserted",
            new("ParentVisualId", parentVisual), new("ChildVisualId", childVisual)));
        writer.Process(3, VisualEvent("WucVisualChildRemoved",
            new("ParentVisualId", parentVisual), new("ChildVisualId", childVisual)));
        writer.Process(4, VisualEvent("WucVisualChildInserted",
            new("ParentVisualId", parentVisual), new("ChildVisualId", childVisual)));
        writer.Process(5, VisualEvent("WucVisualChildrenCleared", new EventField("ParentVisualId", parentVisual)));
        writer.Process(6, VisualEvent("WucVisualRootCleared", new EventField("TargetId", target)));
        writer.Process(7, VisualEvent("WucVisualRootSet",
            new("VisualId", legacyVisual), new("TargetId", target), new("OwnerCompNodeId", owner),
            new("VisualType", "SpriteVisual")));
        writer.Process(8, VisualEvent("CompPeerLinked",
            new("UIElementId", linkedElement), new("CompNodeId", owner), new("ParentCompNodeId", 0L),
            new("UIElementPeerHandle", 0xa000L)));
        writer.Process(9, VisualEvent("CompPeerLinked",
            new("UIElementId", linkedElement), new("CompNodeId", owner), new("ElementLabel", "LegacyOwner")));
        writer.Process(10, VisualEvent("ElementEnteredTree",
            new("ElementId", linkedElement), new("ParentId", 0L), new("IsLive", true),
            new("IsTemplateChild", false), new("PeerHandle", 0xa000L)));
        writer.Process(11, VisualEvent("ChildAdded",
            new("ParentId", linkedElement), new("ChildId", childElement), new("Index", 0),
            new("IsTemplateChild", false), new("ChildPeerHandle", 0xb000L)));
        writer.Process(12, VisualEvent("ChildInserted",
            new("ParentId", linkedElement), new("ChildId", childElement), new("Index", 0),
            new("IsTemplateChild", false), new("ChildPeerHandle", 0xb000L)));
        writer.Process(13, VisualEvent("ContentChanged",
            new("ParentId", linkedElement), new("OldContentId", 0L), new("NewContentId", childElement),
            new("NewContentPeerHandle", 0xb000L)));
        writer.Process(14, VisualEvent("PopupOpened",
            new("PopupId", linkedElement), new("ChildId", childElement), new("ChildPeerHandle", 0xb000L)));
    }

    var visuals = SessionFile.ReadObjects(visualPath, ObjectKind.Visual);
    if (visuals.Count != 3 ||
        visuals.Single(visual => visual.Address == parentVisual) is not { Type: null, Name: null, Events: 5 } ||
        visuals.Single(visual => visual.Address == childVisual) is not { Type: null, Name: null, Events: 4 } ||
        visuals.Single(visual => visual.Address == legacyVisual) is not { Type: "SpriteVisual", Name: "LegacyOwner" })
    {
        throw new InvalidOperationException("Identity-only visual events must be indexed without requiring legacy type or owner fields.");
    }

    var visualMentions = new List<string>();
    SessionFile.ReadTimeline(visualPath, visuals.Single(visual => visual.Address == childVisual).ObjectId, 0, -1,
        (profilerEvent, _, _) => visualMentions.Add(profilerEvent.Name));
    if (!visualMentions.SequenceEqual(new[]
        { "WucVisualChildInserted", "WucVisualChildRemoved", "WucVisualChildInserted", "WucVisualChildrenCleared" }))
    {
        throw new InvalidOperationException("Identity-only insert, remove and clear events must share the child's timeline.");
    }

    Console.WriteLine("Passed: identity-only visual events preserve tree history without type or owner metadata.");
    Console.WriteLine("Passed: legacy visual type and owner labels remain supported.");

    var linked = SessionFile.ReadObjects(visualPath, ObjectKind.Element, address: linkedElement).Single();
    var linkedMentions = new List<string>();
    SessionFile.ReadTimeline(visualPath, linked.ObjectId, 0, -1,
        (profilerEvent, _, _) => linkedMentions.Add(profilerEvent.Name));
    if (!linkedMentions.SequenceEqual(new[]
        { "CompPeerLinked", "CompPeerLinked", "ElementEnteredTree", "ChildAdded", "ChildInserted", "ContentChanged", "PopupOpened" }) ||
        SessionFile.ReadObjects(visualPath, ObjectKind.Element, address: childElement).Single().Events != 4)
    {
        throw new InvalidOperationException("Label-free composition links and tree events must preserve element timelines.");
    }

    Console.WriteLine("Passed: label-free composition links and UI tree events preserve element timelines.");

    var regions = new RegionsLoader().Load(Path.Combine(AppContext.BaseDirectory, "XAML.regions.complete.with.startup.xml"));
    string[] activities =
    [
        "VirtualizationMeasure", "VirtualizationCleanup", "VirtualizationAdd", "GenerateContainer",
        "MeasureChild", "PrepareContainer", "UpdateLogicalScrollData", "SetupCCC", "GenerateItems",
        "GenerateMCContainer", "PlaceElement", "GetElementCount"
    ];
    foreach (var activity in activities)
    {
        var current = regions.Single(region => region.Name == activity && region.Start?.Opcode == 1);
        var start = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventName = activity + "/Start", Opcode = 1 };
        var stop = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventName = activity + "/Stop", Opcode = 2 };
        if (current.Start is null || current.Stop is null ||
            !current.Start.Matches(start) || current.Start.Matches(stop) ||
            !current.Stop.Matches(stop) || current.Stop.Matches(start))
        {
            throw new InvalidOperationException($"{activity} must distinguish TraceLogging Start and Stop opcodes.");
        }

        var legacy = regions.Single(region => region.Name == activity && region.Start?.Opcode is null);
        if (legacy.Start?.EventId is null || legacy.Stop?.EventId is null)
        {
            throw new InvalidOperationException($"{activity} must retain its legacy manifest selectors.");
        }

        var legacyStart = new TelemetryEvent
        {
            ProviderGuid = XamlProviders.Xaml,
            EventId = legacy.Start.EventId.Value,
            EventName = activity + "/Start",
            Opcode = 1
        };
        var legacyStop = new TelemetryEvent
        {
            ProviderGuid = XamlProviders.Xaml,
            EventId = legacy.Stop.EventId.Value,
            EventName = activity + "/Stop",
            Opcode = 2
        };
        if (current.Start.Matches(legacyStart) || current.Stop.Matches(legacyStop) ||
            legacy.Start.Matches(start) || legacy.Stop.Matches(stop) ||
            !legacy.Start.Matches(legacyStart) || !legacy.Stop.Matches(legacyStop))
        {
            throw new InvalidOperationException($"{activity} must not produce duplicate regions in new or legacy traces.");
        }
    }

    try
    {
        new RegionsLoader().LoadFromString("""
            <InstrumentationManifest><Instrumentation><Regions><RegionRoot Name="Invalid">
              <Region Name="InvalidOpcode">
                <Start><Event Provider="{531a35ab-63ce-4bcf-aa98-f88c7a89e455}" Name="GenerateContainer" Opcode="256" /></Start>
                <Stop><Event Provider="{531a35ab-63ce-4bcf-aa98-f88c7a89e455}" Name="GenerateContainer" Opcode="2" /></Stop>
              </Region>
            </RegionRoot></Regions></Instrumentation></InstrumentationManifest>
            """);
        throw new InvalidOperationException("An out-of-range opcode must be rejected.");
    }
    catch (InvalidDataException)
    {
    }

    Console.WriteLine("Passed: all 12 migrated regions distinguish Start/Stop without duplicating legacy manifest regions.");
    Console.WriteLine("Passed: invalid region opcodes are rejected.");
}
finally
{
    foreach (var file in new[] { path, path + "-wal", path + "-shm", visualPath, visualPath + "-wal", visualPath + "-shm" })
    {
        File.Delete(file);
    }
}

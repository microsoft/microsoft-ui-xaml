// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using XamlTimeline;

const long element = 0x1234;
var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-EventCompatibility-{Guid.NewGuid():N}.sqlite");
var visualPath = Path.ChangeExtension(path, "visuals.sqlite");
var typePath = Path.ChangeExtension(path, "types.sqlite");
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
    const long typedParentVisual = 0x11000;
    const long typedChildVisual = 0x12000;
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
        writer.Process(15, VisualEvent("WucVisualRootSet",
            new("VisualId", typedParentVisual), new("TargetId", target), new("VisualType", "ContainerVisual")));
        writer.Process(16, VisualEvent("WucVisualChildInserted",
            new("ParentVisualId", typedParentVisual), new("ChildVisualId", typedChildVisual), new("VisualType", "SpriteVisual")));
    }

    var visuals = SessionFile.ReadObjects(visualPath, ObjectKind.Visual);
    if (visuals.Count != 5 ||
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

    if (visuals.Single(visual => visual.Address == typedParentVisual) is not { Type: "ContainerVisual", Name: null, Events: 2 } ||
        visuals.Single(visual => visual.Address == typedChildVisual) is not { Type: "SpriteVisual", Name: null, Events: 1 })
    {
        throw new InvalidOperationException("VisualType-only root and insertion events must preserve types without owner metadata.");
    }

    Console.WriteLine("Passed: VisualType-only root and insertion events preserve types without owner metadata.");

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

    const long templated = 0xc000;
    const long rendered = 0xd000;
    const long reused = 0xe000;
    const long entered = 0xf000;
    const long untyped = 0x10000;
    const long renderedThenNamed = 0x13000;
    const long heldThenNamed = 0x14000;
    using (var writer = SessionWriter.Create(typePath, new(1, "TypeRecoveryTests", null, 1_000_000, 0, SessionCoverage.Trace)))
    {
        ProfilerEvent TypeEvent(string name, params EventField[] fields) =>
            new(1, 0.001, 1, 1, XamlProvider.Xaml, true, name, 0, fields);

        writer.Process(1, TypeEvent("FrameworkElement_ApplyTemplate",
            new("ObjectPointer", templated), new("ClassName", "Microsoft.UI.Xaml.WindowChrome"), new("Name", "Chrome")));
        writer.Process(2, TypeEvent("RenderWalk_RenderElement",
            new("ElementPointer", rendered), new("ClassName", "XamlIslandRoot")));
        writer.Process(3, TypeEvent("PeerAssociated", new EventField("ElementId", reused)));
        writer.Process(4, TypeEvent("RenderWalk_RenderElement",
            new("ElementPointer", reused), new("ClassName", "WindowChrome")));
        writer.Process(5, TypeEvent("DependencyObject_Enter",
            new("ObjectPointer", reused), new("Type", "Microsoft.UI.Xaml.Controls.Control"), new("Name", "Original")));
        writer.Process(6, TypeEvent("FrameworkElement_ApplyTemplate",
            new("ObjectPointer", reused), new("ClassName", "Microsoft.UI.Xaml.WindowChrome"), new("Name", "Ignored")));
        writer.Process(7, TypeEvent("ElementLeftTree", new("ElementId", reused), new("ParentId", 0L)));
        writer.Process(8, TypeEvent("DependencyObject_Enter",
            new("ObjectPointer", reused), new("Type", "Microsoft.UI.Xaml.Controls.Button"), new("Name", "Replacement")));
        writer.Process(9, TypeEvent("ChildAdded", new("ParentId", 0L), new("ChildId", reused)));
        writer.Process(10, TypeEvent("DependencyObject_Enter",
            new("ObjectPointer", entered), new("Type", "Microsoft.UI.Xaml.Controls.Control"), new("Name", "Entered")));
        writer.Process(11, TypeEvent("FrameworkElement_ApplyTemplate",
            new("ObjectPointer", entered), new("ClassName", "CustomControl"), new("Name", "Ignored")));
        writer.Process(12, TypeEvent("ChildAdded", new("ParentId", 0L), new("ChildId", untyped)));
        writer.Process(13, TypeEvent("RenderWalk_RenderElement",
            new("ElementPointer", renderedThenNamed), new("ClassName", "RenderType")));
        writer.Process(14, TypeEvent("FrameworkElement_ApplyTemplate",
            new("ObjectPointer", renderedThenNamed), new("ClassName", "TemplateType"), new("Name", "Recovered")));
        writer.Process(15, TypeEvent("EventManager_RaiseLoadedEvent",
            new("ObjectPointer", renderedThenNamed), new("ClassName", "LoadedType"), new("Name", "Ignored")));
        writer.Process(16, TypeEvent("DependencyObject_Enter",
            new("ObjectPointer", heldThenNamed), new("Type", "AuthoritativeType"), new("Name", "")));
        writer.Process(17, TypeEvent("FrameworkElement_ApplyTemplate",
            new("ObjectPointer", heldThenNamed), new("ClassName", "TemplateType"), new("Name", "RecoveredHeld")));
        writer.Process(18, TypeEvent("EventManager_RaiseLoadedEvent",
            new("ObjectPointer", heldThenNamed), new("ClassName", "LoadedType"), new("Name", "Ignored")));
        writer.Close("End of type recovery test");
        if (writer.Failure is { } failure)
        {
            throw new InvalidOperationException("Type recovery recording failed.", failure);
        }
    }

    var typedObjects = SessionFile.ReadObjects(typePath);
    if (typedObjects.Single(item => item.Address == templated) is not { Type: "Microsoft.UI.Xaml.WindowChrome", Name: "Chrome" } ||
        typedObjects.Single(item => item.Address == rendered).Type != "XamlIslandRoot" ||
        typedObjects.Single(item => item.Address == untyped).Type is not null)
    {
        throw new InvalidOperationException("Existing template/render metadata must fill missing types without inventing unknown types.");
    }

    var reusedObjects = typedObjects.Where(item => item.Address == reused).ToList();
    if (reusedObjects.Count != 2 ||
        reusedObjects[0] is not { Type: "Microsoft.UI.Xaml.Controls.Control", Name: "Original", Events: 5 } ||
        reusedObjects[1] is not { Type: "Microsoft.UI.Xaml.Controls.Button", Name: "Replacement", Events: 2 } ||
        typedObjects.Single(item => item.Address == entered) is not { Type: "Microsoft.UI.Xaml.Controls.Control", Name: "Entered" })
    {
        throw new InvalidOperationException("Fallback types must not split lifetimes or override enter types, and real pointer reuse must still split.");
    }

    Console.WriteLine("Passed: existing template/render metadata fills missing element types without inventing unknown types.");
    Console.WriteLine("Passed: fallback metadata preserves enter types and pointer-reuse lifetime detection.");

    if (typedObjects.Single(item => item.Address == renderedThenNamed) is not { Type: "RenderType", Name: "Recovered" } ||
        typedObjects.Single(item => item.Address == heldThenNamed) is not { Type: "AuthoritativeType", Name: "RecoveredHeld" })
    {
        throw new InvalidOperationException("Fallback metadata must fill missing tracked and held names without replacing types or existing names.");
    }

    Console.WriteLine("Passed: fallback metadata fills missing tracked and held names without replacing types or existing names.");

    var regions = new RegionsLoader().Load(Path.Combine(AppContext.BaseDirectory, "XAML.regions.complete.with.startup.xml"));
    string[] activities =
    [
        "VirtualizationMeasure", "VirtualizationCleanup", "VirtualizationAdd", "GenerateContainer",
        "MeasureChild", "PrepareContainer", "UpdateLogicalScrollData", "SetupCCC", "GenerateItems",
        "GenerateMCContainer", "PlaceElement", "GetElementCount",
        "PutRootVisual", "Layout", "Measure", "Arrange", "FireLayoutUpdated", "FireSizeChanged",
        "IndividualSizeChanged", "CancelTransitions", "ProcessLayoutForTransition",
        "XYFocusEntered", "GetBuiltInStyle", "HitTest"
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

        if (current.Start.ExactTaskName)
        {
            var bareStart = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventName = activity, Opcode = 1 };
            var bareStop = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventName = activity, Opcode = 2 };
            var otherStart = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventName = activity + "Extra/Start", Opcode = 1 };
            var otherStop = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventName = activity + "Extra/Stop", Opcode = 2 };
            if (!current.Start.Matches(bareStart) || !current.Stop.Matches(bareStop) ||
                current.Start.Matches(otherStart) || current.Stop.Matches(otherStop))
            {
                throw new InvalidOperationException($"{activity} must match its exact task with or without the opcode suffix.");
            }
        }
    }

    var layout = regions.Single(region => region.Name == "Layout" && region.Start?.Opcode == 1);
    var measure = regions.Single(region => region.Name == "Measure" && region.Start?.Opcode == 1);
    foreach (var otherTask in new[] { "FireLayoutUpdated", "ProcessLayoutForTransition", "VirtualizationMeasure", "MeasureElement" })
    {
        foreach (var opcode in new[] { 1, 2 })
        {
            var other = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventName = otherTask, Opcode = opcode };
            if (layout.Start!.Matches(other) || layout.Stop!.Matches(other) ||
                measure.Start!.Matches(other) || measure.Stop!.Matches(other))
            {
                throw new InvalidOperationException("Layout and Measure must not match other tasks containing their names.");
            }
        }
    }

    var startup = regions.Where(region => region.Name == "Start-To-Layout").ToList();
    var initialize = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventId = 31 };
    var layoutStop = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventName = "Layout/Stop", Opcode = 2 };
    var legacyLayoutStop = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventId = 42, EventName = "Layout/Stop", Opcode = 2 };
    if (startup.Count != 2 || startup.Any(region => region.Start?.Matches(initialize) != true) ||
        startup.Count(region => region.Stop?.Matches(layoutStop) == true) != 1 ||
        startup.Count(region => region.Stop?.Matches(legacyLayoutStop) == true) != 1 ||
        startup.Any(region => region.Stop?.Matches(new TelemetryEvent
        {
            ProviderGuid = XamlProviders.Xaml, EventName = "FireLayoutUpdated/Stop", Opcode = 2
        }) == true))
    {
        throw new InvalidOperationException("Start-To-Layout must stop on the exact new Layout task or legacy event 42 without duplicates.");
    }

    if (!new EventMatcher(XamlProviders.Xaml, EventName: "Layout").Matches(new TelemetryEvent
        {
            ProviderGuid = XamlProviders.Xaml, EventName = "FireLayoutUpdated"
        }))
    {
        throw new InvalidOperationException("Existing selectors must retain substring matching unless exact task matching is requested.");
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

    Console.WriteLine($"Passed: all {activities.Length} migrated regions distinguish Start/Stop without duplicating legacy manifest regions.");
    Console.WriteLine("Passed: exact task selectors avoid Layout/Measure collisions and preserve legacy substring matching.");
    Console.WriteLine("Passed: Start-To-Layout supports current and legacy stops without duplicate matches.");
    Console.WriteLine("Passed: invalid region opcodes are rejected.");
}
finally
{
    foreach (var file in new[] { path, path + "-wal", path + "-shm", visualPath, visualPath + "-wal", visualPath + "-shm",
        typePath, typePath + "-wal", typePath + "-shm" })
    {
        File.Delete(file);
    }
}

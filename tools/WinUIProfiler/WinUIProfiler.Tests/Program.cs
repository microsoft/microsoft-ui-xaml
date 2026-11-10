// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Runtime.InteropServices;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Launch;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;
using WinUIProfiler.Contracts;
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

static SnapshotResourceCapture ResourceCapture(XamlResourceSnapshot values) => new("available", 0, 10, 20, values);
static SnapshotResourceRow ResourceRow(SnapshotResourceComparison comparison, string metric, int? index = null) =>
    comparison.Rows.Single(row => row.Metric == metric && row.ResourceIndex == index);

var beforeResources = new XamlResourceSnapshot(10, 100, -1, 4, 40,
[
    new(0, 10, 0, 0),
    new(13, 5, 0, 0),
    new(17, 4, 1, 3),
    new(24, 2, 0, 2),
    new(31, 10, 3, 7)
]);
var afterResources = new XamlResourceSnapshot(13, 150, 0, 3, 30,
[
    new(0, 12, 0, 0),
    new(1, 1, 0, 0),
    new(13, 7, 0, 0),
    new(17, 5, 3, 2),
    new(24, 3, 1, 2),
    new(31, 20, 4, 16)
]);
var resourceComparison = new SnapshotResourceComparison(ResourceCapture(beforeResources), ResourceCapture(afterResources));
if (!resourceComparison.HasChanges ||
    ResourceRow(resourceComparison, "AllocCount").Delta != 3 ||
    ResourceRow(resourceComparison, "AllocSize") is not { IsBytes: true, Delta: 50 } ||
    ResourceRow(resourceComparison, "OutstandingAllocCount").Delta != -1 ||
    ResourceRow(resourceComparison, "OutstandingAllocSize") is not { IsBytes: true, Delta: -10 } ||
    ResourceRow(resourceComparison, "AllocatedMemory") is not { Before: null, After: 0, Delta: null })
{
    throw new InvalidOperationException("Allocation comparisons must retain byte units, signed deltas and unavailable versus measured-zero values.");
}
Console.WriteLine("Passed: allocator snapshot rows preserve units, signed deltas and unavailable values.");

if (ResourceRow(resourceComparison, "TotalCreated", 0) is not
        { ResourceName: "Composition.ContainerVisual", Before: 10, After: 12, Delta: 2 } ||
    ResourceRow(resourceComparison, "TotalCreated", 13) is not
        { ResourceName: "D2D.SolidColorBrush", Before: 5, After: 7, Delta: 2 } ||
    resourceComparison.Rows.Where(row => row.ResourceIndex is 0 or 13 && row.Metric != "TotalCreated")
        .Any(row => row.IsTracked || row.Before.HasValue || row.After.HasValue || row.Delta.HasValue))
{
    throw new InvalidOperationException("Named Composition and raw D2D resources must expose creations without inventing lifetime measurements.");
}
Console.WriteLine("Passed: Composition and raw D2D creation totals are named; lifetime counters remain not tracked.");

if (ResourceRow(resourceComparison, "TotalCreated", 17) is not
        { ResourceName: "WinUI.DCompSurfaceWrapper", IsTracked: true, Delta: 1 } ||
    ResourceRow(resourceComparison, "TotalDestroyed", 17) is not { IsTracked: true, Delta: 2 } ||
    ResourceRow(resourceComparison, "LiveCount", 17) is not { IsTracked: true, Before: 3, After: 2, Delta: -1 } ||
    ResourceRow(resourceComparison, "LiveCount", 24) is not
        { ResourceName: "WinUI.D2DBitmapWrapper", IsTracked: true, Before: 2, After: 2, Delta: 0 })
{
    throw new InvalidOperationException("Native WinUI wrappers must retain observed lifetime counters and signed live-count changes.");
}
Console.WriteLine("Passed: native WinUI wrappers retain named creation, destruction and live-count comparisons.");

var singleSnapshot = new SnapshotResourceComparison(null, ResourceCapture(afterResources));
if (ResourceRow(singleSnapshot, "TotalCreated", 0) is not { Before: null, After: 12, Delta: null } ||
    ResourceRow(resourceComparison, "TotalCreated", 1) is not { Before: null, After: 1, Delta: null } ||
    ResourceRow(resourceComparison, "TotalCreated", 31) is not { ResourceName: null, After: 20, Delta: 10 } ||
    ResourceRow(resourceComparison, "LiveCount", 31) is not { IsTracked: true, After: 16, Delta: 9 })
{
    throw new InvalidOperationException("Single snapshots and missing slots must not invent baselines; unknown slots must retain raw values.");
}
Console.WriteLine("Passed: single/missing snapshot values remain distinct from zero, and unknown graphics slots are preserved.");

var unavailableResources = new SnapshotResourceCapture("unavailable", unchecked((int)0x80004002), 10, 20, null);
foreach (var capture in new SnapshotResourceCapture?[] { null, unavailableResources,
    ResourceCapture(afterResources with { Resources = [] }) })
{
    var empty = new SnapshotResourceComparison(null, capture);
    if (empty.Rows.Any(row => row.ResourceIndex.HasValue) ||
        (capture?.Values is null && empty.Rows.Any(row => row.Before.HasValue || row.After.HasValue || row.Delta.HasValue)))
    {
        throw new InvalidOperationException("Older, unavailable and empty graphics captures must not fabricate resource measurements.");
    }
}
Console.WriteLine("Passed: older, unavailable and empty graphics captures do not fabricate measurements.");

var largeBefore = ResourceCapture(beforeResources with { Resources = [new(17, ulong.MaxValue, 0, ulong.MaxValue)] });
var largeAfter = ResourceCapture(beforeResources with { Resources = [new(17, 1, 0, 1)] });
var largeComparison = new SnapshotResourceComparison(largeBefore, largeAfter);
if (ResourceRow(largeComparison, "TotalCreated", 17).Delta != 1m - ulong.MaxValue ||
    ResourceRow(largeComparison, "LiveCount", 17).Delta != 1m - ulong.MaxValue)
{
    throw new InvalidOperationException("Graphics comparisons must preserve the full unsigned 64-bit range without overflow.");
}
Console.WriteLine("Passed: graphics deltas retain the full unsigned 64-bit range.");

var allKinds = ResourceCapture(beforeResources with
{
    Resources = Enumerable.Range(0, 25).Select(index => new GraphicsResourceSnapshot(index, 10, 2, 8)).ToArray()
});
var groupedResources = new SnapshotResourceComparison(null, allKinds);
var expectedGroups = new[]
{
    SnapshotResourceGroupKind.Allocations, SnapshotResourceGroupKind.Composition,
    SnapshotResourceGroupKind.D2D, SnapshotResourceGroupKind.Wrappers
};
if (!groupedResources.Groups.Select(group => group.Kind).SequenceEqual(expectedGroups) ||
    !groupedResources.Groups.Select(group => group.Entries.Count).SequenceEqual(new[] { 5, 13, 4, 8 }) ||
    !groupedResources.Groups[1].Entries.Select(entry => entry.ResourceIndex).SequenceEqual(
        Enumerable.Range(0, 13).Select(index => (int?)index)) ||
    !groupedResources.Groups[2].Entries.Select(entry => entry.ResourceIndex).SequenceEqual(
        Enumerable.Range(13, 4).Select(index => (int?)index)) ||
    !groupedResources.Groups[3].Entries.Select(entry => entry.ResourceIndex).SequenceEqual(
        Enumerable.Range(17, 8).Select(index => (int?)index)) ||
    groupedResources.Groups.Take(3).SelectMany(group => group.Entries).Any(entry => entry.Metrics.Count != 1) ||
    groupedResources.Groups[3].Entries.Any(entry => !entry.Metrics.Select(metric => metric.Metric)
        .SequenceEqual(new[] { "TotalCreated", "TotalDestroyed", "LiveCount" })))
{
    throw new InvalidOperationException("The four groups must contain each allocator metric and known graphics type exactly once, with only supported counters.");
}
Console.WriteLine("Passed: four ordered groups contain one row per metric/resource and omit unsupported lifetime columns.");

var groupedComposition = resourceComparison.Groups.Single(group => group.Kind == SnapshotResourceGroupKind.Composition);
var groupedWrapper = resourceComparison.Groups.Single(group => group.Kind == SnapshotResourceGroupKind.Wrappers)
    .Entries.Single(entry => entry.ResourceIndex == 17);
var unknownResource = resourceComparison.UnknownResources.Single();
if (groupedComposition.Entries.Single(entry => entry.ResourceIndex == 0).Metrics.Single().Delta != 2 ||
    groupedComposition.Entries.Single(entry => entry.ResourceIndex == 1).Metrics.Single() is not
        { Before: null, After: 1, Delta: null } ||
    !groupedWrapper.Metrics.Select(metric => metric.Delta).SequenceEqual(new decimal?[] { 1, 2, -1 }) ||
    unknownResource.ResourceIndex != 31 || unknownResource.ResourceName is not null ||
    !unknownResource.Metrics.Select(metric => metric.Delta).SequenceEqual(new decimal?[] { 10, 1, 9 }) ||
    resourceComparison.Groups.SelectMany(group => group.Entries).Any(entry => entry.ResourceIndex == 31))
{
    throw new InvalidOperationException("Grouped comparisons must preserve signed deltas and missing values, while unknown slots stay separate with their raw counters.");
}
Console.WriteLine("Passed: grouped comparisons preserve deltas/missing values and keep unknown slots unclassified.");

foreach (var capture in new SnapshotResourceCapture?[] { null, unavailableResources,
    ResourceCapture(afterResources with { Resources = [] }) })
{
    var empty = new SnapshotResourceComparison(null, capture);
    if (!empty.Groups.Select(group => group.Kind).SequenceEqual(expectedGroups) ||
        empty.Groups[0].Entries.Count != 5 || empty.Groups.Skip(1).Any(group => group.Entries.Count != 0) ||
        empty.UnknownResources.Count != 0)
    {
        throw new InvalidOperationException("Older, unavailable and empty snapshots must retain the four group headings without inventing graphics rows.");
    }
}
Console.WriteLine("Passed: older/unavailable/empty captures retain all four groups without fabricating graphics rows.");

// A recorded trace's session file times its events with this machine's frequency until they span a second and it derives
// the trace's, so a launch timeline that follows the file must not keep times from before, the launcher's included. A
// snapshot of the file reads the launcher's events with the info that names their launcher.
var launchPath = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-LaunchTimeline-{Guid.NewGuid():N}.sqlite");
try
{
    var traceFrequency = System.Diagnostics.Stopwatch.Frequency == 10_000_000 ? 20_000_000L : 10_000_000L;
    long Qpc(double milliseconds) => 1_000_000_000L + (long)Math.Round(milliseconds * traceFrequency / 1000);
    ProfilerEvent Frame(double milliseconds, uint number, bool start) => new(Qpc(milliseconds), milliseconds, 200, 1,
        XamlProvider.Xaml, true, "CoreServices_Frame", 0, [new("FrameNumber", number), new("IsStart", start)]);
    var contoso = new MarkerProvider("Contoso.Launch", Guid.Parse("0f9d6b4e-8c1a-4b7e-a2d3-5e6f7a8b9c0d"));
    var config = MarkerConfig.Parse("""
        {
          "version": 1,
          "providers": [{ "name": "Contoso.Launch", "guid": "0f9d6b4e-8c1a-4b7e-a2d3-5e6f7a8b9c0d" }],
          "milestones": [
          { "label": "Clicked", "provider": "Contoso.Launch", "event": "Clicked", "from": "launcher" },
          { "builtin": "ProcessCreated" },
          { "label": "Ready", "provider": "Contoso.Launch", "event": "Ready" },
          { "builtin": "FirstFrameDrawn" }
          ]
        }
        """);
    static string Describe(LaunchTimeline timeline) =>
        string.Join(", ", timeline.Milestones.Select(milestone => $"{milestone.Label} {milestone.Milliseconds:R} ms"));

    using var writer = SessionWriter.Create(launchPath, new(200, "Contoso", null, null, null, SessionCoverage.Trace, @"C:\trace.etl"));
    void Store(params ProfilerEvent[] events)
    {
        var stored = writer.Status.StoredEvents;
        foreach (var profilerEvent in events)
        {
            writer.Process(++stored, profilerEvent);
        }

        var deadline = DateTime.UtcNow.AddSeconds(30);
        while (writer.Status.StoredEvents < stored)
        {
            if (writer.Failure is not null || DateTime.UtcNow > deadline)
            {
                throw new InvalidOperationException("The session writer did not store the events.", writer.Failure);
            }

            Thread.Sleep(10);
        }
    }

    Store(Frame(0, 1, true), Frame(40, 1, false), Frame(60, 2, true),
        new(Qpc(100), 100, 200, 1, XamlProvider.Marker, true, "Ready", 0, [], contoso));
    (Guid, string)[] clicked = [(contoso.Id, "Clicked")];
    using (var before = SessionFile.OpenSnapshot(launchPath))
    {
        writer.SetProcessStart(new(Qpc(-5), "Contoso", 100, "explorer"));
        Store(new ProfilerEvent(Qpc(-50), -50, 100, 7, XamlProvider.Marker, true, "Clicked", 0, [], contoso));
        var seen = new List<ProfilerEvent>();
        var seenTo = before.ReadEvents(clicked, null, seen.Add);
        using var after = SessionFile.OpenSnapshot(launchPath);
        var stored = new List<ProfilerEvent>();
        var storedTo = after.ReadEvents(clicked, null, stored.Add);
        if (before.Info.LauncherProcessId is not null || seen.Count != 0 || seenTo != 4 ||
            after.Info.LauncherProcessId != 100 || stored is not [{ ProcessId: 100, ThreadId: 7 }] || storedTo != 5)
        {
            throw new InvalidOperationException(
                "A snapshot must read the events stored when it was opened, with the info that names their launcher.");
        }
    }

    var following = new LaunchTimelineReader(launchPath, config);
    following.Read();
    Store(Frame(1500, 2, false));
    var followed = Describe(following.Read());
    var fresh = Describe(new LaunchTimelineReader(launchPath, config).Read());
    if (followed != fresh || fresh != "Clicked 0 ms, Process created 45 ms, First frame drawn 90 ms, Ready 150 ms")
    {
        throw new InvalidOperationException(
            $"A launch timeline read before the trace's frequency was known must be timed with it once it is: followed '{followed}', afresh '{fresh}'.");
    }

    Console.WriteLine("Passed: a snapshot of a session file reads its launcher's events with the info that names it.");
    Console.WriteLine("Passed: a launch timeline that follows a recorded trace's session file is retimed once its frequency is known.");
}
finally
{
    foreach (var file in new[] { launchPath, launchPath + "-wal", launchPath + "-shm" })
    {
        File.Delete(file);
    }
}

// A launcher, process 100, whose activities A and B, on its threads 1 and 2, overlap and each launch an app, 201 or 202.
// The parent of a process's first thread is the thread that created it, so each app's launch has the events of the
// activity whose thread created its process, or that no other activity's thread did.
{
    var shellExecute = new MarkerProvider("Microsoft.Windows.ShellExecute", Guid.Parse("382b5e24-181e-417f-a8d6-2155f749e724"));
    Guid a = Guid.NewGuid(), b = Guid.NewGuid();
    Action<LaunchWatcher, int> Shell(double time, string name, Guid activity, int thread) => (watcher, _) =>
        watcher.ReadLauncherProviderEvent(new(1_000_000_000L + (long)(time * 10_000), time, 100, thread, XamlProvider.Marker,
            true, name, name.EndsWith("/Start", StringComparison.Ordinal) ? 1 : 2, [], shellExecute), activity);
    static Action<LaunchWatcher, int> Created(int id, double time) =>
        (watcher, _) => watcher.ReadCreation(id, 100, null, 1_000_000_000L + (long)(time * 10_000), time);
    static Action<LaunchWatcher, int> FirstThread(int id, int parentProcessId, int parentThreadId, double time) =>
        (watcher, _) => watcher.ReadThreadStart(id, parentProcessId, parentThreadId, time);
    static Action<LaunchWatcher, int> FirstEvent(int id, double time) => (watcher, target) =>
    {
        if (target == id)
        {
            watcher.ProcessEvent(time);
        }
    };
    static string Launch(int target, params Action<LaunchWatcher, int>[] trace)
    {
        var delivered = new List<ProfilerEvent>();
        var watcher = new LaunchWatcher(target, null, delivered.Add);
        foreach (var step in trace)
        {
            step(watcher, target);
        }

        watcher.Complete();
        return string.Join(",", delivered.OrderBy(launcherEvent => launcherEvent.RelativeMilliseconds)
            .Select(launcherEvent => $"{launcherEvent.Name}@{launcherEvent.RelativeMilliseconds:R}"));
    }

    // B's thread creates 202 during A, before or after A's thread creates 201.
    Action<LaunchWatcher, int>[] before =
    [
        Shell(0, "A/Start", a, 1), Shell(80, "B/Start", b, 2), Created(202, 90), FirstThread(202, 100, 2, 95),
        Created(201, 100), FirstThread(201, 100, 1, 104), Shell(105, "B/Stop", b, 2), Shell(110, "A/Stop", a, 1),
        FirstEvent(202, 120), FirstEvent(201, 130)
    ];
    Action<LaunchWatcher, int>[] after =
    [
        Shell(0, "A/Start", a, 1), Shell(80, "B/Start", b, 2), Created(201, 100), FirstThread(201, 100, 1, 104),
        Created(202, 105), FirstThread(202, 100, 2, 109), Shell(110, "A/Stop", a, 1), Shell(112, "B/Stop", b, 2),
        FirstEvent(201, 120), FirstEvent(202, 130)
    ];
    if (Launch(201, before) != "A/Start@0,A/Stop@110" || Launch(202, before) != "B/Start@80,B/Stop@105" ||
        Launch(201, after) != "A/Start@0,A/Stop@110" || Launch(202, after) != "B/Start@80,B/Stop@112")
    {
        throw new InvalidOperationException("Each of a launcher's overlapping launches must have only its own activity's events.");
    }

    // A service creates 201 for the launcher, so A's events count for it once the trace says B's thread created 202.
    Action<LaunchWatcher, int>[] forLauncher =
    [
        Shell(0, "A/Start", a, 1), Shell(80, "B/Start", b, 2), Created(201, 100), FirstThread(201, 999, 5, 102),
        FirstEvent(201, 104), Created(202, 108), Shell(110, "A/Stop", a, 1), Shell(112, "B/Stop", b, 2),
        FirstThread(202, 100, 2, 113)
    ];
    if (Launch(201, forLauncher) != "A/Start@0,A/Stop@110" || Launch(201, forLauncher[..^1]) != "")
    {
        throw new InvalidOperationException(
            "A launch whose process another process created must wait for the thread that created the launcher's other process.");
    }

    // Without the kernel's thread events, an activity during which the launcher created both apps counts for neither.
    Action<LaunchWatcher, int>[] unknown =
    [
        Shell(0, "A/Start", a, 1), Shell(10, "B/Start", b, 2), Created(201, 50), FirstEvent(201, 51), Created(202, 60),
        FirstEvent(202, 61), Shell(70, "B/Stop", b, 2), Shell(80, "A/Stop", a, 1)
    ];
    if (Launch(201, unknown) != "" || Launch(202, unknown) != "" ||
        Launch(201, unknown[0], unknown[2], unknown[3], unknown[7]) != "A/Start@0,A/Stop@80")
    {
        throw new InvalidOperationException(
            "Without thread events, only an activity during which the launcher created no other process must count.");
    }

    Console.WriteLine("Passed: a launcher's overlapping launches each have the events of the activity whose thread created the app.");
    Console.WriteLine("Passed: a launch whose process another process created waits for the thread that created the launcher's next app.");
    Console.WriteLine("Passed: without thread events, an activity during which the launcher created two apps counts for neither.");
}

var commandLine = ProfilerCommandLine.Parse(
[
    "--launch-exe", @"C:\Apps\App.exe", "--arguments", "--mode \"a b\"", "--private-runtime", @"C:\Private\runtime",
    "--runtime-source", @"C:\Apps", "--redirect-controls", "--launch-directory", @"C:\Private\launch", "--trust-local-binaries",
    "--connect"
]);
Expect(commandLine == new ProfilerCommandLine(TargetLaunchKind.Unpackaged, @"C:\Apps\App.exe", "--mode \"a b\"",
        @"C:\Private\runtime", @"C:\Apps", true, @"C:\Private\launch", true, true) &&
    commandLine.GetPrivateRuntime(@"C:\Profiler\NativeLauncher") == new PrivateRuntimeLaunch(@"C:\Private\runtime", @"C:\Apps", true,
        @"C:\Private\launch", @"C:\Profiler\NativeLauncher"),
    "The command line must select the app and its private WinUI DLLs.");
var packagedCommandLine = ProfilerCommandLine.Parse(["--launch-aumid", "Fabrikam.App_8wekyb3d8bbwe!App", "--connect"]);
Expect(packagedCommandLine == new ProfilerCommandLine(TargetLaunchKind.Packaged, "Fabrikam.App_8wekyb3d8bbwe!App", "", null, null,
        false, null, false, true) && packagedCommandLine.GetPrivateRuntime(@"C:\Profiler\NativeLauncher") is null &&
    ProfilerCommandLine.Parse([]) == ProfilerCommandLine.Empty,
    "A packaged app, or no app, must be read without private WinUI DLLs.");
string[] privateFolders = ["--private-runtime", @"C:\Private\runtime", "--runtime-source", @"C:\Apps", "--launch-directory", @"C:\L"];
string[][] invalidCommandLines =
[
    ["--launch-exe"],
    ["--launch"],
    ["--launch-exe", @"C:\Apps\A.exe", "--launch-exe", @"C:\Apps\B.exe"],
    ["--launch-exe", @"C:\Apps\A.exe", "--launch-aumid", "Fabrikam.App_8wekyb3d8bbwe!App"],
    ["--launch-exe", "A.exe"],
    ["--connect"],
    ["--launch-exe", @"C:\Apps\A.exe", "--private-runtime", @"C:\Private\runtime"],
    ["--launch-exe", @"C:\Apps\A.exe", "--private-runtime", "runtime", "--runtime-source", @"C:\Apps", "--launch-directory", @"C:\L"],
    ["--launch-exe", @"C:\Apps\A.exe", "--redirect-controls"],
    ["--launch-aumid", "Fabrikam.App_8wekyb3d8bbwe!App", "--arguments", "x", .. privateFolders]
];
foreach (var arguments in invalidCommandLines)
{
    ExpectThrows<ArgumentException>(() => ProfilerCommandLine.Parse(arguments), $"Must reject: {string.Join(' ', arguments)}");
}

Console.WriteLine("Passed: the command line selects the app and its private WinUI DLLs, and rejects incomplete or conflicting options.");

var runtime = new PrivateRuntimeLaunch(@"C:\Private Runtime\runtime", @"C:\Apps", true, @"C:\L", @"C:\Profiler\NativeLauncher");
const string log = @"C:\L\1\redirect.jsonl";
const string result = @"C:\L\1\launch.json";
if (OperatingSystem.IsWindows())
{
    string[] words = ["", "plain", "has space", @"C:\Program Files\App\", "say \"hi\"", @"back\\""quote", "tab\there", @"\\server\share\",
        "--mode \"a b\""];
    Expect(SplitCommandLine(string.Join(' ', words.Select(NativeLaunchProtocol.Quote))).SequenceEqual(words),
        "Launcher arguments must reach the launcher unchanged.");
    Expect(SplitCommandLine(NativeLaunchProtocol.BuildArguments(@"C:\Apps\App.exe", runtime, "--mode \"a b\"", log, result, null, null))
        .SequenceEqual(["--exe", @"C:\Apps\App.exe", "--source", @"C:\Apps", "--private", @"C:\Private Runtime\runtime", "--controls", "1",
            "--log", log, "--result", result, "--arguments", "--mode \"a b\""]),
        "An unpackaged launch must pass the app's arguments.");
    Expect(SplitCommandLine(NativeLaunchProtocol.BuildArguments(@"C:\Apps\App.exe", runtime with { RedirectControls = false }, "", log,
            result, "Fabrikam.App_8wekyb3d8bbwe!App", "Fabrikam.App_1.0.0.0_x64__8wekyb3d8bbwe"))
        .SequenceEqual(["--exe", @"C:\Apps\App.exe", "--source", @"C:\Apps", "--private", @"C:\Private Runtime\runtime", "--controls", "0",
            "--log", log, "--result", result, "--aumid", "Fabrikam.App_8wekyb3d8bbwe!App", "--package",
            "Fabrikam.App_1.0.0.0_x64__8wekyb3d8bbwe", "--expected-aumid", "Fabrikam.App_8wekyb3d8bbwe!App"]),
        "A packaged launch must name the package and the expected application.");
    Console.WriteLine("Passed: launcher arguments are quoted so that CommandLineToArgvW reads them back unchanged.");
}

ExpectThrows<ArgumentException>(() => NativeLaunchProtocol.BuildArguments(@"C:\Apps\App.exe", runtime, "x", log, result,
    "Fabrikam.App_8wekyb3d8bbwe!App", "Fabrikam.App_1.0.0.0_x64__8wekyb3d8bbwe"), "A packaged launch must not take arguments.");
ExpectThrows<ArgumentException>(() => NativeLaunchProtocol.BuildArguments(@"C:\Apps\App.exe", runtime, "", log, result,
    "Fabrikam.App_8wekyb3d8bbwe!App", null), "A packaged launch must name its package.");
Expect(NativeLaunchProtocol.ParseResult(
        """{"schemaVersion":1,"pid":1234,"creationTimeFileTime":133000000000000000,"error":0,"stage":"ready","packageFullName":"P","aumid":"F!A"}""") ==
    new NativeLaunchResult(1234, 133000000000000000, 0, "ready", "P", "F!A") &&
    NativeLaunchProtocol.ParseResult(
        """{"schemaVersion":1,"pid":0,"creationTimeFileTime":0,"error":2147942405,"stage":"package_activate"}""") ==
    new NativeLaunchResult(0, 0, 0x80070005, "package_activate", "", "") &&
    NativeLaunchProtocol.ParseOwner(
        """{"schemaVersion":1,"pid":42,"creationTimeFileTime":133000000000000000,"error":0,"stage":"launcher","packageFullName":"","aumid":""}""") ==
    (42, 133000000000000000L),
    "Launcher results must be read.");
string[] invalidResults =
[
    """{"schemaVersion":2,"pid":1,"creationTimeFileTime":1,"error":0,"stage":"ready"}""",
    """{"schemaVersion":1,"pid":0,"creationTimeFileTime":1,"error":0,"stage":"ready"}""",
    """{"schemaVersion":1,"pid":1,"creationTimeFileTime":1,"error":0,"stage":"helper_ready"}""",
    """{"schemaVersion":1,"pid":-1,"creationTimeFileTime":0,"error":5,"stage":"configuration"}""",
    """{"schemaVersion":1,"pid":"1","creationTimeFileTime":1,"error":0,"stage":"ready"}""",
    """{"schemaVersion":1,"pid":1,"creationTimeFileTime":0,"error":5,"stage":""}""",
    // A launched app needs its creation time, since a later process can reuse its ID.
    """{"schemaVersion":1,"pid":1,"error":0,"stage":"ready"}""",
    """{"schemaVersion":1,"pid":1,"creationTimeFileTime":0,"error":0,"stage":"ready"}""",
    """{"schemaVersion":1,"pid":1,"creationTimeFileTime":"1","error":0,"stage":"ready"}""",
    """{"schemaVersion":1,"pid":0,"creationTimeFileTime":-1,"error":5,"stage":"configuration"}""",
    "[]"
];
foreach (var json in invalidResults)
{
    ExpectThrows<InvalidDataException>(() => NativeLaunchProtocol.ParseResult(json), $"Must reject the result {json}");
}

string[] invalidOwners =
[
    """{"schemaVersion":1,"pid":42,"creationTimeFileTime":1,"error":0,"stage":"ready"}""",
    """{"schemaVersion":1,"pid":42,"error":0,"stage":"launcher"}""",
    """{"schemaVersion":1,"pid":42,"creationTimeFileTime":0,"error":0,"stage":"launcher"}"""
];
foreach (var json in invalidOwners)
{
    ExpectThrows<InvalidDataException>(() => NativeLaunchProtocol.ParseOwner(json), $"Must reject the owner file {json}");
}

Console.WriteLine("Passed: launcher results are read, and invalid or inconsistent results are rejected.");

Expect(WinUIProfiler.Tests.Mcp.McpTestRunner.Run(Console.Out) == 0, "The MCP server's tests must pass.");
Console.WriteLine("Passed: the MCP server's tools, plugin manifests and skills.");

static void Expect(bool condition, string message)
{
    if (!condition)
    {
        throw new InvalidOperationException(message);
    }
}

static void ExpectThrows<T>(Action action, string message) where T : Exception
{
    try
    {
        action();
    }
    catch (T)
    {
        return;
    }

    throw new InvalidOperationException(message);
}

// The arguments CommandLineToArgvW, which the launcher uses, reads from a command line after the program name.
static string[] SplitCommandLine(string arguments)
{
    var argv = CommandLineToArgvW("launcher " + arguments, out var count);
    if (argv == IntPtr.Zero)
    {
        throw new System.ComponentModel.Win32Exception();
    }

    try
    {
        return [.. Enumerable.Range(1, count - 1).Select(index => Marshal.PtrToStringUni(Marshal.ReadIntPtr(argv, index * IntPtr.Size))!)];
    }
    finally
    {
        LocalFree(argv);
    }
}

[DllImport("shell32.dll", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
[DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
static extern IntPtr CommandLineToArgvW(string commandLine, out int count);

[DllImport("kernel32.dll", ExactSpelling = true)]
[DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
static extern IntPtr LocalFree(IntPtr memory);

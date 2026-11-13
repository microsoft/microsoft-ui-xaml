// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Runtime.InteropServices;
using System.Text.Json.Nodes;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Launch;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Symbols;
using WinUIProfiler.Engine.Target;
using WinUIProfiler.Contracts;
using XamlTimeline;

if (args is ["--startup-integration", var nativeDirectory])
{
    if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Startup integration requires Windows.");
    await StartupIntegration.RunAsync(Path.GetFullPath(nativeDirectory));
    return;
}

if (args is ["--record-cpu-fixture", var fixturePath])
{
    if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Recording the CPU fixture requires Windows.");
    CpuFixtureRecorder.Record(Path.GetFullPath(fixturePath));
    return;
}

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

// The default marker configuration is valid, and "repeats" is only for an app's milestone, as true or false.
{
    Expect(MarkerConfig.Default.Milestones.Any(milestone => milestone.Repeats),
        "The default marker configuration must be valid and have milestones whose event repeats.");
    foreach (var milestone in new[]
    {
        """{ "builtin": "FirstFrameDrawn", "repeats": true }""",
        """{ "label": "Clicked", "provider": "Contoso.Launch", "event": "Clicked", "from": "launcher", "repeats": false }""",
        """{ "label": "Frame", "provider": "Contoso.Launch", "event": "Frame", "repeats": "yes" }"""
    })
    {
        ExpectThrows<MarkerConfigException>(() => MarkerConfig.Parse($$"""
            {
              "version": 1,
              "providers": [{ "name": "Contoso.Launch", "guid": "0f9d6b4e-8c1a-4b7e-a2d3-5e6f7a8b9c0d" }],
              "milestones": [{{milestone}}]
            }
            """), $"Must reject the milestone {milestone}.");
    }

    Console.WriteLine("Passed: the default marker configuration is valid, and \"repeats\" is only for an app's milestone.");
}

// Attached to a running app, the first event the recording has of a type the app writes again after its launch need not
// be the launch's, so its milestone is not reached; recorded from the app's start, it is.
{
    var marker = new MarkerProvider("Contoso.Launch", Guid.Parse("0f9d6b4e-8c1a-4b7e-a2d3-5e6f7a8b9c0d"));
    var repeatsConfig = MarkerConfig.Parse("""
        {
          "version": 1,
          "providers": [{ "name": "Contoso.Launch", "guid": "0f9d6b4e-8c1a-4b7e-a2d3-5e6f7a8b9c0d" }],
          "milestones": [
          { "label": "Frame", "provider": "Contoso.Launch", "event": "Frame", "repeats": true },
          { "label": "Ready", "provider": "Contoso.Launch", "event": "Ready" }
          ]
        }
        """);
    foreach (var (coverage, reached, notReached) in new[]
    {
        (SessionCoverage.Connection, "Ready", "Frame"),
        (SessionCoverage.ProcessStart, "Frame, Ready", "")
    })
    {
        var repeatsPath = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-LaunchRepeats-{Guid.NewGuid():N}.sqlite");
        try
        {
            var frequency = System.Diagnostics.Stopwatch.Frequency;
            var startQpc = System.Diagnostics.Stopwatch.GetTimestamp();
            using (var repeatsWriter = SessionWriter.Create(repeatsPath,
                new(300, "Contoso", DateTime.UtcNow.AddSeconds(-5), frequency, startQpc, coverage)))
            {
                string[] names = ["Frame", "Ready", "Frame"];
                for (var index = 0; index < names.Length; index++)
                {
                    repeatsWriter.Process(index + 1, new(startQpc + (index + 1) * frequency / 100, (index + 1) * 10, 300, 1,
                        XamlProvider.Marker, true, names[index], 0, [], marker));
                }
            }

            var timeline = new LaunchTimelineReader(repeatsPath, repeatsConfig).Read();
            var actual = (string.Join(", ", timeline.Milestones.Select(milestone => milestone.Label)),
                string.Join(", ", timeline.NotReached.Select(definition => definition.Label)));
            Expect(actual == (reached, notReached) &&
                timeline.Notes.HasFlag(LaunchNotes.Attached) == (coverage == SessionCoverage.Connection),
                $"A {coverage} session must reach '{reached}' and not '{notReached}', not '{actual.Item1}' and '{actual.Item2}'.");
        }
        finally
        {
            foreach (var file in new[] { repeatsPath, repeatsPath + "-wal", repeatsPath + "-shm" })
            {
                File.Delete(file);
            }
        }
    }

    Console.WriteLine("Passed: attached to a running app, a milestone whose event repeats is not reached; recorded from the app's start, it is.");
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
const string wpaSymbolPath = @"srv*C:\symbols*https://msdl.microsoft.com/download/symbols";
Expect(ProfilerCommandLine.Parse(["--launch-exe", @"C:\Apps\App.exe", "--symbol-path", wpaSymbolPath]).SymbolPath == wpaSymbolPath &&
    ProfilerCommandLine.Parse(["--symbol-path", @"C:\symbols"]) == ProfilerCommandLine.Empty with { SymbolPath = @"C:\symbols" } &&
    commandLine.SymbolPath is null,
    "The symbol path must be read as given, with or without an app.");
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
    ["--launch-aumid", "Fabrikam.App_8wekyb3d8bbwe!App", "--arguments", "x", .. privateFolders],
    ["--symbol-path"],
    ["--symbol-path", ";"],
    ["--symbol-path", @"srv*https://cache.example*https://msdl.microsoft.com/download/symbols"],
    ["--symbol-path", @"C:\symbols", "--connect"]
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

if (OperatingSystem.IsWindows())
{
    // A launch's launcher is the process that created the target, which ProcessParent finds.
    using var child = System.Diagnostics.Process.Start(
        new System.Diagnostics.ProcessStartInfo(Path.Combine(Environment.SystemDirectory, "cmd.exe"))
        {
            RedirectStandardInput = true,
            CreateNoWindow = true,
            UseShellExecute = false
        })!;
    long created;
    try
    {
        using var current = System.Diagnostics.Process.GetCurrentProcess();
        created = child.StartTime.ToFileTimeUtc();
        Expect(ProcessParent.Find(child) == (Environment.ProcessId, current.ProcessName),
            "A process's parent must be the process that created it, named while it runs.");
        Expect(ProcessParent.Find(child.Id, created) == (Environment.ProcessId, current.ProcessName) &&
            ProcessParent.Find(child.Id, created + 1) is null,
            "A process instance's parent must be found only by that instance's creation time.");
        // A process that started after the child only reused its parent's ID, so it doesn't name the parent.
        Expect(ProcessParent.FindName(Environment.ProcessId, DateTime.UtcNow) == current.ProcessName &&
            ProcessParent.FindName(Environment.ProcessId, DateTime.UnixEpoch) is null,
            "A parent's ID must be named only by a process that started before the child.");
    }
    finally
    {
        child.Kill(entireProcessTree: true);
    }

    // As for a startup's exited bootstrap, which the launch's handles keep.
    child.WaitForExit();
    Expect(ProcessParent.Find(child.Id, created)?.ProcessId == Environment.ProcessId,
        "An exited process's parent must be found while a handle to it is open.");

    Console.WriteLine("Passed: a process's parent is the process that created it, and a process that reused its ID doesn't name it.");
}

const string startupResult = """
    {"schemaVersion":2,"pid":20,"creationTimeFileTime":200,"error":0,"stage":"ready","activationPid":10,
     "processes":[
       {"pid":10,"parentPid":0,"creationTimeFileTime":100,"helperReady":true,"runtimeLoaded":false,"error":0},
       {"pid":20,"parentPid":10,"creationTimeFileTime":200,"helperReady":true,"runtimeLoaded":true,"error":0}]}
    """;
var nativeStartup = NativeLaunchProtocol.ParseResult(startupResult);
Expect(nativeStartup.ProcessId == 20 && nativeStartup.ActivationProcessId == 10 && nativeStartup.Processes.Count == 2 &&
    nativeStartup.Processes[1].ParentProcessId == 10 && !nativeStartup.Processes[0].RuntimeLoaded &&
    NativeLaunchProtocol.LogFileFor(nativeStartup) == "redirect.20.jsonl" &&
    NativeLaunchProtocol.LogFileFor(nativeStartup, 10) == "redirect.jsonl",
    "A handoff must retain its bootstrap and select the child's identity and redirect log.");
Action<JsonNode>[] invalidStartupResults =
[
    node => node["processes"]![1]!["pid"] = 10,
    node => node["processes"]![1]!["parentPid"] = 99,
    node => node["processes"]![0]!["parentPid"] = 20,
    node => node["processes"]![1]!["parentPid"] = 0,
    node => node["processes"]![1]!["creationTimeFileTime"] = 0,
    node => node["processes"]![1]!["creationTimeFileTime"] = 99,
    node => node["processes"]![1]!["creationTimeFileTime"] = long.MaxValue,
    node => node["processes"]![1]!["helperReady"] = false,
    node => node["processes"]![1]!["runtimeLoaded"] = false,
    node => node["processes"]![0]!["error"] = 5,
    node => node["creationTimeFileTime"] = 201,
    node => node["pid"] = 99,
    node => node["activationPid"] = 99,
    node => node["processes"] = new JsonArray(),
    node => node["processes"] = new JsonArray(Enumerable.Range(0, 9)
        .Select(_ => node["processes"]![0]!.DeepClone()).ToArray())
];
foreach (var mutate in invalidStartupResults)
{
    var node = JsonNode.Parse(startupResult)!;
    mutate(node);
    ExpectThrows<InvalidDataException>(() => NativeLaunchProtocol.ParseResult(node.ToJsonString()),
        "Malformed ancestry, reused identities, unready selections and oversized inventories must fail.");
}
Expect(NativeLaunchProtocol.ParseResult(startupResult.Replace("\"error\":0,\"stage\":\"ready\"",
    "\"error\":234,\"stage\":\"runtime_ambiguous\"")).Processes.Count == 2,
    "A failed launch must retain its process inventory for exact-identity cleanup.");
Console.WriteLine("Passed: startup ancestry, selected runtime identity, bounded inventories and child logs are validated.");

IReadOnlyDictionary<int, long?> startupIdentities = new Dictionary<int, long?> { [10] = 100, [20] = 200 };
Expect(XamlEventSubscription.MatchesProcess(startupIdentities, 10, 100) &&
    XamlEventSubscription.MatchesProcess(startupIdentities, 20, 201) &&
    !XamlEventSubscription.MatchesProcess(startupIdentities, 20, 199) &&
    !XamlEventSubscription.MatchesProcess(startupIdentities, 30, 201),
    "Capture must accept each exact startup lifetime and reject earlier or unrelated same-image events.");

var groupDirectory = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-ProcessIsolation-{Guid.NewGuid():N}");
var groupPaths = new List<string>();
try
{
    SessionStart[] sources =
    [
        new(10, "StartupTests", null, 1_000_000, 0, SessionCoverage.ProcessStart, LauncherProcessId: 99),
        new(20, "StartupTests", null, 1_000_000, 0, SessionCoverage.ProcessStart, LauncherProcessId: 99),
        new(40, "StartupTests", null, 1_000_000, 0, SessionCoverage.ProcessStart, LauncherProcessId: 99)
    ];
    using var group = new ProcessRecordingGroup(sources, 20, 99, groupDirectory);
    groupPaths.AddRange(group.Processes.Select(item => item.Recording!.Path));
    ProfilerEvent ProcessEvent(int pid, string name) =>
        new(1000, 1, pid, 7, XamlProvider.Xaml, true, "FrameworkElement_ApplyTemplate", 0,
        [
            new("ObjectPointer", element), new("Name", name), new("ClassName", name + "Control")
        ]);
    group.Post(ProcessEvent(10, "Parent"));
    group.Post(ProcessEvent(20, "Child"));
    group.Post(ProcessEvent(30, "Unrelated"));
    group.Post(new(500, 0.5, 99, 7, XamlProvider.Marker, true, "Launch", 0, [], new("Launcher", Guid.NewGuid())));
    group.Close("Test complete", 2);

    Expect(group.Primary.Source.ProcessId == 20 && group.Primary.XamlEvents == 1 &&
        group.Primary.Recording!.StoredEvents == 2 && group.RecordingError is null,
        "The UI child must remain primary; external launcher markers must not become target XAML.");
    foreach (var item in group.Processes)
    {
        var saved = item.Recording!.Path;
        var info = SessionFile.ReadInfo(saved);
        Expect(info.ProcessId == item.Source.ProcessId && info.LostByEtw == 2 && info.DroppedByProfiler == 0,
            "Each file must preserve its own process scope and capture health.");
        if (info.ProcessId == 40)
        {
            Expect(info.StoredEvents == 0 && item.XamlEvents == 0 && SessionFile.ReadObjects(saved).Count == 0,
                "A process without telemetry must not inherit UI events or fabricated objects.");
            continue;
        }
        var objects = SessionFile.ReadObjects(saved);
        var expected = info.ProcessId == 10 ? "Parent" : "Child";
        Expect(objects.Count == 1 && objects[0].Address == element && objects[0].Name == expected,
            "Equal process-local addresses must remain distinct in separate recordings.");
        var events = new List<ProfilerEvent>();
        SessionFile.Read(saved, events.Add);
        Expect(events.All(item => item.ProcessId == info.ProcessId ||
            info.ProcessId == 20 && item.ProcessId == 99 && item.Provider == XamlProvider.Marker),
            "No startup member or unrelated same-image process may be relabeled as another process's event.");
    }
}
finally
{
    foreach (var saved in groupPaths)
    {
        foreach (var file in new[] { saved, saved + "-wal", saved + "-shm" }) File.Delete(file);
    }
    if (Directory.Exists(groupDirectory)) Directory.Delete(groupDirectory);
}
Console.WriteLine("Passed: parent/child recordings, equal addresses, launcher markers and absent telemetry stay process-isolated.");

var recordedDirectory = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-Recorded-{Guid.NewGuid():N}");
var recordedPaths = new List<string>();
try
{
    var created = new DateTime(2026, 1, 1, 0, 0, 0, DateTimeKind.Utc);
    SessionStart[] recordedSources =
    [
        new(10, "RecordedTests", created, 1_000_000, 0, SessionCoverage.ProcessStart),
        new(20, "RecordedTests", null, 1_000_000, 0, SessionCoverage.ProcessStart)
    ];
    using var recorded = new ProcessRecordingGroup(recordedSources, 20, null, recordedDirectory);
    recordedPaths.AddRange(recorded.Processes.Select(item => item.Recording!.Path));
    Expect(!recorded.IsRecorded(10, created.ToFileTimeUtc() - 1) && recorded.IsRecorded(10, created.ToFileTimeUtc()) &&
        recorded.IsRecorded(20, 0) && !recorded.IsRecorded(30, created.ToFileTimeUtc()),
        "A target's samples must be recorded from its creation on, not an earlier process's that had its ID.");
    Expect(recorded.RecordingPaths.SequenceEqual(
        [recorded.Primary.Recording!.Path, recorded.Processes.Single(item => !item.IsPrimary).Recording!.Path]),
        "Every session file of the group must be listed, the primary process's first.");
}
finally
{
    foreach (var saved in recordedPaths)
    {
        foreach (var file in new[] { saved, saved + "-wal", saved + "-shm" }) File.Delete(file);
    }
    if (Directory.Exists(recordedDirectory)) Directory.Delete(recordedDirectory);
}
Console.WriteLine("Passed: a process's samples are recorded from its creation on, and the group lists its files, the primary's first.");

Expect(WinUIProfiler.Tests.Mcp.McpTestRunner.Run(Console.Out) == 0, "The MCP server's tests must pass.");
Console.WriteLine("Passed: the MCP server's tools, plugin manifests and skills.");

{
    const string testCache = @"C:\cache";
    const string msdl = "https://msdl.microsoft.com/download/symbols";
    Expect(SymbolPaths.Normalize(@"C:\syms", testCache) == @"C:\syms;srv*C:\syms*C:\syms",
        "A directory must be searched as it is and as a symbol store.");
    Expect(SymbolPaths.Normalize(msdl + "/", testCache) == $@"srv*C:\cache*{msdl}",
        "A bare symbol server URL must download into the default cache.");
    Expect(SymbolPaths.Normalize($@"srv*C:\c*{msdl}", testCache) == $@"srv*C:\c*{msdl}",
        "A WPA srv*cache*server element must be kept as it is.");
    Expect(SymbolPaths.Normalize($@"srv*C:\c*\\server\share*{msdl}", testCache) ==
        $@"srv*C:\c*\\server\share;srv*C:\c*{msdl}",
        "Each server of a srv element must be searched in order, with the element's cache.");
    Expect(SymbolPaths.Normalize($@"cache*C:\c;srv*{msdl}", testCache) == $@"srv*C:\c*C:\c;srv*C:\c*{msdl}",
        "cache*dir must be searched, and must hold what servers without their own cache download.");
    Expect(SymbolPaths.Normalize("http://symweb", testCache) == @"srv*C:\cache*https://symweb.azurefd.net",
        "http://symweb must become the symweb server that an Azure sign-in reaches.");
    Environment.SetEnvironmentVariable("WINUIPROFILER_TEST_SYMBOLS", @"C:\syms");
    Expect(SymbolPaths.Normalize(@" ""%WINUIPROFILER_TEST_SYMBOLS%\"" ; C:\syms ;", testCache) == @"C:\syms;srv*C:\syms*C:\syms",
        "Quotes, environment variables, trailing separators and repeated locations must be handled.");
    foreach (var malformed in new[] { "", ";", "a*b", "srv*", $"srv*https://x.example*{msdl}" })
    {
        ExpectThrows<ArgumentException>(() => SymbolPaths.Normalize(malformed, testCache),
            $"The symbol path '{malformed}' must be rejected.");
    }

    Expect(SymbolPaths.Redact("https://user:pw@host/p?sig=a#f") == "https://host/p?<redacted>" &&
        SymbolPaths.Redact($@"srv*C:\c*https://u@h/x?t=1;C:\syms;srv*{msdl}") == $@"srv*C:\c*https://h/x?<redacted>;C:\syms;srv*{msdl}" &&
        SymbolPaths.Redact("https://h/x?") == "https://h/x",
        "A symbol path that is stored or shown must hide its URLs' user info, query and fragment, and keep the rest.");
}
Console.WriteLine("Passed: symbol paths keep only the locations given, in WPA's forms, and hide their URLs' credentials.");

Expect(StackSampleCollector.NextInterval(null, collection: true, sampled: false, 10_000, 0) == 1.0 &&
    StackSampleCollector.NextInterval(2.0, collection: true, sampled: false, 10_000, 0) == 2.0 &&
    StackSampleCollector.NextInterval(1.0, collection: false, sampled: false, 5_000, 10_000) == 0.5 &&
    StackSampleCollector.NextInterval(0.5, collection: false, sampled: true, 10_000, 5_000) == 0.5 &&
    StackSampleCollector.NextInterval(null, collection: false, sampled: true, 10_000, 5_000) == 0.5 &&
    StackSampleCollector.NextInterval(null, collection: false, sampled: false, 0, 0) is null,
    "The sampling interval must be the one in effect while the trace sampled, not the reset when a recording stops.");
Expect(TraceReplay.FlushesPendingSamples(halted: false, stopRequested: false, failed: false) &&
    TraceReplay.FlushesPendingSamples(halted: false, stopRequested: false, failed: true) &&
    TraceReplay.FlushesPendingSamples(halted: true, stopRequested: true, failed: false) &&
    !TraceReplay.FlushesPendingSamples(halted: true, stopRequested: false, failed: false) &&
    !TraceReplay.FlushesPendingSamples(halted: true, stopRequested: true, failed: true),
    "Samples awaiting their stacks must be stored at the trace's end, even a damaged one, and on Stop, but not after a failure.");
Console.WriteLine("Passed: the sampling interval is the one in effect while sampling, and Stop keeps the samples awaiting their stacks.");

{
    var heldSampleSet = new HeldSamples(5);
    heldSampleSet.Hold(1, new(10, 0x1000, 1.0, 100));
    heldSampleSet.Hold(1, new(12, 0x1000, 3.0, 50));
    heldSampleSet.Hold(1, new(13, 0x1000, 4.0, 100));
    heldSampleSet.Hold(2, new(11, 0x1000, 2.0, 100));
    heldSampleSet.Hold(1, new(14, 0x1000, 5.0, 100));
    // The sample before the thread's start was an earlier thread's, so it's left out; the other process's isn't counted.
    var heldReleased = heldSampleSet.Release(1, 7, 2.0, (pid, fileTime) => pid == 7 && fileTime >= 100);
    Expect(heldReleased.Select(sample => sample.Qpc).SequenceEqual([13L, 14L]) &&
        heldSampleSet is { Count: 1, Unattributed: 1 } &&
        heldSampleSet.Release(1, 7, 0, (_, _) => true).Count == 0 &&
        heldSampleSet.Release(2, -1, 0, (_, _) => true).Count == 0 && heldSampleSet.Count == 1,
        "A thread's held samples must be released once, in order, from its start and for the target process only.");

    var heldCapped = new HeldSamples(2);
    heldCapped.Hold(1, new(1, 0x1000, 1.0, 100));
    heldCapped.Hold(2, new(2, 0x1000, 2.0, 100));
    heldCapped.Hold(3, new(3, 0x1000, 3.0, 100));
    var heldCappedCounts = (heldCapped.Count, heldCapped.Unattributed);
    heldCapped.Clear();
    Expect(heldCappedCounts == (2, 1L) && heldCapped is { Count: 0, Unattributed: 3 },
        "Samples past the limit, and those never given a process, must be counted as left out.");

    var heldPartNote = StackSampleCollector.TraceNote(new(1.0, false, 4, [], 3));
    var heldCompressedNote = StackSampleCollector.TraceNote(new(1.0, true, 4, [], 3));
    var heldOnlyNote = StackSampleCollector.TraceNote(new(1.0, false, 0, [], 3));
    Expect(heldPartNote is not null && heldPartNote.StartsWith("3 CPU samples were left out", StringComparison.Ordinal) &&
        StackSampleCollector.TraceNote(new(1.0, false, 4, [])) is null &&
        heldCompressedNote is not null && heldCompressedNote.Contains("compressed", StringComparison.Ordinal) &&
        heldCompressedNote.EndsWith(heldPartNote, StringComparison.Ordinal) &&
        heldOnlyNote == heldPartNote,
        "A trace's note must say how many samples were left out, and not that it has none when they all were.");
}
Console.WriteLine("Passed: samples of threads whose process isn't known are held up to a limit, and those left out are counted and noted.");

{
    const ulong kernelLeaf = 0xFFFFF800_00001000, kernelRoot = 0xFFFFF800_00002000;
    const ulong userLeaf = 0x7FF6_0000_1000, userRoot = 0x7FF6_0000_2000;
    var mergedSamples = new List<MergedSample>();
    var stackMerger = new StackSampleMerger(mergedSamples.Add);

    // Samples in kernel code wait for the user part the kernel walks once the thread returns to user mode.
    stackMerger.OnSample(100, 1, 7, kernelLeaf);
    stackMerger.OnStack(101, 100, 1, 7, [kernelLeaf, kernelRoot], 8);
    Expect(mergedSamples.Count == 0, "A sample in kernel code must wait for its user part.");
    stackMerger.OnSample(110, 1, 7, kernelLeaf);
    stackMerger.OnStack(111, 110, 1, 7, [kernelLeaf, kernelRoot], 8);
    stackMerger.OnStack(120, 120, 1, 7, [userLeaf, userRoot], 8);
    Expect(mergedSamples.Count == 2 && mergedSamples[0].Qpc == 100 && mergedSamples[1].Qpc == 110 &&
        mergedSamples.All(sample => sample is { ProcessId: 1, ThreadId: 7, Frames: [kernelLeaf, kernelRoot, userLeaf, userRoot] }),
        "Each sample waiting for its user part must get the thread's user stack after its kernel part.");

    mergedSamples.Clear();
    stackMerger.OnSample(130, 1, 7, userLeaf);
    stackMerger.OnStack(131, 130, 1, 7, [userLeaf, userRoot], 8);
    Expect(mergedSamples is [{ Qpc: 130, Frames: [userLeaf, userRoot] }], "A sample in user code must get its whole stack.");

    mergedSamples.Clear();
    stackMerger.OnSample(140, 1, 8, 0x1000);
    stackMerger.OnSample(150, 1, 8, 0x2000);
    Expect(mergedSamples is [{ Qpc: 140, ThreadId: 8, Frames: [0x1000] }],
        "A sample without a stack by its thread's next sample must keep its instruction pointer.");

    stackMerger.OnSample(160, 4, 9, kernelLeaf);
    stackMerger.OnStack(161, 160, 4, 9, [kernelLeaf, kernelRoot], 8);
    stackMerger.OnStack(162, 162, 1, 99, [userLeaf], 8);
    Expect(mergedSamples.Count == 2 && mergedSamples[1] is { Qpc: 160, ProcessId: 4, Frames: [kernelLeaf, kernelRoot] },
        "A System process sample must not wait for a user part, and an unknown thread's stack must be ignored.");

    stackMerger.OnSample(170, 1, 10, kernelLeaf);
    stackMerger.OnStack(171, 170, 1, 10, [kernelLeaf, kernelRoot], 8);
    stackMerger.OnSample(180, 2, 11, 0x3000);
    stackMerger.OnSample(190, 3, 11, 0x4000);
    Expect(mergedSamples.Count == 3 && mergedSamples[2] is { Qpc: 180, ProcessId: 2, Frames: [0x3000] },
        "A thread ID another process reuses must end the earlier thread's samples.");
    stackMerger.Flush();
    Expect(mergedSamples.Count == 6 &&
        mergedSamples.Any(sample => sample is { Qpc: 150, ThreadId: 8, Frames: [0x2000] }) &&
        mergedSamples.Any(sample => sample is { Qpc: 170, ThreadId: 10, Frames: [kernelLeaf, kernelRoot] }) &&
        mergedSamples.Any(sample => sample is { Qpc: 190, ProcessId: 3, Frames: [0x4000] }),
        "Flush must emit every incomplete sample with what it has.");
}
Console.WriteLine("Passed: sampled stacks join their kernel and user parts as TraceLog does.");

var cpuPath = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-Cpu-{Guid.NewGuid():N}.sqlite");
var cpuIdentityPath = Path.ChangeExtension(cpuPath, "identity.sqlite");
var cpuEmptyPath = Path.ChangeExtension(cpuPath, "empty.sqlite");
var cpuLivePath = Path.ChangeExtension(cpuPath, "live.sqlite");
var cpuTrimPath = Path.ChangeExtension(cpuPath, "trim.sqlite");
var cpuPlacePath = Path.ChangeExtension(cpuPath, "place.sqlite");
var cpuLatePath = Path.ChangeExtension(cpuPath, "late.sqlite");
var cpuNativePath = Path.ChangeExtension(cpuPath, "native.sqlite");
var cpuNativeImagePath = Path.ChangeExtension(cpuPath, "native-image.sqlite");
var cpuNativeStalePath = Path.ChangeExtension(cpuPath, "native-stale.sqlite");
var cpuSymbols = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-Symbols-{Guid.NewGuid():N}");
try
{
    const ulong cpuBase = 0x7FF0_0000_0000;
    SampledModule CpuModule() => new(1, @"C:\WinUIProfilerTests\Missing\Test.dll", cpuBase, 0x10000, 0);
    void WriteCpuSession(string file, SampledModule module)
    {
        using var writer = SessionWriter.Create(file, new(1, "CpuTests", null, 1_000_000, 0, SessionCoverage.Trace));
        CodeFrame At(ulong rva) => new(module, cpuBase + rva);
        writer.PostSample(new(1_000, 1, 5, [At(0x1100), At(0x1000)]), wait: true);
        writer.PostSample(new(2_000, 1, 5, [At(0x1100), At(0x1000)]), wait: true);
        writer.PostSample(new(3_000, 1, 6, [At(0x1200), At(0x1300), At(0x1300), At(0x1000)]), wait: true);
        writer.PostSample(new(4_000, 1, 7, [new(null, 0xABC0), At(0x1000)]), wait: true);
        writer.SetCpuSampling(new(1.0, false, 4, [module]), null);
    }

    WriteCpuSession(cpuPath, CpuModule());
    var cpuProfile = CpuProfileReader.Read(cpuPath, top: 10);
    Expect(cpuProfile is { Samples: 4, ThreadId: null, Sampling: { IntervalMs: 1.0, Samples: 4, Dropped: 0, SymbolPath: null } },
        "The CPU sampling row and every stored sample must be read back.");
    Expect(cpuProfile!.Threads.Select(thread => (thread.ThreadId, thread.Samples)).SequenceEqual([(5, 2L), (6, 1L), (7, 1L)]),
        "Threads must be listed by their samples, most first.");
    Expect(cpuProfile.Exclusive.Select(frame => (frame.Frame, frame.Samples)).SequenceEqual(
        [("Test.dll!0x1100", 2L), ("?!0xABC0", 1L), ("Test.dll!0x1200", 1L)]) && cpuProfile.Exclusive[0].Ms == 2.0,
        "Exclusive samples must go to each stack's leaf frame, most first, named by RVA until symbolized.");
    Expect(cpuProfile.Inclusive.Select(frame => (frame.Frame, frame.Samples)).SequenceEqual(
        [("Test.dll!0x1000", 4L), ("Test.dll!0x1100", 2L), ("?!0xABC0", 1L), ("Test.dll!0x1200", 1L), ("Test.dll!0x1300", 1L)]),
        "Inclusive samples must count each frame once per stack, even when it recurses.");
    Expect(cpuProfile.Modules is [{ Name: "Test.dll", Status: null, Addresses: 4 }],
        "The sampled module must be listed with its code addresses.");
    Expect(CpuProfileReader.Read(cpuPath, threadId: 6) is { ThreadId: 6, Samples: 1, Exclusive: [{ Frame: "Test.dll!0x1200" }] },
        "A thread's profile must keep only its own samples.");
    using (var cpuConnection = SessionFile.Open(cpuPath))
    {
        Expect(Sql.Scalar(cpuConnection, "SELECT Samples FROM HotFunctions WHERE ThreadId = 5 AND Frame = 'Test.dll!0x1100'") == 2 &&
            Sql.Scalar(cpuConnection, "SELECT COUNT(*) FROM CpuSampleDetails") == 4,
            "The CPU views must show each sample's frame.");
    }

    Expect(cpuProfile.Threads.All(thread => !thread.UiThread), "A thread without frames must not be a UI thread.");
    using (var cpuConnection = new Microsoft.Data.Sqlite.SqliteConnection($"Data Source={cpuPath};Pooling=False"))
    {
        cpuConnection.Open();
        using var tick = cpuConnection.CreateCommand();
        tick.CommandText = "INSERT INTO Frames (FrameId, ThreadId, FrameNumber, StartQpc, EndQpc) VALUES (1, 6, 1, 2500, 3500)";
        tick.ExecuteNonQuery();
    }

    Expect(CpuProfileReader.Read(cpuPath)?.Threads.Select(thread => (thread.ThreadId, thread.UiThread)).SequenceEqual(
        [(5, false), (6, true), (7, false)]) == true,
        "The thread XAML ticked frames on must be the UI thread.");

    // The same samples as call trees: from the stacks' root frames down, and up to or down from one function.
    (string Frame, long Samples, long? Self) TreeNode(CpuCallTreeNode node) => (node.Frame, node.Samples, node.Self);
    var cpuStacks = CpuProfileReader.ReadStacks(cpuPath);
    var topDown = cpuStacks?.TopDown();
    Expect(cpuStacks is { Samples: 4, ThreadId: null, Threads.Count: 3 } && cpuStacks.Ms(2) == 2.0 &&
        topDown is { FrameIndex: -1, Samples: 4, Self: 0, HasChildren: true, Children: [{ Frame: "Test.dll!0x1000", Samples: 4, Self: 0 }] },
        "The call tree's root must have every sample, by the stacks' root frames.");
    var rootFrame = topDown!.Children[0];
    Expect(rootFrame.Children.Select(TreeNode).SequenceEqual(
        [("Test.dll!0x1100", 2L, (long?)2), ("?!0xABC0", 1L, (long?)1), ("Test.dll!0x1300", 1L, (long?)0)]),
        "A node's children must be the functions its stacks called next, most samples first, each with the samples it ran.");
    Expect(rootFrame.Children[2].Children is
        [{ Frame: "Test.dll!0x1300", Self: 0, Children: [{ Frame: "Test.dll!0x1200", Samples: 1, Self: 1, HasChildren: false }] }],
        "A recursive call must nest under its caller, down to the frame that ran.");
    var functions = cpuStacks!.Functions();
    Expect(functions.Select(function => (function.Frame, function.Self, function.Total)).SequenceEqual(
        [("Test.dll!0x1100", 2L, 2L), ("?!0xABC0", 1L, 1L), ("Test.dll!0x1200", 1L, 1L), ("Test.dll!0x1000", 0L, 4L), ("Test.dll!0x1300", 0L, 1L)]) &&
        functions.All(function => !function.Named),
        "Functions must be listed by the samples they ran, then by the samples they were on the stack, once per sample.");
    int FrameOf(string frame) => functions.Single(function => function.Frame == frame).FrameIndex;
    Expect(cpuStacks.Callers(FrameOf("Test.dll!0x1100")) is
        { Frame: "Test.dll!0x1100", Samples: 2, Self: 2, Children: [{ Frame: "Test.dll!0x1000", Samples: 2, Self: null, HasChildren: false }] },
        "A function's callers must have the samples it was on the stack, up to the stacks' root frames.");
    Expect(cpuStacks.Callers(FrameOf("Test.dll!0x1300")) is { Samples: 1, Self: 0, Children: [{ Frame: "Test.dll!0x1000", Samples: 1 }] },
        "A recursive function's callers must be those of its outermost call.");
    var callees = cpuStacks.Callees(FrameOf("Test.dll!0x1000"));
    Expect(callees is { Samples: 4, Self: 0 } && callees.Children.Select(TreeNode).SequenceEqual(rootFrame.Children.Select(TreeNode)),
        "A function's callees must be the functions it called, down to the frames that ran.");
    ExpectThrows<ArgumentOutOfRangeException>(() => cpuStacks.Callers(topDown.FrameIndex), "The root of all samples must not be taken for a function.");
    Expect(CpuProfileReader.ReadStacks(cpuPath, threadId: 6) is { ThreadId: 6, Samples: 1, Threads.Count: 3 } threadStacks &&
        threadStacks.TopDown().Children is [{ Frame: "Test.dll!0x1000", Samples: 1 }],
        "A thread's call tree must keep only its own samples.");

    // A frame's code address finds it in a later read, where naming its function changed its text and index.
    var recursiveAddress = cpuStacks.AddressOf(FrameOf("Test.dll!0x1300"));
    var renamedStacks = CpuProfileReader.ReadStacks(cpuPath, functionName: address => address == recursiveAddress ? "Recurse" : null);
    Expect(functions.All(function => cpuStacks.AddressOf(function.FrameIndex) is { } address &&
            cpuStacks.FrameOfAddress(address) == function.FrameIndex) &&
        cpuStacks.AddressOf(topDown.FrameIndex) is null && cpuStacks.FrameOfAddress(long.MaxValue) is null &&
        recursiveAddress is { } recursive && renamedStacks?.FrameOfAddress(recursive) is { } renamedFrame &&
        renamedStacks.Functions().Single(function => function.FrameIndex == renamedFrame) is
            { Frame: "Test.dll!Recurse", Named: true, Self: 0, Total: 1 },
        "A frame's code address must find the frame again in a later read, once its function is named.");

    // The module's binary is gone and the trace didn't name its PDB: nothing can be searched for.
    Directory.CreateDirectory(cpuSymbols);
    var symbolized = SessionSymbolizer.Symbolize(cpuPath, cpuSymbols);
    Expect(symbolized is { Modules: 1, NoIdentity: 1, Resolved: 0, Failed: 0, Named: 0 } &&
        CpuProfileReader.Read(cpuPath) is { Modules: [{ Status: "NoIdentity" }], Sampling: { SymbolPath: not null, SymbolizedUtc: not null } },
        "A module without a PDB identity must be reported, not fail symbolizing.");

    // A symbol path is stored and reported without what its URLs may carry as credentials, such as a SAS token.
    var secretSymbols = $"srv*{cpuSymbols}*https://user:secret@127.0.0.1:9/symbols?sig=token";
    Expect(SessionSymbolizer.Symbolize(cpuPath, secretSymbols).SymbolPath is var reported &&
        CpuProfileReader.ReadSampling(cpuPath)?.SymbolPath is { } stored &&
        new[] { reported, stored }.All(shown => shown == $"srv*{cpuSymbols}*https://127.0.0.1:9/symbols?<redacted>"),
        "Symbolizing must store and report the symbol path without its URLs' credentials.");

    // The trace named a PDB that isn't on the symbol path, and only the symbol path is searched.
    var identified = CpuModule();
    identified.PdbName = "Test.pdb";
    identified.PdbGuid = Guid.Parse("0f6b9c1e-8d3a-4a5b-9c7d-2e1f3a4b5c6d");
    identified.PdbAge = 1;
    WriteCpuSession(cpuIdentityPath, identified);
    Expect(SessionSymbolizer.Symbolize(cpuIdentityPath, cpuSymbols) is { Modules: 1, NotFound: 1, Failed: 0, Named: 0 } &&
        CpuProfileReader.Read(cpuIdentityPath) is { Modules: [{ Status: "NotFound" }] },
        "A PDB that isn't on the symbol path must be reported as not found.");

    using (var canceled = new CancellationTokenSource())
    {
        canceled.Cancel();
        ExpectThrows<OperationCanceledException>(
            () => SessionSymbolizer.Symbolize(cpuIdentityPath, cpuSymbols, force: true, cancellationToken: canceled.Token),
            "A canceled symbolizing must stop.");
    }

    Expect(!File.Exists(cpuIdentityPath + "-journal") && CpuProfileReader.Read(cpuIdentityPath) is { Modules: [{ Status: "NotFound" }] },
        "A canceled symbolizing must leave the file as it was, readable.");

    // A native DLL built with its PDB (Fixtures\NativeSymbols), so naming reads a real PDB through DIA, from a local
    // directory without a network. Its stacks are FixtureBeta called by FixtureAlpha, at the RVAs its exports give, so the
    // PDB must name the functions the image says are there.
    static (int Size, int TimeDateStamp, Dictionary<string, uint> Exports) ReadNativeImage(string file)
    {
        using var pe = new System.Reflection.PortableExecutable.PEReader(File.OpenRead(file));
        System.Reflection.Metadata.BlobReader At(int rva) => pe.GetSectionData(rva).GetReader();
        var headers = pe.PEHeaders;
        var peHeader = headers.PEHeader ?? throw new InvalidDataException($"{file} has no PE header.");
        var directory = At(peHeader.ExportTableDirectory.RelativeVirtualAddress);
        directory.Offset = 24; // IMAGE_EXPORT_DIRECTORY's NumberOfNames, AddressOfFunctions, AddressOfNames and AddressOfNameOrdinals
        var (count, functions, names, ordinals) = (directory.ReadInt32(), directory.ReadInt32(), directory.ReadInt32(), directory.ReadInt32());
        var exports = new Dictionary<string, uint>(StringComparer.Ordinal);
        for (var index = 0; index < count; index++)
        {
            var name = At(At(names + 4 * index).ReadInt32());
            exports[name.ReadUTF8(name.IndexOf(0))] = At(functions + 4 * At(ordinals + 2 * index).ReadUInt16()).ReadUInt32();
        }

        return (peHeader.SizeOfImage, headers.CoffHeader.TimeDateStamp, exports);
    }

    var nativeDll = Path.Combine(AppContext.BaseDirectory, "Fixtures", "NativeSymbols", "NativeSymbols.dll");
    var nativeImage = ReadNativeImage(nativeDll);
    var nativeIdentity = PeIdentity.Read(nativeDll) ?? throw new InvalidOperationException("The native fixture must name its PDB.");
    Expect(nativeImage.Exports.ContainsKey("FixtureAlpha") && nativeImage.Exports.ContainsKey("FixtureBeta") &&
        !nativeIdentity.Portable && Path.GetFileName(nativeIdentity.PdbName) == "NativeSymbols.pdb",
        "The native fixture must export its functions and name its PDB.");
    if (OperatingSystem.IsWindows())
    {
        // build.cmd builds them outside the enlistment, so they name neither it nor the user who built them.
        foreach (var file in new[] { nativeDll, Path.ChangeExtension(nativeDll, ".pdb") })
        {
            var found = CpuFixtureRecorder.FindPrivate(File.ReadAllBytes(file));
            Expect(found is null, $"{Path.GetFileName(file)} must not contain {found}: build it again with build.cmd.");
        }
    }

    // The symbol path holds only the PDB, away from the image.
    var nativeSymbols = Directory.CreateDirectory(Path.Combine(cpuSymbols, "Native")).FullName;
    File.Copy(Path.ChangeExtension(nativeDll, ".pdb"), Path.Combine(nativeSymbols, "NativeSymbols.pdb"));
    const ulong nativeBase = 0x7FF1_0000_0000;
    void WriteNativeSession(string file, bool recordedIdentity, int? timeDateStamp = null)
    {
        var module = new SampledModule(1, nativeDll, nativeBase, nativeImage.Size, timeDateStamp ?? nativeImage.TimeDateStamp);
        if (recordedIdentity)
        {
            // As a merged trace records it, from the image's DbgID_RSDS event.
            module.PdbName = nativeIdentity.PdbName;
            module.PdbGuid = nativeIdentity.Guid;
            module.PdbAge = nativeIdentity.Age;
        }

        using var writer = SessionWriter.Create(file, new(1, "CpuTests", null, 1_000_000, 0, SessionCoverage.Trace));
        CodeFrame In(string function, uint offset) => new(module, nativeBase + nativeImage.Exports[function] + offset);
        for (var sample = 1; sample <= 3; sample++)
        {
            writer.PostSample(new(sample * 1_000, 1, 5, [In("FixtureBeta", 0x10), In("FixtureAlpha", 9)]), wait: true);
        }

        writer.SetCpuSampling(new(1.0, false, 3, [module]), null);
    }

    const string alphaFrame = "NativeSymbols.dll!FixtureAlpha";
    const string betaFrame = "NativeSymbols.dll!FixtureBeta";
    WriteNativeSession(cpuNativePath, recordedIdentity: true);
    using (var nativeNamer = new LiveSymbolNamer(cpuNativePath, nativeSymbols))
    {
        _ = CpuProfileReader.ReadStacks(cpuNativePath, functionName: nativeNamer.Request);
        Expect(nativeNamer.Name() == 2 &&
            CpuProfileReader.ReadStacks(cpuNativePath, functionName: nativeNamer.NameOf)?.TopDown().Children is
                [{ Frame: alphaFrame, Samples: 3, Self: 0, Children: [{ Frame: betaFrame, Samples: 3, Self: 3 }] }] &&
            CpuProfileReader.Read(cpuNativePath) is { Modules: [{ Status: null }], Exclusive: [{ Frame: var unnamedLeaf }] } &&
            unnamedLeaf.StartsWith("NativeSymbols.dll!0x", StringComparison.Ordinal),
            "A live namer must name native functions from their PDB, in memory only.");
    }

    Expect(SessionSymbolizer.Symbolize(cpuNativePath, nativeSymbols) is { Modules: 1, Resolved: 1, Failed: 0, Named: 2, Addresses: 2 },
        "Symbolizing must name a module's functions from its PDB on the symbol path.");
    using (var nativeConnection = SessionFile.Open(cpuNativePath))
    {
        Expect(Sql.Scalar(nativeConnection, "SELECT COUNT(*) FROM CodeAddresses WHERE Function IN ('FixtureAlpha', 'FixtureBeta')") == 2,
            "Symbolizing must store each address's function name in the session file.");
    }

    var nativeProfile = CpuProfileReader.Read(cpuNativePath);
    var nativeStacks = CpuProfileReader.ReadStacks(cpuNativePath);
    Expect(nativeProfile is
        {
            Exclusive: [{ Frame: betaFrame, Samples: 3 }],
            Modules: [{ Name: "NativeSymbols.dll", Status: "Resolved", SymbolFile: { } nativeSymbolFile, Addresses: 2 } nativeModule],
        } &&
        string.Equals(Path.GetFullPath(nativeSymbolFile), Path.Combine(nativeSymbols, "NativeSymbols.pdb"), StringComparison.OrdinalIgnoreCase) &&
        nativeModule.Pdb == nativeIdentity.Key &&
        nativeProfile.Inclusive.Select(frame => (frame.Frame, frame.Samples)).SequenceEqual([(alphaFrame, 3L), (betaFrame, 3L)]) &&
        nativeStacks?.Functions() is { Count: 2 } nativeFunctions && nativeFunctions.All(function => function.Named) &&
        nativeStacks.Callers(nativeFunctions.Single(function => function.Frame == betaFrame).FrameIndex) is
            { Samples: 3, Self: 3, Children: [{ Frame: alphaFrame, Samples: 3 }] },
        "The CPU profile and call tree must show the stored names, and the PDB they came from.");

    // A trace that didn't record the module's PDB: symbolizing reads it from the image, which has the size and timestamp
    // the trace loaded, and stores it.
    WriteNativeSession(cpuNativeImagePath, recordedIdentity: false);
    Expect(SessionSymbolizer.Symbolize(cpuNativeImagePath, nativeSymbols) is { Resolved: 1, Named: 2 } &&
        CpuProfileReader.Read(cpuNativeImagePath) is { Modules: [{ Status: "Resolved" } imageModule], Exclusive: [{ Frame: betaFrame }] } &&
        imageModule.Pdb == nativeIdentity.Key,
        "Symbolizing must read the PDB identity of an image the trace didn't record it for, and store it.");

    // The image on disk was rebuilt since the trace loaded it: its PDB would give wrong names, so none are given.
    WriteNativeSession(cpuNativeStalePath, recordedIdentity: false, timeDateStamp: nativeImage.TimeDateStamp + 1);
    Expect(SessionSymbolizer.Symbolize(cpuNativeStalePath, nativeSymbols) is { NoIdentity: 1, Resolved: 0, Named: 0 } &&
        CpuProfileReader.Read(cpuNativeStalePath) is { Modules: [{ Status: "NoIdentity", Pdb: null }] },
        "Symbolizing must not read the PDB identity of an image that differs from the one the trace loaded.");

    // A recorded CPU profile (Fixtures\NativeSymbols\CpuProfile.etl, from --record-cpu-fixture): one process's kernel samples
    // and stacks, the load of the fixture DLL, deleted since, with the PDB identity the merged trace recorded for it, and two
    // XAML events written by the thread that ran FixtureAlpha, before and after it did. Importing it as the app does
    // (TraceReplay) and as the CLI does (OfflineEtlSource.Read) must store every sample of the process, the module as the
    // trace loaded it, and the fixture's samples on that thread between the events; symbolizing then names their stacks.
    var fixtureEtl = Path.Combine(AppContext.BaseDirectory, "Fixtures", "NativeSymbols", "CpuProfile.etl");
    var fixtureProcess = OfflineEtlSource.ListProcesses(fixtureEtl) is { EventsLost: 0, Processes: [{ XamlEvents: 2 } fixtureOnly] }
        ? fixtureOnly
        : throw new InvalidOperationException("The CPU fixture must have one process, with its two XAML events.");
    var fixtureSamples = CpuFixtureRecorder.CountSamples(fixtureEtl, fixtureProcess.ProcessId);
    Expect(fixtureSamples > 0, "The CPU fixture must have CPU samples of its process.");
    void CheckImport(string session, string how)
    {
        Expect(CpuProfileReader.ReadSampling(session) is { IntervalMs: > 0, Dropped: 0, CompressedStacks: false, Note: null } importSampling &&
            importSampling.Samples == fixtureSamples,
            $"{how} must store every CPU sample of the process, and the sampling interval.");
        // The image is gone, so its PDB identity can only be the trace's.
        Expect(CpuProfileReader.Read(session) is { Modules: [{ Name: "NativeSymbols.dll", Status: null } importModule] } &&
            importModule.Pdb == nativeIdentity.Key && !File.Exists(importModule.FilePath),
            $"{how} must store the sampled module with the PDB identity the trace recorded for it.");
        using (var connection = SessionFile.Open(session))
        {
            long EventOf(string name, string column) => Sql.Scalar(connection,
                $"SELECT Events.{column} FROM Events JOIN EventTypes ON EventTypes.TypeId = Events.TypeId WHERE EventTypes.Name = $name",
                null, ("$name", name));
            var (begin, end, thread) = (EventOf(CpuFixtureRecorder.BeginEvent, "Qpc"), EventOf(CpuFixtureRecorder.EndEvent, "Qpc"),
                EventOf(CpuFixtureRecorder.BeginEvent, "ThreadId"));
            const string fixtureLeaves =
                "FROM CpuSamples JOIN CpuFrames ON CpuFrames.StackId = CpuSamples.StackId WHERE CpuFrames.Module = 'NativeSymbols.dll'";
            Expect(Sql.Scalar(connection, "SELECT ImageSize FROM Modules") == nativeImage.Size &&
                Sql.Scalar(connection, "SELECT TimeDateStamp FROM Modules") == nativeImage.TimeDateStamp,
                $"{how} must store the size and timestamp of the image the trace loaded.");
            Expect(thread > 0 && begin > 0 && end > begin && Sql.Scalar(connection, $"SELECT COUNT(*) {fixtureLeaves}") > 0 &&
                Sql.Scalar(connection, $"SELECT COUNT(*) {fixtureLeaves} AND (CpuSamples.ThreadId <> $thread OR CpuSamples.Qpc NOT BETWEEN $begin AND $end)",
                    null, ("$thread", thread), ("$begin", begin), ("$end", end)) == 0,
                $"{how} must store the fixture's samples on the thread that ran it, timed between the XAML events it wrote around it.");
        }

        Expect(SessionSymbolizer.Symbolize(session, nativeSymbols) is { Modules: 1, Resolved: 1, Failed: 0, Named: > 0 } importSymbolized &&
            importSymbolized.Named == importSymbolized.Addresses &&
            CpuProfileReader.Read(session) is { Modules: [{ Status: "Resolved" }] } &&
            CpuProfileReader.ReadStacks(session) is { } importStacks &&
            importStacks.Functions().SingleOrDefault(function => function.Frame == betaFrame) is { Named: true, Total: > 0 } importBeta &&
            importStacks.Callers(importBeta.FrameIndex) is { Children: [{ Frame: alphaFrame } importCaller] } &&
            importCaller.Samples == importBeta.Total,
            $"Symbolizing what {how} stored must name the fixture's stacks, FixtureBeta called by FixtureAlpha, from the recorded PDB identity.");
    }

    var replayDirectory = Directory.CreateDirectory(Path.Combine(cpuSymbols, "Replay")).FullName;
    string replayPath;
    await using (var replay = TraceReplay.Start(fixtureEtl, fixtureProcess.ProcessId, fixtureProcess.Name, replayDirectory))
    {
        await replay.Completion.WaitAsync(TimeSpan.FromMinutes(1));
        Expect(replay is { State: TraceReplayState.Finished, Failure: null, RecordingError: null, Status: { XamlEvents: 2, LostEvents: 0 } },
            "The app's import of a CPU profile must read it to its end.");
        replayPath = replay.Recording.Path;
    }

    CheckImport(replayPath, "The app's import (TraceReplay)");

    var readPath = Path.Combine(cpuSymbols, "Read.sqlite");
    var readWriter = SessionWriter.Create(readPath, new(fixtureProcess.ProcessId, fixtureProcess.Name, null,
        OfflineEtlSource.ReadQpcFrequency(fixtureEtl), null, SessionCoverage.Trace, fixtureEtl));
    EtlReadResult? readResult = null;
    long readEvents = 0;
    try
    {
        readResult = OfflineEtlSource.Read(fixtureEtl, fixtureProcess.ProcessId, [], profilerEvent => readWriter.Process(++readEvents, profilerEvent),
            readWriter.SetProcessStart, default, readWriter);
    }
    finally
    {
        readWriter.Close(readResult is null ? "Reading the trace failed" : "End of trace", readResult?.EventsLost, 0);
    }

    Expect(readResult is { EventsLost: 0 } && readEvents == 2 && readWriter.Failure is null,
        "The CLI's import of a CPU profile must read the process's events to the trace's end.");
    CheckImport(readPath, "The CLI's import (OfflineEtlSource.Read)");

    SessionWriter.Create(cpuEmptyPath, new(1, "CpuTests", null, 1_000_000, 0, SessionCoverage.Trace)).Dispose();
    Expect(CpuProfileReader.Read(cpuEmptyPath) is null && CpuProfileReader.ReadStacks(cpuEmptyPath) is null,
        "A session that didn't sample CPU must have no CPU profile.");

    // A recording's samples are read before it ends, while it has no CpuSampling row yet.
    using (var liveWriter = SessionWriter.Create(cpuLivePath, new(1, "CpuTests", null, 1_000_000, 0, SessionCoverage.Trace)))
    {
        var liveModule = CpuModule();
        CodeFrame LiveAt(ulong rva) => new(liveModule, cpuBase + rva);
        liveWriter.PostSample(new(1_000, 1, 5, [LiveAt(0x1100), LiveAt(0x1000)]), wait: true);
        liveWriter.PostSample(new(2_000, 1, 6, [LiveAt(0x1200), LiveAt(0x1000)]), wait: true);
        liveWriter.PostSample(new(3_000, 1, 6, [LiveAt(0x1200), LiveAt(0x1000)]), wait: true);
        var deadline = DateTime.UtcNow.AddSeconds(10);
        while (CpuProfileReader.ReadSampling(cpuLivePath)?.Samples != 3 && DateTime.UtcNow < deadline)
        {
            Thread.Sleep(50);
        }

        Expect(CpuProfileReader.ReadSampling(cpuLivePath) is { IntervalMs: null, Samples: 3, SymbolizedUtc: null } &&
            CpuProfileReader.ReadStacks(cpuLivePath) is { Samples: 3, Threads.Count: 2, Sampling: { IntervalMs: null, Samples: 3 } } liveStacks &&
            liveStacks.TopDown().Children is [{ Frame: "Test.dll!0x1000", Samples: 3 }] &&
            CpuProfileReader.Read(cpuLivePath) is { Samples: 3, Exclusive: [{ Frame: "Test.dll!0x1200", Samples: 2, Ms: null }, ..] },
            "A recording's stored samples must be read before it ends, without a sampling interval.");
        liveWriter.SetCpuSampling(new(1.0, false, 3, [liveModule]), null);
    }

    Expect(CpuProfileReader.ReadSampling(cpuLivePath) is { IntervalMs: 1.0, Samples: 3 },
        "The recording's end must write its sampling row.");

    // While a session records, its functions are named in memory rather than in the file.
    Expect(CpuProfileReader.ReadStacks(cpuLivePath, functionName: address => $"F{address}")?.Functions() is { Count: 3 } namedFunctions &&
        namedFunctions.All(function => function.Named && function.Frame.StartsWith("Test.dll!F", StringComparison.Ordinal)),
        "functionName must name the frames the file doesn't name yet.");
    ExpectThrows<ArgumentException>(() => new LiveSymbolNamer(cpuLivePath, "srv*").Dispose(),
        "A live namer must reject a malformed symbol path.");
    using (var liveNamer = new LiveSymbolNamer(cpuLivePath, cpuSymbols))
    {
        Expect(!liveNamer.Requested && CpuProfileReader.ReadStacks(cpuLivePath, functionName: liveNamer.Request) is { Samples: 3 } &&
            liveNamer.Requested,
            "Reading with Request must ask for the frames that aren't named.");
        Expect(liveNamer.Name() == 0 && !liveNamer.Requested &&
            CpuProfileReader.ReadStacks(cpuLivePath, functionName: liveNamer.NameOf)!.Functions().All(function => !function.Named),
            "A module without a PDB identity must stay unnamed, not fail naming.");
        Expect(CpuProfileReader.ReadStacks(cpuLivePath, functionName: liveNamer.Request) is not null && liveNamer.Requested,
            "A module whose PDB identity isn't known yet must be asked for again, as the recording may still find it.");
    }

    using (var identityNamer = new LiveSymbolNamer(cpuIdentityPath, cpuSymbols))
    {
        _ = CpuProfileReader.ReadStacks(cpuIdentityPath, functionName: identityNamer.Request);
        Expect(identityNamer.Requested && identityNamer.Name() == 0 && !identityNamer.Requested &&
            CpuProfileReader.ReadStacks(cpuIdentityPath, functionName: identityNamer.Request) is not null && !identityNamer.Requested,
            "Addresses whose PDB isn't on the symbol path, or outside any module, must be looked up once, not on every read.");
        identityNamer.Dispose();
        ExpectThrows<ObjectDisposedException>(() => identityNamer.Name(), "A disposed live namer must not name.");
    }

    // A symbol server that takes each PDB request and stops, before its answer or after the start of the PDB. Canceling a
    // naming round, and disposing the namer during the next one, must stop the request or download, free the symbolizing
    // gate and keep nothing of the PDB. The next round must ask the server again.
    foreach (var answers in new[] { false, true })
    {
        var stallCache = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-Stall-{Guid.NewGuid():N}");
        var listener = new System.Net.Sockets.TcpListener(System.Net.IPAddress.Loopback, 0);
        listener.Start();
        using var serverStop = new CancellationTokenSource();
        using var requests = new SemaphoreSlim(0);
        var server = Task.Run(async () =>
        {
            var connections = new List<Task>();
            try
            {
                while (true)
                {
                    var client = await listener.AcceptTcpClientAsync(serverStop.Token);
                    connections.Add(Task.Run(async () =>
                    {
                        using (client)
                        {
                            try
                            {
                                var stream = client.GetStream();
                                var received = new System.Text.StringBuilder();
                                var buffer = new byte[4096];
                                while (!received.ToString().Contains("\r\n\r\n", StringComparison.Ordinal))
                                {
                                    var read = await stream.ReadAsync(buffer, serverStop.Token);
                                    if (read == 0)
                                    {
                                        return;
                                    }

                                    received.Append(System.Text.Encoding.ASCII.GetString(buffer, 0, read));
                                }

                                requests.Release();
                                if (answers)
                                {
                                    await stream.WriteAsync(System.Text.Encoding.ASCII.GetBytes(
                                        "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 104857600\r\n\r\n"),
                                        serverStop.Token);
                                    await stream.WriteAsync(new byte[1000], serverStop.Token);
                                }

                                await Task.Delay(Timeout.Infinite, serverStop.Token);
                            }
                            catch (Exception error) when (error is OperationCanceledException or IOException)
                            {
                            }
                        }
                    }));
                }
            }
            catch (Exception error) when (error is OperationCanceledException or System.Net.Sockets.SocketException or ObjectDisposedException)
            {
            }

            await Task.WhenAll(connections);
        });

        static Exception? Ended(Task round)
        {
            try
            {
                return round.Wait(TimeSpan.FromSeconds(5)) ? null : new TimeoutException();
            }
            catch (AggregateException error)
            {
                return error.InnerException;
            }
        }

        try
        {
            var port = ((System.Net.IPEndPoint)listener.LocalEndpoint).Port;
            using var stallNamer = new LiveSymbolNamer(cpuIdentityPath, $"srv*{stallCache}*http://127.0.0.1:{port}");
            _ = CpuProfileReader.ReadStacks(cpuIdentityPath, functionName: stallNamer.Request);
            using (var cancel = new CancellationTokenSource())
            {
                var round = Task.Run(() => stallNamer.Name(cancel.Token));
                var asked = requests.Wait(TimeSpan.FromSeconds(10));

                // SymbolReader copies the answer to a temporary file in the cache, then waits for the rest of the PDB.
                var downloading = !answers;
                var deadline = DateTime.UtcNow.AddSeconds(10);
                while (!downloading && DateTime.UtcNow < deadline)
                {
                    downloading = Directory.Exists(stallCache) &&
                        Directory.EnumerateFiles(stallCache, "*.new*", SearchOption.AllDirectories).Any();
                    Thread.Sleep(50);
                }

                cancel.Cancel();
                Expect(asked && downloading && Ended(round) is OperationCanceledException &&
                    SessionSymbolizer.Gate.CurrentCount == 1 && stallNamer.Requested,
                    $"Canceling a naming round must stop its symbol server {(answers ? "download" : "request")} and free the gate.");
            }

            while (requests.Wait(0))
            {
            }

            var next = Task.Run(() => stallNamer.Name());
            var askedAgain = requests.Wait(TimeSpan.FromSeconds(10));
            stallNamer.Dispose();
            Expect(askedAgain && Ended(next) is OperationCanceledException && SessionSymbolizer.Gate.CurrentCount == 1,
                "A canceled lookup must be retried by the next round, and disposing the namer must stop that round.");
        }
        finally
        {
            serverStop.Cancel();
            listener.Stop();
            _ = server.Wait(TimeSpan.FromSeconds(10));
        }

        Expect(!Directory.Exists(stallCache) || !Directory.EnumerateFiles(stallCache, "*", SearchOption.AllDirectories).Any(),
            "A canceled download must keep nothing of the PDB.");
        if (Directory.Exists(stallCache))
        {
            Directory.Delete(stallCache, recursive: true);
        }
    }

    // A session over its size limit deletes its oldest samples, then the stacks and code addresses no sample has any more.
    const int trimSamples = 30_000;
    var trimModule = CpuModule();
    Exception? trimFailure;
    using (var trimWriter = SessionWriter.Create(cpuTrimPath, new(1, "CpuTests", null, 1_000_000, 0, SessionCoverage.Trace),
        limitBytes: 1024 * 1024))
    {
        // Each sample has its own leaf, so each adds a stack node and a code address. The first 1,000 but one have a root of
        // their own, which trimming must delete with them. The root the rest have is the first sample's whole stack, which
        // trimming must keep for the samples it keeps.
        for (var sample = 0; sample < trimSamples; sample++)
        {
            CodeFrame root = new(trimModule, cpuBase + (sample is > 0 and < 1_000 ? 0xE000UL : 0xF000UL));
            CodeFrame[] frames = sample == 0 ? [root] : [new(trimModule, cpuBase + 0x100 + (ulong)sample), root];
            trimWriter.PostSample(new(1_000 + sample, 1, 5, frames), wait: true);
        }

        // The second sample's leaf again, under the other root, after trimming deleted it.
        trimWriter.PostSample(new(1_000 + trimSamples, 1, 5,
            [new(trimModule, cpuBase + 0x101), new(trimModule, cpuBase + 0xF000)]), wait: true);
        trimWriter.SetCpuSampling(new(1.0, false, trimSamples + 1, [trimModule]), null);
        trimWriter.Close("Test complete");
        trimFailure = trimWriter.Failure;
    }

    var trimmedSampling = CpuProfileReader.ReadSampling(cpuTrimPath);
    using (var trimConnection = SessionFile.Open(cpuTrimPath))
    {
        Expect(trimFailure is null && new FileInfo(cpuTrimPath).Length <= 1024 * 1024 &&
            trimmedSampling is { Samples: > 0 and < trimSamples, Note: CpuSampleIndex.TrimmedNote } &&
            Sql.Scalar(trimConnection, "SELECT MIN(Qpc) FROM CpuSamples") >= 2_000 &&
            Sql.Scalar(trimConnection, "SELECT MAX(Qpc) FROM CpuSamples") == 1_000 + trimSamples,
            "A session over its size limit must delete its oldest CPU samples, and say so.");
        Expect(Sql.Scalar(trimConnection, "SELECT COUNT(*) FROM CodeAddresses WHERE Rva = 0xE000") == 0 &&
            Sql.Scalar(trimConnection, "SELECT COUNT(*) FROM CallStacks s JOIN CodeAddresses a USING (AddressId) WHERE a.Rva = 0xF000") == 1,
            "Trimming must delete a caller only trimmed samples had, and keep one kept samples have, though its own sample went.");
        Expect(Sql.Scalar(trimConnection, """
            WITH RECURSIVE Used (StackId) AS (
                SELECT StackId FROM CpuSamples
                UNION
                SELECT s.ParentId FROM CallStacks s JOIN Used u ON s.StackId = u.StackId WHERE s.ParentId IS NOT NULL)
            SELECT (SELECT COUNT(*) FROM CallStacks WHERE StackId NOT IN (SELECT StackId FROM Used)) +
                (SELECT COUNT(*) FROM CodeAddresses WHERE AddressId NOT IN (SELECT AddressId FROM CallStacks)) +
                (SELECT COUNT(*) FROM CpuSamples WHERE StackId NOT IN (SELECT StackId FROM CallStacks)) +
                (SELECT COUNT(*) FROM CallStacks WHERE ParentId IS NOT NULL AND ParentId NOT IN (SELECT StackId FROM CallStacks)) +
                (SELECT COUNT(*) FROM CallStacks WHERE AddressId NOT IN (SELECT AddressId FROM CodeAddresses))
            """) == 0,
            "Trimming must delete the stacks and code addresses no sample has, and only those.");
    }

    Expect(CpuProfileReader.ReadStacks(cpuTrimPath) is { } trimmedStacks && trimmedStacks.Samples == trimmedSampling!.Samples &&
        trimmedStacks.Functions().Single(function => function.Frame == "Test.dll!0x101") is { Self: 1, Total: 1 },
        "A frame whose rows trimming deleted must be stored again by a later sample.");

    // A live sample can reach the writer after the events of its time. Trimming the events deletes the samples from before
    // the time the file is then complete from, so one that comes later from before that time isn't stored either. The
    // writer stores a batch's events before its samples, so these come after every trim the events made.
    const int lateEvents = 3_000;
    Exception? lateFailure;
    using (var lateWriter = SessionWriter.Create(cpuLatePath, new(1, "CpuTests", null, 1_000_000, 0, SessionCoverage.Trace),
        limitBytes: 1024 * 1024))
    {
        var lateModule = CpuModule();
        var padding = new string('x', 2_000);
        for (var index = 0; index < lateEvents; index++)
        {
            lateWriter.Process(index + 1, new(1_000 + index, index / 1000.0, 1, 5, XamlProvider.Xaml, true, "Tick", 0,
                [new("Padding", padding)]));
        }

        lateWriter.PostSample(new(1_000 + lateEvents, 1, 5, [new(lateModule, cpuBase + 0x1100), new(lateModule, cpuBase + 0x1000)]),
            wait: true);
        lateWriter.PostSample(new(1_000, 1, 5, [new(lateModule, cpuBase + 0x1200), new(lateModule, cpuBase + 0x1000)]), wait: true);
        lateWriter.SetCpuSampling(new(1.0, false, 2, [lateModule]), null);
        lateWriter.Close("Test complete");
        lateFailure = lateWriter.Failure;
    }

    using (var lateConnection = SessionFile.Open(cpuLatePath))
    {
        Expect(lateFailure is null && Sql.Scalar(lateConnection, "SELECT CompleteFromQpc FROM SessionInfo") > 1_000 &&
            Sql.Scalar(lateConnection, "SELECT COUNT(*) FROM CpuSamples WHERE Qpc < (SELECT CompleteFromQpc FROM SessionInfo)") == 0 &&
            Sql.Scalar(lateConnection, "SELECT COUNT(*) FROM CpuSamples") == 1 &&
            Sql.Scalar(lateConnection, "SELECT COUNT(*) FROM CodeAddresses WHERE Rva = 0x1200") == 0 &&
            CpuProfileReader.ReadSampling(cpuLatePath) is { Samples: 1, Note: CpuSampleIndex.TrimmedNote },
            "A sample that comes after trimming, from before the time the file is complete from, must not be stored.");
    }

    // An address outside every module known when its sample was stored goes to the module the trace's end named, if that
    // module had it at the sample's time.
    using (var placeWriter = SessionWriter.Create(cpuPlacePath, new(1, "CpuTests", null, 1_000_000, 0, SessionCoverage.Trace)))
    {
        var placeRoot = CpuModule();
        var late = new SampledModule(1, @"C:\WinUIProfilerTests\Missing\Late.dll", 0x7FF1_0000_0000, 0x10000, 0) { EndOnly = true };
        var gone = new SampledModule(1, @"C:\WinUIProfilerTests\Missing\Gone.dll", 0x7FF2_0000_0000, 0x10000, 0)
        {
            EndOnly = true,
            UnloadQpc = 500
        };
        placeWriter.PostSample(new(1_000, 1, 5, [new(null, 0x7FF1_0000_1100), new(placeRoot, cpuBase + 0x1000)]), wait: true);
        placeWriter.PostSample(new(2_000, 1, 5, [new(null, 0x7FF2_0000_1100), new(placeRoot, cpuBase + 0x1000)]), wait: true);
        placeWriter.SetCpuSampling(new(1.0, false, 2, [placeRoot, late, gone]), null);
    }

    Expect(CpuProfileReader.Read(cpuPlacePath) is { Exclusive: var placedFrames, Modules: var placedModules } &&
        placedFrames.Select(frame => frame.Frame).Order(StringComparer.Ordinal).SequenceEqual(["?!0x7FF200001100", "Late.dll!0x1100"]) &&
        placedModules.Any(module => module is { Name: "Late.dll", Addresses: 1 }) &&
        placedModules.All(module => module.Name != "Gone.dll"),
        "An address must go to the module the trace's end named only when that module had it while it was sampled.");
}
finally
{
    foreach (var cpuFile in new[] { cpuPath, cpuIdentityPath, cpuEmptyPath, cpuLivePath, cpuTrimPath, cpuPlacePath, cpuLatePath,
        cpuNativePath, cpuNativeImagePath, cpuNativeStalePath }.SelectMany(cpuSession =>
        new[] { cpuSession, cpuSession + "-wal", cpuSession + "-shm", cpuSession + "-journal" }))
    {
        File.Delete(cpuFile);
    }

    if (Directory.Exists(cpuSymbols))
    {
        Directory.Delete(cpuSymbols, recursive: true);
    }
}
Console.WriteLine("Passed: CPU samples read back by thread, frame and call tree, symbolizing names native functions from their PDB and reports modules it can't name, and live naming names only what was asked for; a recorded CPU profile imports with its samples' threads, times and module identity; trimming keeps stacks whole, and late modules get their addresses.");

// The live kernel session keeps the samples from before it's read, and gives the sink the target's complete samples.
KernelSampleSession? liveCpu = null;
try
{
    liveCpu = KernelSampleSession.Start();
}
catch (UnauthorizedAccessException)
{
}

if (liveCpu is null)
{
    Console.WriteLine("Skipped: live CPU sampling needs an elevated test run.");
}
else
{
    var liveSamples = new List<CompletedSample>();
    long liveSpins = 0;
    void SpinCpu()
    {
        var spinEnd = Environment.TickCount64 + 400;
        while (Environment.TickCount64 < spinEnd)
        {
            liveSpins++;
        }
    }

    using (liveCpu)
    {
        SpinCpu();
        liveCpu.StartReading((processId, _) => processId == Environment.ProcessId, sample =>
        {
            lock (liveSamples)
            {
                liveSamples.Add(sample);
            }
        });
        SpinCpu();
        var liveSummary = liveCpu.Stop();
        Expect(liveSpins > 0 && liveCpu.Failure is null && liveSummary is { Samples: > 0, IntervalMilliseconds: > 0 } &&
            liveSamples.Count == liveSummary.Samples && liveSamples.All(sample => sample.ProcessId == Environment.ProcessId) &&
            liveSamples.Any(sample => sample.Frames.Length > 1 && sample.Frames.Any(frame => frame.Module is not null)),
            "A live session must sample this process's stacks in its modules.");
        Expect(liveCpu.Stop() == liveSummary, "Stopping the live session again must return the same summary.");
    }

    Console.WriteLine($"Passed: the live kernel session sampled this process {liveSamples.Count} times, with stacks.");
}

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

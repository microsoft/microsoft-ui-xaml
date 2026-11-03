// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Launch;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Contracts;
using XamlTimeline;

const long element = 0x1234;
var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-EventCompatibility-{Guid.NewGuid():N}.sqlite");
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
    foreach (var file in new[] { path, path + "-wal", path + "-shm" })
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
// the trace's, so a launch timeline that follows the file must not keep times from before.
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

    writer.SetProcessStart(new(Qpc(-5), "Contoso", 100, "explorer"));
    Store(Frame(0, 1, true), Frame(40, 1, false), Frame(60, 2, true),
        new(Qpc(100), 100, 200, 1, XamlProvider.Marker, true, "Ready", 0, [], contoso));
    var following = new LaunchTimelineReader(launchPath, config);
    following.Read();
    Store(Frame(1500, 2, false));
    var followed = Describe(following.Read());
    var fresh = Describe(new LaunchTimelineReader(launchPath, config).Read());
    if (followed != fresh || fresh != "Process created 0 ms, First frame drawn 45 ms, Ready 105 ms")
    {
        throw new InvalidOperationException(
            $"A launch timeline read before the trace's frequency was known must be timed with it once it is: followed '{followed}', afresh '{fresh}'.");
    }

    Console.WriteLine("Passed: a launch timeline that follows a recorded trace's session file is retimed once its frequency is known.");
}
finally
{
    foreach (var file in new[] { launchPath, launchPath + "-wal", launchPath + "-shm" })
    {
        File.Delete(file);
    }
}

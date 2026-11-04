// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
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

// A trace's time zero is the process's start event when it recorded one: the latest at or before the process's first event,
// as in a live session that saw the process start. A start after the first event is not this process's.
CheckTimeZero([400_000, 1_000_000, 1_300_000], 0, [250, 260]);
Console.WriteLine("Passed: a trace's time zero is the process's latest start event at or before its first event.");
CheckTimeZero([1_300_000], null, [0, 10]);
Console.WriteLine("Passed: without such a start event, a trace's time zero is its first event and the process start is unknown.");

static void CheckTimeZero(long[] processStarts, double? processStartMilliseconds, double[] eventMilliseconds)
{
    var sessionPath = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-TimeZero-{Guid.NewGuid():N}.sqlite");
    try
    {
        using (var writer = SessionWriter.Create(sessionPath, new(1, "TimeZeroTests", null, 1_000_000, 0, SessionCoverage.Trace)))
        {
            foreach (var qpc in processStarts)
            {
                writer.SetProcessStart(qpc);
            }

            writer.Process(1, new(1_250_000, 0, 1, 1, XamlProvider.Xaml, true, "First", 0, []));
            writer.Process(2, new(1_260_000, 10, 1, 1, XamlProvider.Xaml, true, "Second", 0, []));
        }

        var times = new List<double>();
        SessionFile.Read(sessionPath, profilerEvent => times.Add(profilerEvent.RelativeMilliseconds));
        var info = SessionFile.ReadInfo(sessionPath);
        if (info.ProcessStartMilliseconds != processStartMilliseconds || times.Count != eventMilliseconds.Length ||
            times.Zip(eventMilliseconds).Any(time => Math.Abs(time.First - time.Second) > 1e-9))
        {
            throw new InvalidOperationException(
                $"The process started at {info.ProcessStartMilliseconds?.ToString() ?? "an unknown time"} and the events are at " +
                $"[{string.Join(", ", times)}] ms; expected {processStartMilliseconds?.ToString() ?? "an unknown time"} and " +
                $"[{string.Join(", ", eventMilliseconds)}] ms.");
        }
    }
    finally
    {
        foreach (var file in new[] { sessionPath, sessionPath + "-wal", sessionPath + "-shm" })
        {
            File.Delete(file);
        }
    }
}

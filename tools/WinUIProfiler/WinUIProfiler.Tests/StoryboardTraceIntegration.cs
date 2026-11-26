// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Runtime.Versioning;
using Microsoft.Diagnostics.Tracing;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;

internal static class StoryboardTraceIntegration
{
    // The enabled workload begins nested storyboards, pauses/resumes/stops/restarts a root, attempts an invalid
    // Begin, rejects a public Begin on the nested storyboard, and begins a storyboard with a long x:Name.
    // The control runs the same workload.
    [SupportedOSPlatform("windows")]
    internal static void Run(string enabledTrace, int enabledPid, string disabledTrace, int disabledPid)
    {
        var enabled = Read(enabledTrace, enabledPid);
        var disabled = Read(disabledTrace, disabledPid);
        var activity = enabled.Where(value => value.Provider == XamlProvider.XamlProfiler && value.Name == "StoryboardActivity").ToArray();
        Check(activity.Length > 0, "The instrumented runtime must produce real StoryboardActivity events.");
        Check(disabled.Any(value => value.Provider == XamlProvider.Xaml),
            "The disabled-provider control must still exercise XAML.");
        Check(disabled.All(value => value.Name != "StoryboardActivity"),
            "Storyboard activity must not leak onto the default XAML provider.");
        foreach (var value in activity)
        {
            Check(value.TraceLogging && value.Fields.Select(field => field.Name).SequenceEqual(new[]
            {
                "StoryboardId", "ObjectAddress", "RunId", "Operation", "Name", "State", "IsPaused",
                "HasIndependentAnimation", "IsInTimeManager", "ChildCount"
            }), "Native events must preserve the ordered, self-describing payload contract.");
        }

        var operations = activity.Select(value => Field(value, "Operation")).ToHashSet();
        foreach (var operation in new[] { "BeginRequested", "Begin", "BeginFailed", "Registered", "Unregistered",
            "PauseRequested", "ResumeRequested", "StopRequested", "Observed", "Destroyed" })
        {
            Check(operations.Contains(operation), $"The real trace must exercise {operation}.");
        }
        Check(activity.Any(value => Field(value, "Name") == "RootPulse")
            && activity.Any(value => Field(value, "Name") == "NestedPulse"),
            "Both root and nested storyboards must have attribution.");
        var nested = activity.Where(value => Field(value, "Name") == "NestedPulse").ToArray();
        var rejected = nested.Where(value => Field(value, "Operation") == "BeginFailed").ToArray();
        Check(rejected.Length == 1 && RunId(rejected[0]) == 2,
            "The public wrapper's rejected nested Begin must advance the tracked generation exactly once.");
        Check(nested.Any(value => RunId(value) == 2 && Field(value, "Operation") == "BeginRequested")
            && !nested.Any(value => RunId(value) == 2 && Field(value, "Operation") == "Begin")
            && nested.Any(value => RunId(value) == 2 && Field(value, "Operation") == "Observed"
                && Field(value, "State") == "Active")
            && nested.Any(value => RunId(value) == 3 && Field(value, "Operation") == "Begin"),
            "A rejected Begin must report the attempt/failure without stopping the clock or double-counting later Begins.");
        Check(activity.Any(value => Field(value, "Name") is { Length: 128 } name && name.EndsWith("...", StringComparison.Ordinal)),
            "The native producer must bound long x:Name labels.");

        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-StoryboardTrace-{Guid.NewGuid():N}.sqlite");
        try
        {
            var frequency = OfflineEtlSource.ReadQpcFrequency(enabledTrace)
                ?? throw new InvalidDataException("The integration trace has no usable event clock.");
            using (var writer = SessionWriter.Create(path, new(enabledPid, "StoryboardProbe", null,
                frequency, enabled.Min(value => value.TimestampQpc), SessionCoverage.Trace, enabledTrace)))
            {
                long sequence = 0;
                foreach (var value in enabled) writer.Process(++sequence, value);
                writer.Close("Integration complete");
                if (writer.Failure is { } error) throw error;
            }

            var result = SessionFile.ReadStoryboards(path);
            Check(result.Observations == activity.Length && result.Storyboards.Any(row => row.Name == "RootPulse" && row.RunId == 2),
                "Real decoding, generic storage and storyboard projection must preserve restarts and identities.");
            Check(result.Storyboards.Any(row => row.Name == "FailedBegin" && !row.BeginObserved && row.State is "Stopped" or "Destroyed"),
                "Failed native Begin attempts must not appear successful.");
            Check(!result.HistoryIncomplete && result.Storyboards.All(row => row.State == "Destroyed"),
                "Known failed Begins must not invent missing successful Begins, and destruction must be observed.");
            Console.WriteLine($"Passed: {activity.Length} native storyboard observations, nested attribution, restart generations, " +
                "pause/resume/stop, failed and wrapper-rejected Begins, bounded names, SQLite projection and disabled-provider control.");
        }
        finally
        {
            foreach (var file in new[] { path, path + "-wal", path + "-shm" })
            {
                if (File.Exists(file)) File.Delete(file);
            }
        }
    }

    [SupportedOSPlatform("windows")]
    private static List<ProfilerEvent> Read(string path, int pid)
    {
        ArgumentOutOfRangeException.ThrowIfNegativeOrZero(pid);
        var events = new List<ProfilerEvent>();
        using var source = new ETWTraceEventSource(path);
        XamlEventSubscription.Subscribe(source, pid, [], events.Add);
        source.Process();
        Check(source.EventsLost == 0, "The integration trace must not lose events.");
        return events;
    }

    private static string Field(ProfilerEvent value, string name) =>
        value.Fields.Single(field => field.Name == name).Value as string
            ?? throw new InvalidDataException($"Missing native string field {name}.");

    private static ulong RunId(ProfilerEvent value) =>
        value.Fields.Single(field => field.Name == "RunId").Value switch
        {
            ulong run => run,
            long run when run >= 0 => (ulong)run,
            _ => throw new InvalidDataException("Missing or invalid native RunId.")
        };

    private static void Check(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
}

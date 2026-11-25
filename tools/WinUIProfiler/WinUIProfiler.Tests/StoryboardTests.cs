// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

internal static class StoryboardTests
{
    internal static void Run()
    {
        var root = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-Storyboards-{Guid.NewGuid():N}");
        Directory.CreateDirectory(root);
        try
        {
            var path = Path.Combine(root, "activity.sqlite");
            Write(path,
                Event(0, 1, 1, "BeginRequested", "Stopped"),
                Event(1, 1, 1, "Begin", "NotStarted", name: "Pulse"),
                Event(2, 1, 1, "Observed", "Active", independent: true),
                Event(3, 1, 1, "PauseRequested", "Paused", paused: true),
                Event(4, 1, 1, "ResumeRequested", "Active"),
                Event(5, 1, 1, "StopRequested", "Stopped"),
                Event(6, 1, 1, "Unregistered", "Stopped", registered: false),
                Event(7, 1, 2, "BeginRequested", "Stopped", name: "Second run"),
                Event(8, 1, 2, "Begin", "NotStarted"),
                Event(9, 1, 2, "Observed", "Active", independent: true),
                Event(10, ulong.MaxValue, 1, "Observed", "Active", address: ulong.MaxValue),
                Event(11, 1, 2, "Destroyed", "Destroyed", name: "", registered: false));

            var at = SessionFile.ReadStoryboards(path, 2);
            Check(at.Observations == 3 && at.Storyboards is
                [{ RunId: 1, Name: "Pulse", State: "Active", HasIndependentAnimation: true, BeginObserved: true,
                    FirstMilliseconds: 0, LastMilliseconds: 2 }],
                "A time selection must not include later pauses, restarts or destruction.");
            Check(SessionFile.ReadStoryboards(path, 3).Storyboards is [{ State: "Paused", IsPaused: true }],
                "A pause request must retain its observed paused flag.");
            Check(SessionFile.ReadStoryboards(path, 4).Storyboards is [{ State: "Active", IsPaused: false }],
                "A resume request must update the observation without claiming a compositor result.");
            Check(SessionFile.ReadStoryboards(path, 6).Storyboards is [{ State: "Stopped", IsInTimeManager: false }],
                "Removal must clear time-manager bookkeeping.");
            Check(SessionFile.ReadStoryboards(path, 7).Storyboards is [{ RunId: 2, BeginObserved: false }]
                && SessionFile.ReadStoryboards(path, 7).HistoryIncomplete,
                "Beginning again must not inherit the earlier run's successful Begin.");
            var latest = SessionFile.ReadStoryboards(path);
            Check(latest.Storyboards is
                [{ StoryboardId: 1, RunId: 2, State: "Destroyed", Name: "Second run", BeginObserved: true, ChildCount: 2 },
                 { StoryboardId: ulong.MaxValue, ObjectAddress: ulong.MaxValue, BeginObserved: false }]
                && latest.HistoryIncomplete,
                "Destruction must retain the label, and late capture and full-width identities must survive storage.");

            var reuse = Path.Combine(root, "reuse.sqlite");
            Write(reuse, Event(1, 1, 1, "Begin", "Active"),
                Event(2, 1, 1, "Destroyed", "Destroyed"), Event(3, 2, 1, "Begin", "Active"),
                Event(4, 3, 1, "BeginFailed", "Stopped"));
            var reused = SessionFile.ReadStoryboards(reuse);
            Check(reused.Storyboards.Count == 3 && reused.Storyboards.Count(row => row.ObjectAddress == 0x1234) == 3
                && reused.Storyboards[0] is { State: "Stopped", BeginObserved: false, BeginFailed: true } && !reused.HistoryIncomplete,
                "Reused addresses must not merge lifetimes; a failed Begin is not a successful start.");
            var limited = SessionFile.ReadStoryboards(reuse, limit: 2);
            Check(limited.Limited && limited.HistoryIncomplete && limited.Storyboards.Select(row => row.StoryboardId)
                .SequenceEqual(new ulong[] { 3, 2 }), "The limit must retain the most recent lifetimes and report omissions.");

            var legacy = Path.Combine(root, "legacy.sqlite");
            Write(legacy,
                Event(1, 1, 1, "Observed", "Active") with { Name = "BeginStoryboard/Start", TraceLogging = false },
                Event(2, 2, 1, "Observed", "Active") with { Provider = XamlProvider.Xaml });
            Check(SessionFile.ReadStoryboards(legacy) is { Observations: 0, Storyboards.Count: 0, Limited: false },
                "Old manifest storyboard regions and other providers must not fabricate bookkeeping observations.");
            Check(SessionFile.ReadObjects(path).Count == 0,
                "Storyboard identities must not be indexed as UI elements or visuals.");
            var destroyedOnly = Path.Combine(root, "destroyed-only.sqlite");
            Write(destroyedOnly, Event(1, 1, 1, "Destroyed", "Destroyed"));
            Check(SessionFile.ReadStoryboards(destroyedOnly).Storyboards is [{ ChildCount: null, BeginObserved: false }],
                "Destruction-only history has no measured child count, not zero children.");
            var lateCapture = Path.Combine(root, "late-capture.sqlite");
            Write(lateCapture, Event(1, 1, 0, "Observed", "Active"));
            Check(SessionFile.ReadStoryboards(lateCapture) is
                { HistoryIncomplete: true, Storyboards: [{ RunId: 0, BeginObserved: false, State: "Active" }] },
                "Lazy profiler enrollment may first observe generation zero; it must not invent a captured Begin.");

            var reordered = Path.Combine(root, "reordered.sqlite");
            Write(reordered, Event(3, 1, 1, "PauseRequested", "Paused", name: "Latest", paused: true),
                Event(2, 1, 1, "Observed", "Active", name: "Earlier"), Event(1, 1, 1, "Begin", "NotStarted"));
            Check(SessionFile.ReadStoryboards(reordered).Storyboards is
                [{ State: "Paused", Name: "Latest", BeginObserved: true, FirstMilliseconds: 1, LastMilliseconds: 3 }],
                "Out-of-order delivery must not replace a later state or label, but may supply a missing Begin.");
            var reentry = Path.Combine(root, "reentry.sqlite");
            Write(reentry, Event(1, 1, 1, "Begin", "Active"), Event(2, 2, 1, "Begin", "Active"),
                Event(3, 1, 1, "Observed", "Active"));
            Check(SessionFile.ReadStoryboards(reentry, limit: 1) is
                { Limited: true, HistoryIncomplete: true, Storyboards: [{ StoryboardId: 1, BeginObserved: false }] },
                "An evicted lifetime that returns must not silently recover its missing Begin.");

            var complete = Path.Combine(root, "complete.sqlite");
            Write(complete, Event(1, 1, 1, "Begin", "Active"), Event(2, 1, 1, "StopRequested", "Stopped"));
            Check(!SessionFile.ReadStoryboards(complete).HistoryIncomplete, "The complete control must have a captured Begin.");
            using (var connection = new SqliteConnection($"Data Source={complete};Pooling=False"))
            {
                connection.Open();
                foreach (var column in new[] { "CompleteFromSeq", "LostByEtw", "DroppedByProfiler" })
                {
                    using var command = connection.CreateCommand();
                    command.CommandText = $"UPDATE SessionInfo SET CompleteFromSeq = NULL, LostByEtw = NULL, DroppedByProfiler = NULL, {column} = 2";
                    command.ExecuteNonQuery();
                    Check(SessionFile.ReadStoryboards(complete).HistoryIncomplete,
                        $"{column} must report partial history independently of surviving Begin records.");
                }
            }

            var malformed = Path.Combine(root, "malformed.sqlite");
            Write(malformed, Event(1, 1, 1, "Observed", "Active") with
            {
                Fields = Event(1, 1, 1, "Observed", "Active").Fields.Where(field => field.Name != "RunId").ToArray()
            });
            Throws<InvalidDataException>(() => SessionFile.ReadStoryboards(malformed));
            var changedAddress = Path.Combine(root, "changed-address.sqlite");
            Write(changedAddress, Event(1, 1, 1, "Begin", "Active"),
                Event(2, 1, 1, "Observed", "Active", address: 0x4321));
            Throws<InvalidDataException>(() => SessionFile.ReadStoryboards(changedAddress));
            Throws<ArgumentOutOfRangeException>(() => SessionFile.ReadStoryboards(path, double.NaN));
            Throws<ArgumentOutOfRangeException>(() => SessionFile.ReadStoryboards(path, -1));
            Throws<ArgumentOutOfRangeException>(() => SessionFile.ReadStoryboards(path, limit: 0));
            using var cancellation = new CancellationTokenSource();
            cancellation.Cancel();
            Throws<OperationCanceledException>(() => SessionFile.ReadStoryboards(path, cancellationToken: cancellation.Token));
            Console.WriteLine("Passed: storyboard time selection, lifecycle, restarts, partial history, address reuse, UInt64, limits and invalid records.");
        }
        finally
        {
            foreach (var file in Directory.EnumerateFiles(root)) File.Delete(file);
            Directory.Delete(root);
        }
    }

    private static ProfilerEvent Event(long ms, ulong id, ulong run, string operation, string state,
        string name = "", bool paused = false, bool independent = false, bool registered = true, ulong address = 0x1234) =>
        new(1 + ms * 1000, ms, 1, 7, XamlProvider.XamlProfiler, true, "StoryboardActivity", 0,
        [
            new("StoryboardId", id), new("ObjectAddress", address), new("RunId", run), new("Operation", operation),
            new("Name", name), new("State", state), new("IsPaused", paused),
            new("HasIndependentAnimation", independent), new("IsInTimeManager", registered), new("ChildCount", 2u)
        ]);

    private static void Write(string path, params ProfilerEvent[] events)
    {
        using var writer = SessionWriter.Create(path, new(1, "StoryboardTests", null, 1_000_000, 1, SessionCoverage.Trace));
        long sequence = 0;
        foreach (var value in events) writer.Process(++sequence, value);
        writer.Close("Test complete");
        if (writer.Failure is { } error) throw error;
    }

    private static void Check(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }

    private static void Throws<T>(Action action) where T : Exception
    {
        try { action(); }
        catch (T) { return; }
        throw new InvalidOperationException($"Expected {typeof(T).Name}.");
    }
}

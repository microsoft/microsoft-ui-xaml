// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Tests;

internal static class CompositionLifetimeTests
{
    private static readonly Guid VisualInterface = new("c0eeab6c-c897-5ac6-a1c9-63abd5055b9b");
    private static readonly Guid SurfaceInterface = new("bb8a4953-2c99-4f5a-96f5-4819027fa3ac");

    internal static void Run()
    {
        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-Composition-{Guid.NewGuid():N}.sqlite");
        var otherPath = Path.ChangeExtension(path, "other.sqlite");
        var trimmedPath = Path.ChangeExtension(path, "trimmed.sqlite");
        try
        {
            PairAndLink(path, otherPath);
            TrimAndReadLegacy(trimmedPath);
        }
        finally
        {
            foreach (var file in new[] { path, otherPath, trimmedPath })
            {
                foreach (var suffix in new[] { "", "-wal", "-shm" }) File.Delete(file + suffix);
            }
        }
    }

    private static void PairAndLink(string path, string otherPath)
    {
        const ulong pointer = 0xf123456789abcde0UL;
        const ulong implementation = pointer - 0x80;
        const uint surfacePointer = 0xf0000088;
        var first = Guid.NewGuid();
        var surface = Guid.NewGuid();
        var reused = Guid.NewGuid();
        var wrongIid = Guid.NewGuid();
        var legacy = Guid.NewGuid();
        var orphan = Guid.NewGuid();
        var duplicate = Guid.NewGuid();
        var missingEnd = Guid.NewGuid();
        var reusedAfterGap = Guid.NewGuid();
        var boundary = Guid.NewGuid();
        var invalidOrder = Guid.NewGuid();
        var references = new Dictionary<string, long>();
        var events = new List<ProfilerEvent>();
        void Add(ProfilerEvent item) => events.Add(item);
        void Observe(string name, ProfilerEvent item)
        {
            Add(item);
            references[name] = events.Count;
        }

        // Timestamp order, not delivery order, establishes the interval.
        Add(Destroyed(first, 40_000));
        Add(Created(first, 10_000, VisualInterface, pointer, implementation));
        Observe("first", Visual(20_000, unchecked((long)pointer)));
        Observe("implementation", Visual(25_000, implementation));
        Add(Created(surface, 10_000, SurfaceInterface.ToString().ToUpperInvariant(), surfacePointer));
        Observe("surface", Xaml(20_000, "DCompSurfaceInit",
            new("NativeSurfaceId", unchecked((int)surfacePointer)), new("SurfaceId", 0xabcL)));
        Observe("differentIid", Visual(20_000, surfacePointer));
        Add(Destroyed(surface, 40_000));
        Add(Created(reused, 50_000, VisualInterface, pointer));
        Observe("reused", Visual(60_000, pointer));
        Add(Destroyed(reused, 70_000));
        Observe("afterEnd", Visual(80_000, pointer));
        Add(Created(wrongIid, 10_000, Guid.NewGuid(), 0xdeadL));
        Observe("unknownIid", Visual(20_000, 0xdeadL));
        Add(Created(legacy, 10_000) with
        {
            Fields = [new("ObjectId", legacy.ToString("B").ToUpperInvariant()), new("TypeName", "OldVisual"), new("ObjectPointer", 0xbadf00dL)]
        });
        Add(Destroyed(legacy, 40_000));
        Add(Destroyed(orphan, 50_000));
        Add(Created(duplicate, 10_000, VisualInterface, 0xd00dL));
        Add(Created(duplicate, 15_000, VisualInterface, 0xd00dL));
        Observe("duplicate", Visual(20_000, 0xd00dL));
        Add(Destroyed(duplicate, 30_000));
        Add(Created(missingEnd, 80_000, VisualInterface, 0xbeefL));
        Observe("beforeReuse", Visual(85_000, 0xbeefL));
        Add(Created(reusedAfterGap, 90_000, VisualInterface, 0xbeefL));
        Observe("reuseBoundary", Visual(90_000, 0xbeefL));
        Observe("gap", Visual(100_000, 0xbeefL));
        Add(Destroyed(reusedAfterGap, 110_000));
        Add(Created(boundary, 120_000, VisualInterface, 0xb00bL));
        Observe("atCreation", Visual(120_000, 0xb00bL));
        Observe("inside", Visual(130_000, 0xb00bL));
        Observe("atDestruction", Visual(140_000, 0xb00bL));
        Add(Destroyed(boundary, 140_000));
        Add(Created(invalidOrder, 30_000, VisualInterface, 0xbadL));
        Add(Destroyed(invalidOrder, 20_000));
        Observe("invalidOrder", Visual(40_000, 0xbadL));
        Add(Destroyed(Guid.Empty, 50_000));
        Add(Destroyed(Guid.NewGuid(), 50_000) with { Fields = [] });
        Add(Destroyed(Guid.NewGuid(), 50_000) with { Fields = [new("ObjectId", "not a GUID")] });
        Add(Destroyed(Guid.NewGuid(), 50_000) with { Fields = [new("ObjectId", first), new("ObjectId", reused)] });
        Add(Created(Guid.NewGuid(), 10_000) with { TraceLogging = false });
        Add(Created(Guid.NewGuid(), 10_000) with { Name = "AnotherDCompEvent" });
        Add(Destroyed(first, 30_000) with { ProcessId = 2 });
        Observe("manifest", Visual(20_000, pointer) with { TraceLogging = false });
        Observe("diagnostics", Visual(20_000, pointer) with { Provider = XamlProvider.XamlDiagnostics });
        Observe("nullPointer", Visual(20_000, 0UL));

        Write(path, 1, events);
        var queries = new List<SessionQuery>();
        CompositionLifetimeSummary summary;
        IReadOnlyList<CompositionLifetime> lifetimes;
        IReadOnlyList<CompositionWinUIReference> links;
        using (SessionQueries.Observe(queries.Add))
        {
            summary = SessionFile.ReadCompositionSummary(path);
            lifetimes = SessionFile.ReadCompositionLifetimes(path);
            links = SessionFile.ReadCompositionReferences(path);
        }
        Mcp.QueryTests.RunAsReported(queries, "Composition lifetime reads");
        Check(summary is
        {
            CreatedObjects: 10, DestroyedObjects: 9, MatchedDestroyed: 6, OutstandingObserved: 2,
            UnmatchedDestructions: 1, AmbiguousLifetimes: 1, InvalidOrderLifetimes: 1, MalformedEvents: 4
        },
            $"Retained GUID counts must distinguish paired, missing, duplicate and malformed endpoints: {summary}");
        Check(summary.Session is { ProcessId: 1, LostByEtw: 2, DroppedByProfiler: 3 },
            "The native report must carry capture health, not turn missing events into leak claims.");
        var paired = lifetimes.Single(item => item.ObjectId == first);
        Check(paired is
        {
            Status: "Destroyed", ObjectPointer: "0xF123456789ABCD60",
            PublicInterfacePointer: "0xF123456789ABCDE0", CreatedEvents: 1, DestroyedEvents: 1
        } &&
            paired.CreatedSeq > paired.DestroyedSeq && paired.CreatedMilliseconds < paired.DestroyedMilliseconds,
            "Reversed delivery must preserve QPC ordering, full pointer bits, and non-primary public interface offsets.");
        Check(lifetimes.Single(item => item.ObjectId == legacy) is { Status: "Destroyed", PublicInterfacePointer: null },
            "Creation events without alias metadata still pair, with no fabricated public identity.");
        Check(Link("first").ObjectId == first && Link("reused").ObjectId == reused &&
            Link("beforeReuse").ObjectId == missingEnd && Link("inside").ObjectId == boundary,
            "A reference must link to the unique lifetime active at its event time, including pointer reuse.");
        var surfaceLink = links.Single(item => item.Sequence == references["surface"]);
        Check(surfaceLink is { Field: "NativeSurfaceId", Address: "0x00000000F0000088", SurfaceId: "0x0000000000000ABC" } &&
            surfaceLink.ObjectId == surface && surfaceLink.InterfaceId == SurfaceInterface,
            "Native surface pointers must use the exact surface IID and zero-extend signed 32-bit pointer bits.");
        foreach (var name in new[] { "implementation", "differentIid", "afterEnd", "unknownIid" })
        {
            Check(Link(name) is { Status: "Unmatched", ObjectId: null }, $"{name} must not be joined by an unrelated address.");
        }
        foreach (var name in new[] { "duplicate", "reuseBoundary", "gap", "atCreation", "atDestruction", "invalidOrder" })
        {
            Check(Link(name) is { Status: "Ambiguous", ObjectId: null }, $"{name} must not fabricate a unique lifetime.");
        }
        Check(Link("gap").Candidates == 2 && Link("reuseBoundary").Candidates == 2,
            "A missing destruction must keep later pointer reuse ambiguous, including an equal-time boundary.");
        Check(new[] { "manifest", "diagnostics", "nullPointer" }.All(name =>
                links.All(item => item.Sequence != references[name])),
            "Untrusted event layouts and null pointers must not create WinUI references.");
        Check(SessionFile.ReadObjects(path, ObjectKind.Visual).All(item => item.Address != 0xbadf00dL),
            "Native implementation pointers must not become WinUI visual identities.");

        Write(otherPath, 2, [Created(first, 10_000, VisualInterface, pointer) with { ProcessId = 2 }]);
        Check(SessionFile.ReadCompositionSummary(otherPath) is { CreatedObjects: 1, DestroyedObjects: 0, OutstandingObserved: 1 } &&
            SessionFile.ReadCompositionLifetimes(otherPath).Single().ObjectId == first,
            "Equal GUIDs and addresses in separate process recordings must not share endpoints.");
        Console.WriteLine("Passed: native GUID pairing, exact public-interface joins, pointer reuse, uncertainty and process isolation.");

        CompositionWinUIReference Link(string name) =>
            links.Single(item => item.Sequence == references[name] && item.Field == "VisualId");
    }

    private static void TrimAndReadLegacy(string path)
    {
        var old = Guid.NewGuid();
        var current = Guid.NewGuid();
        long sequence = 0;
        using (var writer = SessionWriter.Create(path, new(1, "CompositionTrim", null, 1_000_000, 1, SessionCoverage.Trace),
            limitBytes: 1024 * 1024))
        {
            writer.Process(++sequence, Created(old, 1_000, VisualInterface, 0x1234L));
            writer.Process(++sequence, Visual(2_000, 0x1234L));
            WaitForCommit(writer, sequence);
            for (var batch = 0; batch < 10; batch++)
            {
                for (var i = 0; i < 10; i++)
                {
                    writer.Process(++sequence, Xaml(sequence * 1_000, "Padding", new EventField("Text", new string('x', 16_000))));
                }
                WaitForCommit(writer, sequence);
            }
            Check(SessionFile.ReadInfo(path).CompleteFromSeq > 2, "The fixture must exercise actual rolling retention.");
            writer.Process(++sequence, Destroyed(old, sequence * 1_000));
            writer.Process(++sequence, Visual(sequence * 1_000, 0x1234L));
            writer.Process(++sequence, Created(current, sequence * 1_000, VisualInterface, 0x5678L));
            writer.Process(++sequence, Visual(sequence * 1_000, 0x5678L));
            writer.Close("Test ended", lostByEtw: 0, droppedByProfiler: 0);
            Check(writer.Failure is null, $"The trimmed native recording must be written: {writer.Failure}");
        }
        var summary = SessionFile.ReadCompositionSummary(path);
        Check(summary is
        {
            CreatedObjects: 1, MatchedDestroyed: 0, OutstandingObserved: 1, UnmatchedDestructions: 1,
            WinUILinkedObjects: 1, UnmatchedReferences: 1
        } && summary.Session.CompleteFromSeq > 2,
            "Trimming a creation must leave an orphan destruction and no stale pointer link.");
        using (var connection = Open(path))
        using (var command = connection.CreateCommand())
        {
            command.CommandText = """
                SELECT (SELECT COUNT(*) FROM CompositionObjectEvents c LEFT JOIN Events e ON e.Seq = c.Seq WHERE e.Seq IS NULL)
                    + (SELECT COUNT(*) FROM CompositionReferences c LEFT JOIN Events e ON e.Seq = c.Seq WHERE e.Seq IS NULL)
                """;
            Check((long)command.ExecuteScalar()! == 0, "Trimming must remove native evidence atomically with raw events.");
            command.CommandText = "DROP VIEW CompositionLifetimeSummary; DROP VIEW CompositionWinUILinks; DROP VIEW CompositionObjectLifetimes;";
            command.ExecuteNonQuery();
        }
        Check(SessionFile.ReadInfo(path).ProcessId == 1 && SessionFile.ReadFrames(path).Count == 0,
            "A readable older recording must remain readable for existing features.");
        foreach (var read in new Action[]
        {
            () => SessionFile.ReadCompositionSummary(path), () => SessionFile.ReadCompositionLifetimes(path),
            () => SessionFile.ReadCompositionReferences(path)
        })
        {
            try
            {
                read();
                throw new InvalidOperationException("An older recording must not silently report zero native objects.");
            }
            catch (InvalidDataException error) when (error.Message.Contains("Record it again", StringComparison.Ordinal))
            {
            }
        }
        Console.WriteLine("Passed: native retention removes stale evidence; legacy recordings stay readable with explicit feature detection.");
    }

    private static void Write(string path, int processId, IReadOnlyList<ProfilerEvent> events)
    {
        using var writer = SessionWriter.Create(path,
            new(processId, "CompositionTests", null, 1_000_000, 1, SessionCoverage.Trace, LauncherProcessId: 2));
        for (var i = 0; i < events.Count; i++) writer.Process(i + 1, events[i]);
        writer.Close("Test ended", lostByEtw: 2, droppedByProfiler: 3);
        Check(writer.Failure is null, $"The native recording must be written: {writer.Failure}");
    }

    private static ProfilerEvent Created(Guid id, long qpc, object? iid = null, object? pointer = null, object? implementation = null) =>
        new(qpc, qpc / 1000.0, 1, 1, XamlProvider.DComp, true, "CompositionObjectCreated", 0,
            iid is null ? [new("ObjectId", id)]
                : [new("ObjectId", id), new("TypeName", "TestVisual"), new("ObjectPointer", implementation ?? pointer),
                    new("PublicInterfaceId", iid), new("PublicInterfacePointer", pointer)]);

    private static ProfilerEvent Destroyed(Guid id, long qpc) =>
        new(qpc, qpc / 1000.0, 1, 1, XamlProvider.DComp, true, "CompositionObjectDestroyed", 0, [new("ObjectId", id)]);

    private static ProfilerEvent Visual(long qpc, object pointer) => Xaml(qpc, "WucVisualRootSet", new EventField("VisualId", pointer));

    private static ProfilerEvent Xaml(long qpc, string name, params EventField[] fields) =>
        new(qpc, qpc / 1000.0, 1, 1, XamlProvider.XamlProfiler, true, name, 0, fields);

    private static void WaitForCommit(SessionWriter writer, long sequence)
    {
        var started = Stopwatch.GetTimestamp();
        while (Stopwatch.GetElapsedTime(started) < TimeSpan.FromSeconds(10))
        {
            Check(writer.Failure is null, $"The writer must keep running: {writer.Failure}");
            using var connection = Open(writer.Path);
            using var command = connection.CreateCommand();
            command.CommandText = "SELECT COALESCE(MAX(Seq), 0) FROM Events";
            if ((long)command.ExecuteScalar()! >= sequence) return;
            Thread.Sleep(20);
        }
        throw new TimeoutException("The native retention fixture did not commit its batch.");
    }

    private static SqliteConnection Open(string path)
    {
        var connection = new SqliteConnection(new SqliteConnectionStringBuilder { DataSource = path, Pooling = false }.ToString());
        connection.Open();
        return connection;
    }

    private static void Check(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
}

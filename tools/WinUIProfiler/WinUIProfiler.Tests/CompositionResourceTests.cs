// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text.Json;
using Microsoft.Data.Sqlite;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Cli;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;

internal static class CompositionResourceTests
{
    private const ulong NativePointer = 0xfedcba9876543210;
    private const ulong PublicPointer = 0xffff800000010001;

    internal static void Run()
    {
        ProviderSelection();
        Attribution(reverse: false);
        Attribution(reverse: true);
        ReuseAndMissingRelease();
        MissingAndMalformedEvidence();
        AmbiguityAndDuplicates();
        RollingRetention();
        Console.WriteLine("Passed: trace-only Composition resource attribution, exact identities, release endpoints, " +
            "reuse, ambiguity, legacy/malformed evidence, provider filters, CLI queries and rolling retention.");
    }

    private static void ProviderSelection()
    {
        Expect(XamlProviders.IsBuiltIn(XamlProviders.DirectComposition) &&
            XamlProviders.IsBuiltIn(XamlProviders.CompositionDiagnostics) &&
            !XamlProviders.XamlIds.Contains(XamlProviders.CompositionDiagnostics),
            "Native evidence must be built in without enabling its telemetry keywords through the XAML provider loop.");
        Expect(XamlEventSubscription.IsCompositionEvent(XamlProviders.DirectComposition, 1, false, "AnyManifestName") &&
            XamlEventSubscription.IsCompositionEvent(XamlProviders.DirectComposition, 30, false, "AnyManifestName") &&
            !XamlEventSubscription.IsCompositionEvent(XamlProviders.DirectComposition, 2, false, "Creation") &&
            !XamlEventSubscription.IsCompositionEvent(XamlProviders.DirectComposition, 1, true, "Creation") &&
            XamlEventSubscription.IsCompositionEvent(XamlProviders.CompositionDiagnostics, 0, true, "CompositionResourceIdentity") &&
            !XamlEventSubscription.IsCompositionEvent(XamlProviders.CompositionDiagnostics, 0, true, "OtherTelemetry") &&
            !XamlEventSubscription.IsCompositionEvent(XamlProviders.XamlProfiler, 1, false, "CompositionResourceIdentity"),
            "Only the native creation/release IDs and the self-describing identity event may be delivered.");

        foreach (var original in new[]
        {
            new TraceEventProviderOptions { ProcessIDFilter = [42], EventIDsToEnable = [7] },
            new TraceEventProviderOptions { ProcessNameFilter = ["Probe.exe"], EventIDsToEnable = [7] }
        })
        {
            List<(Guid Provider, TraceEventProviderOptions Options)> enabled = [];
            LiveEtwSource.EnableComposition((provider, options) => enabled.Add((provider, options)), original);
            Expect(enabled.Count == 2 && enabled[0].Provider == XamlProviders.DirectComposition &&
                enabled[0].Options.EventIDsToEnable is { } ids && ids.SequenceEqual([1, 30]) &&
                enabled[1].Provider == XamlProviders.CompositionDiagnostics && enabled[1].Options.EventIDsToEnable is null &&
                original.EventIDsToEnable!.SequenceEqual([7]) &&
                enabled.All(item => item.Options.ProcessIDFilter == original.ProcessIDFilter &&
                    item.Options.ProcessNameFilter == original.ProcessNameFilter),
                "Native provider enablement must preserve PID/name filters and never mutate the XAML filters.");
        }
        var attempts = 0;
        LiveEtwSource.EnableComposition((_, _) =>
        {
            ++attempts;
            if (attempts == 1) throw new UnauthorizedAccessException("Test provider unavailable.");
        }, new());
        Expect(attempts == 2, "One unavailable native provider must not abort XAML capture or prevent the other attempt.");
    }

    private static void Attribution(bool reverse)
    {
        ProfilerEvent[] events =
        [
            Create(10, 1), Identity(11, 1), WinUI(12), Release(20, 1),
            Create(10, 2, pointer: 200), Identity(11, 2, pointer: 200, publicId: 201), Release(19, 2, pointer: 200),
            WinUI(13, publicId: 201) with { ProcessId = 99 },
            WinUI(14) with { Fields = [new("ResourceKind", 17U), new("ResourceName", "WinUI.DCompSurfaceWrapper"), new("HasLifetime", true)] }
        ];
        WithRecording(reverse ? events.Reverse() : events, connection =>
        {
            Expect(Count(connection, "CompositionResourceLifetimes", "ResourceState = 'Released'") == 2 &&
                Count(connection, "CompositionResourceLinks", "LinkStatus = 'Matched' AND ResourceState = 'Released'") == 1 &&
                Scalar(connection, "SELECT CreatedObservations FROM CompositionResourceSummary") == 1 &&
                Scalar(connection, "SELECT ReleasedResources FROM CompositionResourceSummary") == 1,
                "App-only resources on the same channel, other processes and WinUI wrappers must not count as WinUI-native releases.");
            Expect(Count(connection, "CompositionResourceLinks",
                $"ResourcePointer = '{Hex(NativePointer)}' AND PublicObjectId = '{Hex(PublicPointer)}'") == 1,
                "The public COM identity and native resource owner must remain distinct full-width pointers.");
            Expect(Count(connection, "Objects") == 0, "Canonical IUnknown must not be indexed as an IVisual address.");

            var result = SessionFile.ReadCompositionResources(connection.DataSource, CompositionResourceView.Links, count: -1);
            Expect(result.Rows.Count == 1 && result.Session.ProcessId == 42 && result.NextOffset is null,
                "The shared reader must expose the same process-local links.");
            using var output = new StringWriter();
            using var error = new StringWriter();
            Expect(CliApplication.Run(["composition-resources", connection.DataSource, "--view", "links", "--json"],
                output, error, default) == 0, $"The CLI must read shared resource links: {error}");
            using var json = JsonDocument.Parse(output.ToString());
            Expect(json.RootElement.GetProperty("Result").GetProperty("Rows").GetArrayLength() == 1 &&
                output.ToString().Contains(Hex(PublicPointer), StringComparison.Ordinal) &&
                output.ToString().Contains("not final COM destruction", StringComparison.Ordinal),
                "CLI JSON must preserve exact pointers and state the resource-release limitation.");
            Expect(CliApplication.Run(["composition-resources", connection.DataSource, "--view", "invalid"],
                new StringWriter(), new StringWriter(), default) == 2, "Unknown views must be rejected.");
        });
    }

    private static void ReuseAndMissingRelease()
    {
        WithRecording(
        [
            Create(10, 1), Identity(11, 1), WinUI(12), Release(20, 1),
            Create(30, 1), Identity(31, 1), WinUI(32), Release(40, 1),
            Create(50, 1, pointer: 300), Identity(51, 1, pointer: 300, publicId: 301), WinUI(52, publicId: 301),
            Create(60, 1, pointer: 400), Identity(61, 1, pointer: 400, publicId: 401), WinUI(62, publicId: 401),
            Release(70, 1, pointer: 400)
        ], connection =>
        {
            Expect(Count(connection, "CompositionResourceLifetimes") == 4 &&
                Count(connection, "CompositionResourceLifetimes", "Generation = 2 AND CreateQpc = 30 AND ReleaseQpc = 40") == 1 &&
                Count(connection, "CompositionResourceLinks", "LinkStatus = 'Matched'") == 4 &&
                Scalar(connection, "SELECT ReleasedResources FROM CompositionResourceSummary") == 3 &&
                Scalar(connection, "SELECT ReleaseNotObserved FROM CompositionResourceSummary") == 1,
                "Reused handles/public/native pointers must form separate generations, not cumulative pointer lifetimes.");
            Expect(Count(connection, "CompositionResourceLinks", "ResourceState = 'HandleReuse' AND ReleaseQpc IS NULL AND EndQpc = 60") == 1,
                "A reused handle with a missing release is only an upper bound, not an observed destruction.");
        });
    }

    private static void MissingAndMalformedEvidence()
    {
        WithRecording(
        [
            Create(10, 1), Identity(11, 1), WinUI(12),
            Release(20, 1, pointer: 999),
            Release(21, 1, channel: 999),
            WinUI(22, publicId: 700),
            WinUI(23) with { Fields = [new("ResourceKind", 1U), new("ResourceName", "Composition.SpriteVisual"), new("HasLifetime", false)] },
            WinUI(24, publicId: 0),
            WinUI(25) with { Fields = [new("ResourceKind", 11U), new("ResourceName", "Composition.Surface"),
                new("HasLifetime", false), new("PublicObjectId", 800UL)] },
            Identity(26, 2, publicId: 800),
            Release(27, 2),
            Create(28, 0),
            Create(29, 3) with { Version = 2 },
            Identity(30, 3) with { Fields = [new("ChannelId", 9U), new("ResourceHandle", 3U),
                new("ResourcePointer", 123.0), new("PublicObjectId", 801UL)] },
            Identity(31, 4) with { Fields = [new("ChannelId", 9U), new("ResourceHandle", 4U),
                new("ResourceHandle", 5U), new("ResourcePointer", NativePointer), new("PublicObjectId", 802UL)] },
            new(32, 0.032, 42, 7, XamlProvider.Xaml, true, "AfterMalformed", 0, [])
        ], connection =>
        {
            Expect(Count(connection, "CompositionResourceLifetimes") == 1 &&
                Count(connection, "CompositionResourceLinks", "LinkStatus = 'Matched' AND ResourceState = 'NoReleaseObserved'") == 1 &&
                Count(connection, "CompositionResourceLinks", "LinkStatus = 'MissingPublicIdentity'") == 2 &&
                Count(connection, "CompositionResourceLinks", "LinkStatus = 'NoIdentityMatch'") == 2,
                "Legacy/zero identities, unsupported surface associations and release-only evidence must remain explicit.");
            Expect(Count(connection, "CompositionResourceEvidence", "InputStatus = 'MalformedNativeIdentity'") == 3 &&
                Count(connection, "CompositionResourceEvidence", "InputStatus = 'UnsupportedNativeVersion'") == 1 &&
                Count(connection, "EventDetails", "Name = 'AfterMalformed'") == 1,
                "Malformed/duplicate fields or unsupported native versions must never abort the raw event batch.");
            Expect(Count(connection, "CompositionResourceLifetimes", "ReleaseQpc IS NOT NULL") == 0,
                "A slot pointer or channel-pointer representation must not close an ID-based WinRT resource.");
            Sql.Execute(connection, "UPDATE SessionInfo SET LostByEtw = 3, DroppedByProfiler = 4");
            var info = SessionFile.ReadCompositionResources(connection.DataSource, CompositionResourceView.Summary).Session;
            Expect(info.LostByEtw == 3 && info.DroppedByProfiler == 4, "Resource reports must retain recording-loss context.");
            Expect(SessionFile.ReadCompositionResources(connection.DataSource, CompositionResourceView.Links, 1, 2).NextOffset == 3,
                "Resource links must support bounded paging.");

            Sql.Execute(connection, "DROP VIEW CompositionResourceSummary");
            using var error = new StringWriter();
            Expect(CliApplication.Run(["composition-resources", connection.DataSource], new StringWriter(), error, default) == 3 &&
                error.ToString().Contains("predates the Composition resource index", StringComparison.Ordinal),
                "Older session files must report unsupported indexing, not measured zero resources.");
        });
    }

    private static void AmbiguityAndDuplicates()
    {
        WithRecording(
        [
            Create(10, 1), Create(10, 2, pointer: 200),
            Identity(11, 1), Identity(11, 2, pointer: 200), WinUI(12),
            Release(20, 1), Release(20, 2, pointer: 200)
        ], connection =>
        {
            Expect(Count(connection, "CompositionResourceLinks", "LinkStatus = 'AmbiguousIdentity' AND CandidateCount = 2 AND ResourceId IS NULL") == 1 &&
                Scalar(connection, "SELECT ReleasedResources FROM CompositionResourceSummary") == 0,
                "An ambiguous public identity must not pick one resource or double-count releases.");
        });
        WithRecording([Create(10, 1), Identity(11, 1), WinUI(12), WinUI(13), Release(20, 1)], connection =>
        {
            Expect(Count(connection, "CompositionResourceLinks", "LinkStatus = 'DuplicateWinUICreation' AND ResourceId IS NULL") == 2,
                "Repeated WinUI creation observations cannot claim two objects from one native generation.");
        });
        WithRecording(
        [
            Create(10, 1), Create(10, 1, pointer: 200), Identity(11, 1, pointer: 200),
            WinUI(12), Release(20, 1, pointer: 200)
        ], connection => Expect(Count(connection, "CompositionResourceLinks", "LinkStatus = 'AmbiguousCreation'") == 1,
            "Simultaneous handle generations must remain ambiguous instead of relying on buffer arrival order."));
        WithRecording(
        [
            Create(10, 1), Identity(11, 1), Identity(12, 1), WinUI(13), Release(20, 1), Release(21, 1)
        ], connection => Expect(Count(connection, "CompositionResourceLinks", "LinkStatus = 'Matched'") == 1 &&
            Scalar(connection, "SELECT ReleasedResources FROM CompositionResourceSummary") == 1,
            "Repeated stateless identity observations and release events must not create extra lifetimes."));
    }

    private static void RollingRetention()
    {
        foreach (var limitBytes in new[] { 1024 * 1024, 4 * 1024 * 1024 })
        {
            WithRecording(Enumerable.Range(1, 20_000).Select(index =>
                Identity(index, (uint)index, pointer: (ulong)index, publicId: (ulong)index)), connection =>
            {
                Expect(Scalar(connection, "SELECT CompleteFromSeq FROM SessionInfo") > 1 &&
                    Count(connection, "CompositionResourceEvidence") > 0 &&
                    Count(connection, "CompositionResourceEvidence") < 20_000 &&
                    Scalar(connection, """
                        SELECT COUNT(*) FROM CompositionResourceEvidence c
                        WHERE NOT EXISTS (SELECT 1 FROM Events e WHERE e.Seq = c.Seq)
                        """) == 0, "Rolling retention must trim normalized evidence with raw events, leaving no unbounded object metadata.");
                Expect(Scalar(connection, "PRAGMA page_count") * Scalar(connection, "PRAGMA page_size") <= limitBytes,
                    "Batches including normalized evidence must fit the recording limit, including at its 1 MiB minimum.");
                Expect(Count(connection, "CompositionResourceLifetimes") == 0 &&
                    Count(connection, "CompositionResourceSummary") == 0, "Identity-only evidence must not fabricate resource creations.");
            }, limitBytes);
        }
    }

    internal static void ValidateTrace(string path, int processId)
    {
        var events = new List<ProfilerEvent>();
        OfflineEtlSource.Read(path, processId, [], events.Add);
        Expect(events.Any(data => data.Provider == XamlProvider.CompositionDiagnostics) &&
            events.Any(data => data.Provider == XamlProvider.DirectComposition && data.EventId == 1) &&
            events.Any(data => data.Provider == XamlProvider.DirectComposition && data.EventId == 30) &&
            events.Any(data => data.Provider == XamlProvider.XamlProfiler && data.Name == "GraphicsResourceCreated"),
            "Integration requires real WinUI creation, native creation, identity and native release events.");
        WithRecording(events, connection =>
        {
            Expect(Count(connection, "CompositionResourceLinks", "LinkStatus = 'MissingPublicIdentity'") == 0,
                "The current WinUI producer must pass its created interface at every observed Composition creation site, including handoff visuals.");
            Expect(Count(connection, "CompositionResourceLinks", "LinkStatus = 'Matched'") > 0 &&
                Count(connection, "CompositionResourceLinks", "LinkStatus = 'Matched' AND ResourceState = 'Released'") > 0,
                "The real cross-runtime trace must correlate and release WinUI-attributed resources.");
        }, processId: processId);
        Console.WriteLine("Passed: real WinUI/native ETW resource identities and matched resource releases.");
    }

    private static ProfilerEvent Create(long qpc, uint handle, ulong pointer = NativePointer, uint channel = 9) =>
        new(qpc, qpc / 1000.0, 42, 7, XamlProvider.DirectComposition, false, "ResourceCreation", 0,
            [new("channelHandle", channel), new("resourceHandle", handle), new("resourcePointer", pointer),
                new("resourceType", 123U), new("createShared", false), new("openShared", false)], EventId: 1, Version: 0);

    private static ProfilerEvent Release(long qpc, uint handle, ulong pointer = NativePointer, uint channel = 9) =>
        new(qpc, qpc / 1000.0, 42, 7, XamlProvider.DirectComposition, false, "ResourceRelease", 0,
            [new("channelHandle", channel), new("resourceHandle", handle), new("resourcePointer", pointer)], EventId: 30, Version: 0);

    private static ProfilerEvent Identity(long qpc, uint handle, ulong pointer = NativePointer, ulong publicId = PublicPointer) =>
        new(qpc, qpc / 1000.0, 42, 7, XamlProvider.CompositionDiagnostics, true, "CompositionResourceIdentity", 0,
            [new("ChannelId", 9U), new("ResourceHandle", handle), new("ResourcePointer", pointer), new("PublicObjectId", publicId)],
            EventId: 0, Version: 0);

    private static ProfilerEvent WinUI(long qpc, ulong publicId = PublicPointer) =>
        new(qpc, qpc / 1000.0, 42, 7, XamlProvider.XamlProfiler, true, "GraphicsResourceCreated", 0,
            [new("ResourceKind", 1U), new("ResourceName", "Composition.SpriteVisual"), new("HasLifetime", false),
                new("PublicObjectId", publicId)], EventId: 0, Version: 0);

    private static void WithRecording(IEnumerable<ProfilerEvent> events, Action<SqliteConnection> check,
        long limitBytes = SessionWriter.DefaultLimitBytes, int processId = 42)
    {
        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-CompositionResources-{Guid.NewGuid():N}.sqlite");
        try
        {
            var writer = SessionWriter.Create(path, new(processId, "CompositionResources", null, 1_000_000, 0, SessionCoverage.Trace),
                limitBytes);
            using (writer)
            {
                long sequence = 0;
                foreach (var data in events) writer.Process(++sequence, data);
            }
            Expect(writer.Failure is null, $"Resource indexing must finish its raw event transactions: {writer.Failure}");
            using var connection = new SqliteConnection(new SqliteConnectionStringBuilder { DataSource = path, Pooling = false }.ToString());
            connection.Open();
            check(connection);
        }
        finally
        {
            foreach (var file in new[] { path, path + "-wal", path + "-shm" }) File.Delete(file);
        }
    }

    private static long Count(SqliteConnection connection, string table, string where = "1") =>
        Scalar(connection, $"SELECT COUNT(*) FROM {table} WHERE {where}");
    private static long Scalar(SqliteConnection connection, string sql) => Sql.Scalar(connection, sql);
    private static string Hex(ulong value) => $"0x{value:x16}";
    private static void Expect(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
}

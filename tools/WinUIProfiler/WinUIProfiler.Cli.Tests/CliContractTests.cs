using System.Diagnostics;
using System.Text.Json;
using System.Text.Json.Serialization;
using WinUIProfiler.Cli;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine;
using WinUIProfiler.Engine.Storage;
using Xunit;
using Xunit.Abstractions;

namespace WinUIProfiler.Cli.Tests;

public sealed class CliContractTests(ITestOutputHelper log)
{
    private static readonly string[] Commands =
    [
        "session", "metrics", "startup", "frames", "object-history", "xaml-tree",
        "composition-tree", "correlations", "resources", "snapshots", "compare", "retention"
    ];

    [Fact]
    public async Task Subprocess_session_and_metrics_preserve_schema_identity_quality_and_committed_paging()
    {
        using var f = new CliFixture();
        using (var store = f.Create()) store.CreateWriter().Commit(f.Batch(1, 3));
        var session = await Execute("session", "--db", f.Path);
        Assert.Equal(0, session.Code);
        Assert.Equal("", session.Error);
        using var info = JsonDocument.Parse(session.Output);
        AssertEnvelope(info.RootElement, "session", f);
        Assert.Equal(1_000_000, info.RootElement.GetProperty("result").GetProperty("value").GetProperty("qpcFrequency").GetInt64());

        var first = await Execute("metrics", "--db", f.Path, "--start-qpc", "0", "--end-qpc", "31", "--limit", "2");
        Assert.Equal(0, first.Code);
        using var page = JsonDocument.Parse(first.Output);
        AssertEnvelope(page.RootElement, "metrics", f);
        var result = page.RootElement.GetProperty("result");
        Assert.Equal(2, result.GetProperty("items").GetArrayLength());
        Assert.True(result.GetProperty("page").GetProperty("hasMore").GetBoolean());
        Assert.Equal(2, result.GetProperty("page").GetProperty("nextOffset").GetInt64());
        Assert.Equal(3, result.GetProperty("context").GetProperty("watermark").GetProperty("ingestOrdinal").GetInt64());
        using (var writer = SessionStore.OpenWritable(f.Path)) writer.CreateWriter().Commit(f.Batch(4, 4));
        var next = await Execute("metrics", "--db", f.Path, "--start-qpc", "0", "--end-qpc", "31",
            "--limit", "2", "--offset", "2", "--watermark-ordinal", "3", "--watermark-qpc", "30");
        Assert.Equal(0, next.Code);
        using var continued = JsonDocument.Parse(next.Output);
        Assert.Single(continued.RootElement.GetProperty("result").GetProperty("items").EnumerateArray());
        Assert.Equal(result.GetProperty("context").GetRawText(),
            continued.RootElement.GetProperty("result").GetProperty("context").GetRawText());
    }

    [Fact]
    public async Task Every_route_uses_persisted_queries_without_creating_unobserved_data_or_mutating_files()
    {
        using var f = new CliFixture();
        using (var store = f.Create()) store.CreateWriter().Commit(f.Batch(1, 1));
        var before = Files(f.Path);
        foreach (var command in Commands)
        {
            var args = new List<string> { command, "--db", f.Path };
            if (command is "metrics" or "frames" or "object-history" or "resources")
                args.AddRange(["--start-qpc", "0", "--end-qpc", "11"]);
            if (command is "xaml-tree" or "composition-tree" or "correlations") args.AddRange(["--at-qpc", "10"]);
            if (command is "object-history" or "correlations" or "retention")
                args.AddRange(["--domain", "XamlCore", "--producer-epoch", f.Epoch.ToString(), "--runtime-id", ulong.MaxValue.ToString()]);
            if (command == "compare") args.AddRange(["--baseline-id", Guid.NewGuid().ToString(), "--snapshot-ids", Guid.NewGuid().ToString()]);
            if (command == "retention") args.AddRange(["--snapshot-id", Guid.NewGuid().ToString(), "--maximum-depth", "10"]);
            var response = await Execute(args.ToArray());
            if (command is "compare" or "retention")
            {
                Assert.Equal(4, response.Code);
                using var error = JsonDocument.Parse(response.Error);
                Assert.Equal("unsupported_query", error.RootElement.GetProperty("error").GetProperty("code").GetString());
                Assert.Equal("", response.Output);
            }
            else
            {
                Assert.Equal(0, response.Code);
                using var json = JsonDocument.Parse(response.Output);
                AssertEnvelope(json.RootElement, command, f);
                if (command is not ("session" or "metrics"))
                {
                    var result = json.RootElement.GetProperty("result");
                    Assert.Empty(result.GetProperty("items").EnumerateArray());
                    Assert.Equal("Unavailable", result.GetProperty("context").GetProperty("quality").GetProperty("kind").GetString());
                }
            }
        }
        AssertFiles(before, Files(f.Path));
    }

    public static IEnumerable<object[]> InvalidArguments()
    {
        string[][] cases =
        [
            [], ["sql"], ["target"], ["capture"], ["session"], ["session", "--db"],
            ["session", "--db", "unused", "--db", "again"],
            ["session", "--db", "unused", "--sql", "SELECT 1"],
            ["session", "--db", "unused", "--pid", "42"],
            ["session", "--db", "unused", "--limit", "1"],
            ["metrics", "--db", "unused", "--start-qpc", "0"],
            ["metrics", "--db", "unused", "--start-qpc", "-1", "--end-qpc", "1"],
            ["metrics", "--db", "unused", "--start-qpc", "10", "--end-qpc", "1"],
            ["metrics", "--db", "unused", "--start-qpc", "0", "--end-qpc", "1", "--limit", "4097"],
            ["metrics", "--db", "unused", "--start-qpc", "0", "--end-qpc", "1", "--offset", "1"],
            ["metrics", "--db", "unused", "--start-qpc", "0", "--end-qpc", "1", "--metric", "0"],
            ["metrics", "--db", "unused", "--start-qpc", "0", "--end-qpc", "1", "--metric", "Bogus"],
            ["metrics", "--db", "unused", "--start-qpc", "0", "--end-qpc", "1", "--watermark-ordinal", "1"],
            ["xaml-tree", "--db", "unused", "--at-qpc", "1", "--domain", "Resource"],
            ["correlations", "--db", "unused", "--at-qpc", "1", "--domain", "XamlCore", "--producer-epoch", Guid.Empty.ToString(), "--runtime-id", "1"],
            ["correlations", "--db", "unused", "--at-qpc", "1", "--domain", "XamlCore", "--producer-epoch", Guid.NewGuid().ToString(), "--runtime-id", "0"],
            ["session", "--db", new string('x', 4097)],
            ["session", "--db", @"\\server\share\session.sqlite"],
            ["session", "--db", "https://example.invalid/session.sqlite"],
            ["metrics", "--db", "unused", "--start-qpc", "0", "--end-qpc", "9223372036854775808"],
            ["metrics", "--db", "unused", "--start-qpc", "0", "--end-qpc", "1", "--limit", "0"],
            ["metrics", "--db", "unused", "--start-qpc", "0", "--end-qpc", "1", "--offset", "9223372036854775807"],
            ["metrics", "--db", "unused", "--start-qpc", "0", "--end-qpc", "1", "--offset", "-1"],
            ["snapshots", "--db", "unused", "--kind", "0"],
            ["snapshots", "--db", "unused", "--kind", "Unknown"],
            ["retention", "--db", "unused", "--snapshot-id", Guid.NewGuid().ToString(), "--maximum-depth", "257",
                "--domain", "XamlCore", "--producer-epoch", Guid.NewGuid().ToString(), "--runtime-id", "1"],
            ["correlations", "--db", "unused", "--at-qpc", "1", "--domain", "0",
                "--producer-epoch", Guid.NewGuid().ToString(), "--runtime-id", "1"],
            ["correlations", "--db", "unused", "--at-qpc", "1", "--domain", "XamlCore",
                "--producer-epoch", Guid.NewGuid().ToString(), "--runtime-id", "18446744073709551616"],
            ["compare", "--db", "unused", "--baseline-id", Guid.Empty.ToString(), "--snapshot-ids", Guid.NewGuid().ToString()],
            ["compare", "--db", "unused", "--baseline-id", Guid.NewGuid().ToString(), "--snapshot-ids",
                string.Join(',', Enumerable.Range(0, 65).Select(_ => Guid.NewGuid()))]
        ];
        return cases.Select(args => new object[] { args });
    }

    [Theory]
    [MemberData(nameof(InvalidArguments))]
    public async Task Subprocess_rejects_invalid_and_unbounded_input_before_opening_a_database(string[] args)
    {
        var response = await Execute(args);
        Assert.Equal(2, response.Code);
        Assert.Equal("", response.Output);
        using var error = JsonDocument.Parse(response.Error);
        Assert.Equal(1, error.RootElement.GetProperty("schemaVersion").GetInt32());
        Assert.Equal("invalid_arguments", error.RootElement.GetProperty("error").GetProperty("code").GetString());
        Assert.DoesNotContain(" at ", response.Error);
    }

    [Fact]
    public async Task Missing_database_is_not_created()
    {
        using var f = new CliFixture();
        var response = await Execute("session", "--db", f.Path);
        Assert.Equal(3, response.Code);
        Assert.Equal("", response.Output);
        Assert.Empty(Files(f.Path));
        using var error = JsonDocument.Parse(response.Error);
        Assert.Equal("database_error", error.RootElement.GetProperty("error").GetProperty("code").GetString());
    }

    [Fact]
    public async Task Cancellation_before_open_does_not_create_files_or_report_success()
    {
        using var f = new CliFixture();
        using var output = new StringWriter();
        using var error = new StringWriter();
        var code = await CliApplication.RunAsync(["metrics", "--db", f.Path, "--start-qpc", "0", "--end-qpc", "10"],
            output, error, new CancellationToken(canceled: true));
        Assert.Equal(6, code);
        Assert.Equal("", output.ToString());
        using var json = JsonDocument.Parse(error.ToString());
        Assert.Equal("cancelled", json.RootElement.GetProperty("error").GetProperty("code").GetString());
        Assert.Empty(Files(f.Path));
    }

    [Fact]
    public async Task Typed_object_identity_is_bound_to_saved_process_and_filters_epoch_domain_and_unsigned_id()
    {
        using var f = new CliFixture();
        var key = new ObjectKey(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, ulong.MaxValue);
        using (var store = f.Create())
        {
            var history = new ObjectHistoryEntry(key, 10, 1,
                new LifetimeObservation(LifetimeOperation.Birth, null, null),
                new(CauseKind.OutsideAttempt, null, null), CliFixture.Partial);
            store.CreateWriter().Commit(f.Batch(1, 1) with { Changes = [new(1, history)] });
        }
        var args = new[] { "object-history", "--db", f.Path, "--start-qpc", "10", "--end-qpc", "11",
            "--domain", "XamlCore", "--producer-epoch", f.Epoch.ToString(), "--runtime-id", ulong.MaxValue.ToString() };
        var matching = await Execute(args);
        Assert.Equal(0, matching.Code);
        using var json = JsonDocument.Parse(matching.Output);
        var row = Assert.Single(json.RootElement.GetProperty("result").GetProperty("items").EnumerateArray());
        var identity = row.GetProperty("object");
        Assert.Equal(f.Scope.Process.InstanceId, identity.GetProperty("process").GetProperty("instanceId").GetGuid());
        Assert.Equal(ulong.MaxValue, identity.GetProperty("runtimeObjectId").GetUInt64());
        Assert.Equal(f.Epoch, identity.GetProperty("producerEpoch").GetGuid());
        Assert.Equal("XamlCore", identity.GetProperty("domain").GetString());
        args[8] = "XamlPeer";
        var otherDomain = await Execute(args);
        Assert.Equal(0, otherDomain.Code);
        using var empty = JsonDocument.Parse(otherDomain.Output);
        Assert.Empty(empty.RootElement.GetProperty("result").GetProperty("items").EnumerateArray());
        args[8] = "XamlCore";
        args[10] = Guid.NewGuid().ToString();
        var otherEpoch = await Execute(args);
        Assert.Equal(0, otherEpoch.Code);
        using var absent = JsonDocument.Parse(otherEpoch.Output);
        Assert.Empty(absent.RootElement.GetProperty("result").GetProperty("items").EnumerateArray());
    }

    [Fact]
    public async Task Every_typed_route_forwards_cancellation_to_the_real_query_service()
    {
        using var f = new CliFixture();
        using (f.Create()) { }
        using var store = SessionStore.OpenSavedReadOnly(f.Path);
        foreach (var name in Commands)
        {
            var invocation = CliArguments.Parse(RouteArguments(name, f));
            var request = invocation.Bind(store.Scope);
            var exception = await Assert.ThrowsAnyAsync<OperationCanceledException>(
                async () => await CliApplication.Route(store.Queries, request, new CancellationToken(canceled: true)));
            Assert.True(exception.CancellationToken.IsCancellationRequested);
        }
    }

    [Fact]
    public void Parser_builds_all_typed_request_shapes_and_preserves_every_optional_selection()
    {
        using var f = new CliFixture();
        Type[] types =
        [
            typeof(GetSessionRequest), typeof(GetMetricsRequest), typeof(GetStartupTimelineRequest), typeof(GetFramesRequest),
            typeof(GetObjectHistoryRequest), typeof(GetXamlTreeRequest), typeof(GetCompositionTreeRequest), typeof(GetCorrelationsRequest),
            typeof(GetResourceUsageRequest), typeof(GetSnapshotsRequest), typeof(CompareSnapshotsRequest), typeof(GetRetentionPathsRequest)
        ];
        for (var i = 0; i < Commands.Length; i++)
        {
            var request = CliArguments.Parse(RouteArguments(Commands[i], f)).Bind(f.Scope);
            Assert.IsType(types[i], request);
            QueryValidation.Validate(request, default);
        }
        var metric = Assert.IsType<GetMetricsRequest>(CliArguments.Parse(
            ["metrics", "--db", f.Path, "--start-qpc", "5", "--end-qpc", "10", "--metric", "ProcessPrivateCommit",
                "--limit", "4096", "--offset", "4096", "--watermark-ordinal", "10", "--watermark-qpc", "100"]).Bind(f.Scope));
        Assert.Equal(f.Scope, metric.Scope);
        Assert.Equal(new TimeRange(5, 10), metric.Range);
        Assert.Equal(MetricKind.ProcessPrivateCommit, metric.Metric);
        Assert.Equal(new PageRequest(4096, 4096, new(10, 100)), metric.Page);
        var key = new ObjectKey(f.Scope.Process, IdentityDomain.XamlCore, f.Epoch, ulong.MaxValue);
        string[] identity = ["--domain", "XamlCore", "--producer-epoch", f.Epoch.ToString(), "--runtime-id", ulong.MaxValue.ToString()];
        var tree = Assert.IsType<GetXamlTreeRequest>(CliArguments.Parse(
            ["xaml-tree", "--db", f.Path, "--at-qpc", "10", .. identity]).Bind(f.Scope));
        Assert.Equal(key, tree.Root);
        Assert.Equal(10, tree.AtQpc);
        identity[1] = "CompositionVisual";
        var composition = Assert.IsType<GetCompositionTreeRequest>(CliArguments.Parse(
            ["composition-tree", "--db", f.Path, "--at-qpc", "10", .. identity]).Bind(f.Scope));
        Assert.Equal(new ObjectKey(f.Scope.Process, IdentityDomain.CompositionVisual, f.Epoch, ulong.MaxValue), composition.Root);
        var owner = Assert.IsType<GetResourceUsageRequest>(CliArguments.Parse(
            ["resources", "--db", f.Path, "--start-qpc", "0", "--end-qpc", "10",
                "--domain", "XamlCore", "--producer-epoch", f.Epoch.ToString(), "--runtime-id", ulong.MaxValue.ToString()]).Bind(f.Scope));
        Assert.Equal(key, owner.Owner);
        var snapshots = Assert.IsType<GetSnapshotsRequest>(CliArguments.Parse(
            ["snapshots", "--db", f.Path, "--kind", "NativeHeap"]).Bind(f.Scope));
        Assert.Equal(SnapshotKind.NativeHeap, snapshots.Kind);
    }

    [Fact]
    public async Task Saved_nonempty_wal_is_rejected_without_ignoring_committed_data_or_changing_sidecars()
    {
        using var source = new CliFixture();
        using var saved = new CliFixture();
        using (var store = source.Create())
        {
            store.CreateWriter().Commit(source.Batch(1, 3));
            File.Copy(source.Path, saved.Path);
            File.Copy(source.Path + "-wal", saved.Path + "-wal");
        }
        var before = Files(saved.Path);
        var response = await Execute("metrics", "--db", saved.Path, "--start-qpc", "0", "--end-qpc", "31");
        Assert.Equal(3, response.Code);
        Assert.Empty(response.Output);
        AssertFiles(before, Files(saved.Path));
    }

    [Fact]
    public async Task Checkpointed_database_with_existing_empty_wal_and_shared_memory_retains_all_bytes()
    {
        using var f = new CliFixture();
        using (var store = f.Create()) store.CreateWriter().Commit(f.Batch(1, 1));
        File.WriteAllBytes(f.Path + "-wal", []);
        File.WriteAllBytes(f.Path + "-shm", new byte[32768]);
        var before = Files(f.Path);
        var response = await Execute("session", "--db", f.Path);
        Assert.Equal(0, response.Code);
        AssertFiles(before, Files(f.Path));
    }

    [Fact]
    public async Task Live_writer_is_rejected_and_saved_leases_block_new_writers_without_mutation()
    {
        using var f = new CliFixture();
        using (var writer = f.Create())
        {
            writer.CreateWriter().Commit(f.Batch(1, 1));
            using var output = new StringWriter();
            using var error = new StringWriter();
            Assert.Equal(3, await CliApplication.RunAsync(["session", "--db", f.Path], output, error));
        }
        var before = Files(f.Path);
        using (var saved = SessionStore.OpenSavedReadOnly(f.Path))
        {
            Assert.ThrowsAny<IOException>(() => SessionStore.OpenWritable(f.Path));
            Assert.Throws<InvalidOperationException>(() => saved.CreateWriter());
            Assert.Equal(new CommittedWatermark(1, 10), saved.Watermark);
        }
        AssertFiles(before, Files(f.Path));
        using var reopened = SessionStore.OpenWritable(f.Path);
    }

    [Fact]
    public async Task Global_argument_count_and_total_character_bounds_are_enforced_in_process()
    {
        foreach (var args in new[]
        {
            Enumerable.Repeat("session", 66).ToArray(),
            Enumerable.Repeat(new string('x', 4096), 65).ToArray()
        })
        {
            using var output = new StringWriter();
            using var error = new StringWriter();
            Assert.Equal(2, await CliApplication.RunAsync(args, output, error));
            Assert.Empty(output.ToString());
            using var json = JsonDocument.Parse(error.ToString());
            Assert.Equal("invalid_arguments", json.RootElement.GetProperty("error").GetProperty("code").GetString());
        }
    }

    [Fact]
    public async Task Oversized_page_reports_bounded_error_without_partial_success_output()
    {
        using var f = new CliFixture();
        using (var store = f.Create())
        {
            var source = new string('x', 900_000);
            var writer = store.CreateWriter();
            for (var i = 1; i <= 20; i++)
            {
                var batch = f.Batch(i, i);
                writer.Commit(batch with
                {
                    Changes = batch.Changes.Select(change => change with
                    {
                        Value = ((MetricSample)change.Value) with { Source = source }
                    }).ToArray()
                });
            }
        }
        var response = await Execute("metrics", "--db", f.Path, "--start-qpc", "0", "--end-qpc", "201", "--limit", "20");
        Assert.Equal(5, response.Code);
        Assert.Empty(response.Output);
        Assert.InRange(response.Error.Length, 1, 1024);
        using var json = JsonDocument.Parse(response.Error);
        Assert.Equal("query_error", json.RootElement.GetProperty("error").GetProperty("code").GetString());
    }

    [Fact]
    public async Task Corrupt_database_is_not_modified_or_reported_as_an_unsupported_query()
    {
        using var f = new CliFixture();
        Directory.CreateDirectory(Path.GetDirectoryName(f.Path)!);
        File.WriteAllBytes(f.Path, [1, 2, 3, 4]);
        var before = Files(f.Path);
        var response = await Execute("session", "--db", f.Path);
        Assert.Equal(3, response.Code);
        Assert.Empty(response.Output);
        AssertFiles(before, Files(f.Path));
    }

    [Fact]
    public async Task Json_envelope_budget_is_enforced_even_when_the_engine_materialization_fits()
    {
        using var f = new CliFixture();
        var options = new JsonSerializerOptions
        {
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
            Converters = { new JsonStringEnumConverter(allowIntegerValues: false) }
        };
        using (var store = f.Create())
        {
            var writer = store.CreateWriter();
            for (var i = 1; i <= 16; i++)
            {
                var batch = f.Batch(i, i);
                var sample = ((MetricSample)batch.Changes.Single().Value) with { Source = "" };
                var sourceLength = ContractVersions.MaximumPayloadBytes - JsonSerializer.SerializeToUtf8Bytes(sample, options).Length;
                sample = sample with { Source = new string('x', sourceLength) };
                Assert.Equal(ContractVersions.MaximumPayloadBytes, JsonSerializer.SerializeToUtf8Bytes(sample, options).Length);
                writer.Commit(batch with { Changes = [new(i, sample)] });
            }
        }
        using (var saved = SessionStore.OpenSavedReadOnly(f.Path))
        {
            var page = await saved.Queries.GetMetrics(new(saved.Scope, new(0, 161), new(16)), default);
            Assert.Equal(16, page.Items.Count);
        }
        var response = await Execute("metrics", "--db", f.Path, "--start-qpc", "0", "--end-qpc", "161", "--limit", "16");
        Assert.Equal(5, response.Code);
        Assert.Empty(response.Output);
        Assert.InRange(response.Error.Length, 1, 1024);
    }

    private static string[] RouteArguments(string command, CliFixture f)
    {
        var args = new List<string> { command, "--db", f.Path };
        if (command is "metrics" or "frames" or "object-history" or "resources")
            args.AddRange(["--start-qpc", "0", "--end-qpc", "11"]);
        if (command is "xaml-tree" or "composition-tree" or "correlations") args.AddRange(["--at-qpc", "10"]);
        if (command is "object-history" or "correlations" or "retention")
            args.AddRange(["--domain", "XamlCore", "--producer-epoch", f.Epoch.ToString(), "--runtime-id", ulong.MaxValue.ToString()]);
        if (command == "compare") args.AddRange(["--baseline-id", Guid.NewGuid().ToString(), "--snapshot-ids", Guid.NewGuid().ToString()]);
        if (command == "retention") args.AddRange(["--snapshot-id", Guid.NewGuid().ToString(), "--maximum-depth", "10"]);
        return args.ToArray();
    }

    private static void AssertEnvelope(JsonElement json, string command, CliFixture fixture)
    {
        Assert.Equal(1, json.GetProperty("schemaVersion").GetInt32());
        Assert.Equal(command, json.GetProperty("command").GetString());
        var context = json.GetProperty("result").GetProperty("context");
        Assert.Equal(fixture.Scope.SessionId, context.GetProperty("scope").GetProperty("sessionId").GetGuid());
        Assert.Equal(fixture.Scope.Process.InstanceId, context.GetProperty("scope").GetProperty("process").GetProperty("instanceId").GetGuid());
        Assert.True(context.TryGetProperty("watermark", out _));
        Assert.True(context.TryGetProperty("quality", out _));
    }

    internal sealed class CliFixture : IDisposable
    {
        internal string Path { get; } = System.IO.Path.GetFullPath(System.IO.Path.Combine("TestResults", Guid.NewGuid() + ".sqlite"));
        internal SessionScope Scope { get; } = new(Guid.NewGuid(), new(42, null, Guid.NewGuid()));
        internal Guid Epoch { get; } = Guid.NewGuid();
        internal SessionInfo Info { get; } = new(SessionMode.Offline, SessionStatus.Recording, 1_000_000, null, [], null);
        internal static DataQuality Partial { get; } = new(MeasurementKind.Estimated, CoverageState.Partial, "explicit synthetic estimate");

        internal SessionStore Create()
        {
            Directory.CreateDirectory(System.IO.Path.GetDirectoryName(Path)!);
            return SessionStore.Create(Path, Scope, Info, Partial);
        }

        internal SessionBatch Batch(int first, int last)
        {
            var sources = new List<NormalizedEvent>();
            var changes = new List<ProjectionChange>();
            for (var i = first; i <= last; i++)
            {
                var sample = new MetricSample(i * 10, MetricKind.ProcessPrivateCommit,
                    new(ValueKind.Integer, ValueUnit.Bytes, i, null, null, null, Partial),
                    new(i * 10 - 1, i * 10), "synthetic", "1");
                var payload = new MetricPayload(sample.Metric, sample.Value, sample.Interval, sample.Source, sample.SourceVersion, null);
                var identity = EventCatalog.Get(EventCatalog.CollectorProviderId, 2).Identity;
                var envelope = new EventEnvelope(Scope.SessionId, Scope.Process, 99, 7, sample.TimestampQpc, i,
                    (ulong)i, null, identity, EventCodec.EncodePayload(payload, Scope.Process));
                sources.Add(EventCodec.Normalize(envelope, new(identity.ProviderId, 99, Epoch)));
                changes.Add(new(i, sample));
            }
            return new(sources, changes, Info, Partial);
        }

        public void Dispose()
        {
            foreach (var suffix in new[] { "", "-wal", "-shm", ".writer", ".failure.json" })
                if (File.Exists(Path + suffix)) File.Delete(Path + suffix);
        }
    }

    private static Dictionary<string, byte[]> Files(string path) =>
        new[] { "", "-wal", "-shm", ".writer", ".failure.json" }.Where(suffix => File.Exists(path + suffix))
            .ToDictionary(suffix => suffix, suffix => File.ReadAllBytes(path + suffix));

    private static void AssertFiles(Dictionary<string, byte[]> before, Dictionary<string, byte[]> after)
    {
        Assert.Equal(before.Keys.Order(), after.Keys.Order());
        foreach (var (suffix, bytes) in before) Assert.Equal(bytes, after[suffix]);
    }

    private async Task<(int Code, string Output, string Error)> Execute(params string[] args)
    {
        var root = new DirectoryInfo(AppContext.BaseDirectory);
        while (root is not null && !File.Exists(Path.Combine(root.FullName, "WinUIProfiler.sln"))) root = root.Parent;
        Assert.NotNull(root);
        var configuration = new DirectoryInfo(AppContext.BaseDirectory).Parent!.Name;
        var assembly = Path.Combine(root.FullName, "WinUIProfiler.Cli", "bin", configuration, "net10.0", "WinUIProfiler.Cli.dll");
        Assert.True(File.Exists(assembly), "The CLI executable must be built before running its subprocess contract.");
        var start = new ProcessStartInfo("dotnet") { RedirectStandardOutput = true, RedirectStandardError = true, UseShellExecute = false };
        start.ArgumentList.Add(assembly);
        foreach (var arg in args) start.ArgumentList.Add(arg);
        using var process = Process.Start(start)!;
        var output = process.StandardOutput.ReadToEndAsync();
        var error = process.StandardError.ReadToEndAsync();
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(30));
        try { await process.WaitForExitAsync(timeout.Token); }
        catch { if (!process.HasExited) process.Kill(entireProcessTree: true); throw; }
        var stdout = await output;
        var stderr = await error;
        var arguments = string.Join(' ', args);
        log.WriteLine($"dotnet \"{assembly}\" {arguments[..Math.Min(arguments.Length, 512)]}");
        log.WriteLine($"exit={process.ExitCode}; stdout={stdout[..Math.Min(stdout.Length, 1024)]}; stderr={stderr[..Math.Min(stderr.Length, 512)]}");
        return (process.ExitCode, stdout, stderr);
    }
}

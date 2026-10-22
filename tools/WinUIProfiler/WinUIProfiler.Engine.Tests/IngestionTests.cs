using System.Diagnostics;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;
using Xunit.Abstractions;

namespace WinUIProfiler.Engine.Tests;

public sealed class IngestionTests(ITestOutputHelper output)
{
    internal static string Root => Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\.."));
    internal static string Harness => Path.Combine(Root, @"BuildOutput\ProfilerHarness\x64\Release\ProfilerInstrumentationHarness.exe");

    [Fact]
    public void Owned_native_stop_returns_final_transport_statistics()
    {
        var name = "WinUIProfiler-P03-stop-" + Guid.NewGuid().ToString("N");
        using var session = new TraceEventSession(name, TraceEventSessionOptions.Create | TraceEventSessionOptions.NoRestartOnCreate);
        session.EnableProvider(EventCatalog.RuntimeProviderId);
        var before = EtwSessionStatistics.Query(name);
        var final = EtwSessionStatistics.Stop(name);
        session.StopOnDispose = false;
        Assert.True(final.Events >= before.Events);
        Assert.True(final.Buffers >= before.Buffers);
        Assert.Throws<System.ComponentModel.Win32Exception>(() => EtwSessionStatistics.Query(name));
    }

    [Fact]
    public async Task Offline_cancellation_and_missing_file_have_explicit_partial_terminal_states()
    {
        using var fixtures = new NativeFixtures();
        using var cancelled = new StoreFixture();
        using var failed = new StoreFixture();
        using var store1 = cancelled.Create();
        using var store2 = failed.Create();
        await using var worker1 = new ProcessingWorker(store1, cancelled.Info);
        await using var worker2 = new ProcessingWorker(store2, failed.Info);
        await new OfflineEtlSource(fixtures.Files[0], cancelled.Scope, cancelled.Info.QpcFrequency).RunAsync(worker1, new(true));
        await Assert.ThrowsAsync<FileNotFoundException>(() =>
            new OfflineEtlSource(Path.Combine(fixtures.DirectoryPath, "absent.etl"), failed.Scope, failed.Info.QpcFrequency).RunAsync(worker2));
        var result1 = await worker1.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        var result2 = await worker2.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.Stopped, result1.Status);
        Assert.Equal(new(0, 0), result1.DurableBoundary);
        Assert.Equal(0, result1.Health.Count(IngestionIssue.ProducerLoss));
        Assert.Equal(SessionStatus.Failed, result2.Status);
        Assert.Equal(1, result2.Health.Count(IngestionIssue.SourceFailure));
    }

    [Fact]
    public void Unknown_creation_stays_null_and_is_not_bound_to_a_reused_or_different_instance()
    {
        using var fixtures = new NativeFixtures();
        var record = fixtures.Read().First(r => r.Name == "Frame");
        var fields = record.Fields.ToDictionary();
        var target = ((IReadOnlyDictionary<string, object>)fields["TargetProcess"]).ToDictionary();
        target["HasCreationTimeFileTime"] = false;
        fields["TargetProcess"] = target;
        var withoutCreation = record.WithFields(fields);
        var traceLocal = withoutCreation.ReadRuntimeTarget();
        Assert.Null(traceLocal.CreationTimeFileTime);
        var scope = new SessionScope(Guid.NewGuid(), traceLocal);
        Assert.Null(new EventDecoder(new(scope.SessionId, new(traceLocal.ProcessId, null, Guid.NewGuid())),
            new(Stopwatch.Frequency, Stopwatch.Frequency)).Decode(withoutCreation));
        var accepted = new EventDecoder(scope, new(Stopwatch.Frequency, Stopwatch.Frequency)).Decode(withoutCreation)!;
        Assert.Null(accepted.Event.Envelope.Process.CreationTimeFileTime);
        Assert.Throws<OverflowException>(() => new QpcMapping(1, long.MaxValue).Convert(long.MaxValue));
        Assert.Throws<InvalidDataException>(() => new QpcMapping(1, 1, 2).Convert(1));
    }

    [Fact]
    public void Native_near_budget_arrays_decode_without_a_capabilities_prerequisite()
    {
        using var fixtures = new NativeFixtures("--emit-budget");
        var records = fixtures.Read();
        Assert.NotEmpty(records);
        Assert.All(records, record => Assert.Equal("CaptureChunk", record.Name));
        foreach (var record in records)
        {
            var target = record.ReadRuntimeTarget();
            var decoded = new EventDecoder(new(Guid.NewGuid(), target), new(Stopwatch.Frequency, Stopwatch.Frequency)).Decode(record)!;
            var chunk = EventCodec.ReadPayload<CaptureChunkPayload>(decoded.Event.Envelope);
            Assert.InRange(chunk.Objects.Count, 35, 48);
            Assert.All(chunk.Objects, item => Assert.Equal(1024, item.TypeName!.Length));
        }
    }

    [Fact]
    public void Native_capture_table_references_and_inactive_arms_are_validated_before_projection()
    {
        using var fixtures = new NativeFixtures();
        var record = fixtures.Read().Single(r => r.Name == "CaptureChunk");
        var target = record.ReadRuntimeTarget();
        var decoder = new EventDecoder(new(Guid.NewGuid(), target), new(Stopwatch.Frequency, Stopwatch.Frequency));
        foreach (var (array, field) in new[] { ("Objects", "KeyIndex"), ("Objects", "SizeIndex"), ("Ownership", "FromIndex"),
            ("Tree", "ChildIndex"), ("Resources", "ClassificationIndex"), ("Visuals", "ValueIndex"), ("Allocations", "QualityIndex") })
            Assert.Throws<InvalidDataException>(() => decoder.Decode(ReplaceRow(record, array, 0, field, 0)));
        Assert.Throws<InvalidDataException>(() => decoder.Decode(ReplaceRow(record, "Resources", 0, "OwnerIndex", int.MaxValue)));
        Assert.Throws<InvalidDataException>(() => decoder.Decode(ReplaceRow(record, "Values", 0, "Integer", 123L)));
    }

    [Fact]
    public void Native_raw_payload_is_owned_and_rejects_invalid_UTF8_and_truncation()
    {
        using var fixtures = new NativeFixtures();
        var record = fixtures.Read().First(r => r.Name == "TypeResolved");
        var target = record.ReadRuntimeTarget();
        var decoder = new EventDecoder(new(Guid.NewGuid(), target), new(Stopwatch.Frequency, Stopwatch.Frequency));
        var input = record.RawPayload.ToArray();
        var owned = new NativeRecord(record.ProviderId, record.Name, record.Level, record.Keywords, record.ProducerProcessId,
            record.ThreadId, record.TimestampQpc, record.ActivityId, record.Fields, input);
        var before = owned.RawPayload.ToArray();
        input.AsSpan().Clear();
        Assert.Equal(before, owned.RawPayload.ToArray());
        Assert.NotNull(decoder.Decode(owned));
        var bad = before.ToArray();
        var index = bad.AsSpan().IndexOf("Button"u8);
        Assert.True(index >= 0);
        bad[index] = 0xff;
        NativeRecord WithRaw(byte[] bytes) => new(record.ProviderId, record.Name, record.Level, record.Keywords,
            record.ProducerProcessId, record.ThreadId, record.TimestampQpc, record.ActivityId, record.Fields, bytes);
        Assert.Throws<InvalidDataException>(() => decoder.Decode(WithRaw(bad)));
        Assert.Throws<InvalidDataException>(() => decoder.Decode(WithRaw(before[..^1])));
    }

    private static NativeRecord ReplaceRow(NativeRecord record, string array, int index, string name, object value)
    {
        var fields = record.Fields.ToDictionary();
        var rows = ((IReadOnlyList<object>)fields[array]).ToArray();
        var row = ((IReadOnlyDictionary<string, object>)rows[index]).ToDictionary();
        row[name] = value; rows[index] = row; fields[array] = rows;
        return record.WithFields(fields);
    }

    [Fact]
    public async Task Offline_real_wire_replay_is_equivalent_through_SQLite_and_committed_queries()
    {
        using var fixtures = new NativeFixtures();
        var target = fixtures.Read()[0].ReadRuntimeTarget();
        var canonical = new ProcessInstanceKey(target.ProcessId, target.CreationTimeFileTime, Guid.NewGuid());
        var results = new List<List<string>>();
        foreach (var pass in Enumerable.Range(0, 2))
        {
            var semantic = new List<string>();
            foreach (var file in fixtures.Files)
            {
                var metadata = OfflineEtlSource.ReadMetadata(file);
                Assert.Equal(Stopwatch.Frequency, metadata.QpcFrequency);
                var scope = new SessionScope(Guid.NewGuid(), canonical);
                var info = new SessionInfo(SessionMode.Offline, SessionStatus.Recording, metadata.QpcFrequency, null, [], null);
                var path = Path.Combine(fixtures.DirectoryPath, $"import-{pass}-{Guid.NewGuid():N}.sqlite");
                using var store = SessionStore.Create(path, scope, info, StoreFixture.Partial);
                await using var worker = new ProcessingWorker(store, info);
                await new OfflineEtlSource(file, scope, metadata.QpcFrequency).RunAsync(worker);
                var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(20));
                Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
                Assert.Equal(0, result.Health.Count(IngestionIssue.WriterFailure));
                using var db = SessionStore.Connect(path, true);
                using var command = SessionStore.Command(db, "SELECT Envelope FROM SourceEvents ORDER BY Ordinal");
                using var rows = command.ExecuteReader();
                while (rows.Read())
                {
                    var e = EventCodec.Decode((byte[])rows[0]);
                    Assert.Equal(canonical, e.Process);
                    semantic.Add($"{e.Identity.Name}|{e.ProducerProcessId}|{e.ThreadId}|{e.TimestampQpc}|{e.SourceSequence}|{Convert.ToHexString(e.OwnedPayload.Span)}");
                }
                var query = await store.Queries.GetSession(new(scope), default);
                Assert.Equal(result.DurableBoundary, query.Context.Watermark);
                Assert.Equal(CoverageState.Partial, query.Context.Quality.Coverage);
                semantic.Add(System.Text.Json.JsonSerializer.Serialize(query.Value));
                Assert.Equal(0L, StoreFixture.Scalar(db, "SELECT count(*) FROM SourceProvenance WHERE OriginalInstanceId='" + canonical.InstanceId.ToString("N") + "'"));
                Assert.True(result.DurableBoundary.IngestOrdinal > 0);
            }
            results.Add(semantic);
        }
        Assert.Equal(results[0], results[1]);
    }

    [Fact]
    public async Task Two_live_sessions_own_only_their_transport_and_native_arrays_keep_flowing_after_one_stops()
    {
        var directory = Path.Combine(Root, @"BuildOutput\ProfilerHarness", "P03-live-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(directory);
        try
        {
            using var childLifetime = new OwnedTestProcess(Process.Start(new ProcessStartInfo(Harness)
            {
                WorkingDirectory = Root, UseShellExecute = false, RedirectStandardInput = true, RedirectStandardOutput = true,
                ArgumentList = { "--emit-fixtures", directory, "--await-input" }
            })!);
            var child = childLifetime.Process;
            await ReadUntil(child, "READY: scalar");
            var target = new ProcessInstanceKey(checked((uint)child.Id), child.StartTime.ToFileTimeUtc(), Guid.NewGuid());
            var scope1 = new SessionScope(Guid.NewGuid(), target);
            var scope2 = new SessionScope(Guid.NewGuid(), target);
            var info = new SessionInfo(SessionMode.LiveAttach, SessionStatus.Recording, Stopwatch.Frequency, null, [], null);
            using var store1 = SessionStore.Create(Path.Combine(directory, "one.sqlite"), scope1, info, StoreFixture.Partial);
            using var store2 = SessionStore.Create(Path.Combine(directory, "two.sqlite"), scope2, info, StoreFixture.Partial);
            var p1 = new SignalProcessor("Frame");
            var p2 = new SignalProcessor("CaptureChunk");
            await using var worker1 = new ProcessingWorker(store1, info, processors: [p1]);
            await using var worker2 = new ProcessingWorker(store2, info, processors: [p2]);
            await using var source1 = new LiveEtwSource(scope1, Stopwatch.Frequency);
            await using var source2 = new LiveEtwSource(scope2, Stopwatch.Frequency);
            var run1 = source1.RunAsync(worker1);
            var run2 = source2.RunAsync(worker2);
            await Task.WhenAll(source1.Started, source2.Started).WaitAsync(TimeSpan.FromSeconds(20));
            Assert.NotEqual(source1.SessionName, source2.SessionName);
            await child.StandardInput.WriteLineAsync();
            await ReadUntil(child, "READY: arrays");
            await p1.Seen.Task.WaitAsync(TimeSpan.FromSeconds(20));
            await source1.StopAsync();
            await run1.WaitAsync(TimeSpan.FromSeconds(20));
            Assert.False(run2.IsCompleted);
            await child.StandardInput.WriteLineAsync();
            await p2.Seen.Task.WaitAsync(TimeSpan.FromSeconds(20));
            await child.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(20));
            Assert.Equal(0, child.ExitCode);
            await source2.StopAsync();
            await run2.WaitAsync(TimeSpan.FromSeconds(20));
            var first = await worker1.Completion;
            var second = await worker2.Completion;
            Assert.Equal(SessionStatus.Stopped, first.Status);
            Assert.Equal(SessionStatus.Stopped, second.Status);
            Assert.Equal(0, first.Health.Count(IngestionIssue.DecodeFailure));
            Assert.Equal(0, second.Health.Count(IngestionIssue.DecodeFailure));
            Assert.True(second.DurableBoundary.IngestOrdinal > first.DurableBoundary.IngestOrdinal);
        }
        finally { Directory.Delete(directory, true); }
    }

    private static async Task ReadUntil(Process child, string expected)
    {
        while (await child.StandardOutput.ReadLineAsync().WaitAsync(TimeSpan.FromSeconds(20)) is { } line)
            if (line == expected) return;
        Assert.Fail("Native fixture producer exited before " + expected);
    }
    private sealed class SignalProcessor(string eventName) : IEventProcessor
    {
        internal TaskCompletionSource Seen { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);
        public IReadOnlyList<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
        {
            if (events.Any(e => e.Envelope.Identity.Name == eventName)) Seen.TrySetResult();
            return [];
        }
        public void Invalidate(HealthSnapshot health) { }
    }

    [Fact]
    public async Task Platform_process_schema_has_payload_identity_independent_of_header()
    {
        using var session = new TraceEventSession("WinUIProfiler-P03-kernel-" + Guid.NewGuid().ToString("N"),
            TraceEventSessionOptions.Create | TraceEventSessionOptions.NoRestartOnCreate);
        var seen = new TaskCompletionSource<NativeRecord>(TaskCreationOptions.RunContinuationsAsynchronously);
        session.Source.Dynamic.All += data =>
        {
            if (data.ProviderGuid == Guid.Parse("22fb2cd6-0e7b-422b-a0c7-2fad1fd0e716") && (int)data.ID == 1)
            {
                output.WriteLine($"Kernel name={data.EventName} ID={(int)data.ID} version={data.Version} level={(int)data.Level} keyword={(ulong)data.Keywords:X} sourcePID={data.ProcessID} " +
                    string.Join(", ", data.PayloadNames.Select((n, i) => n + "=" + data.PayloadValue(i)?.GetType().Name + ":" + data.PayloadValue(i))));
                try { seen.TrySetResult(NativeRecord.Capture(data)); }
                catch (Exception error) { seen.TrySetException(error); }
            }
        };
        session.EnableProvider(Guid.Parse("22fb2cd6-0e7b-422b-a0c7-2fad1fd0e716"), TraceEventLevel.Informational, 0x10);
        var reading = Task.Factory.StartNew(() => session.Source.Process(), CancellationToken.None, TaskCreationOptions.LongRunning, TaskScheduler.Default);
        try
        {
            using var childLifetime = new OwnedTestProcess(Process.Start(new ProcessStartInfo(Environment.GetEnvironmentVariable("ComSpec")!)
            { ArgumentList = { "/c", "set", "/p", "P03=" }, UseShellExecute = false, RedirectStandardInput = true, RedirectStandardOutput = true })!);
            var child = childLifetime.Process;
            var actualCreation = child.StartTime.ToFileTimeUtc();
            var record = await seen.Task.WaitAsync(TimeSpan.FromSeconds(20));
            var targetPid = unchecked((uint)(int)record.Fields["ProcessID"]);
            var created = ((DateTime)record.Fields["CreateTime"]).ToFileTimeUtc();
            Assert.Equal((uint)child.Id, targetPid);
            Assert.Equal(actualCreation, created);
            Assert.NotEqual(record.ProducerProcessId, targetPid);
            var scope = new SessionScope(Guid.NewGuid(), new(targetPid, created, Guid.NewGuid()));
            var decoded = new EventDecoder(scope, new(Stopwatch.Frequency, Stopwatch.Frequency)).Decode(record)!;
            Assert.Equal(targetPid, decoded.Event.Envelope.Process.ProcessId);
            Assert.Equal(record.ProducerProcessId, decoded.Event.Envelope.ProducerProcessId);
            Assert.Null(decoded.Event.Envelope.SourceSequence);
            Assert.Equal(ProcessOperation.Start, EventCodec.ReadPayload<ProcessPayload>(decoded.Event.Envelope).Operation);
            var directory = Path.Combine(Root, @"BuildOutput\ProfilerHarness", "P03-kernel-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(directory);
            try
            {
                var info = new SessionInfo(SessionMode.LiveAttach, SessionStatus.Recording, Stopwatch.Frequency, null, [], null);
                var path = Path.Combine(directory, "kernel.sqlite");
                using var store = SessionStore.Create(path, scope, info, StoreFixture.Partial);
                await using var worker = new ProcessingWorker(store, info);
                Assert.True(worker.TryEnqueue(decoded));
                worker.Complete(SourceCompletion.FiniteEnd);
                var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
                Assert.Equal(1, result.DurableBoundary.IngestOrdinal);
                using var db = SessionStore.Connect(path, true);
                using var command = SessionStore.Command(db, "SELECT Envelope FROM SourceEvents");
                var persisted = EventCodec.Decode((byte[])command.ExecuteScalar()!);
                Assert.Equal(record.ProducerProcessId, persisted.ProducerProcessId);
                Assert.Equal(targetPid, persisted.Process.ProcessId);
                Assert.Equal(actualCreation, persisted.Process.CreationTimeFileTime);
            }
            finally { Directory.Delete(directory, true); }
            await child.StandardInput.WriteLineAsync("continue");
            await child.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(10));
        }
        finally { session.Stop(); await reading.WaitAsync(TimeSpan.FromSeconds(20)); }
    }

    [Fact]
    public void Decoder_accepts_all_actual_P04_schemas_and_rebinds_native_identity()
    {
        using var fixtures = new NativeFixtures();
        var records = fixtures.Read();
        var accepted = new List<DecodedEvent>();
        foreach (var record in records)
        {
            var native = record.ReadRuntimeTarget();
            var scope = new SessionScope(Guid.NewGuid(), new(native.ProcessId, native.CreationTimeFileTime, Guid.NewGuid()));
            var decoder = new EventDecoder(scope, new(Stopwatch.Frequency, Stopwatch.Frequency));
            var decoded = decoder.Decode(record)!;
            Assert.Equal(scope.Process, decoded.Event.Envelope.Process);
            Assert.Equal(native, decoded.Provenance.OriginalTarget);
            Assert.Equal(record.ProducerProcessId, decoded.Event.Envelope.ProducerProcessId);
            accepted.Add(decoded);
        }
        Assert.Equal(Enumerable.Range(1, 16).Select(n => (ushort)n), accepted.Select(e => e.Event.Envelope.Identity.EventId).Distinct().Order());
        var chunk = EventCodec.ReadPayload<CaptureChunkPayload>(accepted.Single(e => e.Event.Envelope.Identity.EventId == 14).Event.Envelope);
        Assert.Equal("second variable-width µ name", chunk.Objects[1].TypeName);
        Assert.Equal(80, chunk.Objects[1].Size.Integer);
        Assert.Equal(4096ul, chunk.Resources[0].AllocationBytes);
        Assert.Equal(9ul, chunk.Visuals[0].Cause!.Attempt!.Value.AttemptId);
        Assert.Equal(chunk.Objects[0].Key, chunk.Resources[0].Owner);
        Assert.Equal(2, chunk.Objects.Count);
    }

    [Fact]
    public void Decoder_rejects_corruption_and_filters_by_actual_creation_not_producer_GUID()
    {
        using var fixtures = new NativeFixtures();
        var record = fixtures.Read().First(r => r.Name == "Frame");
        var native = record.ReadRuntimeTarget();
        var selected = new ProcessInstanceKey(native.ProcessId, native.CreationTimeFileTime, Guid.NewGuid());
        var scope = new SessionScope(Guid.NewGuid(), selected);
        var decoder = new EventDecoder(scope, new(Stopwatch.Frequency, Stopwatch.Frequency));
        Assert.NotNull(decoder.Decode(record));
        var reused = new EventDecoder(new SessionScope(scope.SessionId, new(native.ProcessId, native.CreationTimeFileTime + 1, Guid.NewGuid())),
            new(Stopwatch.Frequency, Stopwatch.Frequency));
        Assert.Null(reused.Decode(record));
        var fields = record.Fields.ToDictionary();
        fields["EventSchemaVersion"] = (byte)2;
        Assert.Throws<NotSupportedException>(() => decoder.Decode(record.WithFields(fields)));
        fields["EventSchemaVersion"] = (byte)1;
        fields["Outcome"] = (byte)255;
        Assert.Throws<ArgumentOutOfRangeException>(() => decoder.Decode(record.WithFields(fields)));
        fields["Outcome"] = (byte)3;
        fields.Remove("HResult");
        Assert.Throws<InvalidDataException>(() => decoder.Decode(record.WithFields(fields)));
        fields = record.Fields.ToDictionary();
        fields.Add("UnexpectedSchemaField", 1);
        Assert.Throws<InvalidDataException>(() => decoder.Decode(record.WithFields(fields)));
    }

    [Fact]
    public void Native_fixture_seam_retains_real_typed_ETLs()
    {
        var directory = Path.Combine(Root, @"BuildOutput\ProfilerHarness", "P03-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(directory);
        try
        {
            using var process = Process.Start(new ProcessStartInfo(Harness)
            {
                WorkingDirectory = Root, UseShellExecute = false, RedirectStandardOutput = true,
                ArgumentList = { "--emit-fixtures", directory }
            })!;
            Assert.True(process.WaitForExit(60_000), "Native fixture producer did not finish.");
            var text = process.StandardOutput.ReadToEnd();
            output.WriteLine(text);
            Assert.Equal(0, process.ExitCode);
            var files = Directory.GetFiles(directory, "*.etl");
            Assert.Equal(2, files.Length);
            foreach (var file in files)
            {
                using var source = new ETWTraceEventSource(file);
                source.Kernel.EventTraceHeader += data => output.WriteLine($"CLOCK flags={data.ReservedFlags} freq={data.PerfFreq}");
                source.Dynamic.All += data =>
                {
                    if (data.ProviderGuid != Contracts.EventCatalog.RuntimeProviderId) return;
                    output.WriteLine(data.EventName + ": " + string.Join(", ", data.PayloadNames.Select((n, i) =>
                        n + "=" + data.PayloadValue(i)?.GetType().FullName + ":" + data.PayloadValue(i))));
                };
                Assert.True(source.Process());
            }

        }
        finally { Directory.Delete(directory, true); }
    }
}

internal sealed class NativeFixtures : IDisposable
{
    internal string DirectoryPath { get; } = Path.Combine(IngestionTests.Root, @"BuildOutput\ProfilerHarness", "P03-" + Guid.NewGuid().ToString("N"));
    internal NativeFixtures(string mode = "--emit-fixtures")
    {
        Directory.CreateDirectory(DirectoryPath);
        using var process = Process.Start(new ProcessStartInfo(IngestionTests.Harness)
        {
            WorkingDirectory = IngestionTests.Root, UseShellExecute = false, RedirectStandardOutput = true,
            ArgumentList = { mode, DirectoryPath }
        })!;
        Assert.True(process.WaitForExit(60_000), "Native producer timed out.");
        var text = process.StandardOutput.ReadToEnd();
        Assert.True(process.ExitCode == 0, text);
    }
    internal string[] Files => Directory.GetFiles(DirectoryPath, "*.etl").Order().ToArray();
    internal IReadOnlyList<NativeRecord> Read()
    {
        var records = new List<NativeRecord>();
        foreach (var file in Files)
        {
            using var source = new ETWTraceEventSource(file);
            source.Dynamic.All += data =>
            {
                if (data.ProviderGuid == EventCatalog.RuntimeProviderId) records.Add(NativeRecord.Capture(data));
            };
            Assert.True(source.Process());
        }
        return records;
    }
    public void Dispose() => Directory.Delete(DirectoryPath, true);
}

internal sealed class OwnedTestProcess(Process process) : IDisposable
{
    internal Process Process { get; } = process;
    public void Dispose()
    {
        try
        {
            if (!Process.HasExited)
            {
                if (Process.StartInfo.RedirectStandardInput) Process.StandardInput.Close();
                if (!Process.WaitForExit(5000))
                {
                    Process.Kill(entireProcessTree: true);
                    Assert.True(Process.WaitForExit(10_000), "Owned test producer failed to terminate.");
                }
            }
        }
        finally { Process.Dispose(); }
    }
}

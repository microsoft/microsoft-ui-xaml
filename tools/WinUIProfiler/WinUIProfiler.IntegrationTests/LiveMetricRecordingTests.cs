using System.Diagnostics;
using System.Runtime.InteropServices;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.IntegrationTests;

[Collection("Native collector ETW")]
public sealed class LiveMetricRecordingTests : IDisposable
{
    private readonly List<string> databases = [];
    private static string Dll => Path.Combine(
        Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\..")),
        @"tools\WinUIProfiler\WinUIProfiler.Native\bin",
        RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "ARM64" : "x64",
        @"Release\WinUIProfiler.Native.dll");

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void Recording_rejects_duplicate_or_oversized_adapter_configuration_at_construction(bool oversized)
    {
        using var process = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(process));
        var info = Info();
        using var store = SessionStore.Create(NewPath(scope), scope, info, Quality());
        var adapters = oversized ? Enumerable.Range(0, 257).Select(index =>
            new PhysicalAdapterKey((uint)index, 0, 0)).ToArray() :
            new PhysicalAdapterKey[] { new(1, 0, 0), new(1, 0, 0) };
        Assert.Throws<ArgumentException>(() =>
        {
            _ = new LiveMetricRecording(store, info, Dll, TimeSpan.FromSeconds(30), gpuMemoryAdapters: adapters);
        });
    }

    [Fact]
    public async Task Configured_adapter_memory_drains_through_owned_collector_and_survives_reopen()
    {
        using var process = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(process));
        var path = NewPath(scope);
        var info = Info();
        var adapter = new PhysicalAdapterKey(uint.MaxValue, int.MaxValue, 0);
        PhysicalAdapterKey[] selection = [adapter];
        using (var store = SessionStore.Create(path, scope, info, Quality()))
        {
            await using var recording = new LiveMetricRecording(store, info, Dll, TimeSpan.FromSeconds(30),
                gpuMemoryAdapters: selection);
            selection[0] = new(1, 2, 3);
            recording.Start();
            await recording.Started.WaitAsync(TimeSpan.FromSeconds(10));
            var result = await recording.StopAsync().WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(SessionStatus.Stopped, result.Status);
            Assert.True(result.DurableBoundary.IngestOrdinal >= 18);
            Assert.Equal(0, result.Health.Count(IngestionIssue.SourceFailure));
            Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
            Assert.Equal(0, result.Health.Count(IngestionIssue.EtwEventsLost));
            Assert.Equal(0, result.Health.Count(IngestionIssue.EtwBuffersLost));
        }
        using var reopened = SessionStore.OpenReadOnly(path);
        var metrics = await reopened.Queries.GetMetrics(new(scope,
            new(0, checked(reopened.Watermark.TimestampQpc + 1)), new(512)), default);
        var memory = metrics.Items.Where(metric => metric.Gpu?.Scope == GpuMetricScope.PerAdapterMemory).ToArray();
        Assert.Equal(4, memory.Length);
        Assert.All(memory, metric =>
        {
            Assert.Equal(adapter, metric.Gpu!.Adapter);
            Assert.Null(metric.Gpu.Engine);
            Assert.Equal(ValueKind.Missing, metric.Value.Kind);
            Assert.Equal("DXCore AdapterMemoryUsageByProcessBytes", metric.Source);
            Assert.Equal(MetricClock.Boot100ns, metric.SourceObservation!.Clock);
            Assert.Equal(MetricSampleStatus.Unavailable, metric.SourceObservation.Status);
            Assert.Equal(MetricErrorDomain.HResult, metric.SourceObservation.ErrorDomain);
            Assert.NotEqual(0u, metric.SourceObservation.NativeError);
        });
    }

    [Fact]
    public async Task Recording_starts_after_first_native_publication_and_drains_before_committing_stop()
    {
        using var inventorySource = new NativeGpuAdapterInventory(Dll);
        var inventory = inventorySource.Capture();
        var expectedMemoryScopes = inventory.CanConfigureMemory ? inventory.SelectMemoryAdapters(false).Count : 0;
        using var process = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(process));
        var path = NewPath(scope);
        var info = Info();
        using (var store = SessionStore.Create(path, scope, info, Quality()))
        {
            await using var recording = new LiveMetricRecording(store, info, Dll, TimeSpan.FromSeconds(30));
            recording.Start();
            await recording.Started.WaitAsync(TimeSpan.FromSeconds(10));
            var result = await recording.StopAsync().WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(SessionStatus.Stopped, result.Status);
            Assert.True(result.DurableBoundary.IngestOrdinal >= 14,
                "The ready barrier must include Attach and at least thirteen native metrics before transport drain.");
            Assert.Equal(0, result.Health.Count(IngestionIssue.SourceFailure));
            Assert.Equal(0, result.Health.Count(IngestionIssue.ProducerLoss));
            Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
            Assert.Equal(0, result.Health.Count(IngestionIssue.UnknownSchema));
            Assert.Equal(0, result.Health.Count(IngestionIssue.EtwEventsLost));
            Assert.Equal(result, await recording.Completion);
            Assert.Equal(result, await recording.StopAsync());
        }
        using var reopened = SessionStore.OpenReadOnly(path);
        Assert.Equal(ProcessOperation.Attach, Assert.Single(ReadProcessOperations(reopened)));
        var attach = Assert.Single(ReadProcessPayloads(reopened));
        Assert.Contains("DXGI-visible graphics inventory:", attach.Quality.Reason);
        Assert.Contains(FormattableString.Invariant($"status={inventory.Status};"), attach.Quality.Reason);
        Assert.Contains(FormattableString.Invariant($"selectedPhysicalScopes={expectedMemoryScopes};"), attach.Quality.Reason);
        Assert.Contains("excludes compute/NPU-only inventory", attach.Quality.Reason);
        Assert.Contains("no process-wide aggregation", attach.Quality.Reason);
        var metrics = await reopened.Queries.GetMetrics(new(scope,
            new(0, checked(reopened.Watermark.TimestampQpc + 1)), new(512)), default);
        Assert.True(metrics.Items.Count >= 13);
        Assert.All(metrics.Items, metric => Assert.NotNull(metric.SourceObservation));
    }

    [Fact]
    public async Task Native_startup_failure_faults_readiness_and_persists_failed_health()
    {
        using var process = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(process));
        var path = NewPath(scope);
        var info = Info();
        using (var store = SessionStore.Create(path, scope, info, Quality()))
        {
            await using var recording = new LiveMetricRecording(store, info, Dll + ".missing", TimeSpan.FromSeconds(30));
            recording.Start();
            await Assert.ThrowsAsync<DllNotFoundException>(() => recording.Started.WaitAsync(TimeSpan.FromSeconds(10)));
            var result = await recording.Completion.WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(SessionStatus.Failed, result.Status);
            Assert.True(result.Health.Count(IngestionIssue.SourceFailure) > 0);
            Assert.Equal(0, result.DurableBoundary.IngestOrdinal);
        }
        using var reopened = SessionStore.OpenReadOnly(path);
        Assert.Contains(reopened.ReadIngestionDiagnostics(),
            diagnostic => diagnostic.Issue == IngestionIssue.SourceFailure && diagnostic.Count > 0);
    }

    [Fact]
    public async Task Native_identity_failure_stops_transport_and_persists_final_missing_metrics_and_loss()
    {
        using var process = Process.GetCurrentProcess();
        var actual = LiveEtwSource.Identify(process);
        var scope = new SessionScope(Guid.NewGuid(), new ProcessInstanceKey(
            actual.ProcessId, actual.CreationTimeFileTime + 1, actual.InstanceId));
        var path = NewPath(scope);
        var info = Info();
        using (var store = SessionStore.Create(path, scope, info, Quality()))
        {
            await using var recording = new LiveMetricRecording(store, info, Dll, TimeSpan.FromSeconds(30));
            recording.Start();
            await recording.Started.WaitAsync(TimeSpan.FromSeconds(10));
            var result = await recording.Completion.WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(SessionStatus.Failed, result.Status);
            Assert.True(result.Health.Count(IngestionIssue.SourceFailure) > 0);
            Assert.Equal(1, result.Health.Count(IngestionIssue.ProducerLoss));
            Assert.True(result.DurableBoundary.IngestOrdinal >= 15);
            Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
        }
        using var reopened = SessionStore.OpenReadOnly(path);
        Assert.Equal(ProcessOperation.Attach, Assert.Single(ReadProcessOperations(reopened)));
        var metrics = await reopened.Queries.GetMetrics(new(scope,
            new(0, checked(reopened.Watermark.TimestampQpc + 1)), new(512)), default);
        Assert.True(metrics.Items.Count >= 13);
        Assert.All(metrics.Items, metric =>
        {
            Assert.Equal(ValueKind.Missing, metric.Value.Kind);
            Assert.Equal(MetricSampleStatus.IdentityChanged, metric.SourceObservation!.Status);
        });
    }

    [Fact]
    public async Task Native_target_exit_drains_final_missing_sample_and_records_authoritative_exit_without_failure()
    {
        using var target = Process.Start(new ProcessStartInfo(
            Environment.GetEnvironmentVariable("ComSpec") ?? throw new InvalidOperationException("ComSpec is required."),
            "/d /q /c \"set /p WINUIPROFILER_EXIT= & exit /b 0\"")
        {
            RedirectStandardInput = true,
            CreateNoWindow = true,
            UseShellExecute = false
        }) ?? throw new InvalidOperationException("Could not start the owned target.");
        try
        {
            var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(target));
            var path = NewPath(scope);
            var info = Info();
            using (var store = SessionStore.Create(path, scope, info, Quality()))
            {
                await using var recording = new LiveMetricRecording(store, info, Dll, TimeSpan.FromMilliseconds(20));
                recording.Start();
                await recording.Started.WaitAsync(TimeSpan.FromSeconds(10));
                await target.StandardInput.WriteLineAsync();
                await target.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(10));
                var result = await recording.Completion.WaitAsync(TimeSpan.FromSeconds(10));
                Assert.Equal(SessionStatus.Stopped, result.Status);
                Assert.Equal(0, result.Health.Count(IngestionIssue.SourceFailure));
                Assert.Equal(0, result.Health.Count(IngestionIssue.ProducerLoss));
                Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
            }
            using var reopened = SessionStore.OpenReadOnly(path);
            Assert.Equal([ProcessOperation.Attach, ProcessOperation.Exit], ReadProcessOperations(reopened));
            var metrics = await reopened.Queries.GetMetrics(new(scope,
                new(0, checked(reopened.Watermark.TimestampQpc + 1)), new(512)), default);
            var final = metrics.Items.Where(metric =>
                metric.SourceObservation!.Status == MetricSampleStatus.TargetMissing).ToArray();
            Assert.True(final.Length >= 13);
            Assert.All(final, metric => Assert.Equal(ValueKind.Missing, metric.Value.Kind));
        }
        finally
        {
            if (!target.HasExited) await target.StandardInput.WriteLineAsync();
            await target.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(10));
        }
    }

    [Fact]
    public async Task Stopping_before_start_never_loads_native_modules_or_opens_transport()
    {
        using var target = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(target));
        var info = Info();
        using var store = SessionStore.Create(NewPath(scope), scope, info, Quality());
        await using var recording = new LiveMetricRecording(store, info, Dll + ".missing", TimeSpan.FromSeconds(30));
        var result = await recording.StopAsync().WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.Stopped, result.Status);
        Assert.Equal(0, result.DurableBoundary.IngestOrdinal);
        Assert.Equal(0, result.Health.Count(IngestionIssue.SourceFailure));
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => recording.Started);
        Assert.Throws<InvalidOperationException>(() => recording.Start());
    }

    [Fact]
    public async Task Precanceled_start_does_not_load_native_modules_or_open_transport()
    {
        using var target = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(target));
        var info = Info();
        using var store = SessionStore.Create(NewPath(scope), scope, info, Quality());
        await using var recording = new LiveMetricRecording(store, info, Dll + ".missing", TimeSpan.FromSeconds(30));
        recording.Start(new CancellationToken(true));
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => recording.Started.WaitAsync(TimeSpan.FromSeconds(10)));
        var result = await recording.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.Stopped, result.Status);
        Assert.Equal(0, result.DurableBoundary.IngestOrdinal);
        Assert.Equal(0, result.Health.Count(IngestionIssue.SourceFailure));
    }

    [Fact]
    public async Task Caller_cancellation_drains_first_publication_and_commits_without_source_failure()
    {
        using var target = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(target));
        var info = Info();
        using var store = SessionStore.Create(NewPath(scope), scope, info, Quality());
        using var cancellation = new CancellationTokenSource();
        await using var recording = new LiveMetricRecording(store, info, Dll, TimeSpan.FromSeconds(30));
        recording.Start(cancellation.Token);
        await recording.Started.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Throws<InvalidOperationException>(() => recording.Start());
        await cancellation.CancelAsync();
        var result = await recording.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.Stopped, result.Status);
        Assert.True(result.DurableBoundary.IngestOrdinal >= 14);
        Assert.Equal(0, result.Health.Count(IngestionIssue.SourceFailure));
        Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
        Assert.Equal(result, await recording.StopAsync());
    }

    [Fact]
    public async Task Processing_failure_cancels_native_polling_without_deadlocking_acquisition_finalization()
    {
        using var target = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(target));
        var info = Info();
        using var store = SessionStore.Create(NewPath(scope), scope, info, Quality());
        await using var recording = new LiveMetricRecording(store, info, Dll, TimeSpan.FromSeconds(30),
            processors: [new FailingProcessor()]);
        recording.Start();
        try { await recording.Started.WaitAsync(TimeSpan.FromSeconds(10)); }
        catch (OperationCanceledException) { }
        var result = await recording.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.Failed, result.Status);
        Assert.True(result.Health.Count(IngestionIssue.SourceFailure) > 0);
        Assert.Equal(result, await recording.StopAsync());
    }

    [Fact]
    public async Task Statistics_failure_keeps_etw_alive_until_inflight_native_publication_and_loss_are_drained()
    {
        using var target = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(target));
        var info = Info();
        var path = NewPath(scope);
        using var release = new ManualResetEventSlim();
        var inFlight = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var transportStopped = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var stoppedBeforeNativeCompletion = false;
        NativeMetricSession? sampling = null;
        var queryError = new IOException("Injected owned statistics-query failure.");
        var source = new LiveEtwSource(scope, info.QpcFrequency, name =>
        {
            if (inFlight.Task.IsCompleted) throw queryError;
            return EtwSessionStatistics.Query(name);
        }, name =>
        {
            stoppedBeforeNativeCompletion = sampling?.Completion.IsCompleted != true;
            transportStopped.TrySetResult();
            return EtwSessionStatistics.Stop(name);
        });
        using (var store = SessionStore.Create(path, scope, info, Quality()))
        {
            await using var recording = new LiveMetricRecording(store, info, Dll, TimeSpan.FromSeconds(30),
                null, null, (_, _) => source, publish =>
                {
                    sampling = new(Dll, scope.Process, TimeSpan.FromSeconds(30), batch =>
                    {
                        inFlight.TrySetResult();
                        if (!release.Wait(TimeSpan.FromSeconds(10)))
                            throw new TimeoutException("Held native publication was not released.");
                        return publish(batch);
                    });
                    return sampling;
                });
            recording.Start();
            try
            {
                await inFlight.Task.WaitAsync(TimeSpan.FromSeconds(10));
                Assert.Same(queryError, await source.ControlFailure.WaitAsync(TimeSpan.FromSeconds(10)));
                await Assert.ThrowsAsync<IOException>(() => recording.Started.WaitAsync(TimeSpan.FromSeconds(10)));
                Assert.False(transportStopped.Task.IsCompleted,
                    "A statistics-query failure must not stop usable ETW while native publication is held.");
            }
            finally { release.Set(); }
            var result = await recording.Completion.WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(SessionStatus.Failed, result.Status);
            Assert.False(stoppedBeforeNativeCompletion);
            Assert.True(transportStopped.Task.IsCompletedSuccessfully);
            Assert.True(result.Health.Count(IngestionIssue.SourceFailure) > 0);
            Assert.Equal(1, result.Health.Count(IngestionIssue.ProducerLoss));
            Assert.True(result.DurableBoundary.IngestOrdinal >= 15);
            Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
        }
        using var reopened = SessionStore.OpenReadOnly(path);
        var metrics = await reopened.Queries.GetMetrics(new(scope,
            new(0, checked(reopened.Watermark.TimestampQpc + 1)), new(512)), default);
        Assert.True(metrics.Items.Count >= 13);
        Assert.Contains(reopened.ReadIngestionDiagnostics(),
            diagnostic => diagnostic.Issue == IngestionIssue.ProducerLoss && diagnostic.Count == 1);
    }

    [Fact]
    public async Task Ingress_byte_limit_quiesces_sources_and_preserves_explicit_stopped_at_limit_state()
    {
        using var target = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(target));
        var info = Info();
        using var store = SessionStore.Create(NewPath(scope), scope, info, Quality());
        await using var recording = new LiveMetricRecording(store, info, Dll, TimeSpan.FromSeconds(30),
            new(IngressBytes: 1024));
        recording.Start();
        var readinessError = await Record.ExceptionAsync(() => recording.Started.WaitAsync(TimeSpan.FromSeconds(10)));
        Assert.True(readinessError is null or OperationCanceledException);
        var result = await recording.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.StoppedAtLimit, result.Status);
        Assert.Equal(1, result.Health.Count(IngestionIssue.QueueOverflow));
        Assert.Equal(0, result.Health.Count(IngestionIssue.SourceFailure));
        Assert.Equal(result, await recording.StopAsync());
    }

    [Fact]
    public async Task Concurrent_stop_and_dispose_share_one_complete_durable_shutdown()
    {
        using var target = Process.GetCurrentProcess();
        var scope = new SessionScope(Guid.NewGuid(), LiveEtwSource.Identify(target));
        var info = Info();
        using var store = SessionStore.Create(NewPath(scope), scope, info, Quality());
        await using var recording = new LiveMetricRecording(store, info, Dll, TimeSpan.FromSeconds(30));
        recording.Start();
        await recording.Started.WaitAsync(TimeSpan.FromSeconds(10));
        var stops = Enumerable.Range(0, 8).Select(_ => Task.Run(() => recording.StopAsync())).ToArray();
        var disposals = Enumerable.Range(0, 4).Select(_ =>
            Task.Run(async () => await recording.DisposeAsync())).ToArray();
        await Task.WhenAll(stops.Cast<Task>().Concat(disposals)).WaitAsync(TimeSpan.FromSeconds(10));
        var result = await recording.Completion;
        Assert.All(stops, completed => Assert.Equal(result, completed.Result));
        Assert.Equal(SessionStatus.Stopped, result.Status);
        Assert.True(result.DurableBoundary.IngestOrdinal >= 14);
        Assert.Equal(0, result.Health.Count(IngestionIssue.SourceFailure));
        Assert.Equal(result, await recording.StopAsync());
        Assert.Throws<ObjectDisposedException>(() => recording.Start());
    }

    private sealed class FailingProcessor : IEventProcessor
    {
        public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context) =>
            throw new InvalidOperationException("Test processor rejected the admitted source batch.");
        public void Invalidate(HealthSnapshot health) { }
    }

    private static IReadOnlyList<ProcessOperation> ReadProcessOperations(SessionStore store) =>
        ReadProcessPayloads(store).Select(payload => payload.Operation).ToArray();

    private static IReadOnlyList<ProcessPayload> ReadProcessPayloads(SessionStore store)
    {
        using var db = store.OpenReader();
        using var command = db.CreateCommand();
        command.CommandText = "SELECT Envelope FROM SourceEvents WHERE ProviderId=$provider AND EventId=1 ORDER BY Ordinal LIMIT 512";
        command.Parameters.AddWithValue("$provider", EventCatalog.CollectorProviderId.ToString("N"));
        using var rows = command.ExecuteReader();
        var operations = new List<ProcessPayload>();
        while (rows.Read())
            operations.Add(EventCodec.ReadPayload<ProcessPayload>(EventCodec.Decode((byte[])rows[0])));
        return operations;
    }

    private string NewPath(SessionScope scope)
    {
        var path = Path.GetFullPath(Path.Combine("TestResults", scope.SessionId + ".sqlite"));
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        databases.Add(path);
        return path;
    }

    public void Dispose()
    {
        foreach (var path in databases)
            foreach (var suffix in new[] { "", "-wal", "-shm", ".writer", ".failure.json" })
                if (File.Exists(path + suffix)) File.Delete(path + suffix);
    }

    private static SessionInfo Info() =>
        new(SessionMode.LiveAttach, SessionStatus.Recording, Stopwatch.Frequency, null, [], null);
    private static DataQuality Quality() =>
        new(MeasurementKind.Measured, CoverageState.Partial, "Native live-recording lifecycle test; no runtime baseline or parity claim.");
}

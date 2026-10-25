using System.Diagnostics;
using System.Runtime.InteropServices;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.IntegrationTests;

public sealed class NativeMetricPollingTests
{
    private static string Dll => Path.Combine(
        Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\..")),
        @"tools\WinUIProfiler\WinUIProfiler.Native\bin",
        RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "ARM64" : "x64",
        @"Release\WinUIProfiler.Native.dll");

    [Fact]
    public async Task Default_native_polling_uses_fresh_graphics_inventory_not_active_engine_rows()
    {
        using var inventorySource = new NativeGpuAdapterInventory(Dll);
        var inventory = inventorySource.Capture();
        var expected = inventory.SelectMemoryAdapters(false);
        using var process = Process.GetCurrentProcess();
        var ready = new TaskCompletionSource<NativeMetricBatch>(TaskCreationOptions.RunContinuationsAsynchronously);
        await using var session = new NativeMetricSession(Dll, Identity(process), TimeSpan.FromSeconds(10), batch =>
        {
            ready.TrySetResult(batch);
            return true;
        });
        session.Start();
        var batch = await ready.Task.WaitAsync(TimeSpan.FromSeconds(5));
        await session.StopAsync();
        var memory = Assert.IsAssignableFrom<IReadOnlyList<ProcessGpuMemorySample>>(batch.GpuMemory);
        Assert.Equal(expected, memory.Select(sample => sample.Adapter));
        Assert.All(memory, sample => Assert.Equal(batch.Process, sample.Process));
    }

    [Fact]
    public async Task Explicit_adapter_memory_is_sampled_inside_the_owned_polling_lifetime()
    {
        using var process = Process.GetCurrentProcess();
        var target = Identity(process);
        var adapter = new PhysicalAdapterKey(uint.MaxValue, int.MaxValue, 0);
        var ready = new TaskCompletionSource<NativeMetricBatch>(TaskCreationOptions.RunContinuationsAsynchronously);
        await using var session = new NativeMetricSession(Dll, target, TimeSpan.FromSeconds(10), batch =>
        {
            ready.TrySetResult(batch);
            return true;
        }, gpuMemoryAdapters: [adapter]);
        session.Start();
        var batch = await ready.Task.WaitAsync(TimeSpan.FromSeconds(5));
        await session.StopAsync();
        var sample = Assert.Single(batch.GpuMemory!);
        Assert.Equal(target, sample.Process);
        Assert.Equal(adapter, sample.Adapter);
        Assert.Equal(2u, sample.Status);
        var payloads = NativeMetricPayloads.Create(batch, new(batch.QpcFrequency, batch.QpcFrequency));
        Assert.Equal(17 + batch.Gpu.Sample.Engines.Count, payloads.Count);
        var memory = payloads.Where(payload => payload.Gpu?.Scope == GpuMetricScope.PerAdapterMemory).ToArray();
        Assert.Equal(4, memory.Length);
        Assert.All(memory, payload =>
        {
            Assert.Equal(ValueKind.Missing, payload.Value.Kind);
            Assert.Equal(MetricClock.Boot100ns, payload.SourceObservation!.Clock);
            Assert.Equal(new TimeRange(batch.BeginQpc, batch.EndQpc), payload.Interval);
        });
    }

    [Fact]
    public async Task Actual_cpu_memory_and_gpu_poll_without_ui_and_keep_original_clock_scopes()
    {
        using var process = Process.GetCurrentProcess();
        var target = Identity(process);
        var ready = new TaskCompletionSource<NativeMetricBatch>(TaskCreationOptions.RunContinuationsAsynchronously);
        var samples = new List<NativeMetricBatch>();
        await using var session = new NativeMetricSession(Dll, target, TimeSpan.FromMilliseconds(50), batch =>
        {
            samples.Add(batch);
            if (samples.Count == 3) ready.SetResult(batch);
            return true;
        });
        session.Start();
        await ready.Task.WaitAsync(TimeSpan.FromSeconds(5));
        await session.StopAsync();
        Assert.True(samples.Count >= 3);
        Assert.Equal(1u, samples[0].Cpu.Sample.Status);
        Assert.Contains(samples.Skip(1), batch => batch.Cpu.Sample.SystemUtilization.Kind == ValueKind.Number);
        foreach (var batch in samples)
        {
            Assert.Equal(target, batch.Process);
            Assert.Equal(Stopwatch.Frequency, batch.QpcFrequency);
            Assert.True(batch.BeginQpc > 0 && batch.EndQpc >= batch.BeginQpc);
            Assert.Equal(2ul, batch.Cpu.Sample.ClockId);
            Assert.Equal(2ul, batch.Memory.ClockId);
            Assert.True(batch.Memory.End100ns >= batch.Memory.Begin100ns);
            Assert.True(batch.Memory.PrivateCommit.Integer > 0);
            Assert.Equal(target, batch.Gpu.Sample.Process);
            Assert.Equal(3ul, batch.Gpu.Sample.ClockId);
            Assert.Equal(2ul, batch.Gpu.AcquisitionClockId);
            Assert.True(batch.Gpu.AcquisitionEnd100ns >= batch.Gpu.AcquisitionBegin100ns);
            if (batch.Gpu.RowCount == 0)
                Assert.Equal(ValueKind.Missing, batch.Gpu.Sample.Utilization.Kind);
            var payloads = NativeMetricPayloads.Create(batch, new(batch.QpcFrequency, batch.QpcFrequency));
            Assert.Equal(13 + batch.Gpu.Sample.Engines.Count + 4 * (batch.GpuMemory?.Count ?? 0), payloads.Count);
            Assert.All(payloads, payload => Assert.NotNull(payload.SourceObservation));
        }
    }

    [Fact]
    public async Task Actual_polled_metrics_reach_common_owner_and_sqlite_without_clock_relabel()
    {
        using var process = Process.GetCurrentProcess();
        var target = Identity(process);
        var scope = new SessionScope(Guid.NewGuid(), target);
        var epoch = Guid.NewGuid();
        var path = Path.GetFullPath(Path.Combine("TestResults", scope.SessionId + ".sqlite"));
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        var quality = new DataQuality(MeasurementKind.Measured, CoverageState.Partial,
            "Actual native polling with typed ingress; not collector ETW wire or parity evidence.");
        var info = new SessionInfo(SessionMode.LiveAttach, SessionStatus.Recording, Stopwatch.Frequency,
            null, [], null);
        var clock = new SourceClockMetadata(Guid.NewGuid(), 1, new(Stopwatch.Frequency, Stopwatch.Frequency));
        var ready = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var admitted = 0;
        ulong sequence = 0;
        try
        {
            using (var store = SessionStore.Create(path, scope, info, quality))
            {
                await using var worker = new ProcessingWorker(store, info, processors: [new MetricProcessor()]);
                await using var sampling = new NativeMetricSession(Dll, target, TimeSpan.FromSeconds(10), batch =>
                {
                    foreach (var payload in NativeMetricPayloads.Create(batch, clock.Mapping))
                    {
                        var identity = EventCatalog.Get(EventCatalog.CollectorProviderId, 2).Identity;
                        var envelope = new EventEnvelope(scope.SessionId, target, checked((uint)Environment.ProcessId),
                            GetCurrentThreadId(), batch.EndQpc, 1, checked(++sequence), null, identity,
                            EventCodec.EncodePayload(payload, target));
                        var decoded = new DecodedEvent(EventCodec.Normalize(envelope,
                            new(identity.ProviderId, envelope.ProducerProcessId, epoch)),
                            new(target, batch.EndQpc, clock));
                        if (!worker.TryEnqueue(decoded)) return false;
                        admitted++;
                    }
                    ready.TrySetResult();
                    return true;
                });
                sampling.Start();
                await ready.Task.WaitAsync(TimeSpan.FromSeconds(10));
                await sampling.StopAsync();
                worker.Complete(SourceCompletion.UserStop);
                var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
                Assert.Equal(SessionStatus.Stopped, result.Status);
                Assert.Equal(admitted, result.DurableBoundary.IngestOrdinal);
            }
            using var reopened = SessionStore.OpenReadOnly(path);
            var resultRows = await reopened.Queries.GetMetrics(new(scope,
                new(0, checked(reopened.Watermark.TimestampQpc + 1)), new(512)), default);
            Assert.Equal(admitted, resultRows.Items.Count);
            Assert.True(admitted >= 13);
            Assert.Equal(clock, Assert.Single(reopened.ReadSourceClocks()));
            var cpu = resultRows.Items.First(p => p.Metric == MetricKind.CpuUtilization);
            var gpu = resultRows.Items.First(p => p.Metric == MetricKind.GpuUtilization);
            Assert.Equal(MetricClock.Boot100ns, cpu.SourceObservation!.Clock);
            Assert.Equal(MetricClock.UtcFileTime100ns, gpu.SourceObservation!.Clock);
            Assert.Equal(scope, resultRows.Context.Scope);
            Assert.All(resultRows.Items, row => Assert.Equal(10_000_000, row.SourceObservation!.Frequency));
        }
        finally
        {
            foreach (var suffix in new[] { "", "-wal", "-shm", ".writer", ".failure.json" })
                if (File.Exists(path + suffix)) File.Delete(path + suffix);
        }
    }

    [Fact]
    public async Task Rejected_ingress_stops_polling_instead_of_silently_dropping_metrics()
    {
        using var process = Process.GetCurrentProcess();
        var count = 0;
        var session = new NativeMetricSession(Dll, Identity(process), TimeSpan.FromMilliseconds(10), _ =>
        {
            Interlocked.Increment(ref count);
            return false;
        });
        session.Start();
        var error = await Assert.ThrowsAsync<InvalidOperationException>(
            () => session.Completion.WaitAsync(TimeSpan.FromSeconds(5)));
        Assert.Contains("ingress rejected", error.Message);
        Assert.Null(session.TerminationStatus);
        Assert.Equal(1, Volatile.Read(ref count));
        Assert.Same(error, await Assert.ThrowsAsync<InvalidOperationException>(() => session.DisposeAsync().AsTask()));
    }

    [Fact]
    public async Task Selected_identity_mismatch_publishes_missing_final_sample_then_stops()
    {
        using var process = Process.GetCurrentProcess();
        var actual = Identity(process);
        var wrong = new ProcessInstanceKey(actual.ProcessId, actual.CreationTimeFileTime + 1, Guid.NewGuid());
        NativeMetricBatch? observed = null;
        var session = new NativeMetricSession(Dll, wrong, TimeSpan.FromMilliseconds(10), batch =>
        {
            observed = batch;
            return true;
        });
        session.Start();
        var error = await Assert.ThrowsAsync<InvalidOperationException>(
            () => session.Completion.WaitAsync(TimeSpan.FromSeconds(5)));
        Assert.Contains("identity changed", error.Message);
        Assert.NotNull(observed);
        Assert.Equal(6u, observed.Cpu.Sample.Status);
        Assert.Equal(ValueKind.Missing, observed.Memory.PrivateCommit.Kind);
        Assert.Equal(6u, observed.Gpu.Sample.Status);
        Assert.Equal(ValueKind.Missing, observed.Gpu.Sample.Utilization.Kind);
        Assert.Equal(MetricSampleStatus.IdentityChanged, session.TerminationStatus);
        Assert.Same(error, await Assert.ThrowsAsync<InvalidOperationException>(() => session.DisposeAsync().AsTask()));
    }

    private static ProcessInstanceKey Identity(Process process) =>
        new(checked((uint)process.Id), process.StartTime.ToUniversalTime().ToFileTimeUtc(), Guid.NewGuid());

    [DllImport("kernel32.dll", ExactSpelling = true)]
    private static extern uint GetCurrentThreadId();
}

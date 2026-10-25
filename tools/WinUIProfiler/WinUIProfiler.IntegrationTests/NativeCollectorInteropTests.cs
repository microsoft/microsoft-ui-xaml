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
public sealed class NativeCollectorInteropTests
{
    [Fact]
    public async Task Actual_native_polling_roundtrips_collector_etw_common_owner_and_sqlite_reopen()
    {
        using var process = Process.GetCurrentProcess();
        var target = LiveEtwSource.Identify(process);
        var scope = new SessionScope(Guid.NewGuid(), target);
        var path = Path.GetFullPath(Path.Combine("TestResults", scope.SessionId + ".sqlite"));
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        var dll = Path.Combine(Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\..")),
            @"tools\WinUIProfiler\WinUIProfiler.Native\bin",
            RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "ARM64" : "x64",
            @"Release\WinUIProfiler.Native.dll");
        var info = new SessionInfo(SessionMode.LiveAttach, SessionStatus.Recording, 3_000_001, null, [], null);
        var quality = new DataQuality(MeasurementKind.Measured, CoverageState.Partial,
            "Actual native sources and collector transport; no aligned parity or complete capability baseline.");
        var projection = new ObservedMetrics();
        try
        {
            using (var store = SessionStore.Create(path, scope, info, quality))
            {
                await using var worker = new ProcessingWorker(store, info, processors: [projection]);
                await using var source = new LiveEtwSource(scope, info.QpcFrequency);
                var emitter = new CollectorMetricEmitter(scope);
                _ = source.RunAsync(worker);
                await source.Started.WaitAsync(TimeSpan.FromSeconds(10));
                await using var sampling = new NativeMetricSession(dll, target, TimeSpan.FromSeconds(10), batch =>
                {
                    projection.Expect(NativeMetricPayloads.Create(batch,
                        new(batch.QpcFrequency, batch.QpcFrequency)).Count);
                    return emitter.TryEmit(batch);
                });
                sampling.Start();
                await projection.Ready.WaitAsync(TimeSpan.FromSeconds(10));
                await sampling.StopAsync();
                await source.StopAsync();
                var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
                Assert.Equal(SessionStatus.Stopped, result.Status);
                Assert.Equal(projection.Expected, result.DurableBoundary.IngestOrdinal);
                Assert.Equal(0, result.Health.Count(IngestionIssue.DecodeFailure));
                Assert.Equal(0, result.Health.Count(IngestionIssue.UnknownSchema));
                Assert.Equal(0, result.Health.Count(IngestionIssue.EtwEventsLost));
                Assert.Equal(0, result.Health.Count(IngestionIssue.EtwBuffersLost));
            }
            using var reopened = SessionStore.OpenReadOnly(path);
            var rows = await reopened.Queries.GetMetrics(new(scope,
                new(0, checked(reopened.Watermark.TimestampQpc + 1)), new(512)), default);
            Assert.Equal(projection.Expected, rows.Items.Count);
            Assert.True(rows.Items.Count >= 13);
            Assert.Equal(scope, rows.Context.Scope);
            var clock = Assert.Single(reopened.ReadSourceClocks());
            Assert.Equal(Stopwatch.Frequency, clock.Mapping.SourceFrequency);
            Assert.Equal(info.QpcFrequency, clock.Mapping.SessionFrequency);
            var cpu = Assert.Single(rows.Items, row => row.Metric == MetricKind.CpuUtilization);
            Assert.Equal(MetricClock.Boot100ns, cpu.SourceObservation!.Clock);
            var gpu = Assert.Single(rows.Items, row => row.Metric == MetricKind.GpuUtilization &&
                row.Gpu?.Scope != GpuMetricScope.PerEngine);
            Assert.Equal(MetricClock.UtcFileTime100ns, gpu.SourceObservation!.Clock);
            var commit = Assert.Single(rows.Items, row => row.Metric == MetricKind.ProcessPrivateCommit);
            Assert.True(commit.Value.Integer > 0);
            Assert.Equal(MetricClock.Boot100ns, commit.SourceObservation!.Clock);
            Assert.Equal(target, reopened.ReadSourceProvenance(1)!.OriginalTarget);
        }
        finally
        {
            foreach (var suffix in new[] { "", "-wal", "-shm", ".writer", ".failure.json" })
                if (File.Exists(path + suffix)) File.Delete(path + suffix);
        }
    }

    private sealed class ObservedMetrics : IEventProcessor
    {
        private readonly MetricProcessor inner = new();
        private readonly TaskCompletionSource ready = new(TaskCreationOptions.RunContinuationsAsynchronously);
        private int expected;
        private int count;
        internal Task Ready => ready.Task;
        internal int Expected => Volatile.Read(ref expected);
        internal void Expect(int value) => Volatile.Write(ref expected, value);
        public IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context)
        {
            foreach (var change in inner.Process(events, context))
            {
                yield return change;
                if (++count == Expected) ready.TrySetResult();
            }
        }
        public void Invalidate(HealthSnapshot health) => inner.Invalidate(health);
    }
}

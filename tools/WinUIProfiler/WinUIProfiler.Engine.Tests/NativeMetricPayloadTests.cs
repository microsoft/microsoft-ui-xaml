using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class NativeMetricPayloadTests
{
    private static readonly ProcessInstanceKey Target = new(123, 456, Guid.NewGuid());
    private static readonly QpcMapping Clock = new(1_000_000, 10_000_000, 100);

    [Fact]
    public void Maximum_distinct_adapter_and_engine_shape_has_exact_bounded_output()
    {
        var payloads = NativeMetricPayloads.Create(MaximumBatch(Batch()), Clock);
        Assert.Equal(1293, payloads.Count);
        Assert.Equal(256, payloads.Count(payload => payload.Gpu?.Scope == GpuMetricScope.PerEngine));
        var adapters = payloads.Where(payload => payload.Gpu?.Scope == GpuMetricScope.PerAdapterMemory)
            .GroupBy(payload => payload.Gpu!.Adapter).ToArray();
        Assert.Equal(256, adapters.Length);
        Assert.All(adapters, adapter => Assert.Equal(4, adapter.Count()));
        Assert.All(payloads, payload => EventValidation.Validate(payload, Target));
    }

    [Fact]
    public void Configured_memory_keeps_independent_adapter_bytes_and_original_clock_without_aggregate_alias()
    {
        var batch = Batch();
        var adapter = new PhysicalAdapterKey(789, -1, 2);
        var sample = GpuMemoryMetricCodec.Decode(GpuMemoryMetricTests.Buffer(), batch.Process, adapter);
        var payloads = NativeMetricPayloads.Create(batch with { GpuMemory = [sample] }, Clock);
        Assert.Equal(19, payloads.Count);
        var memory = payloads.Where(payload => payload.Gpu?.Scope == GpuMetricScope.PerAdapterMemory).ToArray();
        Assert.Equal(4, memory.Length);
        Assert.Equal(new long?[] { 10, 20, 30, 40 }, memory.Select(payload => payload.Value.Integer));
        Assert.All(memory, payload =>
        {
            Assert.Equal(adapter, payload.Gpu!.Adapter);
            Assert.Null(payload.Gpu.Engine);
            Assert.Equal(new TimeRange(3000, 4000), payload.Interval);
            Assert.Equal(MetricClock.Boot100ns, payload.SourceObservation!.Clock);
            Assert.Equal(100ul, payload.SourceObservation.Begin);
            Assert.Equal(payload, EventCodec.ReadPayload<MetricPayload>(Envelope(payload)));
        });
        Assert.All(payloads.Where(payload => ObservationRules.IsGpuMemory(payload.Metric) &&
            payload.Gpu is null), payload => Assert.Equal(ValueKind.Missing, payload.Value.Kind));
        Assert.Throws<ArgumentException>(() => NativeMetricPayloads.Create(
            batch with { GpuMemory = [sample, sample] }, Clock));
        Assert.Throws<ArgumentException>(() => NativeMetricPayloads.Create(
            batch with { GpuMemory = [sample with { Process = new(124, 457, Guid.NewGuid()) }] }, Clock));
    }

    [Theory]
    [InlineData(0x80004005u)]
    [InlineData(0x887A0005u)]
    [InlineData(1u)]
    public void Dxcore_hresult_provenance_roundtrips_without_relabelling_as_win32_or_ntstatus(uint hresult)
    {
        var payload = new MetricPayload(MetricKind.GpuDedicatedResidentBytes, Missing(ValueUnit.Bytes),
            new(3000, 4000), "DXCore AdapterMemoryUsageByProcessBytes", "1", null)
        {
            SourceObservation = new(MetricClock.Boot100ns, 10_000_000, 100, 200,
                MetricSampleStatus.Unavailable, 100, 200, hresult, MetricErrorDomain.HResult, 0)
        };
        var actual = EventCodec.ReadPayload<MetricPayload>(Envelope(payload));
        Assert.Equal(hresult, actual.SourceObservation!.NativeError);
        Assert.Equal(MetricErrorDomain.HResult, actual.SourceObservation.ErrorDomain);
        Assert.Equal(ValueKind.Missing, actual.Value.Kind);
        Assert.Equal(payload, actual);
    }

    [Fact]
    public void Payloads_keep_acquisition_qpc_and_original_native_intervals_separate()
    {
        var payloads = NativeMetricPayloads.Create(Batch(), Clock);
        Assert.Equal(15, payloads.Count);
        foreach (var payload in payloads)
        {
            Assert.Equal(new TimeRange(3000, 4000), payload.Interval);
            Assert.NotNull(payload.SourceObservation);
            payload.SourceObservation.Validate();
            Assert.Equal(payload, EventCodec.ReadPayload<MetricPayload>(Envelope(payload)));
        }
        var cpu = Assert.Single(payloads, p => p.Metric == MetricKind.CpuUtilization);
        Assert.Equal(10, cpu.Value.Number);
        Assert.Equal(MetricClock.Boot100ns, cpu.SourceObservation!.Clock);
        Assert.Equal(100ul, cpu.SourceObservation.Begin);
        Assert.Equal(200ul, cpu.SourceObservation.End);
        var gpu = Assert.Single(payloads, p => p.Metric == MetricKind.GpuUtilization &&
            p.Gpu!.Scope == GpuMetricScope.OverallBusiestEngine);
        Assert.Equal(MetricClock.UtcFileTime100ns, gpu.SourceObservation!.Clock);
        Assert.Equal(134000000000000000ul, gpu.SourceObservation.Begin);
        Assert.Equal(170ul, gpu.SourceObservation.AcquisitionBegin);
        Assert.Equal(180ul, gpu.SourceObservation.AcquisitionEnd);
    }

    [Fact]
    public void Cpu_aggregates_and_memory_gauges_are_distinct_and_utility_is_unavailable()
    {
        var payloads = NativeMetricPayloads.Create(Batch(), Clock);
        Assert.Equal(60, payloads.Single(p => p.Metric == MetricKind.SystemCpuUtilization).Value.Number);
        Assert.Equal(5, payloads.Single(p => p.Metric == MetricKind.InterruptCpuUtilization).Value.Number);
        Assert.Equal(45, payloads.Single(p => p.Metric == MetricKind.OtherOrUnattributedCpuUtilization).Value.Number);
        Assert.Equal(ValueKind.Missing, payloads.Single(p => p.Metric == MetricKind.ProcessCpuUtility).Value.Kind);
        Assert.Equal(10, payloads.Single(p => p.Metric == MetricKind.ProcessPrivateWorkingSet).Value.Integer);
        Assert.Equal(20, payloads.Single(p => p.Metric == MetricKind.ProcessWorkingSet).Value.Integer);
        Assert.Equal(100, payloads.Single(p => p.Metric == MetricKind.ProcessPrivateCommit).Value.Integer);
    }

    [Fact]
    public void Gpu_identity_uses_the_busiest_engine_not_summed_percent_or_memory_aliases()
    {
        var payloads = NativeMetricPayloads.Create(Batch(), Clock);
        var gpu = payloads.Where(p => p.Metric == MetricKind.GpuUtilization).ToArray();
        Assert.Equal(3, gpu.Length);
        var overall = Assert.Single(gpu, p => p.Gpu!.Scope == GpuMetricScope.OverallBusiestEngine);
        Assert.Equal(70, overall.Value.Number);
        Assert.Equal(new PhysicalAdapterKey(123, -1, 0), overall.Gpu!.Adapter);
        Assert.Equal(new GpuEngineKey(1, GpuEngineClass.Unknown), overall.Gpu.Engine);
        Assert.Equal(new double?[] { 20, 70 }, gpu.Where(p => p.Gpu!.Scope == GpuMetricScope.PerEngine)
            .Select(p => p.Value.Number));
        Assert.All(payloads.Where(p => ObservationRules.IsGpuMemory(p.Metric)), p =>
        {
            Assert.Equal(ValueKind.Missing, p.Value.Kind);
            Assert.Null(p.Gpu);
            Assert.Null(p.Value.Integer);
        });
    }

    [Fact]
    public void Foreign_native_sample_and_mismatched_qpc_frequency_are_rejected()
    {
        var batch = Batch();
        var foreign = batch with { Cpu = batch.Cpu with { Sample = batch.Cpu.Sample with {
            Process = new(124, 457, Guid.NewGuid()) } } };
        Assert.Throws<ArgumentException>(() => NativeMetricPayloads.Create(foreign, Clock));
        Assert.Throws<ArgumentException>(() => NativeMetricPayloads.Create(batch, Clock with { SourceFrequency = 999 }));
    }

    private static EventEnvelope Envelope(MetricPayload payload) => new(Guid.NewGuid(), Target, 999, 7,
        4000, 1, 1, null, EventCatalog.Get(EventCatalog.CollectorProviderId, 2).Identity,
        EventCodec.EncodePayload(payload, Target));

    internal static NativeMetricBatch Batch()
    {
        var missingPercent = Missing(ValueUnit.Percent);
        var missingBytes = Missing(ValueUnit.Bytes);
        var cpu = new ProcessCpuSample(Target, 2, 100, 200, "synthetic CPU fixture", 0,
            Percent(60), Percent(10), Percent(5), Percent(45), missingPercent);
        var memory = new ProcessMemorySample(Target, 2, 150, 160, "synthetic memory fixture", "1", 0,
            Bytes(10), Bytes(20), Bytes(100));
        var gpu = new ProcessGpuSample(Target, 3, 134000000000000000, 134000000000100000,
            0, Percent(70), Array.AsReadOnly(new[] {
                new GpuEngineSample(uint.MaxValue, 123, 0, 0, Percent(20)),
                new GpuEngineSample(uint.MaxValue, 123, 0, 1, Percent(70)) }),
            missingBytes, missingBytes, missingBytes, missingBytes);
        return new(Target, 400, 500, 1_000_000, new(cpu, 110, 120, 0, 0), memory,
            new(gpu, 2, 170, 180, 2, 0, 0));
    }

    internal static NativeMetricBatch MaximumBatch(NativeMetricBatch batch)
    {
        var engine = batch.Gpu.Sample.Engines[0];
        var engines = Enumerable.Range(0, 256).Select(index => engine with { Engine = (uint)index }).ToArray();
        var memory = GpuMemoryMetricCodec.Decode(GpuMemoryMetricTests.Buffer(), batch.Process, new(789, -1, 2));
        return batch with
        {
            Gpu = batch.Gpu with
            {
                RowCount = 256,
                Sample = batch.Gpu.Sample with { Utilization = engine.Utilization, Engines = Array.AsReadOnly(engines) }
            },
            GpuMemory = Array.AsReadOnly(Enumerable.Range(0, 256)
                .Select(index => memory with { Adapter = new((uint)index, -1, 2) }).ToArray())
        };
    }

    private static TypedValue Percent(double value) => new(ValueKind.Number, ValueUnit.Percent,
        null, value, null, null, DataQuality.Complete);
    private static TypedValue Bytes(long value) => new(ValueKind.Integer, ValueUnit.Bytes,
        value, null, null, null, DataQuality.Complete);
    private static TypedValue Missing(ValueUnit unit) => new(ValueKind.Missing, unit,
        null, null, null, null, new(MeasurementKind.Unavailable, CoverageState.Unavailable, "Source unavailable."));
}

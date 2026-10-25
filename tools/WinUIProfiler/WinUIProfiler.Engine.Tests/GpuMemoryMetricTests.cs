using System.Buffers.Binary;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class GpuMemoryMetricTests
{
    private static readonly ProcessInstanceKey Target = new(123, 456, Guid.NewGuid());
    private static readonly PhysicalAdapterKey Adapter = new(789, -1, 2);

    [Fact]
    public void Independent_committed_resident_gauges_retain_exact_adapter_clocks_and_values()
    {
        var sample = GpuMemoryMetricCodec.Decode(Buffer(), Target, Adapter);
        Assert.Equal(Target, sample.Process);
        Assert.Equal(Adapter, sample.Adapter);
        Assert.Equal(100ul, sample.Begin100ns);
        Assert.Equal(200ul, sample.End100ns);
        Assert.Equal(new long?[] { 10, 20, 30, 40 },
            new[] { sample.DedicatedCommitted.Integer, sample.SharedCommitted.Integer,
                sample.DedicatedResident.Integer, sample.SharedResident.Integer });
        Assert.Equal("DXCore AdapterMemoryUsageByProcessBytes", sample.Source);
        Assert.Equal(0u, sample.Status);
        Assert.Equal(3u, sample.ProcessQueryFlags);
    }

    [Fact]
    public void Measured_zero_and_successful_informational_hresult_are_not_missing()
    {
        var bytes = Buffer();
        U32(bytes, 72, 1);
        foreach (var offset in new[] { 96, 112, 128, 144 }) U64(bytes, offset, 0);
        var sample = GpuMemoryMetricCodec.Decode(bytes, Target, Adapter);
        Assert.Equal(1u, sample.DedicatedHResult);
        Assert.All(Values(sample), value =>
        {
            Assert.Equal(ValueKind.Integer, value.Kind);
            Assert.Equal(0, value.Integer);
        });
    }

    [Fact]
    public void Partial_family_preserves_hresult_and_missing_sentinel_without_zero_substitution()
    {
        var bytes = Buffer();
        U32(bytes, 12, 3); U32(bytes, 76, 0x887A0005); U32(bytes, 84, 1);
        Missing(bytes, 104, 2); Missing(bytes, 136, 2);
        var sample = GpuMemoryMetricCodec.Decode(bytes, Target, Adapter);
        Assert.Equal(10, sample.DedicatedCommitted.Integer);
        Assert.Equal(30, sample.DedicatedResident.Integer);
        Assert.Equal(0x887A0005u, sample.SharedHResult);
        foreach (var value in new[] { sample.SharedCommitted, sample.SharedResident })
        {
            Assert.Equal(ValueKind.Missing, value.Kind);
            Assert.Null(value.Integer);
            Assert.Contains("887A0005", value.Quality.Reason);
        }
    }

    [Fact]
    public void Memory_payloads_keep_family_hresult_and_boot_interval_separate_from_session_qpc()
    {
        var bytes = Buffer();
        U32(bytes, 12, 3); U32(bytes, 76, 0x887A0005); U32(bytes, 84, 1);
        Missing(bytes, 104, 2); Missing(bytes, 136, 2);
        var sample = GpuMemoryMetricCodec.Decode(bytes, Target, Adapter);
        var payloads = NativeMetricPayloads.Create(sample, new(400, 500),
            new(1_000_000, 10_000_000, 100));
        Assert.Equal(4, payloads.Count);
        Assert.Equal(new[] { MetricKind.GpuDedicatedCommittedBytes, MetricKind.GpuSharedCommittedBytes,
            MetricKind.GpuDedicatedResidentBytes, MetricKind.GpuSharedResidentBytes },
            payloads.Select(value => value.Metric));
        foreach (var payload in payloads)
        {
            Assert.Equal(new TimeRange(3000, 4000), payload.Interval);
            Assert.Equal(Adapter, payload.Gpu!.Adapter);
            Assert.Equal(GpuMetricScope.PerAdapterMemory, payload.Gpu.Scope);
            Assert.Null(payload.Gpu.Engine);
            Assert.Equal(MetricClock.Boot100ns, payload.SourceObservation!.Clock);
            Assert.Equal(100ul, payload.SourceObservation.Begin);
            Assert.Equal(200ul, payload.SourceObservation.End);
            EventValidation.Validate(payload, Target);
            if (payload.Metric is MetricKind.GpuSharedCommittedBytes or MetricKind.GpuSharedResidentBytes)
            {
                Assert.Equal(MetricSampleStatus.Unavailable, payload.SourceObservation.Status);
                Assert.Equal(MetricErrorDomain.HResult, payload.SourceObservation.ErrorDomain);
                Assert.Equal(0x887A0005u, payload.SourceObservation.NativeError);
            }
            else
            {
                Assert.Equal(MetricSampleStatus.Measured, payload.SourceObservation.Status);
                Assert.Equal(MetricErrorDomain.None, payload.SourceObservation.ErrorDomain);
            }
        }
    }

    [Theory]
    [InlineData(2u)]
    [InlineData(5u)]
    [InlineData(6u)]
    public void Unavailable_or_terminal_source_suppresses_all_gauges_even_with_raw_success_flags(uint status)
    {
        var bytes = Buffer();
        U32(bytes, 12, status);
        foreach (var offset in new[] { 88, 104, 120, 136 }) Missing(bytes, offset, status);
        var sample = GpuMemoryMetricCodec.Decode(bytes, Target, Adapter);
        Assert.Equal(3u, sample.ProcessQueryFlags);
        Assert.All(Values(sample), value => Assert.Equal(ValueKind.Missing, value.Kind));
    }

    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(3)]
    [InlineData(4)] [InlineData(5)] [InlineData(6)] [InlineData(7)]
    [InlineData(8)] [InlineData(9)] [InlineData(10)] [InlineData(11)]
    [InlineData(12)] [InlineData(13)] [InlineData(14)] [InlineData(15)]
    [InlineData(16)] [InlineData(17)] [InlineData(18)] [InlineData(19)]
    public void Corrupt_identity_layout_clocks_status_and_family_arms_are_rejected(int fault)
    {
        var bytes = Buffer();
        switch (fault)
        {
            case 0: Array.Resize(ref bytes, 151); break;
            case 1: U32(bytes, 0, 153); break;
            case 2: U32(bytes, 4, 2); break;
            case 3: U32(bytes, 8, 6); break;
            case 4: U32(bytes, 16, 124); break;
            case 5: U32(bytes, 20, 1); break;
            case 6: U64(bytes, 24, 457); break;
            case 7: U32(bytes, 32, 0); break;
            case 8: U32(bytes, 36, 790); break;
            case 9: U32(bytes, 40, 3); break;
            case 10: U32(bytes, 44, 1); break;
            case 11: U64(bytes, 48, 3); break;
            case 12: U64(bytes, 56, 201); break;
            case 13: U64(bytes, 64, ulong.MaxValue); break;
            case 14: U32(bytes, 84, 7); break;
            case 15: U32(bytes, 72, 0x80004005); break;
            case 16: U32(bytes, 84, 2); break;
            case 17: U32(bytes, 92, 0); break;
            case 18: U64(bytes, 96, ulong.MaxValue); break;
            case 19: U32(bytes, 12, 3); break;
        }
        Assert.Throws<InvalidDataException>(() => GpuMemoryMetricCodec.Decode(bytes, Target, Adapter));
    }

    private static IEnumerable<TypedValue> Values(ProcessGpuMemorySample sample) =>
        [sample.DedicatedCommitted, sample.SharedCommitted, sample.DedicatedResident, sample.SharedResident];
    private static void Missing(byte[] bytes, int offset, uint status)
    {
        U32(bytes, offset, status); U64(bytes, offset + 8, ulong.MaxValue);
    }

    internal static byte[] Buffer()
    {
        var bytes = new byte[152];
        U32(bytes, 0, 152); U32(bytes, 4, 1); U32(bytes, 8, 7);
        U32(bytes, 16, Target.ProcessId); U64(bytes, 24, checked((ulong)Target.CreationTimeFileTime!.Value));
        U32(bytes, 32, unchecked((uint)Adapter.LuidHighPart)); U32(bytes, 36, Adapter.LuidLowPart);
        U32(bytes, 40, Adapter.PhysicalAdapterIndex); U64(bytes, 48, 2);
        U64(bytes, 56, 100); U64(bytes, 64, 200); U32(bytes, 84, 3);
        for (var i = 0; i < 4; i++)
        {
            U32(bytes, 92 + 16 * i, 1);
            U64(bytes, 96 + 16 * i, checked((ulong)(10 * (i + 1))));
        }
        return bytes;
    }

    internal static void U32(byte[] bytes, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset), value);
    internal static void U64(byte[] bytes, int offset, ulong value) =>
        BinaryPrimitives.WriteUInt64LittleEndian(bytes.AsSpan(offset), value);
}

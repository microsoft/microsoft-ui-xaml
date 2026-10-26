using System.Buffers.Binary;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class MetricSamplerTests
{
    [Fact]
    public void Process_snapshot_memory_source_preserves_distinct_memory_fields()
    {
        var bytes = Buffer();
        U32(bytes, 8, 10);
        var sample = MetricSampler.DecodeMemory(bytes, Target);
        Assert.Equal("NtQuerySystemInformation/SystemProcessInformation", sample.Source);
        Assert.Equal(10, sample.PrivateWorkingSet.Integer);
        Assert.Equal(20, sample.TotalWorkingSet.Integer);
        Assert.Equal(30, sample.PrivateCommit.Integer);
    }

    private static readonly ProcessInstanceKey Target = new(123, 456, Guid.NewGuid());

    [Fact]
    public void Memory_abi_keeps_resident_and_commit_values_and_native_clock_distinct()
    {
        var sample = MetricSampler.DecodeMemory(Buffer(), Target);
        Assert.Equal(Target, sample.Process);
        Assert.Equal(2ul, sample.ClockId);
        Assert.Equal(100ul, sample.Begin100ns);
        Assert.Equal(200ul, sample.End100ns);
        Assert.Equal("K32GetProcessMemoryInfo/PROCESS_MEMORY_COUNTERS_EX2", sample.Source);
        Assert.Equal("1", sample.SourceVersion);
        Assert.Equal(0u, sample.NativeError);
        Assert.Equal(10, sample.PrivateWorkingSet.Integer);
        Assert.Equal(20, sample.TotalWorkingSet.Integer);
        Assert.Equal(30, sample.PrivateCommit.Integer);
        foreach (var value in new[] { sample.PrivateWorkingSet, sample.TotalWorkingSet, sample.PrivateCommit })
        {
            Assert.Equal(ValueUnit.Bytes, value.Unit);
            Assert.Equal(DataQuality.Complete, value.Quality);
            value.Validate();
        }
    }

    [Fact]
    public void Legacy_memory_source_cannot_fabricate_private_working_set()
    {
        var bytes = Buffer();
        U32(bytes, 8, 5);
        U32(bytes, 12, 3);
        U32(bytes, 56, 50);
        U32(bytes, 64, 2);
        U64(bytes, 72, ulong.MaxValue);
        var sample = MetricSampler.DecodeMemory(bytes, Target);
        Assert.Equal("K32GetProcessMemoryInfo/PROCESS_MEMORY_COUNTERS_EX", sample.Source);
        Assert.Equal(3u, sample.Status);
        Assert.Equal(ValueKind.Missing, sample.PrivateWorkingSet.Kind);
        Assert.Equal(MeasurementKind.Unavailable, sample.PrivateWorkingSet.Quality.Kind);
        Assert.Contains("50", sample.PrivateWorkingSet.Quality.Reason);
        Assert.Equal(20, sample.TotalWorkingSet.Integer);
        Assert.Equal(30, sample.PrivateCommit.Integer);
        U32(bytes, 64, 0);
        U64(bytes, 72, 10);
        Assert.Throws<InvalidDataException>(() => MetricSampler.DecodeMemory(bytes, Target));
    }

    [Theory]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(4)]
    [InlineData(5)]
    [InlineData(6)]
    public void Unavailable_memory_retains_reason_without_numeric_sentinels(uint status)
    {
        var bytes = Buffer();
        U32(bytes, 12, status);
        U32(bytes, 56, 5);
        foreach (var offset in new[] { 64, 80, 96 })
        {
            U32(bytes, offset, status);
            U64(bytes, offset + 8, ulong.MaxValue);
        }
        var sample = MetricSampler.DecodeMemory(bytes, Target);
        Assert.Equal(5u, sample.NativeError);
        Assert.Equal(status, sample.Status);
        foreach (var value in new[] { sample.PrivateWorkingSet, sample.TotalWorkingSet, sample.PrivateCommit })
        {
            Assert.Equal(ValueKind.Missing, value.Kind);
            Assert.Null(value.Integer);
            Assert.Contains("5", value.Quality.Reason);
            value.Validate();
        }
    }

    [Theory]
    [InlineData(0, 111)]
    [InlineData(4, 2)]
    [InlineData(8, 0)]
    [InlineData(12, 7)]
    [InlineData(16, 124)]
    [InlineData(20, 1)]
    [InlineData(60, 1)]
    [InlineData(64, 7)]
    [InlineData(68, 2)]
    public void Invalid_memory_headers_and_metrics_are_rejected(int offset, uint value)
    {
        var bytes = Buffer();
        U32(bytes, offset, value);
        Assert.Throws<InvalidDataException>(() => MetricSampler.DecodeMemory(bytes, Target));
    }

    [Fact]
    public void Memory_decoder_rejects_size_identity_clock_and_value_corruption()
    {
        var valid = Buffer();
        Assert.Throws<InvalidDataException>(() => MetricSampler.DecodeMemory(valid[..111], Target));
        Assert.Throws<InvalidDataException>(() => MetricSampler.DecodeMemory([.. valid, 0], Target));
        foreach (var (offset, value) in new (int, ulong)[]
        {
            (24, 457), (32, 1), (40, 201), (48, ulong.MaxValue),
            (72, ulong.MaxValue), (72, 21), (104, (ulong)long.MaxValue + 1)
        })
        {
            var bytes = Buffer();
            U64(bytes, offset, value);
            Assert.Throws<InvalidDataException>(() => MetricSampler.DecodeMemory(bytes, Target));
        }
    }

    [Fact]
    public void Missing_values_require_missing_status_and_global_status_consistency()
    {
        foreach (var status in new uint[] { 0, 2, 3 })
        {
            var bytes = Buffer();
            U32(bytes, 12, status);
            if (status == 0) U32(bytes, 64, 2);
            Assert.Throws<InvalidDataException>(() => MetricSampler.DecodeMemory(bytes, Target));
        }
    }

    [Fact]
    public void Measured_zero_is_preserved_without_aliasing_the_native_buffer()
    {
        var bytes = Buffer();
        U64(bytes, 72, 0);
        var sample = MetricSampler.DecodeMemory(bytes, Target);
        Array.Clear(bytes);
        Assert.Equal(0, sample.PrivateWorkingSet.Integer);
        Assert.Equal(20, sample.TotalWorkingSet.Integer);
    }

    private static byte[] Buffer()
    {
        var bytes = new byte[112];
        U32(bytes, 0, 112);
        U32(bytes, 4, 1);
        U32(bytes, 8, 4);
        U32(bytes, 16, 123);
        U64(bytes, 24, 456);
        U64(bytes, 32, 2);
        U64(bytes, 40, 100);
        U64(bytes, 48, 200);
        foreach (var (offset, value) in new (int, ulong)[] { (64, 10), (80, 20), (96, 30) })
        {
            U32(bytes, offset + 4, 1);
            U64(bytes, offset + 8, value);
        }
        return bytes;
    }

    private static void U32(byte[] bytes, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset), value);
    private static void U64(byte[] bytes, int offset, ulong value) =>
        BinaryPrimitives.WriteUInt64LittleEndian(bytes.AsSpan(offset), value);
}

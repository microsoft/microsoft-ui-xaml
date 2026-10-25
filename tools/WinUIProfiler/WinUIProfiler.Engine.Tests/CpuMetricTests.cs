using System.Buffers.Binary;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class CpuMetricTests
{
    [Fact]
    public void Process_snapshot_CPU_source_is_not_labelled_as_GetProcessTimes_compatibility()
    {
        var bytes = Result();
        U32(bytes, 8, 9);
        Assert.Equal("NtQuerySystemInformation/SystemProcessInformation + NtQuerySystemInformationEx",
            CpuMetricCodec.Decode(bytes, Target).Source);
    }

    private static readonly ProcessInstanceKey Target = new(123, 456, Guid.NewGuid());

    [Fact]
    public void Cpu_result_preserves_native_interval_source_and_distinct_percentages()
    {
        var sample = CpuMetricCodec.Decode(Result(), Target);
        Assert.Equal(Target, sample.Process);
        Assert.Equal(2ul, sample.ClockId);
        Assert.Equal(100ul, sample.Begin100ns);
        Assert.Equal(200ul, sample.End100ns);
        Assert.Equal("NtQuerySystemInformationEx/GetProcessTimes compatibility", sample.Source);
        Assert.Equal("capture-1/normalize-2", sample.SourceVersion);
        Assert.Equal(40, sample.SystemUtilization.Number);
        Assert.Equal(10, sample.ProcessUtilization.Number);
        Assert.Equal(5, sample.InterruptUtilization.Number);
        Assert.Equal(30, sample.OtherOrUnattributedUtilization.Number);
        Assert.Equal(ValueKind.Missing, sample.ProcessUtility.Kind);
        Assert.All(new[] { sample.SystemUtilization, sample.ProcessUtilization,
            sample.InterruptUtilization, sample.OtherOrUnattributedUtilization, sample.ProcessUtility },
            value => value.Validate());
    }

    [Theory]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(4)]
    [InlineData(5)]
    [InlineData(6)]
    public void Cpu_missing_status_preserves_reason_and_never_exposes_nan(uint status)
    {
        var bytes = Result();
        U32(bytes, 12, status);
        foreach (var offset in new[] { 56, 72, 88, 104 })
        {
            U32(bytes, offset, status);
            F64(bytes, offset + 8, double.NaN);
        }
        var sample = CpuMetricCodec.Decode(bytes, Target);
        Assert.Equal(status, sample.Status);
        Assert.Equal(ValueKind.Missing, sample.ProcessUtilization.Kind);
        Assert.Null(sample.ProcessUtilization.Number);
        Assert.Contains($"status {status}", sample.ProcessUtilization.Quality.Reason);
        sample.ProcessUtilization.Validate();
    }

    [Fact]
    public void Asynchronous_partial_cpu_keeps_measured_system_and_interrupt_but_not_process()
    {
        var bytes = Result();
        U32(bytes, 12, 3);
        foreach (var offset in new[] { 72, 104 })
        {
            U32(bytes, offset, 3);
            F64(bytes, offset + 8, double.NaN);
        }
        var sample = CpuMetricCodec.Decode(bytes, Target);
        Assert.Equal(40, sample.SystemUtilization.Number);
        Assert.Equal(5, sample.InterruptUtilization.Number);
        Assert.Equal(ValueKind.Missing, sample.ProcessUtilization.Kind);
        Assert.Equal(ValueKind.Missing, sample.OtherOrUnattributedUtilization.Kind);
    }

    [Theory]
    [InlineData(0, 135)]
    [InlineData(4, 1)]
    [InlineData(8, 1)]
    [InlineData(12, 7)]
    [InlineData(16, 124)]
    [InlineData(20, 1)]
    [InlineData(56, 7)]
    [InlineData(60, 1)]
    [InlineData(120, 0)]
    public void Cpu_result_rejects_invalid_headers_and_statuses(int offset, uint value)
    {
        var bytes = Result();
        U32(bytes, offset, value);
        Assert.Throws<InvalidDataException>(() => CpuMetricCodec.Decode(bytes, Target));
    }

    [Fact]
    public void Cpu_result_rejects_corrupt_identity_clock_interval_and_percentages()
    {
        Assert.Throws<InvalidDataException>(() => CpuMetricCodec.Decode(Result()[..135], Target));
        foreach (var (offset, value) in new (int, ulong)[]
        {
            (24, 457), (32, 1), (40, 201), (48, ulong.MaxValue)
        })
        {
            var bytes = Result();
            U64(bytes, offset, value);
            Assert.Throws<InvalidDataException>(() => CpuMetricCodec.Decode(bytes, Target));
        }
        foreach (var invalid in new[] { double.NaN, double.PositiveInfinity, -1, 101 })
        {
            var bytes = Result();
            F64(bytes, 80, invalid);
            Assert.Throws<InvalidDataException>(() => CpuMetricCodec.Decode(bytes, Target));
        }
        var inconsistent = Result();
        U32(inconsistent, 12, 2);
        Assert.Throws<InvalidDataException>(() => CpuMetricCodec.Decode(inconsistent, Target));
    }

    [Fact]
    public void Actual_zero_is_measured_and_decoded_result_does_not_alias_native_buffer()
    {
        var bytes = Result();
        F64(bytes, 80, 0);
        F64(bytes, 112, 40);
        var sample = CpuMetricCodec.Decode(bytes, Target);
        Array.Clear(bytes);
        Assert.Equal(0, sample.ProcessUtilization.Number);
        Assert.Equal(MeasurementKind.Measured, sample.ProcessUtilization.Quality.Kind);
    }

    [Fact]
    public void Cpu_capture_preserves_acquisition_bracket_and_owns_only_valid_group_buffers()
    {
        var bytes = Capture();
        var capture = CpuMetricCodec.DecodeCapture(bytes, Target);
        Assert.Equal(0u, capture.Status);
        Assert.Equal(150ul, capture.Timestamp100ns);
        Assert.Equal(100ul, capture.Begin100ns);
        Assert.Equal(200ul, capture.End100ns);
        Assert.Equal(80, capture.Snapshot.Length);
        Assert.Equal(56, capture.Groups.Length);
        Array.Clear(bytes);
        Assert.Equal(80u, BinaryPrimitives.ReadUInt32LittleEndian(capture.Snapshot));
    }

    [Theory]
    [InlineData(0, 3695)]
    [InlineData(4, 2)]
    [InlineData(12, 3)]
    [InlineData(32, 79)]
    [InlineData(36, 1)]
    [InlineData(40, 1)]
    [InlineData(44, 7)]
    [InlineData(64, 124)]
    [InlineData(68, 1)]
    [InlineData(80, 65)]
    [InlineData(84, 65)]
    [InlineData(88, 2)]
    [InlineData(92, 1)]
    [InlineData(112, 55)]
    [InlineData(116, 1)]
    [InlineData(124, 65)]
    public void Corrupt_cpu_capture_is_rejected_before_native_normalization(int offset, uint value)
    {
        var bytes = Capture();
        U32(bytes, offset, value);
        Assert.Throws<InvalidDataException>(() => CpuMetricCodec.DecodeCapture(bytes, Target));
    }

    [Fact]
    public void Cpu_capture_rejects_duplicate_groups_invalid_timestamps_and_missing_shape()
    {
        foreach (var (offset, value) in new (int, ulong)[]
        {
            (16, 201), (24, ulong.MaxValue), (48, 1), (56, 201), (72, 457)
        })
        {
            var bytes = Capture();
            U64(bytes, offset, value);
            Assert.Throws<InvalidDataException>(() => CpuMetricCodec.DecodeCapture(bytes, Target));
        }
        var duplicate = Capture();
        U32(duplicate, 80, 2);
        U32(duplicate, 84, 2);
        duplicate.AsSpan(112, 56).CopyTo(duplicate.AsSpan(168));
        Assert.Throws<InvalidDataException>(() => CpuMetricCodec.DecodeCapture(duplicate, Target));
        var missing = Capture();
        U32(missing, 44, 2);
        Assert.Throws<InvalidDataException>(() => CpuMetricCodec.DecodeCapture(missing, Target));
        Assert.Throws<InvalidDataException>(() => CpuMetricCodec.DecodeCapture(Capture()[..3695], Target));
    }

    private static byte[] Capture()
    {
        var bytes = new byte[3696];
        U32(bytes, 0, 3696);
        U32(bytes, 4, 1);
        U64(bytes, 16, 100);
        U64(bytes, 24, 200);
        U32(bytes, 32, 80);
        U32(bytes, 36, 2);
        U32(bytes, 40, 2);
        U64(bytes, 48, 2);
        U64(bytes, 56, 150);
        U32(bytes, 64, 123);
        U64(bytes, 72, 456);
        U32(bytes, 80, 1);
        U32(bytes, 84, 1);
        U32(bytes, 88, 1);
        U32(bytes, 112, 56);
        U32(bytes, 116, 2);
        U32(bytes, 124, 16);
        return bytes;
    }

    private static byte[] Result()
    {
        var bytes = new byte[136];
        U32(bytes, 0, 136);
        U32(bytes, 4, 2);
        U32(bytes, 8, 2);
        U32(bytes, 16, 123);
        U64(bytes, 24, 456);
        U64(bytes, 32, 2);
        U64(bytes, 40, 100);
        U64(bytes, 48, 200);
        F64(bytes, 64, 40);
        F64(bytes, 80, 10);
        F64(bytes, 96, 5);
        F64(bytes, 112, 30);
        U32(bytes, 120, 2);
        F64(bytes, 128, double.NaN);
        return bytes;
    }

    private static void U32(byte[] bytes, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset), value);
    private static void U64(byte[] bytes, int offset, ulong value) =>
        BinaryPrimitives.WriteUInt64LittleEndian(bytes.AsSpan(offset), value);
    private static void F64(byte[] bytes, int offset, double value) =>
        BinaryPrimitives.WriteDoubleLittleEndian(bytes.AsSpan(offset), value);
}

using System.Buffers.Binary;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class GpuMetricTests
{
    private static readonly ProcessInstanceKey Target = new(123, 456, Guid.NewGuid());

    [Fact]
    public void Gpu_result_keeps_filetime_clock_engine_identity_and_busiest_engine()
    {
        var sample = GpuMetricCodec.Decode(Result(), Target);
        Assert.Equal(Target, sample.Process);
        Assert.Equal(3ul, sample.ClockId);
        Assert.Equal("English PDH GPU Engine", sample.Source);
        Assert.Equal("1", sample.SourceVersion);
        Assert.Equal(100ul, sample.Begin100ns);
        Assert.Equal(200ul, sample.End100ns);
        Assert.Equal(70, sample.Utilization.Number);
        Assert.Equal(2, sample.Engines.Count);
        Assert.Equal(123u, sample.Engines[0].LuidLow);
        Assert.Equal(1u, sample.Engines[1].Engine);
        Assert.Equal(20, sample.Engines[0].Utilization.Number);
        Assert.Equal(70, sample.Engines[1].Utilization.Number);
        Assert.All(new[] { sample.DedicatedCommitted, sample.SharedCommitted,
            sample.DedicatedResident, sample.SharedResident }, value =>
        {
            Assert.Equal(ValueKind.Missing, value.Kind);
            Assert.Null(value.Integer);
            value.Validate();
        });
    }

    [Theory]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    [InlineData(4)]
    [InlineData(5)]
    [InlineData(6)]
    public void Missing_gpu_utilization_is_not_measured_zero(uint status)
    {
        var bytes = Result();
        U32(bytes, 12, status);
        U32(bytes, 56, status);
        F64(bytes, 64, double.NaN);
        U32(bytes, 72, 0);
        if (status == 4) U64(bytes, 40, 201);
        var sample = GpuMetricCodec.Decode(bytes, Target);
        Assert.Equal(status, sample.Status);
        Assert.Equal(ValueKind.Missing, sample.Utilization.Kind);
        Assert.Null(sample.Utilization.Number);
        Assert.Empty(sample.Engines);
        sample.Utilization.Validate();
    }

    [Theory]
    [InlineData(0, 8335)]
    [InlineData(4, 2)]
    [InlineData(8, 2)]
    [InlineData(12, 7)]
    [InlineData(16, 124)]
    [InlineData(20, 1)]
    [InlineData(60, 1)]
    [InlineData(72, 257)]
    [InlineData(76, 1)]
    [InlineData(100, 1)]
    [InlineData(8272, 0)]
    [InlineData(8276, 2)]
    public void Invalid_gpu_result_fields_are_rejected(int offset, uint value)
    {
        var bytes = Result();
        U32(bytes, offset, value);
        Assert.Throws<InvalidDataException>(() => GpuMetricCodec.Decode(bytes, Target));
    }

    [Fact]
    public void Gpu_result_rejects_wrong_max_duplicate_engines_and_nonfinite_values()
    {
        var wrongMax = Result();
        F64(wrongMax, 64, 90);
        Assert.Throws<InvalidDataException>(() => GpuMetricCodec.Decode(wrongMax, Target));
        var duplicate = Result();
        U32(duplicate, 124, 0);
        Assert.Throws<InvalidDataException>(() => GpuMetricCodec.Decode(duplicate, Target));
        foreach (var value in new[] { double.NaN, double.PositiveInfinity, -1, 101 })
        {
            var bytes = Result();
            F64(bytes, 104, value);
            Assert.Throws<InvalidDataException>(() => GpuMetricCodec.Decode(bytes, Target));
        }
        Assert.Throws<InvalidDataException>(() => GpuMetricCodec.Decode(Result()[..8335], Target));
    }

    [Fact]
    public void Raw_gpu_capture_preserves_both_clocks_errors_and_owned_buffer()
    {
        var bytes = Capture();
        var captured = GpuMetricCodec.DecodeCapture(bytes, Target);
        Assert.Equal(3ul, captured.ClockId);
        Assert.Equal(150ul, captured.Sample100ns);
        Assert.Equal(2ul, captured.AcquisitionClockId);
        Assert.Equal(100ul, captured.AcquisitionBegin100ns);
        Assert.Equal(200ul, captured.AcquisitionEnd100ns);
        Assert.Equal(1u, captured.RowCount);
        Array.Clear(bytes);
        Assert.Equal(14424u, BinaryPrimitives.ReadUInt32LittleEndian(captured.Buffer));
    }

    [Theory]
    [InlineData(0, 14423)]
    [InlineData(4, 2)]
    [InlineData(8, 2)]
    [InlineData(12, 7)]
    [InlineData(16, 124)]
    [InlineData(20, 1)]
    [InlineData(76, 257)]
    [InlineData(84, 4097)]
    [InlineData(108, 7)]
    public void Invalid_gpu_capture_fields_are_rejected(int offset, uint value)
    {
        var bytes = Capture();
        U32(bytes, offset, value);
        Assert.Throws<InvalidDataException>(() => GpuMetricCodec.DecodeCapture(bytes, Target));
    }

    [Fact]
    public void Gpu_capture_rejects_clock_identity_and_interval_corruption()
    {
        foreach (var (offset, value) in new (int, ulong)[]
        {
            (24, 457), (32, 1), (48, 201), (56, ulong.MaxValue), (64, 1)
        })
        {
            var bytes = Capture();
            U64(bytes, offset, value);
            Assert.Throws<InvalidDataException>(() => GpuMetricCodec.DecodeCapture(bytes, Target));
        }
        Assert.Throws<InvalidDataException>(() => GpuMetricCodec.DecodeCapture(Capture()[..14423], Target));
    }

    private static byte[] Result()
    {
        var bytes = new byte[8336];
        Header(bytes);
        U64(bytes, 40, 100);
        U64(bytes, 48, 200);
        F64(bytes, 64, 70);
        U32(bytes, 72, 2);
        for (var i = 0; i < 2; i++)
        {
            var offset = 80 + i * 32;
            U32(bytes, offset + 4, 123);
            U32(bytes, offset + 12, (uint)i);
            F64(bytes, offset + 24, i == 0 ? 20 : 70);
        }
        foreach (var offset in new[] { 8272, 8288, 8304, 8320 })
        {
            U32(bytes, offset, 2);
            U32(bytes, offset + 4, 1);
            U64(bytes, offset + 8, ulong.MaxValue);
        }
        return bytes;
    }

    private static byte[] Capture()
    {
        var bytes = new byte[14424];
        Header(bytes);
        U64(bytes, 40, 150);
        U64(bytes, 48, 100);
        U64(bytes, 56, 200);
        U64(bytes, 64, 2);
        U32(bytes, 76, 1);
        U32(bytes, 84, 459);
        U32(bytes, 116, 0x20510500);
        U64(bytes, 136, 150);
        return bytes;
    }

    private static void Header(byte[] bytes)
    {
        U32(bytes, 0, checked((uint)bytes.Length));
        U32(bytes, 4, 1);
        U32(bytes, 8, 6);
        U32(bytes, 16, 123);
        U64(bytes, 24, 456);
        U64(bytes, 32, 3);
    }
    private static void U32(byte[] bytes, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset), value);
    private static void U64(byte[] bytes, int offset, ulong value) =>
        BinaryPrimitives.WriteUInt64LittleEndian(bytes.AsSpan(offset), value);
    private static void F64(byte[] bytes, int offset, double value) =>
        BinaryPrimitives.WriteDoubleLittleEndian(bytes.AsSpan(offset), value);
}

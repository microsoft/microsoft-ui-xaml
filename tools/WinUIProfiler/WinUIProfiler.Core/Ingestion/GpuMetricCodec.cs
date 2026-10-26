using System.Buffers.Binary;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record GpuEngineSample(uint LuidHigh, uint LuidLow, uint Physical, uint Engine, TypedValue Utilization);
internal sealed record ProcessGpuSample(ProcessInstanceKey Process, ulong ClockId, ulong Begin100ns, ulong End100ns,
    uint Status, TypedValue Utilization, IReadOnlyList<GpuEngineSample> Engines,
    TypedValue DedicatedCommitted, TypedValue SharedCommitted, TypedValue DedicatedResident, TypedValue SharedResident)
{
    internal string Source => "English PDH GPU Engine";
    internal string SourceVersion => "1";
}
internal sealed record GpuCounterSnapshot(byte[] Buffer, uint Status, ulong ClockId, ulong Sample100ns,
    ulong AcquisitionClockId, ulong AcquisitionBegin100ns, ulong AcquisitionEnd100ns,
    uint RowCount, uint PdhError, uint Win32Error);

internal static class GpuMetricCodec
{
    internal static ProcessGpuSample Decode(ReadOnlySpan<byte> bytes, ProcessInstanceKey target)
    {
        Header(bytes, 8336, target);
        var status = U32(bytes, 12);
        var begin = U64(bytes, 40);
        var end = U64(bytes, 48);
        var count = U32(bytes, 72);
        if (begin > long.MaxValue || end > long.MaxValue ||
            status != 4 && begin > end || U32(bytes, 56) != status ||
            U32(bytes, 60) != 0 || U32(bytes, 76) != 0 || count > 256 ||
            status == 0 && (count == 0 || begin == end) || status != 0 && count != 0)
            throw new InvalidDataException("Invalid native GPU result interval, status or engine count.");
        var utilization = Percent(status, BinaryPrimitives.ReadDoubleLittleEndian(bytes[64..]));
        var engines = new List<GpuEngineSample>(checked((int)count));
        var ids = new HashSet<(uint, uint, uint, uint)>();
        for (var i = 0; i < count; i++)
        {
            var offset = 80 + i * 32;
            var id = (U32(bytes, offset), U32(bytes, offset + 4),
                U32(bytes, offset + 8), U32(bytes, offset + 12));
            if (!ids.Add(id) || U32(bytes, offset + 16) != 0 || U32(bytes, offset + 20) != 0)
                throw new InvalidDataException("Invalid or duplicate native GPU engine.");
            engines.Add(new(id.Item1, id.Item2, id.Item3, id.Item4,
                Percent(0, BinaryPrimitives.ReadDoubleLittleEndian(bytes[(offset + 24)..]))));
        }
        if (status == 0 && utilization.Number != engines.Max(engine => engine.Utilization.Number))
            throw new InvalidDataException("Native GPU utilization is not the maximum engine measurement.");
        return new(target, 3, begin, end, status, utilization, engines.AsReadOnly(),
            Memory(bytes, 8272), Memory(bytes, 8288), Memory(bytes, 8304), Memory(bytes, 8320));
    }

    internal static GpuCounterSnapshot DecodeCapture(ReadOnlySpan<byte> bytes, ProcessInstanceKey target)
    {
        Header(bytes, 14424, target);
        var sample = U64(bytes, 40);
        var begin = U64(bytes, 48);
        var end = U64(bytes, 56);
        var count = U32(bytes, 76);
        if (sample > long.MaxValue || begin > end || end > long.MaxValue ||
            U64(bytes, 64) != 2 || count > 256 || U32(bytes, 84) > 4096)
            throw new InvalidDataException("Invalid native GPU acquisition clock, interval or row count.");
        for (var i = 0; i < count; i++)
        {
            var offset = 88 + i * 56;
            var status = U32(bytes, offset + 20);
            if (status > 6 || U32(bytes, offset + 28) != 0x20510500 ||
                status == 0 && (U64(bytes, offset + 32) > long.MaxValue ||
                    U64(bytes, offset + 40) > long.MaxValue ||
                    U64(bytes, offset + 48) is 0 or > long.MaxValue))
                throw new InvalidDataException("Invalid native GPU raw counter row.");
        }
        return new(bytes.ToArray(), U32(bytes, 12), 3, sample, 2, begin, end,
            count, U32(bytes, 72), U32(bytes, 80));
    }

    private static void Header(ReadOnlySpan<byte> bytes, uint size, ProcessInstanceKey target)
    {
        target.Validate();
        if (bytes.Length != size || U32(bytes, 0) != size || U32(bytes, 4) != 1 ||
            U32(bytes, 8) != 6 || U32(bytes, 12) > 6 || U32(bytes, 20) != 0 ||
            U64(bytes, 32) != 3)
            throw new InvalidDataException("Invalid native GPU ABI size, version, source, status or clock.");
        if (target.CreationTimeFileTime is not > 0 || U32(bytes, 16) != target.ProcessId ||
            U64(bytes, 24) != checked((ulong)target.CreationTimeFileTime.Value))
            throw new InvalidDataException("Native GPU data does not match the selected process instance.");
    }

    private static TypedValue Percent(uint status, double value)
    {
        if (status == 0 && (!double.IsFinite(value) || value is < 0 or > 100) ||
            status != 0 && !double.IsNaN(value))
            throw new InvalidDataException("Invalid native GPU percentage.");
        return status == 0
            ? new(ValueKind.Number, ValueUnit.Percent, null, value, null, null, DataQuality.Complete)
            : new(ValueKind.Missing, ValueUnit.Percent, null, null, null, null,
                new(MeasurementKind.Unavailable, CoverageState.Unavailable,
                    FormattableString.Invariant($"Native GPU status {status}.")));
    }

    private static TypedValue Memory(ReadOnlySpan<byte> bytes, int offset)
    {
        if (U32(bytes, offset) != 2 || U32(bytes, offset + 4) != 1 || U64(bytes, offset + 8) != ulong.MaxValue)
            throw new InvalidDataException("GPU ABI version 1 does not provide memory measurements.");
        return new(ValueKind.Missing, ValueUnit.Bytes, null, null, null, null,
            new(MeasurementKind.Unavailable, CoverageState.Unavailable,
                "The GPU engine source does not measure committed or resident memory."));
    }

    private static uint U32(ReadOnlySpan<byte> bytes, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    private static ulong U64(ReadOnlySpan<byte> bytes, int offset) =>
        BinaryPrimitives.ReadUInt64LittleEndian(bytes[offset..]);
}

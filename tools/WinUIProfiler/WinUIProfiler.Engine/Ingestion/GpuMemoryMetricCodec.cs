using System.Buffers.Binary;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record ProcessGpuMemorySample(ProcessInstanceKey Process, PhysicalAdapterKey Adapter,
    ulong Begin100ns, ulong End100ns, uint Status, uint DedicatedHResult, uint SharedHResult,
    uint Win32Error, uint ProcessQueryFlags, TypedValue DedicatedCommitted, TypedValue SharedCommitted,
    TypedValue DedicatedResident, TypedValue SharedResident)
{
    internal string Source => "DXCore AdapterMemoryUsageByProcessBytes";
    internal string SourceVersion => "1";
}

internal static class GpuMemoryMetricCodec
{
    internal static ProcessGpuMemorySample Decode(ReadOnlySpan<byte> bytes,
        ProcessInstanceKey target, PhysicalAdapterKey adapter)
    {
        target.Validate();
        if (bytes.Length != 152 || U32(bytes, 0) != 152 || U32(bytes, 4) != 1 ||
            U32(bytes, 8) != 7 || U32(bytes, 12) is not (0 or 2 or 3 or 5 or 6) ||
            U32(bytes, 20) != 0 || U32(bytes, 44) != 0 || U64(bytes, 48) != 2 ||
            U64(bytes, 56) > U64(bytes, 64) || U64(bytes, 64) > long.MaxValue ||
            (U32(bytes, 84) & ~3u) != 0)
            throw new InvalidDataException("Invalid native GPU memory layout, source, status, clock or flags.");
        if (target.CreationTimeFileTime is not > 0 || U32(bytes, 16) != target.ProcessId ||
            U64(bytes, 24) != checked((ulong)target.CreationTimeFileTime.Value) ||
            U32(bytes, 32) != unchecked((uint)adapter.LuidHighPart) ||
            U32(bytes, 36) != adapter.LuidLowPart || U32(bytes, 40) != adapter.PhysicalAdapterIndex)
            throw new InvalidDataException("Native GPU memory belongs to another process or physical adapter.");
        var status = U32(bytes, 12);
        var dedicatedHResult = U32(bytes, 72);
        var sharedHResult = U32(bytes, 76);
        var win32 = U32(bytes, 80);
        var flags = U32(bytes, 84);
        var dedicatedCommitted = Read(bytes, 88, dedicatedHResult, (flags & 1) != 0, win32);
        var sharedCommitted = Read(bytes, 104, sharedHResult, (flags & 2) != 0, win32);
        var dedicatedResident = Read(bytes, 120, dedicatedHResult, (flags & 1) != 0, win32);
        var sharedResident = Read(bytes, 136, sharedHResult, (flags & 2) != 0, win32);
        var dedicatedMeasured = dedicatedCommitted.Status == 0;
        var sharedMeasured = sharedCommitted.Status == 0;
        var measuredFamilies = (dedicatedMeasured ? 1 : 0) + (sharedMeasured ? 1 : 0);
        if (dedicatedCommitted.Status != dedicatedResident.Status ||
            sharedCommitted.Status != sharedResident.Status ||
            status == 0 && measuredFamilies != 2 ||
            status == 3 && measuredFamilies != 1 ||
            status is not (0 or 3) && (measuredFamilies != 0 ||
                dedicatedCommitted.Status != status || sharedCommitted.Status != status) ||
            measuredFamilies > 0 && win32 != 0)
            throw new InvalidDataException("Native GPU memory family arms disagree with their source status.");
        return new(target, adapter, U64(bytes, 56), U64(bytes, 64), status,
            dedicatedHResult, sharedHResult, win32, flags,
            dedicatedCommitted.Value, sharedCommitted.Value, dedicatedResident.Value, sharedResident.Value);
    }

    private static (TypedValue Value, uint Status) Read(ReadOnlySpan<byte> bytes, int offset,
        uint hresult, bool processQuerySucceeded, uint win32)
    {
        var status = U32(bytes, offset);
        var value = U64(bytes, offset + 8);
        if (status is not (0 or 2 or 5 or 6) || U32(bytes, offset + 4) != 1 ||
            status == 0 && (value > long.MaxValue || (hresult & 0x80000000) != 0 || !processQuerySucceeded) ||
            status != 0 && value != ulong.MaxValue)
            throw new InvalidDataException("Invalid native GPU memory byte value or inactive arm.");
        if (status == 0)
            return (new(ValueKind.Integer, ValueUnit.Bytes, checked((long)value), null, null, null,
                DataQuality.Complete), status);
        return (new(ValueKind.Missing, ValueUnit.Bytes, null, null, null, null,
            new(MeasurementKind.Unavailable, CoverageState.Unavailable,
                FormattableString.Invariant($"DXCore memory status {status}; HRESULT 0x{hresult:X8}; processQuerySucceeded={processQuerySucceeded}; Win32 {win32}."))), status);
    }

    private static uint U32(ReadOnlySpan<byte> bytes, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    private static ulong U64(ReadOnlySpan<byte> bytes, int offset) =>
        BinaryPrimitives.ReadUInt64LittleEndian(bytes[offset..]);
}

using System.Buffers.Binary;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record GpuAdapterInfo(uint Status, uint KnownFlags, uint LuidLow, int LuidHigh,
    uint VendorId, uint DeviceId, uint SubsystemId, uint Revision, uint RawDxgiFlags,
    uint? PhysicalCount, bool? IsHardware, bool? SupportsMemory, uint DescriptorHResult,
    uint LookupHResult, uint CountHResult, uint HardwareHResult, uint Win32Error)
{
    internal bool IsBasicRenderDriver => (KnownFlags & 32) != 0;
}

internal sealed record GpuAdapterInventorySnapshot(uint Status, uint Scope, ulong Begin100ns,
    ulong End100ns, uint Flags, uint EnumerationHResult, uint Win32Error,
    bool FactoryCurrentBegin, bool FactoryCurrentEnd, IReadOnlyList<GpuAdapterInfo> Adapters)
{
    internal bool CanConfigureMemory => (Flags & 7) == 1 &&
        FactoryCurrentBegin && FactoryCurrentEnd && Win32Error == 0;

    internal IReadOnlyList<PhysicalAdapterKey> SelectMemoryAdapters(bool hardwareOnly)
    {
        if (!CanConfigureMemory)
            throw new InvalidOperationException("Stale, truncated or failed graphics inventory cannot configure memory sampling.");
        var selected = new List<PhysicalAdapterKey>();
        foreach (var adapter in Adapters)
        {
            if ((adapter.KnownFlags & 23) != 23 || (adapter.KnownFlags & 64) != 0 ||
                adapter.PhysicalCount is not (>= 1 and <= 64) || adapter.SupportsMemory != true ||
                (adapter.DescriptorHResult & 0x80000000) != 0 ||
                (adapter.LookupHResult & 0x80000000) != 0 || (adapter.CountHResult & 0x80000000) != 0 ||
                hardwareOnly && (adapter.IsHardware != true || adapter.IsBasicRenderDriver ||
                    (adapter.RawDxgiFlags & 2) != 0 || (adapter.HardwareHResult & 0x80000000) != 0))
                continue;
            if (selected.Count + adapter.PhysicalCount.Value > NativeMetricPayloads.MaximumGpuMemoryAdapters)
                throw new InvalidOperationException("Graphics inventory exceeds the 256 configured physical-scope budget; select explicit scopes.");
            for (uint physical = 0; physical < adapter.PhysicalCount.Value; physical++)
                selected.Add(new(adapter.LuidLow, adapter.LuidHigh, physical));
        }
        return selected.AsReadOnly();
    }
}

internal static class GpuAdapterInventoryCodec
{
    internal static GpuAdapterInventorySnapshot Decode(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length != 5200 || U32(bytes, 0) != 5200 || U32(bytes, 4) != 1 ||
            U32(bytes, 8) != 8 || U32(bytes, 12) is not (0 or 2 or 3) ||
            U64(bytes, 16) != 2 || U64(bytes, 24) > U64(bytes, 32) || U64(bytes, 32) > long.MaxValue ||
            U32(bytes, 40) != 1 || U32(bytes, 44) > 64 || U32(bytes, 48) != 64 ||
            (U32(bytes, 52) & ~31u) != 0 || U32(bytes, 64) > 1 || U32(bytes, 68) > 1 ||
            U32(bytes, 72) != 0 || U32(bytes, 76) != 0)
            throw new InvalidDataException("Invalid native graphics inventory layout, scope, flags or clock.");
        var count = checked((int)U32(bytes, 44));
        if (bytes[(80 + count * 80)..].IndexOfAnyExcept((byte)0) >= 0)
            throw new InvalidDataException("Native graphics inventory contains nonzero unused rows.");
        var flags = U32(bytes, 52);
        var status = U32(bytes, 12);
        var enumeration = U32(bytes, 56);
        if ((flags & 3) == 3 || (flags & 1) != 0 && enumeration != 0x887A0002 ||
            status == 0 && (flags != 1 || U32(bytes, 60) != 0 || U32(bytes, 64) != 1 || U32(bytes, 68) != 1))
            throw new InvalidDataException("Native graphics inventory status disagrees with its termination or coverage.");
        var adapters = new List<GpuAdapterInfo>(count);
        for (var index = 0; index < count; index++)
        {
            var row = bytes.Slice(80 + index * 80, 80);
            var known = U32(row, 12);
            var descriptor = (known & 1) != 0;
            var identity = (known & 2) != 0;
            var physical = U32(row, 44);
            var hardware = U32(row, 48);
            var state = U32(row, 52);
            if (U32(row, 0) != 80 || U32(row, 4) != 1 || U32(row, 8) is not (0 or 2 or 3) ||
                (known & ~127u) != 0 || U32(row, 76) != 0 || descriptor != identity ||
                (known & 4) == 0 && physical != uint.MaxValue ||
                ((known & 8) == 0 ? hardware != uint.MaxValue : hardware > 1) ||
                ((known & 16) == 0 ? state != uint.MaxValue : state > 1) ||
                descriptor && (U32(row, 56) & 0x80000000) != 0 ||
                !descriptor && row.Slice(16, 28).IndexOfAnyExcept((byte)0) >= 0 ||
                (known & 28) != 0 && (U32(row, 60) & 0x80000000) != 0 ||
                (known & 4) != 0 && (U32(row, 64) & 0x80000000) != 0 ||
                (known & 8) != 0 && (U32(row, 68) & 0x80000000) != 0 ||
                U32(row, 8) == 0 && ((known & 31) != 31 || physical is < 1 or > 64 || U32(row, 72) != 0) ||
                ((known & 32) != 0) != (descriptor && U32(row, 24) == 0x1414 && U32(row, 28) == 0x008C))
                throw new InvalidDataException("Invalid native graphics inventory row or unknown metadata arm.");
            adapters.Add(new(U32(row, 8), known, U32(row, 20), unchecked((int)U32(row, 16)),
                U32(row, 24), U32(row, 28), U32(row, 32), U32(row, 36), U32(row, 40),
                (known & 4) != 0 ? physical : null, (known & 8) != 0 ? hardware == 1 : null,
                (known & 16) != 0 ? state == 1 : null, U32(row, 56), U32(row, 60),
                U32(row, 64), U32(row, 68), U32(row, 72)));
        }
        var duplicateFound = false;
        foreach (var adapter in adapters)
        {
            var duplicate = (adapter.KnownFlags & 2) != 0 && adapters.Count(other =>
                (other.KnownFlags & 2) != 0 && other.LuidLow == adapter.LuidLow &&
                other.LuidHigh == adapter.LuidHigh) > 1;
            if (duplicate != ((adapter.KnownFlags & 64) != 0))
                throw new InvalidDataException("Native graphics inventory duplicate flags disagree with logical identities.");
            duplicateFound |= duplicate;
        }
        if (duplicateFound != ((flags & 16) != 0) ||
            status == 0 && adapters.Any(adapter => adapter.Status != 0))
            throw new InvalidDataException("Native graphics inventory header disagrees with its rows.");
        return new(status, U32(bytes, 40), U64(bytes, 24), U64(bytes, 32), flags, enumeration,
            U32(bytes, 60), U32(bytes, 64) == 1, U32(bytes, 68) == 1, adapters.AsReadOnly());
    }

    private static uint U32(ReadOnlySpan<byte> bytes, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    private static ulong U64(ReadOnlySpan<byte> bytes, int offset) =>
        BinaryPrimitives.ReadUInt64LittleEndian(bytes[offset..]);
}

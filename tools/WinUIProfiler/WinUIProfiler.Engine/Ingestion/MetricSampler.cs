using System.Buffers.Binary;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record ProcessMemorySample(
    ProcessInstanceKey Process, ulong ClockId, ulong Begin100ns, ulong End100ns,
    string Source, string SourceVersion, uint NativeError,
    TypedValue PrivateWorkingSet, TypedValue TotalWorkingSet, TypedValue PrivateCommit)
{
    internal uint Status { get; init; }
}

internal static class MetricSampler
{
    internal static ProcessMemorySample DecodeMemory(ReadOnlySpan<byte> bytes, ProcessInstanceKey target)
    {
        target.Validate();
        if (bytes.Length != 112 || U32(bytes, 0) != 112 || U32(bytes, 4) != 1)
            throw new InvalidDataException("Unsupported native memory result size or version.");
        var source = U32(bytes, 8);
        var status = U32(bytes, 12);
        var error = U32(bytes, 56);
        if (source is not (4 or 5 or 10) || status > 6 ||
            U32(bytes, 20) != 0 || U32(bytes, 60) != 0)
            throw new InvalidDataException("Invalid native memory result source, status or reserved fields.");
        if (target.CreationTimeFileTime is not > 0 ||
            U32(bytes, 16) != target.ProcessId ||
            U64(bytes, 24) != checked((ulong)target.CreationTimeFileTime.Value))
            throw new InvalidDataException("Native memory result does not match the selected process instance.");
        var clock = U64(bytes, 32);
        var begin = U64(bytes, 40);
        var end = U64(bytes, 48);
        if (clock != 2 || begin > end || end > long.MaxValue)
            throw new InvalidDataException("Invalid native memory acquisition clock or interval.");
        var privateWorkingSet = ReadBytes(bytes, 64, error);
        var totalWorkingSet = ReadBytes(bytes, 80, error);
        var privateCommit = ReadBytes(bytes, 96, error);
        var measured = (privateWorkingSet.Kind != ValueKind.Missing ? 1 : 0) +
            (totalWorkingSet.Kind != ValueKind.Missing ? 1 : 0) +
            (privateCommit.Kind != ValueKind.Missing ? 1 : 0);
        if (status == 0 && (measured != 3 || error != 0) ||
            status == 3 && measured is not (1 or 2) ||
            status is not (0 or 3) && (measured != 0 ||
                U32(bytes, 64) != status || U32(bytes, 80) != status || U32(bytes, 96) != status))
            throw new InvalidDataException("Native memory result and per-measurement statuses disagree.");
        if (source == 5 && privateWorkingSet.Kind != ValueKind.Missing)
            throw new InvalidDataException("PROCESS_MEMORY_COUNTERS_EX does not report private working set.");
        if (privateWorkingSet.Integer.HasValue && totalWorkingSet.Integer.HasValue &&
            privateWorkingSet.Integer.Value > totalWorkingSet.Integer.Value)
            throw new InvalidDataException("Private working set exceeds total working set.");
        return new(target, clock, begin, end,
            source == 10 ? "NtQuerySystemInformation/SystemProcessInformation" :
            source == 4 ? "K32GetProcessMemoryInfo/PROCESS_MEMORY_COUNTERS_EX2" :
                "K32GetProcessMemoryInfo/PROCESS_MEMORY_COUNTERS_EX",
            "1", error, privateWorkingSet, totalWorkingSet, privateCommit) { Status = status };
    }

    private static TypedValue ReadBytes(ReadOnlySpan<byte> bytes, int offset, uint error)
    {
        var status = U32(bytes, offset);
        var value = U64(bytes, offset + 8);
        if (status > 6 || U32(bytes, offset + 4) != 1 ||
            status == 0 && value > long.MaxValue || status != 0 && value != ulong.MaxValue)
            throw new InvalidDataException("Invalid native byte measurement.");
        if (status == 0)
            return new(ValueKind.Integer, ValueUnit.Bytes, checked((long)value), null, null, null, DataQuality.Complete);
        var reason = FormattableString.Invariant($"Native memory status {status}; Win32 error {error}.");
        return new(ValueKind.Missing, ValueUnit.Bytes, null, null, null, null,
            new(MeasurementKind.Unavailable, CoverageState.Unavailable, reason));
    }

    private static uint U32(ReadOnlySpan<byte> bytes, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    private static ulong U64(ReadOnlySpan<byte> bytes, int offset) =>
        BinaryPrimitives.ReadUInt64LittleEndian(bytes[offset..]);
}

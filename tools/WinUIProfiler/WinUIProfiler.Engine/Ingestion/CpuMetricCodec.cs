using System.Buffers.Binary;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed record ProcessCpuSample(
    ProcessInstanceKey Process, ulong ClockId, ulong Begin100ns, ulong End100ns,
    string Source, uint Status, TypedValue SystemUtilization, TypedValue ProcessUtilization,
    TypedValue InterruptUtilization, TypedValue OtherOrUnattributedUtilization, TypedValue ProcessUtility)
{
    internal string SourceVersion => "capture-1/normalize-2";
}

internal sealed record CpuCounterSnapshot(byte[] Snapshot, byte[] Groups, uint Status,
    ulong Timestamp100ns, ulong Begin100ns, ulong End100ns, uint ErrorCode, uint ErrorDomain);

internal static class CpuMetricCodec
{
    internal static CpuCounterSnapshot DecodeCapture(ReadOnlySpan<byte> bytes, ProcessInstanceKey target)
    {
        target.Validate();
        if (bytes.Length != 3696 || U32(bytes, 0) != 3696 || U32(bytes, 4) != 1 ||
            U32(bytes, 12) > 2 || U32(bytes, 32) != 80 || U32(bytes, 36) != 2 ||
            U32(bytes, 40) is not (2 or 9) || U32(bytes, 44) > 6 ||
            U32(bytes, 68) != 0 || U32(bytes, 92) != 0)
            throw new InvalidDataException("Invalid native CPU capture headers.");
        if (target.CreationTimeFileTime is not > 0 || U32(bytes, 64) != target.ProcessId ||
            U64(bytes, 72) != checked((ulong)target.CreationTimeFileTime.Value))
            throw new InvalidDataException("Native CPU capture does not match the selected process instance.");
        var status = U32(bytes, 44);
        var error = U32(bytes, 8);
        var domain = U32(bytes, 12);
        var begin = U64(bytes, 16);
        var end = U64(bytes, 24);
        var timestamp = U64(bytes, 56);
        if (U64(bytes, 48) != 2 || begin > end || end > long.MaxValue ||
            (status == 0 || timestamp != 0) && (timestamp < begin || timestamp > end) ||
            status == 0 && (error != 0 || domain != 0) || error != 0 && domain == 0)
            throw new InvalidDataException("Invalid native CPU capture clock, acquisition interval or error domain.");
        var count = U32(bytes, 80);
        var expected = U32(bytes, 84);
        var present = U32(bytes, 88);
        if (count > 64 || expected > 64 || count > expected || present > 1 ||
            status == 0 && (count == 0 || count != expected || present != 1) ||
            status != 0 && (count != 0 || present != 0))
            throw new InvalidDataException("Invalid native CPU capture group count or availability.");
        var ids = new HashSet<uint>();
        for (var i = 0; i < count; i++)
        {
            var offset = 112 + i * 56;
            if (U32(bytes, offset) != 56 || U32(bytes, offset + 4) != 2 ||
                U32(bytes, offset + 8) >= 64 || !ids.Add(U32(bytes, offset + 8)) ||
                U32(bytes, offset + 12) is 0 or > 64)
                throw new InvalidDataException("Invalid or duplicate native CPU group.");
        }
        return new(bytes.Slice(32, 80).ToArray(), bytes.Slice(112, checked((int)count * 56)).ToArray(),
            status, timestamp, begin, end, error, domain);
    }

    internal static ProcessCpuSample Decode(ReadOnlySpan<byte> bytes, ProcessInstanceKey target)
    {
        target.Validate();
        if (bytes.Length != 136 || U32(bytes, 0) != 136 || U32(bytes, 4) != 2 ||
            U32(bytes, 8) is not (2 or 9) || U32(bytes, 12) > 6 || U32(bytes, 20) != 0)
            throw new InvalidDataException("Invalid native CPU result size, version, source or status.");
        if (target.CreationTimeFileTime is not > 0 || U32(bytes, 16) != target.ProcessId ||
            U64(bytes, 24) != checked((ulong)target.CreationTimeFileTime.Value))
            throw new InvalidDataException("Native CPU result does not match the selected process instance.");
        var status = U32(bytes, 12);
        var clock = U64(bytes, 32);
        var begin = U64(bytes, 40);
        var end = U64(bytes, 48);
        if (clock != 2 || begin > end || end > long.MaxValue || status == 0 && begin == end)
            throw new InvalidDataException("Invalid native CPU acquisition clock or interval.");
        var system = ReadPercent(bytes, 56);
        var process = ReadPercent(bytes, 72);
        var interrupt = ReadPercent(bytes, 88);
        var other = ReadPercent(bytes, 104);
        var utility = ReadPercent(bytes, 120);
        var measured = new[] { system, process, interrupt, other };
        var matchingStatuses = true;
        foreach (var offset in new[] { 56, 72, 88, 104 })
            matchingStatuses &= U32(bytes, offset) == status;
        if (U32(bytes, 120) != 2 ||
            status == 0 && measured.Any(value => value.Kind == ValueKind.Missing) ||
            status is not (0 or 3) && !matchingStatuses)
            throw new InvalidDataException("Native CPU result and per-measurement statuses disagree.");
        if (status == 3)
        {
            var allMissing = U32(bytes, 56) == 3 && U32(bytes, 72) == 3 &&
                U32(bytes, 88) == 3 && U32(bytes, 104) == 3;
            var asynchronous = U32(bytes, 56) == 0 && U32(bytes, 72) == 3 &&
                U32(bytes, 88) == 0 && U32(bytes, 104) == 3;
            if (!allMissing && !asynchronous)
                throw new InvalidDataException("Invalid partial native CPU measurement shape.");
        }
        return new(target, clock, begin, end,
            U32(bytes, 8) == 9 ? "NtQuerySystemInformation/SystemProcessInformation + NtQuerySystemInformationEx" :
                "NtQuerySystemInformationEx/GetProcessTimes compatibility", status,
            system, process, interrupt, other, utility);
    }

    private static TypedValue ReadPercent(ReadOnlySpan<byte> bytes, int offset)
    {
        var status = U32(bytes, offset);
        var value = BinaryPrimitives.ReadDoubleLittleEndian(bytes[(offset + 8)..]);
        if (status > 6 || U32(bytes, offset + 4) != 0 ||
            status == 0 && (!double.IsFinite(value) || value is < 0 or > 100) ||
            status != 0 && !double.IsNaN(value))
            throw new InvalidDataException("Invalid native CPU percentage.");
        return status == 0
            ? new(ValueKind.Number, ValueUnit.Percent, null, value, null, null, DataQuality.Complete)
            : new(ValueKind.Missing, ValueUnit.Percent, null, null, null, null,
                new(MeasurementKind.Unavailable, CoverageState.Unavailable,
                    FormattableString.Invariant($"Native CPU status {status}.")));
    }

    private static uint U32(ReadOnlySpan<byte> bytes, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    private static ulong U64(ReadOnlySpan<byte> bytes, int offset) =>
        BinaryPrimitives.ReadUInt64LittleEndian(bytes[offset..]);
}

#include "..\WinUIProfiler.Native\Metrics\CpuSourceApi.h"
#include <cstdio>
#include <cstring>
#include <limits>

namespace
{
enum Fault {
    None, Short, Oversized, Misaligned, Negative, Inconsistent, Overflow, NtFailure,
    GroupChanged, ProcessorChanged, TooManyGroups, TooManyProcessors,
    ExitBefore, ExitAfter, PidBefore, PidAfter, CreationBefore, CreationAfter,
    InventoryShort, InventoryFull, InventoryMissing, InventoryFailure,
    TimesFailure, WaitFailure, TickOverflow
};
Fault fault;
uint32_t groupCalls, cpuCalls, timesCalls, pidCalls, waitCalls, queryCalls;
bool queryArguments;
constexpr WupProcessIdentity Expected{777, 0, 123456};
FILETIME Ft(uint64_t value) { return {static_cast<DWORD>(value), static_cast<DWORD>(value >> 32)}; }
BOOL WINAPI Inventory(DWORD* pids, DWORD capacity, DWORD* bytes)
{
    if (capacity != 16384 * sizeof(DWORD)) return FALSE;
    pids[0] = fault == InventoryMissing ? 778 : Expected.pid;
    *bytes = fault == InventoryShort ? 3 : fault == InventoryFull ? capacity : sizeof(DWORD);
    return fault != InventoryFailure;
}
BOOL WINAPI Times(HANDLE, LPFILETIME created, LPFILETIME exit, LPFILETIME kernel, LPFILETIME user)
{
    ++timesCalls;
    *created = Ft(Expected.creation_filetime_100ns +
        (fault == CreationBefore || (fault == CreationAfter && timesCalls > 1) ? 1 : 0));
    *exit = Ft(0); *kernel = Ft(25); *user = Ft(15);
    return fault != TimesFailure;
}
WORD WINAPI Groups()
{
    ++groupCalls;
    return fault == TooManyGroups ? 65 : fault == GroupChanged && groupCalls > 1 ? 3 : 2;
}
DWORD WINAPI Cpus(WORD group)
{
    ++cpuCalls;
    if (group > 1) return 0;
    return fault == TooManyProcessors ? 65 : fault == ProcessorChanged && cpuCalls > 2 ? 2 : 3;
}
DWORD WINAPI Pid(HANDLE)
{
    ++pidCalls;
    return fault == PidBefore || (fault == PidAfter && pidCalls > 1) ? 778 : Expected.pid;
}
DWORD WINAPI Wait(HANDLE, DWORD milliseconds)
{
    ++waitCalls;
    if (milliseconds != 0) return WAIT_FAILED;
    if (fault == WaitFailure) return WAIT_FAILED;
    return fault == ExitBefore || (fault == ExitAfter && waitCalls > 1) ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
}
ULONGLONG WINAPI Tick() { return fault == TickOverflow ? UINT64_MAX : 100; }
DWORD WINAPI Error() { return ERROR_ACCESS_DENIED; }
LONG NTAPI Query(ULONG type, void* input, ULONG inputBytes, void* output, ULONG outputBytes, ULONG* returned)
{
    ++queryCalls;
    queryArguments &= type == 8 && inputBytes == sizeof(USHORT) &&
        *static_cast<USHORT*>(input) < 2 && outputBytes == 64 * sizeof(wup::detail::ProcessorTimes);
    wup::detail::ProcessorTimes raw[3]{};
    for (auto& row : raw)
    {
        row = {60, 100, 40, 5, 5, 0, 0};
        if (fault == Negative) row.user = -1;
        if (fault == Inconsistent) row.kernel = 50;
        if (fault == Overflow) row.kernel = (std::numeric_limits<int64_t>::max)();
    }
    std::memcpy(output, raw, sizeof(raw));
    *returned = fault == Short ? sizeof(raw) - sizeof(raw[0]) :
        fault == Oversized ? outputBytes + 48 : fault == Misaligned ? sizeof(raw) - 1 : sizeof(raw);
    return fault == NtFailure ? static_cast<LONG>(0xC0000004u) : 0;
}
wup::detail::CpuSourceApi Api()
{
    return {Query, Inventory, Times, Groups, Cpus, Pid, Wait, Tick, Error};
}
wup::CpuCapture Capture(Fault mode, wup::detail::CpuSourceApi api = Api())
{
    fault = mode;
    groupCalls = cpuCalls = timesCalls = pidCalls = waitCalls = queryCalls = 0;
    queryArguments = true;
    return wup::detail::CaptureCpuWithApi(GetCurrentProcess(), Expected, api);
}
}

int main()
{
    int failures = 0, cases = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL CPU fault line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
    auto good = Capture(None); ++cases;
    CHECK(good.snapshot.status == WupOk && good.snapshot.group_count == 2);
    CHECK(queryArguments && queryCalls == 2 && timesCalls == 2 && pidCalls == 2);
    CHECK(good.groups[0].kernel == 300 && good.groups[1].user == 120);
    const auto rejects = [&](Fault mode, WupMetricStatus status, uint32_t error, uint32_t domain = 1) {
        ++cases;
        const auto result = Capture(mode);
        CHECK(result.snapshot.status == status);
        CHECK(result.os_error == error && result.error_domain == domain);
        CHECK(result.snapshot.group_count == 0 && result.snapshot.target_present == 0);
        CHECK(result.snapshot.process_kernel == 0 && result.snapshot.process_user == 0);
        CHECK(result.groups[0].kernel == 0 && result.groups[1].kernel == 0);
    };
    rejects(Short, WupUnavailable, ERROR_INVALID_DATA);
    rejects(Oversized, WupUnavailable, ERROR_INVALID_DATA);
    rejects(Misaligned, WupUnavailable, ERROR_INVALID_DATA);
    rejects(Negative, WupUnavailable, ERROR_INVALID_DATA);
    rejects(Inconsistent, WupUnavailable, ERROR_INVALID_DATA);
    rejects(Overflow, WupUnavailable, ERROR_ARITHMETIC_OVERFLOW);
    rejects(NtFailure, WupUnavailable, 0xC0000004u, 2);
    rejects(GroupChanged, WupUnavailable, ERROR_RETRY);
    rejects(ProcessorChanged, WupUnavailable, ERROR_RETRY);
    rejects(TooManyGroups, WupUnavailable, ERROR_NOT_SUPPORTED);
    CHECK(queryCalls == 0);
    rejects(TooManyProcessors, WupUnavailable, ERROR_INVALID_DATA);
    CHECK(queryCalls == 0);
    rejects(ExitBefore, WupTargetMissing, ERROR_PROCESS_ABORTED);
    rejects(ExitAfter, WupTargetMissing, ERROR_PROCESS_ABORTED);
    rejects(PidBefore, WupIdentityChanged, ERROR_INVALID_DATA);
    rejects(PidAfter, WupIdentityChanged, ERROR_INVALID_DATA);
    rejects(CreationBefore, WupIdentityChanged, ERROR_INVALID_DATA);
    rejects(CreationAfter, WupIdentityChanged, ERROR_INVALID_DATA);
    rejects(InventoryShort, WupUnavailable, ERROR_INVALID_DATA);
    rejects(InventoryFull, WupUnavailable, ERROR_INVALID_DATA);
    rejects(InventoryMissing, WupTargetMissing, ERROR_NOT_FOUND);
    rejects(InventoryFailure, WupUnavailable, ERROR_ACCESS_DENIED);
    rejects(TimesFailure, WupUnavailable, ERROR_ACCESS_DENIED);
    rejects(WaitFailure, WupUnavailable, ERROR_ACCESS_DENIED);
    rejects(TickOverflow, WupUnavailable, ERROR_ARITHMETIC_OVERFLOW);
    for (uint32_t i = 0; i < 5; ++i)
    {
        auto api = Api();
        switch (i) {
        case 0: api.query = nullptr; break;
        case 1: api.inventory = nullptr; break;
        case 2: api.times = nullptr; break;
        case 3: api.group_count = nullptr; break;
        default: api.cpu_count = nullptr; break;
        }
        ++cases;
        const auto result = Capture(None, api);
        CHECK(result.snapshot.status == WupUnavailable && result.os_error == ERROR_NOT_SUPPORTED);
        CHECK(queryCalls == 0 && result.snapshot.group_count == 0);
    }
    std::printf("%s: CPU source fault boundary (%d failures, %d cases)\n", failures ? "FAILED" : "PASSED", failures, cases);
    return failures ? 1 : 0;
}

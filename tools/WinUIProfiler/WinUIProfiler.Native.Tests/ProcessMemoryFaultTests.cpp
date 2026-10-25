#include "..\WinUIProfiler.Native\Metrics\ProcessMemoryApi.h"
#include <cstdio>
#include <cstring>

namespace
{
enum Fault {
    None, Zero, BadLength, InvalidParameter, NotSupported, NotImplemented, UnwrittenPrivate, OlderCb,
    CorruptCb, OversizedCb, PrivateExceedsTotal, UnwrittenBase, Denied, GenericFailure,
    FallbackDenied, FallbackMalformed, FallbackUnwritten, FallbackZero,
    ExitBefore, ExitAfter, PidBefore, PidAfter, CreationBefore, CreationAfter,
    TimesBeforeFailure, TimesAfterFailure, WaitBeforeFailure, WaitAfterFailure, NoPid,
    OverflowBegin, OverflowEnd, Backwards, FailureThenExit, FailureThenIdentity
};
Fault fault;
uint32_t queries, timesCalls, pidCalls, waits, ticks;
DWORD lastError;
bool queryArguments;
constexpr WupProcessIdentity Expected{777, 0, 123456};
FILETIME Ft(uint64_t value) { return {static_cast<DWORD>(value), static_cast<DWORD>(value >> 32)}; }
BOOL WINAPI Times(HANDLE process, LPFILETIME created, LPFILETIME exit, LPFILETIME kernel, LPFILETIME user)
{
    ++timesCalls;
    if (!process) { lastError = ERROR_INVALID_HANDLE; return FALSE; }
    *created = Ft(Expected.creation_filetime_100ns +
        (fault == CreationBefore || (fault == CreationAfter && timesCalls > 1) ? 1 : 0));
    *exit = *kernel = *user = Ft(0);
    if (fault == TimesBeforeFailure || (fault == TimesAfterFailure && timesCalls > 1))
    { lastError = ERROR_ACCESS_DENIED; return FALSE; }
    return TRUE;
}
DWORD WINAPI Pid(HANDLE)
{
    ++pidCalls;
    if (fault == NoPid) { lastError = ERROR_INVALID_HANDLE; return 0; }
    return fault == PidBefore || ((fault == PidAfter || fault == FailureThenIdentity) && pidCalls > 1) ? 778 : Expected.pid;
}
DWORD WINAPI Wait(HANDLE, DWORD milliseconds)
{
    ++waits;
    if (milliseconds || fault == WaitBeforeFailure || (fault == WaitAfterFailure && waits > 1))
    { lastError = ERROR_ACCESS_DENIED; return WAIT_FAILED; }
    return fault == ExitBefore || ((fault == ExitAfter || fault == FailureThenExit) && waits > 1) ?
        WAIT_OBJECT_0 : WAIT_TIMEOUT;
}
ULONGLONG WINAPI Tick()
{
    ++ticks;
    return fault == OverflowBegin || (fault == OverflowEnd && ticks > 1) ? UINT64_MAX :
        fault == Backwards && ticks > 1 ? 99 : 100;
}
DWORD WINAPI Error() { return lastError; }
BOOL WINAPI Query(HANDLE, PPROCESS_MEMORY_COUNTERS common, DWORD bytes)
{
    ++queries;
    queryArguments &= common->cb == bytes && (bytes == sizeof(PROCESS_MEMORY_COUNTERS_EX2) ||
        bytes == sizeof(PROCESS_MEMORY_COUNTERS_EX));
    if (bytes == sizeof(PROCESS_MEMORY_COUNTERS_EX2))
    {
        auto& raw = *reinterpret_cast<PROCESS_MEMORY_COUNTERS_EX2*>(common);
        if (fault != UnwrittenBase) { raw.WorkingSetSize = fault == Zero ? 0 : 400; raw.PrivateUsage = fault == Zero ? 0 : 1600; }
        if (fault != UnwrittenPrivate) raw.PrivateWorkingSetSize = fault == Zero ? 0 : fault == PrivateExceedsTotal ? 401 : 100;
        if (fault == OlderCb) raw.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
        if (fault == CorruptCb) raw.cb = 1;
        if (fault == OversizedCb) raw.cb = sizeof(raw) + 8;
        if (fault == Denied || fault == FailureThenExit || fault == FailureThenIdentity)
        { lastError = ERROR_ACCESS_DENIED; return FALSE; }
        if (fault == GenericFailure) { lastError = ERROR_GEN_FAILURE; return FALSE; }
        if (fault == BadLength || fault == FallbackDenied || fault == FallbackMalformed ||
            fault == FallbackUnwritten || fault == FallbackZero) { lastError = ERROR_BAD_LENGTH; return FALSE; }
        if (fault == InvalidParameter) { lastError = ERROR_INVALID_PARAMETER; return FALSE; }
        if (fault == NotSupported) { lastError = ERROR_NOT_SUPPORTED; return FALSE; }
        if (fault == NotImplemented) { lastError = ERROR_CALL_NOT_IMPLEMENTED; return FALSE; }
        return TRUE;
    }
    auto& raw = *reinterpret_cast<PROCESS_MEMORY_COUNTERS_EX*>(common);
    if (fault == FallbackDenied) { lastError = ERROR_ACCESS_DENIED; return FALSE; }
    if (fault != FallbackUnwritten)
    { raw.WorkingSetSize = fault == FallbackZero ? 0 : 500; raw.PrivateUsage = fault == FallbackZero ? 0 : 2000; }
    if (fault == FallbackMalformed) raw.cb = 1;
    return TRUE;
}
wup::detail::ProcessMemoryApi Api() { return {Query, Times, Pid, Wait, Tick, Error}; }
void Reset(Fault mode)
{
    fault = mode; queries = timesCalls = pidCalls = waits = ticks = 0;
    lastError = 0; queryArguments = true;
}
WupMemoryResult Capture(Fault mode, void* handle = GetCurrentProcess())
{
    Reset(mode);
    const WupMemoryRequest request{sizeof(request), WUP_MEMORY_VERSION, Expected};
    WupMemoryResult result{};
    wup::detail::CaptureMemoryWithApi(handle, &request, &result, sizeof(result), Api());
    return result;
}
}
int main()
{
    int failures = 0, cases = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL memory fault line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
    auto good = Capture(None); ++cases;
    CHECK(good.source == WupProcessMemoryEx2 && good.status == WupOk && queries == 1 && queryArguments);
    CHECK(good.private_working_set.value == 100 && good.total_working_set.value == 400 && good.private_commit.value == 1600);
    CHECK(timesCalls == 2 && pidCalls == 2 && waits == 2 && good.clock_id == 2);
    good = Capture(Zero); ++cases;
    CHECK(good.status == WupOk && good.private_working_set.value == 0 && good.private_commit.value == 0);
    const auto fallback = [&](Fault mode, DWORD error, bool zero = false) {
        ++cases; const auto result = Capture(mode);
        CHECK(result.source == WupProcessMemoryEx && result.status == WupPartial && result.win32_error == error);
        CHECK(queries == 2 && queryArguments && timesCalls == 2);
        CHECK(result.private_working_set.status == WupUnavailable && result.private_working_set.value == UINT64_MAX);
        CHECK(result.total_working_set.status == WupOk && result.total_working_set.value == (zero ? 0u : 500u));
        CHECK(result.private_commit.status == WupOk && result.private_commit.value == (zero ? 0u : 2000u));
    };
    fallback(BadLength, ERROR_BAD_LENGTH); fallback(InvalidParameter, ERROR_INVALID_PARAMETER);
    fallback(NotSupported, ERROR_NOT_SUPPORTED); fallback(NotImplemented, ERROR_CALL_NOT_IMPLEMENTED);
    fallback(UnwrittenPrivate, ERROR_NOT_SUPPORTED); fallback(OlderCb, ERROR_NOT_SUPPORTED);
    fallback(FallbackZero, ERROR_BAD_LENGTH, true);
    const auto reject = [&](Fault mode, WupMetricStatus status, DWORD error, uint32_t calls, void* handle = GetCurrentProcess()) {
        ++cases; const auto result = Capture(mode, handle);
        CHECK(result.status == status && result.win32_error == error && queries == calls);
        CHECK(result.private_working_set.status == status && result.total_working_set.status == status && result.private_commit.status == status);
        CHECK(result.private_working_set.value == UINT64_MAX && result.total_working_set.value == UINT64_MAX && result.private_commit.value == UINT64_MAX);
        CHECK(result.private_working_set.units == WupBytes && result.total_working_set.units == WupBytes);
    };
    reject(CorruptCb, WupUnavailable, ERROR_INVALID_DATA, 1);
    reject(OversizedCb, WupUnavailable, ERROR_INVALID_DATA, 1);
    reject(PrivateExceedsTotal, WupUnavailable, ERROR_INVALID_DATA, 1);
    reject(UnwrittenBase, WupUnavailable, ERROR_INVALID_DATA, 1);
    reject(Denied, WupUnavailable, ERROR_ACCESS_DENIED, 1);
    reject(GenericFailure, WupUnavailable, ERROR_GEN_FAILURE, 1);
    reject(FallbackDenied, WupUnavailable, ERROR_ACCESS_DENIED, 2);
    reject(FallbackMalformed, WupUnavailable, ERROR_INVALID_DATA, 2);
    reject(FallbackUnwritten, WupUnavailable, ERROR_INVALID_DATA, 2);
    reject(ExitBefore, WupTargetMissing, ERROR_PROCESS_ABORTED, 0);
    reject(ExitAfter, WupTargetMissing, ERROR_PROCESS_ABORTED, 1);
    reject(PidBefore, WupIdentityChanged, ERROR_INVALID_DATA, 0);
    reject(PidAfter, WupIdentityChanged, ERROR_INVALID_DATA, 1);
    reject(CreationBefore, WupIdentityChanged, ERROR_INVALID_DATA, 0);
    reject(CreationAfter, WupIdentityChanged, ERROR_INVALID_DATA, 1);
    reject(TimesBeforeFailure, WupUnavailable, ERROR_ACCESS_DENIED, 0);
    reject(TimesAfterFailure, WupUnavailable, ERROR_ACCESS_DENIED, 1);
    reject(WaitBeforeFailure, WupUnavailable, ERROR_ACCESS_DENIED, 0);
    reject(WaitAfterFailure, WupUnavailable, ERROR_ACCESS_DENIED, 1);
    reject(NoPid, WupUnavailable, ERROR_INVALID_HANDLE, 0);
    reject(None, WupUnavailable, ERROR_INVALID_HANDLE, 0, nullptr);
    reject(OverflowBegin, WupUnavailable, ERROR_ARITHMETIC_OVERFLOW, 0);
    reject(OverflowEnd, WupUnavailable, ERROR_ARITHMETIC_OVERFLOW, 1);
    reject(Backwards, WupUnavailable, ERROR_INVALID_DATA, 1);
    reject(FailureThenExit, WupTargetMissing, ERROR_PROCESS_ABORTED, 1);
    reject(FailureThenIdentity, WupIdentityChanged, ERROR_INVALID_DATA, 1);
    Reset(None);
    auto api = Api(); api.query = nullptr;
    const WupMemoryRequest request{sizeof(request), WUP_MEMORY_VERSION, Expected};
    WupMemoryResult result{};
    ++cases;
    CHECK(wup::detail::CaptureMemoryWithApi(GetCurrentProcess(), &request, &result, sizeof(result), api) == WUP_S_OK);
    CHECK(result.status == WupUnavailable && result.win32_error == ERROR_PROC_NOT_FOUND && queries == 0);
    struct Guard { WupMemoryResult result; uint64_t canary; } guard{};
    std::memset(&guard, 0xA5, sizeof(guard)); const auto original = guard;
    auto bad = request; bad.version++;
    ++cases;
    CHECK(wup::detail::CaptureMemoryWithApi(GetCurrentProcess(), &bad, &guard.result, sizeof(result), Api()) == WUP_E_INVALIDARG);
    CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
    ++cases;
    CHECK(wup::detail::CaptureMemoryWithApi(GetCurrentProcess(), &request, &guard.result, sizeof(result) - 1, Api()) == WUP_E_INVALIDARG);
    CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
    std::printf("%s: process memory source faults (%d failures, %d cases)\n", failures ? "FAILED" : "PASSED", failures, cases);
    return failures ? 1 : 0;
}

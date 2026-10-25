#include "..\WinUIProfiler.Native\Metrics\GpuSourceApi.h"
#include <pdhmsg.h>
#include <winperf.h>
#include <cstdio>
#include <cstring>
#include <initializer_list>

namespace
{
enum Fault { None, NoSelected, Empty, OpenFail, AddFail, InfoProbeFail, InfoReadFail, InfoTiny, InfoHuge,
    InfoShortRead, InfoOversizedRead, WrongType, CollectFail, BadSample, ArrayProbeFail, ArrayReadFail,
    ArrayHuge, ArrayOversizedRead, ArrayCountHuge, ArrayShort, BadPointer, PointerInItems, Misaligned,
    NoTerminator, BadName, NoEngine, TooManyRows, Retry, BadCStatus, NegativeFirst, NegativeSecond,
    Suffix, Duplicate, RowTimestampZero, CloseFail, ExitBefore, ExitAfter, PidBefore, PidAfter,
    CreationBefore, CreationAfter, TimesFail, WaitFail, NoPid, OverflowBegin, OverflowEnd, Backwards };
Fault fault;
uint32_t opens, closes, adds, infos, collects, arrays, timesCalls, pids, waits, ticks;
bool arguments;
DWORD lastError;
constexpr WupProcessIdentity Expected{777, 0, 123456};
FILETIME Ft(uint64_t value) { return {static_cast<DWORD>(value), static_cast<DWORD>(value >> 32)}; }
BOOL WINAPI Times(HANDLE handle, LPFILETIME created, LPFILETIME exit, LPFILETIME kernel, LPFILETIME user)
{
    ++timesCalls;
    if (!handle) { lastError = ERROR_INVALID_HANDLE; return FALSE; }
    if (fault == TimesFail) { lastError = ERROR_ACCESS_DENIED; return FALSE; }
    *created = Ft(Expected.creation_filetime_100ns +
        (fault == CreationBefore || (fault == CreationAfter && timesCalls > 1) ? 1 : 0));
    *exit = *kernel = *user = Ft(0);
    return TRUE;
}
DWORD WINAPI Pid(HANDLE)
{
    ++pids;
    if (fault == NoPid) { lastError = ERROR_INVALID_HANDLE; return 0; }
    return fault == PidBefore || (fault == PidAfter && pids > 1) ? 778 : Expected.pid;
}
DWORD WINAPI Wait(HANDLE, DWORD milliseconds)
{
    ++waits; arguments &= milliseconds == 0;
    if (fault == WaitFail) { lastError = ERROR_ACCESS_DENIED; return WAIT_FAILED; }
    return fault == ExitBefore || (fault == ExitAfter && waits > 1) ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
}
ULONGLONG WINAPI Tick()
{
    ++ticks;
    return fault == OverflowBegin || (fault == OverflowEnd && ticks > 1) ? UINT64_MAX :
        fault == Backwards && ticks > 1 ? 99 : 100;
}
DWORD WINAPI Error() { return lastError; }
PDH_STATUS WINAPI Open(LPCWSTR source, DWORD_PTR context, PDH_HQUERY* query)
{
    ++opens; arguments &= !source && !context;
    if (fault == OpenFail) return PDH_INVALID_HANDLE;
    *query = reinterpret_cast<PDH_HQUERY>(1); return ERROR_SUCCESS;
}
PDH_STATUS WINAPI Add(PDH_HQUERY, LPCWSTR path, DWORD_PTR context, PDH_HCOUNTER* counter)
{
    ++adds; arguments &= !context && wcscmp(path, L"\\GPU Engine(*)\\Utilization Percentage") == 0;
    if (fault == AddFail) return PDH_CSTATUS_NO_OBJECT;
    *counter = reinterpret_cast<PDH_HCOUNTER>(2); return ERROR_SUCCESS;
}
PDH_STATUS WINAPI Info(PDH_HCOUNTER, BOOLEAN explain, DWORD* bytes, PPDH_COUNTER_INFO_W info)
{
    ++infos; arguments &= !explain;
    if (!info)
    {
        arguments &= *bytes == 0;
        if (fault == InfoProbeFail) return PDH_INVALID_HANDLE;
        *bytes = fault == InfoTiny ? 1 : fault == InfoHuge ? 1024 * 1024 + 1 : sizeof(*info);
        return PDH_MORE_DATA;
    }
    if (fault == InfoReadFail) return PDH_INVALID_DATA;
    info->dwType = fault == WrongType ? PERF_COUNTER_RAWCOUNT : PERF_100NSEC_TIMER;
    if (fault == InfoShortRead) *bytes = 1;
    if (fault == InfoOversizedRead) ++*bytes;
    return ERROR_SUCCESS;
}
PDH_STATUS WINAPI Collect(PDH_HQUERY, LONGLONG* sample)
{
    ++collects; *sample = fault == BadSample ? -1 : 500;
    return fault == CollectFail ? PDH_NO_DATA : ERROR_SUCCESS;
}
PDH_STATUS WINAPI Close(PDH_HQUERY) { ++closes; return fault == CloseFail ? PDH_INVALID_HANDLE : ERROR_SUCCESS; }
PDH_STATUS WINAPI Array(PDH_HCOUNTER, DWORD* bytes, DWORD* count, PPDH_RAW_COUNTER_ITEM_W rows)
{
    ++arrays;
    const DWORD n = fault == TooManyRows ? 257 : fault == Duplicate ? 2 : 1;
    const DWORD required = n * static_cast<DWORD>(sizeof(*rows) + 256);
    if (!rows)
    {
        arguments &= *bytes == 0;
        if (fault == ArrayProbeFail) return PDH_NO_DATA;
        if (fault == Empty) { *bytes = *count = 0; return ERROR_SUCCESS; }
        *bytes = fault == ArrayHuge ? 1024 * 1024 + 1 : required; *count = n;
        return PDH_MORE_DATA;
    }
    if (fault == ArrayReadFail) return PDH_INVALID_DATA;
    if (fault == Retry) return PDH_MORE_DATA;
    *count = fault == ArrayCountHuge ? 4097 : n;
    const DWORD capacity = *bytes;
    for (DWORD i = 0; i < n; ++i)
    {
        auto name = reinterpret_cast<wchar_t*>(reinterpret_cast<unsigned char*>(rows) + n * sizeof(*rows) + i * 256);
        swprintf_s(name, 128, L"pid_%u_luid_0x00000001_0x00000002_phys_0_eng_%u_engtype_Compute%s",
            fault == NoSelected ? 778u : 777u, fault == Duplicate ? 0 : i, fault == Suffix ? L"#1" : L"");
        if (fault == BadName) wcscpy_s(name, 128, L"pid_777_invalid");
        if (fault == NoEngine) wcscpy_s(name, 128, L"pid_777_luid_0x00000001_0x00000002_phys_0");
        if (fault == NoTerminator) for (uint32_t j = 0; j < 128; ++j) name[j] = L'x';
        rows[i].szName = fault == BadPointer ? reinterpret_cast<wchar_t*>(reinterpret_cast<unsigned char*>(rows) + capacity) :
            fault == PointerInItems ? reinterpret_cast<wchar_t*>(rows) :
            fault == Misaligned ? reinterpret_cast<wchar_t*>(reinterpret_cast<unsigned char*>(name) + 1) : name;
        rows[i].RawValue = {};
        rows[i].RawValue.CStatus = fault == BadCStatus ? PDH_CSTATUS_INVALID_DATA : PDH_CSTATUS_VALID_DATA;
        rows[i].RawValue.FirstValue = fault == NegativeFirst ? -1 : 100;
        rows[i].RawValue.SecondValue = fault == NegativeSecond ? -1 : 1000;
        rows[i].RawValue.TimeStamp = Ft(fault == RowTimestampZero ? 0 : 500);
    }
    if (fault == ArrayOversizedRead) *bytes = capacity + 1;
    if (fault == ArrayShort) *bytes = sizeof(*rows) - 1;
    return ERROR_SUCCESS;
}
wup::detail::GpuSourceApi Api() { return {Open, Add, Info, Collect, Array, Close, Times, Pid, Wait, Tick, Error}; }
void Reset(Fault mode)
{
    fault = mode; opens = closes = adds = infos = collects = arrays = timesCalls = pids = waits = ticks = 0;
    arguments = true; lastError = 0;
}
WupGpuSnapshot Capture(Fault mode, void* handle = GetCurrentProcess())
{
    Reset(mode);
    const WupGpuRequest request{sizeof(request), WUP_GPU_VERSION, Expected};
    WupGpuSnapshot result{};
    wup::detail::CaptureGpuWithApi(handle, &request, &result, sizeof(result), Api());
    return result;
}
}
int main()
{
    int failures = 0, cases = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL GPU source line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
    const auto good = Capture(None); ++cases;
    CHECK(good.status == WupOk && good.row_count == 1 && good.rows[0].running_time == 100);
    CHECK(good.clock_id == 3 && good.acquisition_clock_id == 2 && good.sample_filetime_100ns == 500);
    CHECK(arguments && opens == 1 && closes == 1 && timesCalls == 2 && arrays == 2);
    const auto reject = [&](Fault mode, WupMetricStatus status, uint32_t pdh, DWORD win32 = 0) {
        ++cases; const auto result = Capture(mode);
        CHECK(result.status == status && result.pdh_error == pdh && result.win32_error == win32);
        CHECK(result.row_count == 0);
        WupGpuRawEngine empty[WUP_MAX_GPU_ROWS]{};
        CHECK(std::memcmp(result.rows, empty, sizeof(empty)) == 0);
        CHECK(closes == opens - (mode == OpenFail ? 1u : 0u));
    };
    reject(NoSelected, WupUnavailable, 0); reject(Empty, WupUnavailable, 0);
    reject(OpenFail, WupUnavailable, PDH_INVALID_HANDLE);
    reject(AddFail, WupUnavailable, PDH_CSTATUS_NO_OBJECT);
    reject(InfoProbeFail, WupUnavailable, PDH_INVALID_HANDLE);
    reject(InfoReadFail, WupUnavailable, PDH_INVALID_DATA);
    reject(InfoTiny, WupUnavailable, PDH_INVALID_BUFFER); reject(InfoHuge, WupUnavailable, PDH_INVALID_BUFFER);
    reject(InfoShortRead, WupUnavailable, PDH_INVALID_BUFFER); reject(InfoOversizedRead, WupUnavailable, PDH_INVALID_BUFFER);
    reject(WrongType, WupUnavailable, 0, ERROR_NOT_SUPPORTED);
    reject(CollectFail, WupUnavailable, PDH_NO_DATA);
    reject(BadSample, WupUnavailable, 0, ERROR_INVALID_DATA);
    for (auto mode : {ArrayProbeFail, ArrayReadFail, ArrayHuge, ArrayOversizedRead, ArrayCountHuge, ArrayShort,
        BadPointer, PointerInItems, Misaligned, NoTerminator, BadName, NoEngine, TooManyRows, Retry})
        reject(mode, WupUnavailable, mode == ArrayProbeFail ? PDH_NO_DATA :
            mode == ArrayReadFail ? PDH_INVALID_DATA : mode == Retry ? PDH_MORE_DATA : PDH_INVALID_BUFFER);
    CHECK(arrays == 6);
    for (auto mode : {BadCStatus, NegativeFirst, NegativeSecond, Suffix, Duplicate, RowTimestampZero})
    {
        ++cases; const auto result = Capture(mode);
        CHECK(result.status == WupPartial && result.row_count > 0 && result.rows[0].status != WupOk);
        CHECK(result.rows[0].running_time == 0 && result.rows[0].timer == 0 && closes == 1);
    }
    reject(CloseFail, WupUnavailable, PDH_INVALID_HANDLE);
    reject(ExitBefore, WupTargetMissing, 0, ERROR_PROCESS_ABORTED);
    reject(ExitAfter, WupTargetMissing, 0, ERROR_PROCESS_ABORTED);
    reject(PidBefore, WupIdentityChanged, 0, ERROR_INVALID_DATA); reject(PidAfter, WupIdentityChanged, 0, ERROR_INVALID_DATA);
    reject(CreationBefore, WupIdentityChanged, 0, ERROR_INVALID_DATA); reject(CreationAfter, WupIdentityChanged, 0, ERROR_INVALID_DATA);
    reject(TimesFail, WupUnavailable, 0, ERROR_ACCESS_DENIED); reject(WaitFail, WupUnavailable, 0, ERROR_ACCESS_DENIED);
    reject(NoPid, WupUnavailable, 0, ERROR_INVALID_HANDLE);
    reject(OverflowBegin, WupUnavailable, 0, ERROR_ARITHMETIC_OVERFLOW);
    reject(OverflowEnd, WupUnavailable, 0, ERROR_ARITHMETIC_OVERFLOW);
    reject(Backwards, WupUnavailable, 0, ERROR_INVALID_DATA);
    for (uint32_t i = 0; i < 6; ++i)
    {
        Reset(None); auto api = Api();
        switch (i) { case 0: api.open = nullptr; break; case 1: api.add = nullptr; break;
        case 2: api.info = nullptr; break; case 3: api.collect = nullptr; break;
        case 4: api.array = nullptr; break; default: api.close = nullptr; break; }
        const WupGpuRequest request{sizeof(request), WUP_GPU_VERSION, Expected};
        WupGpuSnapshot result{}; ++cases;
        CHECK(wup::detail::CaptureGpuWithApi(GetCurrentProcess(), &request, &result, sizeof(result), api) == WUP_S_OK);
        CHECK(result.status == WupUnavailable && result.win32_error == ERROR_PROC_NOT_FOUND && opens == 0 && !result.row_count);
    }
    std::printf("%s: GPU capture source faults (%d failures, %d cases)\n", failures ? "FAILED" : "PASSED", failures, cases);
    return failures ? 1 : 0;
}

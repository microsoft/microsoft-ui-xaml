#include "ProcessMemorySampler.h"
#include "ProcessMemoryApi.h"
#include "ProcessSnapshot.h"
#include <Windows.h>
#include <psapi.h>
#include <cstring>

namespace wup
{
void SetMemoryValues(WupMemoryResult& result, uint64_t working_set,
    uint64_t private_usage, const uint64_t* private_working_set)
{
    result.status = private_working_set ? WupOk : WupPartial;
    result.total_working_set = {WupOk, WupBytes, working_set};
    result.private_commit = {WupOk, WupBytes, private_usage};
    result.private_working_set = private_working_set ?
        WupByteMetric{WupOk, WupBytes, *private_working_set} :
        WupByteMetric{WupUnavailable, WupBytes, UINT64_MAX};
}
}

namespace
{
uint64_t Time(FILETIME value)
{
    return (static_cast<uint64_t>(value.dwHighDateTime) << 32) | value.dwLowDateTime;
}
WupMetricStatus ValidateHandle(HANDLE process, WupProcessIdentity expected, DWORD& error,
    const wup::detail::ProcessMemoryApi& api)
{
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!api.times(process, &creation, &exit, &kernel, &user))
    {
        error = api.last_error();
        return WupUnavailable;
    }
    const auto pid = api.process_id(process);
    if (!pid)
    {
        error = api.last_error();
        return WupUnavailable;
    }
    if (pid != expected.pid || Time(creation) != expected.creation_filetime_100ns)
    {
        error = ERROR_INVALID_DATA;
        return WupIdentityChanged;
    }
    const auto wait = api.wait(process, 0);
    if (wait == WAIT_OBJECT_0)
    {
        error = ERROR_PROCESS_ABORTED;
        return WupTargetMissing;
    }
    if (wait != WAIT_TIMEOUT)
    {
        error = wait == WAIT_FAILED ? api.last_error() : ERROR_INVALID_DATA;
        return WupUnavailable;
    }
    error = ERROR_SUCCESS;
    return WupOk;
}
bool Unsupported(DWORD error)
{
    return error == ERROR_BAD_LENGTH || error == ERROR_INVALID_PARAMETER ||
        error == ERROR_NOT_SUPPORTED || error == ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL WINAPI QueryProcessMemorySnapshot(HANDLE process, PPROCESS_MEMORY_COUNTERS output, DWORD size)
{
    if (!output || size != sizeof(PROCESS_MEMORY_COUNTERS_EX2))
    {
        SetLastError(ERROR_BAD_LENGTH);
        return FALSE;
    }
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!GetProcessTimes(process, &creation, &exit, &kernel, &user)) return FALSE;
    const WupProcessIdentity target{GetProcessId(process), 0, Time(creation)};
    const auto snapshot = wup::detail::CaptureProcessSnapshot(wup::detail::ResolveProcessQuery(), target);
    if (snapshot.status != WupOk)
    {
        DWORD error = snapshot.error;
        if (snapshot.domain == 2)
        {
            using Translate = ULONG (WINAPI*)(LONG);
            Translate translate{};
            const auto address = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlNtStatusToDosError");
            std::memcpy(&translate, &address, sizeof(translate));
            error = translate ? translate(static_cast<LONG>(snapshot.error)) : ERROR_PROC_NOT_FOUND;
        }
        SetLastError(error);
        return FALSE;
    }
    auto& value = *reinterpret_cast<PROCESS_MEMORY_COUNTERS_EX2*>(output);
    value.cb = sizeof(value);
    value.WorkingSetSize = snapshot.working_set;
    value.PrivateWorkingSetSize = snapshot.private_working_set;
    value.PrivateUsage = snapshot.commit;
    return TRUE;
}
}

uint32_t WupCaptureProcessMemory(void* process_handle, const WupMemoryRequest* request,
    WupMemoryResult* output, uint32_t output_size) noexcept
{
    const wup::detail::ProcessMemoryApi api{
        QueryProcessMemorySnapshot, GetProcessTimes, GetProcessId, WaitForSingleObject, GetTickCount64,
        GetLastError, WupNtProcessSnapshotMemory};
    return wup::detail::CaptureMemoryWithApi(process_handle, request, output, output_size, api);
}

uint32_t wup::detail::CaptureMemoryWithApi(void* process_handle, const WupMemoryRequest* request,
    WupMemoryResult* output, uint32_t output_size, const ProcessMemoryApi& api) noexcept
{
    if (!request || !output || output_size < sizeof(*output) ||
        request->size != sizeof(*request) || request->version != WUP_MEMORY_VERSION ||
        request->process.reserved || !request->process.pid || !request->process.creation_filetime_100ns ||
        !api.times || !api.process_id || !api.wait || !api.tick || !api.last_error)
        return WUP_E_INVALIDARG;
    WupMemoryResult result{};
    result.size = sizeof(result);
    result.version = WUP_MEMORY_VERSION;
    result.source = api.source;
    result.process = request->process;
    result.clock_id = 2; // Machine-boot GetTickCount64, in 100 ns units (millisecond resolution).
    const auto begin = api.tick();
    auto finish = [&](WupMetricStatus status, DWORD error) {
        result.status = status;
        result.win32_error = error;
        const auto end = api.tick();
        if (begin <= UINT64_MAX / 10000 && end <= UINT64_MAX / 10000 && end >= begin)
        {
            result.acquisition_begin_100ns = begin * 10000;
            result.acquisition_end_100ns = end * 10000;
        }
        else
        {
            result.status = status = WupUnavailable;
            result.win32_error = begin > UINT64_MAX / 10000 || end > UINT64_MAX / 10000 ?
                ERROR_ARITHMETIC_OVERFLOW : ERROR_INVALID_DATA;
        }
        if (status != WupOk && status != WupPartial)
            result.private_working_set = result.total_working_set = result.private_commit =
                {status, WupBytes, UINT64_MAX};
        *output = result;
        return WUP_S_OK;
    };
    if (begin > UINT64_MAX / 10000) return finish(WupUnavailable, ERROR_ARITHMETIC_OVERFLOW);
    DWORD error = 0;
    auto status = ValidateHandle(process_handle, request->process, error, api);
    if (status != WupOk) return finish(status, error);
    auto queryFailure = [&](DWORD queryError) {
        DWORD lifecycleError = 0;
        const auto lifecycle = ValidateHandle(process_handle, request->process, lifecycleError, api);
        return lifecycle == WupOk ? finish(WupUnavailable, queryError) : finish(lifecycle, lifecycleError);
    };
    if (!api.query) return finish(WupUnavailable, ERROR_PROC_NOT_FOUND);

    PROCESS_MEMORY_COUNTERS_EX2 raw{};
    raw.cb = sizeof(raw);
    raw.PrivateWorkingSetSize = SIZE_MAX; // Detect successful calls that did not populate EX2.
    raw.WorkingSetSize = raw.PrivateUsage = SIZE_MAX;
    const auto ok = api.query(process_handle, reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&raw), sizeof(raw));
    error = ok ? ERROR_SUCCESS : api.last_error();
    if (ok && raw.cb != sizeof(raw) && raw.cb != sizeof(PROCESS_MEMORY_COUNTERS_EX))
        return queryFailure(ERROR_INVALID_DATA);
    const bool complete = ok && raw.cb == sizeof(raw) && raw.PrivateWorkingSetSize != SIZE_MAX;
    if (complete)
    {
        if (raw.WorkingSetSize == SIZE_MAX || raw.PrivateUsage == SIZE_MAX ||
            raw.PrivateWorkingSetSize > raw.WorkingSetSize)
            return queryFailure(ERROR_INVALID_DATA);
        const uint64_t privateWorkingSet = raw.PrivateWorkingSetSize;
        wup::SetMemoryValues(result, raw.WorkingSetSize, raw.PrivateUsage, &privateWorkingSet);
    }
    else
    {
        if (api.source == WupNtProcessSnapshotMemory) return queryFailure(error ? error : ERROR_INVALID_DATA);
        if (!ok && !Unsupported(error)) return queryFailure(error);
        PROCESS_MEMORY_COUNTERS_EX fallback{};
        fallback.cb = sizeof(fallback);
        fallback.WorkingSetSize = fallback.PrivateUsage = SIZE_MAX;
        result.source = WupProcessMemoryEx;
        if (!api.query(process_handle, reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&fallback), sizeof(fallback)))
            return queryFailure(api.last_error());
        if (fallback.cb != sizeof(fallback) || fallback.WorkingSetSize == SIZE_MAX || fallback.PrivateUsage == SIZE_MAX)
            return queryFailure(ERROR_INVALID_DATA);
        wup::SetMemoryValues(result, fallback.WorkingSetSize, fallback.PrivateUsage, nullptr);
        error = ok ? ERROR_NOT_SUPPORTED : error;
    }
    const DWORD capabilityError = error;
    status = ValidateHandle(process_handle, request->process, error, api);
    if (status != WupOk) return finish(status, error);
    return finish(result.status, capabilityError);
}

#include "CpuSource.h"

namespace
{
WupMetricStatus ValidateIdentity(HANDLE process, WupProcessIdentity expected, DWORD& error)
{
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!GetProcessTimes(process, &creation, &exit, &kernel, &user))
    {
        error = GetLastError();
        return WupUnavailable;
    }
    const auto pid = GetProcessId(process);
    if (!pid)
    {
        error = GetLastError();
        return WupUnavailable;
    }
    const uint64_t created = (static_cast<uint64_t>(creation.dwHighDateTime) << 32) |
        creation.dwLowDateTime;
    if (pid != expected.pid || created != expected.creation_filetime_100ns)
    {
        error = ERROR_INVALID_DATA;
        return WupIdentityChanged;
    }
    const auto wait = WaitForSingleObject(process, 0);
    if (wait == WAIT_OBJECT_0)
    {
        error = ERROR_PROCESS_ABORTED;
        return WupTargetMissing;
    }
    if (wait != WAIT_TIMEOUT)
    {
        error = wait == WAIT_FAILED ? GetLastError() : ERROR_INVALID_DATA;
        return WupUnavailable;
    }
    error = ERROR_SUCCESS;
    return WupOk;
}
}

uint32_t WupCaptureCpu(void* process_handle, const WupCpuCaptureRequest* request,
    WupCpuCaptureResult* output, uint32_t output_size) noexcept
{
    if (!request || !output || output_size < sizeof(*output) ||
        request->size != sizeof(*request) || request->version != WUP_CPU_CAPTURE_VERSION ||
        request->process.reserved || !request->process.pid || !request->process.creation_filetime_100ns)
        return WUP_E_INVALIDARG;
    WupCpuCaptureResult result{};
    result.size = sizeof(result);
    result.version = WUP_CPU_CAPTURE_VERSION;
    result.snapshot.size = sizeof(result.snapshot);
    result.snapshot.version = WUP_METRICS_VERSION;
    result.snapshot.source = WupNtProcessSnapshotCpu;
    result.snapshot.status = WupUnavailable;
    result.snapshot.process = request->process;
    result.snapshot.clock_id = 2;
    const auto begin = GetTickCount64();
    auto finish = [&](WupMetricStatus status, uint32_t error, uint32_t domain) {
        result.snapshot.status = status;
        result.error_code = error;
        result.error_domain = domain;
        const auto end = GetTickCount64();
        if (begin <= UINT64_MAX / 10000 && end <= UINT64_MAX / 10000)
        {
            result.acquisition_begin_100ns = begin * 10000;
            result.acquisition_end_100ns = end * 10000;
        }
        else
        {
            result.snapshot.status = WupUnavailable;
            result.error_code = ERROR_ARITHMETIC_OVERFLOW;
            result.error_domain = 1;
        }
        if (result.snapshot.status != WupOk)
        {
            result.snapshot.group_count = 0;
            result.snapshot.target_present = 0;
            for (auto& group : result.groups) group = {};
        }
        *output = result;
        return WUP_S_OK;
    };
    DWORD error = 0;
    auto status = ValidateIdentity(process_handle, request->process, error);
    if (status != WupOk) return finish(status, error, 1);
    const auto captured = wup::CaptureCpu(process_handle, request->process);
    if (captured.snapshot.group_count > WUP_MAX_CPU_GROUPS ||
        captured.snapshot.expected_group_count > WUP_MAX_CPU_GROUPS)
        return finish(WupUnavailable, ERROR_INVALID_DATA, 1);
    result.snapshot = captured.snapshot;
    for (uint32_t i = 0; i < result.snapshot.group_count; ++i)
        result.groups[i] = captured.groups[i];
    status = ValidateIdentity(process_handle, request->process, error);
    if (status != WupOk) return finish(status, error, 1);
    return finish(captured.snapshot.status, captured.os_error, captured.error_domain);
}

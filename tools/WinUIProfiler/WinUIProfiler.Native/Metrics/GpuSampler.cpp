#include "GpuPdhArray.h"
#include "GpuSourceApi.h"
#include <pdhmsg.h>
#include <winperf.h>
#include <cmath>
#include <limits>

namespace
{
WupMetricStatus Identity(HANDLE handle, WupProcessIdentity expected, DWORD& error, const wup::detail::GpuSourceApi& api)
{
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!api.times(handle, &creation, &exit, &kernel, &user))
    { error = api.last_error(); return WupUnavailable; }
    const auto pid = api.pid(handle);
    if (!pid) { error = api.last_error(); return WupUnavailable; }
    if (pid != expected.pid || ((static_cast<uint64_t>(creation.dwHighDateTime) << 32) |
        creation.dwLowDateTime) != expected.creation_filetime_100ns)
    { error = ERROR_INVALID_DATA; return WupIdentityChanged; }
    const auto wait = api.wait(handle, 0);
    error = wait == WAIT_TIMEOUT ? ERROR_SUCCESS : wait == WAIT_OBJECT_0 ? ERROR_PROCESS_ABORTED :
        wait == WAIT_FAILED ? api.last_error() : ERROR_INVALID_DATA;
    return wait == WAIT_TIMEOUT ? WupOk : wait == WAIT_OBJECT_0 ? WupTargetMissing : WupUnavailable;
}
bool Valid(const WupGpuSnapshot* value)
{
    return value && value->size == sizeof(*value) && value->version == WUP_GPU_VERSION &&
        value->source == WupEnglishPdhGpuEngine && value->status <= WupIdentityChanged &&
        value->row_count <= WUP_MAX_GPU_ROWS && value->process.pid &&
        !value->process.reserved && value->process.creation_filetime_100ns && value->clock_id == 3;
}
bool Same(const WupGpuRawEngine& a, const WupGpuRawEngine& b)
{
    return a.luid_high == b.luid_high && a.luid_low == b.luid_low &&
        a.physical == b.physical && a.engine == b.engine;
}
}

uint32_t WupCaptureGpu(void* process_handle, const WupGpuRequest* request,
    WupGpuSnapshot* output, uint32_t output_size) noexcept
{
    const wup::detail::GpuSourceApi api{PdhOpenQueryW, PdhAddEnglishCounterW, PdhGetCounterInfoW,
        PdhCollectQueryDataWithTime, PdhGetRawCounterArrayW, PdhCloseQuery,
        GetProcessTimes, GetProcessId, WaitForSingleObject, GetTickCount64, GetLastError};
    return wup::detail::CaptureGpuWithApi(process_handle, request, output, output_size, api);
}

uint32_t wup::detail::CaptureGpuWithApi(void* process_handle, const WupGpuRequest* request,
    WupGpuSnapshot* output, uint32_t output_size, const GpuSourceApi& api) noexcept
{
    if (!request || !output || output_size < sizeof(*output) || request->size != sizeof(*request) ||
        request->version != WUP_GPU_VERSION || request->process.reserved ||
        !request->process.pid || !request->process.creation_filetime_100ns ||
        !api.times || !api.pid || !api.wait || !api.tick || !api.last_error) return WUP_E_INVALIDARG;
    WupGpuSnapshot result{};
    result.size = sizeof(result); result.version = WUP_GPU_VERSION;
    result.source = WupEnglishPdhGpuEngine; result.process = request->process;
    result.clock_id = 3;
    result.acquisition_clock_id = 2;
    result.status = WupUnavailable;
    const auto begin = api.tick();
    auto finish = [&]() {
        const auto end = api.tick();
        if (begin <= UINT64_MAX / 10000 && end <= UINT64_MAX / 10000 && end >= begin)
        { result.acquisition_begin_100ns = begin * 10000; result.acquisition_end_100ns = end * 10000; }
        else {
            result.status = WupUnavailable;
            result.win32_error = begin > UINT64_MAX / 10000 || end > UINT64_MAX / 10000 ?
                ERROR_ARITHMETIC_OVERFLOW : ERROR_INVALID_DATA;
        }
        if (result.status != WupOk && result.status != WupPartial)
        { result.row_count = 0; for (auto& row : result.rows) row = {}; }
        *output = result;
        return WUP_S_OK;
    };
    if (begin > UINT64_MAX / 10000) return finish();
    DWORD identityError = 0;
    result.status = Identity(process_handle, request->process, identityError, api);
    result.win32_error = identityError;
    if (result.status != WupOk) return finish();
    result.status = WupUnavailable;
    if (!api.open || !api.add || !api.info || !api.collect || !api.array || !api.close)
    { result.win32_error = ERROR_PROC_NOT_FOUND; return finish(); }
    PDH_HQUERY query = nullptr;
    PDH_HCOUNTER counter = nullptr;
    auto error = api.open(nullptr, 0, &query);
    DWORD type = 0;
    if (!error) error = api.add(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter);
    if (!error)
    {
        DWORD bytes = 0;
        error = api.info(counter, FALSE, &bytes, nullptr);
        if (error == PDH_MORE_DATA && bytes >= sizeof(PDH_COUNTER_INFO_W) && bytes <= 1024 * 1024)
        {
            const DWORD capacity = bytes;
            auto info = static_cast<PDH_COUNTER_INFO_W*>(HeapAlloc(GetProcessHeap(), 0, capacity));
            if (!info) error = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                error = api.info(counter, FALSE, &bytes, info);
                if (!error && bytes >= sizeof(*info) && bytes <= capacity) type = info->dwType;
                else if (!error) error = PDH_INVALID_BUFFER;
                HeapFree(GetProcessHeap(), 0, info);
            }
        }
        else if (error == PDH_MORE_DATA || error == ERROR_SUCCESS) error = PDH_INVALID_BUFFER;
        if (!error && type != PERF_100NSEC_TIMER) result.win32_error = ERROR_NOT_SUPPORTED;
    }
    LONGLONG sample = 0;
    if (!error && !result.win32_error) error = api.collect(query, &sample);
    if (!error && !result.win32_error && sample <= 0) result.win32_error = ERROR_INVALID_DATA;
    if (!error && !result.win32_error)
    {
        result.sample_filetime_100ns = static_cast<uint64_t>(sample);
        uint32_t observed = 0, malformed = 0;
        error = ReadGpuPdhArrayWithApi(counter, request->process.pid, result, observed, malformed, api.array);
        result.observed_instance_count = observed;
        for (uint32_t i = 0; i < result.row_count; ++i) result.rows[i].counter_type = type;
    }
    if (query)
    {
        const auto closed = api.close(query);
        if (!error) error = closed;
    }
    result.pdh_error = error;
    if (error || result.win32_error || sample <= 0) result.status = WupUnavailable;
    DWORD lifecycleError = 0;
    const auto after = Identity(process_handle, request->process, lifecycleError, api);
    if (after != WupOk)
    { result.status = after; result.win32_error = lifecycleError; }
    return finish();
}

uint32_t WupNormalizeGpu(const WupGpuSnapshot* previous, const WupGpuSnapshot* current,
    WupGpuResult* output, uint32_t output_size) noexcept
{
    if (!output || output_size < sizeof(*output) || !Valid(current) || (previous && !Valid(previous)))
        return WUP_E_INVALIDARG;
    WupGpuResult result{};
    result.size = sizeof(result); result.version = WUP_GPU_VERSION; result.source = current->source;
    result.process = current->process; result.clock_id = current->clock_id;
    result.begin_100ns = previous ? previous->sample_filetime_100ns : current->sample_filetime_100ns;
    result.end_100ns = current->sample_filetime_100ns;
    const double missing = std::numeric_limits<double>::quiet_NaN();
    result.dedicated_committed = result.shared_committed = result.dedicated_resident =
        result.shared_resident = {WupUnavailable, WupBytes, UINT64_MAX};
    auto finish = [&](WupMetricStatus status) {
        result.status = status;
        result.utilization.status = status;
        if (status != WupOk) result.utilization.percent = missing;
        *output = result;
        return WUP_S_OK;
    };
    if (current->status != WupOk) return finish(current->status);
    if (!current->row_count) return finish(WupUnavailable);
    if (!previous) return finish(WupWarmup);
    if (previous->process.pid != current->process.pid ||
        previous->process.creation_filetime_100ns != current->process.creation_filetime_100ns)
        return finish(WupIdentityChanged);
    if (previous->status != WupOk || !previous->row_count) return finish(WupWarmup);
    if (result.end_100ns <= result.begin_100ns) return finish(WupCounterReset);
    if (previous->row_count != current->row_count) return finish(WupPartial);
    wup::GpuContext contexts[WUP_MAX_GPU_ROWS]{};
    uint64_t commonDenominator = 0;
    for (uint32_t i = 0; i < current->row_count; ++i)
    {
        const auto& now = current->rows[i];
        const WupGpuRawEngine* old = nullptr;
        for (uint32_t j = 0; j < previous->row_count; ++j)
            if (Same(now, previous->rows[j])) { if (old) return finish(WupPartial); old = &previous->rows[j]; }
        if (!old) return finish(WupWarmup);
        if (now.status > WupIdentityChanged || old->status > WupIdentityChanged ||
            now.duplicate_index || old->duplicate_index || now.counter_type != PERF_100NSEC_TIMER ||
            old->counter_type != now.counter_type) return finish(WupPartial);
        if (now.status != WupOk) return finish(now.status);
        if (old->status != WupOk) return finish(WupWarmup);
        if ((now.pdh_status != PDH_CSTATUS_VALID_DATA && now.pdh_status != PDH_CSTATUS_NEW_DATA) ||
            (old->pdh_status != PDH_CSTATUS_VALID_DATA && old->pdh_status != PDH_CSTATUS_NEW_DATA))
            return finish(WupUnavailable);
        if (now.running_time < old->running_time || now.timer <= old->timer ||
            now.timestamp_filetime_100ns <= old->timestamp_filetime_100ns) return finish(WupCounterReset);
        const auto numerator = now.running_time - old->running_time;
        const auto denominator = now.timer - old->timer;
        if (numerator > denominator) return finish(WupPartial);
        if (!i)
        {
            result.begin_100ns = old->timestamp_filetime_100ns;
            result.end_100ns = now.timestamp_filetime_100ns;
            commonDenominator = denominator;
        }
        else if (old->timestamp_filetime_100ns != result.begin_100ns ||
            now.timestamp_filetime_100ns != result.end_100ns ||
            denominator != commonDenominator)
            return finish(WupPartial);
        const double percent = 100.0 * static_cast<double>(numerator) / static_cast<double>(denominator);
        contexts[i] = {{current->process.pid, now.luid_high, now.luid_low, now.physical, now.engine, 0, true},
            0, WupOk, percent, result.begin_100ns, result.end_100ns, result.clock_id};
        result.engines[i] = {now.luid_high, now.luid_low, now.physical, now.engine, WupOk, 0, percent};
    }
    const auto aggregate = wup::AggregateGpu(contexts, current->row_count, current->process.pid);
    result.engine_count = aggregate.engine_count; result.utilization.percent = aggregate.percent;
    return finish(aggregate.status);
}

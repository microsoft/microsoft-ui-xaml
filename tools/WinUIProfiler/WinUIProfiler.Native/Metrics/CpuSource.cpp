#include "CpuSource.h"
#include "CpuSourceApi.h"
#include <cstring>
#include <cstddef>

namespace wup
{
namespace
{
// Compatibility-only layout: Microsoft Learn SPPI and publicly documented
// NtDoc DpcTime/InterruptTime. Never expose this OS layout through the ABI.
using detail::ProcessorTimes;
static_assert(sizeof(ProcessorTimes) == 48 && offsetof(ProcessorTimes, dpc) == 24);
using detail::Inventory;
using detail::QueryEx;
template <typename T> T Resolve(HMODULE module, const char* name)
{
    const auto address = GetProcAddress(module, name);
    T result{};
    static_assert(sizeof(result) == sizeof(address));
    std::memcpy(&result, &address, sizeof(result));
    return result;
}
uint64_t Time(FILETIME value)
{
    return (static_cast<uint64_t>(value.dwHighDateTime) << 32) | value.dwLowDateTime;
}
bool Add(uint64_t& value, int64_t increment)
{
    if (increment < 0 || static_cast<uint64_t>(increment) > UINT64_MAX - value)
    {
        return false;
    }
    value += static_cast<uint64_t>(increment);
    return true;
}

CpuCapture FailCapture(CpuCapture& result, WupMetricStatus status, uint32_t error, uint32_t domain = 1)
{
    auto& snapshot = result.snapshot;
    snapshot.status = status;
    snapshot.group_count = 0;
    snapshot.target_present = 0;
    snapshot.process_kernel = 0;
    snapshot.process_user = 0;
    snapshot.timestamp_100ns = 0;
    for (auto& group : result.groups)
    {
        group = {};
    }
    result.os_error = error;
    result.error_domain = domain;
    return result;
}
}

bool ValidProcessorBuffer(uint32_t bytes, uint32_t count)
{
    return count > 0 && count <= 64 && bytes == count * sizeof(ProcessorTimes);
}

CpuCapture CaptureCpu(HANDLE process, WupProcessIdentity expected)
{
    const auto kernel = GetModuleHandleW(L"kernel32.dll");
    const detail::CpuSourceApi api{Resolve<QueryEx>(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformationEx"),
        Resolve<Inventory>(kernel, "K32EnumProcesses"), Resolve<decltype(&GetProcessTimes)>(kernel, "GetProcessTimes"),
        Resolve<decltype(&GetActiveProcessorGroupCount)>(kernel, "GetActiveProcessorGroupCount"),
        Resolve<decltype(&GetActiveProcessorCount)>(kernel, "GetActiveProcessorCount"), GetProcessId,
        WaitForSingleObject, GetTickCount64, GetLastError, detail::ResolveProcessQuery()};
    return detail::CaptureCpuWithApi(process, expected, api);
}

CpuCapture detail::CaptureCpuWithApi(HANDLE process, WupProcessIdentity expected, const CpuSourceApi& api)
{
    CpuCapture result{};
    auto& s = result.snapshot;
    s.size = sizeof(s);
    s.version = WUP_METRICS_VERSION;
    s.source = api.process_query ? WupNtProcessSnapshotCpu : WupNtGroupProcessTimesCompatibility;
    s.status = WupUnavailable;
    s.clock_id = 2; // GetTickCount64, machine-boot origin, converted to 100 ns.
    s.process = expected;
    result.os_error = ERROR_NOT_SUPPORTED;
    result.error_domain = 1;
    if (!api.query || !api.inventory || !api.times || !api.group_count || !api.cpu_count || !api.process_id ||
        !api.wait || !api.tick || !api.last_error)
    {
        return result;
    }
    if (!process || expected.reserved || !expected.pid || !expected.creation_filetime_100ns)
    {
        result.os_error = ERROR_INVALID_PARAMETER;
        return result;
    }
    const auto detectedGroups = api.group_count();
    if (!detectedGroups || detectedGroups > MaxGroups)
    {
        return result;
    }
    s.expected_group_count = detectedGroups;
    const auto begin = api.tick();
    if (begin > UINT64_MAX / 10000)
    {
        return FailCapture(result, WupUnavailable, ERROR_ARITHMETIC_OVERFLOW);
    }
    result.acquisition_begin_100ns = begin * 10000;
    detail::ProcessSnapshot snapshot;
    if (api.process_query)
    {
        snapshot = detail::CaptureProcessSnapshot(api.process_query, expected);
        result.inventory_count = snapshot.count;
        if (snapshot.status != WupOk)
        {
            return FailCapture(result, snapshot.status, snapshot.error, snapshot.domain);
        }
    }
    else
    {
        DWORD pids[16384];
        DWORD bytes = 0;
        if (!api.inventory(pids, sizeof(pids), &bytes))
        {
            return FailCapture(result, WupUnavailable, api.last_error());
        }
        if (bytes >= sizeof(pids) || bytes % sizeof(DWORD))
        {
            return FailCapture(result, WupUnavailable, ERROR_INVALID_DATA);
        }
        result.inventory_count = bytes / sizeof(DWORD);
        bool found = false;
        for (uint32_t i = 0; i < result.inventory_count; ++i)
        {
            found |= pids[i] == expected.pid;
        }
        if (!found)
        {
            return FailCapture(result, WupTargetMissing, ERROR_NOT_FOUND);
        }
    }
    FILETIME creation{};
    FILETIME exit{};
    FILETIME pk{};
    FILETIME pu{};
    if (!api.times(process, &creation, &exit, &pk, &pu))
    {
        return FailCapture(result, WupUnavailable, api.last_error());
    }
    if (api.process_id(process) != expected.pid || Time(creation) != expected.creation_filetime_100ns)
    {
        s.status = WupIdentityChanged;
        result.os_error = ERROR_INVALID_DATA;
        return result;
    }
    const auto alive = api.wait(process, 0);
    if (alive != WAIT_TIMEOUT)
    {
        s.status = alive == WAIT_OBJECT_0 ? WupTargetMissing : WupUnavailable;
        result.os_error = alive == WAIT_FAILED ? api.last_error() : ERROR_PROCESS_ABORTED;
        return result;
    }
    s.process_kernel = api.process_query ? snapshot.kernel : Time(pk);
    s.process_user = api.process_query ? snapshot.user : Time(pu);
    for (uint32_t id = 0; id < s.expected_group_count; ++id)
    {
        const auto count = api.cpu_count(static_cast<WORD>(id));
        if (!count || count > 64)
        {
            return FailCapture(result, WupUnavailable, ERROR_INVALID_DATA);
        }
        ProcessorTimes raw[64]{};
        ULONG length = 0;
        USHORT group = static_cast<USHORT>(id);
        const auto status = api.query(8, &group, sizeof(group), raw, sizeof(raw), &length);
        if (status != 0 || !ValidProcessorBuffer(length, count))
        {
            return FailCapture(result, WupUnavailable, status != 0 ? static_cast<uint32_t>(status) : ERROR_INVALID_DATA,
                status != 0 ? 2u : 1u);
        }
        auto& g = result.groups[id];
        g = {sizeof(g), WUP_METRICS_VERSION, id, count, 0, 0, 0, 0, 0};
        for (uint32_t cpu = 0; cpu < count; ++cpu)
        {
            const auto& r = raw[cpu];
            if (r.kernel < 0 || r.user < 0 || r.idle < 0 || r.dpc < 0 || r.isr < 0 || r.idle > r.kernel ||
                r.dpc > r.kernel - r.idle || r.isr > r.kernel - r.idle - r.dpc)
            {
                return FailCapture(result, WupUnavailable, ERROR_INVALID_DATA);
            }
            if (!Add(g.kernel, r.kernel) || !Add(g.user, r.user) || !Add(g.idle, r.idle) || !Add(g.dpc, r.dpc) ||
                !Add(g.isr, r.isr))
            {
                return FailCapture(result, WupUnavailable, ERROR_ARITHMETIC_OVERFLOW);
            }
        }
        ++s.group_count;
    }
    if (api.group_count() != s.expected_group_count)
    {
        return FailCapture(result, WupUnavailable, ERROR_RETRY);
    }
    for (uint32_t i = 0; i < s.group_count; ++i)
    {
        if (api.cpu_count(static_cast<WORD>(i)) != result.groups[i].logical_processor_count)
        {
            return FailCapture(result, WupUnavailable, ERROR_RETRY);
        }
    }
    if (!api.times(process, &creation, &exit, &pk, &pu))
    {
        return FailCapture(result, WupUnavailable, api.last_error());
    }
    if (api.process_id(process) != expected.pid || Time(creation) != expected.creation_filetime_100ns)
    {
        return FailCapture(result, WupIdentityChanged, ERROR_INVALID_DATA);
    }
    const auto finalWait = api.wait(process, 0);
    if (finalWait != WAIT_TIMEOUT)
    {
        return FailCapture(result, finalWait == WAIT_OBJECT_0 ? WupTargetMissing : WupUnavailable,
            finalWait == WAIT_FAILED ? api.last_error() : ERROR_PROCESS_ABORTED);
    }
    const auto end = api.tick();
    if (end > UINT64_MAX / 10000 || end < begin)
    {
        return FailCapture(result, WupUnavailable, ERROR_ARITHMETIC_OVERFLOW);
    }
    s.timestamp_100ns = end * 10000;
    s.target_present = 1;
    s.status = WupOk;
    result.os_error = 0;
    result.error_domain = 0;
    return result;
}
}

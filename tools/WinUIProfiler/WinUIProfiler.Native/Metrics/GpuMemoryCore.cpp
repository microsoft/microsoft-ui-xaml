#include "GpuMemoryCore.h"
#include "GpuMemorySourceApi.h"
#include <Windows.h>
#include <dxcore.h>
#include <cstddef>
#include <cstring>

static_assert(sizeof(WupGpuMemoryRequest) == 40);
static_assert(sizeof(WupGpuMemoryResult) == 152);
static_assert(offsetof(WupGpuMemoryResult, acquisition_begin_100ns) == 56);
static_assert(offsetof(WupGpuMemoryResult, dedicated_committed) == 88);
static_assert(offsetof(WupGpuMemoryResult, shared_resident) == 136);

namespace
{
bool Valid(const WupGpuMemoryRequest& r)
{
    return r.size == sizeof(r) && r.version == WUP_GPU_MEMORY_VERSION &&
        !r.reserved && !r.process.reserved && r.process.pid &&
        r.process.creation_filetime_100ns;
}
WupMetricStatus Identity(HANDLE handle, const WupProcessIdentity& expected, DWORD& error)
{
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!GetProcessTimes(handle, &creation, &exit, &kernel, &user))
    {
        error = GetLastError();
        return WupUnavailable;
    }
    const auto pid = GetProcessId(handle);
    if (!pid) { error = GetLastError(); return WupUnavailable; }
    const uint64_t token = (uint64_t(creation.dwHighDateTime) << 32) | creation.dwLowDateTime;
    if (pid != expected.pid || token != expected.creation_filetime_100ns)
    {
        error = ERROR_INVALID_DATA;
        return WupIdentityChanged;
    }
    const auto wait = WaitForSingleObject(handle, 0);
    if (wait == WAIT_OBJECT_0) { error = ERROR_PROCESS_ABORTED; return WupTargetMissing; }
    if (wait != WAIT_TIMEOUT)
    {
        error = wait == WAIT_FAILED ? GetLastError() : ERROR_INVALID_DATA;
        return WupUnavailable;
    }
    error = 0;
    return WupOk;
}
}

uint32_t wup::NormalizeGpuMemory(const WupGpuMemoryRequest& request,
    const GpuMemoryFamily& dedicated, const GpuMemoryFamily& shared,
    WupMetricStatus lifecycle, uint32_t error, uint64_t begin, uint64_t end,
    WupGpuMemoryResult* output, uint32_t output_size) noexcept
{
    if (!Valid(request) || !output || output_size < sizeof(*output) ||
        dedicated.process_query_succeeded > 1 || shared.process_query_succeeded > 1 ||
        (lifecycle != WupOk && lifecycle != WupUnavailable &&
         lifecycle != WupTargetMissing && lifecycle != WupIdentityChanged))
        return WUP_E_INVALIDARG;
    WupGpuMemoryResult r{};
    r.size = sizeof(r); r.version = WUP_GPU_MEMORY_VERSION;
    r.source = WupDxCoreProcessMemory; r.process = request.process;
    r.luid_high = request.luid_high; r.luid_low = request.luid_low;
    r.physical_index = request.physical_index; r.clock_id = 2;
    r.dedicated_hresult = dedicated.hresult; r.shared_hresult = shared.hresult;
    r.process_query_flags = dedicated.process_query_succeeded | (shared.process_query_succeeded << 1);
    if (end < begin)
    {
        lifecycle = WupUnavailable; error = ERROR_INVALID_DATA;
    }
    else { r.acquisition_begin_100ns = begin; r.acquisition_end_100ns = end; }
    r.win32_error = error;
    auto family = [&](const GpuMemoryFamily& f, WupByteMetric& committed, WupByteMetric& resident) {
        const bool measured = lifecycle == WupOk && !(f.hresult & 0x80000000u) &&
            f.process_query_succeeded && f.committed != UINT64_MAX && f.resident != UINT64_MAX;
        const auto status = measured ? WupOk : (lifecycle == WupOk ? WupUnavailable : lifecycle);
        committed = {status, WupBytes, measured ? f.committed : UINT64_MAX};
        resident = {status, WupBytes, measured ? f.resident : UINT64_MAX};
        return measured;
    };
    const bool d = family(dedicated, r.dedicated_committed, r.dedicated_resident);
    const bool s = family(shared, r.shared_committed, r.shared_resident);
    r.status = lifecycle != WupOk ? lifecycle :
        (d && s ? WupOk : (d || s ? WupPartial : WupUnavailable));
    *output = r;
    return WUP_S_OK;
}

namespace
{
struct DxContext { HMODULE module{}; IDXCoreAdapterFactory* factory{}; IDXCoreAdapter* adapter{}; };
uint32_t Open(void* context, uint32_t high, uint32_t low, uint32_t* error)
{
    auto& c = *static_cast<DxContext*>(context);
    c.module = LoadLibraryExW(L"dxcore.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!c.module) { *error = GetLastError(); return HRESULT_FROM_WIN32(*error); }
    using Create = HRESULT(WINAPI*)(REFIID, void**);
    const auto address = GetProcAddress(c.module, "DXCoreCreateAdapterFactory");
    if (!address) { *error = GetLastError(); return HRESULT_FROM_WIN32(*error); }
    Create create{};
    static_assert(sizeof(create) == sizeof(address));
    std::memcpy(&create, &address, sizeof(create));
    auto hr = create(IID_PPV_ARGS(&c.factory));
    if (FAILED(hr)) return hr;
    if (!c.factory) return static_cast<uint32_t>(E_UNEXPECTED);
    LUID luid{low, 0};
    static_assert(sizeof(high) == sizeof(luid.HighPart));
    std::memcpy(&luid.HighPart, &high, sizeof(high));
    hr = c.factory->GetAdapterByLuid(luid, IID_PPV_ARGS(&c.adapter));
    return SUCCEEDED(hr) && !c.adapter ? E_UNEXPECTED : hr;
}
uint32_t PhysicalCount(void* context, uint32_t* count)
{
    auto* adapter = static_cast<DxContext*>(context)->adapter;
    if (!adapter->IsPropertySupported(DXCoreAdapterProperty::PhysicalAdapterCount))
        return 0x887A0004u; // DXGI_ERROR_UNSUPPORTED.
    return adapter->GetProperty(DXCoreAdapterProperty::PhysicalAdapterCount, count);
}
bool Supported(void* context)
{
    return static_cast<DxContext*>(context)->adapter->IsQueryStateSupported(
        DXCoreAdapterState::AdapterMemoryUsageByProcessBytes);
}
wup::GpuMemoryFamily Query(void* context, uint32_t physical, uint32_t pid, bool shared)
{
    DXCoreProcessMemoryQueryInput input{physical,
        shared ? DXCoreMemoryType::Shared : DXCoreMemoryType::Dedicated, pid};
    DXCoreProcessMemoryQueryOutput output{};
    const auto hr = static_cast<DxContext*>(context)->adapter->QueryState(
        DXCoreAdapterState::AdapterMemoryUsageByProcessBytes, &input, &output);
    return {static_cast<uint32_t>(hr), output.processQuerySucceeded ? 1u : 0u,
        output.memoryUsage.committed, output.memoryUsage.resident};
}
void Close(void* context)
{
    auto& c = *static_cast<DxContext*>(context);
    if (c.adapter) c.adapter->Release();
    if (c.factory) c.factory->Release();
    if (c.module) FreeLibrary(c.module);
    c = {};
}
uint32_t CheckIdentity(void*, void* handle, WupProcessIdentity expected, uint32_t* error)
{
    DWORD win32{};
    const auto status = Identity(handle, expected, win32);
    *error = win32;
    return status;
}
uint64_t Tick(void*) { return GetTickCount64(); }
}

uint32_t WupCaptureGpuMemory(void* handle, const WupGpuMemoryRequest* request,
    WupGpuMemoryResult* output, uint32_t output_size) noexcept
{
    DxContext context;
    const wup::detail::GpuMemorySourceApi api{
        &context, Open, PhysicalCount, Supported, Query, Close, CheckIdentity, Tick};
    return wup::detail::CaptureGpuMemoryWithApi(handle, request, output, output_size, api);
}

uint32_t wup::detail::CaptureGpuMemoryWithApi(void* handle, const WupGpuMemoryRequest* request,
    WupGpuMemoryResult* output, uint32_t output_size, const GpuMemorySourceApi& api) noexcept
{
    if (!request || !Valid(*request) || !output || output_size < sizeof(*output) ||
        !api.open || !api.physical_count || !api.supported || !api.query ||
        !api.close || !api.identity || !api.tick)
        return WUP_E_INVALIDARG;
    const auto begin = api.tick(api.context);
    uint32_t error = 0;
    auto lifecycle = api.identity(api.context, handle, request->process, &error);
    GpuMemoryFamily dedicated{0x80004001u, 0, UINT64_MAX, UINT64_MAX}, shared = dedicated;
    if (lifecycle == WupOk)
    {
        auto hr = api.open(api.context, request->luid_high, request->luid_low, &error);
        if (!(hr & 0x80000000u))
        {
            uint32_t count = 0;
            hr = api.physical_count(api.context, &count);
            if (!(hr & 0x80000000u) && !count) hr = 0x8000FFFFu;
            if (!(hr & 0x80000000u) && request->physical_index >= count) hr = WUP_E_INVALIDARG;
            if (!(hr & 0x80000000u) && !api.supported(api.context)) hr = 0x887A0004u;
        }
        if (!(hr & 0x80000000u))
        {
            dedicated = api.query(api.context, request->physical_index, request->process.pid, false);
            shared = api.query(api.context, request->physical_index, request->process.pid, true);
        }
        else dedicated.hresult = shared.hresult = hr;
        api.close(api.context); // Also release partial factory/module acquisition on failure.
        uint32_t post_error = 0;
        lifecycle = api.identity(api.context, handle, request->process, &post_error);
        if (lifecycle != WupOk) error = post_error;
    }
    const auto end = api.tick(api.context);
    const bool overflow = begin > UINT64_MAX / 10000 || end > UINT64_MAX / 10000;
    if (overflow) { lifecycle = WupUnavailable; error = ERROR_ARITHMETIC_OVERFLOW; }
    return wup::NormalizeGpuMemory(*request, dedicated, shared, lifecycle, error,
        overflow ? 0 : begin * 10000, overflow ? 0 : end * 10000, output, output_size);
}

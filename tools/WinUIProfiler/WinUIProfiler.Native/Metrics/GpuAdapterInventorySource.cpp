#include "GpuAdapterInventory.h"
#include "GpuAdapterInventoryApi.h"
#include <Windows.h>
#include <dxgi.h>
#include <dxcore.h>
#include <cstring>

namespace
{
template<typename T> struct Com {
    T* value{};
    ~Com() { if (value) value->Release(); }
};
struct State {
    HMODULE dxgi{}, dxcore{};
    Com<IDXGIFactory1> graphics;
    Com<IDXCoreAdapterFactory> core;
    ~State()
    {
        // Release interfaces while their implementing modules are still loaded.
        if (core.value) { core.value->Release(); core.value = nullptr; }
        if (graphics.value) { graphics.value->Release(); graphics.value = nullptr; }
        if (dxcore) FreeLibrary(dxcore);
        if (dxgi) FreeLibrary(dxgi);
    }
};
using Create = HRESULT(WINAPI*)(REFIID, void**);
uint32_t Resolve(HMODULE& module, const wchar_t* name, const char* symbol,
    Create& create, uint32_t& error)
{
    module = LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) { error = GetLastError(); return HRESULT_FROM_WIN32(error); }
    const auto address = GetProcAddress(module, symbol);
    if (!address) { error = GetLastError(); return HRESULT_FROM_WIN32(error); }
    static_assert(sizeof(create) == sizeof(address));
    std::memcpy(&create, &address, sizeof(create));
    return WUP_S_OK;
}
bool Failed(uint32_t hr) { return (hr & 0x80000000u) != 0; }
struct Token {
    const wup::detail::GpuAdapterInventoryApi& api;
    bool core;
    void* value{};
    ~Token() { if (value) api.release(api.context, value, core); }
};
void Enrich(const wup::detail::GpuAdapterInventoryApi& api, void* factory,
    const LUID& luid, WupGpuAdapterInventoryRow& row, uint32_t core_hr, uint32_t core_error)
{
    row.dxcore_lookup_hresult = core_hr;
    row.win32_error = core_error;
    if (Failed(core_hr)) return;
    Token adapter{api, true};
    row.dxcore_lookup_hresult = api.lookup(api.context, factory, luid, &adapter.value);
    if (Failed(row.dxcore_lookup_hresult)) return;
    if (!adapter.value) { row.dxcore_lookup_hresult = 0x8000FFFFu; return; }
    {
        uint32_t count = UINT32_MAX;
        row.count_hresult = api.property(api.context, adapter.value, false, &count);
        if (!Failed(row.count_hresult))
        {
            row.physical_count = count;
            row.known_flags |= WupAdapterCountKnown;
        }
    }
    {
        uint32_t hardware = UINT32_MAX;
        row.hardware_hresult = api.property(api.context, adapter.value, true, &hardware);
        if (!Failed(row.hardware_hresult))
        {
            row.is_hardware = hardware;
            row.known_flags |= WupAdapterHardwareKnown;
        }
    }
    row.state3_supported = api.supported(api.context, adapter.value) ? 1u : 0u;
    row.known_flags |= WupAdapterStateKnown;
}
uint32_t Open(void* context, bool core, void** object, uint32_t* error)
{
    auto& s = *static_cast<State*>(context);
    Create create{};
    auto hr = Resolve(core ? s.dxcore : s.dxgi, core ? L"dxcore.dll" : L"dxgi.dll",
        core ? "DXCoreCreateAdapterFactory" : "CreateDXGIFactory1", create, *error);
    if (!Failed(hr))
    {
        if (core) { hr = create(IID_PPV_ARGS(&s.core.value)); *object = s.core.value; }
        else { hr = create(IID_PPV_ARGS(&s.graphics.value)); *object = s.graphics.value; }
    }
    return hr;
}
bool Current(void*, void* factory) { return static_cast<IDXGIFactory1*>(factory)->IsCurrent() != FALSE; }
uint32_t Enumerate(void*, void* factory, uint32_t index, void** out)
{ return static_cast<IDXGIFactory1*>(factory)->EnumAdapters1(index, reinterpret_cast<IDXGIAdapter1**>(out)); }
uint32_t Describe(void*, void* adapter, DXGI_ADAPTER_DESC1* out)
{ return static_cast<IDXGIAdapter1*>(adapter)->GetDesc1(out); }
uint32_t Lookup(void*, void* factory, const LUID& luid, void** out)
{ return static_cast<IDXCoreAdapterFactory*>(factory)->GetAdapterByLuid(luid, __uuidof(IDXCoreAdapter), out); }
uint32_t Property(void*, void* token, bool hardware, uint32_t* value)
{
    auto* a = static_cast<IDXCoreAdapter*>(token);
    const auto property = hardware ? DXCoreAdapterProperty::IsHardware : DXCoreAdapterProperty::PhysicalAdapterCount;
    if (!a->IsPropertySupported(property)) return 0x887A0004u;
    if (!hardware) return a->GetProperty(property, value);
    static_assert(sizeof(bool) == 1);
    uint8_t flag = 0xff;
    const auto hr = a->GetProperty(property, sizeof(flag), &flag);
    if (SUCCEEDED(hr) && flag <= 1) *value = flag;
    return hr;
}
bool Supported(void*, void* a)
{ return static_cast<IDXCoreAdapter*>(a)->IsQueryStateSupported(DXCoreAdapterState::AdapterMemoryUsageByProcessBytes); }
void Release(void*, void* token, bool core)
{ if (core) static_cast<IDXCoreAdapter*>(token)->Release(); else static_cast<IDXGIAdapter1*>(token)->Release(); }
void Close(void* context)
{
    auto& s = *static_cast<State*>(context);
    if (s.core.value) { s.core.value->Release(); s.core.value = nullptr; }
    if (s.graphics.value) { s.graphics.value->Release(); s.graphics.value = nullptr; }
    if (s.dxcore) { FreeLibrary(s.dxcore); s.dxcore = nullptr; }
    if (s.dxgi) { FreeLibrary(s.dxgi); s.dxgi = nullptr; }
}
uint64_t Tick(void*) { return GetTickCount64(); }
}

uint32_t WupEnumerateGpuAdapters(const WupGpuAdapterInventoryRequest* request,
    WupGpuAdapterInventoryResult* output, uint32_t output_size) noexcept
{
    State state;
    const wup::detail::GpuAdapterInventoryApi api{
        &state, Open, Current, Enumerate, Describe, Lookup, Property, Supported, Release, Close, Tick};
    return wup::detail::EnumerateGpuAdaptersWithApi(request, output, output_size, api);
}
uint32_t wup::detail::EnumerateGpuAdaptersWithApi(const WupGpuAdapterInventoryRequest* request,
    WupGpuAdapterInventoryResult* output, uint32_t output_size, const GpuAdapterInventoryApi& api) noexcept
{
    if (!output || output_size < sizeof(*output) || !request ||
        request->size != sizeof(*request) || request->version != WUP_GPU_INVENTORY_VERSION ||
        request->scope != WupDxgiVisibleGraphics || request->reserved ||
        !api.open || !api.current || !api.enumerate || !api.describe || !api.lookup ||
        !api.property || !api.supported || !api.release || !api.close || !api.tick)
        return WUP_E_INVALIDARG;
    WupGpuAdapterInventoryResult raw{};
    raw.size = sizeof(raw); raw.version = WUP_GPU_INVENTORY_VERSION;
    raw.source = WupDxgiGraphicsInventory; raw.scope = WupDxgiVisibleGraphics;
    raw.clock_id = 2; raw.capacity = WUP_MAX_GPU_ADAPTERS;
    const auto begin = api.tick(api.context);
    {
        void* graphics{};
        auto hr = api.open(api.context, false, &graphics, &raw.win32_error);
        if (!Failed(hr) && !graphics) hr = 0x8000FFFFu;
        raw.enumeration_hresult = hr;
        if (!Failed(hr))
        {
            raw.factory_current_begin = api.current(api.context, graphics) ? 1u : 0u;
            uint32_t core_error = 0;
            void* core{};
            auto core_hr = api.open(api.context, true, &core, &core_error);
            if (!Failed(core_hr) && !core) core_hr = 0x8000FFFFu;
            for (uint32_t index = 0; index <= WUP_MAX_GPU_ADAPTERS; ++index)
            {
                Token adapter{api, false};
                raw.enumeration_hresult = api.enumerate(api.context, graphics, index, &adapter.value);
                if (raw.enumeration_hresult == 0x887A0002u)
                { raw.flags |= WupInventoryEnded; break; }
                if (Failed(raw.enumeration_hresult)) break;
                if (!adapter.value) { raw.enumeration_hresult = 0x8000FFFFu; break; }
                if (index == WUP_MAX_GPU_ADAPTERS)
                { raw.flags |= WupInventoryTruncated; break; }
                auto& row = raw.rows[raw.row_count++];
                row.size = sizeof(row); row.version = WUP_GPU_INVENTORY_VERSION;
                row.physical_count = row.is_hardware = row.state3_supported = UINT32_MAX;
                row.dxcore_lookup_hresult = row.count_hresult = row.hardware_hresult = 0x80004001u;
                DXGI_ADAPTER_DESC1 desc{};
                row.desc_hresult = api.describe(api.context, adapter.value, &desc);
                if (!Failed(row.desc_hresult))
                {
                    row.known_flags = WupAdapterDescriptorKnown | WupAdapterIdentityKnown;
                    std::memcpy(&row.luid_high, &desc.AdapterLuid.HighPart, sizeof(row.luid_high));
                    row.luid_low = desc.AdapterLuid.LowPart;
                    row.vendor_id = desc.VendorId; row.device_id = desc.DeviceId;
                    row.subsystem_id = desc.SubSysId; row.revision = desc.Revision;
                    row.dxgi_flags = desc.Flags;
                    Enrich(api, core, desc.AdapterLuid, row, core_hr, core_error);
                }
            }
            raw.factory_current_end = api.current(api.context, graphics) ? 1u : 0u;
        }
    }
    api.close(api.context);
    const auto end = api.tick(api.context);
    if (begin > UINT64_MAX / 10000 || end > UINT64_MAX / 10000)
    {
        raw.win32_error = ERROR_ARITHMETIC_OVERFLOW;
        raw.factory_current_end = 0;
    }
    else
    {
        raw.acquisition_begin_100ns = begin * 10000;
        raw.acquisition_end_100ns = end * 10000;
    }
    return wup::NormalizeGpuAdapterInventory(*request, raw, output, output_size);
}

#include "../WinUIProfiler.Native/Metrics/GpuAdapterInventory.h"
#include <cstdio>
#if __has_include("../WinUIProfiler.Native/Metrics/GpuAdapterInventoryApi.h")
#include "../WinUIProfiler.Native/Metrics/GpuAdapterInventoryApi.h"
#define HAVE_API 1
#endif
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL %d: %s\n", __LINE__, #x); } } while (0)
#ifdef HAVE_API
struct State { int scenario; uint32_t count{}, released{}, core_released{}, closed{}, currents{}; };
static uint32_t Open(void* p, bool core, void** object, uint32_t* error)
{
    auto& s = *static_cast<State*>(p); *error = 0;
    *object = ((s.scenario == 2 && !core) || (s.scenario == 5 && core)) ? nullptr : &s;
    if (s.scenario == 1 && !core) { *error = 126; return 0x8007007Eu; }
    if (s.scenario == 4 && core) return 0x80004005u;
    return 0;
}
static bool Current(void* p, void*)
{ auto& s = *static_cast<State*>(p); return !(s.scenario == 13 && ++s.currents == 2); }
static uint32_t Enum(void* p, void*, uint32_t index, void** object)
{
    auto& s = *static_cast<State*>(p); ++s.count; *object = nullptr;
    if (s.scenario == 3) return 0x80004005u;
    const uint32_t rows = s.scenario == 14 ? 64 : (s.scenario == 15 ? 65 : (s.scenario == 16 ? 0 : 2));
    if (index >= rows) return 0x887A0002u;
    if (s.scenario != 6) *object = reinterpret_cast<void*>(uintptr_t(index + 1));
    return 0;
}
static uint32_t Desc(void* p, void* object, DXGI_ADAPTER_DESC1* out)
{
    auto& s = *static_cast<State*>(p);
    out->AdapterLuid.LowPart = s.scenario == 12 ? 1u : uint32_t(reinterpret_cast<uintptr_t>(object));
    out->VendorId = s.scenario == 17 ? 0x1414u : 0x10deu;
    out->DeviceId = s.scenario == 17 ? 0x8cu : 1u;
    if (s.scenario == 7) return 0x80004005u;
    return 0;
}
static uint32_t Lookup(void* p, void*, const LUID&, void** object)
{ auto& s = *static_cast<State*>(p); *object = s.scenario == 9 ? nullptr : &s; return s.scenario == 8 ? 0x80070057u : 0; }
static uint32_t Property(void* p, void*, bool hardware, uint32_t* value)
{
    auto& s = *static_cast<State*>(p);
    if (s.scenario == 10) return 0x887A0004u;
    if (s.scenario == 11 && !hardware) { *value = 0; return 0; }
    if (s.scenario == 18 && !hardware) { *value = 65; return 0; }
    if (s.scenario == 21) return 0; // Successful but unwritten output must not invent hardware.
    *value = 1; return 0;
}
static bool Support(void*, void*) { return true; }
static void Release(void* p, void*, bool core)
{ auto& s = *static_cast<State*>(p); if (core) ++s.core_released; else ++s.released; }
static void Close(void* p) { ++static_cast<State*>(p)->closed; }
static uint64_t Tick(void* p)
{ auto& s = *static_cast<State*>(p); return !s.closed ? 100 : (s.scenario == 19 ? 99 : (s.scenario == 20 ? UINT64_MAX : 110)); }
#endif
int main()
{
#ifndef HAVE_API
    CHECK(false && "inventory source fault boundary missing");
#else
    for (int scenario = 0; scenario < 22; ++scenario)
    {
        State s{scenario};
        wup::detail::GpuAdapterInventoryApi api{&s, Open, Current, Enum, Desc, Lookup, Property, Support, Release, Close, Tick};
        WupGpuAdapterInventoryRequest request{16, 1, 1, 0};
        WupGpuAdapterInventoryResult result{};
        CHECK(wup::detail::EnumerateGpuAdaptersWithApi(&request, &result, 5200, api) == 0);
        CHECK(s.closed == 1 && s.count <= 65);
        CHECK(s.released == (scenario == 6 ? 0 : (s.count - ((scenario == 3 || scenario == 1 || scenario == 2) ? s.count : (scenario == 15 ? 0 : 1)))));
        if (scenario == 0) CHECK(result.status == WupOk && result.row_count == 2);
        if (scenario == 1) CHECK(result.enumeration_hresult == 0x8007007Eu && result.win32_error == 126);
        if (scenario == 2 || scenario == 6) CHECK(result.enumeration_hresult == 0x8000FFFFu);
        if (scenario == 7) CHECK(result.rows[0].status == WupUnavailable && !result.rows[0].luid_low);
        if (scenario == 8) CHECK(result.rows[0].dxcore_lookup_hresult == 0x80070057u);
        if (scenario == 9) CHECK(result.rows[0].dxcore_lookup_hresult == 0x8000FFFFu);
        if (scenario == 12) CHECK(result.flags & WupInventoryDuplicate);
        if (scenario == 13 || scenario == 19 || scenario == 20) CHECK(result.flags & WupInventoryStale);
        if (scenario == 14) CHECK(result.row_count == 64 && result.flags == WupInventoryEnded);
        if (scenario == 15) CHECK(result.row_count == 64 && (result.flags & WupInventoryTruncated));
        if (scenario == 16) CHECK(!result.row_count && result.status == WupOk);
        if (scenario == 17) CHECK(!wup::CanSelectGpuMemoryAdapter(result.rows[0], true));
        if (scenario == 18) CHECK(result.rows[0].physical_count == 65 && result.status == WupPartial);
        if (scenario == 21) CHECK(result.rows[0].is_hardware == UINT32_MAX && result.rows[0].physical_count == UINT32_MAX);
    }
#endif
    std::printf("Inventory source faults: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}

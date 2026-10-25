#include "../WinUIProfiler.Native/Metrics/GpuAdapterInventory.h"
#include <cstdio>
#include <cstring>
int main()
{
    WupGpuAdapterInventoryRequest request{sizeof(request), 1, 1, 0};
    struct Guard { uint64_t a; WupGpuAdapterInventoryResult value; uint64_t b; };
    Guard guard{11, {}, 22};
    int failures = 0;
    if (WupEnumerateGpuAdapters(&request, &guard.value, 5199) != WUP_E_INVALIDARG ||
        guard.value.size != 0) ++failures;
    request.version = 2;
    if (WupEnumerateGpuAdapters(&request, &guard.value, sizeof(guard.value)) != WUP_E_INVALIDARG ||
        guard.value.size != 0) ++failures;
    request.version = 1;
    request.scope = 2;
    if (WupEnumerateGpuAdapters(&request, &guard.value, sizeof(guard.value)) != WUP_E_INVALIDARG ||
        guard.value.size != 0) ++failures;
    request.scope = 1; request.reserved = 1;
    if (WupEnumerateGpuAdapters(&request, &guard.value, sizeof(guard.value)) != WUP_E_INVALIDARG ||
        guard.value.size != 0) ++failures;
    request.reserved = 0;
    if (WupEnumerateGpuAdapters(nullptr, &guard.value, sizeof(guard.value)) != WUP_E_INVALIDARG ||
        guard.value.size != 0) ++failures;
    const auto hr = WupEnumerateGpuAdapters(&request, &guard.value, sizeof(guard.value));
    const auto& r = guard.value;
    if (hr || r.size != 5200 || r.version != 1 || r.source != 8 || r.scope != 1 ||
        r.row_count > 64 || r.capacity != 64 || r.clock_id != 2 ||
        r.acquisition_end_100ns < r.acquisition_begin_100ns) ++failures;
    std::printf("DXGI-visible inventory ONLY: dispatch=%08X status=%u rows=%u flags=%u enumHR=%08X Win32=%u current=%u/%u bracket=%llu..%llu clock=%llu\n",
        hr, r.status, r.row_count, r.flags, r.enumeration_hresult, r.win32_error,
        r.factory_current_begin, r.factory_current_end, r.acquisition_begin_100ns,
        r.acquisition_end_100ns, r.clock_id);
    for (uint32_t i = 0; i < r.row_count && i < 64; ++i)
    {
        const auto& row = r.rows[i];
        if (!(row.known_flags & WupAdapterHardwareKnown) && row.is_hardware != UINT32_MAX) ++failures;
        if (!(row.known_flags & WupAdapterCountKnown) && row.physical_count != UINT32_MAX) ++failures;
        if (!(row.known_flags & WupAdapterStateKnown) && row.state3_supported != UINT32_MAX) ++failures;
        if (!(row.known_flags & WupAdapterIdentityKnown) && (row.luid_high || row.luid_low)) ++failures;
        if ((row.known_flags & WupAdapterBasicRender) && wup::CanSelectGpuMemoryAdapter(row, true)) ++failures;
        std::printf(" row%u status=%u known=%u LUID=%08X:%08X vendor/device=%04X:%04X DXGIflags=%u physical=%u hardware=%u state3=%u HR(desc/lookup/count/hardware)=%08X/%08X/%08X/%08X Win32=%u\n",
            i, row.status, row.known_flags, row.luid_high, row.luid_low, row.vendor_id,
            row.device_id, row.dxgi_flags, row.physical_count, row.is_hardware,
            row.state3_supported, row.desc_hresult, row.dxcore_lookup_hresult,
            row.count_hresult, row.hardware_hresult, row.win32_error);
    }
    WupGpuAdapterInventoryRow zero{};
    for (uint32_t i = r.row_count; i < 64; ++i)
        if (std::memcmp(&r.rows[i], &zero, sizeof(zero))) ++failures;
    if (guard.a != 11 || guard.b != 22) ++failures;
    const auto saved = guard.value;
    request.size--;
    if (WupEnumerateGpuAdapters(&request, &guard.value, sizeof(guard.value)) != WUP_E_INVALIDARG ||
        std::memcmp(&saved, &guard.value, sizeof(saved))) ++failures;
    std::printf("Inventory live ABI: %d failures; availability/classification is NOT hardware or all-GPU proof\n", failures);
    return failures ? 1 : 0;
}

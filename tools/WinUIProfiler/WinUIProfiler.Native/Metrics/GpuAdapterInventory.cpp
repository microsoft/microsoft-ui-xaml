#include "GpuAdapterInventory.h"
#include <cstddef>
static_assert(sizeof(WupGpuAdapterInventoryRequest) == 16);
static_assert(sizeof(WupGpuAdapterInventoryRow) == 80);
static_assert(sizeof(WupGpuAdapterInventoryResult) == 5200);
static_assert(offsetof(WupGpuAdapterInventoryResult, rows) == 80);
static_assert(offsetof(WupGpuAdapterInventoryResult, enumeration_hresult) == 56);
static_assert(offsetof(WupGpuAdapterInventoryRow, physical_count) == 44);
static_assert(offsetof(WupGpuAdapterInventoryRow, desc_hresult) == 56);
namespace
{
bool Failed(uint32_t hr) { return (hr & 0x80000000u) != 0; }
}
bool wup::CanSelectGpuMemoryAdapter(const WupGpuAdapterInventoryRow& r, bool hardware_only) noexcept
{
    const uint32_t essential = WupAdapterDescriptorKnown | WupAdapterIdentityKnown |
        WupAdapterCountKnown | WupAdapterStateKnown;
    if (r.size != sizeof(r) || r.version != 1 || (r.known_flags & essential) != essential ||
        (r.known_flags & WupAdapterDuplicate) || !r.physical_count || r.physical_count > 64 ||
        r.state3_supported != 1 || Failed(r.desc_hresult) || Failed(r.dxcore_lookup_hresult) ||
        Failed(r.count_hresult)) return false;
    return !hardware_only || ((r.known_flags & WupAdapterHardwareKnown) &&
        !Failed(r.hardware_hresult) && r.is_hardware == 1 &&
        !(r.dxgi_flags & 2) && !(r.known_flags & WupAdapterBasicRender));
}
uint32_t wup::NormalizeGpuAdapterInventory(const WupGpuAdapterInventoryRequest& request,
    const WupGpuAdapterInventoryResult& input, WupGpuAdapterInventoryResult* output,
    uint32_t output_size) noexcept
{
    if (!output || output_size < sizeof(*output) || request.size != sizeof(request) ||
        request.version != 1 || request.scope != 1 || request.reserved ||
        input.size != sizeof(input) || input.version != 1 || input.source != 8 ||
        input.scope != 1 || input.clock_id != 2 || input.capacity != 64 ||
        input.row_count > 64 || input.reserved0 || input.reserved1 ||
        input.factory_current_begin > 1 || input.factory_current_end > 1 ||
        (input.flags & ~31u)) return WUP_E_INVALIDARG;
    for (uint32_t i = 0; i < input.row_count; ++i)
        if (input.rows[i].size != 80 || input.rows[i].version != 1 ||
            input.rows[i].reserved || (input.rows[i].known_flags & ~127u))
            return WUP_E_INVALIDARG;
    WupGpuAdapterInventoryResult result{};
    result = input;
    result.flags &= WupInventoryEnded | WupInventoryTruncated;
    for (uint32_t i = input.row_count; i < 64; ++i) result.rows[i] = {};
    if (!result.factory_current_begin || !result.factory_current_end)
        result.flags |= WupInventoryStale;
    if ((result.flags & WupInventoryEnded) && result.enumeration_hresult != 0x887A0002u)
        result.flags &= ~WupInventoryEnded;
    if (result.acquisition_end_100ns < result.acquisition_begin_100ns)
    {
        result.win32_error = 13;
        result.acquisition_begin_100ns = result.acquisition_end_100ns = 0;
        result.flags |= WupInventoryStale;
    }
    for (uint32_t i = 0; i < result.row_count; ++i)
    {
        auto& r = result.rows[i];
        r.known_flags &= 31;
        if (Failed(r.desc_hresult) || !(r.known_flags & WupAdapterDescriptorKnown))
        {
            r.known_flags = 0; r.luid_high = r.luid_low = 0;
            r.vendor_id = r.device_id = r.subsystem_id = r.revision = r.dxgi_flags = 0;
        }
        if (!(r.known_flags & WupAdapterIdentityKnown) || Failed(r.dxcore_lookup_hresult))
            r.known_flags &= WupAdapterDescriptorKnown | WupAdapterIdentityKnown;
        if (Failed(r.count_hresult) || !r.physical_count || r.physical_count == UINT32_MAX)
            r.known_flags &= ~WupAdapterCountKnown;
        if (!(r.known_flags & WupAdapterCountKnown)) r.physical_count = UINT32_MAX;
        if (Failed(r.hardware_hresult) || r.is_hardware > 1)
            r.known_flags &= ~WupAdapterHardwareKnown;
        if (!(r.known_flags & WupAdapterHardwareKnown)) r.is_hardware = UINT32_MAX;
        if (r.state3_supported > 1) r.known_flags &= ~WupAdapterStateKnown;
        if (!(r.known_flags & WupAdapterStateKnown)) r.state3_supported = UINT32_MAX;
        const bool complete = (r.known_flags & 31) == 31 && r.physical_count <= 64;
        r.status = !(r.known_flags & WupAdapterDescriptorKnown) ? WupUnavailable :
            (complete ? WupOk : WupPartial);
        if ((r.known_flags & WupAdapterDescriptorKnown) && r.vendor_id == 0x1414 && r.device_id == 0x8c)
            r.known_flags |= WupAdapterBasicRender;
        if (r.status != WupOk) result.flags |= WupInventoryIncompleteRows;
    }
    for (uint32_t i = 0; i < result.row_count; ++i)
        for (uint32_t j = 0; j < i; ++j)
        {
            auto& a = result.rows[i]; auto& b = result.rows[j];
            if ((a.known_flags & b.known_flags & WupAdapterIdentityKnown) &&
                a.luid_high == b.luid_high && a.luid_low == b.luid_low)
            {
                a.known_flags |= WupAdapterDuplicate; b.known_flags |= WupAdapterDuplicate;
                a.status = b.status = WupPartial;
                result.flags |= WupInventoryDuplicate | WupInventoryIncompleteRows;
            }
        }
    const bool complete = result.flags == WupInventoryEnded && !result.win32_error;
    result.status = complete ? WupOk : (result.row_count ? WupPartial : WupUnavailable);
    *output = result;
    return WUP_S_OK;
}

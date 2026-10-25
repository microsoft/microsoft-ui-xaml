#include "../WinUIProfiler.Native/Exports.h"
#include <cstdio>
#include <cstring>
#include <cstddef>
#ifdef WUP_GPU_INVENTORY_VERSION
#include "../WinUIProfiler.Native/Metrics/GpuAdapterInventory.h"
#endif
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL %d: %s\n", __LINE__, #x); } } while (0)
int main()
{
#ifndef WUP_GPU_INVENTORY_VERSION
    CHECK(false && "inventory ABI/core missing");
#else
    static_assert(sizeof(WupGpuAdapterInventoryRequest) == 16);
    static_assert(sizeof(WupGpuAdapterInventoryRow) == 80);
    static_assert(sizeof(WupGpuAdapterInventoryResult) == 5200);
    static_assert(offsetof(WupGpuAdapterInventoryResult, rows) == 80);
    static_assert(offsetof(WupGpuAdapterInventoryRow, physical_count) == 44);
    static_assert(sizeof(WupGpuMemoryRequest) == 40 && sizeof(WupGpuMemoryResult) == 152);
    WupGpuAdapterInventoryRequest request{sizeof(request), 1, 1, 0};
    WupGpuAdapterInventoryResult input{};
    input.size = sizeof(input); input.version = 1; input.source = 8; input.scope = 1;
    input.clock_id = 2; input.acquisition_begin_100ns = 100; input.acquisition_end_100ns = 110;
    input.capacity = 64; input.factory_current_begin = input.factory_current_end = 1;
    input.flags = WupInventoryEnded; input.enumeration_hresult = 0x887A0002u;
    input.row_count = 1;
    input.rows[0] = {80, 1, 0, 31, 9, 8, 0x10de, 1, 0, 0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0};
    struct Guard { uint64_t a; WupGpuAdapterInventoryResult out; uint64_t b; };
    Guard guard{11, {}, 22};
    auto normalize = [&] { return wup::NormalizeGpuAdapterInventory(
        request, input, &guard.out, sizeof(guard.out)); };
    CHECK(normalize() == WUP_S_OK && guard.out.status == WupOk);
    CHECK(wup::CanSelectGpuMemoryAdapter(guard.out.rows[0], true));
    WupGpuAdapterInventoryRow zero{};
    CHECK(!std::memcmp(&guard.out.rows[1], &zero, sizeof(zero)));
    for (int scenario = 0; scenario < 12; ++scenario)
    {
        auto altered = input;
        auto& row = altered.rows[0];
        if (scenario == 0) row.desc_hresult = 0x80004005u;
        if (scenario == 1) row.dxcore_lookup_hresult = 0x8007007Eu;
        if (scenario == 2) row.count_hresult = 0x887A0004u;
        if (scenario == 3) row.hardware_hresult = 0x80070005u;
        if (scenario == 4) { row.vendor_id = 0x1414; row.device_id = 0x8c; }
        if (scenario == 5) row.dxgi_flags = 2;
        if (scenario == 6) row.is_hardware = 0;
        if (scenario == 7) row.state3_supported = 0;
        if (scenario == 8) row.physical_count = 0;
        if (scenario == 9) row.physical_count = 65;
        if (scenario == 10) { altered.row_count = 2; altered.rows[1] = row; }
        if (scenario == 11) altered.factory_current_end = 0;
        CHECK(wup::NormalizeGpuAdapterInventory(request, altered, &guard.out, sizeof(guard.out)) == WUP_S_OK);
        if (scenario != 11) CHECK(!wup::CanSelectGpuMemoryAdapter(guard.out.rows[0], true));
        if (scenario == 0) CHECK(!guard.out.rows[0].luid_high && !(guard.out.rows[0].known_flags & WupAdapterIdentityKnown));
        if (scenario >= 1 && scenario <= 3) CHECK(guard.out.status == WupPartial);
        if (scenario == 1 || scenario == 2) CHECK(guard.out.rows[0].physical_count == UINT32_MAX);
        if (scenario == 3) CHECK(guard.out.rows[0].is_hardware == UINT32_MAX);
        if (scenario == 4) CHECK(guard.out.rows[0].known_flags & WupAdapterBasicRender);
        if (scenario == 10) CHECK(guard.out.status == WupPartial && (guard.out.flags & WupInventoryDuplicate));
        if (scenario == 11) CHECK(guard.out.flags & WupInventoryStale);
    }
    input.row_count = 0;
    CHECK(normalize() == WUP_S_OK && guard.out.status == WupOk && !guard.out.row_count);
    input.enumeration_hresult = 0x80004005u; input.flags = 0;
    CHECK(normalize() == WUP_S_OK && guard.out.status == WupUnavailable);
    const auto saved = guard.out;
    input.row_count = 65;
    CHECK(normalize() == WUP_E_INVALIDARG && !std::memcmp(&saved, &guard.out, sizeof(saved)));
    input.row_count = 0; request.version = 2;
    CHECK(normalize() == WUP_E_INVALIDARG && !std::memcmp(&saved, &guard.out, sizeof(saved)));
    request.version = 1; request.reserved = 1;
    CHECK(normalize() == WUP_E_INVALIDARG);
    request.reserved = 0;
    CHECK(wup::NormalizeGpuAdapterInventory(request, input, &guard.out, 5199) == WUP_E_INVALIDARG);
    CHECK(!std::memcmp(&saved, &guard.out, sizeof(saved)));
    request.scope = 2;
    CHECK(normalize() == WUP_E_INVALIDARG);
    request.scope = 1;
    input.flags = 32;
    CHECK(normalize() == WUP_E_INVALIDARG);
    input.flags = WupInventoryEnded;
    input.enumeration_hresult = 0x887A0002u;
    input.row_count = 64;
    for (uint32_t i = 0; i < 64; ++i)
        input.rows[i] = {80, 1, 0, 31, 9, i, 0x10de, 1, 0, 0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0};
    CHECK(normalize() == WUP_S_OK && guard.out.status == WupOk && guard.out.row_count == 64);
    input.flags = WupInventoryTruncated; input.enumeration_hresult = 0;
    CHECK(normalize() == WUP_S_OK && guard.out.status == WupPartial);
    input.rows[0].known_flags |= 128;
    CHECK(normalize() == WUP_E_INVALIDARG);
    input.rows[0].known_flags &= 127;
    input.rows[0].is_hardware = 2;
    CHECK(normalize() == WUP_S_OK && guard.out.rows[0].is_hardware == UINT32_MAX);
    input.rows[0].is_hardware = 1;
    input.rows[0].desc_hresult = 0x80004005u;
    CHECK(normalize() == WUP_S_OK && guard.out.rows[0].desc_hresult == 0x80004005u);
    input.row_count = 0; input.flags = WupInventoryEnded; input.enumeration_hresult = 0x887A0002u;
    CHECK(normalize() == WUP_S_OK);
    for (const auto& row : guard.out.rows) CHECK(!std::memcmp(&row, &zero, sizeof(zero)));
    CHECK(guard.a == 11 && guard.b == 22);
#endif
    std::printf("GPU inventory: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}

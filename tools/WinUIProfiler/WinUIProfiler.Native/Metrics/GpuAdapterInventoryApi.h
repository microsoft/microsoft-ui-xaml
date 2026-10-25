#pragma once
#include "GpuAdapterInventory.h"
#include <Windows.h>
#include <dxgi.h>
namespace wup::detail
{
struct GpuAdapterInventoryApi {
    void* context;
    uint32_t (*open)(void*, bool core, void**, uint32_t*);
    bool (*current)(void*, void*);
    uint32_t (*enumerate)(void*, void*, uint32_t, void**);
    uint32_t (*describe)(void*, void*, DXGI_ADAPTER_DESC1*);
    uint32_t (*lookup)(void*, void*, const LUID&, void**);
    uint32_t (*property)(void*, void*, bool hardware, uint32_t*);
    bool (*supported)(void*, void*);
    void (*release)(void*, void*, bool core);
    void (*close)(void*);
    uint64_t (*tick)(void*);
};
uint32_t EnumerateGpuAdaptersWithApi(const WupGpuAdapterInventoryRequest* request,
    WupGpuAdapterInventoryResult* output, uint32_t output_size, const GpuAdapterInventoryApi& api) noexcept;
}

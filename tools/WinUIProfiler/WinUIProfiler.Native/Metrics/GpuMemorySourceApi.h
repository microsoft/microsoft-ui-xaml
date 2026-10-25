#pragma once
#include "GpuMemoryCore.h"
namespace wup::detail
{
struct GpuMemorySourceApi {
    void* context;
    uint32_t (*open)(void*, uint32_t luid_high, uint32_t luid_low, uint32_t* win32_error);
    uint32_t (*physical_count)(void*, uint32_t*);
    bool (*supported)(void*);
    GpuMemoryFamily (*query)(void*, uint32_t physical, uint32_t pid, bool shared);
    void (*close)(void*);
    uint32_t (*identity)(void*, void*, WupProcessIdentity, uint32_t*);
    uint64_t (*tick)(void*);
};
uint32_t CaptureGpuMemoryWithApi(void* handle, const WupGpuMemoryRequest* request,
    WupGpuMemoryResult* output, uint32_t output_size, const GpuMemorySourceApi& api) noexcept;
}

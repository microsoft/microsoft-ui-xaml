#pragma once
#include "../Exports.h"
namespace wup
{
struct GpuMemoryFamily {
    uint32_t hresult, process_query_succeeded;
    uint64_t committed, resident;
};
uint32_t NormalizeGpuMemory(const WupGpuMemoryRequest& request,
    const GpuMemoryFamily& dedicated, const GpuMemoryFamily& shared,
    WupMetricStatus lifecycle, uint32_t win32_error, uint64_t begin_100ns,
    uint64_t end_100ns, WupGpuMemoryResult* output, uint32_t output_size) noexcept;
}

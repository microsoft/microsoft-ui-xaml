#pragma once
#include "../Exports.h"
namespace wup
{
uint32_t NormalizeGpuAdapterInventory(const WupGpuAdapterInventoryRequest& request,
    const WupGpuAdapterInventoryResult& input, WupGpuAdapterInventoryResult* output,
    uint32_t output_size) noexcept;
bool CanSelectGpuMemoryAdapter(const WupGpuAdapterInventoryRow& row, bool hardware_only) noexcept;
}

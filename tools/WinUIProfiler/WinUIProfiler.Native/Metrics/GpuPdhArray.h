#pragma once
#include "GpuFoundation.h"
#include <Windows.h>
#include <pdh.h>
namespace wup
{
PDH_STATUS ReadGpuPdhArray(PDH_HCOUNTER counter, uint32_t pid, WupGpuSnapshot& result,
    uint32_t& observed, uint32_t& malformed);
}

#pragma once
#include "GpuPdhArray.h"
namespace wup::detail
{
struct GpuSourceApi
{
    decltype(&PdhOpenQueryW) open;
    decltype(&PdhAddEnglishCounterW) add;
    decltype(&PdhGetCounterInfoW) info;
    decltype(&PdhCollectQueryDataWithTime) collect;
    decltype(&PdhGetRawCounterArrayW) array;
    decltype(&PdhCloseQuery) close;
    decltype(&GetProcessTimes) times;
    decltype(&GetProcessId) pid;
    decltype(&WaitForSingleObject) wait;
    decltype(&GetTickCount64) tick;
    decltype(&GetLastError) last_error;
};
uint32_t CaptureGpuWithApi(void* process, const WupGpuRequest* request,
    WupGpuSnapshot* output, uint32_t output_size, const GpuSourceApi& api) noexcept;
PDH_STATUS ReadGpuPdhArrayWithApi(PDH_HCOUNTER counter, uint32_t pid, WupGpuSnapshot& result,
    uint32_t& observed, uint32_t& malformed, decltype(&PdhGetRawCounterArrayW) read, bool require_engine = true);
}

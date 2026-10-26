#pragma once
#include "ProcessMemorySampler.h"
#include <Windows.h>
#include <psapi.h>

namespace wup::detail
{
using MemoryQuery = BOOL (WINAPI*)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
struct ProcessMemoryApi
{
    MemoryQuery query;
    decltype(&GetProcessTimes) times;
    decltype(&GetProcessId) process_id;
    decltype(&WaitForSingleObject) wait;
    decltype(&GetTickCount64) tick;
    decltype(&GetLastError) last_error;
    WupMetricSource source = WupProcessMemoryEx2;
};
uint32_t CaptureMemoryWithApi(void* process, const WupMemoryRequest* request,
    WupMemoryResult* output, uint32_t output_size, const ProcessMemoryApi& api) noexcept;
}

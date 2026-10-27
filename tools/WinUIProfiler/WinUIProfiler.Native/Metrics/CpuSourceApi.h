#pragma once
#include "CpuSource.h"
#include "ProcessSnapshot.h"

namespace wup::detail
{
struct ProcessorTimes
{
    int64_t idle;
    int64_t kernel;
    int64_t user;
    int64_t dpc;
    int64_t isr;
    uint32_t interrupt_count;
    uint32_t spare;
};
using QueryEx = LONG(NTAPI*)(ULONG, void*, ULONG, void*, ULONG, ULONG*);
using Inventory = BOOL(WINAPI*)(DWORD*, DWORD, DWORD*);
struct CpuSourceApi
{
    QueryEx query;
    Inventory inventory;
    decltype(&GetProcessTimes) times;
    decltype(&GetActiveProcessorGroupCount) group_count;
    decltype(&GetActiveProcessorCount) cpu_count;
    decltype(&GetProcessId) process_id;
    decltype(&WaitForSingleObject) wait;
    decltype(&GetTickCount64) tick;
    decltype(&GetLastError) last_error;
    ProcessQuery process_query = nullptr;
};
// Internal link-time seam only. The production resolver invokes this same boundary.
CpuCapture CaptureCpuWithApi(HANDLE process, WupProcessIdentity expected, const CpuSourceApi& api);
}

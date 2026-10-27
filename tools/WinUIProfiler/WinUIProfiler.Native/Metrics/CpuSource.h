#pragma once
#include "..\Exports.h"
#include <Windows.h>

namespace wup
{
constexpr uint32_t MaxGroups = WUP_MAX_CPU_GROUPS;
struct CpuCapture
{
    WupCpuSnapshot snapshot;
    WupCpuGroup groups[MaxGroups];
    uint32_t os_error;
    uint32_t inventory_count;
    uint64_t acquisition_begin_100ns;
    uint32_t error_domain; // 0 none; 1 Win32; 2 NTSTATUS.
};
// Caller retains a QUERY_LIMITED_INFORMATION | SYNCHRONIZE handle.
// expected must come from that handle. All inventory data stays ephemeral.
CpuCapture CaptureCpu(HANDLE process, WupProcessIdentity expected);
bool ValidProcessorBuffer(uint32_t returned_bytes, uint32_t processor_count);
}

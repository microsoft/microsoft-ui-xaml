#pragma once
#include "..\Exports.h"

namespace wup
{
constexpr uint32_t MaxGpuRows = 256;
struct GpuInstance
{
    uint32_t pid;
    uint32_t luid_high;
    uint32_t luid_low;
    uint32_t physical;
    uint32_t engine;
    uint32_t duplicate_index;
    bool has_engine;
};
struct GpuContext
{
    GpuInstance instance;
    uint64_t context_id; // Independently proven context identity, never a PDH #suffix.
    WupMetricStatus status;
    double percent;
    uint64_t begin_100ns;
    uint64_t end_100ns;
    uint64_t clock_id = 2;
};
struct GpuSummary
{
    WupMetricStatus status;
    double percent;
    uint32_t engine_count;
    WupByteMetric dedicated_committed;
    WupByteMetric shared_committed;
    WupByteMetric dedicated_resident;
    WupByteMetric shared_resident;
    uint64_t clock_id;
    uint64_t begin_100ns;
    uint64_t end_100ns;
};
bool ParseGpuInstance(const wchar_t* text, uint32_t length, GpuInstance& output);
GpuSummary AggregateGpu(const GpuContext* rows, uint32_t count, uint32_t selected_pid);
void ProbeGpu(void* process, WupProcessIdentity expected);
}

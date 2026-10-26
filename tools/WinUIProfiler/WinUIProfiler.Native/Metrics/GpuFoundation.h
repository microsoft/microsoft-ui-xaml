#pragma once
#include "..\Exports.h"

namespace wup
{
constexpr uint32_t MaxGpuRows = 256;
struct GpuInstance {
    uint32_t pid, luid_high, luid_low, physical, engine, duplicate_index;
    bool has_engine;
};
struct GpuContext {
    GpuInstance instance;
    uint64_t context_id; // Independently proven context identity, never a PDH #suffix.
    WupMetricStatus status;
    double percent;
    uint64_t begin_100ns, end_100ns;
    uint64_t clock_id = 2;
};
struct GpuSummary {
    WupMetricStatus status;
    double percent;
    uint32_t engine_count;
    WupByteMetric dedicated_committed, shared_committed, dedicated_resident, shared_resident;
    uint64_t clock_id, begin_100ns, end_100ns;
};
bool ParseGpuInstance(const wchar_t* text, uint32_t length, GpuInstance& output);
GpuSummary AggregateGpu(const GpuContext* rows, uint32_t count, uint32_t selected_pid);
void ProbeGpu(void* process, WupProcessIdentity expected);
}

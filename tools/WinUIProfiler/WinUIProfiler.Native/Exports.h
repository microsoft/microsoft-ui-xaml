#pragma once
#include <stdint.h>

#define WUP_METRICS_VERSION 2u
#define WUP_S_OK 0u
#define WUP_E_INVALIDARG 0x80070057u
#define WUP_MAX_CPU_GROUPS 64u

#pragma pack(push, 8)

#ifdef __cplusplus
#define WUP_ABI_NOEXCEPT noexcept
extern "C" {
#else
#define WUP_ABI_NOEXCEPT
#endif

typedef uint32_t WupMetricStatus;
enum {
    WupOk = 0, WupWarmup = 1, WupUnavailable = 2, WupPartial = 3,
    WupCounterReset = 4, WupTargetMissing = 5, WupIdentityChanged = 6
};
typedef uint32_t WupMetricSource;
enum {
    WupSyntheticCpuSnapshot = 1, WupNtGroupProcessTimesCompatibility = 2,
    WupSyntheticMemory = 3, WupProcessMemoryEx2 = 4, WupProcessMemoryEx = 5,
    WupNtProcessSnapshotCpu = 9, WupNtProcessSnapshotMemory = 10
};

typedef struct WupProcessIdentity {
    uint32_t pid;
    uint32_t reserved;
    uint64_t creation_filetime_100ns;
} WupProcessIdentity;

/* Cumulative system 100 ns units. Kernel includes idle, DPC and ISR. */
typedef struct WupCpuGroup {
    uint32_t size;
    uint32_t version;
    uint32_t group_id;
    uint32_t logical_processor_count;
    uint64_t kernel;
    uint64_t user;
    uint64_t idle;
    uint64_t dpc;
    uint64_t isr;
} WupCpuGroup;

typedef struct WupCpuSnapshot {
    uint32_t size;
    uint32_t version;
    WupMetricSource source;
    WupMetricStatus status;
    uint64_t clock_id;
    uint64_t timestamp_100ns;
    WupProcessIdentity process;
    uint32_t group_count;
    uint32_t expected_group_count;
    uint32_t target_present;
    uint32_t reserved;
    uint64_t process_kernel;
    uint64_t process_user;
} WupCpuSnapshot;

typedef struct WupCpuMetric {
    WupMetricStatus status;
    uint32_t reserved;
    double percent;
} WupCpuMetric;

typedef struct WupCpuResult {
    uint32_t size;
    uint32_t version;
    WupMetricSource source;
    WupMetricStatus status;
    WupProcessIdentity process;
    uint64_t clock_id;
    uint64_t begin_100ns;
    uint64_t end_100ns;
    WupCpuMetric system_utilization;
    WupCpuMetric process_utilization;
    WupCpuMetric interrupt_utilization;
    WupCpuMetric other_or_unattributed_utilization;
    WupCpuMetric process_utility;
} WupCpuResult;

/* Caller owns all buffers. Sizes/counts describe accessible storage.
   Null previous means warmup. No retained state, allocation or OS calls.
   Nonzero HRESULT leaves output untouched, including on version/size mismatch. */
uint32_t WupNormalizeCpu(
    const WupCpuSnapshot* previous, const WupCpuGroup* previous_groups,
    const WupCpuSnapshot* current, const WupCpuGroup* current_groups,
    WupCpuResult* output, uint32_t output_size) WUP_ABI_NOEXCEPT;

#define WUP_MEMORY_VERSION 1u
typedef struct WupMemoryRequest {
    uint32_t size;
    uint32_t version;
    WupProcessIdentity process;
} WupMemoryRequest;

typedef struct WupByteMetric {
    WupMetricStatus status;
    uint32_t units; /* WupBytes. Missing value is UINT64_MAX, status is authoritative. */
    uint64_t value;
} WupByteMetric;
enum { WupBytes = 1 };

typedef struct WupMemoryResult {
    uint32_t size;
    uint32_t version;
    WupMetricSource source;
    WupMetricStatus status;
    WupProcessIdentity process;
    uint64_t clock_id;
    uint64_t acquisition_begin_100ns;
    uint64_t acquisition_end_100ns;
    uint32_t win32_error;
    uint32_t reserved;
    WupByteMetric private_working_set;
    WupByteMetric total_working_set;
    WupByteMetric private_commit;
} WupMemoryResult;

/* Borrowed Windows HANDLE with QUERY_LIMITED_INFORMATION | SYNCHRONIZE access.
   Invalid ABI arguments leave output untouched; OS failures produce missing metrics. */
uint32_t WupCaptureProcessMemory(void* process_handle, const WupMemoryRequest* request,
    WupMemoryResult* output, uint32_t output_size) WUP_ABI_NOEXCEPT;

#define WUP_CPU_CAPTURE_VERSION 1u
typedef struct WupCpuCaptureRequest {
    uint32_t size;
    uint32_t version;
    WupProcessIdentity process;
} WupCpuCaptureRequest;

typedef struct WupCpuCaptureResult {
    uint32_t size;
    uint32_t version;
    uint32_t error_code;
    uint32_t error_domain; /* 0 none; 1 Win32; 2 NTSTATUS. */
    uint64_t acquisition_begin_100ns;
    uint64_t acquisition_end_100ns;
    WupCpuSnapshot snapshot;
    WupCpuGroup groups[WUP_MAX_CPU_GROUPS];
} WupCpuCaptureResult;

/* Stateless capture. Normalize with a null previous snapshot for initial warmup.
   Only groups below snapshot.group_count exist; status governs all counters. */
uint32_t WupCaptureCpu(void* process_handle, const WupCpuCaptureRequest* request,
    WupCpuCaptureResult* output, uint32_t output_size) WUP_ABI_NOEXCEPT;

#define WUP_GPU_VERSION 1u
#define WUP_MAX_GPU_ROWS 256u
enum { WupEnglishPdhGpuEngine = 6 };
typedef struct WupGpuRequest {
    uint32_t size;
    uint32_t version;
    WupProcessIdentity process;
} WupGpuRequest;
typedef struct WupGpuRawEngine {
    uint32_t luid_high;
    uint32_t luid_low;
    uint32_t physical;
    uint32_t engine;
    uint32_t duplicate_index;
    uint32_t status;
    uint32_t pdh_status;
    uint32_t counter_type;
    uint64_t running_time;
    uint64_t timer;
    uint64_t timestamp_filetime_100ns;
} WupGpuRawEngine;
typedef struct WupGpuSnapshot {
    uint32_t size;
    uint32_t version;
    uint32_t source;
    uint32_t status;
    WupProcessIdentity process;
    uint64_t clock_id;
    uint64_t sample_filetime_100ns;
    uint64_t acquisition_begin_100ns;
    uint64_t acquisition_end_100ns;
    uint64_t acquisition_clock_id;
    uint32_t pdh_error;
    uint32_t row_count;
    uint32_t win32_error;
    uint32_t observed_instance_count;
    WupGpuRawEngine rows[WUP_MAX_GPU_ROWS];
} WupGpuSnapshot;
typedef struct WupGpuEngineUsage {
    uint32_t luid_high;
    uint32_t luid_low;
    uint32_t physical;
    uint32_t engine;
    uint32_t status;
    uint32_t reserved;
    double percent;
} WupGpuEngineUsage;
typedef struct WupGpuResult {
    uint32_t size;
    uint32_t version;
    uint32_t source;
    uint32_t status;
    WupProcessIdentity process;
    uint64_t clock_id;
    uint64_t begin_100ns;
    uint64_t end_100ns;
    WupCpuMetric utilization;
    uint32_t engine_count;
    uint32_t reserved;
    WupGpuEngineUsage engines[WUP_MAX_GPU_ROWS];
    WupByteMetric dedicated_committed;
    WupByteMetric shared_committed;
    WupByteMetric dedicated_resident;
    WupByteMetric shared_resident;
} WupGpuResult;
uint32_t WupCaptureGpu(void* process_handle, const WupGpuRequest* request,
    WupGpuSnapshot* output, uint32_t output_size) WUP_ABI_NOEXCEPT;
uint32_t WupNormalizeGpu(const WupGpuSnapshot* previous, const WupGpuSnapshot* current,
    WupGpuResult* output, uint32_t output_size) WUP_ABI_NOEXCEPT;

#ifdef __cplusplus
}

static_assert(sizeof(WupProcessIdentity) == 16);
static_assert(sizeof(WupCpuGroup) == 56);
static_assert(sizeof(WupCpuSnapshot) == 80);
static_assert(sizeof(WupCpuMetric) == 16);
static_assert(sizeof(WupCpuResult) == 136);
static_assert(sizeof(WupMemoryRequest) == 24);
static_assert(sizeof(WupByteMetric) == 16);
static_assert(sizeof(WupMemoryResult) == 112);
static_assert(sizeof(WupCpuCaptureRequest) == 24);
static_assert(sizeof(WupCpuCaptureResult) == 3696);
static_assert(sizeof(WupGpuRequest) == 24);
static_assert(sizeof(WupGpuRawEngine) == 56);
static_assert(sizeof(WupGpuSnapshot) == 14424);
static_assert(sizeof(WupGpuEngineUsage) == 32);
static_assert(sizeof(WupGpuResult) == 8336);
#endif

#pragma pack(pop)

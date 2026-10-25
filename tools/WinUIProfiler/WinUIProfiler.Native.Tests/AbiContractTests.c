#include "../WinUIProfiler.Native/Exports.h"
#include <stddef.h>

typedef char identity_size_is_16[(sizeof(WupProcessIdentity) == 16) ? 1 : -1];
typedef char snapshot_size_is_80[(sizeof(WupCpuSnapshot) == 80) ? 1 : -1];
typedef char group_size_is_56[(sizeof(WupCpuGroup) == 56) ? 1 : -1];
typedef char result_size_is_136[(sizeof(WupCpuResult) == 136) ? 1 : -1];
typedef char creation_offset_is_8[(offsetof(WupProcessIdentity, creation_filetime_100ns) == 8) ? 1 : -1];
typedef char metrics_offset_is_56[(offsetof(WupCpuResult, system_utilization) == 56) ? 1 : -1];
typedef char memory_request_size_is_24[(sizeof(WupMemoryRequest) == 24) ? 1 : -1];
typedef char memory_result_size_is_112[(sizeof(WupMemoryResult) == 112) ? 1 : -1];
typedef char memory_metrics_offset_is_64[(offsetof(WupMemoryResult, private_working_set) == 64) ? 1 : -1];
typedef char cpu_capture_request_size_is_24[(sizeof(WupCpuCaptureRequest) == 24) ? 1 : -1];
typedef char cpu_capture_result_size_is_3696[(sizeof(WupCpuCaptureResult) == 3696) ? 1 : -1];
typedef char cpu_capture_snapshot_offset_is_32[(offsetof(WupCpuCaptureResult, snapshot) == 32) ? 1 : -1];
typedef char cpu_capture_groups_offset_is_112[(offsetof(WupCpuCaptureResult, groups) == 112) ? 1 : -1];
typedef char gpu_request_size_is_24[(sizeof(WupGpuRequest) == 24) ? 1 : -1];
typedef char gpu_raw_row_size_is_56[(sizeof(WupGpuRawEngine) == 56) ? 1 : -1];
typedef char gpu_snapshot_size_is_14424[(sizeof(WupGpuSnapshot) == 14424) ? 1 : -1];
typedef char gpu_rows_offset_is_88[(offsetof(WupGpuSnapshot, rows) == 88) ? 1 : -1];
typedef char gpu_result_size_is_8336[(sizeof(WupGpuResult) == 8336) ? 1 : -1];

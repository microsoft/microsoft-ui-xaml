# Native metrics and collector integration: P07 in progress

The current seven-export native DLL implements CPU normalization/capture,
process memory capture, GPU engine normalization/capture and optional
per-physical-adapter GPU memory capture and graphics-visible inventory. Strict managed
CPU/memory/GPU adapters and independent polling retain original counter clocks,
statuses and errors. Typed collector Process/Metric/Loss emission and decoding,
actual ETL replay, simultaneous live-session isolation and native sampling
through ETW, the bounded owner, SQLite and read-only reopen are tested.
See [architecture](architecture.md) and
[collector wire contracts](event-contract.md#collector-metric-wire-schema-1).

## Connected process graphs

`TargetConnection` starts native acquisition only after genuine TAP hello.
The five process graphs share its collector ETW listener, bounded processing
owner and SQLite writer; no parallel UI-only sample store is introduced.
Runtime XAML observations remain source-only, while a process-only
`MetricProcessor` projects CPU, overall GPU, total/private working set and
private commit. Per-engine GPU rows cannot enter the overall graph.
Configured GPU-memory adapter scopes are explicitly empty for this connection.

| Graph | Acquisition and definition |
| --- | --- |
| CPU (%) | Selected process kernel/user times from `NtQuerySystemInformation(SystemProcessInformation)`, normalized with processor-group system snapshots from `NtQuerySystemInformationEx`. Native source 9. |
| GPU (%) | English PDH GPU Engine running-time deltas; maximum engine utilization, not sum/mean. |
| Working set (MiB) | Process-snapshot `WorkingSetSize`; total resident process working set. Native source 10. |
| Private working set (MiB) | Process-snapshot private working-set bytes; distinct from total working set and commit. Native source 10. |
| Private commit (MiB) | Process-snapshot `PrivatePageCount` bytes; private committed memory, not resident bytes. Native source 10. |

The process-snapshot compatibility boundary is independently implemented,
dynamically resolved, bounded to 16 MiB/four acquisition attempts and validated
for x64/ARM64 layouts, record offsets, selected PID/creation identity and
counter ranges. It retains only selected-process fields. These OS layouts
are not a universally stable public structure contract. A failed source-10
query stays unavailable; it does not silently substitute a K32 measurement.
Existing CPU source 2 and memory sources 4/5 recordings/fault seams remain
supported and distinctly labelled. Export ordinals and ABI sizes are unchanged.

CPU measures processor-time utilization, not frequency-adjusted CPU utility.
Using the same OS API family does not establish exact Task Manager algorithm
or displayed-value parity. Transient-process attribution, aligned sampling
comparisons, real-GPU hardware parity and native ARM64 execution remain open.
GPU availability is driver/counter dependent; measured zero and missing data
are different results.

Acquisition runs independently with a one-second delay between captures.
Each sample retains its actual acquisition interval. The UI asynchronously
queries a pinned committed watermark, with a complete bounded 120-second
window and at most 121 points per graph. Bytes persist unchanged; MiB is a
display conversion. Missing samples and pauses longer than three seconds
split plotted lines. Stop joins acquisition before ETW drain and retains the
final committed window; reconnect resets the graph session. The native DLL
is built, staged beside the app and SHA256-checked in its deployment manifest.
This focused connection persists SQLite observations, not a new raw ETL file.

Release/x64 qualification: full Engine **882 passed**, actual connection
controller **9 passed**, mixed app/native build and seven-export DLL smoke
passed. Real GraphicsEtl qualification verified all five rendered/progressing
charts, independent K32 EX2 memory readings (asynchronous tolerance:
max 1 MiB/3%), final stopped retention, surviving target, genuine TAP heartbeat,
continued XAML events and snapshot viewers. ARM64 native suites cross-compile;
this is not execution or hardware parity. Earlier standalone collector
checkpoint evidence follows.

## Standalone collector qualification checkpoint

Earlier validation: full Debug and Release Engine suites **484 passed**,
IntegrationTests **38 passed**, including owned live-recording lifecycle,
GPU-memory optional-export/identity/stop/reopen and inventory/default-selection
coverage. The Release/x64 mixed solution build and its actual rebuilt
seven-export DLL smoke passed. Native source
fault coverage includes CPU30, memory38 and GPU53 cases; all seven native
suites passed independently. These results do not establish Task Manager
parity or application overhead. `LiveMetricRecording` now coordinates native
polling, collector lifecycle/loss publication, ETW drain and durable health
under a single acquisition lease. Its readiness is first native publication,
not delivery/commit acknowledgment. Stop joins native capture before ETW drain.
Authoritative native target exit is distinct from identity mismatch or generic
failure; accepted final missing samples survive reopen. Reset/removal,
complete inventory persistence and aligned hardware qualification
remain unfinished. ARM64 compilation is not execution; the available GPU
adapters identify as Basic Render Driver and do not satisfy workload gates.

The public DXCore route for per-process dedicated/shared committed/resident
bytes is identified: `AdapterMemoryUsageByProcessBytes` takes a physical-adapter
index, `DXCoreMemoryType` and PID; `DXCoreMemoryUsage` returns distinct
`committed` and `resident` byte counts. Runtime support, successful HRESULT
and `processQuerySucceeded` are all required. This route is wired into an optional
native capture ABI, strict managed decoder and configured live metric publication.
Do not substitute
budget/current-usage/reservation, engine rows or adapter-wide segment totals;
do not infer complete physical inventory or cross-adapter unique byte totals.
PID/creation/liveness checks must bracket the query, and cross-process access
failure remains explicit. SDK availability does not establish OS/driver or
hardware qualification.

## Independent per-adapter GPU memory capture

`WupCaptureGpuMemory` is additive ordinal 6, version 1/source 7. The existing
five exports keep their ordinals/layouts. Its request is 40 bytes and its result
152 bytes; fixed-width C layout is verified for x64 and ARM64. ARM64 compilation
does not establish execution. Missing optional exports are detected explicitly;
requesting configured memory scopes without this export fails visibly.

The request carries PID, actual creation FILETIME, both LUID words and physical
index. The borrowed retained handle must match PID/creation and be live before
and after collection. System32 DXCore loading, exact-LUID lookup, supported
positive physical count/index and state support precede independent dedicated
and shared queries. Failed or unsupported queries never become measured zero.
Each successful family produces its own committed and resident byte values.
Result status is measured, unavailable, partial, target-missing or
identity-changed; raw family HRESULTs, process-query flags and separate lifecycle
Win32 errors are preserved. A successful HRESULT with a false process-query flag
still means unavailable. Zero bytes with a valid family is a measurement.

`NativeGpuMemorySampler` uses the same constrained local DLL loader and
serialized capture/dispose lifetime as other native adapters. The decoder checks
every reserved field, source/clock, requested identity/scope, family status,
units, sentinel and HRESULT/success-flag relationship. An optional immutable
`gpuMemoryAdapters` selection on `LiveMetricRecording` configures at most 256
distinct physical scopes; null discovers graphics-visible scopes once, while
an explicit empty list disables adapter memory queries. Polling owns these modules/queries under the same
acquisition lease, and terminal memory observations participate in target-exit
and identity-change handling. No independent unmanaged polling lifetime is added.

Four typed `PerAdapterMemory` observations per selected scope retain original
boot-clock acquisition intervals. Their session interval is the actual enclosing
batch QPC acquisition bracket, independently converted to session ticks; native
boot timestamps are not converted to QPC. The collector still uses its same
forty-field schema. Error domain 3 means raw HRESULT bits; existing None/Win32/
NTSTATUS numeric identities remain unchanged. At most 1,293 observations fit
one bounded batch: thirteen process observations, 256 engine rows and four
observations for each of 256 configured physical scopes. Validation completes
before sequence reservation or publication.

The four existing process-wide GPU-memory observations remain unavailable:
per-adapter byte counts do not prove unique process-wide totals and are not
summed. Inventory/selection is not inferred from active PDH rows or a
D3D12-only adapter list. Actual tests verify discovered exact-scope queries,
unavailable explicit scopes and lifecycle/transport integrity, not hardware
parity. Native GPU-memory tests also cover 66 core and 105 source-fault checks.

## Graphics-visible inventory and automatic selection

Additive optional ordinal 7, `WupEnumerateGpuAdapters`, has version 1/source 8,
scope 1 (`DXGI-visible graphics`). Its request is 16 bytes and result 5,200:
an 80-byte header and 64 fixed 80-byte rows. The first six exports/layouts
remain unchanged. Both x64 and ARM64 DLLs compile; ARM64 execution remains open.

A securely loaded System32 DXGI factory enumerates adapters with or without
outputs, without a D3D12 feature filter or active-engine requirement. At most
65 enumeration calls distinguish a genuine 64-row end from truncation. Only
`DXGI_ERROR_NOT_FOUND` ends enumeration normally. Every descriptor row stays
present; failed descriptors do not manufacture a LUID. Exact-LUID DXCore
enrichment independently records physical count, hardware classification and
memory-state support. Unknown count/hardware/support remains explicitly unknown,
not zero/false. Raw operation HRESULTs and Win32 errors are retained.

Factory `IsCurrent` is checked before and after acquisition; stale/truncated
inventories cannot silently configure a prefix. Duplicate LUIDs are partial and
unselectable, not merged. The decoder checks inactive bytes, reserved fields,
typed arms, derived Basic/duplicate flags and complete-status consistency.
Selection requires supported memory state and a known positive physical count
at most 64 per logical row, with a total budget of 256 physical scopes.
Exceeding that budget fails explicitly and requests explicit configuration.

Usable rows may be selected independently when other rows have unavailable
enrichment. Default polling records inventory status, flags, raw header errors,
Basic Render row count, selected physical count and scope limitations in its
durable collector Attach quality reason. Tests assert these details after
SQLite reopen; removal of that provenance reproduces RED. Per-adapter byte
gauges do not imply complete device inventory or unique process totals.
An absent optional export or unusable inventory leaves GPU-memory coverage
explicitly missing without stopping independent CPU/process-memory collection.

This is not an OS-wide physical-device inventory: compute/NPU-only, disabled,
unavailable and other-session devices are outside its guarantee. Software/Basic
rows remain labelled; hardware-only selection rejects Basic, software and
unknown classification. The current host exposes three Basic Render identities,
one usable software scope and two failed exact-LUID lookups, so inventory is
partial. Native inventory tests cover 116 pure and 83 source checks across
22 fault scenarios, including a RED/GREEN fix for S_OK without written hardware
output incorrectly becoming known false. Loader internals and exhaustive COM
reference accounting are not completely fault-qualified.

The following sections retain the source/ABI evolution and checkpoint evidence.
Sources distinguish synthetic input from the explicitly labelled
`WupNtGroupProcessTimesCompatibility` OS alternative. Native boot/FILETIME
intervals are never relabelled as QPC; the separately recorded acquisition
bracket is mapped into session ticks.

## CPU normalization ABI v2

`WinUIProfiler.Native\Exports.h` is a C-compatible, fixed-width data contract.
Struct sizes and versions must exactly match v2; output capacity may be larger.
P07b removes fictional per-group process attribution: group size is now 56 bytes,
snapshot size 80 bytes, with aggregate process kernel/user counters. Prior v1
inputs are rejected rather than reinterpreted.
Callers own accessible buffers and must supply arrays covering the stated counts.
The function retains no pointers, allocates no memory, throws no exceptions and
performs no OS calls. `WUP_S_OK` means a result (including missing measurement)
was produced, not that all measurements are available. `WUP_E_INVALIDARG`
leaves the entire caller output unchanged. Output fields carry size, version,
source, process instance, source-clock identity and actual interval boundaries.

Process identity is PID **and actual creation FILETIME**, in 100 ns units.
Snapshot timestamps use an explicitly identified monotonic 100 ns clock.
Inputs with different clocks cannot form an interval. The eventual sampler
must map its source clock explicitly; this core does not infer QPC frequency,
FILETIME-to-QPC mapping, session identity or ETW provenance.

`status` governs every value; missing values are NaN, never observed zero.
Numeric zero is meaningful only with `WupOk`. Utility remains unavailable even
when utilization is valid. Invalid sizes, versions, source, duplicate group IDs,
non-increasing timestamps and arithmetic overflow are argument errors.
The protocol deliberately supports at most 64 groups (a conservative implementation
bound, not a claim about the Windows group-count maximum), each with at most 64
processors. Counts are rejected before array traversal. Incomplete group coverage or changed topology is partial, not a denominator
formed from whichever processors happened to be collected.

## CPU input and normalization

Both snapshots supply every processor group, with stable group ID and active
logical processor count and complete cumulative system counters. The snapshot
contains selected-process cumulative execution **aggregated across all groups**.
Array order is irrelevant.
All execution counters are in 100 ns units. The future compatibility boundary
must prove that its API source provides these semantics. No process-per-group
attribution is required or manufactured.

For every group, let deltas be `K` (kernel **including idle, DPC and ISR**),
`U` (user), `I` (idle), `D` (DPC), `R` (ISR). Let `P` be aggregate selected-process
kernel plus user delta, excluding interrupt execution. Across all groups:

* Capacity denominator: `T = sum(K + U)`.
* System utilization: `100 * sum(K + U - I) / T`.
* Selected-process utilization: `100 * P / T`.
* Interrupt utilization: `100 * sum(D + R) / T`.
* Other-or-unattributed utilization: system busy minus selected-process execution,
  divided by `T`. This includes interrupts, other processes, processes that exited
  between observations and asynchronous/unattributed system work. It is **not**
  claimed to isolate only unattributed work.

DPC/ISR are already included in kernel/system busy and are not added again.
No residual work is redistributed to the selected process. If asynchronous
accounting makes selected execution exceed non-interrupt system busy, system and
interrupt measurements remain valid but process and residual become partial/NaN;
the core neither clamps nor rescales them.

The first observation is warmup, never historical process CPU charged to the
first interval. A different PID or creation time produces identity-changed with
no numeric measurement; the caller must use the new instance as its next baseline.
Missing target, incomplete input, zero denominator and counter decreases produce
explicit statuses. After a reset, the caller can retain the current valid snapshot
as a fresh baseline. A failed argument validation must not become a baseline.

Utilization measures busy execution as a fraction of aggregate processor time.
Frequency-adjusted **utility** requires additional independently validated
performance/capacity information, may have different ranges, and cannot be
derived from these inputs. `GetProcessTimes / wall time` is not implemented and
must not be relabelled Task Manager parity.

All inputs are ephemeral caller-owned normalization data. Only the selected
process identity and necessary system aggregates appear in the result; no other
process names, command lines or histories are collected or persisted here.

## Actual CPU compatibility source

`Metrics\CpuSource.cpp` dynamically resolves `NtQuerySystemInformationEx` and
queries class 8 separately for every active group using a USHORT input selector.
It uses a narrowly isolated 48-byte processor record. Every query must return
exactly `active_processor_count * 48` bytes; negative counters, sum overflow,
missing exports, unsupported status, excess bounds or topology changes prevent
an available sample. The raw per-group buffer is bounded to 64 records.

This is a compatibility API, not a supported-version guarantee. Microsoft Learn
documents the processor layout and units but reserves DPC/ISR fields; the public
NtDoc compatibility reference supplies their interpretation and the Ex group
selector. This provenance is explicit; no private OS implementation was used.

The public `K32EnumProcesses` inventory uses a fixed 16,384 PID buffer, validates
byte divisibility and rejects a full buffer as potentially truncated. It retains
only inventory count and selected PID presence, never other-process histories.
`GetProcessTimes` supplies actual creation FILETIME and aggregate process execution.
`GetProcessId` and creation time must match the caller's expected identity on the
same retained `QUERY_LIMITED_INFORMATION | SYNCHRONIZE` handle. Wait checks before
and after collection distinguish exit from a live sample. Errors preserve a
Win32 versus NTSTATUS domain.

`GetTickCount64` supplies machine-boot-origin timestamps converted to 100 ns;
resolution is milliseconds, not 100 ns precision. Capture records acquisition
begin and end; process and group reads are sequential, not atomic or synchronized
with Task Manager. Baseline normalization still produces warmup. Utility remains
unavailable. No wall-time-only denominator or fallback to group zero is used.

Public references:

* https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysysteminformation
* https://ntdoc.m417z.com/ntquerysysteminformationex
* https://ntdoc.m417z.com/system_processor_performance_information
* https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes
* https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumprocesses
* https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getsystemtimes

`GetSystemTimes` alone is deliberately not used: its documented primary-group
restriction above 64 processors cannot prove complete group coverage.

## Process memory: documented resident/commit fields

Memory has an independent `WUP_MEMORY_VERSION = 1` contract; the CPU ABI remains v2.
`WupMemoryRequest` is 24 bytes; `WupMemoryResult` is 112 bytes. Invalid request
size/version/identity, null ABI pointers or insufficient output capacity return
invalid argument without touching caller output. A valid request produces a
result even for OS errors, with status, Win32 error and missing measurements.
The Windows handle is borrowed, never closed by the sampler; it needs
`PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE`. PID and actual creation FILETIME
are verified on that same handle before and after a successful measurement.
Exit is `WupTargetMissing`; identity mismatch is `WupIdentityChanged`; invalid
handle/access failure is unavailable. There is no numeric fallback on failure.

`K32GetProcessMemoryInfo` is dynamically resolved from kernel32, using the public
SDK `PROCESS_MEMORY_COUNTERS_EX2` layout. The documented fields map directly:

| Metric | Exact field | Units |
| --- | --- | --- |
| Private working set | `PrivateWorkingSetSize` | Bytes |
| Total working set | `WorkingSetSize` | Bytes |
| Private commit | `PrivateUsage` | Bytes |

Commit is allocated private committed virtual memory, **not resident memory**.
These three values must not be summed. Private working set is a subset of total
working set; working set is not guaranteed to equal touched allocation size.
Memory measurements are gauges and do not need CPU-style baseline warmup.

Every metric has its own status and `WupBytes` units. Missing integer values use
`UINT64_MAX`, interpreted only with non-OK status. Actually observed zero remains
zero with `WupOk`. Successful EX2 uses `WupProcessMemoryEx2` source (4).

Microsoft documents EX2 support beginning with Windows 10/11 22H2 plus the
September 2023 cumulative update. The sampler probes the API rather than assuming
all Windows 11 versions support it. It checks structure `cb` and initializes the
EX2-only private-working-set field to an unwritten sentinel. For capability
rejections or a successful call that did not populate EX2, a separately labelled
EX query can provide working set and private commit. Its source is
`WupProcessMemoryEx` (5), result is partial, and private working set remains
unavailable/`UINT64_MAX`. Other errors do not silently trigger this fallback.
The actual fallback API branch is not yet fault-injection tested; deterministic
mapping fixtures establish missingness and field distinctions.

Each result records acquisition beginning/end on clock 2, machine-boot
`GetTickCount64` converted to 100 ns units (millisecond resolution, not 100 ns
precision). The bracket includes identity checks and native collection; it is
not an assertion of atomic measurement or alignment with another tool.

Public API/layout provenance:

* https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo
* https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex2

No Task Manager parity claim follows from documented matching metric names.
That still requires actual source-selection and aligned-window evidence.

## GPU capability and pure aggregation foundation (P07d)

Public DXCore **does** expose selected-PID queries:
`AdapterEngineRunningTimeByProcessMicroseconds` takes `DXCoreEngineQueryInput`
with `processId`, and `AdapterMemoryUsageByProcessBytes` takes
`DXCoreProcessMemoryQueryInput`. These are not the calling-process memory budget.
The installed 26100 SDK supplies these types. The probe dynamically loads
system32 DXCore, creates a D3D12-graphics-filtered adapter list, checks state
support and attempts physical-adapter 0 / engine 0 selected-PID queries. This is
bounded capability evidence, not exhaustive adapter/engine enumeration. At most
16 adapters are probed; compute-only adapters are outside this filter.

The probe also independently labels an **English PDH GPU alternative**, registering
`GPU Engine(*)\Utilization Percentage` and `GPU Process Memory(*)\Dedicated Usage`
and `Shared Usage` with `PdhAddEnglishCounterW`. It consumes the actual wildcard
arrays with `PdhGetFormattedCounterArrayW`, not localized object enumeration or a
guessed full instance. Arrays are bounded to 1 MiB / 4,096 items, names to 255
characters, and retries to three. Every array length, item count, name pointer,
alignment and terminator is checked before parsing. Unrelated process names or
values are not retained or printed; only aggregate instance counts and selected
identity details are reported.

The parser accepts bounded `pid_<decimal>_luid_0x<8hex>_0x<8hex>_phys_<decimal>`
instances, optionally followed by `_eng_<decimal>_engtype_<ASCII label>` and an
optional `#<positive decimal>` duplicate-instance index. It validates all
uint32 overflows, exact hex widths, separators, label bounds and complete input.
Unknown instance formats are malformed/unsupported, never silently mapped to
GPU 0. Failed parsing leaves output unchanged.

PDH's `#n` is a zero-based *same-name instance occurrence index*, not a proven GPU
context ID. The parser preserves it; the pure aggregator refuses nonzero duplicate
indexes rather than assuming their values are independent additive work. Its
context IDs must be independently proven by a future source adapter.

The pure model accepts at most 256 selected-PID rows with identical explicit
clock/interval boundaries, engine identity (adapter LUID + physical index +
engine index), independently identified contexts and quality status. It sums
proven contexts within an engine, then selects the **maximum engine percentage**
across adapters. It never sums/averages independent engines, silently clamps
over-100/inconsistent data, or redistributes missing work. Repeated context IDs,
unknown quality, mismatched intervals/clocks, negative/nonfinite values,
unproven PDH duplicates and excess bounds produce partial/NaN. No rows means
unavailable/NaN; observed zero remains valid; supplied warmup/reset statuses
remain distinct. Raw running-time baseline/reset detection is a future sampler
responsibility, not claimed by this normalized-row model.

Dedicated committed, shared committed, dedicated resident and shared resident
bytes remain separately named and **unavailable** in this slice. No PDH
`Dedicated Usage`/`Shared Usage` value is guessed into a committed/resident field.
DXCore's documented distinctions must be mapped and validated before exposing
actual memory values.

PDH's two collection timestamps come from `PdhCollectQueryDataWithTime` in their
original FILETIME units; the whole acquisition bracket separately uses boot
clock 2. These clocks are not silently equated or mapped to QPC. Retained-handle
PID+creation/liveness checks bracket the probe. This diagnostic probe does not
return a complete sample or persist per-instance history; its final selected-GPU
status remains explicitly unavailable.

Actual host evidence: one DXCore adapter supports both per-PID states. The
engine-0 call returned S_OK but `processQuerySucceeded=false` for the idle native
test; memory returned S_OK with `processQuerySucceeded=true`. English PDH queries
and arrays succeeded with 459 engine and 28 memory instances, all recognized by
the bounded parser, but **zero selected-process instances**. This does not prove
an available selected-process utilization sample, GPU workload correctness or
hardware parity. Foundation tests passing is not a hardware validation pass.

Public provenance:

* https://learn.microsoft.com/en-us/windows/win32/api/dxcore_interface/ne-dxcore_interface-dxcoreadapterstate
* https://learn.microsoft.com/en-us/windows/win32/api/dxcore_interface/ns-dxcore_interface-dxcoreenginequeryinput
* https://learn.microsoft.com/en-us/windows/win32/api/dxcore_interface/ns-dxcore_interface-dxcoreprocessmemoryqueryinput
* https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhaddenglishcounterw
* https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw
* https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhcollectquerydatawithtime
* https://learn.microsoft.com/en-us/windows/win32/perfctrs/specifying-a-counter-path
* https://devblogs.microsoft.com/directx/gpus-in-the-task-manager/

At P07d, the next GPU slice was physical/engine enumeration, selected-process
workload, raw cumulative baselines, reset/removal and process-memory mapping.
Later sections record implemented capture/normalization and source faults;
the current remaining hardware/memory/parity gates are listed above.
P07d did not change the memory ABI v1.

## Reproduction

### Native DLL and real ABI invocation

```powershell
& tools\WinUIProfiler\WinUIProfiler.Native.Tests\Run-NativeDllTests.ps1
& tools\WinUIProfiler\WinUIProfiler.Native.Tests\Run-NativeDllTests.ps1 -Architecture ARM64
```

These build the standalone `WinUIProfiler.Native.vcxproj` with installed Visual
Studio MSBuild/public SDK and print SHA256, PE machine and exact export table.
The script selects an installed native toolset (this host v145); the project
defaults to VS2022 v143 when used directly. Its local Directory.Build.props
isolates managed/product defaults. Outputs are exclusively:

* `WinUIProfiler.Native\bin\x64\Release\WinUIProfiler.Native.dll`
* `WinUIProfiler.Native\bin\ARM64\Release\WinUIProfiler.Native.dll`

The `.def` exposes exactly `WupCaptureProcessMemory`, `WupNormalizeCpu` and
`WupCaptureCpu`, with
unmangled C names. Memory request/result remain 24/112 bytes, version 1; CPU
normalization remains v2. All three entry points are explicitly `noexcept`, allocate
no C++ objects on the heap and pass no STL types. Unexpected internal C++
exceptions terminate rather than cross the ABI; ordinary failures use existing
HRESULT/status rules. Release uses a static C++ runtime. Actual CPU capture
is exported; GPU sampling is not.

The x64 smoke test validates PE architecture before loading, requires an explicit
drive-qualified path and uses `LoadLibraryExW` with only DLL-directory/system32
dependency search flags. There is no default PATH or DLL-name fallback. It checks
all three exports before invocation, exercises actual memory measurement and CPU
warmup/normalization through function pointers, and verifies output canaries and
memory size/version rejection. Missing DLL, wrong architecture and missing
export are explicit failures. ARM64 DLL/smoke compilation is not runtime evidence.

**Solution integration is parent-owned:** the parent is integrating native and
new IntegrationTests projects; P07f does not edit `WinUIProfiler.sln` or managed
files. Until that work is independently validated, the standalone native command
above remains the explicit native build entry point. Mixed builds must include
the C++ project (type `{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}`, project
`{134D1F91-807B-48D6-A015-6C1F31C83BE4}`) with Debug/Release x64/ARM64 mappings,
choose an explicit Any CPU architecture policy and use Visual Studio MSBuild
for mixed builds. Do not silently omit native targets or break managed-only
dotnet test entry points. For this checkpoint the above independent build is
the required native command; solution/build-owner coordination is still needed.

Parent-reported managed validation now includes 23/23 focused tests: 19 decoder
and four actual DLL load/capture interop tests, following four expected RED
implementation failures. The parent is moving interop tests into the new
IntegrationTests project so ordinary Engine unit tests have no DLL-artifact
dependency. The previously reported full suite was 280 passing. The decoder
retains boot clock 2/acquisition bracket rather than mislabelling them QPC.
Managed memory invocation is actual parent evidence, not typed fixtures alone;
CPU managed invocation, polling and ETW wire conformance were still pending at
that historical checkpoint; current verified integration is summarized above.

### CPU capture ABI v1

`WupCpuCaptureRequest` is 24 bytes: size/version and selected PID+actual creation
FILETIME. `WupCpuCaptureResult` is 3,696 bytes:

| Offset | Field |
| --- | --- |
| 0 | uint32 size |
| 4 | uint32 capture version (1) |
| 8 | uint32 error code |
| 12 | uint32 error domain: none/Win32/NTSTATUS (0/1/2) |
| 16 | uint64 acquisition beginning, boot clock 2, 100 ns units |
| 24 | uint64 acquisition end, same clock |
| 32 | 80-byte `WupCpuSnapshot`, unchanged version 2 |
| 112 | 64 × 56-byte `WupCpuGroup`, unchanged version 2 |

The caller owns the entire result buffer. Missing capacity/null request/output,
bad request size/version or malformed identity is invalid argument without
output modification. OS/identity/exit failures instead produce a status-bearing
capture result with no valid groups. Only `snapshot.group_count` entries exist;
unused zero-filled array storage is not observed zero CPU.

The retained borrowed handle requires QUERY_LIMITED_INFORMATION | SYNCHRONIZE.
Actual PID+creation/liveness are checked before and after internal all-group
capture. Source remains explicitly `WupNtGroupProcessTimesCompatibility` (2),
not Task Manager parity. Raw source snapshot timestamps/clock 2 are retained
unchanged; the outer acquisition bracket includes identity validation. Error
domain distinguishes original NTSTATUS from Win32 errors.

Capture is stateless: an available raw snapshot itself is OK, not calculated
utilization. Call `WupNormalizeCpu` with null previous for warmup/NaN, then with
two successful same-identity complete captures for an interval. Failed captures
must not become CPU baselines. Utility remains unavailable. The DLL smoke
performs this exact sequence through resolved exports and checks request
version/size/capacity, canaries, PID/creation mismatch, invalid handle and owned
target exit. Memory ABI v1 remains unchanged.

### Standalone metric regression suites

From the worktree root, using installed Visual Studio C++ tools:

```powershell
& tools\WinUIProfiler\WinUIProfiler.Native.Tests\Run-NativeTests.ps1
& tools\WinUIProfiler\WinUIProfiler.Native.Tests\Run-NativeTests.ps1 -Architecture arm64
```

The first command compiles with `/std:c++17 /W4 /WX /EHsc /O2` and runs the x64
normalizer, live CPU source, process memory and GPU foundation/probe executables, including short owned-child exit
check. The second cross-compiles only: it provides no ARM64 runtime evidence.
Outputs are under the standalone test's ignored `bin` directory. No package
installation, product build, managed build, ETW session or private OS headers
are required.

Next bounded checkpoint: harden injected API failures/topology races and verify
multi-group hardware, clock/provenance mapping, then independently establish
actual Task Manager collection selection and aligned windows. The live host
probe is one-group evidence only, not proof on multi-group hardware.
Managed contracts/build integration and native collector ETW wire layouts must
be coordinated with their owners before edits; synthetic tests are not real-wire
conformance evidence.

## P07g: selected-process PDH engine snapshot ABI (bounded subset)

The native DLL additionally exports `WupCaptureGpu` (ordinal 4) and
`WupNormalizeGpu` (ordinal 5), both noexcept, caller-owned and version 1.
Request/snapshot/result sizes are 24/14424/8336 bytes; memory v1 and CPU
contracts remain unchanged. This is the explicitly labelled English PDH GPU
Engine alternative (source 6), not Task Manager parity or complete GPU support.

Capture registers `\GPU Engine(*)\Utilization Percentage` using
`PdhAddEnglishCounterW`, validates the counter type as `PERF_100NSEC_TIMER`,
and obtains raw wildcard arrays. The probe and sampler share one bounded
reader/parser: at most three size retries, 1 MiB, 4096 provider instances,
256 selected engine rows and 255 characters per name. Only selected-PID
rows are returned; unrelated instance names are not retained.
The borrowed process handle must remain open with query/synchronize access.
PID, creation FILETIME and liveness are checked before and after acquisition.
PDH itself identifies instances by PID, not creation time; provider stale-instance
behavior and PID reuse remain a hardware/fault-validation gate.

Snapshots preserve raw numerator, denominator, per-row CStatus, counter type,
LUID/physical/engine identity and duplicate occurrence. Query FILETIME and
per-row FILETIME are retained as clock 3 (original PDH timestamps, not QPC);
the acquisition bracket separately declares clock 2 (GetTickCount64 × 10000).
No sleeps or historic CPU/GPU values are synthesized. Retain two snapshots:
normalizing a first valid snapshot returns warmup/NaN; no selected instances
returns unavailable/NaN, not zero. Rate normalization uses the documented
100 ns timer formula, detects decreasing counters/nonadvancing time,
requires common row windows, rejects ambiguous `#n` occurrences, and reuses
the existing busiest-engine aggregator. Observed zero is available zero.
Disappearing engines or incomplete coverage do not produce a complete total.
No PDH occurrence is manufactured into an independent context.

All four GPU memory metrics remain explicitly unavailable/UINT64_MAX; this
checkpoint does not relabel committed memory as resident memory.
Actual x64 DLL capture observed 459 provider instances, zero selected rows,
source 6/status unavailable, PDH error 0. This proves acquisition/capability,
not an active selected-process GPU workload. ARM64 is compilation only.
That P07f checkpoint preceded managed GPU/polling/wire integration and the
subsequent source-fault suites. GPU memory, real device reset/removal,
multi-group hardware, aligned parity and ARM64 execution remain open.

Public provenance:
[raw arrays](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetrawcounterarrayw),
[raw values/status](https://learn.microsoft.com/en-us/windows/win32/api/pdh/ns-pdh-pdh_raw_counter),
[timer formula](https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2003/cc728274(v=ws.10)).

## P07h: opt-in hardware workload / independent PDH oracle

`WinUIProfiler.Native.Tests\Run-GpuHardwareTests.ps1` builds and runs an owned
D3D11 compute workload. It explicitly enumerates at most 16 adapters, excludes
software and conservatively excludes Microsoft Basic Render Driver identities
(1414:008C), and verifies the created device's adapter LUID against the requested
adapter. There is no WARP fallback or override. Device creation alone, including
`D3D_DRIVER_TYPE_HARDWARE`, is not sufficient hardware evidence.

On an accepted adapter, one owned worker submits at most one dispatch in flight,
with a 15-second workload deadline, 10000-dispatch cap and two-second event wait.
The test checks completed shader output using staging readback, joins its worker
and releases its device/resources/query/handle. It requires nonempty selected
PID+creation-matched native snapshots, initial warmup, then positive busiest-engine
measurement. `PdhCalculateCounterFromRawValue` independently evaluates those
exact native operands. A separate English PDH query captures its own raw and
formatted arrays and requires those same-query results to agree.
The separate query window differs from the native window: this is not a timing
benchmark, cross-window numeric-parity test or Task Manager parity claim.

**Current host hardware result: unavailable, native exit 2, not PASS.**
All three visible adapters identified as Microsoft Basic Render Driver 1414:008C;
two reported SOFTWARE clear and allowed explicit D3D11 creation. Their ambiguous
identity is deliberately rejected rather than treated as proven hardware.
An earlier less-strict diagnostic exercised 17 selected rows and matching PDH
arithmetic/readback, but is excluded from hardware evidence.
The final existing DLL smoke still reports 459 instances, selected 0/unavailable.

`Run-GpuHardwareTests.ps1 -ValidateAdapterGuardsOnly` passes the pure guard
fixtures and explicitly does **not** validate hardware.
`Run-GpuHardwareTests.ps1 -Architecture ARM64` cross-compiles only.
The full opt-in x64 command intentionally fails on this host rather than hiding
the missing hardware qualification. No native ABI or GPU memory mapping changed.
Next: run this harness on a separately established non-software GPU environment;
keep the workload/parity, GPU memory and fault-matrix gates open.

## P07i: CPU compatibility-source fault validation

The production resolver and deterministic fault tests now enter the same CPU
capture implementation through an internal, typed `CpuSourceApi` table.
The table is constructed per capture; it is not a global injection switch,
runtime configuration, C ABI entry point or exported DLL symbol. The existing
48-byte compatibility processor record remains internal. C ABI sizes, versions,
five exports and original boot-clock metadata are unchanged.

Thirty source-boundary cases cover missing dynamic functions, short/oversized/
misaligned SPPI lengths, negative/inconsistent times, checked sum overflow,
NTSTATUS provenance, group-count and per-group processor-count changes,
protocol bounds, target exit, pre/post PID and creation mismatch, inventory
length/full/missing/failure, process-time/wait errors and timestamp overflow.
These inject only OS replies, not a replacement capture or normalization result.
Failure after any partial collection clears every group and process counter,
sets `group_count`/`target_present` to zero and retains an explicit failure
status/error domain. Malformed counters return invalid data; checked arithmetic
overflow remains distinct; topology races return unavailable/ERROR_RETRY.
PID plus creation FILETIME is verified again after collection.

`Run-NativeTests.ps1` passes all five x64 suites, including 30 source fault
cases; `-Architecture arm64` compiles them only. Actual x64 DLL smoke passes
with the unchanged five-export ABI. The synthetic two-group fixture is not
multi-group hardware evidence. Checks cannot detect topology changes that
occur and revert entirely between observations; atomic multi-group collection
and Task Manager parity remain unproven. Memory EX2 fallback, GPU memory/faults,
the P07h hardware qualification gate and ARM64 runtime remain separate work.

## P07j: process-memory EX2/EX source-fault boundary

The memory export now resolves a per-call internal `ProcessMemoryApi` and
invokes the same capture implementation as the deterministic source-fault
tests. No global fault hook, new export, configuration mode or ABI change was
introduced. Memory request/result remain 24/112 bytes, version 1; the DLL
still has exactly five exports.

EX fallback is limited to the existing capability-error set
(`ERROR_BAD_LENGTH`, `ERROR_INVALID_PARAMETER`, `ERROR_NOT_SUPPORTED`,
`ERROR_CALL_NOT_IMPLEMENTED`), a successful older EX-size reply, or a successful
reply that leaves EX2's private-working-set field unpopulated. Source 5 and
partial status remain explicit; the capability error is retained.
EX supplies total working set and private commit only. Private working set
remains unavailable/UINT64_MAX, never private commit or an invented zero.
Access-denied and unrelated query failures do not trigger EX fallback.

Unexpected structure sizes, unpopulated base fields and private working set
larger than total working set are rejected. Missing-field sentinels distinguish
malformed successful replies from valid measured zeros. Query failures and
malformed replies recheck retained-handle PID/creation/liveness; a lifecycle
failure takes precedence. Post-query failures or invalid clock brackets clear
all three gauges to explicit missing values. Original boot-clock metadata is
unchanged.

`Run-NativeTests.ps1` passes all six x64 suites, including 38 memory source
cases and 30 CPU source cases. Actual x64 EX2 sampling and the touched 16 MiB
allocation regression pass, as does the rebuilt DLL ABI smoke.
ARM64 suites/DLL compile only. EX fallback is exercised by injected OS replies
through the production boundary, not claimed as an observed older-OS capture.
Older-OS hardware validation, GPU memory/faults/parity, the P07h hardware gate
and ARM64 execution remain separate.

## P07k: GPU capture SOURCE fault boundary

`WupCaptureGpu` now constructs an internal per-call `GpuSourceApi` table and
enters the same capture and bounded raw-array reader used by source-fault
fixtures. The table is not exported or globally configurable. PDH remains a
public, statically imported dependency; missing-table-entry tests are internal
capability guards, not a claim that PDH is dynamically loaded.
All five exports, ABI layouts and versions are unchanged.

Provider open/add-English/info/collect/array/close errors are retained in
`pdh_error`. Local unsupported counter type reports Win32
`ERROR_NOT_SUPPORTED`, rather than inventing a missing-PDH-function error.
Invalid sample timestamps and backwards acquisition brackets report
`ERROR_INVALID_DATA`; boot-tick conversion overflow reports
`ERROR_ARITHMETIC_OVERFLOW`. PID, creation and liveness are checked before and
after acquisition with precise lifecycle errors. Closing an opened query is
attempted exactly once, including error paths; close failure is surfaced.

Every rejected capture clears all 256 row slots, not just the row count.
Structural array/name errors reject the capture; unknown selected instances
cannot silently produce a complete total. Bad CStatus, negative counters,
zero row timestamp and ambiguous duplicate identities/`#n` suffixes produce
explicit partial row quality with unusable counter operands cleared.
No selected rows remain unavailable, never measured zero. The generic probe
reader still skips non-engine memory-family instances; the actual GPU Engine
capture requires engine identities.

`Run-NativeTests.ps1` passes all seven x64 suites, including 53 GPU source
fault cases, 38 memory source cases and 30 CPU source cases. Actual rebuilt
x64 DLL smoke passes; ARM64 suites/DLL compile only. This validates the source
boundary, not hardware workload, GPU memory, exhaustive provider fuzzing,
allocation failures, device removal or Task Manager parity. P07h's hardware
qualification gate remains unchanged.

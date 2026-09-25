# Window placement persistence implementation

> **AI-assisted document:** This document was written and revised with AI assistance. AI makes
> mistakes; technical claims require human review.

**Status: proposed implementation design.** These notes adapt the earlier persistence design to the
current [API spec](../../specs/window-placement-persistence-spec.md). They do not describe a completed
implementation. The private storage layout and binary format below remain proposals, not public
contracts or claims of compatibility with prototype data.
The unpackaged-default-store section records a wanted Windows App SDK dependency beyond the API
spec's current packaged-only automatic-storage scope.

### First implementation unit

The private binary record codec and saved-snapshot structural validator are implemented in
[`WindowPlacementRecord.h`](../../dxaml/xcp/dxaml/lib/WindowPlacementRecord.h) and
[`WindowPlacementRecord.cpp`](../../dxaml/xcp/dxaml/lib/WindowPlacementRecord.cpp).
The dedicated native `WindowPlacementRecordTests` class covers golden records for all six states,
optional metadata, versioning, bounded TLV parsing, malformed/duplicate fields, and checked geometry.
The codec compiles into the framework's WinRT foundation library; the isolated test project compiles
the same source without requiring a window, storage, or a display configuration.

This unit consumes and produces **decoded binary bytes**, not settings strings. Base64 transport
(including its encoded-length bound), storage/identity/retention, public API projections, capture,
native placement, and window lifecycle integration remain unimplemented in this unit. No window
currently calls the codec. The internal snapshot types are not public API declarations, and
implementing this unit does not ratify the proposed durable format or establish end-to-end
persistence support.

Validation on 2026-09-23: `.\initrun.ps1 .\build.cmd /q` succeeded for `amd64chk`.
`.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName <vm> '*WindowPlacementRecordTests*'`
refreshed the test payload and passed all 13 tests (none failed, blocked, or skipped). These are
data-only tests; they do not establish native placement, multi-monitor, or variable-DPI coverage.

### Second implementation unit

Base64 transport and the private container/value names are implemented in
[`WindowPlacementStorageFormat.h`](../../dxaml/xcp/dxaml/lib/WindowPlacementStorageFormat.h) and
[`WindowPlacementStorageFormat.cpp`](../../dxaml/xcp/dxaml/lib/WindowPlacementStorageFormat.cpp).
The transport wraps the existing codec rather than repeating the wire format: `EncodeRecordText`
encodes a record to base64, and `DecodeRecordText` enforces the
[encoded-length bound](#envelope-and-bounds) before allocating a decode buffer, rejects
non-canonical base64, and then calls `DecodeRecord`. The naming helpers implement the
[proposed container and value names](#proposed-packaged-container-and-value-names), including the
uppercase Base32 SHA-256 digests, the 16-character readable slug, and the `id` fallback.
`BCryptHash` supplies the platform SHA-256.

The dedicated native `WindowPlacementRecordTextTests` and `WindowPlacementStorageNameTests` classes
cover golden encoded text, all three padding lengths, malformed and non-canonical text, the
encoded-length bound, version reporting, golden digests for ASCII, non-ASCII, case-distinct, and
surrogate-containing ids, non-normalized ids, slug rules and truncation, embedded NUL code units,
empty-id rejection, and the documented maximum name lengths.

This unit still performs **no storage I/O**. It resolves no application identity, reads and writes
no settings, and enforces no retention. Callers must supply the registered application id and the
raw placement id. Public APIs, capture, native placement, and lifecycle integration remain
unimplemented, and no window calls this code.

### Third implementation unit

The internal [`WindowPlacementStore`](../../dxaml/xcp/dxaml/lib/WindowPlacementStore.h) adapter now
owns shared reader outcomes, complete-value replacement, positive save-sequence allocation,
per-namespace save coordination, and best-effort retention of the 32 newest placement records.
Its settings backend is a narrow ABI boundary, with
[`ApplicationDataPlacementSettingsBackend`](../../dxaml/xcp/dxaml/lib/WindowPlacementStore.h)
providing the packaged `ApplicationData.LocalSettings` implementation. Reads look up existing
containers and values without creating them; writes create the placement container and replace the
complete string value. Isolated tests can inject missing containers, wrong value types, corrupt or
oversized records, replacement failures, cleanup failures, unrelated values, recency ordering,
sequence overflow, and HRESULT failures. A real backend test covers the no-package unavailable
classification. Packaged identity resolution uses `GetCurrentApplicationUserModelId` and never
falls back to package-wide or unpackaged storage. This unit does not expose a public API or perform window
integration.

Validation on 2026-09-23: `.\initrun.ps1 .\build.cmd /q` succeeded for `amd64chk`, and
`.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName <vm> '*WindowPlacement*Tests*'` passed all 29
tests in the isolated project (none failed, blocked, or skipped). These remain data-only tests.

Validation on 2026-09-24: the `amd64chk` build succeeded, and
`*WindowPlacementStoreTests*` passed all 12 tests on the `ge_current-260828-Desktop` VM.

### Public Window placement capture API

The WinUI contract 12 `Window.TryGetPlacement` API is implemented for desktop windows.
It synchronously captures the effective current placement and returns an independent
`WindowPlacement` object. UWP returns `false` because placement capture is not supported
by that implementation. The dedicated `WindowPlacementPersistenceTests` integration
coverage validates the desktop API against a real WPF-hosted window.

## Dependency-ordered implementation plan

Steps 1 through 6 are implemented. Step 7 is implemented except for its display-environment
gates. Each step below keeps its original scope
statement followed by a status note, so a reviewer can check the claim against the named source.
Each step has a narrow owner boundary so it can be implemented and
committed in one developer round without treating isolated helpers as feature completion.

1. **Packaged store adapter and retention.** Build an internal adapter over the packaged
   `ApplicationData.LocalSettings` ABI. Resolve the registered application identity with
   `GetCurrentApplicationUserModelId`, derive the application and placement names with the existing
   formatting helpers, and read or replace complete string values without creating containers on
   load. Return the five reader outcomes in this document. Add the per-namespace save/cleanup
   guard, positive save-sequence allocation, whole-value publication, and best-effort 32-record
   trimming. Cover missing/type-invalid/oversized/corrupt/unsupported values, sequence ordering,
   failure between replacement and deletion, and unrelated-value isolation with a dedicated store
   test class. This step depends on the record codec and formatting helpers.
   _Implemented (2026-09-24): `ApplicationDataPlacementSettingsBackend` uses the Windows
   `ApplicationData` ABI, preserves phase-specific HRESULT classifications, does not create
   containers while loading, and creates them only for writes. The dedicated store tests cover
   injected failures and the real no-package backend result._
2. **PlacementEx adapter and policy seam.** Replace the current policy implementation's duplicated
   monitor selection, normal/snap adjustment, and cascade arithmetic with a thin adapter over
   `PlacementEx`. The adapter should construct and validate the engine's working placement, call
   `FindClosestMonitor`, `MoveToMonitor`, and `Cascade` (or the corresponding native operation),
   and translate only the WinUI-owned state, constraint, launch-hint, explicit-cascade, and
   visibility/activation decisions. Do not retain a second geometry engine for production behavior.
   Keep storage and policy independent. Unit tests may use an injected adapter seam, but expected
   geometry must come from the imported engine or an explicitly documented engine-compatible test
   double. Add coverage for differently sized monitors where center-distance selection differs from
   `MonitorFromRect`, work-area/DPI changes, constraints, explicit position-only cascading, and
   no-source-retry-after-apply. This step depends on the store reader only for source data.
   _Step 2A implemented: `WindowPlacementAdapter.h/.cpp` provides checked translation between
   `NativeRequest` and a `PlacementEx` working value._
   _Step 2B implemented: `WindowPlacementPolicy.cpp` no longer carries its own geometry or topology
   algorithms. `SelectMonitor` and `ComputePlacement` call `TrySelectEngineMonitor`,
   `TryCreateEnginePlacement`, `TryMoveEngineToMonitor`, `TryCascadeEngine`, and
   `TryReadEnginePlacement`. The policy unit retains only the WinUI-owned decisions: state mapping,
   cascade permission, constraint clamping, and native request shaping._
   _Step 2C progress (2026-09-24): native application is wired for explicit/saved placement and
   launch fallback. See [native application status](#native-application-status-2026-09-24) for
   capability restrictions, actual validation, and remaining acceptance work._
3. **Capture adapter and placement cache.** Integrate `PlacementEx::GetPlacement` with
   `DesktopWindowImpl` state caches. Preserve meaningful overlapped state while hidden, track
   minimized-from-snapped history, retain complete pre-full-screen snapshots, capture physical
   coordinates, and safely cache optional virtual-desktop identity. Add focused native tests for
   hidden moves, presenter transitions, failed/partial application, message-safe capture, and
   HWND lifetime correlation. This step depends on the private snapshot and native adapter.
   _Not accepted (2026-09-23): the landed `WindowPlacementCapture` re-implements capture with raw
   Win32 instead of `PlacementEx::GetPlacement`, and its state, coordinate-space, minimized, and
   hidden handling are wrong. See the
   [capture and DesktopWindowImpl integration review](#capture-and-desktopwindowimpl-integration-review-2026-09-23)._
   _Reworked and implemented (2026-09-24): capture now runs on `PlacementEx::GetPlacement` plus
   `TryReadEnginePlacement`. See the
   [capture rework](#capture-rework-on-the-placement-engine-2026-09-24)._
4. **Public API projections and detached loading.** Add the `WindowPlacement`, state/options/enums,
   and `Window` property/method projections in the generated/API declaration pattern used by the
   repository. Implement validation-before-side-effects, snapshot independence, C#/C++/WinRT
   projection behavior, and `LoadForPersistPlacementId` through the shared store reader. Detached
   loading must not consult a window, select peers, adjust or apply placement, change phase state,
   or enroll saving. Add a dedicated projection/API test class. This step depends on the store
   reader and private snapshot types.
   _Implemented: XamlOM projects `WindowPlacement`, `WindowShowOptions`, and their three enums. The
   value objects support constructor validation, permissive editing, nullable fields, thread-safe
   properties, and private coherent request snapshots. `Window` exposes `PersistPlacementId`,
   `UseAutomaticPlacementPersistence`, `Show(WindowShowOptions)`, and `TryApplyInitialPlacement`
   through `WindowImpl`, with desktop behavior in `DesktopWindowImpl` and no-op desktop-only
   members on `UWPWindowImpl`. `WindowPlacementFactory::LoadForPersistPlacementIdImpl` reads
   through the shared store without consulting a window, selecting peers, adjusting placement, or
   enrolling saving. `WindowPlacementApiTests` covers the managed projection and
   `WindowPlacementPublicTests` covers request validation and independent native copies._
   _C++/WinRT runtime coverage decision (2026-09-24): Do not add a dedicated C++/WinRT test lane
   for this implementation unit. Such a lane would verify that a C++/WinRT header generated from
   the locally built winmd can compile and consume the projected signatures, and could expose
   C++/WinRT-specific header code-generation or value-marshaling defects. The managed projection
   tests already exercise the public projection at runtime, while the ABI tests exercise the
   underlying WinRT interface, factory, validation, and value behavior. Because the C++/WinRT
   header is a thin wrapper over that same ABI, the proposed lane would add header-consumer
   validation but no distinct placement behavior or ABI coverage for this unit. Header generation
   remains a build concern; the raw ABI lane is the runtime boundary for this repository._

   All three step 4 prerequisites are resolved.

   | Prerequisite | Resolution |
   | --- | --- |
   | Unexpected loader errors | `LoadResult` carries the failing HRESULT. `ReadPersistPlacement` turns a failed load into a failing HRESULT, and `LoadForPersistPlacementIdImpl` returns `loaded.Error` for `LoadStatus::Unexpected` while still mapping missing, unavailable, and invalid records to a null result. |
   | Late persistence configuration | Late setters are accepted. First-display enrollment is fixed, and each save snapshots the current Boolean and id. An initially empty id or an explicit `false` cannot enroll later. Covered by the `WindowPlacementPersistenceTests` persist-id and opt-out cases. |
   | Real packaged storage | `ApplicationDataPlacementSettingsBackend` supplies the concrete Windows `ApplicationData` backend, with storage-backed coverage for load-without-container-creation and application isolation. |

5. **Initial-placement coordinator and cascading.** Wire `Show`, `Activate`, and
   `TryApplyInitialPlacement` to a coordinator that snapshots request/id/Boolean state before
   callbacks, validates reentrancy and options, selects explicit peer/saved/fallback sources, and
   applies the native policy. Track phase-open, in-progress, ever-displayed, first-display
   enrollment snapshot, and close-save completion separately. Keep hidden-application outcomes
   local to each attempt rather than using them as lifecycle state.
   `TryApplyInitialPlacement` remains repeatable while hidden; only first display ends the phase.
   A first `Show` with `SkipInitialPlacement` skips loading, peer selection, cascading, and fallback
   repositioning, still ends the phase, and rejects non-null placement or try-apply requests.
   Add lifecycle tests for repeated hidden application, skip and non-skip first display, later-show
   option ignoring, direct native display, reentrancy, invalid combinations, and both activation
   choices. This step depends on the public projections and native/capture adapters.
   _Implemented: `DesktopWindowImpl` owns a `WindowPlacementCoordinator` and implements
   `IWindowPlacementCoordinatorHost`. `Show`, `Activate`, and `TryApplyInitialPlacement` route
   through the coordinator, which tracks phase-open, in-progress, ever-displayed, and enrollment
   state separately._
6. **Saving and teardown integration.** Add accepted-close, destruction backstop, and confirmed
   session-end hooks in `DesktopWindowImpl`, snapshot after close handlers, perform bounded
   synchronous save attempts while state is valid, and keep cancellation/failure non-fatal.
   Enforce the first-display visibility requirement and current-Boolean/id snapshot for every save. Add
   storage-backed lifecycle tests for accepted/cancelled close, destruction, session-end,
   id changes, clear/reassign, duplicate-save suppression, and save failures. This step depends on
   the coordinator, capture adapter, and store retention implementation.
   _Implemented: `DesktopWindowImpl` calls `OnAcceptedClose` after close handlers, `OnDestroy` from
   both the managed destroy path and `WM_DESTROY`, and `OnSessionEnd` on a confirmed `WM_ENDSESSION`.
   Each save snapshots the current Boolean and id, requires prior display, and is non-fatal on
   failure. `WindowPlacementPersistenceTests` covers the opt-out, late opt-out, close-time id,
   initially empty id, per-id isolation, and replacement cases against real packaged storage._
7. **End-to-end and display validation.** Add a dedicated Window Placement integration test class
   and test app coverage for process-A/process-B round trips, package/application isolation,
   concurrent same-id saves, cascade precedence, saved minimization and snapping, and unchanged
   legacy behavior. Run the targeted isolated tests and the new window tests on the VM. Establish a
   controlled multi-monitor/variable-DPI lane before claiming those criteria; unsupported display
   capabilities must be reported as skipped rather than passing.
   _Implemented except for the display-environment gates (2026-09-25)._

   | Criterion | Coverage |
   | --- | --- |
   | Process-A/process-B round trip | `WindowPlacementCrossProcessTests`, exact bounds with no tolerance, plus per-id isolation across processes |
   | Package and application isolation | `WindowPlacementStorageIsolationTests::RecordWrittenUnderAnotherApplicationIdentityIsNotVisible` |
   | Concurrent same-id saves | `WindowPlacementStorageIsolationTests::ConcurrentSavesForOneIdLeaveOneCoherentRecord` |
   | Cascade precedence | `WindowPlacementExplicitCascadeTests` on one display, in both the managed and isolated classes |
   | Saved minimization | `WindowPlacementPersistenceTests::MinimizedPlacementSurvivesCloseAndRestore` with `ApplicationRestart`; `WindowPlacementApplicationTests::SavedMinimizationRequiresApplicationRestart` |
   | Saved snapping | `WindowPlacementApplicationTests::SavedSnapApplicationOnANewWindow` and `MinimizedFromSnappedRestoresToSavedSnapBounds`; see the [snapped first-show diagnosis](#native-application-status-2026-09-24) |
   | Unchanged legacy behavior | `LegacyShowDoesNotMoveOrResizeTheWindow` and `LegacyActivateDoesNotMoveOrResizeTheWindow` |
   | Live two-display peer enumeration | Open. Needs a VM with two interactive displays |
   | Multi-monitor/variable-DPI lane | Open, and narrowed to selection and relocation between two monitors. Environment not selected. See [what the lane still owns](#what-the-multi-monitorvariable-dpi-lane-still-owns) |

#### What the multi-monitor/variable-DPI lane still owns

A single display cannot reach these production paths. Earlier versions of this table claimed more
than that; the parts one display *can* reach are now covered and listed below. The lane exists for
the rest, and the environment it needs is in
[multi-monitor and DPI environment to establish](#multi-monitor-and-dpi-environment-to-establish).

| Production path | Why one display cannot cover it |
| --- | --- |
| `PlacementEx::FindClosestMonitor` choosing *among several connected displays* | Selection matches the saved device name first, then falls back to `MonitorFromRect`. With one monitor both branches return the same monitor, so which monitor wins is unobservable. A reordered display, or a saved position that lands between two displays, still needs two. |
| `PlacementEx::MoveToMonitor` *between two monitors* | Only two displays can show that the engine relocates a window onto a different monitor's work area and adopts that monitor's device name. |

The stale-display and DPI-change halves of those two rows are already covered on one display, because
neither depends on a second monitor:

| Covered on one display | Test |
| --- | --- |
| A saved display that is no longer connected takes the `MonitorFromRect` fallback and still produces a valid request for the connected monitor | `WindowPlacementApplicationTests::AStaleSavedDisplayNameFallsBackToTheConnectedMonitor` |
| A record saved at a different display scale is rescaled by `MoveToMonitor` | `WindowPlacementApplicationTests::APlacementSavedAtAHigherDpiRescalesOnTheConnectedMonitor` |

`MoveToMonitor` rescales from the placement's own saved work area and DPI to the target monitor's, not
from one monitor to another. So a record saved at 200% and restored at 100% on the same display hits
the rescale in production. `ComputePlacement` calls the move unconditionally, and the test measures a
440x340 saved rectangle coming back as 220x170 at half the saved DPI.

Track-size scaling used to be a third row. It converts the `WM_GETMINMAXINFO` limits from the
window's DPI to the target monitor's DPI, and on one display those two values are equal, so the
conversion is an identity. That arithmetic is WinUI-owned rather than a `PlacementEx` call, so it is
now the named function `WindowPlacementPersistence::TryScaleTrackSize`. `TryComputeNativeRequest`
calls it once per limit and fails the request if any call fails.
`WindowPlacementApplicationTests::TrackSizesScaleToTheTargetMonitorDpi` covers identity, scale up,
scale down, rounding, zero and negative limits, an unusable DPI, and `MulDiv` overflow on a single
display. Only the two live engine paths above still need two displays.

`WindowPlacementPolicyTests` does cover the WinUI-owned arithmetic against synthetic topologies: a
96 DPI primary and a 192 DPI secondary, in `RelocatesAndScalesForLowerDpiMonitor`,
`ScalesSizeUpForHigherDpiMonitor`, `WorkAreaChangeKeepsRelativePosition`,
`MatchesSavedDeviceNameEvenWhenGeometryChanged`, `MissingMonitorSelectsLargestIntersection`, and
`MissingMonitorFallsBackToNearestNotPrimary`. That coverage does not stand in for the rows above.
`TryComputeNativeRequest` builds `Topology topology{{target}}` from the one monitor the engine
already chose, so `TrySelectEngineMonitor`'s multi-monitor branches and the `LaunchMonitorHint`
branch of `ComputePlacement` run only in tests. Read those tests as proof of the arithmetic, not of
live monitor selection.

_Production save wiring:_ `DesktopWindowImpl` constructs the placement
coordinator after the desktop window is created and uses the packaged `ApplicationData` backend
through `PlacementStore` for accepted-close and teardown saves. UWP windows provide no-op
implementations for the desktop-only persistence surface. Native placement application is
implemented; see [native application status](#native-application-status-2026-09-24). The
process-A/process-B round trip, package/application isolation, concurrent same-id saves, and
cascade precedence on one display are covered. Saved maximization round trips through real
packaged storage. Saved minimization also round trips with exact restore bounds when first
`Show` uses `ApplicationRestart`. The earlier skip used `Default`, which intentionally restores
saved minimization to normal; it was not an OS refusal. Unchanged legacy behavior is covered by
`LegacyShowDoesNotMoveOrResizeTheWindow` and `LegacyActivateDoesNotMoveOrResizeTheWindow`: a window
that sets no persist id, no opt-in, and no show options keeps the exact position and size the
application gave it, so the automatic pass cannot move or cascade a window with no source. Applying
a saved `Snapped` record to a never-shown window is declined by the OS, not by WinUI; the measured
result is in the [snapped first-show diagnosis](#native-application-status-2026-09-24). Still open
for step 7: live two-display peer enumeration, and the multi-monitor/variable-DPI lane, whose exact
remaining scope is in [what the lane still owns](#what-the-multi-monitorvariable-dpi-lane-still-owns).

`WindowPlacementCrossProcessTests` runs two method-isolated hosts with
`runtests 'WindowPlacementCrossProcessTests.*' -PreservePackageRegistration`.
The runner registers the package once, refuses an existing registration, and removes its
registration and restores the manifest after the run. The class is excluded from ordinary runs.
Real save/load and per-id isolation work across distinct processes with the same AUMID, and the
full round trip restores the saved bounds exactly. The class asserts exact bounds with no
tolerance and passes 2/2 on `ge_current-260828-Desktop`. The earlier Y=71 to Y=76 drift was a
redundant same-monitor monitor action; see
[native application status](#native-application-status-2026-09-24).

The late-Boolean/enrollment and pre-display abort contracts are implemented. The detached loader
propagates unexpected packaged-storage HRESULTs while still mapping missing, unavailable, and
invalid records to a null result. Follow the reader and save rules below.
The unpackaged default `ApplicationData` dependency remains open. Keep the
packaged-only automatic-storage contract until Windows App SDK provides a supported default-store
capability. Do not expose a public store override or make a hidden application enroll saving.
The `Activate`-after-hidden-preparation hazard is decided: the automatic pass runs, there is no
implicit skip or `Activate` overload, and a never-displayed window never saves. Implement those
as specified rather than reopening them.

#### Codec review (2026-09-23)

Reviewed commit `a8c1c7b1f`: the record header and implementation, dedicated test class, and
project/solution integration. No correctness findings in this unit against the
[private record format](#private-record-format) and the API spec's
[data-validity rules](../../specs/window-placement-persistence-spec.md#data-validity).

The review covered little-endian field and GUID encoding, the 4 KiB envelope bound, checked
edge/dimension conversion, saved-work-area intersection, all six state mappings, optional defaults,
version handling, and duplicate-field validation before last-one-wins assignment. The tests include
independent golden bytes and malformed-input cases. Failed decoding does not publish partial output.
Tracked call sites are confined to the tests; this unit does not change window behavior.
This was a source review, not a new build or VM run, and does not extend the coverage reported above.

Related open work: [PR #11982](https://github.com/microsoft/microsoft-ui-xaml/pull/11982) removes
`MetadataResetter.cpp` from the same foundation project, outside the codec addition.
[PR #11974](https://github.com/microsoft/microsoft-ui-xaml/pull/11974) changes `DesktopWindowImpl`
sizing and closed-window behavior, which matters for later native/lifecycle integration.
Neither inspected change duplicates the codec.

#### Store-adapter formatting review (2026-09-23)

Reviewed commit `61ad247ca`: `WindowPlacementStorageFormat.h/.cpp`, `WindowPlacementStorageFormatTests.cpp`,
and the project integration. No correctness defects found. Two hardening changes were made during
the review, and both build and pass on the VM.

| Area | Result |
| --- | --- |
| Base64 strictness | Correct and canonical. Rejects null/empty text, non-multiple-of-four length, out-of-alphabet characters, whitespace, embedded NUL, misplaced `=`, three or more `=`, and non-zero unused tail bits. One record has exactly one encoding. |
| Encoded-length bound | Checked before the decode buffer is allocated. 5,464 characters allow 4,098 decoded bytes, two over the 4 KiB ceiling; `DecodeRecord` rejects the excess, so the slack is not reachable. |
| Encode path | Bounded implicitly: `IsValid` caps the device name at 31 code units, so the largest record is 194 bytes, or 260 characters. Nothing can be written that cannot later be read. |
| Base32 digest | Matches uppercase RFC 4648 without padding. 256 bits produce 52 characters, the accumulator cannot overflow, and the final character carries one real bit plus four zero bits, so it is always `A` or `Q`. The golden digests in the tests match that prediction, which is independent evidence they were not generated by this code. |
| Slug and lengths | Matches the [proposed names](#proposed-packaged-container-and-value-names): ASCII `[A-Za-z0-9]` only, at most 16 characters, `id` fallback, 57-character container names, 73-character value names. |
| Case-insensitive backends | Safe. Two distinct ids can only produce names that match case-insensitively if their digests collide, and the container name is pure uppercase Base32. |
| `BCryptHash` with `BCRYPT_SHA256_ALG_HANDLE` | The right choice for the framework DLL. It is a one-shot call with no provider handle to open, cache, or close, and no CNG cleanup on unload. The pseudo-handle needs Windows 10 1703, below the supported floor. `OneCoreUAP.Lib` already exports `BCryptHash`, so the framework DLL link needs no new library; `bcrypt.lib` in the isolated test project is redundant but harmless. |

Changes made: `MaximumEncodedCharacters` is now derived from `MaximumRecordBytes` instead of a
hand-copied 5,464, so the two cannot drift; `BCRYPT_SUCCESS` replaces a raw `status >= 0`. Tests
were added for the derived bound and for `EncodeRecordText` leaving its output untouched on failure.

Validation on 2026-09-23: `.\initrun.ps1 .\build.cmd /q` succeeded, and
`*WindowPlacementRecordTextTests*` (8) and `*WindowPlacementStorageNameTests*` (9) passed on the VM
with none failed, blocked, or skipped. These remain data-only tests: no storage I/O, no window, and
no display configuration.

#### Capture and DesktopWindowImpl integration review (2026-09-23)

Reviewed commit `79c2f629a`: `WindowPlacementCapture.h/.cpp`, the `DesktopWindowImpl` cache,
`WindowPlacementCaptureTests.cpp`, and the project integration. **Step 3 is not accepted.** The unit
does not implement
[Maintain a coherent overlapped snapshot](#maintain-a-coherent-overlapped-snapshot) or
[Cache optional virtual-desktop identity safely](#cache-optional-virtual-desktop-identity-safely),
and it re-implements work the step 2 adapter already does. Two hazards were fixed during the review;
everything else is returned to the Developer.

The core problem is that `TryCaptureFromWindow` re-derives placement from `GetWindowRect`,
`GetWindowPlacement`, `MonitorFromPoint`, `IsIconic`, and `IsZoomed` instead of calling
`PlacementEx::GetPlacement` through the adapter. `TryApplyNativeRequest` already calls
`GetPlacement(hwnd, &captured, CaptureFlags::SkipVirtualDesktopId)` followed by
`TryReadEnginePlacement`, so there are now two capture implementations that disagree. The engine
already reports `Arranged`, `RestoreToArranged`, `RestoreToMaximized`, the arrange rectangle, the
device name, and the DPI. The hand-written state machine tries to rebuild those signals and gets
them wrong.

| Area | Finding |
| --- | --- |
| Engine reuse | Blocking. Duplicate capture path. This contradicts the adopted "prioritize PlacementEx reuse" decision and the step 3 plan text, which says to integrate `PlacementEx::GetPlacement`. |
| Engine configuration | Blocking, fixed during review. `WindowPlacementCapture.h` included `User32Utils.h` **without** `USE_VIRTUAL_DESKTOP_APIS`, while `WindowPlacementAdapter.h` includes it **with** that define. `PlacementEx` declares `GUID virtualDesktopId` only under that define, so the same class had two layouts and two sets of inline functions in one binary. That is an ODR violation across `WindowPlacementAdapter.cpp`, `WindowPlacementCapture.cpp`, and `DesktopWindowImpl.cpp` in both the foundation DLL and the isolated test binary. |
| Coordinate space | Blocking. `NormalRect` is taken from `WINDOWPLACEMENT::rcNormalPosition`, which is in Win32 **workspace** coordinates for a top-level non-tool window. `WorkArea` and the fallback rectangle are in screen coordinates. The spec requires physical pixels in the virtual screen coordinate system and names workspace coordinates as the wrong space. Mixing the two also makes the `IsValid` normal/work-area intersection test unreliable. |
| DPI virtualization | Blocking. Nothing establishes a physical-pixel coordinate context. On a thread that is not per-monitor DPI aware, `GetWindowRect` and `GetWindowPlacement` return virtualized geometry, which is the failure the design note warns about. |
| Minimized capture | Blocking. The monitor is chosen from the center of `GetWindowRect`. A minimized window reports roughly `(-32000, -32000)`, so `MonitorFromPoint` picks a monitor unrelated to the window, `WorkArea` comes from that monitor, and the normal/work-area intersection check then fails. Minimized capture cannot produce a valid snapshot. Use the normal rectangle or `MonitorFromWindow`. |
| Restore history | Blocking. A previous state of `MinimizedFromMaximized` or `MinimizedFromSnapped` is not matched by the minimized branch, so the next capture downgrades it to plain `Minimized`. Minimizing sends both `WM_MOVE` and `WM_SIZE`, so the history is destroyed inside a single minimize. This breaks the spec's saved-state restore table. |
| Snapped detection | Blocking. `State::Snapped` is only reachable when the previous snapshot is already `Snapped`, and nothing ever sets it first. `Snapped` and `MinimizedFromSnapped` are therefore unreachable from capture. The commit message's "simple heuristics" is a 50-pixel, DPI-unaware comparison that only re-confirms a state that can never be entered. |
| Hidden state | Blocking. There is no last-meaningful-show-state tracking. State is derived only from live window styles plus the previous snapshot. `IsWindowVisible` was read into an unused local and has been removed. |
| Virtual desktop id | Blocking. Every capture calls `VirtualDesktopId.reset()`, so any safely cached id is discarded on the next `WM_MOVE`. The design requires overlaying the last cached non-empty id. |
| Presenter transitions | Missing. No complete pre-full-screen overlapped snapshot is retained, and capture does not detect full-screen or compact overlay. |
| `noexcept` and allocation | Blocking. `TryCaptureFromWindow` is `noexcept` but builds a `std::wstring` and appends to a `std::u16string`. An allocation failure calls `std::terminate`. `monitorInfo.szDevice` is already a bounded null-terminated array, so no intermediate string is needed. |
| `IsBadReadPtr` | Fixed during review. `CaptureEffectivePlacement` called `::IsBadReadPtr(this, sizeof(*this))`. That API is banned, is not thread-safe, can trip guard pages, and is meaningless on `this`: if `this` were invalid the call itself would already be undefined. |
| Dead code | The snap branch tests `SnapRect.has_value()` twice, and its `placement.SnapRect = previousSnapshot->SnapRect` assignment is immediately overwritten below. |
| Failure reporting | Capture failure leaves the cache silently stale. Leaving stored placement unchanged is correct per the spec, but there is no diagnostic, so a permanently failing capture looks identical to a window that never moved. |

Test coverage does not support the commit message. Four of the six tests never call
`TryCaptureFromWindow`. `TracksStateTransitionsMinimizedFromSnapped`,
`TracksStateTransitionsMinimizedFromMaximized`, and `CapturesDeviceNameConversion` assign a field to
a local `Snapshot` and then assert that the field holds what was just assigned;
`ValidatesSnapshotDimensions` re-tests `IsValid`, which `WindowPlacementRecordTests` already covers.
Only the two negative HWND cases exercise the new code, and neither reaches capture logic. The
claimed state-transition and device-name behavior is untested. Nothing covers the
`DesktopWindowImpl` cache, hidden moves, presenter transitions, or HWND lifetime correlation, all of
which the step 3 plan lists.

Changes made during the review: `WindowPlacementCapture.h` no longer includes the PlacementEx
headers and takes only `<windows.h>`; `WindowPlacementCapture.cpp` includes
`WindowPlacementAdapter.h` so the engine has one configuration; the `IsBadReadPtr` call and the
unused visibility local are removed. These fix the ODR and banned-API hazards only. They do not make
step 3 correct, and capture should be rewritten on `PlacementEx::GetPlacement` with real tests before
step 5 or step 6 consumes the cache.

#### Capture rework on the placement engine (2026-09-24)

Capture was rewritten on the engine. `WindowPlacementCapture` is now a per-window cache whose only
job is the state the engine cannot report from an HWND. Every blocking finding above is resolved.

`TryCapture(HWND, PresenterKind)` does this:

1. Reject a null or destroyed window.
2. Reset tracked state if the bound HWND changed, and take a new lifetime token.
3. Return false for a non-overlapped presenter, which keeps the last valid overlapped snapshot.
4. Enter a per-monitor-DPI-aware scope, so the engine reports physical pixels.
5. Call `PlacementEx::GetPlacement(hwnd, &captured, CaptureFlags::SkipVirtualDesktopId)`.
6. Reject a full-screen window, then normalize the engine value into the request vocabulary.
7. Translate with `TryReadEnginePlacement`, reconcile with the cached snapshot, and overlay the
   cached virtual desktop id.

| Owner | Responsibility |
| --- | --- |
| Engine | Normal rectangle in screen coordinates, work area, device name, DPI, `Arranged` plus the arrange rectangle, `RestoreToMaximized`, and the monitor for a minimized window. |
| WinUI | Last meaningful show state, snapped restore history, last valid overlapped snapshot, cached virtual desktop id. |

Two translation details are required, because the engine's capture vocabulary is wider than the
request vocabulary the adapter validates:

| Detail | Handling |
| --- | --- |
| Show-command aliases | `GetWindowPlacement` reports `SW_SHOWMINIMIZED`, `SW_SHOWMAXIMIZED`, and other aliases. `TryReadEnginePlacement` only accepts `SW_NORMAL`, `SW_MAXIMIZE`, `SW_MINIMIZE`, and `SW_HIDE`, so capture canonicalizes first. Without this, minimized capture fails outright. |
| Flags a request cannot carry | `SW_HIDE` only validates as `NoChange` with `KeepHidden` and `NoActivate`, and neither maximize nor hide may carry `Arranged` or a restore flag. Capture drops those combinations and recovers the state from the cached snapshot instead. |

`TryReconcile` is a static function that takes the engine value, window visibility, and the previous
snapshot. It is separate from the window so the state rules, including the snapped states that
cannot be staged reliably in a test, are testable without one. The rules:

| Engine value | Result |
| --- | --- |
| `Maximize` | `Maximized`. |
| `Minimize` with `RestoreToMaximized` | `MinimizedFromMaximized`. The engine flag wins over tracked history. |
| `Minimize` with `RestoreToArranged` | `MinimizedFromSnapped` with the engine arrange rectangle. |
| `Minimize` after a tracked snapped state | `MinimizedFromSnapped` with the tracked snap rectangle. `GetPlacement` cannot reconstruct this from an already minimized window, and minimizing sends both `WM_MOVE` and `WM_SIZE`, so the history has to survive repeated captures. |
| `Minimize` otherwise | `Minimized`. |
| `NoChange` (`SW_HIDE`) | The last meaningful state, with new geometry. A hidden window can still move. |
| `Normal` with `Arranged` | `Snapped` with the engine arrange rectangle. |
| `Normal`, not visible, tracked `Snapped` | `Snapped`. `IsWindowArranged` goes false as soon as the window is hidden, so hiding alone must not replace meaningful state. |
| `Normal` otherwise | `Normal`. |

A snapped state that ends up without a snap rectangle degrades to `Normal` or `Minimized` rather
than failing capture. The result is gated on `IsValid` and the output is replaced only on success,
so a failed capture always leaves the cache on its last good snapshot.

Virtual desktop identity stays out of capture. `TryCapture` passes `SkipVirtualDesktopId`, because
the query is a cross-apartment COM call that can fail inside a window message handler. A caller
that obtains an id safely calls `TrySetVirtualDesktopId(lifetimeToken, id)` with the token it
started from. A stale token or an empty id is rejected, so a late result cannot attach to a closed
or recycled HWND. `Detach()` invalidates outstanding tokens at close while leaving the snapshot
readable for a save during teardown.

`DesktopWindowImpl` now owns a `WindowPlacementCapture` instead of a bare `std::optional<Snapshot>`.
It derives the presenter kind from `AreNewWindowingApisEnabled` and `AppWindowPresenterSupportsSizing`,
and calls `Detach()` from `OnClosed` before `CloseImpl`. `CaptureEffectivePlacement` is no longer
`noexcept`, which is the fix for the allocation-in-`noexcept` finding: this library has no `catch`
anywhere, so adding a handler would be dead code.

`WindowPlacementCaptureTests.cpp` was replaced. Thirteen tests cover the reconciliation rules
against a synthetic engine value and capture against a real top-level window: shown capture,
hidden moves, the maximize/minimize/restore chain including the double restore through maximized,
presenter transitions, capture after window destruction, and virtual desktop id lifetime
correlation across two windows. All 91 Window Placement tests pass on the VM.

Still open as of this review (2026-09-24), and deliberately not addressed here. The status column
records what later work did with each row:

| # | Item | Note | Status |
| --- | --- | --- | --- |
| 1 | Failure reporting | Capture failure was silent. A permanently failing capture looked like a window that never moves. This needed a diagnostic, not a behavior change. | Closed. `PlacementFailureScope` records `PlacementFailureCategory::Capture` in `WindowPlacementPublic.cpp` and on both capture paths in `WindowPlacementApplication.cpp`. `WindowPlacementDiagnosticsTests` verifies the reported `Save`/`Capture` pair. |
| 2 | Full-screen snapshots | Capture rejects a full-screen window and keeps the last overlapped snapshot, which is the required behavior. Entering and exiting full screen through `PlacementEx` is step 5 work. | Capture side closed (2026-09-26). `WindowPlacementCaptureTests::FullScreenWindowKeepsThePreFullScreenSnapshot` enters full screen with `PlacementEx::EnterFullScreen`, asserts `GetPlacement` really reports `FullScreen`, then verifies `TryCapture` fails, the complete pre-full-screen snapshot survives, no lifetime token is consumed, and capture resumes after `ExitFullScreen`. Still open: the apply path does not handle `FullScreen`. That is unreachable from persistence today, because capture never lets a `FullScreen` record be saved, so it only matters if a future caller supplies one. |

#### Code review of the capture rework (2026-09-24)

The rework is a large improvement. Capture now runs on `PlacementEx::GetPlacement`, the coordinate
space and the minimized monitor are right, the reconciliation rules are stated and tested, the
lifetime token keeps a late virtual-desktop result off a recycled HWND, and the tests exercise real
behavior instead of their own setup. Every blocking finding from the previous review is resolved.

Three new findings, plus three minor ones.

| # | Severity | Area | Finding |
| --- | --- | --- | --- |
| 1 | Blocking | Detach on a cancelable close | `Detach()` runs from `OnClosed`, which is the **`WM_CLOSE` handler**, before the close is known to be accepted. `CloseImpl` raises `Window.Closed` and returns without closing when a handler sets `Handled` (`DesktopWindowImpl.cpp`). `WM_CLOSE` never reaches `DefWindowProc`, so on a cancelled close the window stays alive and detached. The next `WM_MOVE` or `WM_SIZE` calls `TryCapture` with the same HWND, sees `hwnd != m_window` because `Detach()` set `m_window` to null, and **resets `m_placement` and `m_virtualDesktopId`**. |
| 2 | Should fix | Exceptions out of a window procedure | Dropping `noexcept` from `CaptureEffectivePlacement` does not fix the allocation hazard; it relocates it somewhere worse. The function is reached from `OnMoved` and `OnSizeChanged`, which the window procedure dispatches directly. Letting `std::bad_alloc` unwind out of a Win32 callback frame is undefined behavior. `noexcept` at least gave a deterministic fail-fast at a known frame. "This library has no `catch` anywhere, so a handler would be dead code" is an argument for where the handler goes, not for letting an exception escape into user32. |
| 3 | Should fix | Capture frequency | Capture runs on every `WM_MOVE` and every `WM_SIZE` with no coalescing. Each pass does a COM QI (`AppWindowPresenterSupportsSizing` -> `TryGetOverlappedPresenter`), two thread-DPI-context switches, `GetWindowPlacement`, `MonitorData::FromWindow` (`GetMonitorInfo` plus `GetDpiForMonitor`), `GetWindowLongPtr`, `GetWindowRect`, and `DwmGetWindowAttribute` when the window is arranged. A drag or a resize produces hundreds of those per second. The same file already coalesces exactly this burst for the restored-size feature in `ScheduleUpdateLastRestoredClientSize`. |
| 4 | Minor | Show-command aliases | `TryCanonicalizeShowCommand` accepts `SW_SHOW` and `SW_SHOWNOACTIVATE` as normal aliases but not `SW_SHOWNA`. If it ever appears, capture fails permanently and silently for as long as the window stays in that state. |
| 5 | Minor | DPI scope failure | When both `SetThreadDpiAwarenessContext` calls fail, `PhysicalCoordinateScope` has no effect and capture stores virtualized coordinates as if they were physical. The scope exists for coordinate correctness, so capture should fail instead of recording a wrong value. |
| 6 | Minor | Conflicting restore flags | `IsValidRequest` rejects `RestoreToMaximized` together with `RestoreToArranged`, so if the engine ever reported both, capture would fail wholesale. `TryCanonicalizeShowCommand` already drops unrepresentable flag combinations; it should drop `RestoreToArranged` when `RestoreToMaximized` is set. `GetPlacement` never sets `RestoreToArranged` today, so this is defensive only. |

Finding 1 in detail. The consequences of the wipe are not limited to the cached desktop id:

| Lost on a cancelled close | Effect |
| --- | --- |
| Last valid overlapped snapshot | On a non-overlapped presenter, `TryCapture` rebinds and wipes **before** the presenter check, then returns false. The snapshot is gone and cannot be rebuilt while the presenter stays full-screen or compact overlay, so a later save has no placement at all. This is the exact invariant the cache exists to hold. |
| Snapped and hidden history | The next capture runs with `previous == nullptr`. A previously snapped window that is currently hidden reconciles to `Normal`, so a cancelled close silently changes what gets saved. |
| Cached virtual desktop id | Discarded, and `TrySetVirtualDesktopId` also fails for every outstanding token until the rebind. Best-effort state, so this alone is minor. |

Two changes are needed, and each wants a test:

1. Detach when the close is actually accepted, not on `WM_CLOSE`. The end of `CloseImpl` after
   `m_bIsClosed = true`, or `Shutdown()`, is the right place. `Shutdown()` is the safer of the two
   because it also covers a teardown that does not come through `WM_CLOSE`.
2. Make `TryCapture` refuse to capture from the HWND it was detached from, instead of treating it as
   a new window and wiping the cache. A detached-flag check is enough. The current rebind-on-change
   rule is right for a genuinely different HWND and should stay.

Step 6 will hit this directly: "cancelled close" is already one of its listed acceptance cases.

Test gaps behind these findings, as of this review (2026-09-24). Both were closed by the fixes in
the next note:

| Gap | Note | Status |
| --- | --- | --- |
| Detach then recapture the same HWND | No test covered it. `CorrelatesVirtualDesktopIdWithWindowLifetime` detaches and then captures a **different** window, where wiping is the wanted behavior, so it passed while finding 1 was live. | Closed by `RejectsRecaptureOfDetachedWindow` in `WindowPlacementCaptureTests.cpp`. |
| Presenter-kind derivation | Nothing covered the `AreNewWindowingApisEnabled` / `AppWindowPresenterSupportsSizing` mapping in `DesktopWindowImpl`. The mapping reads correctly: `AppWindowPresenterSupportsSizing` is an `IOverlappedPresenter3` QI, which fails for FullScreen and CompactOverlay. | Closed for the mapping itself. It now lives in `ResolvePresenterKind`, and `ResolvesPresenterKindWithoutQueryingWhenFeatureIsOff` and `ResolvesPresenterKindFromSizingSupportWhenFeatureIsOn` cover both inputs and the zero-query path. Still untested: that `DesktopWindowImpl` passes those two specific inputs at its two call sites, which needs a real window with a non-overlapped presenter. |

Accepted as correct during this review, for the record: the `RestoreToArranged` branch in
`TryReconcile` is unreachable from a real `GetPlacement`, which only sets `RestoreToMaximized`,
`Arranged`, `AllowSizing`, `FullScreen`, and `VirtualDesktopId`. The code and the design note both
say so, and the branch is covered by a synthetic value, so it is defensive, not dead. The union of
`NonRequestFlags` and the adapter's mapped flags covers every declared `PlacementFlags` value, so no
engine flag can silently reach `TryReadEnginePlacement` and fail it.

#### Capture review fixes (2026-09-24)

The three findings and three minor items above are addressed:

| Area | Implementation |
| --- | --- |
| Close and retirement | `Shutdown()` detaches the cache. A cancelled `WM_CLOSE` no longer changes capture lifetime. Detach retains the bound HWND and rejects recapture of it, including after repeated detach calls. A successful capture of a different HWND starts a new binding and invalidates the old token. |
| Failed capture | Translation and reconciliation finish before publishing the snapshot, binding, or token. A failed capture of a different HWND no longer discards the existing snapshot or desktop id. |
| Exception boundary | `TryCapture`, `CaptureEffectivePlacement`, and scheduling are `noexcept`. This takes the review's fail-fast alternative: allocation failure terminates rather than unwinding through a native callback. It does not promise recovery from allocation failure or enable exceptions in the product library. Ordinary capture failures still return false without changing the cache. |
| Coalescing | `ScheduleWindowStateUpdate` shares the former restored-size callback and lifetime sentinel across `WM_MOVE` and `WM_SIZE`. One pending callback captures placement and, when requested, updates restored client size. Drag completion still captures synchronously. Missing or rejected dispatch falls back to synchronous capture; enqueue rejection is logged. |
| Callback lifetime | `Shutdown()` disarms the shared sentinel regardless of the new-windowing feature switch. Capture also runs with that switch off, so its callbacks need the same teardown protection. |
| Normalization | Normal aliases include `SW_SHOWNA`. Failure to enter either physical-coordinate DPI scope rejects capture. A maximized restore flag takes precedence over an arranged restore flag. |

`ResolvePresenterKind` retains the existing mapping to `AppWindowPresenterSupportsSizing` and
short-circuits the presenter query when the new APIs are disabled. Unit tests cover both enabled
results and the disabled zero-query path. The dedicated capture class also covers same-HWND
retirement, failed replacement without cache/token changes, normal show aliases, and conflicting
restore-state reconciliation.

The runtime and isolated placement test project build in `amd64chk`. All 97 placement tests and
seven existing presenter/deferred-sizing integration tests pass on the local VM. The integration
selection includes the simulated drag-end case and real FullScreen/CompactOverlay transitions.
Those tests protect the shared sizing behavior; they do not expose the private placement cache.
Native alias tests can encounter commands already normalized by Windows. DPI-context failure and
allocation failure were not fault-injected.

Public capture and saving must capture synchronously when they need current geometry, not assume
that a queued callback has run. Storage-backed accepted/cancelled-close coverage remains part of
step 6. The full-screen hidden-application gate and public projections remain separate work.

Related open work: [PR #11974](https://github.com/microsoft/microsoft-ui-xaml/pull/11974) changes
closed-window sizing in the same implementation files. Keep its accepted-close size snapshot and
closed-state guard when integrating these changes; this capture fix does not replace that work.

#### Code review of the coordinator production wiring (2026-09-24)

Reviewed commits `4c2aa5c53`, `3d07f3a5c`, and `d362c7900`: the coordinator lifecycle,
`DesktopWindowImpl`'s `IWindowPlacementCoordinatorHost` implementation, and the packaged save path.
Three defects were found and fixed during the review. The remaining gaps are pre-existing step 5
and step 6 work, listed below and sent back to the Developer.

| Finding | Detail | Fix |
| --- | --- | --- |
| `Display` never reported success | `ShowWindow` returns the window's *previous* visibility, so the first display always returned `FALSE`. `EndFirstDisplayOperation` treated that as a pre-display abort, so the phase never closed, the window never enrolled, no save could ever happen, and `Show`/`Activate` returned false to the app. | `Display` now calls `ShowWindow` and reports the resulting state through `IsVisible()`. It also null-checks the HWND, which `IsVisible` already did. |
| Saves used the enrolled id | The host wrote to `m_enrollmentId`, the id captured at first display. The [save sequence](#save-sequence) requires a fresh id and Boolean snapshot per attempt, and an enrolled window is allowed to save to a different id. | The host snapshots `m_persistPlacementId` and `m_useAutomaticPlacementPersistence` itself, and skips the attempt when the id is empty. `IWindowPlacementCoordinatorHost::TryCapturePlacementAndSave` now takes no arguments, so the enrolled id cannot be reintroduced as a write destination. The coordinator still gates on enrollment. |
| A failed save blocked the backstop | `m_saveAttempted` was set before the write, so a failed accepted-close save suppressed the destruction backstop. The [lifecycle hooks](#lifecycle-hooks) require marking a save complete only after a successful write. | Renamed to `m_saveCompleted` and set only on success. A completed save is still never repeated. New test `FailedCloseSaveIsRetriedByTheDestroyBackstop`. |

Two smaller changes: the save now calls `CaptureEffectivePlacement()` before reading the cache, so
it does not depend on a queued state-update callback having run, matching the synchronous-capture
rule above. An added `try`/`catch` was removed after it failed to compile: `DesktopWindowImpl.cpp`
is built without exceptions, and this file follows the same fail-fast-on-allocation-failure
convention as the rest of the capture work.

Still open as of this review, and not introduced by these commits. Each row is closed by later
work; see the note that follows this one.

| # | Gap | Owner |
| --- | --- | --- |
| 1 | `ApplyPlacement` is a stub returning false, so no placement is ever applied natively. | Step 5 / 2C |
| 2 | `ShowImpl(options)` and `TryApplyInitialPlacementImpl` ignore `WindowShowOptions`, so `SkipInitialPlacement`, `DoNotActivate`, `Reason`, and `CascadeBehavior` are silently dropped. | Step 5 |
| 3 | `Show`, `Activate`, and `TryApplyInitialPlacement` do not route through the coordinator's `Show`/`Activate` entry points, so the first-display pass never runs from ordinary app code. | Step 5 |
| 4 | `NotifyNativeDisplayed` has no production call site, so a native or `AppWindow` display does not end the phase. | Step 5 |
| 5 | No `WM_DESTROY` or `WM_ENDSESSION` hook. Only `CloseImpl`, `Shutdown`, and the destructor save. | Step 6 |

Validation: `wrtdxamlfoundation.vcxproj` and the isolated placement project build in `amd64chk`.
All 130 `*WindowPlacement*` tests pass on the VM with none failed, blocked, or skipped. No window
round-trip or storage-backed lifecycle coverage exists yet; that remains step 7.

#### Display routing through the coordinator (2026-09-25)

Closes gaps 2, 3, and 4 above: options are read, the three entry points route through the
coordinator, and a native display ends the phase. Gap 1 is also closed, by the separate native
application work. `DesktopWindowImpl::ApplyPlacement` is no longer a stub. It applies pending
client sizing, supplies a deferred peer anchor, and calls
`WindowPlacementPersistence::TryApplyPlacement`, which reaches `PlacementEx::SetPlacement`. See
[native application status](#native-application-status-2026-09-24). Gap 5 is closed by the
step 6 `WM_DESTROY` and `WM_ENDSESSION` hooks.

| Change | Detail |
| --- | --- |
| `Display` learns the operation | `IWindowPlacementCoordinatorHost::Display` now takes the existing `CoordinatorOperation` plus `doNotActivate`. Show and Activate have different native behavior, and the host had no other way to tell them apart. `TryApplyInitialPlacement` never displays, so it never reaches this callback. |
| `Show`/`Activate`/`TryApplyInitialPlacement` route through the coordinator | `ShowImpl()` forwards to `ShowImpl(nullptr)`. All three entry points now call the matching coordinator method and then surface any native failure through a new `m_lastDisplayResult` member, because `Display` returns `bool`, not `HRESULT`. |
| Options are read, with phase-dependent validation | `CopyShowOptions` QIs the sealed `WindowShowOptions` and reuses its existing `CopyInitialRequest`, which validates. After the phase closes, only `DoNotActivate` is read and the ignored fields are not validated. |
| `TryApplyInitialPlacement` checks state before validating | The spec requires the displayed / in-progress check to come first, so a late call returns `false` instead of throwing `E_INVALIDARG`. |
| `WM_SHOWWINDOW` ends the phase | A native or `AppWindow` display now calls `NotifyNativeDisplayed`. The hook filters on `lParam == 0`, which means an explicit `ShowWindow` call rather than an owner-driven change. The coordinator's in-progress guard makes the message sent by its own `Display` a no-op. |
| A later `Activate` is not a no-op on a visible window | The post-phase `Activate` path used to early-return when the window was already visible. Per [Control display and activation](../../specs/window-placement-persistence-spec.md), a later `Activate()` still restores a minimized window and requests activation, so it always runs the display step. `Show` keeps its visible-window no-op. |
| The legacy `ShowImpl` error path is gone | It treated `ShowWindow`'s previous-visibility return as a failure. Routing through the coordinator makes that path unreachable; `Display` now reports success through `IsVisible()`. |

Validation: both projects build in `amd64chk`. The new `WindowPlacementDisplayRoutingTests` class
adds 10 tests covering first Show, first Activate, `SkipInitialPlacement`, `DoNotActivate`, later
Show on hidden and visible windows, later Activate on a visible window, the hidden-application
path, a nested `NotifyNativeDisplayed`, a native display before any framework call, and retry after
an aborted display. All 140 `*WindowPlacement*` tests pass on the VM.

The API spec owns public behavior, declarations, application examples, and material intended for
learn.microsoft.com. These notes own storage mechanics, serialization, native integration, and the
implementation work needed to satisfy that behavior. If the documents disagree, update these notes
to match the API spec; do not infer a new API decision from an implementation sketch.

- [Implementation boundaries](#implementation-boundaries)
- [Storage identity and layout](#storage-identity-and-layout)
- [Loading placement data](#loading-placement-data)
- [Capturing data for persistence](#capturing-data-for-persistence)
- [Saving placement data](#saving-placement-data)
- [Retaining the 32 most recently saved placements](#retaining-the-32-most-recently-saved-placements)
- [Private record format](#private-record-format)
- [PlacementEx integration](#placementex-integration)
- [Automated testing](#automated-testing)
- [Diagnostics and implementation gates](#diagnostics-and-implementation-gates)

## Implementation boundaries

Keep the following responsibilities separate. These are logical components, not proposed public
types or committed source-file names.

| Component | Responsibility |
|---|---|
| Window coordinator | Own initial-operation snapshots, reentrancy, persistence enrollment, visibility history, and save hooks |
| Capture adapter | Produce a coherent placement snapshot from the HWND and WinUI's state caches |
| Store adapter | Resolve packaged application identity, derive names, read or replace complete values, and enforce placement retention |
| Record codec | Translate between validated placement data and a bounded, versioned byte representation |
| Placement adapter | Translate to PlacementEx, apply current-window policy, and report actual results |

The store adapter must not move windows. The codec must not query current monitors, activate WinRT
storage objects, or depend on C++ structure layout. The placement adapter must not choose a storage
identity or save data as a side effect.

Use the spec's [initial-operation pipeline](../../specs/window-placement-persistence-spec.md#loading-applying-and-displaying),
[save eligibility](../../specs/window-placement-persistence-spec.md#know-when-placement-is-saved),
and [data-validity rules](../../specs/window-placement-persistence-spec.md#data-validity) rather than
maintaining another copy of those contracts here.

## Storage identity and layout

### Resolve the packaged application, not just its package

Use `Microsoft.Windows.Storage.ApplicationData.GetDefault()` and `LocalSettings` as the proposed
backend. Access the Windows App SDK storage implementation internally; do not add its types to the
public XAML metadata or expose a placement-specific store override.

The default packaged store supplies current-user and package-family scope. It does not distinguish
two registered applications in the same package. Resolve the registered application identity with
`GetCurrentApplicationUserModelId` and add an application namespace within that store, as required by
the spec's [identity rationale](../../specs/window-placement-persistence-spec.md#why-use-application-identity-rather-than-package-family-alone).

Use the registered identity, not the process's explicit shell grouping id, executable path, window
title, process id, or versioned package full name. Processes of the same registered application
must derive the same namespace across package version and architecture changes.

The current API spec limits automatic storage to packaged apps. The intended extension is to use a
Windows App SDK default store for unpackaged apps too, as described below. Until that dependency and
the corresponding WinUI contract are ready, unpackaged capture, explicit placement, and cascading
remain independent of automatic storage. Do not derive publisher/product strings from an AUMID.

Failure to resolve a packaged application's identity makes storage unavailable for that operation.
It must not select an unpackaged namespace or a package-wide fallback. A missing application-scoped
record must not cause a read from an older package-wide prototype slot.

### Wanted dependency: unpackaged default ApplicationData

We still want a Windows App SDK API that lets a framework obtain the process's default
`ApplicationData` without knowing whether the app is packaged or how an unpackaged app chose its
storage identity. The intended flow is:

1. The unpackaged app establishes its default storage identity through Windows App SDK before a
   framework first needs storage.
2. WinUI queries the default `ApplicationData`, ideally through `GetDefault()`, and uses
   `LocalSettings` without a XAML-specific store setter.
3. The app owns the unpackaged identity and its data-cleanup lifecycle. WinUI does not infer identity
   from an executable, window class, or shell AUMID.

The earlier name `SetDefaultForUnpackaged(publisher, product)` illustrates possible setup, not a
committed API. The required capability is framework discovery of the configured default; the final
setup/query shape, availability, and failure contract need Windows App SDK agreement.

`GetForUnpackaged(publisher, product)` is related but insufficient by itself: it requires the caller
to already know the app's publisher/product values and does not register the result as the process
default. WinUI must not invent those values or add a competing public storage-configuration API.

Before enabling automatic unpackaged persistence, settle default initialization timing, repeated or
late configuration, thread safety, missing-default behavior, and runtime/deployment requirements.
The storage layer must also provide enough stable identity/scope to keep app data separate across
processes and users without borrowing the cascade feature's shared unpackaged namespace. The
application-digest container below describes the packaged path; an unpackaged namespace is not
implicitly derived from a nonexistent registered packaged application identity.

Apply the same resolved default and namespace rules to automatic loading, detached loading, and
saving. Decide how configuration interacts with cached successful store resolution so a window
cannot load from one store and silently save into another. Record those decisions in the API spec
before broadening its packaged-only contract. If the dependency is unavailable, retain that
packaged-only behavior rather than dropping the unpackaged-default-store goal.

### Related Windows App SDK work

A quick PR and source check on **2026-09-23** found:

| PR | Status at review | Relevance |
|---|---|---|
| [#6277: ApplicationData unpackaged support](https://github.com/microsoft/WindowsAppSDK/pull/6277) | Merged 2026-04-01 | Implements unpackaged support through `GetForUnpackaged(publisher, product)`; does not supply the configured process default |
| [#6378: Packaged/unpackaged implementation refactor](https://github.com/microsoft/WindowsAppSDK/pull/6378) | Merged 2026-05-27 | Separates implementation types; not a default-store registration API |
| [#6558: Unpackaged LocalSettings path fix](https://github.com/microsoft/WindowsAppSDK/pull/6558) | Merged 2026-06-12 | Fixes the explicit unpackaged store's backing path; not default-store discovery |
| [#6731: ApplicationData path optimization](https://github.com/microsoft/WindowsAppSDK/pull/6731) | Open | Related maintenance, not the missing setup/query capability |

The inspected [ApplicationData IDL](https://github.com/microsoft/WindowsAppSDK/blob/main/dev/ApplicationData/ApplicationData.idl)
still documents package identity as a requirement for `GetDefault()`. Its
[implementation](https://github.com/microsoft/WindowsAppSDK/blob/main/dev/ApplicationData/M.W.S.ApplicationData.cpp)
resolves the current package family, while `GetForUnpackaged` constructs an instance for explicitly
supplied publisher/product values. The search did not find a PR adding the wanted unpackaged default.
This is a dated search result, not proof that no differently named proposal exists or that a merged
change is available in a particular released package.

### Proposed packaged container and value names

Use this logical `ApplicationData` layout:

```text
LocalSettings
  Microsoft.UI.Xaml.WindowPlacement
    app1_<application-digest>
      wp1_<readable-slug>_<placement-id-digest> = <base64 record>
```

The application digest is `Base32(SHA-256(UTF-16LE(registeredApplicationId)))`. The placement-id
digest is `Base32(SHA-256(UTF-16LE(rawPersistPlacementId)))`. Separate containers keep the two
identity components unambiguous without concatenating arbitrary strings into a compound key.

Hash the exact UTF-16 code units, with no trailing terminator, case folding, or Unicode
normalization. Use the `HSTRING` length rather than a NUL-terminated string function. Preserve the
id's ordinal, case-sensitive semantics even if the settings backend compares names without case.
Use a platform SHA-256 implementation, not a process-specific string hash or a private hash algorithm.

Base32 uses the uppercase RFC 4648 alphabet without padding. A SHA-256 digest produces 52 characters.
Base64 and Base64url are not suitable for these names because their alphabets distinguish uppercase
and lowercase characters. They remain suitable for a settings value, whose contents are not a key.

The readable slug scans the raw placement id from left to right, keeps only ASCII `[A-Za-z0-9]`,
and stops after 16 accepted characters. Use `id` when none are accepted. For example,
`DocumentWindow:doc42` produces `DocumentWindowdo`. The complete value name is at most 73 characters.
The digest provides uniqueness; the slug is only a diagnostic aid and reveals part of the id.
Hashing does not make the name confidential or authenticate the record.

These namespaces prevent accidental collisions through the placement APIs. They do not create an
access-control boundary between applications that share the package's app-data store.

### Storage access and lifetime

Resolve storage only when a load or eligible save needs it. Property setters perform no storage I/O.
An explicit placement or selected live peer can avoid an automatic read while still allowing a
later eligible save.

Carry the resolved application namespace and id-derived value name in an immutable operation
context. An id change during capture or a callback must not redirect an operation already in
progress. A later save builds a context from its own Boolean/id snapshot.

Successful identity and naming results can be cached. Any cached storage interfaces must obey their
threading and lifetime rules; an agile public placement object does not make a storage interface
agile. Do not share an apartment-bound handle across UI threads or detached-load callers.

The older design cached even "no usable store" for the window's lifetime. Do not adopt that as a
public guarantee. Treat unavailability as an operation result; settle any retry or negative-cache
policy during backend integration rather than permanently disabling saves after a transient read
failure.

Reading a missing container or value returns no record without creating settings containers. Create
containers when writing. Do not delete or repair a record as a side effect of loading it. A later
eligible successful save can replace unreadable data.

Use `ApplicationData` for access and app-data lifetime. Do not depend on the backing registry path,
open a package's `settings.dat` directly, or document the private container as an app-facing reset
API. Changing an id neither deletes nor migrates its old record.
Subsequent successful saves can evict old records under the
[retention policy](#retaining-the-32-most-recently-saved-placements).

## Loading placement data

### One reader for automatic restore and detached loading

Both automatic loading and `WindowPlacement.LoadForPersistPlacementId` should use the same identity
resolver, value-name derivation, decoder, and structural validation. The shared reader accepts a
non-empty id snapshot and returns either an immutable saved snapshot or a categorized failure.

1. Resolve the packaged application namespace and default store.
2. Look up the application container and value without creating either.
3. Require a string value and reject an oversized encoded value before allocating a decoded buffer.
4. Base64-decode, validate the envelope, and parse bounded TLV fields.
5. Convert wire rectangles and state into a complete placement snapshot with checked arithmetic.
6. Validate the snapshot against its saved coordinate environment, not today's monitor topology.
7. Return saved data without display adjustment, peer discovery, native application, or a write.

Keep at least these internal outcomes distinct:

| Outcome | Meaning |
|---|---|
| Loaded | One valid, coherent saved snapshot |
| Missing | The application container or value does not exist |
| Unavailable | No supported identity/store, or access denied, under the phase-specific rules below |
| Invalid or unsupported | Wrong value type, invalid encoding/data, or unsupported major version |
| Unexpected failure | A failure outside the preceding categories; retain the original failing HRESULT |

Automatic restore treats storage failure as no usable stored source and continues according to the
spec. The detached API returns null for its documented missing, invalid, unsupported, and
unavailable cases. It propagates unexpected failures with the original HRESULT and normal
C#/C++/WinRT exception mapping. Do not erase that distinction with a blanket catch-and-return-null
implementation or replace a known error with `E_FAIL`.

**PM error decision (2026-09-24):** Apply this bounded classification at the boundary that knows
the operation. Do not classify every failed HRESULT as unavailable.

| Boundary/result | Classification |
|---|---|
| Packaged identity query reports `APPMODEL_ERROR_NO_PACKAGE` or `APPMODEL_ERROR_NO_APPLICATION`; supported capability detection reports no default store | Unavailable |
| Identity/store access reports `E_ACCESSDENIED` or the equivalent Win32 access-denied code | Unavailable; retain the normalized HRESULT for diagnostics |
| Container/value lookup reports absence through its documented lookup result | Missing; do not create a container or treat unrelated lookup errors as absence |
| A successfully read value fails type, size, encoding, version, or structural validation | Invalid or unsupported; no record repair or deletion |
| Any other failing identity, storage, decoder, or public-value construction operation | Unexpected; preserve the HRESULT, including `E_OUTOFMEMORY`, `E_FAIL`, activation/deployment errors, and `RO_E_CLOSED` |

Normalize a Win32 error with `HRESULT_FROM_WIN32`; retain an existing failing HRESULT unchanged.
For example, the identity query's expected buffer-size probe is not a failure. Failure of runtime
class activation is not proof that the product lacks a supported default-store capability.
Malformed data is distinct from failure to allocate its decode buffer. Expected-unavailability
classification must be explicit and covered by backend tests, not inferred from arbitrary
exception text or a broad catch.

Extend the private backend and `LoadResult` boundary to carry a status plus error code. Also replace
the identity helper's error-discarding Boolean boundary for the shared reader. A successful/absent
result need not invent an error code; `Unexpected` must carry a failing HRESULT. If an internal
failure genuinely has no code, use `E_FAIL` and diagnose that category rather than returning null.
Allocation failures must become `E_OUTOFMEMORY`, not `Unavailable` or invalid data, and must not
escape a `noexcept` boundary. These changes are required implementation work: today's
`StorageResult` enum, `LoadResult`, and `TryGetPackagedApplicationId` cannot preserve this contract.

Report failures once at the operation boundary using the diagnostics below. Returning a failure
does not grant a read path permission to mutate storage, enrollment, caches on a window, or phase
state. Do not copy arbitrary backend exception text into diagnostics or newly originated public
error messages; preserving the HRESULT does not require preserving that text.

The detached API can run without a window and from any thread. It must not consult a window's
enrollment, end an initial-placement phase, or invoke the placement adapter to "validate" data by
moving an HWND. Each successful call creates an independent public value from the decoded snapshot.

### Where the reader fits in initial placement

The coordinator snapshots and validates the request, supplied placement, window Boolean, and id
before callbacks. The initial-placement phase runs from window creation until first display.
Hidden application is repeatable inside that phase and does not end it; the first display runs at
most one automatic placement pass and always ends the phase. The coordinator records enrollment at
first display. There is no retained `InitialShowOptions` property in this design.

A first `Show` with `WindowShowOptions.SkipInitialPlacement = true` skips the whole pass: no
storage read, no peer selection, no cascading, and no fallback repositioning. It still ends the
phase, so there is no deferred restoration to run later.

Invoke the reader only after the spec's explicit-placement and eligible-peer selection steps have
left storage as the next permitted source. In particular, opt-out suppresses automatic storage
access, explicit placement does not trigger a speculative read of the saved slot, and a skipping
first `Show` reads nothing at all.

Once a selected source enters native application, failure does not cause another storage lookup
within that pass. Keep source selection separate from application so a partially applied peer or
explicit value cannot accidentally fall through into an automatic restore.

Use a working copy for reason policy, cascading, monitor migration, and current-window constraints.
Do not modify the decoded snapshot, caller-owned placement, or stored record. Applying placement,
including successful hidden application, is not a save operation.

The coordinator should track the placement phase, enrollment, and save eligibility separately:

| State | Implementation purpose |
|---|---|
| Phase-open flag and in-progress guard | Prevent reentrant work and a second automatic pass after first display |
| Enrollment snapshot taken at first display | Record the fixed opt-in decision; later Boolean/id changes gate saves but never create or remove enrollment |
| Ever shown | Record actual display, including direct native/AppWindow display |
| Selected-placement application outcome for the current attempt | Return value and diagnostics only; discard after the attempt, without enrolling or granting save eligibility |
| Close-save completion | Prevent a completed normal-close save from being repeated by destruction |

Direct `AppWindow`/native first display ends the phase and updates visibility history without
creating enrollment. Hidden application does the opposite: it can change the window without ending
the phase and without enrolling it. Do not collapse these states into a single "initial activation
happened" flag.

Late Boolean/id setters store the new values without I/O. Before first display, the next initial
operation reads them; after first display, they only gate future saves on an enrolled window.
An initially empty id, explicit `false`, or direct native first display means no later enrollment.
Neither a store cache nor a save hook may treat a later non-empty id as enrollment. Store availability
at first display is not an enrollment condition. A display attempt that aborts before any display
creates no enrollment; see the pre-display decision below.

### Coordinator ownership and operation boundaries

**Architect handoff (2026-09-23):** The current codec, store, and placement-policy units do not own
phase or enrollment state. No existing helper needs a consumed-opportunity flag migrated. Step 5
should add one private coordinator owned by `DesktopWindowImpl`, rather than independent lifecycle
flags in the public projection, store, and native adapter.

Keep the implementation split along these boundaries:

| Owner | Responsibility |
|---|---|
| Public projection | Copy public options and placement into private values; preserve the spec's API-use and argument-error mappings |
| Window coordinator | Classify the call, guard reentrancy, own phase and first-display enrollment, and choose whether to run placement |
| Per-call operation context | Hold immutable request/id/Boolean snapshots, selected source, working placement, and application outcome |
| Placement policy and native adapter | Adjust and apply one selected snapshot, then capture effective results; do not enroll or end the phase |
| Capture cache and save hooks | Retain effective window state; require enrollment and actual display before an eligible save |

The coordinator must classify late and reentrant calls before the projection validates placement
fields. The public boundary first rejects null options; this required-argument check is not
placement-field validation. With non-null options, `TryApplyInitialPlacement` checks thread
and closed-window errors, then returns `false` for an already-displayed window or an in-progress
operation without validating ignored
fields. A later `Show` reads only its applicable display options, including `DoNotActivate`;
it must not reject a stale placement object or an invalid ignored reason/cascade/skip combination.
For an eligible initial call, snapshot and validate before storage, peer discovery, native changes,
or enrollment. Scope the in-progress guard to the whole operation, including display callbacks,
and release it on every exit.

`SkipInitialPlacement` belongs at the coordinator's placement-pass boundary, not in
`PlacementRequest` or `NativeApplyOptions`. After initial validation, a skipping `Show` goes
directly to display with the validated activation policy. It must not call the source selector,
store reader, geometry policy, placement adapter, or fallback repositioning path. Ordinary pending
`Width`/`Height` sizing still follows the spec. Validate reason and cascade enum values even when
skipping; reject non-null placement with skip, and reject skip on an eligible hidden-application
request. `ApplicationRestart` still suppresses activation on a skipping first `Show`.

Hidden application creates a fresh operation context on every call. Do not retain its options,
source choice, enrollment decision, or `ApplyOutcome` for the next call. In particular,
`AllowsSourceRetry` limits retries **within one placement pass**; a previous hidden attempt that
reached native application cannot suppress source selection in the next hidden attempt or first
non-skipping display. Effective geometry belongs in the capture cache, not in a cached request
that is replayed later.

On a framework first-display operation, keep its snapshotted Boolean/id enrollment decision
separate from actual visibility history. A skip, missing source, or failed placement application
does not change that decision. Direct `AppWindow`/native first display records display and closes
the phase without creating enrollment; later framework calls cannot retroactively enroll it.
Save hooks use the current Boolean and id snapshot, not the first-display configuration as a
permanent save permission or destination. A late `false` suspends saving without clearing enrollment.

**Pre-display failure decision (PM, 2026-09-24):** The spec's operation sequence previously said
to end the phase and record enrollment before callbacks, while its lifecycle tables end the phase
only on actual display. The spec now uses the lifecycle-table rule everywhere. Keep the in-progress
guard, enrollment snapshot, and actual-display evidence distinct in the coordinator:

| State | Set by | Cleared or fixed by |
|---|---|---|
| In-progress guard | Entering a display or hidden-application operation | Every exit from that operation, including aborts |
| Initial-placement phase | Open from construction | Closed only by actual display, framework or native |
| First-display enrollment | Actual framework display, from that operation's snapshot | Never; a late `false` suspends saving instead |
| Actual-display evidence | The host reporting a real display | Never |

A validated display operation that aborts before any display releases the guard and leaves the
phase open. It discards its request, Boolean, id, source, and enrollment decision. If the window
survives and is still hidden, the next `Show` or `Activate` is a complete initial pass with fresh
snapshots; the coordinator must not carry a consumed-opportunity flag or a cached request across
that boundary. Once the host reports an actual display, the phase is closed even if the operation
then fails. A never-displayed window is ineligible to save either way. See
[Aborting before display](../../specs/window-placement-persistence-spec.md#aborting-before-display)
and
[Why a pre-display abort can retry](../../specs/window-placement-persistence-spec.md#why-a-pre-display-abort-can-retry).

Step 5's dedicated coordinator tests should observe collaborator calls as well as final geometry:

| Sequence | Required observation |
|---|---|
| Hidden apply, then hidden apply with different options | Two independent passes; phase remains open; no enrollment or save |
| Hidden apply succeeds or partially fails, then first non-skipping `Show` | Fresh source selection and policy; previous outcome does not suppress the new pass |
| Hidden preparation, then opted-in first `Activate` | Default automatic pass still runs per the decided contract; no implicit skip or remembered options |
| Hidden preparation, then skipping first `Show` | No selector, read, adjustment, cascade, or placement-application calls; display and ordinary enrollment still occur |
| Invalid initial skip combination or enum | Argument error before collaborators run; no display, phase change, or enrollment |
| Eligible hidden apply with skip | Argument error even with null placement; a late or reentrant hidden request instead follows its early-false rule |
| Hidden apply, then direct native display, then framework display | Phase ended by native display; no later automatic pass or enrollment |
| First display, hide, then `Show` with invalid ignored placement fields | No placement validation or replay; only applicable display policy is used |
| Hidden apply, then close without any display | No save regardless of hidden-application success |
| Validated first `Show` whose callback aborts display while the window survives | Guard released; phase still open; no enrollment; a following `Show` runs a fresh full pass |
| Same abort, then close while still never displayed | No save |

Use injected selector/store/native collaborators for call counts and dedicated real-window tests
for visibility, reentrancy, and enrollment timing. The existing policy tests cannot establish
these lifecycle guarantees. The pre-display abort expectations above follow the PM decision in
this note. The separate `Activate` discoverability and hidden-only saving questions are now
decided: see
[Why Activate() gets no skip affordance](../../specs/window-placement-persistence-spec.md#why-activate-gets-no-skip-affordance)
and
[Why a never-displayed window never saves](../../specs/window-placement-persistence-spec.md#why-a-never-displayed-window-never-saves).
The "Hidden preparation, then opted-in first `Activate`" and "Hidden apply, then close without
any display" rows above are the corresponding required observations, and the debug diagnostic for
the `Activate` case must not change either of them.

## Capturing data for persistence

### Maintain a coherent overlapped snapshot

The capture adapter supplies both public capture and saving. Start with current overlapped geometry
from `PlacementEx::GetPlacement`, then reconcile it with WinUI's tracked placement state. Run native
capture in a coordinate context that produces physical screen pixels; the helper can otherwise
return DPI-virtualized geometry.

Maintain the last meaningful show state independently from HWND visibility. Hiding alone must not
replace maximized, minimized, or snapped state with `SW_HIDE`. A hidden overlapped window can still
move, resize, or change state, so combine its current geometry with current meaningful state rather
than replaying an old entire snapshot solely because it is hidden.

WinUI must also track minimized-from-snapped history. The helper captures current arranged state
and restore-to-maximized information but cannot reconstruct every restore-to-snapped history from
an already minimized HWND.

Before entering full-screen or compact overlay, retain a complete valid overlapped snapshot:
normal and snap rectangles, show/restore state, monitor work area, DPI, and device name. The earlier
position-and-size-only cache is insufficient. A maximized window entering full-screen must retain
both its maximized state and normal restore bounds. Do not combine presenter bounds with stale
overlapped monitor metadata.

Use the spec's [capture selection rules](../../specs/window-placement-persistence-spec.md#capture-change-and-supply-placement)
when choosing current geometry or that cached snapshot. If meaningful state required for a valid
snapshot is unavailable, fail capture rather than fabricate a record. Initial valid overlapped
geometry can use `Normal` before any meaningful show state exists; that alone does not enable saving.

After application, capture the actual resulting placement. The mutable PlacementEx argument is not
a reliable post-apply snapshot: backend paths can mutate it differently, and partial application can
leave a state other than the requested state. Do not cache a requested special state as if it were
successfully applied.

### Cache optional virtual-desktop identity safely

Virtual-desktop queries can call into the shell through COM. The imported helper documents failures
before taskbar registration and from input-synchronous messages, as well as potential blocking.
Do not make a synchronous shell query from `WM_ENDSESSION` or another unsafe window-message path.

Use the existing `CaptureFlags::SkipVirtualDesktopId` option for those captures and overlay the last
safely cached non-empty desktop id. If no valid cached id exists, omit it. Geometry capture and saving
must not depend on obtaining the optional id.

Schedule a safe query after first display and refresh at suitable later opportunities. Correlate
completion with the live window's lifetime, not just its numeric HWND, so late results cannot attach
to a closed or reused window.

`DesktopWindowImpl` schedules the query with a **posted registered window message**, not a helper
thread:

| Choice | Reason |
| --- | --- |
| Posted, not sent | The documented shell failure is `RPC_E_CANTCALLOUT_ININPUTSYNCCALL`, which only happens inside an input-synchronous call. A posted message is dispatched by the window's own pump, so it is a safe call-out point. |
| Registered message | Avoids colliding with anything the app posts to the same window. |
| After display, not inline | The shell commonly fails with `TYPE_E_ELEMENTNOTFOUND` until the taskbar knows about the window. |
| No helper thread | Removes the COM apartment and thread-lifetime work a background query would need. The UI thread already meets the helper's apartment requirement. |

Scheduling happens on both display paths: the framework `Display` call and the native `WM_SHOWWINDOW`
path. `WM_SHOWWINDOW` arrives before the window reports itself visible, so that path states that a
display is happening instead of asking. Re-showing a window re-schedules, which is how the id is
refreshed later. A single display reaches both paths, so a pending-post flag coalesces them into one
shell query and is cleared when the message is handled.

The query is paid for only by windows that can actually use it. The id is only ever read back out of
a saved record, and every save path requires enrollment, so the handler drops the refresh for a
window that never enrolled. Enrollment is recorded after the first `Display` returns, so this is
checked when the posted message runs rather than when it is posted.

Three seams keep this testable without the shell:

- `ShouldScheduleVirtualDesktopRefresh(isClosed, displayed)` is the scheduling decision. It
  deliberately ignores the lifetime token, because the cache may not be bound yet at display time and
  the token is what correlates the *result*, not the request.
- `ShouldRunVirtualDesktopRefresh(isClosed, hasEnrollment)` is the decision made when the posted
  message arrives, after enrollment has settled.
- `TryRefreshVirtualDesktopId(cache, hwnd, query)` takes the lifetime token **before** running the
  injected query and only then calls `TrySetVirtualDesktopId`. A failed query, a zero token, or a
  window retired mid-query all end with the placement unchanged and still saveable.

If nothing has captured when the message arrives, the handler captures first so a live token exists
for the result to attach to. That capture is behind the enrollment check too, so a window that never
persists placement is not forced to capture on display.

Loading retains the saved optional id without contacting the shell. Whether application uses it
belongs to [reason policy](../../specs/window-placement-persistence-spec.md#choose-launch-or-restart-policy),
not the codec or store adapter.

## Saving placement data

### Lifecycle hooks

The current [DesktopWindowImpl](../../dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp) provides these
integration points. The persistence hooks still need to be implemented.

| Trigger | Proposed hook and ordering |
|---|---|
| Accepted framework close | In `CloseImpl`, after `Closed` handlers return and cancellation is checked, before `PrepareToClose`, content removal, or `Shutdown` |
| Native destruction bypassing normal close | At the start of `OnMessage` handling for `WM_DESTROY`, before the last-window `PostQuitMessage` early return |
| Confirmed session end | On `WM_ENDSESSION` with nonzero `wParam`, while valid geometry or tracked placement remains available |
| Cancelled close or cancelled session end | No write for that request |

`WM_CLOSE` reaches `CloseImpl` through `OnClosed`; `Window.Close()` also uses the framework close
path. Handlers can change live placement, the Boolean, or the id. Snapshot only after accepted-close handlers
finish so those changes affect the save.

`AppWindow.Destroy()`, external destruction, or teardown can bypass accepted close. The destruction
hook is a best-effort backstop, not permission to postpone the ordinary save until the HWND and
XAML state are already dismantled.

Keep normal-close/destruction deduplication separate from session-end handling. Mark a normal-close
save complete after a successful write; a failed attempt must not masquerade as saved data. A
backstop may still attempt a write if usable state remains. Do not suppress a later accepted-close
save merely because session end already wrote: shutdown can terminate the process before normal
teardown, and a redundant small write is preferable to relying on a later message.

Hide, move, resize, placement application, and capture do not write storage. They can update the
state caches needed by a later save. There is no continuous-save or crash-recovery mechanism here.

### Save sequence

1. Check enrollment and the spec's visibility requirement: the window must have been displayed at
   least once. Hidden placement application alone is insufficient, whether it succeeded or failed.
2. Snapshot the current Boolean and id together before capture or storage access, after accepted-close
   handlers where applicable. Skip this attempt unless the Boolean is `true` and the id is non-empty.
   Keep both snapshots for the entire attempt; subsequent edits cannot redirect or cancel it.
3. Capture one coherent snapshot while the HWND and WinUI's tracked placement are valid. Use the
   message-safe virtual-desktop path when required.
4. Validate the complete snapshot and translate it into WinUI-owned wire state. Do not serialize
   native flags wholesale or save presenter geometry as overlapped geometry.
5. Resolve the supported application/store context and derive the value name from the id snapshot.
   Acquire the bounded per-namespace save/cleanup coordination described below. Create missing
   private containers only at this write stage.
6. Assign the next save sequence, then serialize and base64-encode the complete record in memory with
   checked sizes. Publish placement and sequence together, not in separate settings writes.
7. Replace one settings value with the complete string. Record the result and update the relevant
   close-save completion state only on success.
8. After a successful replacement, trim excess placement records under the same coordination, then
   release it. A cleanup failure does not turn the completed placement write into a failed save.

An already enrolled window can save to a different id. Changing the id does not itself copy or
delete its old slot, but that slot remains eligible for retention cleanup after a later save.
Clearing the id or setting the Boolean to `false` skips new saves without removing enrollment.
Reassignment or `true` resumes future eligible saves only for an enrolled window. A skipped attempt
is not a completed save and must not set close-save completion. A cancelled close does not undo
handler edits; a subsequent save takes fresh snapshots. Do not use the id from initial loading as
a permanent write destination, or grant enrollment from current property values.

A failed capture or serialization must leave the prior stored value alone. Do not delete the old
value before inserting its replacement. A storage error is a failed save, not an empty successful
record. Display and close continue under the API's best-effort persistence contract, with diagnostic
reporting rather than an app-visible persistence exception.

Complete the bounded save attempt before the relevant teardown proceeds. An unawaited queued write
is not a reliable close/session-end save. Backend latency and failure behavior must be measured;
"best effort" does not establish that an arbitrary storage operation is safe on a shutdown path.

### Multiple windows and processes

Publish all fields as one value rather than independently updating geometry, DPI, and state keys.
The backend must support coherent whole-value reads and replacement for concurrent processes. Verify
that property; do not infer transaction or power-loss guarantees solely from using `LocalSettings`.

There is no read-modify-write merge. Two eligible saves to the same application/id slot follow the
spec's last-successful-save-wins rule. Different registered applications in one package use different
containers. A reader must see a complete record or report a read/parse failure, never apply mixed
fields from two saves.

Save-sequence assignment, replacement, and retention deletion require per-namespace cross-process
coordination. This protects storage ordering, not window positions. It is not the older proposed
cascade mutex or unoccupied-slot search. Peer discovery and placement follow the spec's
[cross-process cascade design](../../specs/window-placement-persistence-spec.md#cross-process-cascade-discovery)
and do not require a store transaction.

## Retaining the 32 most recently saved placements

### Policy and scope

Treat automatic placement storage as a bounded cache. After a successful save, if the application's
namespace contains more than **32 placement records**, delete the least recently saved records until
32 remain. Saving a 33rd distinct id normally evicts one record; an already oversized store can
require several deletions.

The count is distinct stored placement ids, not live windows, processes, monitors, or cascade peers.
Two windows saving the same id use one slot. The limit is per current-user/application storage
namespace, shared by its processes; another application in the same package has its own allowance.
The future unpackaged default-store integration must preserve equivalent application isolation.

Recency means last successful save, not last load, display, activation, or window creation. Updating
an existing slot makes it newest without increasing the count. Loading does not change retention
metadata or trigger cleanup. Failed writes and cancelled close/session-end requests do neither.

Eviction affects only stored placement. Do not close, move, or un-enroll a live window whose old
record is evicted. Its next eligible save can recreate the slot. A later detached load of an evicted
id returns no saved value; automatic placement follows normal peer/stored/fallback selection.
Do not add a public retention setting, pinning mechanism, or deletion API in this design.

### Persist save order with the record

Add the private optional `TAG_SAVE_SEQUENCE` field described below. Each successful save carries a
positive `u64` sequence greater than every supported sequence currently in its application namespace.
Allocate it while holding the save/cleanup guard, using the maximum stored sequence plus one; start
at one for an empty namespace. Reject overflow rather than wrapping.

This orders successful replacements across processes and restarts without relying on wall-clock
timestamps, which can tie or move backward. No separate persisted counter or recency index is needed:
the sequence and placement are published in the same value. A failed write does not publish a new
sequence, and a later attempt can reuse that number.

The sequence belongs to storage, not public `WindowPlacement`. Detached loading does not expose it,
explicit placement cannot choose it, and saving a loaded or copied placement assigns a fresh sequence.
Readers accept older records without the tag and treat their retention sequence as zero. Use ordinal
value-name order to break ties deterministically, including legacy zero sequences.

### Save and trim under one guard

Coordinate all writers and cleaners for the same user/application/store namespace, for example with a
named mutex whose identity and access control match that scope. It must cover processes in different
sessions if they share the same store, not just writers on one UI thread or in one session.
Serialization of an individual settings value is not sufficient to prevent a cleaner from deleting
a slot another process has just refreshed.

Capture geometry before entering the guard. Do not perform native placement or invoke app callbacks
while holding it; guard against reentrant saves too. Use a bounded acquisition and work budget,
especially during session end. If coordination cannot be obtained, report a failed best-effort save
without writing or deleting outside the protocol. Lock availability is not permission to block
shutdown indefinitely.

While holding the guard, enumerate only placement value names owned by this format in the current
application container. Read and validate their bounded record metadata, assign the new sequence, and
publish the new record. Only after that replacement succeeds:

1. Count the owned placement values, including the newly written one.
2. If the count exceeds 32, order eviction candidates by ascending save sequence, then value name.
3. Delete the oldest candidates until the count reaches 32. Never evict the just-saved value in this
   cleanup pass.

Do not delete first to make room for a save that might fail. Do not delete unrelated values,
unrecognized key formats, or another application's container. Recognized corrupt records can be
treated as sequence zero and evicted first; missing/zero-sequence legacy records join that oldest
group. A transient read error is not evidence that a record is corrupt.

If enumeration or a read fails, or an owned record uses an unsupported major format whose ordering
cannot be interpreted safely, do not guess which valid record is oldest. Defer the save/cleanup
cycle and report the reason. Readers of newer major formats must preserve or explicitly migrate the
ordering protocol before cooperating with this cleaner.

### Failure recovery and bounds

The save and multiple deletions are not one storage transaction. A crash after replacement or a
failed deletion can leave more than 32 records. Stop cleanup on a deletion failure, retain the
successful save, and report cleanup separately; a later successful save retries trimming against a
fresh enumeration. An abandoned guard similarly requires rereading store state, not trusting an
in-memory victim list.

The normal completed cycle retains at most 32 records. This is a best-effort bound during storage
failures, interruption, unsupported formats, or writes from older implementations that do not
participate in the guard protocol. Do not advertise a strict instantaneous quota or a hard byte
limit. The existing per-record size ceiling remains independent.

Measure enumeration and deletion costs for an oversized store as well as the usual 32-record case.
If the operation budget is exhausted, stop safely and report deferred work; do not risk shutdown
responsiveness to force the count down immediately. Retention checks do not introduce move/resize,
load-time, or background persistence writes.

## Private record format

### Envelope and bounds

Retain the earlier proposal's base64-encoded, versioned TLV representation, independently of
PlacementEx's string and registry helpers. WinUI owns the wire meanings; C++ packing, pointer size,
native enum changes, and helper revisions must not alter them accidentally.

| Part | Representation |
|---|---|
| Header | ASCII `WPL1` (4 bytes), major `u16`, minor `u16`, total decoded byte length `u32` |
| Field | tag `u16`, value byte length `u16`, then that many value bytes |

The header is 12 bytes. Version 1.0 writes major `1` and minor `0`; all integer fields are
little-endian. Total length includes the header and must exactly equal the decoded byte count.
There is no field count or implicit padding. Each TLV must fit entirely within the remaining bytes,
and the final field must end at the declared boundary.

Use a 4 KiB decoded-record ceiling, with a corresponding maximum of 5,464 base64 characters, before
allocating decode storage. Enforce backend value limits too; this parser ceiling is not a promise
that every future record of that size fits the store. The largest canonical record using all tags
below is only 194 decoded bytes, or 260 base64 characters.

Write each applicable tag once in ascending order. The reader need not require that order. Generate
golden byte sequences rather than persisting a C++ struct or accepting host-endian output.

### Fields

| Tag | Value | Encoding | Value length |
|---|---|---|---|
| `TAG_NORMAL_RECT` | `0x0001` | Four `i32`: left, top, right, bottom | 16 |
| `TAG_WORK_AREA` | `0x0002` | Four `i32` | 16 |
| `TAG_ARRANGE_RECT` | `0x0003` | Four `i32` | 16 |
| `TAG_DPI` | `0x0010` | `u32` | 4 |
| `TAG_SHOW_CMD` | `0x0011` | Canonical native show command as `u32` | 4 |
| `TAG_FLAGS` | `0x0012` | WinUI-owned state bits as `u32` | 4 |
| `TAG_DEVICE_NAME` | `0x0020` | UTF-16LE code units, no terminator | 0-62 |
| `TAG_VIRTUAL_DESKTOP_ID` | `0x0021` | Windows `GUID` field layout | 16 |
| `TAG_SAVE_SEQUENCE` | `0x0030` | `u64` save-order metadata, little-endian | 8 |

The normal rectangle, work area, and DPI are required. A missing show command defaults to normal;
missing flags default to zero. The writer emits the arrange rectangle only for a snapped restore
state, the device name only when non-empty, and the desktop id only when available and non-empty.
The automatic writer emits the assigned positive save sequence. It is optional on read for older
records; absence or zero means unknown/oldest recency and does not invalidate placement. Keep this
metadata out of the detached public snapshot and PlacementEx translation.

Encode a GUID as little-endian `u32 Data1`, little-endian `u16 Data2`, little-endian `u16 Data3`,
and the eight `Data4` bytes. Do not serialize a textual GUID or copy an unspecified structure layout.

Wire rectangles use edges; the public value uses X, Y, width, and height. Both conversions require
checked arithmetic. Every present rectangle must have positive dimensions representable by the
public type. The normal rectangle must intersect the saved work area with positive area. Do not
retain the older draft's arbitrary +/-1,000,000-coordinate cutoff: it is not part of the current
data-validity contract.

DPI must be at least 96 and representable by the public signed integer type. Device-name length must
be even, at most 62 bytes, and contain no embedded NUL. Empty device names and `GUID_NULL` mean no
identity. Loading does not reject a structurally valid record because its monitor or desktop no
longer exists.

### State translation, not persisted policy

Retain only these meanings from the earlier durable-flags proposal:

| Wire bit | Meaning |
|---|---|
| `0x0001` | Restore to maximized after minimization |
| `0x0002` | Snapped placement |
| `0x0020` | Restore to snapped after minimization |

The old `AllowPartiallyOffScreen` (`0x0004`) and `AllowSizing` (`0x0008`) bits are not emitted or
honored. Keep their values reserved rather than assigning new meanings. Current target-window
capabilities and WinUI policy govern fitting; saved source-window styles must not override them.

`KeepHidden`, `NoActivate`, forced-downlevel testing, structural `FullScreen`, and presenter kind
are not durable state. Do not persist the native virtual-desktop flag either: derive it from the
optional id and the current request policy at the placement boundary.

The canonical writer maps the six public states as follows. This is a wire conversion table, not a
second definition of their public behavior.

| Snapshot state | `TAG_SHOW_CMD` | State bits |
|---|---|---|
| `Normal` | `SW_SHOWNORMAL` (1) | None |
| `Maximized` | `SW_SHOWMAXIMIZED` (3) | None |
| `Minimized` | `SW_SHOWMINIMIZED` (2) | None |
| `Snapped` | `SW_SHOWNORMAL` (1) | `0x0002` |
| `MinimizedFromMaximized` | `SW_SHOWMINIMIZED` (2) | `0x0001` |
| `MinimizedFromSnapped` | `SW_SHOWMINIMIZED` (2) | `0x0020` |

Both snapped states require a valid arrange rectangle. The codec never writes `SW_HIDE`, and saving
does not normalize minimization into an ordinary-launch state. That normalization happens later on
an apply-time working copy; otherwise application restart would lose the saved restore state.

### Reader validation and evolution

Reject invalid base64, invalid magic, unsupported major versions, missing required fields, incorrect
known-field lengths, truncated TLVs, trailing bytes, and unsafe arithmetic. Run the reconstructed
snapshot through the common structural validator before exposing it or calling native placement.
`PlacementEx::IsValid()` alone does not enforce all public field and conversion requirements.

Retain these compatibility rules from the proposed reader:

- Accept higher minor versions under major version 1 and skip unknown tags by their checked length.
- Default absent optional fields. Canonicalize recognized normal, maximized, and minimized show
  aliases; treat an unknown or non-displaying command, including `SW_HIDE`, as an absent show tag.
- Ignore unknown and reserved flag bits rather than passing them to PlacementEx.
- After defaults and masking, require a state combination represented in the conversion table.
  Reject conflicting known restore-state bits rather than guessing a restore target.
- Require valid snap geometry for a snapped state. Validate every present rectangle even when the
  resulting state does not use its snap rectangle.
- Retain last-one-wins handling for duplicate tags, but validate every encountered known field's
  length and bounds. The canonical writer never emits duplicates.

An additive optional field can increment the minor version only if ignoring it remains safe.
A must-understand semantic change requires a new major version. Do not reuse tags or reserved bits
with incompatible meanings. The `WPL1` envelope marker and explicit version fields must be handled
consistently by future readers and writers.

Older readers can discard unknown fields when they later write their own canonical record. Do not
promise preservation of future fields across downgrade or describe private prototype records as a
shipped compatibility contract. Establish golden records and migration requirements before shipping
the first durable format.

## PlacementEx integration

The imported helper is under [external/inc/PlacementEx](../../external/inc/PlacementEx/PlacementEx.h).
Its [window-position guidance](../../external/inc/PlacementEx/RememberingWindowPositions.md) and
[Win32 concepts](../../external/inc/PlacementEx/Win32Concepts.md) provide native background, not
additional WinUI API guarantees.

Include the helper through [User32Utils.h](../../external/inc/PlacementEx/User32Utils.h). The imported
code already has `CaptureFlags::SkipVirtualDesktopId` and a
`PLACEMENTEX_STRING_SERIALIZATION` switch. The persistence adapter uses its own codec instead of
`ToString`, `FromString`, or registry-storage helpers. Do not assume the prototype component or blob
implementation mentioned in header comments has also been imported.

For virtual desktops, the helper conditionally includes
[VirtualDesktopIds.h](../../external/inc/PlacementEx/VirtualDesktopIds.h) when
`USE_VIRTUAL_DESKTOP_APIS` is defined. That header records COM and library requirements; verify them
against the WinUI build and supported deployment configurations rather than assuming that copying
the headers establishes a usable backend.

The spec's [native-engine policy](../../specs/window-placement-persistence-spec.md#native-engine-and-winui-policy)
owns the public decisions, but not a second implementation of the engine's geometry algorithms.
Keep policy out of serialization and put the translation at one adapter boundary. Select only
call-time flags appropriate for the operation; in particular, do not enable PlacementEx's startup
show-command handling as part of this version.

The older notes suggested cloaking and rehiding as a downlevel non-activation implementation. Do not
carry that sequence forward as a solution. Select a safe native path before application, subject to
the spec's [fallback review gates](../../specs/window-placement-persistence-spec.md#safe-fallback-and-review-gates).
Hidden and non-activating paths must not show or activate first and repair visibility or focus later.
The hidden full-screen presenter transition also remains a separate integration gate.

### Placement policy unit

`WindowPlacementPolicy.h/.cpp` owns the WinUI decisions that do not belong to PlacementEx:
source precedence, reason and cascade permissions, six-state and restore-target mapping, current
window constraints, launch-monitor override, and hidden/non-activating safety. It should not own
another monitor selector or rectangle-adjustment implementation. `NativeRequest` remains a
backend-neutral description of the operation, but its adapter must materialize a `PlacementEx`
working value and delegate monitor selection, DPI/work-area adjustment, and ordinary cascade
wrapping to the imported engine.

The existing synthetic-topology tests are therefore an interim seam, not the production geometry
source of truth. Replace their expected values with engine-backed adapter tests and retain only
the topology injection needed to make monitor-selection differences deterministic. The adapter
must also expose the engine gaps instead of hiding them:

| Engine gap | Smallest WinUI-owned handling |
|---|---|
| `FindClosestMonitor` has no launch-monitor override and uses live topology | Select a validated `LaunchMonitorHint` as an explicit target; otherwise delegate selection to `PlacementEx`. Do not recreate its fallback distance rule. |
| `Cascade` operates on one working normal rectangle | For a peer, copy the peer placement and call `Cascade`. For explicit placement, use the peer anchor only to seed a working normal rectangle, then call the same engine cascade and restore the explicit size/state. |
| Current-window min/max/snap constraints are not durable PlacementEx fields | Validate and apply current target constraints before the engine call, then let the engine perform monitor/DPI fitting. Re-enter the engine's fit path after a constraint resize rather than maintaining a parallel clamp-and-fit algorithm. |
| `SetPlacement` can show or activate and its hidden state behavior is backend-dependent | Choose native flags and capability paths before application. If the engine cannot honor hidden or non-activating semantics without a show/activate/repair sequence, return the documented failure or fallback; never simulate the contract after the fact. |
| `GetPlacement` does not reconstruct minimized-from-snapped history or WinUI presenter caches | Keep those snapshots in WinUI capture state and use `PlacementEx::GetPlacement` for the native placement fields it can observe. |

Two design points are worth flagging:

| Point | Decision |
|---|---|
| Cascade coordinate space | A peer placement is cascaded in the peer's own work area before display adjustment, matching `CascadeOver`. Explicit placement is cascaded after adjustment, using the peer anchor and the explicit adjusted size. |
| Cascade offset | The caller supplies the caption-plus-frame offset. A zero or negative offset means the caller could not read system metrics, and the result is simply uncascaded. |

`BuildNativeRequest` must fail when the selected engine path cannot honor hidden application or
non-activation without showing or activating first. That is an adapter capability result, not a
reason to add a parallel implementation. The spec's hidden-preparation example accepts
`applied == false`, and failing is better than showing the window and repairing visibility
afterward. If the PM wants hidden maximized, minimized, or snapped application to succeed, that
needs a decided native transition and engine support, not a policy-only workaround.

#### Step 2A: engine-value seam

`WindowPlacementAdapter.h/.cpp` now includes the engine through `User32Utils.h` in the foundation
and isolated-test builds. `TryCreateEnginePlacement` validates rectangles, device names, state
flags, and optional metadata before publishing a working value. `TryReadEnginePlacement` reads
that value back after engine edits, checks representable dimensions, and rejects unsupported
flags or show commands. Both leave the output unchanged on failure. The existing storage format
and backend-neutral policy types are unchanged.

The adapter enables the virtual-desktop field and disables the unused string/registry serialization
in both builds. It does not call the shell or replay startup settings. The missing imported
Window Action compatibility header is replaced by local SDK-compatible type declarations;
WinUI's SDK and minimum Windows target are unchanged. The native API remains dynamically resolved.

The dedicated `WindowPlacementAdapterTests` class covers all six policy states, call-time flags,
optional identity, malformed input/output, checked rectangle conversion, and Window Action layout.
Synthetic monitor data feeds the real `PlacementEx::MoveToMonitor`; separate tests read back
`Cascade` and `KeepRectOnMonitor` results. No second adjustment implementation was added.

Validation on 2026-09-23: the foundation and isolated Window Placement test projects build for
`amd64chk`. The VM run of `*WindowPlacement*Tests*` passes all 77 tests, including the nine adapter
tests, with none failed, blocked, or skipped. These do not establish native application, physical
capture, real multi-monitor behavior, virtual-desktop operations, or downlevel capability.

Step 2B still must replace the existing policy's duplicate geometry and monitor-selection code.
`PlacementEx::Cascade()` currently reads system metrics itself; any injected caption/frame offset
must enter the engine rather than become another WinUI cascade implementation. Engine adjustment
uses native-width arithmetic, so the integration must also guard intermediate representability,
not merely validate the input and final rectangles.

##### Status correction (2026-09-24)

The paragraph above is stale. Step 2B has landed. `WindowPlacementPolicy.cpp` no longer carries its
own geometry or monitor-selection code: `ComputePlacement` delegates through
`TrySelectEngineMonitor`, `TryCreateEnginePlacement`, `TryMoveEngineToMonitor`, `TryCascadeEngine`,
and `TryReadEnginePlacement`. Treat the engine as the single geometry implementation.

Explicit-placement cascading is now wired end to end. The policy always implemented the
[explicit-cascade rule](../../specs/window-placement-persistence-spec.md#cascade-explicitly-supplied-placement),
but production passed no peer anchor, so the branch was unreachable. Three changes fix that.

| Layer | Change |
| --- | --- |
| `WindowPlacementApplication.h` | New `PeerAnchorSource` seam: a captureless-lambda-compatible function pointer plus a `void*` context. Added as a trailing defaulted parameter on `TryApplyPlacement`; replaces the `std::optional<Rect>` parameter on `TryComputeNativeRequest`. |
| `WindowPlacementApplication.cpp` | Resolves the anchor *after* the target monitor is known, and only when cascading is permitted and the source is `PlacementSource::Explicit`. Validates the returned rectangle before feeding `policy.Cascade.PeerAnchorNormalRect`. |
| `DesktopWindowImpl.cpp` | Peer search split into `TryFindPlacementPeer(placementId, requiredDeviceName, snapshot)`. A monitor mismatch now continues the Z-order walk instead of aborting it, so a lower peer on the target monitor is still found. `ApplyPlacement` binds a self-referencing `PeerAnchorSource`. |

The ordering matters: the spec requires the target monitor first, then the peer search restricted to
that monitor. The coordinator has neither an `HWND` nor a monitor, so it cannot do the lookup.
`DesktopWindowImpl` is both the placement host and the caller of `TryApplyPlacement`, so it supplies
the callback directly. `IWindowPlacementCoordinatorHost` is unchanged.

`WindowPlacementExplicitCascadeTests` covers the seam: the anchor is requested only for explicit
placement under `CascadeBehavior::Enabled` and a non-restart reason; the requested device name
matches the resolved target monitor; the cascaded result keeps the explicit size, state, and monitor
and moves only its position; and a missing or invalid anchor leaves the explicit result untouched.

Validation on 2026-09-24: `Microsoft.UI.Xaml` and the isolated Window Placement test project build
for `amd64chk`. The full isolated suite passes 169 of 169 with none failed, blocked, or skipped.
This run was single-display. The spec's multi-monitor requirement - a peer on a different monitor
must be skipped while a lower-Z peer on the target monitor is used - is **not covered** and needs a
multi-monitor lane.

Update: that requirement is now covered on a single display. The walk itself moved into
`WindowPlacementPersistence::TryFindAcceptablePlacementPeer`, which pulls candidates from a
`PlacementPeerCandidateSource` (`bool TryGetNext(void*, Snapshot&)`) and applies
`IsAcceptablePlacementPeer` to each one. `DesktopWindowImpl` supplies the production source, which
walks Z order and yields up to two candidates per marked window: the framework placement cache,
then a direct `WindowPlacementCapture` for windows this process does not own. Tests script the
candidate order directly, so `PeerWalkSkipsOffMonitorCandidatesAndKeepsWalking` proves an
off-monitor candidate at the top of Z order is skipped and a lower matching candidate is returned.
Only live Z-order enumeration itself still needs real multi-monitor hardware.

At the initial step 2A checkpoint, step 2C still had to read live topology, prove native
hidden/non-activating capability, and capture
effective physical geometry after application. `TryReadEnginePlacement` is not that capture:
neither backend's `SetPlacement` in/out value is a reliable post-apply snapshot. The existing
policy's hidden-state checks and the seam's flag translation do not prove backend safety.

### Native application status (2026-09-24)

`DesktopWindowImpl::ApplyPlacement` now calls the separate `WindowPlacementApplication` unit.
It rejects closed/non-overlapped windows, applies pending client sizing before the placement pass,
and refreshes the restored-size cache afterward. The application unit takes the copied explicit
snapshot first; otherwise, an opted-in nonempty id uses the existing packaged reader once.
A selected source is never replaced after computation or application failure. With no source,
only a valid Launch monitor hint can migrate current fallback geometry; that does not report
successful selected placement to `TryApplyInitialPlacement`.

Production monitor selection uses `PlacementEx::FindClosestMonitor` or a valid launch monitor
handle. Current HWND style, `WM_GETMINMAXINFO`, target DPI, and snapping policy feed the existing
policy/engine adapter. Physical-coordinate scope is shared with capture. The adapter now rejects
a conservative envelope of unrepresentable native-width adjustment intermediates before calling
`MoveToMonitor`; it does not duplicate the geometry algorithm.

Native application calls `PlacementEx::SetPlacement`. The imported API has an optional
`allowLegacyFallback` argument, defaulting to the existing behavior for other callers. Hidden,
non-activating, and virtual-desktop requests require Window Action and prohibit legacy fallback,
including after a Window Action failure. The legacy path can show/activate before repairing
state, so it cannot establish these contracts. Hidden non-normal requests continue to return
false before application. There is no rollback or second-source retry after a partial failure.

`PlacementEx::SetPlacementWithApplyWindowAction` treats a same-monitor restore specially. When the
placement is not full screen or arranged, and the window's current monitor has the same work area
and DPI as the target monitor, the call omits the move-to-monitor action and the historical work
area and DPI modifiers. The saved normal rectangle is already in that monitor's coordinate space,
so those modifiers only add a frame adjustment that shifts the restored position. This also covers
maximized and minimized placements because their normal rectangle is the restore position. A
different monitor, work area, or DPI keeps the existing monitor action and modifiers, which lets a
placement from a removed or changed monitor be migrated.
`WindowPlacementApplicationTests` covers a nonzero-origin restore with an unchanged monitor.

The application path refreshes the existing capture cache from the HWND after both success and
failure; success requires native success and effective capture. It never treats `SetPlacement`'s
mutable argument as the effective snapshot. Successful minimized-from-snapped application can
supply otherwise unobservable restore history; actual geometry/state still use the capture path.
Virtual-desktop identity is not synchronously queried or inferred from the request.

First `Show` now suppresses activation for ApplicationRestart even when placement fails or is
skipped. Nonactivating display uses `SW_SHOWNA`, not the restoring `SW_SHOWNOACTIVATE`, so it does
not undo a successfully applied maximized state. A skipping initial Show still handles ordinary
pending client sizing.

**Validation:** `amd64chk` foundation, isolated placement tests, the native XAML DLL, and the full
repository build succeed. A refreshed-payload run on `ge_current-260828-Desktop` of
`*WindowPlacement*Tests*` passes **164/164**, with zero failed,
blocked, or skipped. The new real-HWND `WindowPlacementApplicationTests` class had 13 methods at
that run: hidden/no-activation behavior, actual capture, native states, guarded legacy fallback,
current constraints, malformed/extreme inputs, destruction during application, source precedence
and single load, repeated hidden saved application, and coordinator-to-native application/skip.
It now has 18, including same-monitor nonzero-coordinate restore coverage, a comparison of saved
minimized application under Default and ApplicationRestart, and restore-to-snap bounds coverage.
Display-routing coverage also exercises restart suppression after failure and skip.

**Minimized first-show diagnosis (2026-09-25):** the packaged round-trip test omitted `Reason`.
`TryApplyPlacement` passes that Default reason to `TryComputeNativeRequest`, which uses
`MapState` in `WindowPlacementPolicy.cpp` to map saved `Minimized` to `Normal`, as the spec requires.
`BuildNativeRequest` therefore selects `NativeShowCommand::Normal`, and `TryCreateEnginePlacement`
sets `SW_NORMAL`. `DoNotActivate` suppresses activation; it does not preserve minimization.
With `ApplicationRestart`, those same steps select `Minimize` and `SW_MINIMIZE` (6).
`PlacementEx::SetPlacementWithApplyWindowAction` calls `CWindowAction::SetMinimized`, which sets
`WAK_PLACEMENT_STATE` and `WPS_MINIMIZED`, before `action.Apply(hwnd)` calls the native API.

On `ge_current-260828-Desktop`, `SavedMinimizationRequiresApplicationRestart` records
`reason=0 engineShow=1 outcome=1 iconic=0 visible=1` and
`reason=2 engineShow=6 outcome=1 iconic=1 visible=1`. Here outcome 1 is `Applied`, which requires
`PlacementEx::SetPlacement` to return true and capture to succeed. NoActivate prohibits legacy
fallback, so this also proves that `action.Apply` returned true (the native API returned nonzero).
Both cases preserve exact restore bounds and produce zero activations.
The later `DesktopWindowImpl::ShowWindowForDisplay` uses `SW_SHOWMINNOACTIVE` for an iconic window;
it does not undo the minimized state. The packaged test now requests ApplicationRestart and
asserts the restored presenter is Minimized rather than skipping: saved and restored bounds are
`107,100 640x480`, at 96 DPI on work area `0,0 1024x720`. The native application class passes
18/18 and the packaged persistence class passes 18/18, with no failures or skips.
No product-policy change was needed.

**Snapped first-show diagnosis (2026-09-25):** this is an OS limitation, not a product gap. WinUI
builds the arrange request correctly. `MapState` maps saved `Snapped` to `TargetState::Snapped` when
the user snapping setting is on, `ComputePlacement` derives the snap rect from the saved arrange
rect, `BuildNativeRequest` sets `Flags.Arranged` with `NativeShowCommand::Normal`, and
`TryCreateEnginePlacement` sets `PlacementFlags::Arranged` with `SW_NORMAL`. Note that
`SetPlacementWithApplyWindowAction` treats an `Arranged` request as never being a same-monitor
placement, so it always issues `SetMoveToMonitor` along with `SetArranged`.

`SavedSnapApplicationOnANewWindow` records the result on `ge_current-260828-Desktop` with
`NoActivate`, which prohibits legacy fallback and makes the native return value the only thing that
decides the outcome.

| Saved state | snapping | Request flag | Engine show | Outcome | Observed |
| --- | --- | --- | --- | --- | --- |
| `Snapped` | 1 | `Arranged` | 1 (`SW_NORMAL`) | 3 (`FailedAfterStart`) | flags 8, iconic 0, visible 0 |
| `MinimizedFromSnapped` | 1 | `RestoreToArranged` | 6 (`SW_MINIMIZE`) | 1 (`Applied`) | show 2, flags 8, iconic 1, visible 1 |

So `action.Apply` returns false for `SetArranged` on a window that has never been shown, while
`SetMinRestoreToArranged` on the same window succeeds. Observed flags 8 is `AllowSizing` alone;
`GetPlacement` only reports `Arranged` for a window that is actually arranged, and
`RestoreToArranged` is a request-only concept that it never reports. The declined case leaves the
window hidden with zero shows and zero activations, and `AllowsSourceRetry` is false, so it does not
trigger another peer search or storage load. No product or policy change was made.

**Validation update (2026-09-25):** `MinimizedFromSnapped` restores to the saved snapped bounds.
`MinimizedFromSnappedRestoresToSavedSnapBounds` logs `saved=(0,0,512,720)` and
`observed=(0,0,512,720)` on `ge_current-260828-Desktop`, and verifies the post-restore
`PlacementEx::GetPlacement` arrange rectangle exactly. The commands were:

```text
.\initrun.ps1 msb /q dxaml\xcp\dxaml\lib\unittests\WindowPlacement\Microsoft.UI.Xaml.Tests.Isolated.WindowPlacement.vcxproj
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName ge_current-260828-Desktop 'WindowPlacementApplicationTests::*' '-SkipPackageUninstall'
```

The targeted run passes 18/18 with zero failed, blocked, or skipped. Live peer discovery now
publishes the specified per-group HWND marker and enumerates both framework-owned and foreign-process
windows. New-window snapped application is still declined by the OS, as measured above. Real
multi-monitor/mixed-DPI transitions, actual downlevel systems, virtual-desktop moves, and launch
hint scenarios still need acceptance coverage. The coordinator
inbox task is complete for the implemented integration boundary; these environment-specific
scenarios remain open acceptance gates.

Presenter transitions now have runtime coverage.
`WindowPlacementPersistenceTests::FullScreenPresenterKeepsAndSavesThePreFullScreenPlacement`
shows an enrolled window at 640x480, switches the AppWindow to the FullScreen presenter,
confirms the window really resized to the monitor work area, and then verifies that
`TryGetPlacement` still reports the pre-full-screen rectangle in the `Normal` state and that
closing while full screen saves that same rectangle. That exercises the whole production chain -
`AppWindowPresenterSupportsSizing` to `ResolvePresenterKind` to the non-overlapped early return in
`WindowPlacementCapture::TryCapture` to the retained cache - rather than only the isolated
`ResolvesPresenterKind*` unit tests. The test reports a skip, not a pass, if the environment
refuses the presenter change or does not resize the window.

There are two ways to be full screen, and they leave capture through different doors. The
presenter test above covers the AppWindow FullScreen presenter, where `ResolvePresenterKind`
reports `NonOverlapped` and `TryCapture` returns at the presenter check. An app that instead
removes its own `WS_CAPTION` and `WS_THICKFRAME` and fills the monitor rect still reports an
overlapped presenter, so it reaches the `PlacementFlags::FullScreen` guard further down.
`WindowPlacementCaptureTests::FullScreenWindowKeepsThePreFullScreenSnapshot` covers that second
door and asserts the same retention invariant.

Related open PRs checked during self-review:
[11974](https://github.com/microsoft/microsoft-ui-xaml/pull/11974) changes the pending-size and
restored-size helpers that this path calls; revalidate sizing when integrating those changes.
[11772](https://github.com/microsoft/microsoft-ui-xaml/pull/11772) changes first-display preparation
and [11587](https://github.com/microsoft/microsoft-ui-xaml/pull/11587) changes window redirection.
They touch `DesktopWindowImpl`, but do not supply this placement application implementation.

## Automated testing

**Open infrastructure requirement:** We do not currently have a selected, validated solution for
automated multi-monitor and variable-DPI testing. This feature needs both. Synthetic geometry tests
and a single-monitor run cannot establish that native placement works across real topology and DPI
changes. Selecting and proving a repeatable test environment is part of the implementation work.

### Test layers

Use the existing repository test infrastructure where it fits. Separate tests that need only
controlled input data from tests that require real windows, storage, or display configuration:

| Layer | Coverage and approach |
|---|---|
| Codec and identity unit tests | Deterministic record/name inputs, golden data, malformed values, and checked arithmetic; no display changes or real user settings |
| Policy tests with synthetic topology | Inject saved/current monitor descriptions and DPI values at an internal test seam; exercise selection and geometry adjustment where they can be separated from native calls |
| Storage integration tests | Real supported `ApplicationData` access, isolated test identities/ids, cross-process save/load, whole-value concurrency, and injected storage failures |
| Window lifecycle tests | Real HWNDs and WinUI events for enrollment, accepted/cancelled close, hidden capture/application, presenter caches, and destruction backstops |
| Display-environment tests | Actual multi-monitor, mixed-DPI, and changing-topology behavior on a controlled interactive desktop; run native and forced-downlevel placement paths |

Injected topology is not evidence that Windows sends the expected notifications, reports physical
coordinates correctly, restores snapping, or honors visibility and activation restrictions. Keep
those assertions in native integration tests. Internal seams must not become new public placement
or storage APIs.

For persistence round trips, process A creates and changes a test window, then reaches an eligible
save point. Process B loads the saved record and restores a new window. This distinguishes durable
storage from an in-memory cache. Use separate test apps when checking application isolation within
one package, and avoid touching unrelated settings.

Test retention with 31, 32, 33, and already oversized sets of distinct ids. Verify that refreshing an
old slot protects it, reading it does not, and trimming removes exactly the oldest excess records.
Run competing save/cleanup processes against one namespace, and inject failures before replacement,
between replacement and deletion, and partway through deletion. Check recovery after interruption,
lock timeout/abandonment, legacy or corrupt records, unsupported versions, and sequence overflow.
Keep another application's records and unrelated settings present to detect overbroad cleanup.

Exercise confirmed/cancelled session-end handling with controlled integration hooks where possible.
Actual sign-out or shutdown tests require a disposable, dedicated environment. Do not sign out a
shared development or test machine merely to trigger a save.

### Multi-monitor and DPI environment to establish

Evaluate dedicated physical-display lab machines, a controllable virtual/indirect-display setup, and
VM configurations as candidates. No candidate is assumed to support the required topology changes,
independent per-monitor scale, shell behavior, or unattended runs. A remote session's reported
resolution is not proof of multi-monitor or effective-DPI coverage.

The selected environment must make these operations repeatable and observable:

- Establish one or multiple monitors with known bounds, work areas, identities, and effective DPI;
  include monitors left of or above the primary monitor.
- Save and restore with different DPI on different monitors, such as 100%, 150%, and 200% scale.
  Change DPI between runs and move a live window between monitors with different DPI.
- Remove or disable the saved monitor, reconnect it, change the primary monitor, and reorder monitor
  positions between save and restore.
- Change resolution or work area independently from DPI so those adjustments are tested separately.
- Observe configuration completion and actual monitor/DPI values before performing assertions;
  changing resolution or just writing a requested scale value is not proof that DPI changed.

Tie cases to the spec's
[placement scenario inventory](../../specs/window-placement-persistence-spec.md#placement-scenarios-and-winui-decisions),
rather than creating a second source of expected public behavior. Prioritize unchanged-topology
round trips, missing-monitor fallback, same-monitor DPI changes, and mixed-DPI monitor migration,
then combine those cases with normal, maximized, minimized, and snapped placement.

Each run must record its requested and observed display configuration, Windows build, app DPI
awareness, placement backend, and resulting placement. Assert actual normal/snap bounds, restore
state, visibility, and activation, not just a final screenshot. Use event observations for transient
visibility or focus violations. Compare backend-specific expected results or documented invariants,
not an unsupported assumption that all backends produce identical pixels.

This detailed output belongs to controlled tests using generated test data. It does not relax the
production diagnostic restrictions below.

### Isolation, cleanup, and readiness

Topology and DPI changes affect the desktop, not just the test window. Run them with exclusive
ownership of the test environment; do not parallelize conflicting display tests. Record the initial
configuration and restore it after failures, with recovery or reimaging available if the desktop
becomes inaccessible. Keep the controller able to detect failed configuration changes and clean up
test processes and settings.

Before claiming automated coverage, demonstrate unattended setup, save/restore, observed DPI and
topology changes, failure capture, and cleanup in the chosen environment. Assign infrastructure
ownership and decide which cases run in PR validation versus a dedicated scheduled lane. Missing
display capabilities must produce explicit unsupported/skipped results, not passing tests.

Unit and single-monitor integration tests can proceed while this infrastructure is unresolved.
Manual lab runs can supply interim evidence, but do not close the automated multi-monitor and
variable-DPI gap. Track that gap as a readiness requirement alongside backend implementation.

## Diagnostics and implementation gates

### Report failures without logging placement data

Use structured internal results rather than success-shaped defaults. Report capture, identity,
decode, apply, and write failures through an ETW event and a debug breadcrumb containing the
operation, failure category, and error code. Treat an ordinary missing record as an expected outcome,
not an unexpected storage error.

Do not include raw ids, derived names/slugs, record contents, coordinates, device names,
virtual-desktop ids, or arbitrary backend error text. A hash is not anonymization and is not needed
to diagnose the operation class. Retain the original normalized HRESULT when one is available;
data validation without a native error uses its internal failure category, not a fabricated HRESULT.
Automatic persistence failures remain non-fatal to display and close; the detached loader's
unexpected failures propagate their original HRESULT. Emit one failure event/breadcrumb per failed
load or save attempt at the operation boundary, rather than duplicating reports in each helper.

Status: implemented through the internal `ReportPlacementFailure` seam. Detached loading,
automatic restore (including apply), and saving report once at their operation boundary.
An explicit placement pass uses the `Apply` operation. Reports contain only fixed operation and
category values and an HRESULT; `S_OK` means the category has no native error. The bool-only
placement engine supplies no normalized HRESULT, so its failures use the category alone.
`WindowPlacementFailure` uses the existing XAML ETW provider; debug builds also emit a breadcrumb.
Missing records and successful operations emit neither.

### Warn a developer when Activate() discards a prepared placement

The spec requires a development debug diagnostic when an opted-in first `Activate()` follows a
`TryApplyInitialPlacement` call on the same window. That first `Activate()` runs the automatic pass
and replaces whatever the hidden application prepared, so the silent case is easy to miss.

Status: implemented in `WindowPlacementCoordinator`. The coordinator records that a hidden
application pass ran, and reports once per coordinator on the first opted-in `Activate()` while the
initial placement phase is still open. Debug builds emit developer output; release builds emit none.

| Decision | Reason |
| --- | --- |
| Arm the diagnostic only when `TryApplyInitialPlacement` reached `ApplyPlacement` | Calls rejected for a closed, visible, reentrant, or invalid request never prepared anything |
| Report after the operation starts, before the automatic pass | A rejected `Activate()` prepares nothing to discard, so it reports nothing |
| Report at most once per coordinator | The message is about the window, not about each call |
| Track state per coordinator instance, not globally | Keeps the flag thread-affine with the coordinator and needs no reset between tests |

This is development output only. It is not an event, not a failure, and not part of the behavioral
contract, so apps must not depend on it.

### Storage-specific acceptance work

The API spec already owns the full
[implementation acceptance criteria](../../specs/window-placement-persistence-spec.md#implementation-acceptance-criteria).
Add these implementation-focused checks without copying its complete display/placement matrix:

| Area | Required evidence |
|---|---|
| Identity and names | Golden digests for ASCII, non-ASCII, case-distinct, non-normalized, embedded-NUL, and surrogate-containing ids; separate applications in one package; stable results across processes and architectures |
| Store backend | Packaged read/write through the internal ABI; framework-dependent and self-contained deployment; supported value/container limits; thread use; update/uninstall lifetime |
| Proposed unpackaged default | Once the dependency is available: setup before first use, missing/late/repeated configuration, stable identity and cleanup, and the same namespace for automatic load, detached load, and save |
| Reader errors | Each phase-specific null case, original HRESULT propagation through identity/backend/reader/public boundaries, allocation failure, C#/C++/WinRT exception mapping, and automatic-restore fallback for the same injected failures; no writes during load or package-wide fallback |
| Codec | Golden little-endian records and all six states; bounded base64/TLV parsing; duplicate and unknown tags; unknown versions/bits; malformed lengths; signed conversion and scaling overflow |
| Save hooks | Handler changes to id and geometry; cancellation; destruction backstop before the early return; close deduplication; intentional session-end plus close writes |
| Eligibility | Never-displayed windows; hidden application alone never enrolls; opted-in skipping first `Show` enrolls; direct display without enrollment; late Boolean and id clear/reassign combinations; initially empty id and explicit `false` never enroll later; storage recovery does not require reenrollment |
| Concurrency | Same-slot whole-value replacement under multiple processes; independent application/id slots; read/write races; no partially published snapshot |
| Retention | 32-record cap by successful-save order, not read order; same-id refresh; deterministic legacy ties; cross-process guard; no eviction before a successful write; interrupted/failed cleanup recovery and application isolation |
| Capture caches | Hidden moves and state changes; minimized-from-snapped history; complete overlapped state across presenters; actual-state capture after partial application |
| Shutdown and COM | Message-safe capture using cached desktop identity; no blocking shell query on input-synchronous paths; scheduling gate for closed and never-displayed windows; failed shell query leaves placement saveable; late query completion after close or HWND reuse |
| Diagnostics | Operation/error information present, with identity and placement data absent |

The earlier notes recorded prototype experiments and version-specific unpackaged storage results.
Those reports are not evidence that this checkout implements the current design or that the
packaged application namespace and revised lifecycle have been validated. Re-establish the relevant
storage and deployment evidence for this implementation.

Leave the hidden-state and backend fallback decisions open until their owners
resolve them. The late-property, detached-load error, and pre-display abort decisions are now
specified, implemented, and covered:

| Decision | Covering tests |
| --- | --- |
| Late property changes | `WindowPlacementPersistenceTests::SaveUsesThePersistIdSetAtCloseTime`, `::SaveUsesTheGeometrySetByACloseHandler`, `::LateAutomaticPersistenceOptOutCancelsTheSave`, `::EmptyPersistIdAtFirstDisplayNeverEnrollsSaving`, `::LateRequestsRejectNullButIgnoreFieldsInNonNullOptions` |
| Detached-load errors | `WindowPlacementDiagnosticsTests::DetachedIdentityFailureReportsOriginalErrorOnce`, `::DetachedReadFailureReportsAndPropagatesOriginalError` |
| Pre-display abort | `WindowPlacementCoordinatorTests::AbortedFirstShowLeavesPhaseOpenAndRetriesWithFreshSnapshot`, `::AbortedSkippingFirstShowDoesNotEnroll`, `::AbortedFirstActivateLeavesPhaseOpen`, `::AbortThenCloseNeverEnrollsOrDisplays`, `WindowPlacementDisplayRoutingTests::AbortedDisplayIsRetriedAsAFirstDisplay` |

Backend concurrency and durability are no longer open items:

| Property | Covering tests |
| --- | --- |
| Two concurrent saves at the same id publish exactly one whole snapshot, never a mix | `WindowPlacementStoreTests::ConcurrentSavesPublishOneWholeSnapshot`; `WindowPlacementStorageIsolationTests::ConcurrentSavesForOneIdLeaveOneCoherentRecord` against real packaged storage |
| A load interleaved with a save returns the old record or the new one, never a partial read | `WindowPlacementStoreTests::LoadDuringSaveSeesOldOrNewWholeSnapshot` |

The store-layer tests hold the backend to the contract stated under
[Multiple windows and processes](#multiple-windows-and-processes): each value read and each value
replacement is atomic. `ConcurrentSavesForOneIdLeaveOneCoherentRecord` is what shows the real
packaged `ApplicationData` backend honors that contract, so the two layers only prove the end-to-end
property together.

Storage format ratification, cache policy, and virtual-desktop thread integration remain
implementation review items. The proposed unpackaged-default-store capability and automated
multi-monitor/variable-DPI environment also remain explicit dependencies; neither is supplied by the
current design notes.

#### Null or empty `persistPlacementId` and the out-parameter

`WindowPlacement.LoadForPersistPlacementId` rejects a null or empty `persistPlacementId` with
`E_INVALIDARG`. WinRT canonicalizes an empty string to a null `HSTRING`, so both inputs take the
same path.

On that argument-rejection path the out-parameter is not guaranteed to be written. The generated
stub runs its `ARG_NOTNULL` check before `*ppResult = {}`, so it returns before clearing anything.
ABI callers must initialize the out-parameter themselves and must not read it after an error. The
projections are unaffected: C# and C++/WinRT turn the failure into an exception and never hand back
a value.

A non-null id does reach the implementation, which clears the out-parameter before doing any work.
A valid but unused id therefore returns `S_OK` with a null placement.

`WindowPlacementAbiTests::DetachedLoadOfEmptyOrNullIdReturnsInvalidArgumentAndNoObject` covers all
three cases.

## Public Show and Hide projection (2026-09-25, Developer)

The spec declares `public void Show();` and `public void Hide();` on `Window`, but both methods
only existed on the private `IWindowPrivate` interface. They are now projected onto the public
contract-12 surface.

| Change | Detail |
| --- | --- |
| XamlOM model | Added `Show()` and `Hide()` at `[Version(12)]` on `Window` |
| ABI naming | The existing options overload is now `ShowWithOptions` in the IDL; parameterless `Show()` is the `[DefaultOverload]` |
| `DesktopWindowImpl::HideImpl` | Implemented; was `E_NOTIMPL` |
| `Window::ShowImpl` | Now forwards to `m_spWindowImpl->ShowImpl()`; was `E_NOTIMPL` |

`Hide` behavior follows "Control display and activation" and the "Window.Hide method" section:

- Fails with `E_INVALID_OPERATION` when the window is closed.
- No-ops when the window is already hidden.
- Captures effective placement before hiding, so a later save or `TryGetPlacement` reports the
  geometry the window was hidden from.
- Does not save, does not close, and does not open or end the initial-placement phase. The
  `WM_SHOWWINDOW` hook only notifies display when `wParam` is true, so hiding stays inert.
- A hidden window drops out of live peer discovery on its own, because that enumeration already
  skips windows that fail `::IsWindowVisible`.

Validation: full `amd64chk` build succeeded. On VM `ge_current-260828-Desktop`, the new
`WindowShowHideTests` class passed 9 of 9, and the broader `*WindowPlacement*` suite passed 169 of
170. The single failure was the already-documented transient `coreclr.dll` crash in
`FirstActivateUsesPersistenceConfiguration`, which passed when re-run alone.

The UWP scope question is resolved by the decision below, and the public/private dispatch
separation is implemented and covered. See the
[implementation record](#implementation-record-developer-2026-09-24).

## Public display API host boundary decision (2026-09-24)

**Decision:** keep the public display APIs desktop-only, with a runtime host check.
For a UWP-backed or framework dummy `Microsoft.UI.Xaml.Window`, otherwise valid public
`Show()`, `Show(WindowShowOptions)`, and `Hide()` calls must fail with `E_NOTIMPL` without
changing visibility, activation, placement, persistence, or audio state. Keep the methods
in the common contract metadata. Normal thread and argument validation still applies.
This does not add support to classic `Windows.UI.Xaml.Window` or change `Activate()`.

Being outside the feature's supported scope is not permission to expose an unrelated
private operation as successful public hiding. The current source establishes the risk:

- `Window::Window` still creates `UWPWindowImpl` for the framework's first/dummy window.
  A real UWP window is feature-gated, but the implementation is not dead code.
- `Window.g.h` forwards public `IWindow12::Hide` to `WindowGenerated::Hide`, which also
  implements `IWindowPrivate::Hide`. `Window::HideImpl` forwards both to the same backend.
- `UWPWindowImpl::HideImpl` calls `TearDownAudioGraph` when a sound service exists and
  otherwise returns success. It does not hide a window.
- Both UWP Show overloads already return `E_NOTIMPL`. The new `WindowShowHideTests`
  class uses WPF hosting and desktop windows, so it does not cover this host boundary.

Alternatives considered:

| Option | Decision |
| --- | --- |
| Leave UWP behavior undocumented because it is unsupported | Rejected. Public Hide can report success and perform private lifecycle work instead of hiding. |
| Implement public showing/hiding for UWP | Rejected for this feature. CoreWindow lifecycle and audio semantics need a separate design; desktop placement is the current scope. |
| Hide the entire new interface on non-desktop instances | Rejected. That would also change discovery of unrelated placement members on `IWindow12`. |
| Reject only public display calls on non-desktop instances | Selected. It bounds the feature and preserves the private lifecycle contract. |

Developer follow-up: separate public Hide dispatch from `IWindowPrivate::Hide` using the
model/code-generation path, not a hand edit to generated files. Keep the projected public
name `Hide()` and preserve private UWP audio cleanup. An unconditional `E_NOTIMPL` in the
existing shared `UWPWindowImpl::HideImpl` is not an acceptable fix. Check the underlying
window implementation, not package identity: unpackaged desktop windows remain supported.
Do not suppress errors with a successful no-op.

Acceptance requires coverage of public rejection on a non-desktop/dummy instance, unchanged
private Hide behavior with and without a sound service, and the existing desktop Show/Hide
behavior. Verify both ABI dispatch paths so a passing desktop projection test cannot mask a
shared private implementation. Where a host-specific case cannot run, record the limitation
and leave that acceptance item open. This decision was based on source review. The
[implementation record](#implementation-record-developer-2026-09-24) below reports the runtime
isolation as implemented and exercised.

Related open work: [PR #11240](https://github.com/microsoft/microsoft-ui-xaml/pull/11240)
proposes `TrySetForeground` and explicitly defers Show. It overlaps the display/activation
design, but does not supply a UWP implementation or resolve this boundary.
[PR #11974](https://github.com/microsoft/microsoft-ui-xaml/pull/11974) modifies
`DesktopWindowImpl` sizing and closed-window behavior; preserve that work when integrating.

### Implementation record (Developer, 2026-09-24)

Implemented as decided. Three parts:

**1. Generator: separate C++ names from ABI names.**

`DXamlNameAttribute` gained an optional `IdlName`. A method can now keep its projected ABI
name while getting a distinct generated C++ entry point:

```csharp
[DXamlName("HidePublic", IdlName = "Hide")]
public void Hide() { }
```

`MemberDefinition` gained `CppNameOverride` and a derived `CppName`. The framework header,
body, and forwarder templates emit `CppName`; the IDL template emits `IdlMemberInfo.Name`.
This is a general capability, not a `Window` special case.

Renaming the model member also makes `Helper.ImplementInterface` stop treating the public
member as a duplicate of `IWindowPrivate.Show`/`Hide`, so the private interface methods are
implemented again. The generated dispatch is now:

| ABI entry | Generated C++ | Partial-class impl |
| --- | --- | --- |
| `IWindowPrivate::Show` / `Hide` | `WindowGenerated::Show` / `Hide` (IFACEMETHOD override) | `Window::ShowImpl` / `HideImpl` (unchanged) |
| `IWindow12::Show` / `Hide` | `WindowGenerated::ShowPublic` / `HidePublic` | `Window::ShowPublicImpl` / `HidePublicImpl` |

The two paths also get distinct telemetry names (`Window_Show` vs `Window_ShowPublic`).

**2. Product code: the host check.**

`WindowImpl` gained `SupportsPublicDisplayApis()`: `true` in `DesktopWindowImpl`, `false` in
`UWPWindowImpl` (which also backs the framework dummy window). `Window::ShowPublicImpl`,
`Window::HidePublicImpl`, and `Window::ShowWithOptionsImpl` return `E_NOTIMPL` before doing
anything when the check fails. `UWPWindowImpl::HideImpl` and its audio cleanup were not
touched, and `IWindow12` is not gated as a whole.

**3. Coverage.**

| Acceptance item | Status |
| --- | --- |
| Existing desktop Show/Hide behavior | Covered. `WindowShowHideTests`, 9/9 pass on the VM, and they now run through `ShowPublic`/`HidePublic`. |
| Public dispatch is separate from private dispatch | Covered at build time. `Window_Partial.cpp` static-asserts that `WindowGenerated::ShowPublic`/`HidePublic` exist, so a codegen regression that collapses the names fails the build instead of silently routing public `Hide()` into the private path. |
| Public rejection on a non-desktop/dummy instance | Covered. `WindowDisplayApiHostTests` (native foundation target, `RunAs=UAP`) proves `Show()`, `Show(options)`, and `Hide()` all return `E_NOTIMPL` on the non-desktop instance. No test hook was needed: under UAP hosting `Window::Current` already routes to `DXamlCore::GetDummyWindowNoRef()`, which is backed by `UWPWindowImpl`. |
| Unchanged private `Hide` with and without a sound service | Covered. `PrivateHideSucceedsWithoutSoundPlayerService` and `PrivateHideSucceedsWithSoundPlayerService` call `IWindowPrivate::Hide` on the same instance whose public `Hide()` is gated. `PublicAndPrivateHideDispatchSeparately` proves both paths on one object. |

**`WindowDisplayApiHostTests`**, 7/7 pass on the VM:

| Test | What it proves |
| --- | --- |
| `PublicShowReturnsNotImplementedOnNonDesktopHost` | `IWindow12::Show()` returns `E_NOTIMPL`, visibility unchanged. |
| `PublicShowWithOptionsReturnsNotImplementedOnNonDesktopHost` | `ShowWithOptions(options)` returns `E_NOTIMPL`; `ShowWithOptions(nullptr)` returns `E_INVALIDARG`. |
| `PublicHideReturnsNotImplementedOnNonDesktopHost` | `IWindow12::Hide()` returns `E_NOTIMPL`, visibility unchanged. |
| `PublicAndPrivateHideDispatchSeparately` | On one object, public `Hide()` is gated and `IWindowPrivate::Hide()` still succeeds. |
| `PrivateHideSucceedsWithoutSoundPlayerService` | Private `Hide` succeeds when no sound service was ever created. |
| `PrivateHideSucceedsWithSoundPlayerService` | Private `Hide` succeeds after `ElementSoundPlayer` forces the service to exist, and the service is still usable afterwards. |
| `PlacementMembersRemainAvailableOnNonDesktopHost` | The rest of `IWindow12` is not gated and reports "nothing persisted". |

Two limitations worth recording. `UWPWindowImpl::HideImpl` calls `TearDownAudioGraph()`, which a
test cannot observe directly, so the sound-service test verifies the service is still readable and
settable afterwards rather than asserting the teardown itself. And `Window.Visible` can legitimately
fail on the dummy window, so the tests snapshot `{succeeded, hr, visible}` and assert the snapshot is
identical before and after, which holds whether the read succeeds or fails.

**4. Null options are rejected at the public boundary.**

Writing these tests settled an open question about the SAL on the `options` parameter. The generated
`WindowGenerated::ShowWithOptions` and `WindowGenerated::TryApplyInitialPlacement` both start with
`ARG_NOTNULL(pOptions, "options")`, so an external caller passing null gets `E_INVALIDARG` before the
host gate runs. That is the enforced public contract, and `_In_` in `Window.g.h` matches it. The
`_In_opt_` on `Window_Partial`, `DesktopWindowImpl`, and `UWPWindowImpl` is also correct, because
`ShowImpl` calls `ShowWithOptionsImpl(nullptr)` internally to mean "no options". The two annotations
describe different layers and neither needs to change.

The [PM decision below](#required-options-decision-2026-09-24) retains this public boundary:
default hidden preparation requires an empty `WindowShowOptions`, not null or a new overload.

**5. The private path is deliberately ungated.**

`Window::ShowImpl` and `Window::HideImpl` (the `IWindowPrivate` path) forward straight to
`m_spWindowImpl` with no host check. That is intentional and preserves the existing `IWindowPrivate`
lifecycle contract. On a non-desktop host the forward still ends in `UWPWindowImpl::ShowImpl`, which
returns `E_NOTIMPL` on its own, so the observable result is unchanged from before this work.

Regression run: `*WindowPlacement*` on the VM, 170/171 pass. The one failure is the known
`FirstActivateUsesPersistenceConfiguration` flake, which passes when re-run alone.

### Required-options decision (2026-09-24)

**PM decision:** Keep `TryApplyInitialPlacement(WindowShowOptions options)` as the only hidden
application signature. Require non-null options for both it and `Show(options)`. Use
`TryApplyInitialPlacement(new WindowShowOptions())` for default hidden preparation and `Show()`
for default display. This resolves the SAL follow-up; it is not final public API review approval.

| Option | Benefit | Cost and decision |
| --- | --- | --- |
| Require a non-null options object; retain the current signatures | Both options methods have the same argument rule; matches the generated boundary and existing hidden-preparation examples | Default hidden preparation requires an object. Selected: hidden preparation already requires explicit lifecycle sequencing, and the default request has a direct expression. |
| Accept null as defaults | Avoids constructing an options object for default hidden preparation | Adds a second representation of defaults and changes public argument validation. Making only hidden application nullable would also make the two options methods inconsistent. Not selected. |
| Add `TryApplyInitialPlacement()` | Gives hidden preparation the same convenience overload as `Show()` | Adds a public overload and generated ABI surface only for shorthand; it does not add a placement capability. Not selected for this version. |

An empty options object has `Placement = null`, `Reason = Default`, and
`CascadeBehavior = Automatic`. With a non-empty id and automatic persistence enabled, it selects
a peer, saved value, or fallback under existing policy. It does not mean "saved placement only";
set `CascadeBehavior = Disabled` to avoid selecting a peer. No separate load is required.
Hidden application remains repeatable and does not enroll saving. A revealing first `Show`
still needs `SkipInitialPlacement = true` to preserve preparation.

Null **options** and null **Placement** have different meanings. The former is `E_INVALIDARG`
(`ArgumentException` in .NET) on every public options call, including late or reentrant calls.
The latter is a valid field value meaning no explicit placement. Generated required-argument
checks run before thread, closed-window, host, and coordinator checks. Only fields of a non-null
request can be ignored by late/reentrant policy. Rejection performs no placement, display,
storage, or enrollment work and does not consume the initial-placement phase.

The implementation may continue using null internally as a default request. Do not change the
public SAL to `_In_opt_` or expose that internal convention. Source inspection of
`WindowGenerated::ShowWithOptions` and `WindowGenerated::TryApplyInitialPlacement` confirms their
`ARG_NOTNULL` checks already implement the chosen boundary. The previous spec text promising
null-as-default behavior was inconsistent with those entry points and is now corrected.

**Developer handoff:** No signature or product behavior change is requested. Add public
projection coverage for both null arguments, rejection before side effects, a valid request
after rejection, and late/reentrant null rejection versus ignored fields in non-null requests.
Cover hidden preparation with no explicit placement, a known saved placement, and cascading
disabled; separately retain default peer-selection coverage. Check HRESULT/exception mapping in C# and C++/WinRT.
The existing native non-desktop Show test covers only one null-argument path; it does not
establish desktop hidden-application coverage. Run the new tests on the VM.

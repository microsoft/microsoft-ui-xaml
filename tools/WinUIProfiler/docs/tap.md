# Native TAP and architecture-matched injection

The native TAP, injector helper, strict native/managed codecs and managed client
are implemented. They authenticate an owned target, discover an optional private
profiler bridge and disconnect without terminating it. The [live controller and UI](app.md)
support packaged/unpackaged launch and PID attach with hello-gated connection.
This does not implement complete baseline/rundown.

The runtime's `RequestProfilerCapture` still returns `E_NOTIMPL`.
Rundown/lightweight-snapshot requests receive an explicit Unsupported response;
empty native capability lists do not advertise those features. Registry-only
observations and fake-runtime fixtures are not complete pre-attach enumeration.

## Injection boundary

`InjectorResolver` checks the live PID/creation-time identity and actual target
architecture using `IsWow64Process2`, including x64 emulation on ARM64. The
architecture-matched helper and TAP must be supplied explicitly. PE checks
reject mismatched architectures and DLL-versus-EXE payloads.

The helper independently checks target identity, current-user ownership,
architecture, trusted local paths and the exact loaded XAML/FrameworkUdk module
paths. Unsigned local binaries require the explicit trust option; neither the
UI nor the managed resolver loads the target runtime into its process.
Native dependency loading uses constrained DLL-directory/System32 search.

The helper invokes the verified `InitializeXamlDiagnosticsEx` export from the
target's loaded `Microsoft.Internal.FrameworkUdk.dll`, not an assumed XAML DLL
export. The endpoint is `WinUIVisualDiagConnection` plus an instance number,
starting at 1. Automatic multi-instance endpoint discovery is not implemented.

Helper arguments:

```text
<runtimeDll> <frameworkUdkDll> <tapDll> <endpointNumber> --trust-local-binaries
```

Initialization data is supplied over redirected standard input, not on the
command line. The helper reports schema-1 JSON with the unsigned HRESULT and
`injectionAccepted` and returns the HRESULT as its process exit code.
Acceptance means that the initializer accepted the request; only a subsequent
authenticated TAP handshake establishes a usable connection.

`SetSite` requires standard `IXamlDiagnostics`, not the private
`IWinUIProfilerRuntimeDiagnostics` bridge. A private-interface query returning
`E_NOINTERFACE` means those runtime features are unavailable; pipe creation,
authentication and the actual native hello handler still run. Other query
failures remain errors. When the bridge exists, strict `RuntimeInfo` validation
still runs. Without it, capabilities return `epoch:null` and an empty list,
never a fabricated runtime epoch or supported capture feature. Private capture
commands explicitly return Unsupported.

The consumed FrameworkUdk ABI stores initialization data in `WCHAR[MAX_PATH]`.
The helper therefore converts the managed legacy WUP1 bootstrap to WUP2 before
calling the initializer. WUP2 is exactly 165 characters: `WUP2|` followed by
standard Base64 of 120 bytes. It preserves all session/process-instance GUIDs,
PID/creation-time identities and the 32-byte nonce. Its pipe GUID derives the
canonical `WinUIProfiler-<GUID N>` name. GUID bytes use RFC 4122 order; integers
are little-endian. Decoders reject noncanonical or malformed input. TAP paths
must also fit the consumed ABI's `MAX_PATH` field.

## Pipe and lifetime

The target creates one unpredictable local-only named pipe with a protected
current-user DACL. Kernel-observed client PID, creation time and user identity
are checked before body authentication. The session, target identity and
cryptographic nonce must then match exactly.

Messages use a 28-byte little-endian binary header and strict typed UTF-8 JSON.
Bodies are bounded to 1 MiB; missing/duplicate/unknown fields, unsupported
versions, malformed UTF-8, oversized/truncated messages and reused request IDs
are rejected. A server accepts at most 4,096 distinct requests, with bounded
30-second native I/O operations; the managed caller also supplies a deadline
and cancellation token. A terminal acknowledgement is allowed a bounded client
drain rather than being discarded by an immediate pipe disconnect.

`Hello = 10` is a typed request/reply permitted only after authentication. Both
directions use the same request ID and exactly `{"message":"hello"}`; missing,
null, duplicate or extra fields and other message values are rejected.
`TapClient.HelloAsync(TimeSpan, CancellationToken)` returns `"hello"` only after
receiving the correlated Hello from the target's native `PipeServer`, never a
local greeting or injector acknowledgement. Any acknowledgement (including
Completed), other reply type or mismatched request ID fails explicitly and
closes the transport. Deadline, cancellation and pipe loss keep the existing
fail-closed exchange semantics. Hello counts toward the unchanged 4,096-request
limit; callers must account for that limit when scheduling periodic probes.
Each probe also satisfies the existing 30-second native idle read deadline;
the caller must schedule probes with margin below that deadline.

The framework releases its temporary TAP COM reference immediately after
`SetSite`. Consequently the COM object's destructor cannot own connection
shutdown. The worker owns its server/site lifetime, with a weak reverse link.
Explicit site removal signals cancellation; worker completion releases the site,
runtime interface, pipe and callbacks before COM apartment teardown.
The worker pins its DLL until cleanup and exits with `FreeLibraryAndExitThread`.
The fake-runtime host checks release/unload rather than merely observing EOF.
Automatic reconnect and arbitrary dispatcher/root capture are not qualified capabilities.

## Build and qualification

Use full Visual Studio MSBuild for the mixed solution. From
`tools\WinUIProfiler`, after building the matching local Products and resource
satellites described in [App deployment](app.md):

```powershell
.\scripts\Build-App.ps1 -Configuration Debug -Architecture x64 -Restore -Solution
.\scripts\Build-App.ps1 -Configuration Release -Architecture x64 -Restore -Solution
.\WinUIProfiler.Native.Tests\Run-TapProtocolTests.ps1 -Architecture x64
.\WinUIProfiler.Native.Tests\Run-TapComTests.ps1 -Architecture x64
.\WinUIProfiler.Native.Tests\Build-TapHost.ps1 -Architecture x64
dotnet test .\WinUIProfiler.IntegrationTests\WinUIProfiler.IntegrationTests.csproj -c Debug --no-restore --filter 'FullyQualifiedName~NativeTapProtocolTests|FullyQualifiedName~NativeInjectionTests'
```

Real injection tests start the actual self-contained App against both checked
and optimized local XAML, verify loaded module paths and deployment/source
SHA256 hashes, authenticate, query capabilities, disconnect and close the target
normally with exit code 0. Separate cases explicitly enable runtime ETW before
startup, so transport success without active producer hooks cannot hide a
producer fault. The fake-runtime host separately covers malformed requests,
nonce/client identity failures, duplicate IDs and deterministic unload. It does
not establish framework capture completeness.

`TargetConnectionTests` additionally qualify actual x64 unpackaged launch and
PID attach, packaged activation and PID attach to the registered fixture,
real native hello, target module hashes, separate listener/decoder/owner
threads, ETW receipt/persistence/reopen, cancellation, heartbeat past the
native idle deadline and target-exit drain. `Test-LiveConnectionUi.ps1` checks
the actual button-to-launch/hello/ETW/disconnect path. These use ordinary matched
local Release Product outputs, not interrupted diagnostic builds.

`Test-InstalledPackagedConnection.ps1` instead activates a supplied existing AUMID,
requires the actual injected TAP hello and standard XAML ETW receipt, disconnects,
and reopens the nonempty Stopped SQLite recording through the CLI. It neither
registers a fixture nor replaces the selected app's runtime. `-VerifyHeartbeat`
checks connection health beyond the 30-second native idle deadline.
`-LeaveConnected` then qualifies PID attach to that same surviving target and
leaves the profiler/target open.

ARM64 native compilation is a separate cross-build check. It is not native
ARM64 execution, and x64 success does not qualify ARM64 injection.

## TAP-owned XAML/composition tree snapshots

`TakeSnapshot = 11` requests `{"captureId":"<nonempty GUID>"}` after the
normal authenticated handshake. It does not call the product's unsupported
`RequestProfilerCapture`, use a product dump entry point, or depend on the old
profiler implementation. Standard `IXamlDiagnostics` is the integration
boundary, independent of private producer capabilities.

On success the TAP sends ordered `SnapshotChunk = 12` responses, each containing
`captureId`, zero-based `index` and canonical `dataBase64` for at most 48 KiB
of UTF-8 bytes. The final `SnapshotResult = 13` contains the exact `captureId`,
`chunkCount` and `byteCount`. Every frame retains the request ID. The complete
document is bounded to 64 MiB and 131,072 root/node/link records. These response
types cannot be sent as commands. Failure is a correlated terminal
Acknowledgement with explicit status/reason, not an empty successful snapshot.

The schema-1 document contains process PID/creation FILETIME, capture ID, QPC
frequency/global interval, root IDs/thread IDs/intervals, `xamlNodes`,
`compositionNodes`, `correlations` and `limitations`. Nodes have snapshot-local
IDs, root/parent IDs, child order, type and known properties. XAML objects have
object address, optional name/diagnostics handle; composition nodes have object
and visual-interface addresses. Links come from actual object-to-visual API
results, not geometry, names or guessed ancestry. Many-to-one links remain valid.
The managed client checks process instance, capture identity, all graph
references, root scope, parent cycles, duplicate IDs/links, bounds and intervals.

`TapClient.TakeSnapshotAsync` exclusively owns the pipe through the exact
terminal result. Partial/corrupt/cancelled exchanges fail closed. A completed
Unsupported/Failed acknowledgement preserves the healthy channel. The live
controller serializes heartbeats outside their five-second deadline while
capture runs. It publishes a unique JSON file next to its SQLite recording
only after complete validation, flush-to-disk and atomic rename.

Coverage is exposed realized visual state, not a private producer baseline.
Roots are independently captured on their UI threads, not globally atomic.
Public `GetElementVisual` may materialize handout visuals. Addresses/handles
are debugger metadata, not persistent lifetime IDs or invented core/HWCompNode
identities. See [the app guide](app.md#take-a-tree-snapshot) for usage.

## Baseline staging component

`BaselineCoordinator` stages one capture on the processing owner. It checks
Begin/End scope, stream/epoch, matching sequence vectors, real chunk indices,
chunk source times, counts and graph consistency. Reordered chunks and End
remain pending until all required input exists. Conflicting producer sequences,
duplicate chunks, foreign scopes and declared loss fail explicitly. Staging
stops at 8,192 unique source facts or 16 MiB of encoded-source/index accounting;
it does not evict history to make a capture appear successful.

`Prepare` additionally requires actual listener-domain intervals and completed
processing receipts, or qualified closed-source evidence. It reuses
`SequenceContinuity`; an End, provider flush, queued callback or largest seen
sequence is not proof. Exact duplicate deltas are idempotent. Facts delivered
before Begin and beyond End's cut retain their actual source identity and time.
Selected metadata domains also require real receipts, not staging alone.
Metadata staged before canonical processing does not reserve processing
ordinals; receipt checkpoints must identify their actual processed source.

A `PreparedBaseline` proves **delivery and assembly only**. Its buffered deltas
are not yet reconciled, and its original End is not permission to publish with
an old or acquisition-local ordinal. The owner still must defer/rebind the real
End before canonical assignment, reconcile scope-specific deltas and atomically
publish within storage limits. This component is not wired into recording,
does not implement runtime traversal and does not make rundown available.

Focused staging and dependent graph-publication tests:

```powershell
dotnet test .\WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj -c Release --no-restore --filter 'FullyQualifiedName~BaselineCoordinatorTests|FullyQualifiedName~ObjectStateTests|FullyQualifiedName~XamlTreeTests|FullyQualifiedName~CompositionTreeTests|FullyQualifiedName~ResourceUsageTests'
```

## Atomic baseline storage component

`SessionWriter.CommitBaseline` persists the real next-canonical baseline End and
bounded projection pages in one SQLite transaction. The checkpoint, End, original
source provenance/clock mapping, derived rows and quality interval become visible
together; a concurrent reader retains
the previous committed watermark until the transaction commits. Ordinary
`Commit` limits are unchanged.

Each nonempty page is limited to 4,096 records and 16 MiB of accounted output.
Each record is limited to 64 KiB of serialized value. A whole publication is
limited to 131,072 derived records and 64 MiB, including its encoded End, frozen
session/quality metadata, validated provenance and row-accounting headers. These are serialization
budgets, not predictions of SQLite file size; the configured database-page quota
also applies. Only the object, XAML, composition, correlation, resource,
`FrameInfo` and `StartupEntry` projection DTOs are accepted, and every change
must reference this End. `MetricSample` and snapshot/session results remain
rejected. Frame's ordinary End-time revisions retain their historical ranges
and change provenance; End is not a fabricated frame observation.
Cross-page duplicate projection keys fail rather than overwrite another page.

Optional End provenance preserves the original target identity and QPC, including
a trace-local identity distinct from the canonical session identity when their
actual target PID agrees. A source clock must map that original QPC to the End's
normalized timestamp at the session frequency. Invalid provenance is rejected;
new clock metadata and provenance roll back with the rest of the publication.

Cancellation, invalid pages and producer-enumeration errors roll back the whole
publication. Storage failures also use the existing reserved failure record and
prevent recording from resuming. Tests exercise real concurrent readers,
reopening, the exact aggregate byte limit, bounded pages/records, provenance
validation, rollback and SQLite `SQLITE_FULL`;
they do not substitute a mock transaction for these behaviors.

This is a storage primitive, not baseline verification or reconciliation.
`BaselinePagePreparer` now produces immutable owned pages on the owner, using one
aggregate projection budget for the exact five ordinary processors. Verified
processors call `PublishBaseline`; Frame processes only the actual singleton End,
including retained quality/invalidation revisions. The caller must already have
validated the next-canonical End and reconciled owned chunks; `PreparedBaseline`
alone remains insufficient authority. Preparation is single-attempt and a failed
attempt requires discarding all involved processor state.

The coordinator/reconciliation handoff and writer queue are **not integrated**
yet; the SQLite thread must not enumerate mutable processor state. The ordinary
4,096-record projection budget cannot be reused unchanged for a large baseline.
Nor may the owner publish a fabricated End or reuse an already committed source
ordinal to bypass the next-canonical-source requirement.

```powershell
dotnet test .\WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj -c Release --no-restore --filter 'FullyQualifiedName~AtomicBaselineTests'
```

# Retail-compatible TAP transport

This standalone DLL uses the public Windows SDK `IXamlDiagnostics` initialization
site. It does not query a private WinUI interface or require an instrumented
runtime, WinRT projections, or product builds. Its COM class ID and two DLL
exports match the injector.

## Wire contract

- Preserve WUP2 compact bootstrap (165 characters) and legacy WUP1 parsing.
- Frames have a 28-byte header: body length (LE32), protocol version (LE16,
  `1`), message type (LE16), request GUID (16 bytes, network order), schema
  version (LE32, `1`), followed by bounded UTF-8 JSON.
- Authenticate first with message `1` containing `sessionId`, target `process`
  (`processId`, `creationTimeFileTime`, `instanceId`), `schemaVersion`, and
  `nonceBase64`. Successful authentication returns message `8`, status
  `Completed`. A second handshake is rejected.
- Message `10`, `{"message":"hello"}`, returns that same type and JSON.
- Message `2`, `{"schemaVersion":1}`, returns message `9`,
  `{"epoch":null,"schemaVersion":1,"capabilities":[]}`.
- Valid capture, tree snapshot, graph, and cancellation requests explicitly
  return message `8`, status `Unsupported`; no work is fabricated.
- Message `7`, `{"reason":"done"}`, returns message `8`, status `Completed`.
  The controller must consume the acknowledgement and close its pipe. The
  server waits for that close instead of discarding its final response.
- Invalid commands, authentication failures, and duplicate request IDs are
  rejected and terminate the connection. Malformed headers terminate directly.

Every transfer/connection has a cancellable 30-second deadline. A controller
that remains connected while graphing should send hello more frequently than
every 30 seconds (for example every 10 seconds), even though process metrics
are collected independently.

Request replay protection is a fixed 8 MiB, seven-probe Bloom filter, with no
eviction or total request-count limit. Previously seen IDs are always rejected.
Conservative false-positive rejection is possible (approximately 1e-7 per
request at one million IDs); a rejected session must be re-established with a
fresh bootstrap and nonce. Exact, unbounded arbitrary-GUID replay detection
cannot use bounded memory without changing the wire protocol.

## Explicit trust boundary

Only inject into an explicitly selected, same-user process with matching
creation time and architecture. The target independently checks its bootstrap
identity, restricts its first-instance local pipe to its user SID, rejects
remote clients, and checks the controller PID, creation time, user SID, and
nonce. The controller must independently check the named-pipe server PID and
creation time against its selected target before sending the nonce.

The helper loads caller-approved native binaries. `--trust-local-binaries` is
an explicit opt-in to that trust boundary, not a signature check. Keep the
bootstrap secret; never include it in diagnostic logging.

## Build and tests

Use VS 18 MSBuild (`v145`), or VS 17 MSBuild (`v143`), with SDK
`10.0.26100.0`. Each project supports x64/ARM64, Debug/Release, and static CRT.
Local `Directory.Build.props` and `.targets` prevent managed/product defaults
from leaking into these standalone native projects.

From the repository root, for each project:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe' tools\WinUIProfiler\WinUIProfiler.Tap\WinUIProfiler.Tap.vcxproj /nologo /v:minimal /p:Configuration=Release /p:Platform=x64
& 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe' tools\WinUIProfiler\WinUIProfiler.Injector\WinUIProfiler.Injector.vcxproj /nologo /v:minimal /p:Configuration=Release /p:Platform=x64
& 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe' tools\WinUIProfiler\WinUIProfiler.Tap.Tests\WinUIProfiler.Tap.Tests.vcxproj /nologo /v:minimal /p:Configuration=Release /p:Platform=x64
& .\tools\WinUIProfiler\WinUIProfiler.Tap.Tests\bin\x64\Release\WinUIProfiler.Tap.Tests.exe
```

Tests cover framing/bootstrap, authentication and nonce mismatch, malformed
commands, 100,000 replay-guard identities, 5,000 live pipe hello requests,
unsupported commands, client PID/creation ownership, delivered disconnect
acknowledgement, and overlapped connection/partial-read cancellation. These
do not qualify injection into a real packaged or unpackaged WinUI app.

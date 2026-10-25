# Saved-session query CLI foundation (bounded P16)

`WinUIProfiler.Cli` is a UI-independent `net10.0` executable over the existing
`SessionStore` and twelve `IProfilerQueryService` operations. It opens only
already-persisted, local, **closed and checkpointed** SQLite sessions. It does
not capture, attach, inject, contact a network service, execute SQL, create a
database, migrate a database, or issue target commands.

This is the read-only P16 foundation, not completion of P16 or the overall
plan. P12/P13/P14 snapshot/heap/ownership implementations and end-to-end data
producers remain open. Snapshot metadata is returned only if already persisted;
empty selections retain explicit unavailable quality. `compare` and `retention`
currently return `unsupported_query` because their engine processors are not
implemented. No baseline, delta, heap object or retention path is synthesized.

## Build and run

From `tools\WinUIProfiler`, with the pinned .NET 10 SDK:

```powershell
dotnet restore WinUIProfiler.Cli\WinUIProfiler.Cli.csproj --locked-mode
dotnet build WinUIProfiler.Cli\WinUIProfiler.Cli.csproj -c Release --no-restore
dotnet .\WinUIProfiler.Cli\bin\Release\net10.0\WinUIProfiler.Cli.dll session --db C:\captures\saved.sqlite
dotnet .\WinUIProfiler.Cli\bin\Release\net10.0\WinUIProfiler.Cli.dll metrics --db C:\captures\saved.sqlite --start-qpc 0 --end-qpc 10000000 --metric ProcessPrivateCommit --limit 128
```

The standalone solution includes CLI and CLI tests with Debug/Release mappings
for Any CPU, x64 and ARM64. These managed projects have no WinUI/UI dependency.
Mixed native/app solution builds still require the documented Visual Studio
MSBuild environment; a CLI build does not qualify the native or app artifacts.

## Argument contract

Syntax: `<command> --db <local-path> [--option <value> ...]`.
The command and option names are exact, lowercase tokens. Options may be in
any order after the command. Every option occurs at most once and has exactly
one separate value: no positional extras, `--name=value`, switches or arbitrary
SQL are accepted. Quote paths containing spaces.

| Command | Required additional options | Optional selection options |
| --- | --- | --- |
| `session` | None | None (not paged) |
| `metrics` | `--start-qpc`, `--end-qpc` | `--metric` (`MetricKind` name) |
| `startup` | None | None |
| `frames` | `--start-qpc`, `--end-qpc` | None |
| `object-history` | Range plus object identity | None |
| `xaml-tree` | `--at-qpc` | Object identity selects the root |
| `composition-tree` | `--at-qpc` | Object identity selects the root |
| `correlations` | `--at-qpc` plus object identity | None |
| `resources` | `--start-qpc`, `--end-qpc` | Object identity selects the owner |
| `snapshots` | None | `--kind` (`SnapshotKind` name) |
| `compare` | `--baseline-id`, `--snapshot-ids` | None |
| `retention` | `--snapshot-id`, `--maximum-depth`, object identity | None |

Every command except `session` also accepts `--limit`, `--offset`,
`--watermark-ordinal` and `--watermark-qpc`.

* A **range** requires both nonnegative signed 64-bit QPC bounds with
  `end >= start`. These are actual counter ticks, not milliseconds, durations
  or a fabricated frequency. Read `session.result.value.qpcFrequency` before
  converting time. Engine range semantics apply: ordinary ranges are
  half-open; equal bounds select the instant. Frame queries include overlaps;
  resource queries retain the engine's point-in-time-at-range-end semantics.
  Tree/correlation `--at-qpc` must not exceed the selected committed timestamp.
* **Object identity** is all three of `--domain`, `--producer-epoch`,
  `--runtime-id`, even for optional root/owner filters. The process instance
  and session identity always come from the database, never a caller's PID.
  Domains are `XamlCore`, `XamlPeer`, `CompNode`, `CompositionVisual`, `Resource`.
  Runtime IDs are positive unsigned 64-bit **decimal** integers; epochs are
  nonempty hyphenated GUIDs. XAML roots require `XamlCore`; composition roots
  require `CompNode` or `CompositionVisual`.
* Enum values are symbolic contract names, case-insensitive; numeric enum
  values, undefined values and combined names are rejected. For example,
  metrics accept `ProcessPrivateCommit` and `CpuUtilization`; snapshots accept
  `Lightweight`, `NativeHeap`, `ManagedHeap`, `OwnershipGraph`. A filter does
  not imply that its data producer is available.
* All snapshot IDs are nonempty hyphenated GUIDs. `--snapshot-ids` is one
  comma-separated list of 1–64 distinct IDs, excluding the baseline.
  `--maximum-depth` is 1–256.
* `--limit` defaults to 128 and is 1–4096. `--offset` defaults to zero and is
  nonnegative/nonoverflowing. The two watermark fields must appear together.
  A nonzero offset **requires** that pair.
* All integer options use invariant decimal digits only, without signs,
  grouping, whitespace, exponent or hexadecimal notation. Unknown, repeated,
  missing, incomplete and out-of-range options fail before database opening.
* At most 65 arguments, 4096 UTF-16 code units per argument and 32768 total
  code units are accepted, also subject to the OS command-line limit.
* UNC/device paths, URI paths, mapped network drives, alternate data streams
  and filesystem reparse-point paths (including sidecars) are not supported.
  Windows read-only sharing leases exclude writers while a query runs.
  Active writers and **nonempty WAL files are rejected**, not recovered,
  checkpointed or silently ignored. Close/checkpoint the session using its
  owning writer before querying. The CLI itself never performs that operation.

For example, a qualified history request:

```powershell
dotnet .\WinUIProfiler.Cli\bin\Release\net10.0\WinUIProfiler.Cli.dll object-history --db C:\captures\saved.sqlite --start-qpc 100 --end-qpc 200 --domain XamlCore --producer-epoch 12345678-1234-1234-1234-123456789abc --runtime-id 18446744073709551615
```

## JSON, paging, errors and cancellation

A successful invocation writes exactly one compact JSON document and a newline
to stdout. Stderr is empty:

```json
{"schemaVersion":1,"command":"metrics","result":{"context":{"scope":{"sessionId":"...","process":{"processId":42,"creationTimeFileTime":null,"instanceId":"..."}},"watermark":{"ingestOrdinal":3,"timestampQpc":30},"quality":{"kind":"Estimated","coverage":"Partial","reason":"..."}},"items":[],"page":{"limit":128,"offset":0,"nextOffset":null,"hasMore":false}}}
```

`result` is the typed query result without dropping its scope, watermark,
quality, rows or paging. `session` uses `value` instead of `items`/`page`.
Property names are camelCase; enums are strings. Polymorphic object observations
retain their contract discriminators. Unsigned 64-bit IDs are exact JSON
integers; consumers must use an integer-preserving parser.

For the next page, repeat the **same command and selection options**, use
`page.nextOffset`, and copy both fields from `result.context.watermark` into
`--watermark-ordinal` and `--watermark-qpc`. Do not replace that watermark with
a newer session watermark. The engine rejects unknown/inconsistent boundaries
and freezes continuation at the original committed view.

The engine's 16 MiB materialization budget and 1 MiB individual persisted payload
bound still apply. The CLI additionally buffers a maximum 16 MiB JSON document
(plus its terminating newline), rejecting overflow before publishing success
output. Reduce `--limit` if a page exceeds those budgets.

Failures write one bounded JSON document to stderr, no successful stdout and
no exception stack dump:

```json
{"schemaVersion":1,"command":"compare","error":{"code":"unsupported_query","message":"This operation requires later snapshot/ownership processors; no data was synthesized."}}
```

`command` is null if parsing never produced a valid invocation. Error messages
are stable, bounded explanations, not database paths or raw exception details.

| Exit code | Error code | Meaning |
| --- | --- | --- |
| 0 | None | Query completed |
| 2 | `invalid_arguments` | Invalid syntax, typed selection or continuation |
| 3 | `database_error` | Missing, corrupt, incompatible, linked, active or uncheckpointed database; read-only opening failed |
| 4 | `unsupported_query` | Later engine snapshot/ownership implementation needed |
| 5 | `query_error` | Query/materialization/JSON/output failure |
| 6 | `cancelled` | Cancellation observed; no successful result |

Ctrl+C is translated into a cancellation token and forwarded into the typed
query call. Cancellation exits nonzero, including pre-cancelled invocations.
Opening/schema validation is synchronous; cancellation is rechecked before
the query and before output. Forced process termination cannot guarantee a
structured error or atomic terminal/pipe writes.

## Focused verification

```powershell
dotnet restore WinUIProfiler.Cli.Tests\WinUIProfiler.Cli.Tests.csproj --locked-mode
dotnet test WinUIProfiler.Cli.Tests\WinUIProfiler.Cli.Tests.csproj -c Release --no-restore --filter FullyQualifiedName~CliContractTests
```

The separate test project isolates CLI work from concurrent baseline test
changes. It uses real saved SQLite fixtures, executes the CLI as subprocesses
(including session, metrics and invalid arguments), checks all routes and
typed request fields, verifies continuation across a later saved commit,
forwards cancellation into every real query route, and compares database and
sidecar bytes. It deliberately does not run ETW/capture/native/UI suites.

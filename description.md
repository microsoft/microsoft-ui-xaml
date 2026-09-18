# WinUI binary size reductions

These changes reduce repeated implementation code in `Microsoft.ui.xaml.dll`.
They do not change public interfaces, metadata, compiler/linker options, or
security settings. Measurements use the x64 Release build with PGO disabled.

## Starting commit summary (`a8b8ae4c1`)

The starting commit combines the three measured reductions below and adds seven
regression tests across the ValueBoxer and COM suites.

| Change included in this commit | Additional DLL savings |
|---|---:|
| Direct `QueryInterface` during unboxing | 512 bytes |
| Direct HSTRING creation for boxed-value class names | 12,800 bytes |
| Direct HSTRING creation in `INSPECTABLE_CLASS` | 97,792 bytes |
| Total | 111,104 bytes (108.5 KiB) |

The measured DLL size falls from **14,535,168 to 14,424,064 bytes** with the
same x64 Release `/nopgo` configuration. The commit also makes the boxed-value
name tests use the production macro rather than a copied implementation.
Detailed measurements, coverage, and limitations follow.

## Changes

### 1. Query directly into an empty smart pointer

File: `dxaml\xcp\components\valueboxer\inc\IValueBoxer.h`

In the generic `IValueBoxer::UnboxValue` implementation, replace
`ctl::do_query_interface(spObjAsRef, box)` with a direct `QueryInterface` call
using `IID_PPV_ARGS(spObjAsRef.ReleaseAndGetAddressOf())`.

The destination is a freshly created, empty `ComPtr`. Querying directly into
it avoids the general helper's intermediate smart pointer and move. The
general helper remains unchanged because other callers may have an existing
destination whose lifetime must be preserved during the query.

Null checks, the requested interface, `get_Value`, HRESULT handling, and
temporary interface release remain unchanged.

**Measured DLL reduction: 512 bytes.** A separate LTCG relink reproduced the
same file and section sizes.

### 2. Create boxed-value runtime class names directly

File: `dxaml\xcp\dxaml\lib\DXamlTypes.h`

In `REFERENCE_ELEMENT_NAME_IMPL`, replace construction of an
`HStringReference` followed by `CopyTo` with:

```cpp
IFC(WindowsCreateString(
    STR_LEN_PAIR(L"Windows.Foundation.IReference`1<" NAME L">"),
    pClassName));
```

The old path created a temporary fast-pass string and then copied it into an
owned HSTRING. The new path creates the owned HSTRING directly. The returned
characters, length, ownership, and ordinary HRESULT error handling remain
unchanged.

The temporary setup appeared across 199 non-folded function representatives.
`ReferenceBase<T>::GetRuntimeClassNameImpl` code attributed by SizeBench fell
from 22,487 to 10,149 bytes. A representative body shrank from 113 to 51 bytes.
The engine's representative count remained 199: this simplifies template bodies,
rather than enabling additional identical-code folding.

**Measured DLL reduction: 12,800 bytes (12.5 KiB).** The reduction is in
`.text`. Function padding and section alignment make whole-file savings
different from the sum of attributed function bytes.

### 3. Create general inspectable runtime class names directly

File: `dxaml\xcp\components\com\inc\ComMacros.h`

Apply the same simplification to `INSPECTABLE_CLASS`, which supplies runtime
class names for generated and handwritten COM classes:

```cpp
return WindowsCreateString(NAME, SZ_COUNT(NAME), className);
```

This preserves the existing name and length expression. It removes temporary
string-reference setup without changing interface maps, object layout, or
aggregation. Generated class headers do not need edits.

The `ComObject<T>::GetRuntimeClassName` template family fell from 72,746 to
32,588 attributed bytes, with the engine's non-folded representative count
unchanged at 785. The representative `Image` implementation shrank from 114
to 45 bytes. It now tail-calls the string API without reserving stack space;
its outer-object delegation remains intact.

**Measured DLL reduction: 97,792 bytes (95.5 KiB).** Raw `.text` bytes fell
83,968 and `.pdata` bytes fell 13,824. Smaller wrappers also reduce x64
unwind metadata. These section measurements, not a sum of template estimates,
account for the whole-file reduction.

## Measured results

All values below are bytes. Each row includes the preceding changes.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| Original baseline | 14,535,168 | 14,534,144 | 14,544,776 |
| Direct `QueryInterface` | 14,534,656 | 14,533,632 | 14,544,212 |
| Direct runtime class name creation | 14,521,856 | 14,520,832 | 14,531,476 |
| Direct general inspectable class names | 14,424,064 | 14,423,040 | 14,433,688 |
| Total reduction | 111,104 | 111,104 | 111,088 |

The DLL is **111,104 bytes (108.5 KiB) smaller** overall. These are measured
results from preserved DLL/PDB pairs, not template-foldability estimates.
Baseline and candidate comparisons use the same frozen SizeBench deployment
within each experiment.

Build initialization and compilation run in the same command process:

```bat
call init.cmd amd64fre /nopgo /envcheck /notitle && call Build.cmd mux /q
```

Compiler, linker, and resource compiler command logs match the original
baseline. All eight exported ordinal/name identities are unchanged.

## Regression coverage and limitations of the starting commit

The isolated suites have 17 passing tests: 13 ValueBoxer and four COM tests.
New coverage includes:

- Unboxing scalar and struct references with different interface IDs.
- Null arguments, failed queries, failed `get_Value` calls, successful
  `S_FALSE` normalization, and temporary reference release.
- Struct and enum runtime class names, exact lengths, null output errors,
  repeated calls, string duplication, and ownership after releasing the box.
- General inspectable names, empty names, UTF-16 and embedded-NUL contents,
  and delegating versus non-delegating names for aggregated objects.

The new checks passed against both their original and changed implementations.
The runtime class name tests now include the production macro from
`DXamlTypes.h` instead of maintaining a separate copy in `Stubs.cpp`.

In the latest run, an alternating string-creation API-path microbenchmark
improved from a median of 58.32 to 50.68 ns per create/delete operation.
It uses a representative string, not the full COM call path. The earlier unboxing
microbenchmark was noisy and did not establish performance equivalence.
Neither benchmark measures application-level performance.

No UI interaction, application startup, whole-application memory, OOM fault
injection, x86, or ARM64 validation has been performed. Neither runtime class
name candidate has had a second independent relink.

## Follow-up: transfer newly created typed references directly

File: `dxaml\xcp\components\valueboxer\inc\Value.h`

In `PropertyValue::CreateTypedReference<T>`, replace the temporary interface
smart pointer and `QueryInterface` with:

```cpp
*ppValue = ctl::interface_cast<wf::IReference<T>>(ref.Detach());
```

The factory creates a concrete `Reference<T>` with no controlling outer object.
Its `QueryInterfaceImpl` returns exactly this embedded `IReference<T>` forwarder.
The cast transfers the factory's existing owning reference instead of adding
one through `QueryInterface` and releasing the factory reference afterward.
The caller still receives one owning reference to the same interface.

The output-pointer check, allocation, initialization, `SetValue`, failure
cleanup, and `S_OK` return remain unchanged. This includes the fallible HSTRING
duplication used by the `TypeName` value specialization. General-purpose
interface queries and enum-reference creation remain unchanged. No public
interface, class layout, generated source, or metadata definition changes.

### Measurements

All values are bytes, using the same x64 Release build with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| Starting commit, fresh baseline | 14,424,064 | 14,423,040 | 14,433,688 |
| Direct typed-reference transfer | 14,419,456 | 14,418,432 | 14,428,832 |
| Incremental reduction | 4,608 | 4,608 | 4,856 |
| Cumulative reduction from original baseline | 115,712 | 115,712 | 115,944 |

This change saves **4,608 bytes (4.5 KiB)** in the actual DLL file. Cumulative
file savings are **115,712 bytes (113 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 4,096 bytes and `.rdata` by 512 bytes.

As supporting attribution, SizeBench reports the two out-of-line
`CreateTypedReference` representatives falling from 555 to 315 bytes.
The `CreateReference` family, which can inline this helper, falls from 7,187
bytes across 24 representatives to 2,572 bytes across 15 representatives.
These family totals are not summed or substituted for whole-file savings.

The original source was restored byte-for-byte and rebuilt, reproducing all
three baseline metrics. Reapplying the candidate and rebuilding reproduced all
three candidate metrics. The restored baseline and repeated candidate both
compiled affected code and performed an LTCG link; these were not no-op
incremental builds.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-162516-8800db1c`.
The `000-baseline`, `001-direct-typed-reference`, `002-baseline-rebuilt`, and
`003-direct-typed-reference-repeat` directories contain read-only DLL/PDB
copies, SHA-256 hashes, source patches, build logs/binlogs, SizeBench snapshots,
query results, and tool identity sidecars. `verification.json`,
`repeat-command-comparison.json`, `export-security-comparison.json`,
`metadata-comparison.json`, and `progress.json` record the comparison and
handoff.

All measurements use the frozen deployment at
`D:\SizeBench\artifacts\cli-frozen\winui-template-folding-20260918-1428`.
Its managed identity is
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
The full deployment manifest and file hashes were verified on both sides.
Initialization and build used the same command process:

```bat
call init.cmd amd64fre /nopgo /envcheck /notitle && set PGOBuildMode && set Configuration && set Platform && set VCToolsVersion && call Build.cmd mux /q
```

Logs confirm `PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. All 430 command-tlog hashes match between the
rebuilt control and repeated candidate. Eight export ordinal/name identities,
PE security flags, and five built WinMD file hashes are unchanged.

**Tests not run, as requested.** The regression results above belong to the
starting commit, not this follow-up. Source review found that the cast uses
the same forwarder and transfers the same ownership. It removes interface
dispatch and a reference-count increment/decrement pair without introducing
new runtime work. Compilation does not prove runtime correctness, and this
turn did not measure application performance, exercise failure injection, or
validate other architectures.

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

## Follow-up: compact enum interface-ID comparisons

File: `dxaml\xcp\components\valueboxer\inc\Value.h`

In `EnumReference<T>::QueryInterfaceImpl`, replace the three
`InlineIsEqualGUID` checks with fixed-size `std::memcmp` equality checks:

```cpp
std::memcmp(&iid, &__uuidof(wf::IReference<T>), sizeof(IID)) == 0
```

Both forms compare all 16 bytes of the GUID for equality. The Windows SDK's
ordinary `IsEqualGUID` implementation also uses `memcmp`; the explicit call
here avoids its possible `__INLINE_ISEQUAL_GUID` macro override. The three
requested interfaces retain their original order: `IReference<INT>`,
`IPropertyValue`, then `IReference<T>`. The returned forwarders, `AddRefOuter`,
base-class fallback, and HRESULTs are unchanged. The existing omission of
`IReference<INT>` from `GetIids` is also unchanged.

The change affects only the handwritten enum template. It does not change
interface maps, class layout, generated source, public interfaces, or metadata.
No helper extraction or compiler/linker option change is involved.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| Previous accepted commit, fresh baseline | 14,419,456 | 14,418,432 | 14,428,832 |
| Fixed-size enum GUID comparisons | 14,410,752 | 14,409,728 | 14,420,048 |
| Incremental reduction | 8,704 | 8,704 | 8,784 |
| Cumulative reduction from original baseline | 124,416 | 124,416 | 124,728 |

The actual DLL file is **8,704 bytes (8.5 KiB) smaller**. Cumulative file
savings are **124,416 bytes (121.5 KiB)** from the original 14,535,168-byte
baseline. Only `.text` changes: its raw size falls by 8,704 bytes and its
virtual size by 8,784 bytes. Other section sizes remain unchanged.

As supporting attribution, the enum `QueryInterfaceImpl` family falls from
39,162 to 32,025 attributed bytes, with 183 representatives on both sides.
The resolved `HoldingState` primary code block shrinks from 214 to 175 bytes.
Its disassembly replaces four 32-bit comparisons per GUID with two 64-bit
comparisons. There is no out-of-line `memcmp` call in that representative.
Family totals are not summed with section savings. Code layout and alignment
make the whole-file reduction different from the attributed family reduction.

Restoring the original source byte-for-byte and rebuilding reproduced all
three baseline metrics. Reapplying the exact candidate and rebuilding
reproduced all three candidate metrics. Each of these non-baseline builds
compiled affected code and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-165053-b738dcfa`.
The `000-baseline`, `001-enum-guid-memcmp`, `002-baseline-rebuilt`, and
`003-enum-guid-memcmp-repeat` directories contain preserved read-only DLL/PDB
pairs, hashes, source patches, build logs/binlogs, SizeBench snapshots,
query receipts, stderr, and tool identity sidecars. The first baseline and
candidate also have targeted enum-family reports and resolved disassembly.
`verification.json`, `repeat-command-comparison.json`,
`export-security-comparison.json`, `metadata-comparison.json`, and
`progress.json` record validation and the handoff.

All four measurements use the same frozen SizeBench deployment and managed
identity documented above. The complete deployment manifest and file hashes
were verified before and after each measurement. Initialization and build
used the same `cmd.exe` process with explicit `/nopgo` and the same command
documented in the typed-reference follow-up.

Logs confirm `PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`,
and `VCToolsVersion=14.44.35207`. All 934 captured command-tlog hashes match
between the rebuilt control and repeated candidate. Eight export ordinal/name
identities, PE security flags, and five built WinMD file hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this follow-up. Source review found no ownership,
interface-identity, error-handling, or threading change. The fixed-size
comparison introduces no allocation or additional function call in the
inspected code. It still short-circuits mismatches, but comparison width,
loads, and branch layout differ; application-level performance has not been
measured. Compilation does not establish runtime correctness. Runtime failure
injection and other architectures were not validated.

## Follow-up: compact general reference interface-ID comparisons

File: `dxaml\xcp\components\valueboxer\inc\Value.h`

Apply the fixed-size GUID comparison used by `EnumReference<T>` to the two
checks in `Reference<T>::QueryInterfaceImpl`:

```cpp
std::memcmp(&iid, &__uuidof(wf::IPropertyValue), sizeof(IID)) == 0
std::memcmp(&iid, &__uuidof(wf::IReference<T>), sizeof(IID)) == 0
```

Both comparisons still check all 16 GUID bytes. `IPropertyValue` remains
first, followed by `IReference<T>`. The same interface forwarders,
`AddRefOuter`, inherited fallback, output writes, and HRESULTs remain intact.
No interface map, class layout, metadata, generated output, or build option
changes. This experiment changes only `Reference<T>`, not the previously
optimized `EnumReference<T>` implementation.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `49ac77e6b`, fresh baseline | 14,410,752 | 14,409,728 | 14,420,048 |
| Fixed-size general reference GUID comparisons | 14,410,240 | 14,409,216 | 14,419,648 |
| Incremental reduction | 512 | 512 | 400 |
| Cumulative reduction from original baseline | 124,928 | 124,928 | 125,128 |

The actual DLL file is **512 bytes (0.5 KiB) smaller**. Cumulative file
savings are **124,928 bytes (122 KiB)** from the original 14,535,168-byte
baseline. Only `.text` changes: raw size falls by 512 bytes and virtual size
by 400 bytes. Other section sizes remain unchanged.

The `Reference<T>::QueryInterfaceImpl` family falls from 3,881 to 3,606
attributed bytes, with 25 representatives in both builds. The resolved
`CornerRadius` primary code block shrinks from 155 to 144 bytes. Its
disassembly replaces four 32-bit comparisons per GUID with two 64-bit
comparisons, without adding an out-of-line `memcmp` call. Different branch
and pointer-conversion layout limits the saving. Family totals are supporting
attribution, not added to section savings.

Restoring the original source byte-for-byte and rebuilding reproduced all
three baseline metrics. Reapplying the exact candidate and rebuilding
reproduced all three candidate metrics. Each non-baseline build compiled
affected code and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-171827-641a0e07`.
The `000-baseline`, `001-reference-guid-memcmp`, `002-baseline-rebuilt`, and
`003-reference-guid-memcmp-repeat` directories contain read-only DLL/PDB
pairs, hashes, source revisions and patches, build logs/binlogs, SizeBench
snapshots, receipts, stderr, and frozen tool identity sidecars. The first
baseline and candidate also include targeted family reports and resolved
disassembly. `verification.json` and `progress.json` record the comparison
and handoff.

All measurements use the same frozen deployment and managed identity
documented above. The full deployment manifest and file hashes were verified
before and after each measurement. Initialization and build used the same
`cmd.exe` process and explicit `/nopgo` recipe documented above. Logs confirm
`PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. All 934 captured command-tlog hashes match
between the rebuilt control and repeated candidate. Eight export ordinal/name
identities, PE security flags, and five built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier test results belong to the starting
commit, not this experiment. Source review found no ownership, interface
identity, error handling, or threading change. The inspected code introduces
no allocation or additional function call. Comparison width, loads, and
branch layout differ; application performance has not been measured.
Compilation does not establish runtime correctness. Failure injection and
other architectures were not validated.

## Follow-up: compact collection interface-ID comparisons

File: `dxaml\xcp\dxaml\lib\JoltCollections.h`

In `PresentationFrameworkCollectionTemplateBase<T>::QueryInterfaceImpl`,
replace its three `InlineIsEqualGUID` checks with fixed-size `std::memcmp`
equality checks, following the reference-template changes above:

```cpp
std::memcmp(&iid, &__uuidof(wfc::IVectorView<T>), sizeof(IID)) == 0
```

The comparisons still check all 16 GUID bytes. Matching remains ordered as
`IVectorView<T>`, `IVector<T>`, then `IIterable<T>`. The same `static_cast`
expressions return the same interface pointers. Successful matches still call
`AddRefOuter`; unmatched requests still delegate to the inherited
`QueryInterfaceImpl`. Output writes and HRESULTs remain unchanged.

This change affects only this handwritten template and adds its explicit
`<cstring>` include. Other collection query implementations, interface maps,
reference tracking, class layouts, generated outputs, and metadata are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `3eae16d9a`, fresh baseline | 14,410,240 | 14,409,216 | 14,419,648 |
| Fixed-size collection GUID comparisons | 14,407,168 | 14,406,144 | 14,416,784 |
| Incremental reduction | 3,072 | 3,072 | 2,864 |
| Cumulative reduction from original baseline | 128,000 | 128,000 | 127,992 |

The actual DLL file is **3,072 bytes (3 KiB) smaller**. Cumulative file savings
are **128,000 bytes (125 KiB)** from the original 14,535,168-byte baseline.
Only `.text` changes: its raw size falls by 3,072 bytes and its virtual size
by 2,864 bytes. Other section sizes remain unchanged.

The template's `QueryInterfaceImpl` family falls from 9,870 to 7,050
attributed bytes, with 47 representatives in both builds. The resolved
`ColumnDefinition` primary code block shrinks from 210 to 150 bytes.
Disassembly shows two 64-bit comparisons per GUID instead of four 32-bit
comparisons, without an out-of-line `memcmp` call. These family totals support
attribution; they are not added to section savings.

Restoring the original source byte-for-byte and rebuilding reproduced all
three baseline metrics. Reapplying the exact candidate and rebuilding
reproduced all three candidate metrics. Each non-baseline build compiled
affected code and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-174445-a5fb6435`.
The `000-baseline`, `001-collection-guid-memcmp`, `002-baseline-rebuilt`, and
`003-collection-guid-memcmp-repeat` directories contain preserved read-only
DLL/PDB pairs, hashes, source revisions and patches, build logs/binlogs,
SizeBench snapshots, receipts, stderr, and frozen tool identity sidecars.
The first baseline and candidate also contain targeted family reports and
resolved disassembly. `verification.json`, `semantic-review.json`, and
`progress.json` record the result and handoff.

All four measurements use the same frozen SizeBench deployment and managed
identity documented above. The complete deployment manifest and hashes were
verified before and after each measurement. Initialization and build used the
same `cmd.exe` process and explicit `/nopgo` recipe documented above. Logs
confirm `PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. All 934 captured command-tlog hashes match between
the rebuilt control and repeated candidate. Eight export ordinal/name
identities, PE security flags, and five built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier test results belong to the starting
commit, not this experiment. Source review found no change to ownership,
interface identity, error handling, or threading. The inspected code adds no
allocation or function call. Comparison width, loads, and branch layout
differ, including on mismatching GUID prefixes; application performance has
not been measured. Compilation does not establish runtime correctness.
Failure injection and other architectures were not validated.

## Follow-up: compact `ctl::implements` interface-ID comparisons

File: `dxaml\xcp\components\com\inc\ComTemplates.h`

In `ctl::implements<TINTERFACE>::QueryInterface`, replace the two
`InlineIsEqualGUID` checks with fixed-size `std::memcmp` equality checks:

```cpp
std::memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0
std::memcmp(&riid, &__uuidof(TINTERFACE), sizeof(IID)) == 0
```

Both forms compare all 16 GUID bytes for equality. `IUnknown` remains first,
followed by `TINTERFACE`, including when `TINTERFACE` is `IUnknown`. The
existing casts, virtual `AddRef` call, output writes, and `E_NOINTERFACE`
path remain unchanged. This also preserves the existing failure behavior
that leaves the output pointer untouched for an unsupported interface.

The change adds an explicit `<cstring>` include. It does not change
`implements_inspectable`'s own comparison, inheritance, class layout, interface
maps, vtable shape, reference-count operations, generated source, or metadata.
Derived event handlers continue to use the same implementation.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `e8ceb37d9`, fresh baseline | 14,407,168 | 14,406,144 | 14,416,784 |
| Fixed-size `ctl::implements` GUID comparisons | 14,404,608 | 14,403,584 | 14,414,124 |
| Incremental reduction | 2,560 | 2,560 | 2,660 |
| Cumulative reduction from original baseline | 130,560 | 130,560 | 130,652 |

The actual DLL file is **2,560 bytes (2.5 KiB) smaller**. Cumulative file
savings are **130,560 bytes (127.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 2,560 bytes; other raw section sizes remain
unchanged. Virtual `.text` shrinks by 2,624 bytes and virtual `.pdata` by
36 bytes.

As supporting attribution, the `ctl::implements<T>::QueryInterface` family
falls from 10,140 to 6,834 attributed bytes. The engine reports 73 non-folded
representatives before and 67 after. The resolved
`IInputPreTranslateKeyboardSourceHandler` primary code block shrinks from
139 to 102 bytes. Its disassembly shows two 64-bit comparisons per GUID
instead of four 32-bit comparisons, no out-of-line `memcmp` call, and the
same guarded virtual `AddRef` call. Family totals are not added to section
savings or treated as contiguous disassembly ranges.

Restoring the original source byte-for-byte and rebuilding reproduced all
three baseline metrics. Reapplying the exact candidate and rebuilding
reproduced all three candidate metrics. Each non-baseline build compiled
affected code and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-182720-852ea098`.
The `000-baseline`, `001-implements-guid-memcmp`, `002-baseline-rebuilt`, and
`003-implements-guid-memcmp-repeat` directories contain read-only DLL/PDB
pairs, hashes, source patches, build logs/binlogs, SizeBench snapshots,
query receipts, stderr, and frozen tool identity sidecars. The first
baseline and candidate include targeted family reports and resolved
disassembly. `verification.json`, `semantic-review.json`, and `progress.json`
record the comparison, source review, and handoff.

All four measurements use the frozen deployment and managed identity
documented above. The complete deployment manifest and hashes were verified
before and after each measurement. Initialization and build used the same
`cmd.exe` process and explicit `/nopgo` recipe documented above. Logs confirm
`PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. All 934 captured command-tlog hashes match between
the rebuilt control and repeated candidate. Eight export ordinal/name
identities, PE security flags, and five built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier test results belong to the starting
commit, not this experiment. Source review found no change to interface
identity, ownership, error handling, or threading. The inspected code adds
no allocation or function call. Comparison width, loads, and branch layout
differ; application performance has not been measured. Compilation does
not establish runtime correctness. Failure injection and other architectures
were not validated.

## Follow-up: compact tracker-collection interface-ID comparisons

File: `dxaml\xcp\dxaml\lib\TrackerCollections.h`

In `TrackerCollection<T>::QueryInterfaceImpl`, replace the three
`InlineIsEqualGUID` checks with fixed-size `std::memcmp` equality checks:

```cpp
std::memcmp(&iid, &__uuidof(wfc::IIterable<T>), sizeof(IID)) == 0
std::memcmp(&iid, &__uuidof(wfc::IVector<T>), sizeof(IID)) == 0
std::memcmp(&iid, &__uuidof(IUntypedVector), sizeof(IID)) == 0
```

Both forms compare all 16 GUID bytes. The interface order, exact `static_cast`
results, output writes, `AddRefOuter`, inherited fallback, and HRESULTs are
unchanged. The existing interface map is unchanged, including its omission
of `IUntypedVector`. The change adds an explicit `<cstring>` include.
It does not change `TrackerView`, `TrackerIterator`, observable collection
comparisons, collection operations, reference tracking, class layout, public
interfaces, generated outputs, or metadata.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `3dc5d4986`, fresh baseline | 14,404,608 | 14,403,584 | 14,414,124 |
| Fixed-size tracker-collection GUID comparisons | 14,403,584 | 14,402,560 | 14,412,988 |
| Incremental reduction | 1,024 | 1,024 | 1,136 |
| Cumulative reduction from original baseline | 131,584 | 131,584 | 131,788 |

The actual DLL file is **1,024 bytes (1 KiB) smaller**. Cumulative file
savings are **131,584 bytes (128.5 KiB)** from the original 14,535,168-byte
baseline. Only `.text` changes: its raw size falls by 1,024 bytes and its
virtual size by 1,136 bytes. Other section sizes remain unchanged.

The `TrackerCollection<T>::QueryInterfaceImpl` family falls from 3,780 to
2,700 attributed bytes, with 18 representatives in both builds. The resolved
`AutomationPeer` primary code block shrinks from 210 to 150 bytes. Its
disassembly uses two 64-bit comparisons per GUID instead of four 32-bit
comparisons, with no out-of-line `memcmp` call. The guarded reference-count
call and base fallback remain. Family totals support attribution; they are
not added to section savings or treated as contiguous disassembly ranges.

Restoring the original source byte-for-byte and rebuilding reproduced all
three baseline metrics. Reapplying the candidate and rebuilding reproduced
all three candidate metrics. Each non-baseline build compiled affected code
and performed an LTCG link. The repeated candidate also preserves the
original header's line endings and trailing blank line.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-185742-b7b7cb5e`.
The `000-baseline`, `001-tracker-guid-memcmp`, `002-baseline-rebuilt`, and
`003-tracker-guid-memcmp-repeat` directories contain read-only DLL/PDB pairs,
hashes, source patches, build logs/binlogs, SizeBench snapshots, query receipts,
stderr, and frozen tool identity sidecars. The first baseline and candidate
include targeted family reports and resolved disassembly. `verification.json`,
`semantic-review.json`, and `progress.json` record the result and handoff.

All four measurements use the frozen deployment and managed identity
documented above. The complete deployment manifest and hashes were verified
before and after each measurement. Initialization and build used the same
`cmd.exe` process and explicit `/nopgo` recipe documented above. Logs confirm
`PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. All 934 captured command-tlog hashes match between
the rebuilt control and repeated candidate. Eight export ordinal/name
identities, PE security flags, and five built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier test results belong to the starting
commit, not this experiment. Source review found no change to interface
identity, ownership, errors, or threading. The inspected code adds no allocation
or function call. Comparison widths, loads, and mismatch branch layout differ;
application performance has not been measured. Compilation does not establish
runtime correctness. Failure injection and other architectures were not
validated.

## Follow-up: compact tracker-view interface-ID comparisons

File: `dxaml\xcp\dxaml\lib\TrackerCollections.h`

In `TrackerView<T>::QueryInterfaceImpl`, replace its two `InlineIsEqualGUID`
checks with fixed-size `std::memcmp` equality checks:

```cpp
std::memcmp(&iid, &__uuidof(wfc::IIterable<T>), sizeof(IID)) == 0
std::memcmp(&iid, &__uuidof(wfc::IVectorView<T>), sizeof(IID)) == 0
```

Both forms compare all 16 GUID bytes. `IIterable<T>` remains first, followed
by `IVectorView<T>`. The exact interface casts, output writes, `AddRefOuter`,
`WeakReferenceSource` fallback, and HRESULTs remain unchanged. The existing
interface map, including its `IVector<T>` entry, is unchanged. No collection
operations, reference tracking, class layouts, metadata, generated outputs,
or build options change. The existing `<cstring>` include is reused.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `aa4d6c651`, fresh baseline | 14,403,584 | 14,402,560 | 14,412,988 |
| Fixed-size tracker-view GUID comparisons | 14,403,072 | 14,402,048 | 14,412,412 |
| Incremental reduction | 512 | 512 | 576 |
| Cumulative reduction from original baseline | 132,096 | 132,096 | 132,364 |

The actual DLL file is **512 bytes (0.5 KiB) smaller**. Cumulative file
savings are **132,096 bytes (129 KiB)** from the original 14,535,168-byte
baseline. Only `.text` changes: raw size falls by 512 bytes and virtual size
by 576 bytes. Other section sizes remain unchanged.

The `TrackerView<T>::QueryInterfaceImpl` family falls from 2,646 to 2,070
attributed bytes, with 18 representatives in both builds. The resolved
`AutomationPeer` primary code block shrinks from 147 to 115 bytes.
Disassembly shows two 64-bit comparisons per GUID instead of four 32-bit
comparisons, without an out-of-line `memcmp` call. The guarded reference-count
call and base fallback remain. Family totals support attribution; they are
not added to section savings or treated as contiguous disassembly ranges.

Restoring the original source byte-for-byte and rebuilding reproduced all
three baseline metrics. Reapplying the two-line candidate and rebuilding
reproduced all three candidate metrics. Every non-baseline build compiled
affected code and performed an LTCG link. The retained source preserves the
original header's line endings and trailing blank line.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-192100-dbe0bae2`.
The `000-baseline`, `001-view-guid-memcmp`, `002-baseline-rebuilt`, and
`003-view-guid-memcmp-repeat` directories contain read-only DLL/PDB pairs,
hashes, source patches, build logs/binlogs, SizeBench snapshots, query receipts,
stderr, and frozen tool identity sidecars. The first baseline and candidate
also contain targeted family reports and resolved disassembly.
`verification.json`, `semantic-review.json`, and `progress.json` record the
comparison and handoff.

All four measurements use the frozen deployment and managed identity
documented above. The complete deployment manifest and hashes were verified
before and after each measurement. Initialization and build used the same
`cmd.exe` process and explicit `/nopgo` recipe documented above. Logs confirm
`PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. All 934 captured command-tlog hashes match
between the rebuilt control and repeated candidate. Eight export ordinal/name
identities, PE security flags, and five built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to
interface identity, ownership, error handling, or threading. The inspected
code adds no allocation or function call. Load widths and mismatch branch
layout differ; application performance has not been measured. Compilation
does not establish runtime correctness. Failure injection and other
architectures were not validated.

## Follow-up: compact generated framework interface-ID comparisons

Source of truth:
`dxaml\xcp\tools\XCPTypesAutoGen\XamlGen\Templates\Code\Framework\Bodies\Class.tt`.
The accompanying preprocessed `Class.cs` and 643 generated framework `.g.cpp`
files were regenerated with the existing T4 and XamlGen tools, not hand-edited.

In the instance `QueryInterfaceImpl` generator, replace the five emitted
`InlineIsEqualGUID` expression forms with fixed-size `std::memcmp` equality:

```cpp
std::memcmp(&iid, &__uuidof(InterfaceType), sizeof(IID)) == 0
```

Both forms compare all 16 GUID bytes. Implementation, public, protected,
virtual, and implemented interfaces retain their original order and casts.
Feature-presence directives and short-circuited runtime feature checks are
unchanged. Output writes, `AddRefOuter`, inherited fallback, and HRESULTs are
unchanged. The generator adds an explicit `<cstring>` include. Factory and
event-argument generators, class layouts, interface maps, public interfaces,
metadata, and compiler/linker options are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `9ec426828`, fresh baseline | 14,403,072 | 14,402,048 | 14,412,412 |
| Fixed-size generated framework GUID comparisons | 14,377,472 | 14,376,448 | 14,386,684 |
| Incremental reduction | 25,600 | 25,600 | 25,728 |
| Cumulative reduction from original baseline | 157,696 | 157,696 | 158,092 |

The actual DLL file is **25,600 bytes (25 KiB) smaller**. Cumulative file
savings are **157,696 bytes (154 KiB)** from the original 14,535,168-byte
baseline. Only `.text` changes: raw size falls by 25,600 bytes and virtual
size by 25,728 bytes. Other section sizes remain unchanged.

As supporting attribution, the 390 `DirectUI::*Generated::QueryInterfaceImpl`
representatives in `Aggregate.g.obj` fall from 71,347 to 53,657 attributed
bytes. This is a named subset, not every affected generated class. Resolved
primary code blocks shrink from 561 to 358 bytes for `UIElementGenerated`,
150 to 118 for `Grid`, and 285 to 194 for `ControlGenerated`. Their disassembly
uses two 64-bit comparisons per GUID instead of four 32-bit comparisons,
without an out-of-line `memcmp` call. Guarded reference-count calls and
base fallbacks remain. Symbol totals are not added to section savings.

Restoring original source contents and forcing timestamp invalidation of
only the owned changed files reproduced all three baseline metrics after
recompilation and an LTCG link. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. An earlier restoration copied old
timestamps, so its incremental build did not recompile or relink and retained
the candidate DLL. That attempt is explicitly marked invalid as a baseline
control and excluded from the measurements above.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-204039-7bbf3db6`.
The valid comparisons are `000-baseline`, `001-generated-guid-memcmp`,
`003-baseline-recompiled`, and `004-generated-guid-memcmp-repeat`.
They contain read-only DLL/PDB pairs, hashes, source patches, build logs and
binlogs, SizeBench snapshots, receipts, stderr, and frozen tool identity
sidecars. The first baseline and candidate also contain scoped generated-QI
symbol reports and resolved disassembly. `002-baseline-rebuilt` preserves
the excluded no-op control attempt and its explanation.

All measurements use the frozen deployment and managed identity documented
above. The full deployment manifest and hashes were verified before and after
each measurement. Initialization and build used the same `cmd.exe` process
and explicit `/nopgo` recipe. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

T4 preprocessing with the installed `TextTransformCore.exe --preprocess`
reproduced the original checked-in `Class.cs` byte-for-byte before editing.
The existing `RunCodeGen` project's `BuildGenerated` target generated the C++
updates with partial XBF generation. Each generated file was compared against
its backup before copying: only the new include and instance GUID equality
expressions changed. The `runcodegen.cmd` wrapper was not used because it
restores CSV files and requests a clean code-generator rebuild.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to
interface identity, ownership, error handling, or threading. The inspected
code adds no allocation or comparison function call. Loads and branch layout
differ, including fallback paths for partially matching GUIDs; application
performance has not been measured. Compilation does not establish runtime
correctness. Failure injection and other architectures were not validated.

## Follow-up: compact generated factory interface-ID comparisons

Source of truth:
`dxaml\xcp\tools\XCPTypesAutoGen\XamlGen\Templates\Code\Framework\Bodies\ClassFactory.tt`.
The supporting `EventArgsClass.tt` change emits `<cstring>` only when the
event-argument class has a custom factory with factory interfaces. Both
preprocessed C# files and 402 affected framework `.g.cpp` files were
regenerated with the existing T4 and XamlGen tools, not hand-edited.

Replace the factory template's three emitted `InlineIsEqualGUID` expression
forms with fixed-size equality checks:

```cpp
std::memcmp(&iid, &__uuidof(InterfaceType), sizeof(IID)) == 0
```

Both forms compare all 16 GUID bytes. The 544 generated comparisons preserve
the order of factory, static, and explicitly implemented interfaces. Feature
directives, interface casts, output writes, `AddRefOuter`, inherited fallback,
and HRESULTs remain unchanged. Ten event-argument files gain an explicit
include; other affected files already include `<cstring>`. Event-argument
instance comparisons, public interfaces, class layouts, metadata, and
compiler/linker options are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `d009bd618`, fresh baseline | 14,377,472 | 14,376,448 | 14,386,684 |
| Fixed-size generated factory GUID comparisons | 14,370,816 | 14,369,792 | 14,380,092 |
| Incremental reduction | 6,656 | 6,656 | 6,592 |
| Cumulative reduction from original baseline | 164,352 | 164,352 | 164,684 |

The actual DLL file is **6,656 bytes (6.5 KiB) smaller**. Cumulative file
savings are **164,352 bytes (160.5 KiB)** from the original 14,535,168-byte
baseline. Only `.text` changes: raw size falls by 6,656 bytes and virtual
size by 6,592 bytes. Other section sizes remain unchanged.

As supporting attribution, the 411 factory `QueryInterfaceImpl` entries in
`Aggregate.g.obj`, including `FactoryGenerated` names, fall from 46,283 to
39,597 attributed bytes. Both builds have 408 positive-sized entries.
This is a named subset, not an estimate of total savings. Resolved primary
code blocks shrink from 205 to 160 bytes for `UIElementFactory` and from
146 to 129 bytes for `PropertyMetadataFactory` and `GridFactory`. Their
disassembly uses two 64-bit comparisons per GUID instead of four 32-bit
comparisons, without an out-of-line `memcmp` call. Guarded reference-count
calls and base fallbacks remain. Symbol totals are not added to section
savings or assumed to be contiguous disassembly ranges.

Restoring the original source contents and invalidating timestamps only on
the owned changed files reproduced all three baseline metrics. Reapplying
the exact candidate reproduced all three candidate metrics. Each
non-baseline build compiled `Aggregate.g.cpp` and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-211102-51c343bb`.
The `000-baseline`, `001-factory-guid-memcmp`, `002-baseline-recompiled`, and
`003-factory-guid-memcmp-repeat` directories contain read-only DLL/PDB pairs,
hashes, source patches, build logs/binlogs, SizeBench snapshots, receipts,
stderr, and frozen tool identity sidecars. The first baseline and candidate
also contain scoped factory symbol reports and resolved disassembly.
`ownership.json`, `generation-verification.json`, `verification.json`,
`semantic-review.json`, and `progress.json` record the experiment and handoff.

All measurements use the frozen deployment and managed identity documented
above. The full deployment manifest and file hashes were verified before and
after each measurement. Initialization and build used the same `cmd.exe`
process and explicit `/nopgo` recipe. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

T4 preprocessing uses the installed `TextTransformCore.exe --preprocess`.
Baseline preprocessing differs only in generator-version annotations and
the event-argument generator's license prefix, which the regeneration script
preserves. The existing `BuildGenerated` target ran with partial XBF
generation. Before copying generated outputs, each file was compared against
its backup: only factory equality expressions and the ten includes changed.
The generated update script and `runcodegen.cmd` wrapper were not executed.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to
interface identity, ownership, error handling, or threading. The inspected
code adds no allocation or comparison function call. Loads and branch layout
differ, including fallback paths for partially matching GUIDs; application
performance has not been measured. Compilation does not establish runtime
correctness. Failure injection and other architectures were not validated.

## Follow-up: compact generated event-argument interface-ID comparisons

Source of truth:
`dxaml\xcp\tools\XCPTypesAutoGen\XamlGen\Templates\Code\Framework\Bodies\EventArgsClass.tt`.
The preprocessed `EventArgsClass.cs` and 116 affected framework `.g.cpp` files
were regenerated with the existing T4 and XamlGen tools, not hand-edited.

Replace the event-argument instance template's five emitted
`InlineIsEqualGUID` expression forms with fixed-size equality checks:

```cpp
std::memcmp(&iid, &__uuidof(InterfaceType), sizeof(IID)) == 0
```

Both forms compare all 16 GUID bytes. The 228 generated comparisons preserve
interface order, feature directives and short-circuited feature checks, exact
casts, output writes, `AddRefOuter`, inherited fallback, and HRESULTs.
The template now always includes `<cstring>`; 106 outputs gain that include,
while ten already included it for their factories. Factory implementations,
event properties and methods, public interfaces, class layouts, metadata,
and compiler/linker options are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `c1662098d`, fresh baseline | 14,370,816 | 14,369,792 | 14,380,092 |
| Fixed-size generated event-argument GUID comparisons | 14,367,232 | 14,366,208 | 14,376,652 |
| Incremental reduction | 3,584 | 3,584 | 3,440 |
| Cumulative reduction from original baseline | 167,936 | 167,936 | 168,124 |

The actual DLL file is **3,584 bytes (3.5 KiB) smaller**. Cumulative file
savings are **167,936 bytes (164 KiB)** from the original 14,535,168-byte
baseline. Only `.text` changes: raw size falls by 3,584 bytes and virtual
size by 3,440 bytes. Other section sizes remain unchanged.

As supporting attribution, the 126 event-argument `QueryInterfaceImpl`
entries selected by name in `Aggregate.g.obj` fall from 15,514 to 12,400
attributed bytes. All remain positive-sized. This name subset is not complete
coverage of the generated outputs. Resolved primary code blocks shrink from
193 to 140 bytes for `DragStartingEventArgsGenerated` and from 125 to 96
bytes for both `RoutedEventArgsGenerated` and `PointerRoutedEventArgsGenerated`.
Disassembly uses two 64-bit comparisons per GUID instead of four 32-bit
comparisons, without an out-of-line `memcmp` call. Guarded reference-count
calls and base fallbacks remain. Symbol totals are not added to section
savings or assumed to be contiguous disassembly ranges.

Restoring original source contents and invalidating timestamps only on the
owned changed files reproduced all three baseline metrics. Reapplying the
exact candidate reproduced all three candidate metrics. Every non-baseline
build compiled `Aggregate.g.cpp` and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-214009-4be2a6d6`.
The `000-baseline`, `001-eventargs-guid-memcmp`, `002-baseline-recompiled`,
and `003-eventargs-guid-memcmp-repeat` directories contain read-only DLL/PDB
pairs, hashes, source patches, build logs/binlogs, SizeBench snapshots,
receipts, stderr, and frozen tool identity sidecars. The first baseline and
candidate include scoped symbol reports and resolved disassembly.
`ownership.json`, `generation-verification.json`, `verification.json`,
`semantic-review.json`, and `progress.json` record the experiment and handoff.

All four measurements use the frozen deployment and managed identity
documented above. The full deployment manifest and file hashes were verified
before and after each measurement. Initialization and build used the same
`cmd.exe` process and explicit `/nopgo` recipe. Logs confirm
`PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. All 934 command-tlog hashes match between the
recompiled control and repeated candidate. Eight export ordinal/name
identities, PE security flags, and five built WinMD hashes are unchanged.

Before editing, installed `TextTransformCore.exe --preprocess` reproduced
the original preprocessed file except its existing license prefix, which
the regeneration script preserves. The existing `BuildGenerated` target ran
with partial XBF generation. Every generated output was compared against its
backup before copying: only instance equality expressions and the 106
includes changed. The generated update script and `runcodegen.cmd` wrapper
were not executed.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to
interface identity, ownership, error handling, or threading. The inspected
code adds no allocation or comparison function call. Comparison widths,
loads, and mismatch branch layout differ; application performance has not
been measured. Compilation does not establish runtime correctness. Failure
injection and other architectures were not validated.

## Follow-up: share untyped COM factory completion

Files: `dxaml\xcp\components\com\inc\ComObject.h`,
`dxaml\xcp\components\com\inc\ComObjectBase.h`, and
`dxaml\xcp\components\com\ComObjectBase.cpp`.

Move the untyped `ComObject<T>::CreateInstance` overload's result publication
and failure cleanup into an out-of-line `ComObjectBase::CreateInstanceBase`
overload. Allocation and type-specific construction remain in the template.
The shared overload consumes the initial reference, transfers the same
`ComBase`-based `IInspectable` pointer on success, and releases it on failure.

The existing initializer still runs exactly once. Its normalization of
successful initialization to `S_OK`, both levels of failure propagation,
and the unchanged caller output on failure remain intact. Failure cleanup
still dispatches the object's delegating `Release`, including controlling-outer
behavior for aggregated objects. Typed factory overloads and their debug
leak-check option are unchanged. This adds no virtual slot, object field,
interface, export, metadata definition, or compiler/linker option.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `bcf0a613a`, fresh baseline | 14,367,232 | 14,366,208 | 14,376,652 |
| Shared untyped factory completion | 14,340,096 | 14,339,072 | 14,349,724 |
| Incremental reduction | 27,136 | 27,136 | 26,928 |
| Cumulative reduction from original baseline | 195,072 | 195,072 | 195,052 |

The actual DLL file is **27,136 bytes (26.5 KiB) smaller**. Cumulative file
savings are **195,072 bytes (190.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 24,576 bytes and `.pdata` by 2,560 bytes.
Other raw section sizes are unchanged.

The untyped factory template family falls from 66,882 attributed bytes across
268 representatives to 4,885 across 27 representatives. This is not a separate
61,997-byte saving: much of the remaining construction code moves into callers.
For example, the resolved `AccessKeyInvokedEventArgs` factory block was 235
bytes and is no longer a standalone block; its activation caller grows from
85 to 208 bytes. The surviving `BasicConnectedAnimationConfiguration` factory
block shrinks from 281 to 196 bytes and tail-jumps to the shared completion.
The new shared overload has one 86-byte primary block in the collected
`.text` symbols; the existing initializer remains 47 bytes. These observations
support attribution and are not summed with whole-file savings.

Restoring all three source files byte-for-byte and recompiling reproduced
every baseline size metric. Reapplying the exact candidate and recompiling
reproduced every candidate metric. Every non-baseline build compiled affected
code and performed an LTCG link. The sharing claim comes from this measured
output, not an assumption that `noinline` prevents LTCG specialization.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-225046-cd2758ee`.
The `000-baseline`, `001-shared-factory-completion`,
`002-baseline-recompiled`, and `003-shared-factory-completion-repeat`
directories contain read-only DLL/PDB copies, hashes, source patches, build
logs/binlogs, SizeBench snapshots, receipts, stderr, and frozen tool identity
sidecars. The first two also contain `.text` symbol coverage and resolved
primary-block disassembly. `ownership.json`, `semantic-review.json`,
`verification.json`, and `progress.json` record source ownership and handoff.

All measurements use the frozen SizeBench deployment and managed identity
documented above. Its full 268-file manifest was verified before and after
each measurement. Initialization and build used the same `cmd.exe` process
and explicit `/nopgo` recipe. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Review found no change to ownership,
aggregation, initialization order, output-pointer behavior, or HRESULTs.
Error propagation remains, but diagnostic source locations and stacks change
with the refactoring. The inspected activation path retains the same number
of calls on success; a surviving factory uses a tail jump to the shared helper.
Failure cleanup now uses a guarded virtual `Release` rather than the
template's devirtualized implementation. No additional allocation is introduced.
Call layout and instruction-cache behavior change; application performance
has not been measured. Compilation does not establish runtime correctness.
Failure injection and other architectures were not validated.

## Follow-up: share typed COM factory failure cleanup

Files: `dxaml\xcp\components\com\inc\ComObject.h`,
`dxaml\xcp\components\com\inc\ComObjectBase.h`, and
`dxaml\xcp\components\com\ComObjectBase.cpp`.

Move the typed `ComObject<T>::CreateInstance` overload's failure-only
`ReleaseInterface` into the out-of-line
`ComObjectBase::ReleaseFailedInstance(ComBase*)` helper. Return `S_OK`
immediately after publishing the typed pointer, rather than clearing the local
pointer and falling through the cleanup label. The existing initializer
already normalizes successful initialization to `S_OK`.

Allocation, initialization, the exact typed cast, output publication, and the
debug-only leak-check actions retain their order. Failed initialization still
propagates the same HRESULT, leaves the caller's output unchanged, and releases
the initial reference exactly once through the object's delegating `Release`.
Controlling-outer behavior is unchanged. The existing `IFC` and `RRETURN`
failure propagation remain in the template. The success path does not call
the new helper. Untyped factories, class layouts, vtable slots, public
interfaces, metadata, and compiler/linker options are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `795ebb6e4`, fresh baseline | 14,340,096 | 14,339,072 | 14,349,724 |
| Shared typed factory failure cleanup | 14,295,040 | 14,294,016 | 14,304,812 |
| Incremental reduction | 45,056 | 45,056 | 44,912 |
| Cumulative reduction from original baseline | 240,128 | 240,128 | 239,964 |

The actual DLL file is **45,056 bytes (44 KiB) smaller**. Cumulative file
savings are **240,128 bytes (234.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 44,544 bytes and `.pdata` by 512 bytes.
Other raw section sizes are unchanged. Virtual `.text` shrinks by 44,560
bytes and `.pdata` by 516 bytes, while `.rdata`, `.reloc`, and `.data` grow
by 96, 4, and 64 bytes respectively.

As supporting attribution, the typed factory family with matching template
types falls from 57,318 attributed bytes across 215 representatives to 45,178
across 181. The family with distinct template types falls from 17,117 bytes
across 61 representatives to 15,257 across 60. These are not whole-file
savings: factories can inline into callers, and symbol families do not describe
all affected code. Their totals are not summed with section savings.

Resolved primary blocks shrink from 244 to 220 bytes for `BindingOperations`
and from 402 to 378 bytes for `BindableObservableVectorWrapper`. The new helper
has one 27-byte primary block in the collected `.text` coverage. It performs a
null check and a guarded virtual `Release` call. Both inspected factories call
it only after failed initialization; their successful paths add no call.
The sharing claim is based on emitted code, not an assumption that `noinline`
prevents LTCG specialization.

Restoring all three original source files and recompiling reproduced every
baseline metric. Reapplying the exact candidate and recompiling reproduced
every candidate metric. Each non-baseline build compiled `ComObjectBase.cpp`
in the prerequisites stage, compiled affected consumers including
`Aggregate.g.cpp`, and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260918-232351-1eb125cc`.
The `000-baseline`, `001-typed-factory-failure-release`,
`002-baseline-recompiled`, and `003-typed-factory-failure-release-repeat`
directories contain read-only DLL/PDB copies, hashes, source patches, build
logs/binlogs, SizeBench snapshots, receipts, stderr, and frozen tool identity
sidecars. The first two include `.text` symbol coverage and resolved
primary-block disassembly. `ownership.json`, `semantic-review.json`,
`verification.json`, and `progress.json` record source ownership and handoff.

All four measurements use the frozen deployment and managed identity
documented above. Its complete deployment manifest was verified before and
after each measurement. Initialization and build used the same `cmd.exe`
process and explicit `/nopgo` recipe. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to
ownership, aggregation, initialization order, output-pointer behavior, or
HRESULTs. Failure cleanup now has an additional helper call and uses guarded
virtual dispatch instead of the template's devirtualized release path.
Diagnostic source locations and failure stacks change. Successful-path
call counts remain unchanged in the inspected factories, but code layout and
instruction-cache behavior change. Application performance, runtime failure
injection, debug behavior, and other architectures were not exercised.
Compilation does not establish runtime correctness.

## Follow-up: share the dependency-object activation guard

Files: `dxaml\xcp\dxaml\lib\comInstantiation.h` and
`dxaml\xcp\dxaml\lib\DXamlServices.cpp`.

Move the core-initialization check from the dependency-object `ctl::make<T>`
overload into `DXamlServices::ActivatePeer`, its shared activation wrapper.
The template is the wrapper's only production caller in this checkout.
The wrapper returns `RPC_E_WRONG_THREAD` without activating or writing the
output when the same `IsDXamlCoreInitialized` check fails. The caller's
existing `IFC_RETURN` still reports and propagates that failure once.

Successful activation, temporary `ComPtr` cleanup, the exact typed cast,
release of an existing destination, ownership transfer, and `S_OK`
normalization remain unchanged. The initialization predicate retains its
existing feature-dependent idle-state handling. All 561 inspected static
type-index getters return constants, so evaluating the type index before
entering the shared check introduces no side effect. The isolated ValueBoxer
stub always reports an initialized core and retains its existing behavior.
Non-dependency-object factories, class layouts, public interfaces, metadata,
compiler/linker options, and security settings are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `982a4edba`, fresh baseline | 14,295,040 | 14,294,016 | 14,304,812 |
| Shared dependency-object activation guard | 14,281,728 | 14,280,704 | 14,291,380 |
| Incremental reduction | 13,312 | 13,312 | 13,432 |
| Cumulative reduction from original baseline | 253,440 | 253,440 | 253,396 |

The actual DLL file is **13,312 bytes (13 KiB) smaller**. Cumulative file
savings are **253,440 bytes (247.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 12,800 bytes and `.pdata` by 512 bytes.
Other raw section sizes are unchanged. Virtual `.text`, `.pdata`, and `.data`
shrink by 12,800, 756, and 16 bytes respectively; virtual `.rdata` and `.reloc`
grow by 128 and 12 bytes.

As supporting attribution, the main `ctl::make<T>(ComPtrRef<ComPtr<T>>)` family
falls from 54,408 bytes across 239 representatives to 41,241 across 176.
The resolved `Border` primary block shrinks from 182 to 148 bytes, while
the shared activation wrapper grows from 141 to 157 bytes. The inspected
non-dependency-object `AddPagesEventArgs`, `BindingFailedEventArgs`, and
`BudgetManager` blocks retain their respective 265, 265, and 248-byte sizes.
Template totals include inlining and layout effects; they are not added to
section savings or treated as contiguous disassembly ranges. The emitted
wrapper contains one initialization check, with no new helper or `noinline`
annotation.

Restoring both original source files byte-for-byte and recompiling reproduced
every baseline metric. Reapplying the exact candidate and recompiling
reproduced every candidate metric. Each non-baseline build compiled
`DXamlServices.cpp` and affected consumers including `Aggregate.g.cpp`, then
performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-001004-6854be49`.
The `000-baseline`, `001-shared-activation-guard`, `002-baseline-recompiled`,
and `003-shared-activation-guard-repeat` directories contain read-only
DLL/PDB copies, hashes, source patches, build logs/binlogs, SizeBench
snapshots, receipts, stderr, and frozen tool identity sidecars. The first
two also contain `.text` symbol coverage and resolved primary-block
disassembly. `ownership.json`, `semantic-review.json`, `verification.json`,
and `progress.json` record ownership, review, and handoff.

All four measurements use the frozen deployment and managed identity
documented above. Its complete 268-file manifest was verified before and
after each measurement. Initialization and build used the same `cmd.exe`
process and explicit `/nopgo` recipe. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to
ownership, activation order, output-pointer behavior, or HRESULTs. Diagnostic
source locations and debug expression text change. Successful-path call
counts remain unchanged in the inspected code; an uninitialized-core failure
now enters the shared wrapper before returning. No additional allocation is
introduced. Code layout and instruction-cache behavior change. Application
performance, runtime failure injection, debug behavior, and other
architectures were not exercised. Compilation does not establish runtime
correctness.

## Follow-up: simplify smart-pointer interface query transfer

File: `dxaml\xcp\components\com\inc\ComUtils.h`.

In the `ctl::do_query_interface(ctl::ComPtr<T>&, U*)` overload, query directly
into one initially empty temporary `ComPtr`, then swap it with the destination.
This replaces a raw output pointer, an `Attach`, and move assignment through
an additional temporary.

The same interface query runs before the old destination is released,
including when the source aliases the destination. The new pointer is
published before the old reference is released, preserving reentrant
observation of the destination. The old reference is released exactly once,
even when the queried interface pointer equals it. Null input still clears
the destination and returns `S_OK`. Failed queries and successful HRESULTs
retain their original output and return behavior. The raw-pointer overload,
interface casts, public signatures, class layouts, metadata, and build
options are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `e889f1a35`, fresh baseline | 14,281,728 | 14,280,704 | 14,291,380 |
| Direct query into temporary, then swap | 14,281,216 | 14,280,192 | 14,290,708 |
| Incremental reduction | 512 | 512 | 672 |
| Cumulative reduction from original baseline | 253,952 | 253,952 | 254,068 |

The actual DLL file is **512 bytes (0.5 KiB) smaller**. Cumulative file savings
are **253,952 bytes (248 KiB)** from the original 14,535,168-byte baseline.
Raw `.text` shrinks by 512 bytes; other raw section sizes are unchanged.
Virtual `.text`, `.rdata`, and `.reloc` shrink by 464, 192, and 16 bytes
respectively.

As supporting attribution, one smart-pointer query family falls from 2,938
to 2,886 bytes across the same 21 representatives. The resolved
`IVector<SetterBase*>` query helper shrinks from 178 to 126 bytes, removes
cleanup calls on empty temporaries, and reduces local stack reservation
from 64 to 48 bytes. The inspected `Binding` and `FrameworkElement` helper
blocks remain 142 bytes. Other query-family totals are unchanged. Inlining
and layout effects also contribute; these totals are not added to section
savings or treated as contiguous disassembly ranges without resolving blocks.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Every non-baseline build compiled
affected consumers, including `Aggregate.g.cpp`, and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-004249-a9390911`.
The `000-baseline`, `001-query-interface-swap`, `002-baseline-recompiled`,
and `003-query-interface-swap-repeat` directories contain read-only DLL/PDB
copies, hashes, source patches, build logs/binlogs, SizeBench snapshots,
receipts, stderr, and frozen tool identity sidecars. The first two include
`.text` symbol coverage and resolved primary-block disassembly.
`ownership.json`, `semantic-review.json`, `verification.json`, and
`progress.json` record ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment and managed
identity documented above. Its complete 268-file manifest was verified before
and after each measurement. Initialization and build used the same `cmd.exe`
process and explicit `/nopgo` recipe. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to
ownership, query/release order, destination publication, or HRESULTs.
The change adds no allocation or COM call. Register choices, stack writes,
and code layout differ; application performance remains unmeasured.
Runtime reentrancy, failure injection, debug behavior, and other architectures
were not exercised. Compilation does not establish runtime correctness.

## Follow-up: transfer newly created aggregated inner references directly

File: `dxaml\xcp\dxaml\lib\comTemplateLibrary.h`.

In the three templated aggregation factory paths, replace the fixed
`NonDelegatingQueryInterface(IID_IInspectable)` call and balancing
`NonDelegatingRelease` with direct transfer of the initial inner reference:

```cpp
*instance = reinterpret_cast<IInspectable*>(static_cast<INonDelegatingInspectable*>(pObjAsAggregable));
```

The final `ComObject<T>::NonDelegatingQueryInterface` implementation delegates
to `ComObjectBase::NonDelegatingQueryInterfaceBase`. For `IID_IInspectable`,
that implementation returns this exact base-interface pointer, increments
its nondelegating reference count, and always returns `S_OK`. Transferring the
existing reference removes the balanced increment/decrement pair. The caller
still receives the same interface pointer and one owning reference.

The change covers `AggregableActivationFactory<T>::ActivateInstanceStatic`,
its `CreateInstance` helper, and
`AggregableAbstractActivationFactory<T>::ActivateInstanceStatic`. The
non-template `BetterAggregable` factories already use direct inner-reference
transfer. Construction, initialization, activation guards, outer validation,
output publication, and failure cleanup remain unchanged. Successful
initialization still normalizes to `S_OK`. Public interfaces, class layouts,
generated outputs, metadata, and compiler/linker options are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `ebf7e2dd3`, fresh baseline | 14,281,216 | 14,280,192 | 14,290,708 |
| Direct aggregated inner-reference transfer | 14,277,632 | 14,276,608 | 14,287,764 |
| Incremental reduction | 3,584 | 3,584 | 2,944 |
| Cumulative reduction from original baseline | 257,536 | 257,536 | 257,012 |

The actual DLL file is **3,584 bytes (3.5 KiB) smaller**. Cumulative file
savings are **257,536 bytes (251.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 3,072 bytes and `.pdata` by 512 bytes.
Other raw section sizes are unchanged. Virtual `.text` and `.pdata` shrink
by 3,008 and 120 bytes, while `.rdata` and `.reloc` grow by 176 and 8 bytes.

As supporting attribution, the concrete `ActivateInstanceStatic` template
family falls from 4,502 bytes across 12 representatives to 2,714 across nine.
The validation-wrapper family falls from 5,065 bytes across nine
representatives to 3,582 across seven. These totals reflect inlining and
folding changes, not separate whole-file savings. Resolved primary blocks
shrink from 408 to 325 bytes for `BasicConnectedAnimationConfiguration`,
375 to 292 for `DataTemplateKey`, and 600 to 520 for the
`DesktopWindowXamlSource` validation wrapper. The inspected concrete
activation path removes `NonDelegatingQueryInterfaceBase` and `ReleaseImpl`
calls and adds no helper call or stack frame. Symbol totals are not added to
section savings or assumed to be contiguous disassembly ranges.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Each non-baseline build compiled
affected consumers, including `Aggregate.g.cpp`, and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-021253-60e8c06a`.
The `000-baseline`, `001-direct-inner-transfer`, `002-baseline-recompiled`,
and `003-direct-inner-transfer-repeat` directories contain read-only DLL/PDB
copies, hashes, source revisions/patches, build logs/binlogs, SizeBench
snapshots, receipts, stderr, and frozen tool identity sidecars. The first
two also include `.text` symbol coverage and resolved primary-block
disassembly. `ownership.json`, `semantic-review.json`, `verification.json`,
and `progress.json` record source ownership, review, and handoff.

All four measurements use the frozen deployment and managed identity
documented above. Its full 268-file manifest was verified before and after
each measurement. Initialization and build ran in the same `cmd.exe` process
with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and
repeated candidate. Eight export ordinal/name identities, PE security flags,
and five built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to
interface identity, net ownership, initialization order, failure handling,
or HRESULTs. The transient reference-count pair is removed; no allocation
or new call is introduced. Code layout and register choices change, and
application performance remains unmeasured. Runtime aggregation/reentrancy,
failure injection, debug behavior, and other architectures were not exercised.
Compilation does not establish runtime correctness.

## Follow-up: compact untyped collection interface-ID comparisons

File: `dxaml\xcp\dxaml\lib\JoltCollections.h`.

In `PresentationFrameworkCollection<T>::QueryInterfaceImpl`, replace the
remaining `InlineIsEqualGUID` check for `IUntypedVector` with fixed-size equality:

```cpp
std::memcmp(&iid, &__uuidof(IUntypedVector), sizeof(IID)) == 0
```

Both forms compare all 16 GUID bytes. The exact `static_cast<IUntypedVector*>`,
output write, `AddRefOuter`, inherited fallback, and HRESULTs remain unchanged.
Concrete classes still check their own interfaces before this inherited check.
The existing `<cstring>` include is reused. The change does not alter
interface maps, class layouts, collection operations, reference tracking,
generated outputs, metadata, compiler/linker options, or security settings.
The separate value-type specializations and observable collections are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `621b7108f`, fresh baseline | 14,277,632 | 14,276,608 | 14,287,764 |
| Fixed-size untyped collection GUID comparison | 14,277,120 | 14,276,096 | 14,287,108 |
| Incremental reduction | 512 | 512 | 656 |
| Cumulative reduction from original baseline | 258,048 | 258,048 | 257,668 |

The actual DLL file is **512 bytes (0.5 KiB) smaller**. Cumulative file savings
are **258,048 bytes (252 KiB)** from the original 14,535,168-byte baseline.
Only `.text` changes: raw size falls by 512 bytes and virtual size by 656 bytes.
Other section sizes remain unchanged.

This base implementation is inlined into concrete collection query methods,
rather than reported as a standalone template family. Resolved primary code
blocks shrink from 168 to 152 bytes for both `ColumnDefinitionCollection`
and `BrushCollection`, and from 206 to 190 bytes for `UIElementCollection`.
Their disassembly replaces four 32-bit comparisons with two 64-bit comparisons,
without an out-of-line `memcmp` call. The guarded reference-count call, exact
output-pointer adjustment, inherited fallback, and stack reservation remain.
Symbol evidence supports attribution; it is not added to section savings.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Every non-baseline build compiled
affected consumers, including `Aggregate.g.cpp`, and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-025836-e13430a7`.
The `000-baseline`, `001-untyped-vector-guid-memcmp`,
`002-baseline-recompiled`, and `003-untyped-vector-guid-memcmp-repeat`
directories contain read-only DLL/PDB copies, hashes, source revisions/patches,
build logs/binlogs, SizeBench snapshots, receipts, stderr, and frozen tool
identity sidecars. The first two include `.text` symbol coverage and resolved
primary-block disassembly. `ownership.json`, `semantic-review.json`,
`verification.json`, and `progress.json` record ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment and managed identity
documented above. Its full 268-file manifest was verified before and after
each core measurement. Initialization and build ran in the same `cmd.exe`
process with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to
interface identity, ownership, error handling, or threading. The inspected
code introduces no allocation or function call. Comparison widths, loads,
and mismatch branch layout differ; application performance remains unmeasured.
Runtime reentrancy, failure injection, debug behavior, and other architectures
were not exercised. Compilation does not establish runtime correctness.

## Follow-up: transfer same-type template-part references directly

File: `dxaml\xcp\dxaml\lib\Control_Partial.h`.

In `Control::GetTemplatePart<TInterface, TRuntime>`, use the existing
`ComPtr::MoveTo` when the two types are identical. The temporary already owns
the queried interface reference. Transferring it to the caller removes the
`CopyTo` reference-count increment and the temporary's balancing release.
The caller receives the same pointer and one owning reference.

An `if constexpr` keeps the existing `CopyTo` path for different types,
including the `IUIElement` to `IScrollViewer` query. Name and output-pointer
validation, HSTRING creation, template-child lookup, optional-interface query,
output publication, cleanup of the child and string, and HRESULT handling
remain unchanged. Missing parts and unsupported optional interfaces still
produce a null result through the existing `AsOrNull` path. Errors before
publication leave the caller's output untouched, as before.

The change adds no helper call, allocation, runtime type test, public API,
class-layout change, generated output, or metadata change. It preserves
compiler/linker options and security settings.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `a70a1d17b`, fresh baseline | 14,277,120 | 14,276,096 | 14,287,108 |
| Direct same-type template-part transfer | 14,276,096 | 14,275,072 | 14,285,860 |
| Incremental reduction | 1,024 | 1,024 | 1,248 |
| Cumulative reduction from original baseline | 259,072 | 259,072 | 258,916 |

The actual DLL file is **1,024 bytes (1 KiB) smaller**. Cumulative file savings
are **259,072 bytes (253 KiB)** from the original 14,535,168-byte baseline.
Raw `.text` shrinks by 1,024 bytes; other raw section sizes are unchanged.
Virtual `.text` and `.rdata` shrink by 1,120 and 128 bytes respectively.

As supporting attribution, the same-type `GetTemplatePart` family falls from
8,562 to 7,468 attributed bytes across the same 22 representatives. Resolved
primary blocks shrink from 395 to 346 bytes for `IGrid` and from 379 to 328
bytes for `ICalendarView`. The different-type `IUIElement` to `IScrollViewer`
block remains 404 bytes. The inspected grid path removes the guarded AddRef
and Release calls and retains its 64-byte local stack reservation. Symbol
totals are not added to section savings or assumed to be contiguous
disassembly ranges without resolving the code blocks.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Each non-baseline build recompiled
affected consumers and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-032628-dbaee7c4`.
The `000-baseline`, `001-template-part-transfer`, `002-baseline-recompiled`,
and `003-template-part-transfer-repeat` directories contain read-only DLL/PDB
copies, hashes, source revisions/patches, build logs/binlogs, SizeBench
snapshots, receipts, stderr, and frozen tool identity sidecars. The first two
also contain `.text` symbol coverage and resolved primary-block disassembly.
`ownership.json`, `semantic-review.json`, `verification.json`, and
`progress.json` record ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment and managed
identity documented above. Its complete 268-file manifest was verified before
and after each core measurement. Initialization and build ran in the same
`cmd.exe` process with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to net
ownership, pointer identity, lookup/query order, output behavior, or HRESULTs.
The transient reference-count pair is removed; no runtime work is added.
Code layout and register choices change. Application performance, runtime
reentrancy, failure injection, debug behavior, and other architectures were
not exercised. Compilation does not establish runtime correctness.

## Follow-up: avoid a redundant input reference during untyped insertion

File: `dxaml\xcp\dxaml\lib\JoltCollections.h`.

In `PresentationFrameworkCollection<T>::UntypedInsertAt`, query the borrowed
`IInspectable` input directly into the existing typed owning `ComPtr`:

```cpp
IFC_RETURN(pItem->QueryInterface(IID_PPV_ARGS(spTypedItem.ReleaseAndGetAddressOf())));
```

This removes the temporary input `ComPtr` and its AddRef/Release pair.
The COM input must remain valid for the call. The discovered
`DiagnosticsInterop::InsertAt` caller also holds its own input reference.
The queried typed reference still owns the object across `InsertAt`, including
callbacks, and is released on every return path. The borrowed input is not
used after the query.

The thread check still precedes the same interface query and insertion.
The exact IID, typed result, virtual insertion call, failure propagation,
typed cleanup, and final `S_OK` normalization remain unchanged. Null remains
outside the existing non-null input contract; this does not introduce a
null-tolerant fallback. The tracker-collection implementation is unchanged.
No helper, allocation, public interface, class-layout change, generated output,
metadata change, or compiler/linker option change is introduced.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `1754a5da4`, fresh baseline | 14,276,096 | 14,275,072 | 14,285,860 |
| Direct query of borrowed insertion input | 14,274,560 | 14,273,536 | 14,284,096 |
| Incremental reduction | 1,536 | 1,536 | 1,764 |
| Cumulative reduction from original baseline | 260,608 | 260,608 | 260,680 |

The actual DLL file is **1,536 bytes (1.5 KiB) smaller**. Cumulative file
savings are **260,608 bytes (254.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 1,536 bytes; other raw section sizes are
unchanged. Virtual `.text` and `.reloc` shrink by 1,776 and 4 bytes
respectively, while virtual `.data` grows by 16 bytes.

As supporting attribution, the `UntypedInsertAt` family falls from 10,989 to
9,435 attributed bytes across the same 37 representatives. Resolved primary
blocks shrink from 297 to 255 bytes for `ColumnDefinition`, `Brush`, and
`UIElement`. The inspected column-definition path removes the guarded input
AddRef and Release calls and retains its 48-byte local stack reservation.
The typed-reference cleanup, CFG dispatch, stack-cookie checks, and mitigation
barriers remain. Symbol totals are not added to section savings or assumed to
be contiguous disassembly ranges without resolving the code blocks.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Each non-baseline build compiled
affected consumers, including `Aggregate.g.cpp`, and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-041533-3d2549a5`.
The `000-baseline`, `001-borrowed-insert-input`, `002-baseline-recompiled`,
and `003-borrowed-insert-input-repeat` directories contain read-only DLL/PDB
copies, hashes, source revisions/patches, build logs/binlogs, SizeBench
snapshots, receipts, stderr, and frozen tool identity sidecars. The first two
also contain `.text` symbol coverage and resolved primary-block disassembly.
`ownership.json`, `semantic-review.json`, `verification.json`, and
`progress.json` record source ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment and managed identity
documented above. Its full 268-file manifest was verified before and after
each core measurement. Initialization and build ran in the same `cmd.exe`
process with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to net
ownership under the COM input-lifetime contract, interface identity,
query/insertion order, or HRESULTs. The transient input reference-count pair
is removed; no runtime work is added. Code layout and register choices change.
Application performance, runtime reentrancy, failure injection, debug behavior,
and other architectures were not exercised. Compilation does not establish
runtime correctness.

## Follow-up: avoid a redundant input reference during tracker insertion

File: `dxaml\xcp\dxaml\lib\TrackerCollections.h`.

In `TrackerCollection<T>::UntypedInsertAt`, query the borrowed `IInspectable`
input directly into the existing typed owning WRL `ComPtr`:

```cpp
IFC_RETURN(pItem->QueryInterface(IID_PPV_ARGS(spTypedItem.ReleaseAndGetAddressOf())));
```

This removes the temporary input `ComPtr` and its AddRef/Release pair.
The COM input must remain valid for the call. The discovered
`DiagnosticsInterop::InsertAt` caller also holds its own input reference.
The queried typed reference still owns the object across the collection
operation, including callbacks, and is released on every return path.
The borrowed input is not used after the query.

The exact IID, query-before-size-check order, virtual dispatch, failure
propagation, and final `S_OK` normalization remain unchanged. The existing
choice of `Append` when `index == Size()` and `SetAt` otherwise is preserved;
this change does not replace `SetAt` with `InsertAt` or alter bounds behavior.
Read-only and observable overrides still dispatch through the same methods.
Null remains outside the existing non-null input contract. No helper,
allocation, public interface, class-layout change, generated output, metadata
change, compiler/linker option change, or security-setting change is introduced.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `a4a284f20`, fresh baseline | 14,274,560 | 14,273,536 | 14,284,096 |
| Direct query of borrowed tracker input | 14,273,536 | 14,272,512 | 14,283,220 |
| Incremental reduction | 1,024 | 1,024 | 876 |
| Cumulative reduction from original baseline | 261,632 | 261,632 | 261,556 |

The actual DLL file is **1,024 bytes (1 KiB) smaller**. Cumulative file
savings are **261,632 bytes (255.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 1,024 bytes; other raw section sizes remain
unchanged. Virtual `.text` and `.data` shrink by 864 and 16 bytes respectively,
while virtual `.reloc` grows by 4 bytes.

As supporting attribution, the tracker `UntypedInsertAt` family falls from
5,286 to 4,458 attributed bytes across the same 18 representatives. Resolved
primary blocks shrink from 293 to 247 bytes for `AutomationPeer`,
`CalendarViewDayItem`, and `DependencyObject`. The inspected automation-peer
path removes the guarded input AddRef and Release calls and retains its
48-byte local stack reservation. The typed-reference cleanup, guarded
collection dispatch, stack-cookie checks, and mitigation barriers remain.
The previously optimized presentation-framework insertion family remains
9,435 bytes across 37 representatives. Symbol totals are not added to section
savings or assumed to be contiguous disassembly ranges without resolving
the code blocks.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the candidate and recompiling reproduced
all three candidate metrics. Each non-baseline build recompiled affected
consumers, including `Aggregate.g.cpp`, and performed an LTCG link.
The repeated candidate preserves the original header's trailing blank line.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-044223-f605366a`.
The `000-baseline`, `001-borrowed-tracker-input`, `002-baseline-recompiled`,
and `003-borrowed-tracker-input-repeat` directories contain read-only DLL/PDB
copies, hashes, source revisions/patches, build logs/binlogs, SizeBench
snapshots, receipts, stderr, and frozen tool identity sidecars. The first two
also contain `.text` symbol coverage and resolved primary-block disassembly.
`ownership.json`, `semantic-review.json`, `verification.json`, and
`progress.json` record source ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment and managed identity
documented above. Its full 268-file manifest was verified before and after
each core measurement. Initialization and build ran in the same `cmd.exe`
process with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to net
ownership under the COM input-lifetime contract, interface identity,
query/mutation order, or HRESULTs. The transient input reference-count pair
is removed; no runtime work is added. Code layout and register choices change.
Application performance, runtime reentrancy, failure injection, debug behavior,
and other architectures were not exercised. Compilation does not establish
runtime correctness.

## Follow-up: transfer diagnostic collection-item references directly

File: `dxaml\xcp\dxaml\lib\InternalDebugInterop.cpp`.

In `GetCollectionItemInternal<Item, IItem>`, replace the final same-type
`spItemAsDO.CopyTo(ppDO)` with `spItemAsDO.MoveTo(ppDO)`. The temporary already
owns the queried `IDependencyObject` reference. Transferring that reference
removes the additional AddRef and the temporary's balancing Release. The
caller receives the same interface pointer and one owning reference.

The collection query, indexed lookup, `IDependencyObject` query, initial null
output, failure propagation, and final `S_OK` normalization remain unchanged.
All failure paths retain their existing cleanup. The item and collection
references are still released after publishing the output, in the same order.
The existing null-result behavior also remains unchanged. This is a private
template implementation change; no public interface, class layout, generated
output, metadata, compiler/linker option, or security setting changes.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `288f1ad9d`, fresh baseline | 14,273,536 | 14,272,512 | 14,283,220 |
| Direct diagnostic collection-item transfer | 14,273,024 | 14,272,000 | 14,282,612 |
| Incremental reduction | 512 | 512 | 608 |
| Cumulative reduction from original baseline | 262,144 | 262,144 | 262,164 |

The actual DLL file is **512 bytes (0.5 KiB) smaller**. Cumulative file savings
are **262,144 bytes (256 KiB)** from the original 14,535,168-byte baseline.
Raw `.text` shrinks by 512 bytes and virtual `.text` by 608 bytes. All other
raw and virtual section sizes remain unchanged.

As supporting attribution, the `GetCollectionItemInternal` family falls from
7,561 to 7,200 attributed bytes across the same 19 representatives. Resolved
primary blocks shrink from 403 to 384 bytes for `ColumnDefinition`,
`DependencyObject`, and `UIElement`. The inspected column-definition path
removes the guarded AddRef call and skips the now-empty temporary's Release
on success. Failure cleanup remains. Local stack reservation stays 64 bytes,
and CFG dispatch, stack-cookie checks, and mitigation barriers remain.
Symbol totals are not added to section savings or assumed to be contiguous
disassembly ranges without resolving the code blocks.

Restoring the original source byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Each non-baseline build compiled
`InternalDebugInterop.cpp` and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-050846-136779a4`.
The `000-baseline`, `001-collection-item-transfer`, `002-baseline-recompiled`,
and `003-collection-item-transfer-repeat` directories contain read-only
DLL/PDB copies, hashes, source revisions/patches, build logs/binlogs, SizeBench
snapshots, receipts, stderr, and frozen tool identity sidecars. The first two
also contain scoped `InternalDebugInterop.obj` symbol coverage and resolved
primary-block disassembly. `ownership.json`, `semantic-review.json`,
`verification.json`, and `progress.json` record ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment and managed identity
documented above. Its full 268-file manifest was verified before and after
each core measurement. Initialization and build ran in the same `cmd.exe`
process with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to net
ownership, pointer identity, lookup/query order, output behavior, or HRESULTs.
The transient reference-count pair is removed; no helper, allocation, or
runtime work is added. Code layout and register choices change. Application
performance, runtime reentrancy, failure injection, debug behavior, and other
architectures were not exercised. Compilation does not establish runtime
correctness.

## Follow-up: query diagnostic collection size into an empty owner

File: `dxaml\xcp\dxaml\lib\InternalDebugInterop.cpp`.

In `GetCollectionSizeInternal<Item>`, select the existing raw-output
`ctl::do_query_interface` overload with
`*spCollection.ReleaseAndGetAddressOf()`. The local smart pointer is freshly
constructed and empty. The query writes its owning result directly into that
pointer instead of querying into another smart pointer and swapping it into
the empty destination.

The requested IID, input pointer, query-before-size order, output behavior,
and failure cleanup remain unchanged. The raw overload retains the existing
null-input branch; the subsequent size lookup still requires a non-null
collection, as before. A populated query output is released on every return
path, including failure. Both `IFC_RETURN` sites and final `S_OK` normalization
remain unchanged. The general smart-pointer overload remains available for
callers with existing destination references. No public interface, class
layout, generated output, metadata, build option, or security setting changes.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `f1dad4027`, fresh baseline | 14,273,024 | 14,272,000 | 14,282,612 |
| Direct collection-size query output | 14,271,488 | 14,270,464 | 14,281,188 |
| Incremental reduction | 1,536 | 1,536 | 1,424 |
| Cumulative reduction from original baseline | 263,680 | 263,680 | 263,588 |

The actual DLL file is **1,536 bytes (1.5 KiB) smaller**. Cumulative file
savings are **263,680 bytes (257.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 1,536 bytes. Virtual `.text` and `.pdata`
shrink by 1,184 and 240 bytes respectively. Other section sizes are unchanged.

As supporting attribution, the `GetCollectionSizeInternal` family grows from
2,782 to 3,742 attributed bytes across the same 20 representatives as query
logic moves inline. The resolved `ColumnDefinition`, `DependencyObject`, and
`UIElement` primary blocks each grow from 140 to 187 bytes. The inspected
column-definition path no longer calls the general smart-pointer query helper.
Its guarded QI, size lookup, release, and 48-byte local stack reservation
remain. The stack-cookie check now runs in the wrapper rather than the
removed helper frame. One separately grouped smart-pointer query family,
previously 2,886 bytes across 21 representatives, no longer appears in the
complete template report. These family observations are not summed with
section savings or substituted for the actual file measurement.

Restoring the original source byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Each non-baseline build compiled
`InternalDebugInterop.cpp` and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-053018-70ca83d0`.
The `000-baseline`, `001-direct-collection-size-query`,
`002-baseline-recompiled`, and `003-direct-collection-size-query-repeat`
directories contain read-only DLL/PDB copies, hashes, source revisions and
patches, build logs/binlogs, SizeBench snapshots, receipts, stderr, and frozen
tool identity sidecars. The first two also contain scoped symbol coverage and
resolved primary-block disassembly. `ownership.json`, `semantic-review.json`,
`verification.json`, and `progress.json` record ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment and managed
identity documented above. Its full 268-file manifest was verified before and
after each core measurement. Initialization and build ran in the same
`cmd.exe` process with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to the
starting commit, not this experiment. Source review found no change to net
ownership, pointer identity, query/lookup order, output behavior, or HRESULTs.
The inspected path removes a helper call and frame without adding an
allocation or COM call. Code layout and register choices change. Application
performance, runtime reentrancy, failure injection, debug behavior, and other
architectures were not exercised. Compilation does not establish runtime
correctness.

## Follow-up: transfer acquired weak-reference ownership (2026-09-19)

The `ctl::AsWeak` helper in
`dxaml\xcp\components\com\inc\ComPtr.h` previously constructed a temporary
`WeakRefPtr` by copying its local `ComPtr<IWeakReference>`, then moved that
temporary into the output. The copy added a transient AddRef/Release pair.
The helper now calls `pWeak->Swap(weakref)` using the existing `ComPtr::Swap`.
This transfers the acquired reference directly and lets the local release the
old destination. Only this publication statement changes; `AsWeakOrNull` and
the general smart-pointer operations remain unchanged.

The QueryInterface/GetWeakReference order and both `IFC_RETURN` sites remain
unchanged. Failure preserves the previous output and cleans up populated
local references. Null input still clears the output and returns `S_OK`.
Equal old/new interface pointers still release exactly one old ownership
reference. The new pointer is published before the old destination is
released, and the reference-source interface remains alive through that
release. There are no new calls, allocations, interfaces, or object fields.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `b7416b74e`, fresh baseline | 14,271,488 | 14,270,464 | 14,281,188 |
| Direct weak-reference transfer | 14,270,464 | 14,269,440 | 14,280,148 |
| Incremental reduction | 1,024 | 1,024 | 1,040 |
| Cumulative reduction from original baseline | 264,704 | 264,704 | 264,628 |

The actual DLL file is **1,024 bytes (1 KiB) smaller**. Cumulative file
savings are **264,704 bytes (258.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 1,024 bytes and virtual `.text` shrinks by
1,040 bytes. All other section sizes are unchanged.

As supporting attribution, the free `ctl::AsWeak` family shrinks from 899
to 740 attributed bytes across three unique representatives. The
`ctl::ComPtr::AsWeak` family shrinks from 716 to 610 across two representatives.
Each of the five resolved primary code blocks shrinks by 53 bytes. The
inspected AppBarButton block removes the transient guarded AddRef/Release
pair and retains the 64-byte local stack reservation, stack-cookie check,
guarded remaining COM calls, and failure fences. Folded aliases are not
distinct code bodies. Family totals are not summed with section savings or
substituted for the actual file measurement.

Restoring the original source byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Each non-baseline build recompiled
native source and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-070447-95be4eb1`.
The `000-baseline`, `001-weak-reference-swap`,
`002-baseline-recompiled`, and `003-weak-reference-swap-repeat` directories
contain read-only DLL/PDB copies, hashes, source revisions and patches,
build logs/binlogs, SizeBench snapshots, receipts, stderr, and frozen tool
identity sidecars. The first two also contain scoped symbol coverage and
resolved primary-block disassembly. `ownership.json`, `semantic-review.json`,
`verification.json`, and `progress.json` record ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment and managed
identity documented above. Its full 268-file manifest was verified before and
after each core measurement and again during final verification. Initialization
and build ran in the same `cmd.exe` process with explicit `/nopgo`. Logs
confirm `PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. All 934 command-tlog hashes match between the
recompiled control and repeated candidate. Eight export ordinal/name
identities, PE security flags, and five built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier regression results belong to earlier
work, not this experiment. Source review found no change to net ownership,
pointer identity, query order, output behavior, or HRESULTs. Removing the
transient reference changes AddRef/Release traffic; custom reentrant release
code that clears the destination could observe different destruction timing.
The helper does not use the destination or acquired reference after the old
destination is released. Runtime reentrancy, custom COM side effects, failure
injection, application performance, debug behavior, and other architectures
were not exercised. Compilation does not establish runtime correctness.

## Follow-up: select the typed enum-reference factory (2026-09-19)

File: `dxaml\xcp\components\valueboxer\inc\Value.h`.

In `PropertyValue::CreateEnumReference<T>`, pass
`spRef.ReleaseAndGetAddressOf()` instead of `&spRef` to
`ComObject<EnumReference<T>>::CreateInstance`. The explicit
`EnumReference<T>**` selects the existing typed factory overload. The
`ComPtrRef` expression selected the untyped `IInspectable**` overload instead,
because template argument deduction does not use its conversion operators.

The smart pointer is initially empty, so obtaining its output address releases
no reference. Both factories allocate the same `ComObject<EnumReference<T>>`
with a null controlling outer, run the same initializer, and transfer the
initial reference on success. The typed factory uses its existing failure-only
release helper. Its optional leak-check disabling argument remains false.
The caller retains its output-pointer check, all `IFC_RETURN` sites,
`SetValue`, RAII cleanup, and final owning `IInspectable` publication.
Pointer identity, successful HRESULT normalization, failure HRESULTs, and
output preservation on failure remain unchanged.

No factory implementation, public signature, class layout, generated output,
metadata, compiler/linker option, or security setting changes. This is not a
retry of the earlier rejected enum cleanup refactoring: it changes only
overload selection, without introducing goto cleanup or mixing IFC styles.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `d885233fc`, fresh baseline | 14,270,464 | 14,269,440 | 14,280,148 |
| Explicit typed enum-reference factory | 14,265,344 | 14,264,320 | 14,274,536 |
| Incremental reduction | 5,120 | 5,120 | 5,612 |
| Cumulative reduction from original baseline | 269,824 | 269,824 | 270,240 |

The actual DLL file is **5,120 bytes (5 KiB) smaller**. Cumulative file savings
are **269,824 bytes (263.5 KiB)** from the original 14,535,168-byte baseline.
Raw `.text` shrinks by 5,120 bytes; other raw section sizes are unchanged.
Virtual `.text` and `.pdata` shrink by 5,536 and 84 bytes respectively, while
virtual `.reloc` grows by 8 bytes.

As supporting attribution, the `CreateEnumReference` family falls from
48,784 attributed bytes across 183 representatives to 41,888 across 176.
The resolved primary blocks for `HoldingState`, `FocusState`, `Visibility`,
`ScrollBarVisibility`, and `CommandBarOverflowButtonVisibility` each shrink
from 267 to 238 bytes. The inspected `HoldingState` body calls the initializer
helper directly rather than through the untyped completion helper. It keeps
the created pointer in a register and removes the now-unneeded temporary
output slot and guarded local cleanup. Local stack reservation falls from
48 to 32 bytes. Failure cleanup still calls the existing
`ReleaseFailedInstance` helper. No new helper or `noinline` annotation is
introduced. Family totals are not summed with section savings, and attributed
function sizes are not assumed to be contiguous disassembly ranges.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Every non-baseline build compiled
affected native consumers, including `Aggregate.g.cpp`, and performed an
LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-074453-ad9013ae`.
The `000-baseline`, `001-typed-enum-factory`, `002-baseline-recompiled`, and
`003-typed-enum-factory-repeat` directories contain read-only DLL/PDB copies,
hashes, source revisions and patches, build logs/binlogs, SizeBench snapshots,
receipts, stderr, and frozen tool identity sidecars. The first two also contain
`.text` symbol coverage and resolved primary-block disassembly.
`ownership.json`, `semantic-review.json`, `verification.json`, and
`progress.json` record ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment and managed
identity documented above. Its full 268-file manifest was verified before and
after each core measurement and during final verification. Initialization and
build ran in the same `cmd.exe` process with explicit `/nopgo`. Logs confirm
`PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. All 934 command-tlog hashes match between the
recompiled control and repeated candidate. Eight export ordinal/name
identities, PE security flags, and five built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier test results belong to earlier work,
not this experiment. Source review found no change to allocation count,
initialization order, net ownership, interface identity, output behavior, or
HRESULTs. The inspected success path removes a nested completion-helper call;
initialization failure instead reaches the existing release helper through
the typed factory. Diagnostic source locations, failure stacks, register
choices, and code layout change. Application performance, runtime failure
injection, debug behavior, and other architectures were not exercised.
Compilation does not establish runtime correctness.

## Follow-up: select the typed non-enum reference factory (2026-09-19)

File: `dxaml\xcp\components\valueboxer\inc\Value.h`.

In `PropertyValue::CreateTypedReference<T>`, pass
`ref.ReleaseAndGetAddressOf()` instead of `&ref` to
`ComObject<Reference<T>>::CreateInstance`. The explicit `Reference<T>**`
selects the existing typed factory overload. The `ComPtrRef` expression
selected the untyped `IInspectable**` overload because template argument
deduction does not use conversion operators.

The local smart pointer is empty, so neither expression releases a reference.
Both factories allocate the same concrete object with a null controlling
outer, initialize it, and transfer its initial reference on success. On
initialization failure, the typed factory uses the existing failure-only
release helper. The caller retains its output-pointer check, `IFC_RETURN`
sites, value assignment, RAII cleanup, and final typed-forwarder transfer.
The fallible `TypeName` HSTRING duplication and its cleanup remain unchanged.
The typed factory's optional leak-check disabling argument remains false.

This changes only overload selection. No factory implementation, public
interface, class layout, generated output, metadata, compiler/linker option,
or security setting changes. The previously accepted enum-factory change
remains intact.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `fc2b681ea`, fresh baseline | 14,265,344 | 14,264,320 | 14,274,536 |
| Explicit typed non-enum reference factory | 14,264,832 | 14,263,808 | 14,274,276 |
| Incremental reduction | 512 | 512 | 260 |
| Cumulative reduction from original baseline | 270,336 | 270,336 | 270,500 |

The actual DLL file is **512 bytes (0.5 KiB) smaller**. Cumulative file
savings are **270,336 bytes (264 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 512 bytes; other raw sections are unchanged.
Virtual `.text`, `.rdata`, and `.reloc` shrink by 160, 96, and 20 bytes,
respectively, while virtual `.data` grows by 16 bytes.

As supporting attribution, the `CreateTypedReference` family falls from 564
to 518 bytes across the same two representatives. The `CreateReference`
family, which can inline it, falls from 6,615 to 6,505 bytes across the same
24 representatives. Resolved `Point` and `Rect` primary blocks shrink from
285 to 261 and 279 to 257 bytes. The `TypeName` primary block grows from
325 to 334 bytes. Family totals are not added together or substituted for
whole-file savings.

The inspected `Point` and `TypeName` paths call the initializer helper directly
instead of through the untyped completion helper. They no longer need the
temporary factory output slot. `TypeName` retains the string APIs and guarded
release after failed value assignment; its local stack reservation falls
from 64 to 48 bytes. `Point` retains a 48-byte local reservation, with different
register saves. Initialization failure still releases through the existing
helper. Disassembly ranges came from resolved positive primary code blocks,
not family totals or zero-sized aliases.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Each non-baseline build compiled
affected native consumers, including `Aggregate.g.cpp`, and performed an
LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-081947-fbe4f756`.
The `000-baseline`, `001-typed-reference-factory`, `002-baseline-recompiled`,
and `003-typed-reference-factory-repeat` directories contain read-only DLL/PDB
copies, hashes, source revisions and patches, build logs/binlogs, SizeBench
snapshots, receipts, stderr, and frozen tool identity sidecars. The first two
also contain `.text` symbol coverage and resolved primary-block disassembly.
`ownership.json`, `hypothesis.json`, `semantic-review.json`, `verification.json`,
and `progress.json` record source ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment documented above,
with managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
Its complete 268-file manifest was verified before and after each measurement
and during final verification. Initialization and build ran in the same
`cmd.exe` process with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags, and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier test results belong to earlier work,
not this experiment. Source review found no change to allocation count,
initialization order, net ownership, output behavior, interface identity, or
HRESULTs. The inspected success paths remove a nested helper call without
adding an allocation or COM call. Failure stacks, diagnostic source locations,
mitigation-fence placement, register saves, and code layout differ.
Application performance, runtime failure injection, debug behavior, and
other architectures were not exercised. Compilation does not establish
runtime correctness.

## Follow-up: simplify non-DependencyObject factory result transfer (2026-09-19)

File: `dxaml\xcp\dxaml\lib\comInstantiation.h`.

In the zero-argument, non-DependencyObject COM `ctl::make` overload, replace
the local `ComPtr<tobject>` with a raw owning temporary. Both forms call the
same typed `ComObject<tobject>::CreateInstance` overload. That factory writes
its output only on success and releases its allocation on initialization
failure. The original local smart pointer therefore has nothing to release
on an HRESULT failure.

After success, the remaining operations cannot fail: the destination's
`ReleaseAndGetAddressOf` forwards to the existing `noexcept` smart-pointer
operation, then the new pointer is stored. The old destination stays intact
until construction succeeds. It is cleared and released before publication,
as before. The caller receives the same initial owning reference, without
adding an interface query, reference-count operation, allocation or helper.
The caller's `IFC_RETURN`, failure HRESULTs and successful `S_OK` normalization
remain unchanged.

Dependency-object activation, non-COM construction, `make_ignoreleak`, and
argument-bearing overloads are unchanged. No public signature, class layout,
generated output, metadata, compiler/linker option or security setting changes.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `0ee5f25a0`, fresh baseline | 14,264,832 | 14,263,808 | 14,274,276 |
| Raw non-DependencyObject factory result | 14,263,296 | 14,262,272 | 14,272,972 |
| Incremental reduction | 1,536 | 1,536 | 1,304 |
| Cumulative reduction from original baseline | 271,872 | 271,872 | 271,804 |

The actual DLL file is **1,536 bytes (1.5 KiB) smaller**. Cumulative file
savings are **271,872 bytes (265.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 1,536 bytes; other raw sections are unchanged.
Virtual `.text`, `.pdata` and `.data` shrink by 1,424, 12 and 16 bytes,
respectively. Virtual `.rdata` and `.reloc` grow by 128 and 20 bytes.

As supporting attribution, the main `ctl::make<T1>` family falls from
41,241 to 40,384 attributed bytes across the same 176 representatives.
A separately reported nested-template family grows from 11,444 to 11,505
bytes across the same 37 representatives. These mixed families include
other overloads and are not summed or substituted for whole-file savings.

Resolved primary blocks for `AddPagesEventArgs` and `BindingFailedEventArgs`
each shrink from 265 to 260 bytes. `BudgetManager` shrinks from 248 to 231
bytes, while `AnchorRequestedEventArgs` grows from 302 to 313 bytes.
All four inspected blocks preserve the allocation, constructor, initializer,
old-output release and failure-release calls. The redundant RAII cleanup was
already optimized away in these bodies; the source simplification changes
register allocation and instruction selection. `BudgetManager` saves three
nonvolatile registers instead of five. `AnchorRequestedEventArgs` saves four
instead of five but uses longer immediate-zero stores. Both retain 32-byte
local stack reservations. Disassembly ranges came from resolved positive
primary code blocks, not summed function sizes or zero-sized aliases.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics. Reapplying the exact candidate and recompiling
reproduced all three candidate metrics. Each non-baseline build recompiled
affected native consumers and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-085615-500b45b3`.
The `000-baseline`, `001-raw-make-result`, `002-baseline-recompiled`, and
`003-raw-make-result-repeat` directories contain read-only DLL/PDB copies,
hashes, source revisions and patches, build logs/binlogs, SizeBench snapshots,
receipts, stderr and tool identity sidecars. The first two also contain
`.text` symbol coverage and representative primary-block disassembly.
`ownership.json`, `hypothesis.json`, `semantic-review.json`, `verification.json`,
and `progress.json` record ownership, review and handoff.

All measurements use the frozen SizeBench deployment and managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
Its full 268-file manifest was verified before and after each core measurement
and during final verification. Initialization and build ran in the same
`cmd.exe` process with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and repeated
candidate. Eight export ordinal/name identities, PE security flags and five
built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier test results belong to earlier work,
not this experiment. Source and emitted-code review found no added allocation,
COM call, reference-count operation, lock or helper. No runtime performance
improvement is claimed. The removed stack-local `ComPtr` no longer runs its
`XCP_STRONG` constructor annotation in monitor builds; destination and
created-object tracking implementations are unchanged. Debug diagnostic
equivalence was not exercised or established. Source locations, failure
stacks, register saves and code layout change. Application performance,
runtime failure injection and other architectures were not exercised.
Compilation does not establish runtime correctness.

## Follow-up: simplify dependency-object activation result transfer (2026-09-19)

File: `dxaml\xcp\dxaml\lib\comInstantiation.h`.

In the dependency-object `ctl::make` overload, replace the local
`ComPtr<DependencyObject>` with a raw owning temporary. The existing
`DXamlServices::ActivatePeer` call chain returns an owning output only on
success. Core-initialization failure leaves the null temporary untouched.
Peer creation retains its local owner through fallible initialization and
releases it on failure. Resolving an existing peer either acquires its
reference successfully or clears the output before returning failure.

No fallible operation follows successful activation in `ctl::make`.
`ReleaseAndGetAddressOf` still clears and releases the old destination before
publishing the same `static_cast<tobject*>` result. The old destination remains
intact if activation fails, and the caller receives the same owning reference
on success. The activation guard, initialization order, failure HRESULTs,
`IFC_RETURN` reporting, and successful `S_OK` normalization remain unchanged.
This removes redundant local cleanup, not a reference needed at runtime.

Non-dependency-object factories, `make_ignoreleak`, activation implementations,
public signatures, class layouts, generated outputs, metadata, compiler/linker
options, and security settings are unchanged. No helper or allocation is added.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `c8c96c1a7`, fresh baseline | 14,263,296 | 14,262,272 | 14,272,972 |
| Raw dependency-object activation result | 14,257,664 | 14,256,640 | 14,267,112 |
| Incremental reduction | 5,632 | 5,632 | 5,860 |
| Cumulative reduction from original baseline | 277,504 | 277,504 | 277,664 |

The actual DLL file is **5,632 bytes (5.5 KiB) smaller**. Cumulative file
savings are **277,504 bytes (271 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 5,632 bytes; other raw sections are unchanged.
Virtual `.text`, `.rdata`, and `.pdata` shrink by 5,760, 16, and 84 bytes,
respectively.

As supporting attribution, the main `ctl::make<T1>` family falls from
40,384 attributed bytes across 176 representatives to 37,704 across 170.
The separately grouped nested-template family stays at 11,505 bytes across
37 representatives. These mixed families are not summed or substituted for
the actual DLL measurement.

Resolved primary blocks for `Border`, `Grid`, and `SolidColorBrush` each
shrink from 148 to 110 bytes. The inspected `Border` body removes the guarded
release of the temporary activation result on failure and the corresponding
success-path cleanup branch. It preserves the activation call, failure
reporting and fence, old-destination guarded release, and 32-byte local stack
reservation. The non-dependency-object `AddPagesEventArgs` block remains
260 bytes. Disassembly ranges came from resolved positive primary blocks,
not family totals or zero-sized aliases.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics and every `.text` byte. Reapplying the exact candidate
and recompiling reproduced all three candidate metrics and every candidate
`.text` byte. Each non-baseline build compiled affected consumers, including
`Aggregate.g.cpp`, and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-115534-088f7362`.
The `000-baseline`, `001-raw-dependency-result`, `002-baseline-recompiled`,
and `003-raw-dependency-result-repeat` directories contain read-only DLL/PDB
copies, hashes, source revisions and patches, build logs/binlogs, SizeBench
snapshots, receipts, stderr, and tool identity sidecars. The first two also
contain complete `.text` symbol coverage and representative primary-block
disassembly. `ownership.json`, `hypothesis.json`, `semantic-review.json`,
`verification.json`, and `progress.json` record ownership, review, and handoff.

All four measurements use the frozen SizeBench deployment and managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
Its full 268-file manifest was verified before and after each core measurement
and during final verification. Initialization and build ran in the same
`cmd.exe` process with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and both
candidate builds. Eight export ordinal/name identities, PE security flags,
and five built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier test results belong to earlier work,
not this experiment. Source and emitted-code review found no added allocation,
COM call, reference-count operation, lock, or helper. No runtime performance
improvement is claimed. Removing the stack-local `ComPtr` also removes its
`XCP_STRONG` constructor annotation in monitor builds; destination and
created-object tracking are unchanged. Debug diagnostic equivalence was not
exercised or established. Diagnostic source locations and code layout change.
Application performance, runtime failure injection, reentrancy, and other
architectures were not exercised. Compilation does not establish runtime
correctness.

## Follow-up: remove redundant aggregation-factory cleanup (2026-09-19)

File: `dxaml\xcp\dxaml\lib\comTemplateLibrary.h`.

Remove the final `pObj = NULL` assignments and `ctl::release_interface(pObj)`
cleanup calls from `AggregableActivationFactory<T>::ActivateInstanceStatic`,
its `CreateInstance` helper, and
`AggregableAbstractActivationFactory<T>::ActivateInstanceStatic`.

The typed `ComObject<T>::CreateInstance(pOuter, &pObj)` factory retains
ownership through initialization. It releases the allocation on failure and
writes `pObj` only on success. Successful construction is followed only by
pointer casts and publication of the same initial owning inner reference.
The wrappers therefore have no reference to release on failure and no
fallible operation after construction. Their no-outer paths never assign
`pObj`; the untyped factory retains its own failure cleanup.

The existing activation checks, abstract null-outer check, `IFC`/`IFCPTR`
macros, `hr`, cleanup labels, and `return hr` remain unchanged. Interface
identity, controlling outer, output-on-failure behavior, initialization order,
and HRESULT propagation are preserved. Later interface-query failures in
the validation wrappers still use their existing smart-pointer cleanup.
No public signature, layout, metadata, generated output, compiler/linker
option, or security setting changes.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `6b7135b5e`, fresh baseline | 14,257,664 | 14,256,640 | 14,267,112 |
| Remove redundant aggregation cleanup | 14,257,152 | 14,256,128 | 14,266,632 |
| Incremental reduction | 512 | 512 | 480 |
| Cumulative reduction from original baseline | 278,016 | 278,016 | 278,144 |

The actual DLL file is **512 bytes (0.5 KiB) smaller**. Cumulative file
savings are **278,016 bytes (271.5 KiB)** from the original 14,535,168-byte
baseline. Raw `.text` shrinks by 512 bytes; other raw sections are unchanged.
Virtual `.text`, `.pdata`, and `.reloc` shrink by 448, 24, and 8 bytes,
respectively.

The resolved `DesktopWindowXamlSource` validation-wrapper primary block
shrinks from 520 to 498 bytes. It removes the redundant guarded release,
saves six nonvolatile registers instead of seven, and retains its 80-byte
local stack reservation. Its activation guard, factory calls, interface
query, necessary owner cleanup, security-cookie checks, and guarded indirect
calls remain. No helper call is added in the inspected wrapper.

The concrete `ActivateInstanceStatic` family remains at 2,714 attributed
bytes across nine representatives. The validation-wrapper family falls from
3,582 bytes across seven representatives to 2,550 across five. These totals
reflect compiler inlining/folding decisions and are not summed or substituted
for whole-file savings. The inspected `BasicConnectedAnimationConfiguration`,
`DataTemplateSelector`, and `XamlRenderingBackgroundTask` primary blocks
remain 325, 301, and 296 bytes, respectively. Disassembly ranges came from
resolved positive primary blocks, not function totals or zero-sized aliases.

Restoring the original header byte-for-byte and recompiling reproduced all
three baseline metrics and every baseline `.text` byte. Reapplying the exact
candidate and recompiling reproduced all three candidate metrics and every
candidate `.text` byte. Each non-baseline build compiled affected consumers,
including `Aggregate.g.cpp`, and performed an LTCG link.

### Provenance and limitations

Evidence is preserved under
`C:\Users\jecollin\AppData\Local\WinUI\Ralph\D_x1\20260919-122944-3b241f2c`.
The `000-baseline`, `001-aggregation-cleanup`, `002-baseline-recompiled`,
and `003-aggregation-cleanup-repeat` directories contain read-only DLL/PDB
copies, hashes, source revisions and patches, build logs/binlogs, SizeBench
snapshots, receipts, stderr, and tool identity sidecars. The first two also
contain complete `.text` symbol coverage and resolved primary-block
disassembly. `ownership.json`, `hypothesis.json`, `semantic-review.json`,
`verification.json`, and `progress.json` record ownership, review, and handoff.

All measurements use the frozen SizeBench deployment and managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
Its full 268-file manifest was verified before and after each core measurement
and during final verification. Initialization and build used the same
`cmd.exe` process with explicit `/nopgo`. Logs confirm `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
All 934 command-tlog hashes match between the recompiled control and both
candidates. Eight export ordinal/name identities, PE security flags, and
five built WinMD hashes are unchanged.

**Tests not run, as requested.** Earlier test results belong to earlier work,
not this experiment. No allocation, helper, lock, or reference-count operation
is added. Compiler inlining, register allocation, and code layout change;
no runtime performance improvement is claimed. Runtime aggregation,
reentrancy, failure injection, debug behavior, and other architectures were
not exercised. Compilation does not establish runtime correctness.
The simplification relies on the existing output-only-on-success factory
contract. Future fallible work between successful construction and ownership
transfer would need cleanup.

## Follow-up: compact interface-map IID array indexing (2026-09-19)

File: `dxaml\xcp\components\com\inc\ComMacros.h`.

In `END_INTERFACE_MAP`, write each local IID through
`(pResult + first)[current]` instead of `pResult[first + current]`.
This expresses the initial destination offset separately from the loop index.
It changes compiler code generation without adding a helper or changing the
interface map, virtual signature, array allocation, or base-class call.

`ComBase::GetIidsImpl` allocates the complete array and starts copying at index
zero. Each derived map writes its local entries in order, then passes
`first + current` and the unchanged original array pointer to its base.
For these valid, in-bounds indices, the two expressions address the same IID.
Count calculation, output publication, allocation failure, ownership, and
reference counting remain unchanged. The no-base macro and handwritten
`ComBase` copy implementation are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `449091e7c`, fresh baseline | 14,257,152 | 14,256,128 | 14,266,632 |
| Separate initial pointer offset | 14,254,080 | 14,253,056 | 14,263,336 |
| Incremental reduction | 3,072 | 3,072 | 3,296 |

The actual DLL file is **3,072 bytes (3 KiB) smaller**. Cumulative file
savings are **281,088 bytes (274.5 KiB)** from the original 14,535,168-byte
baseline. Only `.text` changes size: raw bytes fall by 3,072 and virtual
bytes by 3,296.

The `EnumReference<T>::CopyIIDsToArray` family falls from 18,117 to 16,836
attributed bytes, with 183 representatives in both builds. The resolved
`HoldingState` primary block shrinks from 99 to 92 bytes. Its local-copy
loop remains the same; the inlined base-copy loop uses indexed addressing
instead of a separate destination pointer. Both versions copy the same
16-byte entries and introduce no calls or stack allocation. These findings
describe compiler choices, not a guarantee that every affected loop becomes
faster. Family attribution is not added to whole-file savings.

Restoring the exact original header and recompiling reproduced all baseline
size metrics and every `.text` byte. Reapplying the candidate and recompiling
reproduced the candidate metrics and every `.text` byte. Binlog events confirm
affected consumers, including `Boxes.g.cpp`, compiled and LTCG linking ran in
each changed-source build. Eight export ordinal/name identities and PE
security characteristics remain unchanged.

### This turn's validation and provenance

The final `prodtest` build succeeded with `PGOBuildMode=Off`,
`Configuration=Release`, and `Platform=x64`; its preserved DLL retains all
candidate measurements and identical `.text` bytes. Native builds used
`VCToolsVersion=14.44.35207`. Initialization and each build ran in the same
`cmd.exe` process with explicit `/nopgo`.

On `ge_current-260820-Desktop`, the full CalendarView integration suite ran
in WPF mode against a freshly generated and deployed amd64fre payload:
**121 total, 120 passed, 1 failed, 0 blocked, 0 not run, 0 skipped**.
The only failure was `TestCICEvents` at `IsTrue(didOutputMatchMaster)`,
within the explicitly allowed baseline. `VerifySelfAdaptivePanel` passed.
This is a new run, not reused evidence from the earlier 119/121 baseline.
The cause of the known failure remains unestablished.

The built DLL, frozen final copy, local payload root/Test copies, and VM
root/Test copies all have SHA256:
`8F2C0D7E6C957010B7C43F3AB8A9B665937A6DDCC23C03AF950DB70644EF0D62`.

Evidence is under
`D:\x1\artifacts\ralph\20260919-153645-537e3e86`.
The five numbered measurement directories preserve hashes, source patches,
SizeBench snapshots, receipts, and tool identities; build logs/binlogs
accompany the first four. Final build evidence is in `final-test-build.log`
and `final-test-build.binlog`. VM evidence is in `tests.log`,
`vm-testrun-output.log`, `WexLogFileOutput`, `test-result.json`, and
`tested-dll-hashes.json`. `verification.json` and `build-events.json` record
repeatability and compilation evidence. Older binary snapshots may be
removed by the supervisor; their reports remain.

All measurements use the frozen SizeBench managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`,
with the full deployment manifest verified for each core collection.
The prior turn's ILLink crash did not recur in this turn's unchanged-source
preflight or final product/test build. No flags or dependencies were changed
to address it. Source review found no behavioral or ABI change. This suite
does not exhaustively cover interface maps; other architectures, allocation
failure injection, and runtime performance were not measured.

## Rejected: separate the root ComBase IID-copy offset (2026-09-19)

Starting from `d5b5e2f14`, tried changing the handwritten
`ComBase::CopyIIDsToArray` assignment in
`dxaml\xcp\components\com\inc\ComBase.h` from
`pResult[first + current]` to `(pResult + first)[current]`.
The accepted interface-map macro change remained in place.

The root loop copies `IUnknown` and `IInspectable` after the derived maps.
The proposed expression addresses the same elements for the existing valid
indices, with unchanged counts, ordering, allocation, ownership, and ABI.
However, applying it to this root loop reverses the size benefit observed
when changing only the derived macro. Do not assume the two locations have
additive benefits.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `d5b5e2f14`, fresh baseline | 14,254,080 | 14,253,056 | 14,263,336 |
| Root-loop candidate, rejected | 14,257,152 | 14,256,128 | 14,266,632 |
| Restored source after `prodtest` | 14,254,080 | 14,253,056 | 14,263,336 |

The candidate grows the actual DLL by **3,072 bytes**, all in raw `.text`.
Virtual `.text` grows by 3,296 bytes; other section sizes are unchanged.
The `EnumReference<T>::CopyIIDsToArray` family grows from 16,836 to 18,117
attributed bytes with 183 representatives in both builds. Family totals
are supporting attribution, not additional savings or disassembly ranges.
The source header was restored byte-for-byte. Rebuilding restored all three
baseline measurements and every baseline `.text` byte. This turn retains
**no source optimization**; cumulative file savings remain **281,088 bytes**.

### This turn's validation and provenance

Both candidate and restored-source builds compiled affected consumers and
performed LTCG linking. The final product/test build succeeded. All builds
used same-process initialization with `amd64fre /nopgo`, `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
Export ordinal/name identities are unchanged. Final PE security
characteristics match the baseline.

The full CalendarView suite ran again on `ge_current-260820-Desktop` in
WPF mode using a freshly generated and deployed restored-source payload:
**121 total, 120 passed, 1 failed, 0 blocked, 0 not run, 0 skipped**.
Only `TestCICEvents` failed, at the allowed
`IsTrue(didOutputMatchMaster)` assertion. `VerifySelfAdaptivePanel` passed.
The complete VM log contains all 121 distinct results. The live host tail
omitted the `VerifySkippedDaysInSamoa` completion line; the complete VM log
confirms that test passed. This is this turn's run, not earlier coverage.

SHA256 of the final built DLL, frozen snapshot, local payload root/Test
copies, and VM root/Test copies:
`F05524AA05F4FC5C52D4087331C36E31044EB6AAC40643B9C225244376697408`.

Evidence is under `D:\x1\artifacts\ralph\20260919-163821-29f34dbc`.
`000-baseline`, `001-root-iid-indexing`, and `002-restored-final` contain
preserved measurements, hashes, source patches, receipts, and tool identity.
`final-test-build.log`, `final-test-build.binlog`, `restored-native.binlog`,
and `build-events.json` preserve build evidence. `tests.log`,
`vm-testrun-output.log`, `WexLogFileOutput`, `test-result.json`,
`tested-dll-hashes.json`, and `verification.json` preserve validation.
The complete VM output combines a UTF-8 setup prefix with UTF-16LE test
output; the verification script reads its ASCII TAEF fields accordingly.
The frozen SizeBench managed identity remains
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`;
its full manifest was verified for every core collection. No tool defect
was observed. No candidate runtime performance benefit is claimed, and the
rejected candidate was not deployed to the VM.

## Rejected: compact iterator interface-ID comparison (2026-09-19)

Starting from `0c03c6b58`, tried replacing the single `InlineIsEqualGUID`
predicate in `IteratorBase<T>::QueryInterfaceImpl` with
`std::memcmp(&iid, &__uuidof(wfc::IIterator<T>), sizeof(IID)) == 0`.
The source is `dxaml\xcp\dxaml\lib\JoltCollections.h`; its existing
`<cstring>` include and adjacent fixed-size comparison pattern were reused.
Both expressions compare all 16 GUID bytes. The interface cast, output,
`AddRefOuter`, inherited `WeakReferenceSource` fallback, HRESULTs, and
threading behavior were unchanged. Iterator creation and ownership were
not changed.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `0c03c6b58`, fresh baseline | 14,254,080 | 14,253,056 | 14,263,336 |
| Iterator GUID candidate, rejected | 14,254,080 | 14,253,056 | 14,263,908 |
| Restored source after `prodtest` | 14,254,080 | 14,253,056 | 14,263,336 |

The actual DLL file does not shrink. Raw `.text` falls by 512 bytes, but
raw `.rdata` grows by 512 bytes. Virtual section bytes grow by 572:
`.text` falls by 224, `.rdata` grows by 512, `.pdata` by 168, and `.reloc`
by 116. The iterator `QueryInterfaceImpl` family falls from 3,969 to 3,185
attributed bytes with 49 representatives in both builds. This local
improvement is not a whole-file saving; it does not include all effects
of compiler and linker decisions.

Rejected the candidate and restored the header byte-for-byte. The restored
product/test build reproduces all baseline size metrics and every baseline
`.text` byte. No source optimization is retained. Cumulative file savings
remain **281,088 bytes**. Do not repeat this GUID-only candidate as a
standalone size reduction.

### This turn's validation and provenance

Candidate and restoration binlogs confirm affected source compilation and
LTCG linking. Builds used same-process `amd64fre /nopgo` initialization,
`PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. The final `prodtest` build succeeded.
Exports are unchanged; restored PE security characteristics match baseline.

The complete CalendarView suite ran on `ge_current-260820-Desktop` in WPF
mode using a fresh restored-source payload: **121 total, 120 passed,
1 failed, 0 blocked, 0 not run, 0 skipped**. The only failure was
`TestCICEvents` at the allowed `IsTrue(didOutputMatchMaster)` assertion.
`VerifySelfAdaptivePanel` passed. All 121 distinct results were confirmed
in this turn's complete VM log; no earlier result was reused.

The built DLL, frozen final snapshot, local payload root/Test copies, and
VM root/Test copies share SHA256:
`BC39878133434BD02F7C7D949E7AD53227C8EDCEC9A6DD2D4FC2AA11E3D44983`.

Evidence is under `D:\x1\artifacts\ralph\20260919-172635-9ab8bdcb`.
The numbered baseline, candidate, and restored directories contain frozen
measurements, hashes, source patches, receipts, and tool identities.
`final-test-build.log`, `final-test-build.binlog`, `restored-native.binlog`,
and `build-events.json` preserve build evidence. `tests.log`,
`vm-testrun-output.log`, `WexLogFileOutput`, `tested-dll-hashes.json`,
`test-result.json`, and `verification.json` preserve validation.
SizeBench used frozen managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`;
the complete deployment manifest was verified before every core collection.
No SizeBench defect was observed. The rejected candidate was not deployed,
and no performance improvement or other-architecture coverage is claimed.

## Rejected: explicitly copy the two root interface IDs (2026-09-19)

Starting from `c09910f32`, tried replacing the fixed two-iteration loop in
`ComBase::CopyIIDsToArray` with assignments to `pResult[first]` and
`pResult[first + 1]`, reading entries zero and one from the existing local
IID table. A `static_assert` tied the table's two-entry count to the explicit
copies. The source was `dxaml\xcp\components\com\inc\ComBase.h`.

This was distinct from the rejected pointer-offset rewrite: it removed the
root loop, rather than changing its indexing expression. The existing
table remained the source of the `IUnknown` and `IInspectable` values and
order. Counts, derived-map traversal, allocation, output publication,
ownership, errors, and virtual signatures were unchanged.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `c09910f32`, fresh baseline | 14,254,080 | 14,253,056 | 14,263,336 |
| Explicit root copies, rejected | 14,306,816 | 14,305,792 | 14,316,328 |
| Restored source after `prodtest` | 14,254,080 | 14,253,056 | 14,263,336 |

The candidate grows the DLL by **52,736 bytes (51.5 KiB)**, all in raw
`.text`. Virtual `.text` grows by 52,992 bytes; other section sizes are
unchanged. The `EnumReference<T>::CopyIIDsToArray` family decreases from
16,836 to 16,653 attributed bytes, with 183 representatives in both builds,
but that small local decrease does not represent the overall effect.
`Aggregate.g.obj` attribution grows from 1,569,592 to 1,620,167 bytes.
These figures locate the larger impact without establishing a particular
inlining or folding mechanism; they are not added to file-size differences.

Rejected the candidate and restored `ComBase.h` byte-for-byte. Rebuilding
restored all baseline size metrics and every baseline `.text` byte.
This turn retains no source change. Cumulative file savings remain
**281,088 bytes**. Avoid retrying explicit root IID-loop unrolling as a
standalone optimization.

### This turn's validation and provenance

Candidate and restored-source binlogs confirm affected compilation and
LTCG linking. The final `prodtest` build succeeded. Builds retained
same-process initialization with `amd64fre /nopgo`, `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
Export identities are unchanged; restored PE security characteristics
match baseline.

The full CalendarView suite ran on `ge_current-260820-Desktop` in WPF mode
against a freshly generated and deployed restored-source payload:
**121 total, 121 passed, 0 failed, 0 blocked, 0 not run, 0 skipped**.
Both previously allowed failing tests passed in this run. This does not
establish why they failed earlier or imply a source fix: the optimization
was removed before this run. The complete VM log contains all 121 distinct
passing results.

The built DLL, preserved final copy, local payload root/Test copies, and
VM root/Test copies share SHA256:
`7176201EE3B7880FD5C87C9BCD07A73D4A0922B143E48F66CD5C2E656DFE6EB7`.

Evidence is under `D:\x1\artifacts\ralph\20260919-180724-14a7bf42`.
The numbered baseline, candidate, and restored directories preserve
measurements, hashes, source patches, receipts, and tool identity.
`compiland-growth.json` and the family reports contain supporting
attribution. `final-test-build.log`, `final-test-build.binlog`,
`restored-native.binlog`, and `build-events.json` preserve build evidence.
`tests.log`, `vm-testrun-output.log`, `WexLogFileOutput`,
`tested-dll-hashes.json`, `test-result.json`, and `verification.json`
preserve this turn's validation.
All core collections verified the complete frozen SizeBench manifest and
used managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed. The candidate was not runtime-tested,
and no performance benefit or coverage of other architectures is claimed.

## Rejected: scoped cleanup for untyped collection append (2026-09-19)

Starting from `9272f4964`, tried replacing the raw pointer and goto cleanup
in `PresentationFrameworkCollection<T>::UntypedAppend` with an existing
`ctl::ComPtr` owner and early HRESULT propagation. The only source edit was
in `dxaml\xcp\dxaml\lib\JoltCollections.h`.

The candidate kept the thread check before querying or appending, preserved
the query helper's null-input behavior, and held the queried reference
through `Append`. The smart pointer released that reference on success and
failure. It retained the full `Append` HRESULT, including successful values
other than `S_OK`, and failure reporting before release. It changed no
interface, class layout, metadata, or build option. Diagnostic expression
text and source locations changed.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `9272f4964`, fresh baseline | 14,254,080 | 14,253,056 | 14,263,336 |
| Scoped append cleanup, rejected | 14,254,080 | 14,253,056 | 14,263,448 |
| Restored source after `prodtest` | 14,254,080 | 14,253,056 | 14,263,336 |

The candidate does **not reduce the actual DLL file**. Raw section sizes
are unchanged. Virtual `.text` grows by 96 bytes and `.data` by 16 bytes.
The targeted template family decreases from 8,125 to 8,080 attributed bytes,
with 37 representatives in both builds, but this is not a whole-file saving.
The family falls out of the candidate's top-20 report; the complete offline
name-filtered query confirms that it remains present. These attribution
figures are not added to section or file differences.

Rejected the candidate and restored `JoltCollections.h` byte-for-byte.
The final build reproduces all baseline size metrics and every baseline
`.text` byte. No source optimization is retained. Cumulative actual savings
remain **281,088 bytes (274.5 KiB)** from the original 14,535,168-byte DLL.
Avoid repeating this scoped-cleanup rewrite as a standalone size experiment.

### This turn's validation and provenance

The candidate and restored-source binlogs confirm affected compilation and
LTCG linking. The final `prodtest` build succeeded. All builds used
same-process initialization with `amd64fre /nopgo`, `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
Export identities are unchanged; restored PE security characteristics
match baseline.

The full CalendarView suite ran on `ge_current-260820-Desktop` in WPF mode
with a refreshed and deployed restored-source payload:
**121 total, 119 passed, 2 failed, 0 blocked, 0 not run, 0 skipped**.
The complete VM log confirms all 121 unique results. Only `TestCICEvents`
and `VerifySelfAdaptivePanel` failed, both at
`IsTrue(didOutputMatchMaster)` in `Utilities::VerifySuccess`, line 1627.
These exactly match the allowed baseline failures; no new failure was
observed. The previous turn's all-passing result is separate evidence and
does not establish the cause of this variation.

The built DLL, preserved final copy, local payload root/Test copies, and
VM root/Test copies share SHA256:
`3D399D5AD89B76B5891034BD2E5B4961EBFDA5C4B2FBC8844657F1886B97C50B`.

Evidence is under `D:\x1\artifacts\ralph\20260919-185211-c33c728d`.
The numbered baseline, candidate, and restored directories preserve
measurements, source patches, hashes, receipts, and tool identity.
`append-family.json` in the baseline and candidate directories contains
the complete family queries. `final-test-build.log`, the saved binlogs,
and `build-events.json` preserve build evidence. `tests.log`,
`vm-testrun-output.log`, `WexLogFileOutput`, `tested-dll-hashes.json`,
`test-result.json`, and `verification.json` preserve this turn's validation.
All three core collections verified the full frozen SizeBench manifest and
used managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed. The candidate was not runtime-tested;
no performance benefit or coverage of other architectures is claimed.

## Follow-up: share converted event-reference cleanup (2026-09-19)

Files: `dxaml\xcp\dxaml\lib\JoltClasses.h` and
`dxaml\xcp\dxaml\lib\JoltClasses.cpp`.

Move the two final releases in `CEventSourceBase::UntypedRaise` and
`CRoutedEventSourceBase::UntypedRaise` into
`ReleaseConvertedEventReferences(IUnknown*, IUnknown*, HRESULT)`.
The helper releases the converted source before the converted arguments
and returns the original HRESULT. Both templates retain their interface
queries, `Raise` call, failure reporting, and cleanup convergence.

Null pointers remain permitted. Each queried reference is consumed exactly
once, including when the second query fails or both references identify the
same object. The change adds no reference acquisition, allocation, thread
transition, interface, vtable slot, instance field, or exported API.
Generated code, metadata, and compiler/linker settings are unchanged.

### Measurements

All values are bytes, using x64 Release with PGO off.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `09aded999`, fresh baseline | 14,254,080 | 14,253,056 | 14,263,336 |
| Shared event cleanup | 14,247,424 | 14,246,400 | 14,256,704 |
| Restored source, recompiled control | 14,254,080 | 14,253,056 | 14,263,336 |
| Reapplied candidate, recompiled | 14,247,424 | 14,246,400 | 14,256,704 |
| Final candidate after `prodtest` | 14,247,424 | 14,246,400 | 14,256,704 |

The actual DLL is **6,656 bytes (6.5 KiB) smaller**. Cumulative actual
savings are **287,744 bytes (281 KiB)** from the original 14,535,168-byte
DLL. All raw savings are in `.text`. Virtual `.text` decreases by 6,608
bytes and `.data` by 48 bytes; `.pdata` and `.reloc` each grow by 12 bytes.
Other section sizes are unchanged.

The largest `CEventSourceBase::UntypedRaise` family decreases from 18,340
to 15,610 attributed bytes with 70 representatives. The selected routed
family decreases from 6,550 to 5,575 with 25 representatives. Complete
family queries preserve the other signature groups. These overlapping
attributions and heuristic estimates are not added to file-size savings.

Resolved primary blocks show the CalendarView day-item-changing
`UntypedRaise` shrinking from 262 to 223 bytes. The candidate calls one
68-byte shared cleanup block containing the same two null-guarded,
CFG-dispatched releases, in the same order. Complete collected `.text`
symbol coverage finds one helper symbol, not a set of specialized clones.
This sharing result is based on emitted code, not on an assumption that
`noinline` prevents LTCG specialization.

Restoring the source reproduces every baseline size metric and every
baseline `.text` byte. Reapplying the exact candidate reproduces every
candidate metric and `.text` byte. The final product/test build retains
those candidate metrics and bytes. Binlogs confirm affected compilation
and LTCG linking for both transitions and the repeated candidate.

### Runtime tradeoff and review

The inspected caller makes an additional ordinary helper call, **not a
tail jump**. The helper saves two registers and retains the same release
dispatch; it adds no allocation or reference-count operation. This is a
size optimization with a small call overhead, not a speed improvement.

A standalone source-level probe using the installed compiler measured
24 alternating samples of two mock interface queries plus cleanup. Median
times were 25.46 ns before and 26.38 ns after, about 0.92 ns or 3.6% more
for that synthetic operation. The probe also checked all null combinations,
two references to the same object, source-before-arguments release order,
and preservation of `S_OK`, `S_FALSE`, `E_NOINTERFACE`, and `E_FAIL`.
Those checks passed. The probe used optimized x64 code with CFG, stack
protection, and Spectre mitigation, without LTCG. It does not execute the
linked DLL or establish application event-path performance.

The bounded additional call is accepted for the reproducible 6.5 KiB
reduction, with that measured tradeoff recorded. No claim is made about
application throughput, debug performance, or other architectures.
Source review found no ownership, HRESULT, ABI, or reentrancy contract
change. Release-side call stacks now include the helper.

### This turn's validation and provenance

All builds used same-process initialization with `amd64fre /nopgo`,
`PGOBuildMode=Off`, `Configuration=Release`, `Platform=x64`, and
`VCToolsVersion=14.44.35207`. The final `prodtest` build succeeded.
All eight export ordinal/name identities and PE security characteristics
match baseline.

The full CalendarView suite ran on `ge_current-260820-Desktop` in WPF mode
against a refreshed and deployed candidate payload:
**121 total, 119 passed, 2 failed, 0 blocked, 0 not run, 0 skipped**.
The complete VM log confirms 121 unique results. Only `TestCICEvents` and
`VerifySelfAdaptivePanel` failed, both with the allowed
`IsTrue(didOutputMatchMaster)` assertion in `Utilities::VerifySuccess`,
line 1627. No new failing test or assertion was observed.

The built DLL, preserved final copy, local payload root/Test copies, and
VM root/Test copies share SHA256:
`2F10A900BCB64646922A276091354F13E02DB7DDB236F37C006E4A9492B224DA`.

Evidence is under `D:\x1\artifacts\ralph\20260919-193137-f0921ec6`.
The five numbered directories preserve frozen binary measurements,
hashes, patches, receipts, and tool identity. The first two also contain
full `.text` symbol coverage, resolved primary blocks, disassembly, and
complete event-family queries. `EventCleanupProbe.cpp`, `probe.csv`,
`probe-result.json`, and the probe build log record the isolated checks.
`final-test-build.log`, saved binlogs, `build-events.json`, `tests.log`,
`vm-testrun-output.log`, `WexLogFileOutput`, `tested-dll-hashes.json`,
`test-result.json`, and `verification.json` record this turn's evidence.
All five core collections verified the complete frozen SizeBench manifest
and used managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed.

## Rejected: return factory failure HRESULT through cleanup (2026-09-19)

Starting from `0b9541c6f`, tried changing the existing
`ComObjectBase::ReleaseFailedInstance` helper from a void function to an
HRESULT-returning function that takes and returns the original failure
result. The typed `ComObject<T>::CreateInstance` cleanup returned that
helper call directly instead of preserving `hr` across the void call.
The experiment touched `ComObject.h`, `ComObjectBase.h`, and
`ComObjectBase.cpp` under `dxaml\xcp\components\com`.

The hypothesis was that this would reduce caller register saves or return
sequences. It left successful factory paths, allocation, initialization,
typed output casts, debug leak handling, failure reporting, and the
guarded delegating release unchanged. It introduced no new helper,
instance field, vtable entry, export, or build option.

| Source state | DLL file | Analyzed sections | Section virtual size |
|---|---:|---:|---:|
| `0b9541c6f`, fresh baseline | 14,247,424 | 14,246,400 | 14,256,704 |
| Failure-result return, rejected | 14,268,928 | 14,267,904 | 14,278,252 |
| Restored source after `prodtest` | 14,247,424 | 14,246,400 | 14,256,704 |

The candidate grows the actual DLL by **21,504 bytes (21 KiB)**.
All raw growth is in `.text`. Virtual `.text` grows by 21,424 bytes,
`.rdata` by 16 bytes, and `.pdata` by 108 bytes.
The same-type typed factory family grows from 45,178 attributed bytes
across 181 representatives to 45,765 across 182. The different-type
family grows from 15,257 across 60 to 15,603 across 61. These local
attributions do not explain the full growth and are not added to file
differences. No precise inlining or register-allocation cause is claimed.

Rejected the candidate and restored all three source files byte-for-byte.
The final build reproduces every baseline size metric and every baseline
`.text` byte. This turn retains no source change. Cumulative actual savings
remain **287,744 bytes (281 KiB)**. Do not repeat HRESULT-through-cleanup
for this factory helper as a standalone size experiment.

### This turn's validation and provenance

Candidate and restored-source binlogs confirm affected compilation and
LTCG linking. The final `prodtest` build succeeded. Builds used
same-process initialization with `amd64fre /nopgo`, `PGOBuildMode=Off`,
`Configuration=Release`, `Platform=x64`, and `VCToolsVersion=14.44.35207`.
Export identities are unchanged; restored PE security characteristics
match baseline.

The full CalendarView suite ran on `ge_current-260820-Desktop` in WPF mode
against a refreshed and deployed restored-source payload:
**121 total, 120 passed, 1 failed, 0 blocked, 0 not run, 0 skipped**.
The complete VM log confirms 121 unique results. Only `TestCICEvents`
failed, with the allowed `IsTrue(didOutputMatchMaster)` assertion in
`Utilities::VerifySuccess`, line 1627. `VerifySelfAdaptivePanel` passed
in this run; its earlier failure cause remains unknown.

The built DLL, preserved final copy, local payload root/Test copies, and
VM root/Test copies share SHA256:
`BD1411B0726DA72A8A6F805F76CA3B9B8C4D33D362E4390FA7779CD650CA123C`.

Evidence is under `D:\x1\artifacts\ralph\20260919-202747-d8588807`.
The three numbered directories preserve measurements, source patches,
hashes, receipts, and tool identity. `factory-families.json` in the first
two directories preserves complete offline family queries.
`final-test-build.log`, saved binlogs, and `build-events.json` record build
evidence. `tests.log`, `vm-testrun-output.log`, `WexLogFileOutput`,
`tested-dll-hashes.json`, `test-result.json`, and `verification.json`
record this turn's validation.
All three core collections verified the complete frozen SizeBench manifest
and used managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed. The rejected candidate was not deployed
or performance-tested; no performance or other-architecture benefit is
claimed.

## Rejected: prevent boxed runtime-name implementation inlining (2026-09-19)

Starting from `d7a39b0249ff86621515b93455026883d2cd43da`, this experiment
added `__declspec(noinline)` to the existing explicit
`ReferenceBase<T>::GetRuntimeClassNameImpl` specializations through
`REFERENCE_ELEMENT_NAME_IMPL` in `dxaml\xcp\dxaml\lib\DXamlTypes.h`.
The hypothesis was that keeping the already-emitted virtual implementation
out of `ComObject<T>::GetRuntimeClassName` wrappers might reduce duplication.
It did not change the function bodies, HSTRING ownership, HRESULT handling,
controlling-outer dispatch, or interface shape.

Fresh baseline SizeBench evidence ranked the COM runtime-name family at
32,588 attributed bytes across 785 representatives and the reference
implementation family at 10,149 bytes across 199 representatives.
Both families retained exactly those figures in the candidate. More
importantly, every raw `.text` byte and all three whole-image metrics were
unchanged. This is not evidence that noinline guarantees sharing or prevents
LTCG specialization; it produced no emitted-code benefit here.

| Measurement | Fresh baseline | Candidate | Delta |
| --- | ---: | ---: | ---: |
| Actual DLL file bytes | 14,247,424 | 14,247,424 | 0 |
| Analyzed section bytes | 14,246,400 | 14,246,400 | 0 |
| Virtual section bytes | 14,256,704 | 14,256,704 | 0 |

The candidate rebuilt affected code, including `Boxes.g.cpp`, and performed
an LTCG link. Both builds used x64 Release (`amd64fre`) with explicit
`/nopgo`, reported `PGOBuildMode=Off`, and used toolset 14.44.35207.
Build logs and the candidate binlog report success.

Rejected because the actual DLL did not shrink. The owned header was restored
byte-for-byte against its pre-edit backup; this commit changes documentation
only. No source change or performance benefit is retained. Incremental
savings are zero. Cumulative actual savings remain 287,744 bytes (281 KiB)
against the original 14,535,168-byte baseline.

**Tests not run: experiment rejected.** Per the updated rejection policy,
this turn did not build prodtest, set up the VM, or run or reuse prior
CalendarView results. No runtime performance claim is made.

Evidence is under
`D:\x1\artifacts\ralph\20260919-211050-f35d3c2b`.
`000-baseline` and `001-noinline-reference-name` preserve source revision
and patch, build command/log/binlog, DLL/PDB snapshots and hashes, core
SizeBench collections, offline reports, receipts, stderr, and tool identity.
`ownership.json` and `originals` record ownership and exact source backups;
`verification.json` records unchanged metrics, identical `.text`, and exact
restoration; `build-events.json` records compilation and link evidence.
Both measurements verified the complete frozen CLI manifest and used managed
identity `e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No CLI defect was observed. Investigate another family, such as
`ActivationFactoryCreator`, rather than repeating this noinline annotation.

## Rejected: use the activation factory's null result for failure (2026-09-19)

Starting from `256354ba2a55cc79aa27fdc8627391008063bd70`, this experiment
removed the explicit failed-HRESULT branch in
`ctl::ActivationFactoryCreator<T>::CreateActivationFactory`, in
`dxaml\xcp\dxaml\lib\comTemplateLibrary.h`. Instead, it ignored the HRESULT
and returned the existing null-preserving `interface_cast` of the output.
The local output starts as null, and the typed `ComObject<T>::CreateInstance`
overload writes it only after successful initialization. Failure cleanup,
the typed factory overload, the debug leak-check flag, successful reference
transfer, and interface conversion remained unchanged.

Fresh baseline SizeBench evidence attributed 10,688 bytes across 58
representatives to this activation-factory family. The candidate increased
that figure to 11,140 bytes across 57 representatives. Fewer representatives
did not mean a smaller DLL. Family totals are not added to section savings.

| Measurement | Fresh baseline | Candidate | Delta |
| --- | ---: | ---: | ---: |
| Actual DLL file bytes | 14,247,424 | 14,254,592 | +7,168 |
| Analyzed section bytes | 14,246,400 | 14,253,568 | +7,168 |
| Virtual section bytes | 14,256,704 | 14,263,956 | +7,252 |

Raw growth was entirely in `.text`. Virtual `.text` grew 7,264 bytes and
`.pdata` shrank 12 bytes. Other section sizes were unchanged. The candidate
compiled affected consumers, including `Boxes.g.cpp`, and performed an LTCG
link. Both builds used x64 Release (`amd64fre`) with explicit `/nopgo`,
reported `PGOBuildMode=Off`, and used toolset 14.44.35207. Build logs and the
candidate binlog report success.

Rejected because the actual DLL grew 7,168 bytes (7 KiB). The owned header
was restored byte-for-byte against its pre-edit backup. This commit changes
only documentation; no speculative source change is retained. Incremental
savings are zero, and cumulative actual savings remain 287,744 bytes
(281 KiB). No runtime or other-architecture benefit is claimed.

**Tests not run: experiment rejected.** This turn skipped prodtest, VM
setup, and CalendarView execution under the updated rejection policy.
There was no post-restoration build; the mutable build output still contains
the rejected candidate until the next build. The next experiment must build
the restored source for its baseline, not measure that stale output.

Evidence is under
`D:\x1\artifacts\ralph\20260919-212209-042193f5`.
`000-baseline` and `001-factory-null-result` preserve revision and patch,
build command/log/binlog, DLL/PDB snapshots and hashes, core SizeBench
collections, offline reports, receipts, stderr, and tool identity.
Each `factory-family.json` contains the complete offline query for the
selected family. `ownership.json` and `originals` record ownership and
backups; `verification.json` records section metrics and exact source
restoration; `build-events.json` records compilation and link evidence.
Both measurements verified the complete frozen CLI manifest and used managed
identity `e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No CLI defect was observed. Keep the explicit factory HRESULT branch.

## Follow-up: transfer tracker iterators directly (2026-09-19)

Starting from `7b71afaaf02da80867382804627c5f152ca34e12`, change
`TrackerView<T>::First` and `TrackerCollection<T>::First` in
`dxaml\xcp\dxaml\lib\TrackerCollections.h` to create a concrete iterator
pointer and transfer it directly to the caller. This removes a local
interface `ComPtr` and its redundant failure-path release check.

The factory still initializes the iterator and releases it on initialization
failure. It writes the output only on success. After successful creation,
`SetCollection` returns void and the iterator is transferred to the output.
Thread checking, output-pointer error origination, HRESULT propagation,
tracker registration and collection-reference acquisition remain unchanged.
The concrete-to-interface conversion uses the same public base interface;
no QueryInterface call, extra reference, helper, vtable change, or public
signature change is introduced.

| Measurement | Fresh baseline | Final candidate | Reduction |
| --- | ---: | ---: | ---: |
| Actual DLL file bytes | 14,247,424 | 14,246,400 | 1,024 |
| Analyzed section bytes | 14,246,400 | 14,245,376 | 1,024 |
| Virtual section bytes | 14,256,704 | 14,255,548 | 1,156 |

Incremental actual savings are **1,024 bytes (1 KiB)**. Cumulative actual
savings are **288,768 bytes (282 KiB)** against the original
14,535,168-byte baseline. Raw savings are in `.text`; virtual `.text`
shrinks 1,152 bytes and `.reloc` shrinks 4 bytes.

Each tracker `First` family falls from 4,158 to 3,582 attributed bytes
across 18 representatives. These family figures may overlap and are not
summed or substituted for file savings. Resolved AutomationPeer primary
blocks shrink from 231 to 199 bytes for both methods. Disassembly shows the
same initialization, registration, and reference-setting operations, without
the local null-test/virtual-release cleanup. The inspected collection body
reserves 32 rather than 48 bytes of local stack space. No runtime timing
claim is made.

Restoring and recompiling the original header reproduces all baseline
metrics and every baseline `.text` byte. Reapplying the candidate and
building prodtest reproduces all candidate metrics and every candidate
`.text` byte. The eight exports and PE security characteristics remain
unchanged. All builds use x64 Release (`amd64fre`), explicit `/nopgo`,
`PGOBuildMode=Off`, and toolset 14.44.35207. Binlogs confirm affected
compilation and LTCG linking.

One attempted control build, `002-restored-control`, was invalid: copying
the backup preserved an old header timestamp, so the incremental build
retained the candidate. Its evidence is kept but excluded from comparisons.
After refreshing that owned header's timestamp,
`002b-restored-recompiled` performed the required recompile and reproduced
the baseline. The final source restores the original trailing blank line;
`final-source-formatting.json` verifies that this is the only difference
from the source used for the final measured and tested candidate.

### Behavior review and validation

The removed owner has no HRESULT failure path to cover after successful
creation. `SetCollection` calls the existing void `SetPtrValue`; its
registration implementation uses the existing allocation policy and is
built through the lifetime component's `_HAS_EXCEPTIONS=0` configuration.
This does not introduce an exception-handling policy or change allocation
behavior. A future fallible setup step would need explicit local ownership
again. No fault-injection or other-architecture validation was performed.

This turn's complete CalendarViewIntegrationTests run used
`ge_current-260820-Desktop`, WPF hosting, and `amd64fre /nopgo`:
121 total, 119 passed, 2 failed, and none blocked, skipped, or not run.
The failures were exactly `TestCICEvents` and `VerifySelfAdaptivePanel`,
each with the approved `IsTrue(didOutputMatchMaster)` assertion in
`Utilities.cpp` line 1627. These are allowed baseline failures, not a claim
that all tests passed or that their cause is known. The runner returned 1;
the complete raw VM log and all 121 unique test endings were reviewed.

Built, frozen-final, local payload root/Test, and remote payload root/Test
DLL copies all matched SHA256
`820F05C0F716AD20FBFF8623378CF4CAB35F4A7E53975E271E5356E9784AC9BF`.

### Provenance

Evidence is under
`D:\x1\artifacts\ralph\20260919-213559-66ad6105`.
`000-baseline`, `001-direct-tracker-iterator`,
`002b-restored-recompiled`, and `003-candidate-final` preserve measurements,
snapshots, hashes, source patches, receipts, and frozen tool identity.
`first-family-full.json`, resolved primary-block reports and disassembly
provide the emitted-code evidence. Ownership and source backups are kept
separately. `final-test-build.log`, binlogs, and `build-events.json` record
build evidence; `tests.log`, `vm-testrun-output.log`, `WexLogFileOutput`,
`test-result.json`, `tested-dll-hashes.json`, and `verification.json` record
this turn's validation. All core collections verified the complete frozen
manifest and managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed.

## Follow-up: transfer tracker collection views directly (2026-09-19)

Starting from `da8bf1b2c35573ce0ea16ce1bc484ea26d005b21`, change
`TrackerCollection<T>::GetView` in
`dxaml\xcp\dxaml\lib\TrackerCollections.h` to create a concrete
`TrackerView<T>` pointer and transfer it directly to the output.
This removes the local interface `ComPtr`, its downcast, and redundant
failure-path release check. The factory still initializes the view and
releases it on failure; it only writes the output on success.

Thread checking, output-pointer error origination, HRESULT propagation,
tracker registration, and collection-reference acquisition are unchanged.
After successful creation, `SetCollection` returns void and ownership
transfers to the caller. The implicit base conversion adjusts the concrete
pointer to the same `IVectorView<T>` interface. No extra AddRef,
QueryInterface, helper, interface change, or build-setting change is added.

| Measurement | Fresh baseline | Final candidate | Reduction |
| --- | ---: | ---: | ---: |
| Actual DLL file bytes | 14,246,400 | 14,245,888 | 512 |
| Analyzed section bytes | 14,245,376 | 14,244,864 | 512 |
| Virtual section bytes | 14,255,548 | 14,255,104 | 444 |

Incremental actual savings are **512 bytes (0.5 KiB)**. Cumulative actual
savings are **289,280 bytes (282.5 KiB)** against the original
14,535,168-byte baseline. Raw savings are in `.text`. Virtual `.text`
shrinks 448 bytes, `.pdata` grows 12 bytes, and `.reloc` shrinks 8 bytes.

The complete GetView family query shows the tracker family falling from
4,464 to 3,999 attributed bytes across 18 representatives. These attributed
figures are not summed with whole-file savings. The resolved AutomationPeer
primary block shrinks from 248 to 217 bytes. Its disassembly retains thread
and pointer checks, initialization, registration, and reference-setting,
but removes the local guarded release. Local stack reservation falls from
48 to 32 bytes. No timing benchmark or other-architecture benefit is claimed.

Restoring the exact original header and refreshing its timestamp before
recompiling reproduces every baseline metric and `.text` byte. Reapplying
the candidate and building prodtest reproduces every candidate metric and
`.text` byte. The eight exports and PE security characteristics are unchanged.
All builds used x64 Release (`amd64fre`), explicit `/nopgo`,
`PGOBuildMode=Off`, and toolset 14.44.35207. Binlogs confirm affected
compilation and LTCG linking.

As with the preceding iterator change, the removed local owner has no
HRESULT failure path after successful creation. Tracker registration uses
the existing allocation policy and lifetime component configuration
(`_HAS_EXCEPTIONS=0`); neither changes here. A future fallible setup step
would need local ownership again. Fault injection was not performed.

This turn's full CalendarViewIntegrationTests run used the local VM
`ge_current-260820-Desktop`, WPF hosting, and `amd64fre /nopgo`:
121 total, 120 passed, 1 failed, none blocked, skipped, or not run.
The only failure was `TestCICEvents`, with the approved
`IsTrue(didOutputMatchMaster)` assertion in `Utilities.cpp` line 1627.
`VerifySelfAdaptivePanel` passed. The runner returned 1; the complete raw
log, exact failure assertion, and all 121 unique test endings were reviewed.
This is an allowed baseline failure, not an all-tests-passed claim.

The built DLL, frozen final snapshot, local payload root/Test copies, and
remote payload root/Test copies all matched SHA256
`70F5D460F565E7A437085A8AEB445B3DBA9F26FEE3F5F65BEC5CF3641A80C53A`.

Evidence is under
`D:\x1\artifacts\ralph\20260919-223317-1631e747`.
`000-baseline`, `001-direct-tracker-view`, `002-restored-control`, and
`003-candidate-final` preserve snapshots, hashes, source patches,
measurements, receipts, and tool identity. Full GetView family queries,
resolved primary-block reports, and disassembly record emitted-code evidence.
Build logs/binlogs and `build-events.json` prove compilation and linking.
`tests.log`, `vm-testrun-output.log`, `WexLogFileOutput`, `test-result.json`,
`tested-dll-hashes.json`, and `verification.json` record fresh validation.
`final-source-formatting.json` records that final source differs from the
tested snapshot only by restoring original trailing blank-line formatting.
All core collections verified the complete frozen manifest and managed
identity `e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed.

## Rejected: use ctl ownership for untyped insertion (2026-09-19)

Starting from `ada7ae440e9836db530a0534f9637504dba47151`, try changing
the local typed owner in `PresentationFrameworkCollection<T>::UntypedInsertAt`
from `wrl::ComPtr` to the existing `ctl::ComPtr` in
`dxaml\xcp\dxaml\lib\JoltCollections.h`. The hypothesis was that using
the repository's existing cleanup implementation could improve linked-code
sharing. No new helper, manual lifetime management, or build setting was added.

Both owners start null, expose the same typed output slot for QueryInterface,
and release the acquired reference on scope exit. Thread checking, the borrowed
input query, insertion order, HRESULT propagation, and cleanup on failure
were unchanged. This was a one-line experiment, not another removal of the
input AddRef already eliminated by an earlier accepted change.

| Measurement | Fresh baseline | Candidate | Candidate minus baseline |
| --- | ---: | ---: | ---: |
| Actual DLL file bytes | 14,245,888 | 14,245,888 | 0 |
| Analyzed section bytes | 14,244,864 | 14,244,864 | 0 |
| Virtual section bytes | 14,255,104 | 14,255,412 | +308 |

Every raw section size stayed unchanged. Virtual `.text` grew 160 bytes,
`.rdata` grew 128 bytes, `.pdata` grew 12 bytes, and `.reloc` grew 8 bytes.
The complete template-family query shows the targeted untyped insertion
family shrinking from 9,435 to 9,333 attributed bytes across the same
37 representatives. That local reduction is not a whole-file improvement
and does not justify keeping the change. The tracker insertion family
remained 4,458 attributed bytes across 18 representatives; it was not edited.
No overlapping symbol totals or heuristic savings are added to these metrics.

Reject the experiment because the actual DLL is not smaller. Restore the
header exactly to its backed-up SHA256; only this description update is
committed. Incremental actual savings are **0 bytes**. Cumulative accepted
savings remain **289,280 bytes (282.5 KiB)** against the original
14,535,168-byte baseline.

Tests not run: experiment rejected. No prodtest build, VM setup, or post-restore
test run was required. Previous sections describe historical validation only,
not validation of this rejected candidate. No runtime-performance claim is made.

Evidence is under
`D:\x1\artifacts\ralph\20260919-232403-203d7f4b`.
`000-baseline` and `001-ctl-insertion-owner` preserve DLL/PDB snapshots,
hashes, source revision/patch, build commands/logs/binlogs, complete core
SizeBench reports, offline insertion-family queries, receipts, and tool
identity. Both builds used x64 Release (`amd64fre`), explicit `/nopgo`,
`PGOBuildMode=Off`, and toolset 14.44.35207. `build-events.json` confirms
affected compilation and LTCG linking in both builds. `ownership.json`,
`originals`, and `restored-source-hashes.json` record the owned files and
exact restoration. Both measurements verified the frozen CLI manifest and
managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed.

The mutable BuildOutput DLL/PDB still contain the rejected candidate.
The next turn must build restored source for its fresh baseline, rather
than treating those outputs as accepted binaries. A separate unexplored
lead in this baseline is `GetCollectionItemInternal` (7,601 attributed bytes
across 19 representatives); its ownership and failure paths need source
review before proposing any change.

## Rejected: query diagnostic collection items into an empty owner (2026-09-19)

Starting from `4fe83be92b010f66d6c0888290c93f422fb9c46b`, try selecting
the existing raw-output `ctl::do_query_interface` overload in
`GetCollectionItemInternal<Item, IItem>` in
`dxaml\xcp\dxaml\lib\InternalDebugInterop.cpp`:

```cpp
IFC_RETURN(ctl::do_query_interface(*spCollection.ReleaseAndGetAddressOf(), pValue));
```

The collection owner is newly constructed and empty. This writes the acquired
reference directly into it instead of querying into a temporary owner and
swapping. The adjacent `GetCollectionSizeInternal` already uses this pattern
from an earlier accepted experiment. That earlier result does not establish
a benefit for the item accessor.

The requested IID, null-input handling, output initialization, query/GetAt/As
order, and final output ownership transfer remain unchanged. All local owners
still clean up on every return, including a populated query result on failure.
The existing `IFC_RETURN` sites and final `S_OK` remain unchanged. No public
interface, metadata, security setting, or build flag changes.

| Measurement | Fresh baseline | Candidate | Candidate minus baseline |
| --- | ---: | ---: | ---: |
| Actual DLL file bytes | 14,245,888 | 14,245,888 | 0 |
| Analyzed section bytes | 14,244,864 | 14,244,864 | 0 |
| Virtual section bytes | 14,255,104 | 14,255,104 | 0 |

Every individual section's raw and virtual sizes are unchanged. The complete
item-accessor template-family query falls from 7,601 to 7,564 attributed
bytes across the same 19 representatives. The `.text` hashes differ, so this
is not an identical-code result, but there is no file-size improvement.
The attributed 37-byte change is not added to whole-file savings.

Reject the experiment and restore the source exactly to the backed-up SHA256.
Only this description update is committed. Incremental actual savings are
**0 bytes**; cumulative accepted savings remain **289,280 bytes (282.5 KiB)**
against the original 14,535,168-byte baseline.

Tests not run: experiment rejected. No prodtest build, VM setup, or
post-restore test run was required. Historical test results above do not
validate this candidate. No runtime-performance or other-architecture
benefit is claimed.

Evidence is under
`D:\x1\artifacts\ralph\20260919-233803-db885a74`.
`000-baseline` and `001-direct-item-query` preserve DLL/PDB snapshots,
hashes, source revision/patch, commands, build logs/binlogs, core SizeBench
reports, complete item-family queries, receipts, and frozen-tool identity.
Both builds used x64 Release (`amd64fre`), explicit `/nopgo`,
`PGOBuildMode=Off`, and toolset 14.44.35207. `build-events.json` records
`InternalDebugInterop.cpp` compilation, LTCG linking, and successful completion
for both builds. `verification.json` records family totals and per-section
sizes/hashes. `ownership.json`, `originals`, and `restored-source-hashes.json`
record ownership and restoration. Both measurements verified the full frozen
manifest and managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed.

BuildOutput still contains the rejected candidate DLL/PDB. Rebuild the
restored source before the next baseline. Do not retry this isolated overload
selection as a demonstrated saving. A distinct future lead is reducing
repeated post-GetAt interface conversion and cleanup in these diagnostic
accessors; preserve null output on failure and release order, and measure
any proposed sharing rather than assuming noinline prevents specialization.

## Follow-up: share diagnostic collection-item conversion (2026-09-20)

Starting from `b1de4bdf1fbfd2fa59eb517d3f76155985e1d75e`, extract the
fixed DependencyObject conversion from `GetCollectionItemInternal<Item, IItem>`
into the non-template `GetCollectionItemAsDependencyObject` helper in
`dxaml\xcp\dxaml\lib\InternalDebugInterop.cpp`.

The helper borrows the item's IUnknown pointer without adding a reference.
It uses the same raw `do_query_interface` operation as `ComPtr::As`, keeps
the queried reference in a local owner, and transfers it only on success.
It preserves null-item behavior (S_OK with a null result), normalizes
nonnegative query results to S_OK, and returns failures unchanged.
The accessor still initializes the caller's output to null before any work.
On failure, a populated query result is released before the item and then
the collection, matching the original destruction order. Collection querying,
GetAt, typed item ownership, and caller-visible interfaces are unchanged.

The accessor returns the helper's HRESULT directly. The first measured
version also used IFC_RETURN around the helper call, duplicating its failure
hook. Final review removed that redundant hook, then rebuilt and reran the
full suite. The final code retains one conversion-failure reporting site
before cleanup, now inside the helper.

| Measurement | Fresh baseline | Final candidate | Reduction |
| --- | ---: | ---: | ---: |
| Actual DLL file bytes | 14,245,888 | 14,243,840 | 2,048 |
| Analyzed section bytes | 14,244,864 | 14,242,816 | 2,048 |
| Virtual section bytes | 14,255,104 | 14,253,004 | 2,100 |

Incremental actual savings are **2,048 bytes (2 KiB)**. Cumulative actual
savings are **291,328 bytes (284.5 KiB)** against the original
14,535,168-byte baseline. Raw `.text` shrinks 2,048 bytes. Virtual `.text`
shrinks 2,112 bytes and `.pdata` grows 12 bytes; all other raw and virtual
section sizes are unchanged.

The complete accessor family query falls from 7,601 to 5,414 attributed
bytes across 19 unique representatives. Scoped code collection and
disassembly show all 20 emitted accessor primary blocks calling one
138-byte helper. Thus sharing is observed in the emitted code, not assumed
from noinline. The ColumnDefinition accessor's resolved primary block shrinks
from 404 to 290 bytes. These figures may overlap and are not added to the
whole-file saving.

The tradeoff is one additional ordinary helper call, its stack frame, and
its cookie check. The inspected caller retains its 64-byte local stack
reservation. No allocation, AddRef, or additional QueryInterface is introduced.
This path serves diagnostic collection inspection, not normal rendering.
No timing benchmark, fault injection, or other-architecture benefit is claimed.
The guarded COM calls, failure mitigation barriers, and stack-cookie checks
remain. The eight exported ordinal/name pairs and PE security characteristics
are unchanged; no build settings, class layouts, or metadata definitions change.

Restoring the exact original source and recompiling reproduces every baseline
size metric and `.text` byte. Recompiling the refined candidate through
prodtest reproduces every refined candidate metric and `.text` byte.
Binlogs confirm actual InternalDebugInterop.cpp compilation and LTCG linking,
not merely an up-to-date build. All builds use x64 Release (`amd64fre`),
explicit `/nopgo`, `PGOBuildMode=Off`, and toolset 14.44.35207.

The final full CalendarViewIntegrationTests run used
`ge_current-260820-Desktop`, WPF hosting, and `amd64fre /nopgo`:
121 total, 120 passed, 1 failed, none blocked, skipped, or not run.
The only failure was `TestCICEvents` with
`IsTrue(didOutputMatchMaster)` in Utilities.cpp line 1627.
`VerifySelfAdaptivePanel` passed. The raw log contains all 121 unique test
endings, the expected complete summary, and no additional error assertions.
This meets the allowed-failure gate; it is not an all-tests-passed result.
The earlier unrefined version ran 121 tests, with 119 passed and both exact
allowed failures. That first run is retained separately and was not reused
as final validation.

The final built DLL, frozen final snapshot, local payload root/Test copies,
and remote payload root/Test copies all match SHA256
`58ED589E1CDA6B7A301308868C556D4FBBEAC20111C3C48477312272C3DF83BF`.

Evidence is under
`D:\x1\artifacts\ralph\20260919-235014-ee40fcfc`.
`000-baseline`, `001-shared-item-conversion`, `002-restored-control`, and
`003-candidate-final` retain the initial experiment and control. The initial
candidate had the same actual size as the final version, but virtual size
14,253,352 bytes. `refined\001-refined` and
`refined\003-candidate-final` contain the final measurements and repeat.
Each measurement preserves snapshots, hashes, source revision/patch,
SizeBench receipts, and frozen-tool identity; build logs and binlogs record
the corresponding build commands. Core analysis verified the complete
frozen manifest and managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.

`refined\verification.json`, `refined\helper-sharing.json`, scoped symbol
reports, primary-block disassembly, and both `build-events.json` files record
attribution and emitted-code evidence. `refined\tests.log`,
`refined\vm-testrun-output.log`, `refined\WexLogFileOutput`,
`refined\test-result.json`, and `refined\tested-dll-hashes.json` record final
validation. `ownership.json`, `originals`, `restored-source-hash.json`,
`semantic-review.json`, and `progress.json` record ownership and review.
No SizeBench defect was observed.

## Rejected: keep ComObject construction out of line (2026-09-20)

Starting from `bd972c95bed990fff1ec85781699f858e65d0353`, add
`__declspec(noinline)` to the private `ctl::ComObject<TBASE>` constructor
in `dxaml\xcp\components\com\inc\ComObject.h`. The fresh SizeBench baseline
attributed 118,952 bytes to 500 `CreateComObjectInstanceNoInit` representatives.
The hypothesis was that typed and untyped factory entry points could share
construction code rather than repeat inlined base construction.

This was one constructor annotation, not a change to allocation, initialization,
ownership, aggregation, object layout, or error handling. No generator,
metadata, public interface, or build setting was changed. The experiment
measured the linked result rather than assuming noinline prevents LTCG
specialization.

| Measurement | Fresh baseline | Candidate | Candidate minus baseline |
| --- | ---: | ---: | ---: |
| Actual DLL file bytes | 14,243,840 | 14,279,168 | +35,328 |
| Analyzed section bytes | 14,242,816 | 14,278,144 | +35,328 |
| Virtual section bytes | 14,253,004 | 14,288,500 | +35,496 |

Raw `.text` grew 28,672 bytes and `.pdata` grew 6,656 bytes.
Virtual `.text` grew 28,608 bytes, `.pdata` grew 6,984 bytes, and `.rdata`
shrunk 96 bytes. All other section sizes were unchanged.

The complete family queries show allocation-wrapper attribution falling
from 118,952 to 42,000 bytes across the same 500 representatives.
However, the constructor family expanded from 10,652 bytes across
31 unique representatives to 210,625 bytes across 1,750 representatives.
These family totals can overlap and do not constitute an additive accounting
of the file delta. Smaller wrappers did not yield a smaller DLL; additional
out-of-line construction and unwind data outweighed any sharing benefit.

Reject the experiment and restore the header exactly to its backed-up SHA256.
Only this description update is committed. Incremental accepted savings
are **0 bytes**. Cumulative accepted savings remain **291,328 bytes
(284.5 KiB)** against the original 14,535,168-byte baseline.

Tests not run: experiment rejected. No prodtest build, VM setup, or
post-restore test run was required. Historical test results do not validate
this candidate. No runtime-performance or other-architecture benefit is claimed.

Evidence is under
`D:\x1\artifacts\ralph\20260920-005858-77f03e1a`.
`000-baseline` and `001-out-of-line-constructor` preserve snapshots, hashes,
source revision/patch, build commands/logs/binlogs, core SizeBench reports,
complete allocation/constructor family queries, receipts, and tool identity.
Both builds used x64 Release (`amd64fre`), explicit `/nopgo`,
`PGOBuildMode=Off`, and toolset 14.44.35207. The fresh baseline build was
up to date with the preceding accepted source. `build-events.json` confirms
affected `Boxes.g.cpp` compilation and LTCG linking for the candidate.
`ownership.json`, `originals`, and `restored-source-hashes.json` record exact
source ownership and restoration. Both measurements verified the complete
frozen manifest and managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed.

BuildOutput still contains the rejected candidate DLL/PDB. The next turn
must rebuild the restored source before measuring its baseline.
Avoid this broad constructor noinline annotation. More narrowly shared
non-template work may still be useful, but requires identifying a repeated
operation with preserved ownership and measuring its emitted implementation.

## Rejected: prevent ComBase destructor inlining (2026-09-20)

Starting from `13ee690692e29f339c70b773c849ae5d29b85413`, add
`__declspec(noinline)` to the existing virtual `ctl::ComBase` destructor
in `dxaml\xcp\components\com\inc\ComBase.h`. The fresh baseline attributed
23,375 bytes to 365 ComObject scalar-deleting-destructor representatives.
The hypothesis was that keeping the non-template base cleanup out of line
could reduce repeated cleanup in derived destructors. This is distinct from
the preceding rejected constructor annotation.

The experiment did not alter the destructor body: free-threaded marshaler
release still precedes weak-reference decoding and release. Virtual
destruction, ownership, aggregation, object layout, and build settings stayed
unchanged. No public interface or metadata definition changed.

| Measurement | Fresh baseline | Candidate | Candidate minus baseline |
| --- | ---: | ---: | ---: |
| Actual DLL file bytes | 14,243,840 | 14,243,840 | 0 |
| Analyzed section bytes | 14,242,816 | 14,242,816 | 0 |
| Virtual section bytes | 14,253,004 | 14,253,004 | 0 |

Every section's raw and virtual sizes are unchanged. The complete raw `.text`
bytes are also identical, with SHA256
`CAF48C7F9171BC0C3DFEE2CF5393E4C7C38ECE544ACB5BF292509856A0B302F8`
on both sides. Candidate compilation and LTCG linking did occur, so this
is an ineffective annotation rather than an up-to-date build mistaken for
a measurement. No conclusion about general noinline specialization behavior
is inferred from this particular result.

Reject the experiment and restore the header exactly to its backed-up SHA256.
Only this description update is committed. Incremental accepted savings
are **0 bytes**. Cumulative accepted savings remain **291,328 bytes
(284.5 KiB)** against the original 14,535,168-byte baseline.

Tests not run: experiment rejected. No prodtest build, VM setup, or
post-restore test run was required. Earlier validation results do not apply
to this candidate. No runtime-performance or other-architecture benefit
is claimed.

Evidence is under
`D:\x1\artifacts\ralph\20260920-010928-5e41cf85`.
`000-baseline` and `001-shared-base-destructor` preserve DLL/PDB snapshots,
hashes, source revision/patch, commands, build logs/binlogs, complete core
SizeBench reports, receipts, and tool identity. `section-comparison.json`
records section sizes and hashes. `build-events.json` records affected
`Boxes.g.cpp` compilation and LTCG linking for both builds. Both used x64
Release (`amd64fre`), explicit `/nopgo`, `PGOBuildMode=Off`, and toolset
14.44.35207. `ownership.json`, `originals`, and
`restored-source-hashes.json` record ownership and restoration.
Both measurements verified the complete frozen manifest and managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed.

BuildOutput still holds the rejected candidate's DLL/PDB identity, despite
identical code bytes. Rebuild restored source for the next baseline.
Do not retry this destructor annotation alone. Remaining leads should
identify a repeated operation and a concrete shared implementation rather
than assume compiler annotations alone produce savings.

## Rejected: compact root COM interface-ID comparisons (2026-09-20)

Starting from `e3ec12b5f0a16ea69487f67564dddfd34b53e8d8`, replace the
three `InlineIsEqualGUID` calls in `ctl::ComBase::QueryInterfaceImpl` with
`std::memcmp` equality over `sizeof(IID)`. Add the required `<cstring>`
include in `dxaml\xcp\components\com\inc\ComBase.h`. This is a new target
for the comparison simplification, not a retry of the earlier iterator or
event-source comparisons.

The fresh SizeBench baseline retained 19,932 attributed bytes across 1,475
`ComObject<T>::QueryInterfaceImplBase` representatives. Source inspection
identified the shared root fallback and its three fixed comparisons.
The experiment preserved the order of the IUnknown, IInspectable, and
IMarshal branches, returned interface pointers, AddRef, EnsureFTM,
marshaler query, error reporting, and HRESULT paths. It did not change
interface shape, metadata, object layout, or security settings.

| Measurement | Fresh baseline | Candidate | Candidate minus baseline |
| --- | ---: | ---: | ---: |
| Actual DLL file bytes | 14,243,840 | 14,243,840 | 0 |
| Analyzed section bytes | 14,242,816 | 14,242,816 | 0 |
| Virtual section bytes | 14,253,004 | 14,252,940 | -64 |

The virtual `.text` size decreased from 9,960,044 to 9,959,980 bytes, but
its aligned raw size remained 9,960,448 bytes. Every other section's raw
and virtual sizes were unchanged. The complete `.text` hashes differ;
this was not an unchanged-code result. The `QueryInterfaceImplBase`
family's attributed size and representative count remained unchanged,
so that family total is not evidence of savings in the shared fallback.
Do not count the 64 virtual bytes as an actual DLL-file reduction.

Reject the experiment and restore `ComBase.h` byte-for-byte to its backed-up
SHA256. Only this description update is committed. Incremental accepted
savings are **0 bytes**. Cumulative accepted savings remain **291,328 bytes
(284.5 KiB)** against the original 14,535,168-byte baseline.

Tests not run: experiment rejected. No prodtest build, VM setup, or
post-restore test run was required. Earlier validation results do not apply
to this candidate. No runtime-performance benefit is claimed.

Evidence is under
`D:\x1\artifacts\ralph\20260920-012548-48478222`.
`000-baseline` and `001-root-guid-comparisons` preserve DLL/PDB snapshots,
hashes, source revision/patch, build commands/logs/binlogs, core SizeBench
reports, receipts, and frozen tool identity. `build-events.json` confirms
actual `Boxes.g.cpp` compilation, LTCG code generation, and successful
completion on both sides. Both builds used x64 Release (`amd64fre`),
explicit `/nopgo`, `PGOBuildMode=Off`, and toolset 14.44.35207.
`section-comparison.json` records section sizes and hashes;
`ownership.json`, `originals`, and `restored-source-hashes.json` record
source ownership and exact restoration. Both measurements verified the
complete frozen manifest and managed identity
`e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0`.
No SizeBench defect was observed. A successful offline compiland query
for `ComBase` returned no matching compiland; it does not establish
that this header's emitted functions occupy zero bytes.

BuildOutput still contains the rejected candidate's DLL/PDB. Rebuild the
restored source before the next baseline. Do not repeat these three root
comparisons alone: their measured reduction did not cross file alignment.
Potential follow-up work should inspect emitted code for larger repeated
operations, such as enum-reference factory completion or IID-array copying,
while avoiding the already-rejected constructor annotations and root IID
copy rewrites.

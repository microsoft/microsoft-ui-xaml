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

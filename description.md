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

# WinUI binary size reductions

These changes reduce repeated implementation code in `Microsoft.ui.xaml.dll`.
They do not change public interfaces, metadata, compiler/linker options, or
security settings. Measurements use the x64 Release build with PGO disabled.

## Commit summary

This commit combines the three measured reductions below and adds seven
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

## Regression coverage and limitations

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

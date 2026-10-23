# Activating / Deactivating the Framework-Side Profiler

This document explains the single switch that compiles all XamlProfiler
*producer-side* code (the instrumentation in `Microsoft.UI.Xaml.dll`) in or out,
and what it costs when compiled in.

> For *what* the instrumentation does, see `frameworksidechanges.md`.
> For *how the consumer* (profiler app + tap) uses it, see `profilerworking.md`.

---

## TL;DR

The profiler is gated by a single preprocessor macro, **`XAMLPROFILER_ENABLED`**,
which is driven by **one MSBuild property, `XamlProfilerEnabled`**, declared in
`dxaml\Xaml.Cpp.Props`. Every profiler `#include` and call site is wrapped in
`#ifdef XAMLPROFILER_ENABLED`.

- **On (default, every configuration):** the instrumentation is compiled into Debug
  and Release builds. With no profiler session, each instrumented site costs one
  inline provider check (load, compare, branch); no payload is built and no helper
  is called. Events are written only to a session that enables the
  `Microsoft-Windows-XAML-Profiler` provider at verbose level, and only for the
  keyword groups that session enables (see `frameworksidechanges.md` §2).
- **Off:** build with `/p:XamlProfilerEnabled=false`. `XAMLPROFILER_ENABLED` is
  never defined, the dedicated `.cpp` files are not compiled, and the compiler never
  sees any profiler code: **zero code, zero binary size, zero ETW events.**

---

## The one switch

**File:** `dxaml\Xaml.Cpp.Props`
```xml
<PropertyGroup>
  <XamlProfilerEnabled>true</XamlProfilerEnabled>
</PropertyGroup>
```

- **To turn OFF for one build:** pass `/p:XamlProfilerEnabled=false` to msbuild. A
  command-line (global) property overrides the value in the file, so nothing is
  edited.
- **To turn OFF by default:** change the value to `false`.

The same value applies to every configuration; there is no Debug-only guard.

---

## What the property drives (no manual edits needed here)

`XamlProfilerEnabled` gates three build sites via `Condition="'$(XamlProfilerEnabled)'=='true'"`.
You should not need to touch these — they follow the single switch above.

### Site 1 — macro define (`<PreprocessorDefinitions>`)
**File:** `dxaml\Xaml.Cpp.Targets`
```xml
<PreprocessorDefinitions Condition="'$(XamlProfilerEnabled)'=='true'">%(PreprocessorDefinitions);XAMLPROFILER_ENABLED=1;</PreprocessorDefinitions>
```

### Site 2 — macro define (`-D`, mirror for projects that import LibraryCompile.props)
**File:** `dxaml\msbuild\BuildSettings\LibraryCompile.props`
```xml
<AdditionalOptions Condition="'$(XamlProfilerEnabled)'=='true'">%(AdditionalOptions) -DXAMLPROFILER_ENABLED=1</AdditionalOptions>
```
Two define sites exist because `DBG=1` itself is defined in both `Xaml.Cpp.Targets`
and `LibraryCompile.props`; `XAMLPROFILER_ENABLED` mirrors that reach so every
consumer project gets the macro.

### Site 3 — the dedicated `.cpp` files
**File:** `dxaml\xcp\components\base\lib\Microsoft.UI.Xaml.Base.vcxproj`
```xml
<ClCompile Condition="'$(XamlProfilerEnabled)'=='true'" Include="..\XamlProfilerTracing.cpp"/>
```
**File:** `dxaml\xcp\components\comptree\lib\Microsoft.UI.Xaml.CompTree.vcxproj`
```xml
<ClCompile Condition="'$(XamlProfilerEnabled)'=='true'" Include="..\WucVisualTreeProfiler.cpp"/>
```
`XamlProfilerTracing.cpp` defines the provider and the out-of-line event emitters;
`WucVisualTreeProfiler.cpp` holds the WUC structure, creation and property emitters. Each has a top-of-file
`#error` safeguard that fires if it is ever compiled with `XAMLPROFILER_ENABLED`
undefined. Because all three sites read the same `XamlProfilerEnabled` property,
they stay in sync automatically.

### Cosmetic (header project-membership, no build effect)
Two `ClInclude` entries (in `Microsoft.UI.Xaml.CompTree.vcxproj` and
`Microsoft.UI.Xaml.Base.vcxproj`) are also gated on the property. Headers are never
compiled and are self-guarded (`#ifdef XAMLPROFILER_ENABLED` around their whole body),
so these only affect Solution Explorer / project membership.

---

## Cost in Release

Measured on x64 `Microsoft.ui.xaml.dll`, Release (`amd64fre`), LTCG on, PGO off,
with the switch off and on (`dumpbin /headers` virtual sizes, in bytes):

| Section | Off | On | Delta |
|---|---:|---:|---:|
| `.text` | 10,187,004 | 10,213,596 | +26,592 |
| `.rdata` | 3,125,778 | 3,128,882 | +3,104 |
| `.data` | 61,792 | 62,128 | +336 |
| `.pdata` | 348,384 | 349,104 | +720 |
| File | 14,380,032 | 14,410,240 | +30,208 (+0.21%) |

- **No session, a session below verbose level, or a session without a site's
  keyword:** the site's inline `TraceLoggingProviderEnabled` check fails and nothing
  else runs. Argument helpers and emitters are out of line (`__declspec(noinline)`).
- **Profiler attached (keywords `0x07`):** the tree events and their granular detail
  events (type info, visual creation and properties) are built and written. The
  events that fire on every tree change carry scalars only.
- **Release data:** `ElementTypeInfo.CoreSize` is 0 (unknown) and
  `XamlHeapSnapshot` is never emitted; both need the Debug allocator.

---

## Verifying the switch works

1. After `init.cmd amd64fre`, build with the switch off:
   `msbuild dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj /p:Configuration=Release /p:Platform=x64 /p:XamlProfilerEnabled=false`
   - The build succeeds and `Microsoft.ui.xaml.dll` is about 30 KB (x64) smaller
     than the default build — physical proof the code was excluded.
   - If it fails with the `#error` from `XamlProfilerTracing.cpp` or
     `WucVisualTreeProfiler.cpp`, a `ClCompile` condition is out of sync with the
     property (should not happen with the single property).
2. Attach the profiler / collect a trace → **no XamlProfiler events** appear.
3. Build again without the override → the DLL returns to full size and events flow
   to a verbose session again.

---

## Notes

- The switch has one value for every configuration. To exclude the profiler from a
  particular build or flavor, pass `/p:XamlProfilerEnabled=false` for that build
  rather than editing the individual sites.
- All three build sites and the two cosmetic header entries reference the single
  `XamlProfilerEnabled` property, so there is exactly one place to change.

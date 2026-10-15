# Activating / Deactivating the Framework-Side Profiler

This document explains the single switch that turns all XamlProfiler
*producer-side* code (the instrumentation compiled into `Microsoft.UI.Xaml.dll`)
on or off, and its consequences.

> For *what* the instrumentation does, see `frameworksidechanges.md`.
> For *how the consumer* (profiler app + tap) uses it, see `profilerworking.md`.

---

## TL;DR

The profiler is gated by a single preprocessor macro, **`XAMLPROFILER_ENABLED`**,
which is driven by **one MSBuild property, `XamlProfilerEnabled`**, declared in
`dxaml\Xaml.Cpp.Props`. Every profiler `#include` and call site is wrapped in
`#ifdef XAMLPROFILER_ENABLED`, so when the macro is undefined the compiler never
sees any of it: **zero code, zero binary size, zero ETW events.**

- **On (default, all configurations):** `XamlProfilerEnabled` is `true`, so
  `XAMLPROFILER_ENABLED` is always defined — including in retail/fre (Release)
  builds — the dedicated `.cpp` is compiled in, and every event call site is
  compiled in.
- **Off:** flip the flag to `false` (one line) and rebuild.

Runtime cost when nobody is listening is bounded by `XamlProfilerTracing::IsEnabled()`
(see "Runtime cost" below), not by the build-time switch — the switch only controls
whether the instrumentation exists in the binary at all.

---

## The one switch

**File:** `dxaml\Xaml.Cpp.Props`
```xml
<PropertyGroup>
  <XamlProfilerEnabled>true</XamlProfilerEnabled>
</PropertyGroup>
```

- **To turn OFF:** change the value to `false`, then rebuild.
- **To turn ON:** change it back to `true` (the default).

There is no longer a `Configuration`-based override — the same value applies to
Debug (chk) and Release (fre) builds alike, so an out-of-process profiler can
attach to production binaries.

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

### Site 3 — the dedicated `.cpp` compilation
**File:** `dxaml\xcp\components\comptree\lib\Microsoft.UI.Xaml.CompTree.vcxproj`
```xml
<ClCompile Condition="'$(XamlProfilerEnabled)'=='true'" Include="..\WucVisualTreeProfiler.cpp"/>
```
`WucVisualTreeProfiler.cpp` is the only dedicated profiler translation unit. It has
a top-of-file `#error` safeguard that fires if it is ever compiled with
`XAMLPROFILER_ENABLED` undefined — because all three sites share the same
`XamlProfilerEnabled` property, they now stay in sync automatically.

### Cosmetic (header project-membership, no build effect)
Two `ClInclude` entries (in `Microsoft.UI.Xaml.CompTree.vcxproj` and
`Microsoft.UI.Xaml.Base.vcxproj`) are also gated on the property. Headers are never
compiled and are self-guarded (`#ifdef XAMLPROFILER_ENABLED` around their whole body),
so these only affect Solution Explorer / project membership.

---

## Verifying the switch works

1. Set `XamlProfilerEnabled` to `false`.
2. Build: `.\initrun.ps1 build.cmd mux /q`
   - A clean build with the profiler removed should succeed, and the output
     `Microsoft.ui.xaml.dll` is measurably smaller than the profiler-on build
     (~50 KB) — physical proof the code was excluded.
   - If it fails with the `#error` from `WucVisualTreeProfiler.cpp`, the
     `XamlProfilerEnabled` guard is out of sync (should not happen with the single
     property).
3. Attach the profiler / collect a trace → **no XamlProfiler events** should appear.
4. Set `XamlProfilerEnabled` back to `true` (default), rebuild → the DLL returns to
   full size and events flow again.

---

## Runtime cost when nobody is listening

Building the instrumentation into retail/fre binaries only matters if the
per-call runtime cost is negligible when no session is subscribed. Every call
site is required to check `XamlProfilerTracing::IsEnabled()` (a cached bool kept
fresh by ETW's enable/disable callback — no per-call syscall) before doing any
work, so the common case is a single branch:

- **Pure emit, no extra work** (e.g. `ChildAdded`, `ChildRemoved`): the
  `IsEnabled()` check and the TraceLogging call itself are the entire cost.
- **Emit preceded by non-trivial processing** (e.g. serializing a WUC visual's
  full property set, walking the live tree, or taking a heap/GPU memory
  snapshot): the processing itself — not just the final TraceLogging call —
  must be inside the `IsEnabled()` guard, so that work is skipped entirely when
  no profiler is attached. All current call sites follow this pattern (see
  `WucVisualTreeProfiler.cpp`'s property-serialization helpers, and the heap
  snapshot / GPU memory snapshot call sites in `uielement.cpp` and
  `DCompSurface.cpp`).

One exception: outstanding-allocation bookkeeping (`GetOutstandingAllocationCount/
Size`, fed by a `HeapSize()` call on every allocation/free) is continuous,
symmetric accounting rather than a one-shot emit-time computation, so it is
intentionally **not** gated by a live `IsEnabled()` check — doing so could let the
counters drift if a trace session attaches or detaches mid-process. It stays tied
to Debug/`TRACE_ALLOC` builds (see `XcpAllocation.cpp`); `XamlHeapSnapshot`'s
`OutstandingBytes`/`OutstandingAllocationCount` fields report `0` in retail/fre.

---

## Notes

- The profiler now ships enabled in all configurations, including retail/fre
  (chk and fre), via the single switch in `Xaml.Cpp.Props`. To exclude it from a
  specific flavor, adjust that guard rather than the individual sites.
- All three build sites and the two cosmetic header entries reference the single
  `XamlProfilerEnabled` property, so there is exactly one place to change.

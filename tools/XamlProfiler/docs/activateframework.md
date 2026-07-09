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

- **Off (default):** `XamlProfilerEnabled` is `false`, so `XAMLPROFILER_ENABLED`
  is never defined, the dedicated `.cpp` is not compiled, and no events are emitted.
- **On:** flip the flag to `true` (one line) and rebuild.

The property is **force-disabled outside `Configuration==Debug`**, so retail/fre
builds never get the producer even if the flag is left on.

---

## The one switch

**File:** `dxaml\Xaml.Cpp.Props`
```xml
<PropertyGroup>
  <XamlProfilerEnabled>false</XamlProfilerEnabled>
  <XamlProfilerEnabled Condition="'$(Configuration)'!='Debug'">false</XamlProfilerEnabled>
</PropertyGroup>
```

- **To turn ON:** change the first line's value to `true`, then rebuild.
- **To turn OFF:** change it back to `false` (the default).

The second line is a guard: it re-forces `false` in any non-Debug configuration,
so enabling the profiler can never leak into a retail/fre build.

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

1. Set `XamlProfilerEnabled` to `false` (default).
2. Build: `.\initrun.ps1 build.cmd mux /q`
   - A clean build with the profiler removed should succeed, and the output
     `Microsoft.ui.xaml.dll` is measurably smaller than the profiler-on build
     (~50 KB) — physical proof the code was excluded.
   - If it fails with the `#error` from `WucVisualTreeProfiler.cpp`, the
     `XamlProfilerEnabled` guard is out of sync (should not happen with the single
     property).
3. Attach the profiler / collect a trace → **no XamlProfiler events** should appear.
4. Set `XamlProfilerEnabled` to `true`, rebuild → the DLL returns to full size and
   events flow again.

---

## Notes

- The profiler is scoped to `Configuration==Debug` (chk) via the guard line in
  `Xaml.Cpp.Props`. To ship the profiler in a specific fre flavor, adjust that guard
  rather than the individual sites.
- All three build sites and the two cosmetic header entries reference the single
  `XamlProfilerEnabled` property, so there is exactly one place to change.

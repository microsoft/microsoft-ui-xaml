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
  is never defined, the dedicated profiler sources are not compiled, and no profiler
  events are emitted.
- **On:** flip the flag to `true` (one line) and rebuild.

The normal project settings disable the property outside `Configuration==Debug`,
so retail/fre builds do not get the producer when the local flag is left on.

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

The second line resets the local setting to `false` in non-Debug configurations.
Do not override the property globally to `true` for a shipping build.

---

## What the property drives (no manual edits needed here)

`XamlProfilerEnabled` gates the build sites below via `Condition="'$(XamlProfilerEnabled)'=='true'"`.
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

### Dedicated `.cpp` compilation
**File:** `dxaml\xcp\components\comptree\lib\Microsoft.UI.Xaml.CompTree.vcxproj`
```xml
<ClCompile Condition="'$(XamlProfilerEnabled)'=='true'" Include="..\WucVisualTreeProfiler.cpp"/>
```
**File:** `dxaml\xcp\components\base\lib\Microsoft.UI.Xaml.Base.vcxproj`
```xml
<ClCompile Condition="'$(XamlProfilerEnabled)'=='true'" Include="..\XamlLaunchTrace.cpp"/>
```
Both production translation units have a top-of-file `#error` safeguard that fires
if compiled with `XAMLPROFILER_ENABLED` undefined. Source inclusion and the macro
definitions share the same `XamlProfilerEnabled` property.

The launch test source and header in
`dxaml\xcp\components\base\unittests\Microsoft.UI.Xaml.Tests.Isolated.Base.vcxproj`
are conditioned on that property too. The launch test class is registered only in
profiler-enabled builds; other Base tests are unaffected.

The launch producer uses the existing `Microsoft-Windows-XAML-Profiler` provider,
not the standard XAML provider. Its core member, TLS/counters, startup scope, and
callback/frame calls are all guarded, not replaced with always-present no-ops.
See [XAML launch boundary observations](../../../docs/design-notes/launch-phase-markers.md)
for the event contract.

### Cosmetic (header project-membership, no build effect)
Profiler-specific `ClInclude` entries are gated on the property as well. Headers
are not independent compilation units; their declarations are self-guarded with
`#ifdef XAMLPROFILER_ENABLED`. The project entries control project membership,
while the header guards and guarded includes provide compilation exclusion.

---

## Verifying the switch works

1. Set `XamlProfilerEnabled` to `false` (default).
2. Build: `.\Build.cmd mux /i x64chk /q`. Confirm the dedicated profiler sources
   are absent from evaluated compile items and profiler declarations/state/calls
   are absent from preprocessed code. A profiler-only source's `#error` indicates
   out-of-sync build settings.
3. Confirm profiler event metadata is absent from the resulting DLL. Binary size
   alone, or an empty ETW capture, is not proof that state and calls were removed.
4. Set `XamlProfilerEnabled` to `true` and rebuild consistently, including tests.
   Confirm the launch test class is present and passes, and that subscribed
   profiler events are delivered. Use separate outputs or rebuild when switching
   modes so stale objects or precompiled headers cannot mask a problem.

---

## Notes

- The profiler is scoped to `Configuration==Debug` (chk) via the guard line in
  `Xaml.Cpp.Props`. To ship the profiler in a specific fre flavor, adjust that guard
  rather than the individual sites.
- All build sites and profiler-specific header entries reference the single
  `XamlProfilerEnabled` property, so there is exactly one place to change.

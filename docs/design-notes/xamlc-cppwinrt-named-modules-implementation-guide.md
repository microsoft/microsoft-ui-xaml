# XamlC named-module implementation guide

The public/generated contract is documented in [xamlc-cppwinrt-named-modules.md](xamlc-cppwinrt-named-modules.md). This guide maps that contract to the compiler and MSBuild implementation.

## Core invariants

Changes to this feature should preserve all of the following:

1. Module mode is a generation-time decision (`ProjectInfo.BuildXamlModules`). There is no `XAML_USE_MODULE` source-semantic switch.
2. Each active XAML class owns one independent `<Class>.xaml.g.ixx` interface and one readable logical module identity.
3. The public `Application_Xaml.g.ixx` interface only re-exports per-class modules.
4. There is no generated `Application_Xaml.Support` module.
5. BindingInfo and TypeInfo are separate implementation-facing generated modules.
6. XamlC `.xaml.g.h` files are declaration-free sentinels in module mode and normal declaration headers in header mode.
7. C++/WinRT `<Class>.g.h` remains the producer scaffold and is absorbed by non-App per-class XamlC interfaces under `WINRT_IMPORT_MODULE`.
8. App is special: it normally has no `App.g.h` and the App XamlC interface must not require one.
9. Generated Pass2/TypeInfo/BindingInfo `.cpp` files are ordinary translation units: no `module;`, no named module declaration, and no `import std;`.
10. Ordinary generated TUs establish textual STL/system ownership before their first module import.
11. `AdditionalBMIDirectories` is only a BMI lookup mechanism; declarations still require explicit imports.
12. Module add/remove/rename lifetime follows the complete XAML project state, not only the files re-harvested in the current incremental pass.

## Source ownership

### `CppWinRT_CodeGenerator.cs`

`CppWinRTProjectionDependency` owns:

- semantic namespace -> projection header/module lowering;
- public root module naming;
- per-class module naming and identifier escaping;
- BindingInfo/TypeInfo module names;
- module-interface text scaffolding;
- ordinary C++/WinRT implementation-consumer preambles;
- root aggregator generation.

`WriteSourceInterface` is only for real `.ixx` module interfaces. It may use:

```cpp
module;
export module ...;
import std;
import winrt_base;
```

`WriteImplementationUnitPreamble` is only for ordinary generated `.cpp` consumers. It must not emit `module;` or `import std;`.

### `XamlCodeGenerator.cs`

This class decides which physical generated files exist in module mode.

#### Page/Window/App Pass1

Module mode emits:

```text
<Class>.xaml.g.h      declaration-free sentinel
<Class>.xaml.g.ixx    real XamlC declaration module
```

For non-App classes the `.ixx` also absorbs:

```cpp
#define WINRT_IMPORT_MODULE
#include "<Class>.g.h"
#undef WINRT_IMPORT_MODULE
```

and explicitly imports the projection module for the full `x:Class` namespace.

For App, `cppWinRTProducerHeader` is null because `App.xaml` normally has no matching `App.idl` producer scaffold.

#### Page/App Pass2

Module mode strips the legacy T4 header preamble and emits an ordinary `.xaml.g.cpp` TU. It imports its own per-class XamlC module and any required BindingInfo/TypeInfo modules, then includes normal authored/local producer headers under `WINRT_IMPORT_MODULE` semantics.

#### BindingInfo

Pass1:

```text
XamlBindingInfo.xaml.g.ixx
  -> <Root>.Application_Xaml.BindingInfo
```

Pass2 is an ordinary `.xaml.g.cpp` consumer importing that module.

#### TypeInfo

Pass1:

```text
XamlTypeInfo.xaml.g.ixx
  -> <Root>.Application_Xaml.TypeInfo
```

The interface absorbs the normal C++/WinRT `XamlMetaDataProvider.g.h` producer scaffold. Pass1 implementation and Pass2 TypeInfo sources are ordinary TUs importing the TypeInfo module explicitly.

Pass2 TypeInfo also imports the public XAML root and BindingInfo, then includes local authored headers after the module imports. The generated ordinary-TU preamble has already textually included the STL headers that generated/local code can require.

### `PageDefinition.cs`

This class owns the semantic dependency closure for Page/XAML generated code.

Important distinction:

- `DeclarationCppWinRTProjectionNamespaces` covers dependencies required by the exported Pass1 declaration body.
- `NeededCppWinRTProjectionNamespaces` covers dependencies needed by Pass2 generated implementation code.

The per-class module wrapper additionally imports the projection namespace that owns the non-App `x:Class` runtimeclass because that dependency belongs to the C++/WinRT producer scaffold rather than to XamlC's own declaration analysis.

Never rely on transitive visibility from another projection module.

### T4 templates

Edit `.tt`, not the generated companion `.cs`.

The T4 templates remain responsible for the language-specific declaration/implementation bodies and for legacy header mode. Module packaging and ordinary-TU ownership are applied by the handwritten generator around those bodies.

After modifying a `.tt`, use the existing repository T4 generation workflow and keep exact source/companion synchronization validation intact.

### `CompileXamlInternal.cs`

`UpdateCppWinRTXamlModules()` owns the complete module source set and logical-name manifest.

It must register:

- active per-class `.xaml.g.ixx` files;
- `XamlBindingInfo.xaml.g.ixx` when generated;
- `XamlTypeInfo.xaml.g.ixx` when generated;
- `Application_Xaml.g.ixx`.

For each `.ixx`, `GeneratedModuleNames[path]` must contain the exact logical module name. This becomes `XamlModuleName` MSBuild metadata.

The method also writes `XamlModules.list` and deletes obsolete source artifacts. Old Support filenames may appear only in deletion/cleanup lists for compatibility with earlier development revisions.

### `CompileXaml.cs`

`ExtractWrapperResults()` carries `GeneratedModuleNames` into the output task item metadata:

```text
XamlModuleName=<logical module identity>
```

If a new generated interface is not entered into `GeneratedModuleNames`, the MSBuild side cannot produce a correctly named IFC even if the `.ixx` file exists.

### `Microsoft.UI.Xaml.Markup.Compiler.interop.targets`

`XamlCppWinRTAddModuleInterfaces` registers XamlC `.g.ixx` outputs as VC module compile items.

Key metadata:

```text
CompileAs=CompileAsCppModule
ModulesSupported=true
PrecompiledHeader=NotUsing
ModuleOutputFile=$(XamlCppWinRTModuleIfcDir)<XamlModuleName>.ifc
```

It also propagates C++/WinRT BMI search directories and manages `XamlModules.list` / stale IFC lifetime.

Targets are responsible for build graph and file lifecycle only. They must not inject a macro that changes generated C++ source semantics.

## C++ ownership rule

The most important compiler-facing distinction is between module interfaces and ordinary generated implementation TUs.

### Interface TU

Allowed pattern:

```cpp
module;
#include <unknwn.h>
#include <winrt/base_macros.h>

export module MyApp.Application_Xaml.MainWindow;
import std;
import winrt_base;
export import winrt.Microsoft.UI.Xaml;
```

### Ordinary generated `.cpp`

Required pattern:

```cpp
#include <windows.h>
#include <unknwn.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define WINRT_IMPORT_MODULE
import winrt_base;
import winrt.Microsoft.UI.Xaml;
import MyApp.Application_Xaml.MainWindow;

#include "MainWindow.xaml.h"
```

Forbidden patterns in ordinary generated `.cpp`:

```cpp
module;
```

```cpp
module MyApp.Application_Xaml;
```

```cpp
import std;
#include <unordered_map>
```

The last form is the ownership conflict that produced MSVC duplicate STL definitions (`std::list`, `std::unordered_map`, `xhash`, operators, and related symbols).

## C++/WinRT producer interaction

`WINRT_IMPORT_MODULE` is not an XamlC mode switch. It is C++/WinRT's compatibility guard for ordinary producer headers in a TU that already imported the corresponding `winrt.*` modules.

The distinction is:

```text
BuildXamlModules         XamlC generation-time architecture decision
WINRT_IMPORT_MODULE      C++/WinRT producer-header compatibility mechanism
```

Do not reintroduce `XAML_USE_MODULE`, `WINRT_XAML_MODULE_INTERFACE`, or a self-import/partition macro protocol.

## App ownership

App differs from normal runtimeclass-backed XAML classes:

- no `App.idl` is normally present;
- no `App.g.h` producer scaffold is therefore assumed;
- the App `.xaml.g.ixx` exports `AppT` directly;
- the module-mode authored `App.cpp` imports `<Root>.Application_Xaml.App` before including `App.xaml.h`;
- a source-compatible `App.xaml.h` may include `App.xaml.g.h` only when `WINRT_IMPORT_MODULE` is not active.

Do not create a fake `App.g.h` merely to make App look like Page/Window.

## Naming

A class identity is derived from full `x:Class`, not the short file name.

Required properties:

- readable normal identifiers;
- no collision between same-short-name classes in different namespaces;
- deterministic Unicode handling;
- keyword/invalid-identifier escaping;
- injective escaping, including against strings that resemble the escape prefix.

Examples:

```text
OpenNet.Views.MainPage
  -> OpenNet.Application_Xaml.Views.MainPage

OpenNet.Controls.MainPage
  -> OpenNet.Application_Xaml.Controls.MainPage
```

## Incremental behavior

The following are product requirements, not optional test conveniences:

### No-change build

Unchanged `.ixx` files and IFCs must keep their timestamps and must not rebuild.

### One-class declaration change

Only the changed class interface/IFC should rebuild when the public class set is unchanged. `Application_Xaml.g.ixx` must not be rewritten if its export list did not change.

### Remove / rename

Removed logical module identities must disappear from:

- generated source;
- `Application_Xaml.g.ixx`;
- `XamlModules.list`;
- `$(XamlCppWinRTModuleIfcDir)`;
- dependency-scan sidecars/object outputs.

### Mode switching

Switching to `CppWinRTBuildModule=false` removes `.g.ixx` and XAML IFC output. Switching back recreates a complete module graph without requiring Clean.

## Validation strategy

The focused workflow intentionally tests architecture rather than only file existence.

It should assert:

- exact T4 sync;
- per-class readable logical module identity;
- App does not require `App.g.h`;
- non-App interfaces absorb `<Class>.g.h`;
- root exports only per-class modules;
- no current Support module;
- BindingInfo/TypeInfo IFCs are independently named;
- ordinary generated `.cpp` contains no `module;` and no `import std;`;
- textual STL includes precede the first module import;
- user-authored XAML `.cpp` explicitly imports its per-class XamlC module;
- real NuGet package consumer builds in module mode;
- product sample builds in Debug and Release;
- header coexistence, static libraries, add/remove/rename, and no-change builds remain valid.

When CI fails, start from the failed job and failed step, then search its focused log/artifact for `error C`, `fatal error`, `error MSB`, `WMC`, `LNK`, `Assert`, or `Failed`. Use the binlog only when normal compiler output is insufficient.

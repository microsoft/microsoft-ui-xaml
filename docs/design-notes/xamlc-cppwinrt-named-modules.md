# XamlC and C++/WinRT named modules

This document describes the current product design for C++/WinRT 3.x named-module XAML builds. It is a contributor contract for this branch, not a statement about a released Windows App SDK version.

## Goal

When a native XAML project enables:

```xml
<CppWinRTBuildModule>true</CppWinRTBuildModule>
```

the whole generated-code chain is module-first:

```text
IDL / WinMD
    -> C++/WinRT projection modules (`winrt.<Namespace>`)
    -> C++/WinRT producer scaffold (`<Class>.g.h`)

XAML class
    -> XamlC per-class declaration module
    -> XamlC ordinary Pass2 implementation TU

all XAML classes
    -> project `Application_Xaml` declaration aggregator

shared XAML implementation declarations
    -> BindingInfo module
    -> TypeInfo module
```

The important boundary is that XamlC modules replace XamlC textual declaration consumption. They do **not** replace C++/WinRT's own producer scaffold.

## Generated module graph

For `RootNamespace=MyApp` and XAML classes `MyApp.App`, `MyApp.MainWindow`, and `MyApp.Views.SettingsPage`, XamlC generates:

```text
MyApp.Application_Xaml                         public declaration aggregator
  export import MyApp.Application_Xaml.App
  export import MyApp.Application_Xaml.MainWindow
  export import MyApp.Application_Xaml.Views.SettingsPage

MyApp.Application_Xaml.App                     independent App declaration module
MyApp.Application_Xaml.MainWindow              independent class declaration module
MyApp.Application_Xaml.Views.SettingsPage      independent class declaration module

MyApp.Application_Xaml.BindingInfo             generated implementation-facing module
MyApp.Application_Xaml.TypeInfo                generated implementation-facing module
```

There is no `Application_Xaml.Support` module and no partition/self-import topology.

The root interface is only an aggregator:

```cpp
export module MyApp.Application_Xaml;
export import MyApp.Application_Xaml.App;
export import MyApp.Application_Xaml.MainWindow;
export import MyApp.Application_Xaml.Views.SettingsPage;
```

BindingInfo and TypeInfo are not part of the public root contract. Generated implementation translation units import them explicitly when needed.

## Per-class declaration interface

A normal runtimeclass XAML page/window produces `<Class>.xaml.g.ixx`. Conceptually:

```cpp
module;
#include <unknwn.h>
#include <winrt/base_macros.h>
#undef GetCurrentTime

export module MyApp.Application_Xaml.MainWindow;
import std;
import winrt_base;
export import winrt.Windows.Foundation;
export import winrt.Microsoft.UI.Xaml;
export import winrt.Microsoft.UI.Xaml.Markup;
export import winrt.MyApp;

#define WINRT_IMPORT_MODULE
#include "MainWindow.g.h"
#undef WINRT_IMPORT_MODULE

#define XAML_IMPL_MODULE
// XamlC MainWindowT declarations
#undef XAML_IMPL_MODULE
```

The interface has two different declaration owners:

1. `<Class>.g.h` is C++/WinRT's producer scaffold. It owns `<Class>_base`, factory scaffolding, and ABI producer details.
2. XamlC owns the XAML extension template such as `MainWindowT`.

The per-class module combines them under one BMI without making the XamlC `.xaml.g.h` a module interface.

### Projection namespace for the producer

A producer scaffold must see the projection module for the runtimeclass namespace that owns `x:Class`.

For:

```text
x:Class = MyApp.Views.SettingsPage
```

the generated class module imports:

```cpp
export import winrt.MyApp.Views;
```

It must not assume that a separate `winrt.MyApp` module exists. This matters for projects whose WinMD contains only nested namespaces.

### App is different

`App.xaml` normally has no `App.idl` and therefore no C++/WinRT `App.g.h` producer scaffold. Its XamlC module exports `AppT` directly from the XamlC App template and must not synthesize or require `App.g.h`.

## The `.xaml.g.h` file in module mode

C++/WinRT probes for `<Class>.xaml.g.h` while generating/including its normal component scaffold. XamlC therefore keeps the physical file in module mode, but it is only a declaration-free sentinel:

```cpp
#pragma once
// XamlC named-module sentinel.
// Real XAML declarations are owned by MyApp.Application_Xaml.MainWindow.
```

The sentinel does not include WinRT projection headers, does not declare `MainWindowT`, and does not contain `export module`.

In classic header mode, `.xaml.g.h` remains the normal textual declaration header.

## User-authored implementation model

C++/WinRT 3.x component implementations are ordinary C++ translation units with explicit imports. XamlC follows the same model.

A module-enabled authored XAML source looks like:

```cpp
#include <windows.h>
#define WINRT_IMPORT_MODULE

import winrt_base;
import winrt.Windows.Foundation;
import winrt.Microsoft.UI.Xaml;
import MyApp.Application_Xaml.MainWindow;

#include "MainWindow.xaml.h"
#include "MainWindow.g.cpp"
```

The authored header can continue to include the ordinary C++/WinRT producer header:

```cpp
#pragma once
#include "MainWindow.g.h"

namespace winrt::MyApp::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
    };
}
```

`MainWindowT` comes from the imported XamlC module. The authored code does not include `MainWindow.xaml.g.h` in module mode.

For App, a source-compatible authored header may keep a header-mode fallback:

```cpp
#pragma once
#ifndef WINRT_IMPORT_MODULE
#include "App.xaml.g.h"
#endif

namespace winrt::MyApp::implementation
{
    struct App : AppT<App>
    {
        App();
    };
}
```

The module-mode `App.cpp` imports `MyApp.Application_Xaml.App` before including this header.

### Why imports precede the authored implementation headers

The authored `.cpp` is an ordinary translation unit (one independently compiled source
file), not a named-module implementation unit: it has no `module MyApp...;` declaration.
There is no blanket C++ rule requiring every `import` to precede every `#include` in
such a file. The order follows the declarations that each header needs:

```cpp
// Native declarations and any textual STL dependencies come first.
#include <windows.h>
#include <memory>
#define WINRT_IMPORT_MODULE

// Make the base/template declarations available before defining the derived class.
import winrt_base;
import winrt.Windows.Foundation;
import winrt.Microsoft.UI.Xaml;
import MyApp.Application_Xaml.MainWindow;

#include "MainWindow.xaml.h"
#include "MainWindow.g.cpp"
```

`#include "MainWindow.xaml.h"` textually inserts `struct MainWindow :
MainWindowT<MainWindow>` at that point. Name lookup must already find `MainWindowT`,
which is exported by `MyApp.Application_Xaml.MainWindow`. A later import cannot
make that earlier base-class declaration valid retroactively. The producer `.g.cpp`
then needs the complete authored implementation class, so it follows the `.h`.

`WINRT_IMPORT_MODULE` is a preprocessor switch for the producer-header path, not a
module import. It suppresses redundant textual WinRT projection includes; it does
not provide `MainWindowT` or any other C++ declaration. A named-module import also
does not import macros, which is why this switch and native macro headers remain
explicit. See the C++ draft's [import declaration rules](https://eel.is/c++draft/module.import).

A generated `.ixx` is different: it *is* a named-module interface unit. Its native
headers belong after `module;` in the global module fragment (the region before
`export module ...;`), and its imports precede the module's declaration body.

## Ordinary generated implementation translation units

XamlC Pass2, BindingInfo implementation, and TypeInfo implementation are **ordinary translation units**, not named-module implementation units.

They therefore do not emit:

```cpp
module;
module MyApp.Application_Xaml;
```

and they do not import the standard-library module.

Instead the generated TU establishes textual native/STL ownership first:

```cpp
#include <windows.h>
#include <unknwn.h>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
// other STL headers used by generated/local code

#define WINRT_IMPORT_MODULE
import winrt_base;
import winrt.Windows.Foundation;
import winrt.Microsoft.UI.Xaml;
import MyApp.Application_Xaml.MainWindow;
```

This follows the C++/WinRT 3.x implementation pattern. It avoids the MSVC ownership conflict caused by:

```cpp
import std;
#include <list>
#include <unordered_map>
```

`WINRT_IMPORT_MODULE` allows ordinary C++/WinRT producer headers included after the imports to suppress duplicate textual WinRT projection declarations.

`import std;` remains valid in generated `.ixx` module interfaces, where the interface itself owns that module dependency and does not subsequently textually include STL headers.

### Why ordinary generated sources retain STL includes

`import std;` can supply standard C++ names such as `std::vector`, `std::map` and
`std::shared_ptr`; it does not replace native Windows headers, C++/WinRT macro
headers, standard-library macros, or arbitrary user implementation headers.
Microsoft documents these limits in [the standard-library module tutorial](https://learn.microsoft.com/en-us/cpp/cpp/tutorial-import-stl-named-module?view=msvc-170).

The C++ standard does not generally prohibit combining standard-library modules
and headers. However, the MSVC native consumer validation for this branch has
already exposed declaration/redefinition failures when `import std` is followed
by textual STL includes reached through authored/producer headers. Those headers
can change independently of XamlC, so replacing the generated implementation
preamble with `import std` would require validating their complete include closure
(every header reached transitively), not only the visible generated `.cpp`.
`WINRT_IMPORT_MODULE` suppresses WinRT projection includes; it cannot suppress
unrelated `<vector>`, `<list>`, third-party headers or their macros.

For that reason, ordinary generated implementation sources establish their
required textual STL declarations before WinRT/XamlC imports. Header sets remain
specific to the generated source kind: for example a page implementation needs
`cstdint`, `memory`, `type_traits` and `utility`, while TypeInfo also needs the
lookup containers, locks and algorithms used by its metadata implementation.
The declaration interfaces already use `import std`, and do not embed the
inactive legacy-header preamble. A future all-module implementation option needs
separate compiler/include-closure acceptance evidence; it is not a formatting change.

## Generated native file layout

Module-mode native outputs use CRLF consistently, matching this repository's
native-source `.gitattributes` policy, regardless of the host newline convention
or the newline spelling in the T4 body. The `.ixx` extension is covered by the
same policy. Normalization happens after the complete source is assembled, so
the preamble, local-header list and template body cannot retain mixed line endings.

Every generated module-mode native file has one auto-generated banner at the
top. Declaration interfaces keep the declaration body from the shared header
templates, without their duplicate banners, `#pragma once`, or inactive
`#ifndef XAML_IMPL_MODULE` include preambles. Header-mode output is unchanged.

## BindingInfo and TypeInfo

XamlC generates independent shared interfaces:

```text
XamlBindingInfo.xaml.g.ixx -> MyApp.Application_Xaml.BindingInfo
XamlTypeInfo.xaml.g.ixx    -> MyApp.Application_Xaml.TypeInfo
```

They are implementation-facing dependencies, not exports of `MyApp.Application_Xaml`.

TypeInfo implementation TUs explicitly import:

```cpp
import MyApp.Application_Xaml;
import MyApp.Application_Xaml.BindingInfo;
import MyApp.Application_Xaml.TypeInfo;
```

and import the semantic `winrt.*` projection dependencies discovered by XamlC.

Local authored implementation headers are included only after the generated TU has established textual STL ownership and imported the named WinRT/XamlC modules. `WINRT_IMPORT_MODULE` keeps their normal C++/WinRT header chain compatible with those imports.

## Semantic projection dependencies

XamlC tracks WinRT dependencies as namespaces, not as hard-coded header paths.

For example:

```text
Microsoft.UI.Xaml.Controls
```

is lowered to:

```cpp
#include <winrt/Microsoft.UI.Xaml.Controls.h>
```

in header mode, and:

```cpp
import winrt.Microsoft.UI.Xaml.Controls;
```

in module mode.

Dependency collection includes generated Page/App scaffolding, field types, connection-id target types, x:Bind path/value/parameter types, collection/map tracking types, member owner/type information, and unresolved local runtimeclass namespace fallbacks available during Pass1.

This is necessary because module builds cannot rely on the accidental transitive visibility that textual projection headers often provide.

## Module naming

The class module name preserves the full `x:Class` identity relative to the project root:

```text
MyApp.Views.SettingsPage
    -> MyApp.Application_Xaml.Views.SettingsPage

MyApp.Controls.SettingsPage
    -> MyApp.Application_Xaml.Controls.SettingsPage
```

Normal C++ identifier segments remain readable. Keywords and invalid segments are escaped deterministically and injectively. Unicode identifiers remain deterministic. Same-short-name classes in different namespaces therefore do not collide.

## MSBuild integration

XamlC writes the logical module name for every generated `.g.ixx` into `GeneratedModuleNames`. The MSBuild task exposes that as `XamlModuleName` metadata.

`XamlCppWinRTAddModuleInterfaces` then registers those files as:

```text
CompileAs=CompileAsCppModule
ModulesSupported=true
PrecompiledHeader=NotUsing
ModuleOutputFile=$(XamlCppWinRTModuleIfcDir)<XamlModuleName>.ifc
```

The generated interfaces include:

- every active per-class `.xaml.g.ixx`;
- `XamlBindingInfo.xaml.g.ixx`;
- optional `XamlTypeInfo.xaml.g.ixx`;
- `Application_Xaml.g.ixx`.

`AdditionalBMIDirectories` only makes existing IFC/BMI files discoverable. It does not make declarations visible and is never treated as a substitute for an explicit `import` statement.

### Relative intermediate paths

The generated-source dependency resolver anchors all IFC directories to
`MSBuildProjectDirectory`, normalizes them to absolute paths and removes duplicate
directory aliases before creating `/reference "Module=Path.ifc"` mappings. This
matches the absolute paths produced by the C++ dependency scanner. Without that
normalization, the default relative application `IntDir` can supply both
`x64\Debug\...ifc` and `C:\...\x64\Debug\...ifc` for one module, causing MSVC
`C7684` (ambiguous IFC resolution). Applications do not need to override `IntDir`
to work around that product bug.

The external product-sample gate builds Debug and Release with both an explicit
absolute `IntDir` and the actual evaluated default relative `IntDir`. It also
rebuilds the latter without changing sources, retains text logs/binlogs, and
rejects `C7684` and `LNK4042`. These are validation requirements; new HEADs are
not considered verified until those native builds complete.

## Incremental lifetime

`XamlModules.list` records the active logical module identities. Generated-source and IFC cleanup use the complete saved XAML project state, not only the current harvest workset.

Validation covers:

- no-change rebuild;
- one-page declaration change;
- add/remove/restore page;
- rename page;
- same-short-name classes in different namespaces;
- `NoPageCodeGen`;
- `NoTypeInfoCodeGen`;
- module -> header -> module switching;
- static-library producer/consumer graphs.

Obsolete `Application_Xaml.Support` filenames remain in cleanup lists only so builds upgraded from earlier development revisions cannot retain stale source/IFC artifacts. They are not generated or consumed by the current graph.

## Header-mode coexistence

When:

```xml
<CppWinRTBuildModule>false</CppWinRTBuildModule>
```

existing C++/WinRT XAML header generation remains active. XamlC emits normal `.xaml.g.h` / legacy Pass2 outputs and removes stale XAML module interfaces/IFCs.

The module and header modes are selected at XamlC generation time through `ProjectInfo.BuildXamlModules`. There is no `XAML_USE_MODULE` preprocessor switch controlling generated-source semantics.

## Validation layers

The PR validates the design at several levels:

1. T4 source/companion synchronization.
2. Focused generator unit tests.
3. `SimpleCppWinRTModules` real native regression build.
4. Independent static-library module producer/consumer build.
5. A real NuGet consumer built against the freshly packaged WinUI artifacts.
6. A separate product sample using C++/WinRT 3.x authored per-class imports.
7. Incremental add/remove/rename and header/module mode switching.
8. The normal six product configurations (`x86/x64/arm64`, free/chk) remain real builds and are not skipped to make module CI green.

The focused tests explicitly reject ordinary generated `.cpp` files that contain `module;` or `import std;`, reject a public Support module, and require direct per-class imports rather than relying on BMI search paths as implicit declaration visibility.

## Migration from the old `/FI` workaround

C++/WinRT 3.x documented a forced-include module preamble as a workaround for generated XAML sources that were outside its control. With XamlC itself module-aware, an application should instead:

1. enable `CppWinRTBuildModule=true`;
2. remove the XAML-only `/FI ModulePreamble.h` workaround;
3. import the generated XamlC per-class module from each authored XAML implementation TU;
4. keep `WINRT_IMPORT_MODULE` for the ordinary C++/WinRT producer-header path;
5. stop including XamlC `.xaml.g.h` from module-mode authored code;
6. import `<Root>.Application_Xaml` only when a consumer needs the whole project XAML declaration set.

The design deliberately does not introduce wrapper `.module.cpp` files or a second XamlC-specific project-reference protocol.


# XamlC and C++/WinRT named modules

C++/WinRT named-module generation is implemented on the product integration branch.
The pre-rebase phase2 gate completed native compilation, linking, packaging,
incremental transitions, static-library consumption, and all 10 focused module
unit tests in [run 36811821254](https://github.com/hoshiizumiya/microsoft-ui-xaml/actions/runs/36811821254).
The rebased tree requires a new Windows run. The full unit suite is a separate
validation effort and has not passed.

This is a contributor contract, not a feature shipped in a Windows App SDK release.
The implementation guide documents source ownership and validation limits:
[xamlc-cppwinrt-named-modules-implementation-guide.md](xamlc-cppwinrt-named-modules-implementation-guide.md).

Related issues: [named modules #11524](https://github.com/microsoft/microsoft-ui-xaml/issues/11524)
and the separate [physical companion-header mapping #11525](https://github.com/microsoft/microsoft-ui-xaml/issues/11525).

## Summary

C++/WinRT 3.x can generate C++20 named modules for WinRT projections. Before this
work, XamlC still assumed that every projected WinRT dependency was consumed as a
physical header such as:

```cpp
#include <winrt/Microsoft.UI.Xaml.Controls.h>
```

That mismatch forced module-enabled applications to own extra build glue, commonly a
forced-include preamble for XAML-generated translation units.

The new model is:

> **XamlC discovers semantic WinRT projection dependencies; the C++/WinRT backend
> decides whether those dependencies are materialized as headers or named-module
> imports.**

For a C++/WinRT project with:

```xml
<CppWinRTBuildModule>true</CppWinRTBuildModule>
```

XamlC now participates directly in the named-module build. The public XAML module for a
project is:

```cpp
import <RootNamespace>.Application_Xaml;
```

Normal application source generally does **not** need to write that import explicitly.
The existing C++/WinRT generated-component-header bridge reaches the corresponding
`*.xaml.g.h`, which imports the XAML umbrella automatically.

A repository sample is available at
[`Samples/XamlCppWinRTModules`](../../Samples/XamlCppWinRTModules/README.md).

---

## Prerequisites

Named-module XAML builds require:

- C++/WinRT 3.x with named-module support.
- A C++/WinRT-3.x-compatible MSVC module toolchain. The current focused repository
  validation uses Visual Studio 2026 / MSVC v145.
- `CppWinRTBuildModule=true`.
- A module-capable language mode; the sample uses `/std:c++latest`.
- STL module support when generated code imports `std`; the sample sets
  `BuildStlModules=true`.
- A valid `RootNamespace`. It is part of the public module identity.

The WinUI product PR build remains on its existing VS2022/toolset and SDK package
baseline. Only the focused C++/WinRT 3.x named-module validation uses VS2026/v145.
The current `windows-2025-vs2026` hosted image exposes Windows SDK 10.0.26100.0, so
that gate overrides its installed target SDK to 26100 without changing the repository-wide
SDK package baseline.

For this repository branch, the sample also sets `UseXamlCompiler=true` so it consumes
the in-repo XamlC implementation. That property is a repository-development detail, not
a customer migration requirement for a future SDK that already contains this compiler.

### PCH rule

Do not use a precompiled header that textually includes the WinRT projection headers that
the same translation unit later consumes as modules. A simple migration strategy is to
keep the PCH limited to native/platform headers and put projection imports in the source
file:

```cpp
#include <windows.h>

#define WINRT_IMPORT_MODULE
import winrt.Windows.Foundation;
import winrt.Microsoft.UI.Xaml;

#include "MainWindow.xaml.h"
```

The sample deliberately does not use a WinRT projection PCH. The focused
`SimpleCppWinRTModules` regression fixture is entirely PCH-free. It defines
`WINRT_IMPORT_MODULE` through compile-item metadata only for ordinary consumers,
after module interface registration; module producers do not receive this consumer guard.
It deliberately avoids a shared import-everything preamble so the direct umbrella smoke
test can expose missing generated dependencies.

---

## Project configuration

The minimum feature switch is:

```xml
<PropertyGroup>
  <CppWinRTBuildModule>true</CppWinRTBuildModule>
</PropertyGroup>
```

A typical project also has:

```xml
<PropertyGroup>
  <CppWinRTEnabled>true</CppWinRTEnabled>
  <CppWinRTOptimized>true</CppWinRTOptimized>
</PropertyGroup>

<ItemDefinitionGroup>
  <ClCompile>
    <LanguageStandard>stdcpplatest</LanguageStandard>
    <BuildStlModules>true</BuildStlModules>
  </ClCompile>
</ItemDefinitionGroup>
```

The sample additionally pins C++/WinRT 3.x so the required module targets are present.

---

## Generated module contract

XamlC reuses the existing Pass1 generated `*.xaml.g.h` files as C++ module interface
units. It does not create a parallel `.ixx` code generator.

For a project whose root namespace is `MyApp`, the logical graph is:

```text
MyApp.Application_Xaml                     primary module
|
+-- :MyApp.App                             App partition
+-- :MyApp.MainWindow                      XAML class partition
+-- :MyApp.Pages.SettingsPage              XAML class partition
+-- :XamlTypeInfo                          optional TypeInfo partition
```

The primary interface is generated by:

```text
XamlBindingInfo.xaml.g.h
```

App/Page/Window and TypeInfo Pass1 headers provide interface partitions.

### Why the root namespace is part of the module name

A bare `Application_Xaml` identity would collide if a consumer references more than one
XAML-producing static library. Root qualification makes the module identity project
specific:

```cpp
import ControlsLibrary.Application_Xaml;
import MyApplication.Application_Xaml;
```

---

## End-to-end generation example

For a sample class:

```text
x:Class = XamlCppWinRTModulesSample.MainWindow
RootNamespace = XamlCppWinRTModulesSample
```

the build is conceptually:

```text
MainWindow.idl
    |
    +-- C++/WinRT metadata/component generation
    |      |
    |      +-- MainWindow.g.h
    |      +-- MainWindow.g.cpp
    |      +-- projection module/interface data
    |
MainWindow.xaml
    |
    +-- XamlC Pass1
    |      |
    |      +-- MainWindow.xaml.g.h
    |      |      module partition:
    |      |      XamlCppWinRTModulesSample.Application_Xaml
    |      |          :XamlCppWinRTModulesSample.MainWindow
    |      |
    |      +-- XamlBindingInfo.xaml.g.h
    |             primary module:
    |             XamlCppWinRTModulesSample.Application_Xaml
    |             export-imports MainWindow/App/TypeInfo partitions
    |
    +-- XamlCppWinRTAddModuleInterfaces
    |      |
    |      +-- registers the Pass1 *.xaml.g.h files as CompileAsCppModule
    |
    +-- MSVC module dependency scan / compile
    |      |
    |      +-- $(IntDir)XamlModules\*.ifc
    |
    +-- XamlC Pass2
           |
           +-- MainWindow.xaml.g.hpp and late generated C++ implementation
           +-- XamlMetaDataProvider.cpp / TypeInfo implementation as needed
                  |
                  +-- compile using the already produced projection/XAML BMIs
```

Authored source then consumes the result through the existing component-header path:

```text
MainWindow.xaml.cpp
    |
    +-- #define WINRT_IMPORT_MODULE
    +-- include MainWindow.xaml.h
           |
           +-- include MainWindow.g.h        (C++/WinRT)
                  |
                  +-- __has_include("MainWindow.xaml.g.h")
                         |
                         +-- include MainWindow.xaml.g.h   (XamlC)
                                |
                                +-- import
                                    XamlCppWinRTModulesSample.Application_Xaml
```

There is one source of type metadata (IDL/WinMD), one XAML compiler pipeline, and one
public XAML umbrella. The module path does not introduce a second set of handwritten
XAML declarations or a parallel `.ixx` generator.

## The generated-header bridge

C++/WinRT component headers already probe for XAML companion output:

```cpp
#if __has_include("MainWindow.xaml.g.h")
#include "MainWindow.xaml.g.h"
#endif
```

XamlC uses this existing bridge instead of requiring every authored source file to import
the project XAML module manually.

Conceptually, a generated XAML companion behaves like this:

```cpp
#ifdef WINRT_XAML_MODULE_INTERFACE

export module MyApp.Application_Xaml:MyApp.MainWindow;

// projection dependencies needed by this XAML class
export import winrt.Microsoft.UI.Xaml;
export import winrt.Some.Other.Namespace;

// exported XAML declarations...

#else

#define WINRT_IMPORT_MODULE
import MyApp.Application_Xaml;

#endif
```

The exact generated body varies by App/Page/BindingInfo/TypeInfo template, but the
important rule is stable:

- when compiled as a module interface, the file defines the partition;
- when included through a normal C++/WinRT generated component header, it imports the
  project umbrella instead of textually redefining the XAML declarations.

This is why normal authored source does not need:

```cpp
import MyApp.Application_Xaml;
```

A direct import remains valid and is useful for libraries, diagnostics, and smoke tests.

---

## Semantic projection dependencies

The compiler no longer stores a WinRT dependency primarily as a header path.

Old representation:

```text
winrt/Microsoft.UI.Xaml.Controls.h
```

New representation:

```text
Microsoft.UI.Xaml.Controls
```

The C++/WinRT backend lowers that semantic namespace according to the active mode.

Header mode:

```cpp
#include <winrt/Microsoft.UI.Xaml.Controls.h>
```

Module mode:

```cpp
import winrt.Microsoft.UI.Xaml.Controls;
```

This separation is important because XamlC has three different dependency categories:

1. **WinRT projection dependency**  
   A semantic namespace. It may become a header include or a named-module import.

2. **Local implementation dependency**  
   Files such as `Foo.h`, `Foo.g.h`, or `Foo.xaml.g.hpp`. These remain textual
   includes where appropriate.

3. **Native/STL/COM dependency**  
   Examples include `<cstdint>`, `<unknwn.h>`, or `std`. These follow their own
   native/module rules and are not treated as WinRT projection namespaces.

For a local field such as `<local:PropBag x:Name="propertyBagObject" />`, Pass1
can lack a reflection `Type` while already knowing `FieldTypePath = "Simple"` and
`FieldTypeShortName = "PropBag"`. `CppWinRTProjectionDependency.GetNamespaces(Type,
string)` uses that namespace only when `Type` is null; a resolved type still follows
the recursive projection closure. The generated `winrt::Simple::PropBag` field
therefore requires `winrt.Simple` even before the intermediate component WinMD exists.

---

## x:Bind dependency closure

Module builds expose transitive-header assumptions that header mode can accidentally hide.
The Page generator therefore tracks every projected type that generated x:Bind C++ can
actually spell.

The closure includes:

- the binding data-root type;
- every bind-path step value type;
- observable-vector/map item types when tracking code uses them;
- function owner, declared parameter, parameter value, and assignment types;
- the concrete connection-id target element type, including elements without `x:Name`;
- the target member type and declaring type;
- dependency-property owner types.

Pass1 can only export projection dependencies that are resolvable before the intermediate
component WinMD exists. Local runtimeclasses in a separate project namespace may therefore
still be unresolved during Pass1. After the intermediate WinMD is available, Pass2
recomputes the same semantic closure and materializes the fully resolved dependencies as
direct `import winrt....;` declarations in module mode (or projection-header includes in
header mode).

The regression project intentionally uses independent namespaces on both sides of a
binding graph and checks the generated `MainPage.xaml.g.hpp` for both imports. This
prevents a same-namespace or transitive-header accident from hiding a missing Pass2
dependency.

---

## App and metadata-provider split

The App Pass1 module interface cannot require the concrete generated
`XamlMetaDataProvider` implementation to be complete while the interface itself is being
built.

Module mode therefore keeps provider-dependent App members as declarations in Pass1.
Pass2, after including the complete metadata-provider definition, supplies definitions for
operations such as:

- App default construction and destruction;
- `GetXamlType`;
- `GetXmlnsDefinitions`;
- construction/access of the generated metadata provider.

This keeps implementation-only provider dependencies out of the App partition interface.

Both special members must be declared in Pass1 and defaulted in Pass2. Moving only
the destructor is insufficient: constructing `AppT<App>` in authored `App.cpp` can
instantiate exception cleanup of its `com_ptr<XamlMetaDataProvider>` member. Pass2
includes the complete provider and explicitly instantiates the App specialization.

---

## Page implementation bases

A XAML Page partition must not pull C++/WinRT component implementation `.g.h` files into
named-module purview. Instead, Page Pass1 forward-declares the cppwinrt
`<Type>_base` templates needed by generated declarations.

That preserves the existing implementation-header model while keeping the module
interface free from component implementation ownership and redefinition problems.

---

## TypeInfo and legacy COM

The TypeInfo partition exports XamlC metadata-provider implementation types and may use
legacy COM concepts such as `IUnknown`.

Native COM declarations are provided from the global module fragment where required.
The interop targets also enable `WINRT_ENABLE_LEGACY_COM` while compiling
C++/WinRT's base module: TypeInfo uses `IXamlUserType : ::IUnknown` in
`winrt::implements`, and including `<unknwn.h>` in a later XAML partition alone
cannot enable that support retroactively. WinRT projection dependencies are tracked
semantically and emitted as explicit module imports.

The current regression suite also imports the final XAML umbrella from an independent
consumer translation unit and requires the exported BindingInfo, TypeInfo, and
legacy-COM-backed types to be complete. This tests importer-side reachability rather than
only checking that the module interface can compile itself.

---

## Build pipeline

The important Pass1/module ordering is:

```text
ResolveProjectReferences
        |
CppWinRTResolveModuleReferences
        |
MarkupCompilePass1
        |
XamlCppWinRTAddModuleInterfaces
        |
CppWinRTAddModuleInterfaces
        |
XamlCppWinRTNormalizeModuleCompileItems
        |
FixupCLCompileOptions / MSVC dependency scanning
        |
C++ compilation
```

`XamlCppWinRTAddModuleInterfaces` takes the Pass1 `*.xaml.g.h` outputs and registers
them as `ClCompile` module-interface items with:

```text
CompileAs=CompileAsCppModule
ModulesSupported=true
PrecompiledHeader=NotUsing
WINRT_XAML_MODULE_INTERFACE
ModuleOutputFile=$(IntDir)XamlModules\
```

C++/WinRT-resolved projection BMI search directories are propagated to these items.

### Pass2 / late-generated C++ items

Pass2 adds generated C++ translation units after the project's primary module-reference
resolution has already run. XamlC therefore applies the required BMI search directories
to late generated items such as:

- Page Pass2 output;
- `XamlMetaDataProvider.cpp`;
- other `CompileXamlGeneratedFiles` items.

The search path includes the local projection `$(IntDir)`, the dedicated XAML module
directory, and the projection BMI directories already resolved on normal compile items.

---

## IFC/BMI output

MSVC's compiled module interface format is IFC. “BMI” is the generic term for the binary
module interface; IFC is the MSVC representation.

XAML module IFCs are kept in:

```text
$(IntDir)XamlModules\
```

rather than mixing them into the directory that C++/WinRT uses for projection IFCs.

This makes XAML module lifetime explicit and lets XamlC clean module output when the
project leaves module mode.

The directory can be overridden for diagnostics with:

```xml
<XamlCppWinRTModuleIfcDir>...</XamlCppWinRTModuleIfcDir>
```

---

## Static libraries and cross-project consumption

Do not invent a second XamlC-specific ProjectReference protocol.

For a C++ static library, VC/MSBuild already propagates named-module BMIs through the
normal ProjectReference graph. Static-library BMIs are public by default through
`AllProjectBMIsArePublic` / `ReferencedModuleBMIs`.

A consumer can therefore import:

```cpp
import ControlsLibrary.Application_Xaml;
```

through the normal static-library ProjectReference.

A pure consumer with no local IDL or XAML uses `CppWinRTBuildModule=false`:
it imports the provider's modules through the native graph without generating its
own projection interfaces. The flag controls projection production, not whether
ordinary C++ source may use `import`. Namespace exclusion alone does not prevent
the package from generating `winrt_base` and `winrt_numerics`.

C++/WinRT projection-module policy remains independent. If the provider's projection BMI
is already propagated from the static library, prevent the consumer from generating a
duplicate module for the same namespace:

```xml
<CppWinRTModuleExclude>ControlsLibrary</CppWinRTModuleExclude>
```

Do **not** use `CppWinRTConsumeModule` to mean “consume this component's XAML module”.
That C++/WinRT metadata is the platform/reference-module-builder contract.

---

## Incremental behavior

Named modules make stale generated state much more visible, so XamlC now treats changes
to the project XAML item set and module mode as first-class invalidation inputs.

### Feature-mode changes

`CppWinRTBuildModule=true` is propagated into XamlC as the
`CppWinRTNamedModules` feature-control flag. Feature-control flags are part of saved
state, so switching:

```text
module -> header -> module
```

invalidates generated output instead of silently reusing the previous mode's code.

### Removed XAML items

Removing a Page/App item:

- invalidates the no-change shortcut;
- removes its saved-state entry;
- forces shared BindingInfo/TypeInfo/module state to regenerate;
- deletes orphaned Pass1/Pass2 generated class files and backups.

The physical deletion matters because C++/WinRT uses `__has_include("<Type>.xaml.g.h")`.
A stale header would otherwise create a “ghost” XAML bridge for a class that is no longer
in the project.

### No XAML or header mode

`$(IntDir)XamlModules\` is removed when either:

- `CppWinRTBuildModule` is no longer true; or
- the project no longer has a Page or ApplicationDefinition.

This prevents stale `Application_Xaml` IFCs from advertising a module the current
project no longer produces.

### NoPageCodeGen

`NoPageCodeGen` suppresses non-Application Page codegen but keeps the App partition.
Suppressed Page `*.xaml.g.h/.g.hpp` files are deleted in normal builds so they cannot be
found through the C++/WinRT generated-header bridge.

### NoTypeInfoCodeGen

`NoTypeInfoCodeGen` removes the optional `:XamlTypeInfo` partition and the primary
interface no longer exports it after the mode transition.
The regression gate checks this contract with `MarkupCompilePass1`, then restores
TypeInfo generation and performs a full provider build before the cross-project
consumer test. The flag suppresses generated TypeInfo implementation; it does not
supply a replacement for the metadata-provider runtimeclass that C++/WinRT still
projects for this fixture.

---

## Migration from the old module workaround

A project that previously followed an application-owned XAML module workaround can
usually simplify substantially.

### Remove

Remove project-owned logic that exists only to make generated XAML C++ see modules:

- forced-include (`/FI`) XAML module preambles;
- custom lists of `XamlTypeInfo.g.cpp`, `XamlMetaDataProvider.cpp`, or generated Page
  files whose only purpose is to inject imports;
- hand-maintained projection-import lists derived from XAML types;
- manual project-level `Application_Xaml` imports in every normal implementation TU.

### Keep or add

Keep/add the real C++/WinRT module configuration:

```xml
<CppWinRTBuildModule>true</CppWinRTBuildModule>
```

Use a compatible language/STL module configuration, for example:

```xml
<LanguageStandard>stdcpplatest</LanguageStandard>
<BuildStlModules>true</BuildStlModules>
```

Use `WINRT_IMPORT_MODULE` in authored C++ where C++/WinRT's generated textual component
headers need to coexist with imported projections:

```cpp
#define WINRT_IMPORT_MODULE
import winrt.Microsoft.UI.Xaml;

#include "MainWindow.xaml.h"
```

The macro is **not** a replacement for importing the needed projection modules. It tells
legacy C++/WinRT projection headers to take the module-compatible path when they are
encountered later in the include graph.

### PCH migration

If the old PCH contains many `<winrt/...h>` projection headers, move those dependencies
to imports in the translation units that use them, or otherwise ensure the PCH does not
textually predeclare the same projections that are later imported as modules.

---

## What application authors should import

### Normal XAML implementation source

Import the projection namespaces used by the authored implementation and its headers:

```cpp
#include <windows.h>

#define WINRT_IMPORT_MODULE
import winrt.Windows.Foundation;
import winrt.Microsoft.UI.Xaml;

#include "MainWindow.xaml.h"
```

For example, the fixture's `MainPage.h` declares a `Simple.Models.BindModel` member.
Sources that include it import `winrt.Simple.Models`; authored uses of XAML Input
types likewise require `winrt.Microsoft.UI.Xaml.Input`. XamlC does not inspect an
authored header to infer these dependencies. XamlC collects dependencies of the code
it generates: partitions expose declaration dependencies, while Pass2 imports the
dependencies needed by generated implementation.

Do not add `import MyApp.Application_Xaml;` merely because the file implements a XAML
class. The generated-header bridge already provides that.

### Explicit umbrella consumer

Use the public module directly when a source file is intentionally testing/consuming the
whole XAML surface without going through a component header:

```cpp
#define WINRT_IMPORT_MODULE
import MyApp.Application_Xaml;

static_assert(sizeof(winrt::MyApp::implementation::XamlBindings) > 0);
```

---

## Generated files: what owns what

| Generated file | Role in module mode |
| --- | --- |
| `XamlBindingInfo.xaml.g.h` | Primary `<Root>.Application_Xaml` interface |
| `App.xaml.g.h` | App interface partition |
| `<Page>.xaml.g.h` | XAML class interface partition / normal include shim |
| `XamlTypeInfo.xaml.g.h` | Optional `:XamlTypeInfo` partition |
| `*.xaml.g.hpp` | Pass2 implementation output, not a public module interface |
| `XamlMetaDataProvider.h/.cpp` | Generated metadata-provider implementation surface |
| `$(IntDir)XamlModules\*.ifc` | MSVC compiled XAML module interfaces |

The checked-in C# files generated from C++/WinRT T4 templates are implementation
artifacts, not the source of truth.

---

## XamlC contributor workflow

The C++/WinRT T4 templates under:

```text
src/XamlCompiler/BuildTasks/Microsoft/Xaml/XamlCompiler/CodeGenerators/CppWinRT/
```

are the source of truth.

Do not manually edit the checked-in generated `.cs` files.

The PR workflow:

1. runs T4 regeneration on the VS18 toolchain;
2. verifies that only valid generated `.cs` files changed;
3. commits synchronized generated output when necessary;
4. dispatches validation again;
5. performs product validation on the VS2022 / MSBuild 17.x toolchain;
6. runs the focused named-module path separately with VS2026 / MSVC v145.

The focused module validation currently targets the VS2026 hosted image's installed
Windows SDK 10.0.26100.0. That override is intentionally local to module validation.

A repository-local workflow-dispatch file also exists for a shorter module-only inner
loop. GitHub's Actions UI only exposes `workflow_dispatch` for workflows present on the
default branch, so a workflow introduced only on this feature branch cannot yet be used
as a normal manual Actions entry. This is a CI iteration limitation, not part of the
XamlC design.

This split exists because the T4 tooling, product compiler baseline, and C++/WinRT 3.x
named-module validation have different toolchain requirements.

---

## Validation coverage

The focused regression suite is implemented with the coverage below. The final VS2026/v145/SDK26100 end-to-end gate is still the remaining acceptance step.

The focused x64 Debug module gate covers:

- module clean build;
- no-change rebuild;
- one-Page change;
- Page remove and restore without cleaning;
- stale `*.xaml.g.h` deletion;
- `NoPageCodeGen` partition/output behavior;
- restoring normal Page codegen;
- module -> header -> module switching;
- physical XAML IFC cleanup/regeneration;
- source-side and target-side x:Bind projection closure;
- recursive generic projection-namespace closure;
- `NoTypeInfoCodeGen` partition removal;
- static-library provider and cross-project module consumer;
- no-XAML stale-module cleanup;
- importer-side completeness of BindingInfo/TypeInfo/legacy-COM declarations.

---

## Validation and integration boundaries

[Run 36811821254](https://github.com/hoshiizumiya/microsoft-ui-xaml/actions/runs/36811821254)
validated the original phase2 tree: all six product configurations and the complete
focused named-module job passed. The job covers clean/no-change/one-page rebuilds,
Page removal/restoration, NoPage/NoTypeInfo Pass1 generation, module/header/module
switching, stale IFC removal, and native static-library consumer propagation.
The NoPage and NoTypeInfo checks intentionally exercise Pass1; they do not establish
that an application which depends on suppressed code can link with those flags.

Product integration is rebased on upstream `4f7cd8dc`, including test modernization
[#11837](https://github.com/microsoft/microsoft-ui-xaml/pull/11837).
That rebase moves proxy sources under `Tests/UnitTests/XamlCompilerProxies` and retains
upstream test dependency staging and SDK discovery. It needs fresh Windows validation;
the earlier successful run cannot establish the rebased tree's correctness.

Native module fixtures and fork workflows are maintained on a separate validation
branch, so the normal test solution and product toolchain remain aligned with upstream.
Those fixtures currently use `YexuanXiao.CppWinRTPlus 3.1.260928.1` to work around the
Microsoft package's PCH-free metadata-provider wrapper defect. Their projection-mode
cache target is a fixture workaround for a separate C++/WinRT incremental defect.
Neither workaround changes the product-wide C++/WinRT package pin.

The legacy unfiltered unit run failed with missing Microsoft.Build runtime dependencies
and several assertions. Upstream #11837 restores the runtime closure and fixes many
of those assertions; its own reported validation still has four known succinct-collection
failures. The migrated unfiltered runner must report its actual results without filters
or additional ignored tests. Generated-code comparison masters are not updated merely
to hide a difference: any changed output needs review and regeneration by the owning tools.

---

## Known boundary: physical XAML companion paths

This module work does **not** solve
[microsoft-ui-xaml#11525](https://github.com/microsoft/microsoft-ui-xaml/issues/11525).

C++/WinRT may calculate a logical companion include such as:

```text
<Type>.xaml.g.h
```

while a project can place XAML at a physical path that cannot be reconstructed from
WinRT metadata alone.

That is a separate mapping problem: XamlC already knows the physical XAML item path and
needs a contract that lets the C++/WinRT companion-header lookup consume that mapping.
Do not hide that problem by changing the module naming contract.

---

## PR decomposition

The [issue/commit/dependency index](xamlc-cppwinrt-named-modules-pr-decomposition.md)
separates common compiler fixes, module implementation, native build integration,
regression work and independently owned build defects. It records evidence and
remaining acceptance boundaries for extracting reviewable PRs from the development
branch.

---

## Sample

See [`Samples/XamlCppWinRTModules/README.md`](../../Samples/XamlCppWinRTModules/README.md).

The sample demonstrates:

- the minimum project properties for C++/WinRT 3.x module mode;
- a module-friendly no-projection-PCH source layout;
- normal generated-header XAML module consumption;
- an x:Bind model from a different WinRT namespace;
- a direct `<RootNamespace>.Application_Xaml` smoke import;
- where the generated `*.xaml.g.h` and `XamlModules\*.ifc` outputs come from.

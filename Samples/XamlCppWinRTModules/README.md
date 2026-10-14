# XamlC + C++/WinRT named modules sample

This repo-local sample validates the C++/WinRT 3.x module-first XamlC design implemented by this branch. It is a contributor validation sample; the feature is not yet part of a released Windows App SDK.

The sample uses `YexuanXiao.CppWinRTPlus 3.1.260928.1` and `CppWinRTBuildModule=true`.

## The contract being validated

C++/WinRT still owns WinRT projection modules and its producer scaffolding. XamlC adds a named-module declaration layer for each XAML class:

```text
IDL
  -> C++/WinRT winrt.<Namespace> modules
  -> C++/WinRT <Class>.g.h producer scaffold

XAML
  -> XamlC <Class>.xaml.g.ixx
       export module <Root>.Application_Xaml.<Class>
       imports winrt.* modules
       absorbs <Class>.g.h when the class has a C++/WinRT producer scaffold
       exports the XamlC <Class>T declaration

all XAML classes
  -> Application_Xaml.g.ixx
       export module <Root>.Application_Xaml
       export import <Root>.Application_Xaml.<Class> ...

XamlC shared declarations
  -> XamlBindingInfo.xaml.g.ixx
  -> XamlTypeInfo.xaml.g.ixx

XamlC Pass 2
  -> ordinary generated .cpp translation units
       textual STL/system headers first
       WINRT_IMPORT_MODULE
       import winrt_base / winrt.* / XamlC modules
       no `module;`
       no `import std;`
```

There is deliberately no public `Application_Xaml.Support` module.

## User-authored XAML implementation

The important user-facing pattern is the same implementation style used by C++/WinRT 3.x: an ordinary `.cpp` translation unit imports the generated modules explicitly and then includes its ordinary producer header.

`MainWindow.xaml.cpp` is the concrete example:

```cpp
#include <windows.h>

#define WINRT_IMPORT_MODULE
import winrt.Windows.Foundation;
import winrt.Microsoft.UI.Xaml;
import XamlCppWinRTModulesSample.Application_Xaml.MainWindow;

#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
```

`MainWindow.xaml.h` contains the authored class and includes the normal C++/WinRT producer scaffold:

```cpp
#pragma once
#include "MainWindow.g.h"

namespace winrt::XamlCppWinRTModulesSample::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
    };
}
```

The authored code does **not** include `MainWindow.xaml.g.h` in module mode. `MainWindowT` arrives through the imported XamlC per-class module.

`App` is a special case: it normally has no `App.idl` and therefore no `App.g.h` producer scaffold. Its generated XamlC module directly exports `AppT`. The authored `App.xaml.h` only includes `App.xaml.g.h` when `WINRT_IMPORT_MODULE` is not active so legacy header builds can still use the same source file.

## Public project XAML module

Consumers that need all generated XAML class declarations can import one root aggregator:

```cpp
import XamlCppWinRTModulesSample.Application_Xaml;
```

The root contains only `export import` declarations for the independent per-class modules. `XamlModuleSmoke.cpp` validates that importing the root makes `MainWindowT` usable without any XamlC header or macro.

`XamlBindingInfo` and `XamlTypeInfo` are implementation-facing generated modules. They are imported explicitly by generated implementation translation units and are not re-exported from the public root.

## Why ordinary generated `.cpp` files do not import `std`

C++/WinRT 3.x implementation sources are ordinary translation units, not named-module implementation units. XamlC follows that model.

A generated implementation source establishes all STL/system textual ownership before its first module import:

```cpp
#include <windows.h>
#include <unknwn.h>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
// ...

#define WINRT_IMPORT_MODULE
import winrt_base;
import winrt.Windows.Foundation;
import XamlCppWinRTModulesSample.Application_Xaml.MainWindow;
```

This avoids the invalid ownership pattern that caused MSVC STL redefinition failures:

```cpp
import std;
#include <list>
#include <unordered_map>
```

`import std;` remains appropriate inside the generated `.ixx` module interfaces, where the standard library is owned by the module interface rather than mixed with later textual STL includes.

## x:Bind dependency closure

`MainWindow.xaml` binds to `XamlCppWinRTModulesSample.Models.GreetingModel`. XamlC records projection dependencies while analyzing the declaration and binding graph. The generated per-class interface and Pass 2 implementation therefore import the WinRT projection modules they actually need; authored code does not maintain a parallel hand-written list for XAML-discovered types.

## Generated artifacts

For the sample, the important module-mode outputs are:

```text
Application_Xaml.g.ixx
App.xaml.g.ixx
MainWindow.xaml.g.ixx
XamlBindingInfo.xaml.g.ixx
XamlTypeInfo.xaml.g.ixx

App.xaml.g.h                 # declaration-free sentinel in module mode
MainWindow.xaml.g.h          # declaration-free sentinel in module mode

*.xaml.g.cpp                 # ordinary Pass 2 consumers
XamlTypeInfo.g.cpp
XamlTypeInfo.Impl.g.cpp

$(IntDir)XamlModules\*.ifc
```

Expected module identities include:

```text
XamlCppWinRTModulesSample.Application_Xaml
XamlCppWinRTModulesSample.Application_Xaml.App
XamlCppWinRTModulesSample.Application_Xaml.MainWindow
XamlCppWinRTModulesSample.Application_Xaml.BindingInfo
XamlCppWinRTModulesSample.Application_Xaml.TypeInfo
```

There must be no `Application_Xaml.Support.g.ixx` or `Application_Xaml.Support.ifc`.

## Build

Use a Visual Studio 2026 developer prompt for the current C++/WinRT 3.x development branch. Repository focused validation currently uses MSVC v145 and Windows SDK 10.0.26100.0:

```bat
init.cmd x64chk /nopgo
nuget install Samples\XamlCppWinRTModules\packages.config -OutputDirectory packages -NonInteractive

msbuild Samples\XamlCppWinRTModules\XamlCppWinRTModules.vcxproj ^
  /p:Configuration=Debug ^
  /p:Platform=x64 ^
  /p:VisualStudioVersion=18.0 ^
  /p:PlatformToolset=v145 ^
  /p:WindowsSdkTargetPlatformVersion=10.0.26100.0 ^
  /p:TargetPlatformVersion=10.0.26100.0 ^
  /p:WindowsTargetPlatformVersion=10.0.26100.0
```

`UseXamlCompiler=true` makes this repository sample consume the compiler built from the checkout. That property is a repository-development detail.

## Relevant project settings

```xml
<PropertyGroup>
  <CppWinRTBuildModule>true</CppWinRTBuildModule>
  <CppWinRTPlusVersion>3.1.260928.1</CppWinRTPlusVersion>
  <CppWinRTEnabled>true</CppWinRTEnabled>
  <CppWinRTOptimized>true</CppWinRTOptimized>
</PropertyGroup>

<ItemDefinitionGroup>
  <ClCompile>
    <PrecompiledHeader>NotUsing</PrecompiledHeader>
    <LanguageStandard>stdcpplatest</LanguageStandard>
    <BuildStlModules>true</BuildStlModules>
  </ClCompile>
</ItemDefinitionGroup>
```

## Header-mode coexistence

Setting:

```xml
<CppWinRTBuildModule>false</CppWinRTBuildModule>
```

returns XamlC to the classic header output. The module build instead turns the `.xaml.g.h` files into declaration-free probe/sentinel files and moves the XamlC declarations into `.xaml.g.ixx` interfaces. Mode switching also removes stale XAML IFCs.

## Migration from the old `/FI` workaround

For an existing C++/WinRT 3.x XAML app:

1. Enable `CppWinRTBuildModule=true`.
2. Remove the application-owned `/FI ModulePreamble.h` workaround used only to make old XamlC-generated files see projection modules.
3. Import the relevant generated per-class XamlC module in each authored XAML implementation `.cpp` before including its authored header.
4. Keep `WINRT_IMPORT_MODULE` enabled for the ordinary producer-header path.
5. Do not include XamlC `.xaml.g.h` from module-mode authored code.
6. Import `<Root>.Application_Xaml` only when a consumer needs the complete project XAML declaration set.

For implementation details and incremental-build invariants, see the design notes under `docs/design-notes/`.

# Dxaml vs Core layers / Peer objects

## Table of Contents

- [Background](#background)
- [Xaml Core](#xaml-core)
- [Peer objects](#peer-objects)
  - [Dxaml layer to Core layer communication](#dxaml-layer-to-core-layer-communication)
  - [Core layer to Dxaml layer communication](#core-layer-to-dxaml-layer-communication)
    - [GetPeer in DxamlCore](#getpeer-in-dxamlcore)
    - [GetPeer using DxamlServices](#getpeer-using-dxamlservices)
    - [FxCallbacks](#fxcallbacks)

The WinUI codebase is largely split into Dxaml layer and Core layer parts. The Dxaml layer is public facing and 
includes a lot of types implementation. The Core layer is more isolated and deals with internal implementation like 
computations and talking with OS and hardware.

## Background

Much of the current Xaml code comes from Silverlight. Silverlight was a browser plugin which could be hosted on a web 
page in IE, FireFox, or Chrome, on Windows or Mac (Safari).

Initially all of Silverlight was C++ in what we now call the "core" layer, still today in a directory named "core". 
(The Silverlight DLL was named `agcore.dll`.) Later a layer was added on top of that, written in C#, called 
`System.Windows` or sometimes **"framework"**, the idea being that it was a framework on top of agcore, but there could 
be other frameworks in the future.

The Core layer implemented fundamentals such as DependencyObject, DependencyProperty, layout, input, animations, and 
rendering. Originally, the only types supported were `Canvas, TextBlock, Shapes, Animations`, and `Storyboard`. 
`Control` and `Templating` was added to that, but the actual controls such as Button and ListBox were implemented in 
`System.Xaml`. 

As a result, the control framework was in Core, the controls were in `System.Windows`. Over time some of this code 
moved from `System.Windows` to core for perf reasons, for example Button is now CButton in core.

The original core layer was exposed as JS objects to the browser. The browser interop was implemented in 
`CDependencyObject` (`CDO`). Consequently, everything in the core is a CDO. The `System.Windows` types were only 
exposed to .NET. So, for example, you could see `ItemsControl` from JS, but only see `ListBox` from .NET.

To enable access to these types from .NET, there were C# wrappers in `System.Windows` for every Core type. For example 
there was an Control class in `System.Windows` that was a trivial wrapper around the Core CControl class.

With Windows 8, the Core layer was brought to Windows, and the `System.Windows` layer was re-written in C++.
> That re-written code was all put into what is now the DXaml directory. 

The new C++ DXaml code followed more modern standard at that time, which is why the classes there aren't "C"-prefixed 
like they are in the Core. There are a lot of other examples where you can see the age difference between the code in 
the two layers.

## Xaml Core

The *Xaml core* - not to be confused with the Core *layer* in the Xaml code - is made up of 2 objects, each 
corresponding to one of the layers: a [`DxamlCore`](../../dxaml/xcp/dxaml/lib/DxamlCore.h) object at the Dxaml layer 
and a [`CCoreServices`](../../dxaml/xcp/core/inc/corep.h) object at the Core layer. Together they make up a **Xaml 
instance**, which is one per-thread because WinUI is a single-threaded framework.

## Peer objects

Each control type is composed of 2 objects: a Dxaml layer object and a Core layer object. For example, the *UIElement* 
type is made up of a `UIElement` in the Dxaml layer (which is exposed to public) and `CUIElement` in the Core layer 
(internal to Xaml). Internally, all Xaml types are children of the 
[DependencyObject (DO)](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.dependencyobject?view=winui-3.0) 
class at the Dxaml layer and `CDependencyObject` (CDO) at the Core layer. (More information on this can be found in 
[Journey of a control](./control-overview.md).) These two objects are called *peers* of each other. A Dxaml object’s 
peer is its Core object and vice versa. The two objects cannot talk directly. The flow of instruction is always Dxaml 
layer &rarr; Core layer. A Core layer object cannot directly talk to Dxaml layer peer.

This design is due to historical reasons. In the days of Silverlight, Dxaml and Core layers were in different DLLs but 
since then they have been moved to a single DLL (Microsoft.UI.Xaml.dll). The [Background](#background) section covers 
it in greater detail.

One could try including Dxaml layer headers in core layer but it would result in including a lot of headers and may 
cause circular references. Thus, in absence of good refactoring strategy, some abstractions get used for any talk. The 
following sections will cover both kind of communication: Dxaml &rarr; Core and Core &rarr; Dxaml.

### Dxaml layer to Core layer communication

> Flow of communication: Dxaml &rarr; Core

This direction can be more straightforward. Cast public facing type to `DependencyObject` or even `UIElement` (see 
below) and call `GetHandle()`.

**Example:**

```c++
// usage:
// consider a type someType which is a child of UIElement
// one can get its core peer object by casting the object to UIElement and calling GetHandle
CUIElement* foo = someType.Cast<UIElement>()->GetHandle();

// Internally UIElement's GetHandle is doing the operations at DependencyObject base class level and then re-casting 
// the results back. Here is its definition
CUIElement* UIElement::GetHandle() const
{
    auto result = DependencyObject::GetHandle();
    ASSERT(!result || result->OfTypeByIndex<KnownTypeIndex::UIElement>());
    return static_cast<CUIElement*>(result);
}
```

Not all of the calls from DXaml to Core are implemented this way though. In Silverlight, calling from `System.Windows` 
to Core required a `pinvoke` call, since it was going from managed to native. So in Silverlight there was a 
`pinvokes.cs` file for all calls to Core. That was ported to C++, and so now there's a `pinvokes.cpp` (renamed to `CoreImports.cpp`) file through 
which many calls are made. For example, in `pinvokes.cpp` the `UIElement_Arrange` method is used by DXaml to call 
`CUIElement::Arrange` in the Core.
**Since this boundary does not exist anymore, there is not reason to have this indirection and it should be removed.**

### Core layer to Dxaml layer communication

> Flow of communication: Core &rarr; Dxaml

#### GetPeer in DxamlCore 

If the calling code exists in the Dxaml layer, even though it is calling for a core object, `DxamlCore::GetCurrent()` 
can directly give a `DxamlCore` instance.

```C++
// actual snippet from codebase
CUIElement* publicRootVisual = xamlRoot.Cast<XamlRoot>()->GetVisualTreeNoRef()->GetPublicRootVisual();
ctl::ComPtr<DependencyObject> peer;
DxamlCore::GetCurrent()->GetPeer(publicRootVisual, &peer);
```

#### GetPeer using DxamlServices 

If the calling code exists in core layer, it doesn't have any way to call the `DxamlCore` instance because including 
the `DxamlCore` header in the Core layer would require significant refactoring. `DxamlServices` keeps an instance of 
the `IDxamlCore` interface (of `DxamlCore`) which calls functions on that interface and that's how one can talk to 
DxamlCore instance from Core layer.

`GetPeer`/`TryGetPeer` functions are used by Core objects to get their peer Dxaml objects. `Try` is used when it is OK 
to fail.

```c++
// definition
_Check_return_ HRESULT DxamlServices::GetPeer(
            _In_ CDependencyObject* pDO,
            _Outptr_ DependencyObject** ppObject)

// usage
CDependencyObject *foo;
DirectUI::DependencyObject* DO{};
DirectUI::DxamlServices::GetPeer(foo, &DO); // returns a DO which needs to be casted to children type
auto DxamlLayerObject = static_cast<barType*>(DO);
```

`GetPeer()` takes a `CDependencyObject` and returns a `DependencyObject`, which then gets casted. One can use `ComPtr` 
with it effortlessly ([WinUI Pointers](./pointers.md))

#### FxCallbacks

`FxCallbacks` is yet another abstraction file which is used for certain common actions where a function needs to talk 
from Core to Dxaml layer. 
`dxaml/xcp/dxaml/lib/FxCallbacks.cpp`

Like the `pinvokes.cpp` file, FxCallbacks is a port of code in Silverlight that called
from Core to `System.Windows`, meaning it was a file full of reverse pinvokes.

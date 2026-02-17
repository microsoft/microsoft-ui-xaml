Window.AppWindow API
===

New API to simplify accessing [AppWindow](https://learn.microsoft.com/windows/apps/windows-app-sdk/windowing/windowing-overview)
functionality through WinUI 3 code.

# Background

XAML has a [Window](https://learn.microsoft.com/uwp/api/Windows.UI.Xaml.Window) API that
internally wraps an HWND. Windows has an [AppWindow](https://learn.microsoft.com/uwp/api/Windows.UI.WindowManagement.AppWindow) class
that similarly wraps an HWND in UWP. WinAppSDK has a new [AppWindow](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Windowing.AppWindow) which wraps an HWND and works on Desktop.

You can get an AppWindow from a XAML Window by calling a COM API to get XAML and then
a DLL export API to convert the HWND to an AppWindow. This spec adds a simple `Window.AppWindow`
property to make this much easier and more discoverable.

[XAML Window](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window) will
expose AppWindow object directly to app developer through an API. Instead of writing a lot of boilerplate
code everywhere, app developer can use this API, reducing code bloat, and making AppWindow APIs easily
accessible from WinUI 3 code.

These _before_ and _after_ C# code examples illustrate how this API simplifies integrating AppWindow APIs in WinUI 3 codebase.

- Before

```csharp
// This is needed to get any Window from inside a XAML control 
var xamlWindow = WindowHelper.GetWindowForElement(this); // API to get window object from UIElement (not a real API)  

// unnecessary boilerplate code 
var windowId = Win32Interop.GetWindowIdFromWindow(WindowNative.GetWindowHandle(xamlWindow)); 
var appWindow = AppWindow.GetFromWindowId(windowId); 

//calling function foo  (not a real AppWindow function)
appWindow.foo();
```

- After

```csharp
// This is needed to get any Window from inside a XAML control 
var xamlWindow = WindowHelper.GetWindowForElement(this); // API to get window object from UIElement (not a real API)  
   

//calling function foo directly
xamlWindow.AppWindow.foo();  
```

Notice how `xamlWindow.AppWindow.foo()` doesn't require additional steps to call the function `foo`.

# API Pages

## Window.AppWindow property

Gets the `AppWindow` associated with this `Window`.

```csharp
public Microsoft.UI.Windowing.AppWindow Window.AppWindow { get; }
```

### Example

```csharp
var xamlWindow = WindowHelper.GetWindowForElement(this);   
auto windowSize = xamlWindow.AppWindow.Size;  
```

Mapping an HWND to an AppWindow causes the HWND to be subclassed, meaning that the timing of
when [AppWindow.GetFromWindowId](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Windowing.AppWindow.GetFromWindowId)
is first called can potentially have a behavioral side effect. To ensure this is predictable, the AppWindow
for XAML's HWND will be created during the XAML Window's construction. Note that this could potentially be
an observable behavior change from the behavior before the introduction of this API.

New subclassing order with this new feature change:

- ContentAppWindowBridge<br/>
&darr;
- AppWindow  
&darr;
- XAML Window  
&darr;
- DefaultWndProc

# API Details

- MIDL3

```csharp
namespace Microsoft.UI.Xaml
{
   unsealed runtimeclass Window
  {
      ...
      Microsoft.UI.Windowing.AppWindow AppWindow{ get; };
  }
}
```

# Appendix

- Window.AppWindow API returns reference to the same AppWindow object every time. It gets created
  during XAML window object's creation. There is minimal performance impact observed for this change.
- Since AppWindow is an abstraction for HWND, its lifetime will be same as that of HWND. The
  API cannot return null as AppWindow will always be present if Window object is there. If called after
  window has been closed, it will fail similarly to other WinUI 3 APIs and return a failure HRESULT.
- There will be only one AppWindow object per top level HWND, created during Window object creation.
  No new AppWindow objects are created for child windows.
- Future scope: Since this API is limited to top level HWND, there should be a way to get app window object
  for top level HWND from one of its nested children HWND which doesn’t require app developer writing a lot of code.

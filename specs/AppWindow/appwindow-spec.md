Window.AppWindow api
===
New api to simplify accessing [appwindow](https://docs.microsoft.com/en-us/windows/apps/windows-app-sdk/windowing/windowing-overview)
functionality through WinUI 3 code

# Background
Xaml has a [Window](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Window) API that
internally wraps an hwnd. Windows has an [AppWindow](https://docs.microsoft.com/uwp/api/Windows.UI.WindowManagement.AppWindow) class
that similarly wraps an hwnd in UWP. WinAppSDK has a new [AppWindow](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Windowing.AppWindow) which wraps an hwnd and works on Desktop.

You can get an AppWindow from a Xaml Window by calling a COM API to get Xaml and then
a DLL export API to convert the hwnd to an AppWindow. This spec adds a simple `Window.AppWindow`
property to make this much easier and more discoverable.

[Xaml Window](https://docs.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window) will
expose AppWindow object directly to app developer through an api. Instead of writing a lot of boiler plate
code everywhere, app developer can use this api, reducing code bloat, and making appwindow apis easily
accessible from WinUI 3 code.

These *before* and *after* C# code examples illustrate how this api simplifies integrating appwindow apis in WinUI 3 codebase.

### Before
```c#
// This is needed to get any Window from inside a Xaml control 
var xamlWindow = WindowHelper.GetWindowForElement(this); // api to get window object from UIElement (not a real api)  

// unnecessary boiler plate code 
var windowId = Win32Interop.GetWindowIdFromWindow(WindowNative.GetWindowHandle(xamlWindow)); 
var appWindow = AppWindow.GetFromWindowId(windowId); 

//calling function foo  (not a real appwindow function)
appWindow.foo();
```

### After
```c#
// This is needed to get any Window from inside a Xaml control 
var xamlWindow = WindowHelper.GetWindowForElement(this); // api to get window object from UIElement (not a real api)  
   

//calling function foo directly
xamlWindow.AppWindow.foo();  
```
Notice how `xamlWindow.AppWindow.foo()` doesn't require additional steps to call the function `foo`.


# API Pages

## Window.AppWindow property

Gets the `AppWindow` associated with this `Window`.
```c#
public Microsoft.UI.Windowing.AppWindow Window.AppWindow { get; }
```
### Sample example
```c#
var xamlWindow = WindowHelper.GetWindowForElement(this);   
auto windowSize = xamlWindow.AppWindow.Size;  
```

Mapping an hwnd to an AppWindow causes the hwnd to be subclassed, meaning that the timing of
when [AppWindow.GetFromWindowId](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Windowing.AppWindow.GetFromWindowId)
is first called can potentially have a behavioral side effect. To ensure this is predictable, the AppWindow
for Xaml's hwnd will be created during the Xaml Window's construction. Note that this could potentially be
an observable behavior change from the behavior before the introduction of this API.

New subclassing order with this new feature change:

* ContentAppWindowBridge   
  &darr;
* AppWindow  
  &darr;
* Xaml Window  
&darr;
* DefaultWndProc


# API Details

```c# (but really MIDL3)
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
- Window.AppWindow api returns reference to the same appwindow object every time. It gets created
  during xaml window object's creation. There is minimal performance impact observed for this change.

- Since AppWindow is an abstraction for HWND, its lifetime will be same as that of HWND. The
  api cannot return null as AppWindow will always be present if Window object is there. If called after
  window has been closed, it will fail similarly to other WinUI 3 apis and return a failure HRESULT. 

- There will be only one appwindow object per top level HWND, created during Window object creation.
  No new appwindow objects are created for child windows. 

- Future scope : Since this api is limited to top level HWND, there should be a way to get app window object
  for top level HWND from one of its nested children HWND which doesn’t require app developer writing a lot of code. 


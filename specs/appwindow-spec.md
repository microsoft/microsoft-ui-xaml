Window.AppWindow api
===
New api to simplify accessing [appwindow](https://docs.microsoft.com/en-us/windows/apps/windows-app-sdk/windowing/windowing-overview) functionality through WinUI code

# Background
AppWindow provides good set of functionalities for using HWNDs than dealing with win32 apis directly. It is already accessible to winui desktop apps via winappsdk runtime. Currently, users must write a lot of boiler plate code to get to appwindow object and use it. 

[Xaml Window](https://docs.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window) will expose AppWindow object directly to app developer through an api. Instead of writing a lot of boiler plate code everywhere, app developer can use this api, reducing code bloat, and making appwindow apis easily accessible from winui code.


# API Pages

## Window.AppWindow property

This newly introduced api will allow developers to use appwindow object directly within WinUI code.
```c#
public Microsoft.UI.Windowing.AppWindow Window.AppWindow { get; }
```

These *before* and *after* C# code examples illustrate how this api simplifies integrating appwindow apis in winui codebase.


### Before
```c#
// This is needed to get any Window from inside a Xaml control 
var xamlWindow = WindowHelper.GetWindowForElement(this /* some Xaml UIElement */);   

// unnecessary boiler plate code 
var windowId = Win32Interop.GetWindowIdFromWindow(WindowNative.GetWindowHandle(xamlWindow)); 
var appWindow = AppWindow.GetFromWindowId(windowId); 

//calling function foo  (not a real appwindow function)
appWindow.foo();
```

### After
```c#
// This is needed to get any Window from inside a Xaml control 
var xamlWindow = WindowHelper.GetWindowForElement(this /* some Xaml UIElement */);   

//calling function foo directly
xamlWindow.AppWindow.foo();  
```
Notice how `xamlWindow.AppWindow.foo()` doesn't require additional steps to call the function `foo`.

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
- Window.AppWindow api returns reference to the same appwindow object every time. It gets created during xaml window object's creation.


- Since AppWindow is just an abstraction for HWND, its lifetime will be same as that of HWND. The api cannot return null as AppWindow will always be present if Window object is there. If called after window has been closed, it will fail similarly to other winui apis and return a failure HRESULT. 

- There will be just one appwindow object per top level HWND, created during Window object creation. No new appwindow objects are created for child windows. 

- Future scope : Since this api is limited to top level HWND, there should be a way to get app window object for top level HWND from one of its nested children HWND which doesn’t require app developer writing a lot of code. 


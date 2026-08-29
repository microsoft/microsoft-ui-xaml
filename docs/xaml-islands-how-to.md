# XAML islands in Win32, WinForms, and WPF

## Table of Contents

- [Making a XAML island](#making-a-xaml-island)
  - [Win32](#win32)
  - [WPF and WinForms](#wpf-and-winforms)
- [Don't want to embed an entire XAML application?](#dont-want-to-embed-an-entire-xaml-application)
  - [XAML](#xaml)
  - [Win32](#win32-1)
  - [WPF and WinForms](#wpf-and-winforms-1)
- [Sample applications](#sample-applications)
  - [XAML](#xaml-1)
  - [Win32](#win32-2)
  - [WPF](#wpf)
  - [WinForms](#winforms)

To get a XAML island in a non-XAML application, there are two required halves to put together -
the XAML you want to embed in the application, and the host in the application into which to embed it.

## Making a XAML island

On the XAML side, once you have all of your XAML content coded up that you want to embed, the steps to get it ready are 
as follows:

1. Add a reference to 
   [Microsoft.Toolkit.Win32.UI.XamlApplication.vcxproj](../src/XamlHost/Microsoft.Toolkit.Win32.UI.XamlApplication/Microsoft.Toolkit.Win32.UI.XamlApplication.vcxproj)
   (or to the corresponding NuGet package once we have one).
1. Modify your application such that your `App` type derives from 
   [XamlApplication](../src/XamlHost/Microsoft.Toolkit.Win32.UI.XamlApplication/XamlApplication.idl)
   instead of from `Application`.  This will make it able to provide additional type information that the
   XAML host in the second half requires.
1. Setting and getting the value of `Window.Current.Content` doesn't work in an islands content, so if you want to use 
   the pattern where the App type sets that value in `OnLaunched` to initialize the app content, you'll want to provide 
   a hook that the host app can use. In the XAML app code in 
   [App.xaml.cs](../Samples/XamlIslands/Shared/XamlApplicationCSharp/App.xaml.cs), this was done by providing getter 
   and setter lambda methods that the host app can pass in to provide callbacks to enable the XAML code to set the 
   host's child, and then calling `DispatcherQueue.GetForCurrentThread().TryEnqueue()` on the contents of OnLaunched to 
   ensure it executes after the lambdas have been set.

On the app side, the steps to hook it up to the XAML content depends on the framework you're building your app in.

### Win32

1. Add a reference to WinUI 3, 
   [Microsoft.Toolkit.Win32.UI.XamlApplication.vcxproj](../src/XamlHost/Microsoft.Toolkit.Win32.UI.XamlApplication/Microsoft.Toolkit.Win32.UI.XamlApplication.vcxproj)
   (or to the corresponding NuGet package once we have one), and to the XAML application project (e.g., 
   [XamlApplicationCppWinRT.vcxproj](../Samples/XamlIslands/Shared/XamlApplicationCppWinRT/XamlApplicationCppWinRT.vcxproj)).
2. In the Win32 application (see [this source code](../Samples/XamlIslands/Win32/XamlIslands.Win32.cpp) for an example):
    1. Create an `App` object;
    2. Call `WindowsXamlManager::InitializeForCurrentThread()` to initialize XAML;
    3. Create an instance of `DesktopWindowXamlSource` and call `Initialize`, passing in the WindowId of the HWND (retrieved via `GetWindowIdFromWindow`) of your Win32 application.
    4. Assign the lambdas that the XAML `App` object will use to set the value of `DesktopWindowXamlSource.Content`.
    5. Call `DesktopWindowXamlSource.SiteBridge().MoveAndResize` to resize and position the XAML window as desired.
3. Create a WAP project (see [here](../Samples/XamlIslands/Win32/XamlIslands.Win32.Wap.wapproj) for an example) with 
   references to the Win32 project and the XAML project, and which includes Microsoft.UI.AppX.targets from the WinUI 3 
   NuGet package (needed to embed the list of WinUI 3 types in the app manifest).

### WPF and WinForms

1. Add a reference to WinUI 3, 
   [Microsoft.Toolkit.Wpf.UI.XamlHost.csproj](../src/XamlHost/Microsoft.Toolkit.Wpf.UI.XamlHost/Microsoft.Toolkit.Wpf.UI.XamlHost.csproj)
   or 
   [Microsoft.Toolkit.Forms.UI.XamlHost.csproj](../src/XamlHost/Microsoft.Toolkit.Forms.UI.XamlHost/Microsoft.Toolkit.Forms.UI.XamlHost.csproj) 
   (depending on framework, or to the corresponding NuGet package once we have one), and to the XAML application 
   project (e.g., 
   [XamlApplicationCSharp.vcxproj](../Samples/XamlIslands/Shared/XamlApplicationCSharp/XamlApplicationCSharp.csproj)).
2. In the non-XAML application (see [this WPF markup](../Samples/XamlIslands/WPF/MainWindow.xaml) for an example):
    1. Create an instance of the `WindowsXamlHost` type.
    2. Size it as desired - it will automatically resize its corresponding HWND to match.
    3. Assign the lambdas that the XAML `App` object will use to set the value of `WindowsXamlHost.Child`.
3. Create a WAP project (see [here](../Samples/XamlIslands/WPF/XamlIslands.Wpf.Wap.wapproj) for an example) with 
   references to the non-XAML project and the XAML project, and which includes Microsoft.UI.AppX.targets from the 
   WinUI 3 NuGet package (needed to embed the list of WinUI 3 types in the app manifest).

## Don't want to embed an entire XAML application?

If you want to just embed a single XAML control instead of a whole application, then the steps are mostly the same as 
above, but with a few changes:

### XAML

* No need to have App derive from XAML application.
* No need to provide a hook to replace `Window.Current.Content`.

### Win32

* No need to assign lambdas - instead, manually create XAML objects and pass them to `DesktopWindowXamlSource.Content`.

### WPF and WinForms

* No need to assign lambdas - instead, either set `WindowsXamlHost.InitialTypeName` in your XAML to give it a default 
  type to load as its content, or manually create XAML objects and pass them to `WindowsXamlHost.Child`.

## Sample applications

### XAML

* [XamlApplicationCppWinRT](../Samples/XamlIslands/Shared/XamlApplicationCppWinRT)
* [XamlApplicationSharp](../Samples/XamlIslands/Shared/XamlApplicationSharp)

### Win32

* [XamlIslands.Win32](../Samples/XamlIslands/Win32) (uses [XamlApplicationCppWinRT](../Samples/XamlIslands/Shared/XamlApplicationCppWinRT))

### WPF

* [XamlIslands.WPF](../Samples/XamlIslands/Wpf) (uses [XamlApplicationCSharp](../Samples/XamlIslands/Shared/XamlApplicationCSharp))

### WinForms

* [XamlIslands.WinForms](../Samples/XamlIslands/WinForms) (uses [XamlApplicationCSharp](../Samples/XamlIslands/Shared/XamlApplicationCSharp))
# WinRT API changes for Desktop apps

Some Windows APIs are designed for and only completely supported in a UWP app, especially [CoreDispatcher](https://docs.microsoft.com/uwp/api/Windows.UI.Core.CoreDispatcher), [CoreWindow](https://docs.microsoft.com/uwp/api/Windows.UI.Core.CoreWindow), [ApplicationView](https://docs.microsoft.com/uwp/api/windows.ui.viewmanagement.applicationview), and some related classes. These APIs are partially supported in Desktop apps when _hosting_ XAML ([XAML Islands](https://docs.microsoft.com/windows/apps/desktop/modernize/xaml-islands)), and in WinUI 3 previews before Preview 4. As of WinUI 3 Preview 4 they are fully unsupported.

Note that when an API is unsupported, other APIs that depend on the former become unsupported too. For example, several classes in the Windows Runtime API have a static `GetForCurrentView()` method, such as [UIViewSettings.GetForCurrentView](https://docs.microsoft.com/uwp/api/Windows.UI.ViewManagement.UIViewSettings.GetForCurrentView), which is a pattern that indicates a dependence on `ApplicationView`, and therefore these are not supported in Desktop apps. 

This document lists out these APIs - these are no longer supported in Desktop apps, and neither are any of its members or other APIs that depend on it. There are suggested replacements below, where available, to achieve the same functionality. 

This doc is a live document and will continue to be updated as we identify more workarounds/replacements. If you are encountering issues with an API not listed here, please go ahead and [file a bug](https://github.com/microsoft/microsoft-ui-xaml/issues/new?assignees=&labels=&template=bug_report.md&title=) on this repo with the API in question and what you are trying to achieve by using it. 

## ApplicationView

- [`ApplicationViewTitleBar.ExtendViewIntoTitleBar`](https://docs.microsoft.com/uwp/api/windows.applicationmodel.core.coreapplicationviewtitlebar.extendviewintotitlebar) 
    
    Use `Window.ExtendsContentIntoTitleBar` instead. This API was added in Preview 4 and is intended as a permanent replacement in WinUI 3. API reference documentation for this is coming soon.
    
As mentioned above, `GetForCurrentView()` methods have a dependence on `ApplicationView` and are not supported in Desktop apps. Note that some `GetForCurrentView` methods will not only return null, but will also throw exceptions. We advise checking if `Window.Current()` is null before calling these APIs. 

## CoreWindow

- [`CoreWindow.GetKeyState()`](https://docs.microsoft.com/uwp/api/windows.ui.core.corewindow.getkeystate) 

    Use [`KeyboardInput.GetKeyStateForCurrentThread`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.input.keyboardinput.getkeystateforcurrentthread?view=winui-3.0-preview) instead.

- [`CoreWindow.PointerCursor`](https://docs.microsoft.com/uwp/api/windows.ui.core.corewindow.pointercursor) 

    Use [`UIElement.ProtectedCursor`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.uielement.protectedcursor?view=winui-3.0-preview) instead. Note that you'll need to have a subclass of `UIElement` to access this API. 

## CoreDispatcher
Note that the [Window.Dispatcher](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Window.Dispatcher) and [DependencyObject.Dispatcher](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.DependencyObject.Dispatcher) properties return `null` in a Desktop app.

- [`Window.Dispatcher`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.window.dispatcher?view=winui-3.0-preview) or [`CoreDispatcher`](https://docs.microsoft.com/uwp/api/windows.ui.core.coredispatcher)

    Use [`DispatcherQueue`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.window.dispatcherqueue?view=winui-3.0-preview) instead. `DispatcherQueue` is the permanent WinUI 3 replacement for `Dispatcher` and `CoreDispatcher`.

## CoreApplicationView
This API is no longer supported in Desktop apps, and neither are any of its members or other APIs that depend on it.

## DisplayInformation

- [`DisplayInformation.LogicalDpi`](https://docs.microsoft.com/uwp/api/windows.graphics.display.displayinformation.logicaldpi)

    Use [`XamlRoot.RasterizationScale`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.xamlroot.rasterizationscale?view=winui-3.0-preview) instead, and listen for changes on the [`XamlRoot.Changed`](https://docs.microsoft.com/uwp/api/windows.ui.xaml.xamlroot.changed) event. 

- [`DisplayInformation.RawPixelsPerViewPixel`](https://docs.microsoft.com/uwp/api/windows.graphics.display.displayinformation.rawpixelsperviewpixel)

    Use [`XamlRoot.RasterizationScale`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.xamlroot.rasterizationscale?view=winui-3.0-preview) instead. 

## UIViewSettings

- [`UIViewSettings.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.viewmanagement.uiviewsettings.getforcurrentview)

    No alternative - `UIViewSettings` is not intended to be supported in Desktop apps.

## SystemNavigationManager


- [`SystemNavigationManager.GetForCurrentView()`](https://docs.microsoft.com/uwp/api/windows.ui.core.systemnavigationmanager.getforcurrentview)

    No alternative - this functionality is not intended to be supported in Desktop apps. 

# WinRT API changes for Desktop apps

Some Windows APIs are designed for and only completely supported in a UWP app, especially CoreDispatcher, CoreWindow, ApplicationView, and some related classes. These APIs are partially supported in Desktop apps when _hosting_ Xaml (Xaml Islands), and in WinUI3 previews before Preview4. As of WinUI3 Preview4 they are fully unsupported.

Note that when a class is unsupported in Desktop it can lead to other unsupported classes. For example, several classes in the Windows API have a static GetForCurrentView method, such as [UIViewSettings.GetForCurrentView](https://docs.microsoft.com/uwp/api/Windows.UI.ViewManagement.UIViewSettings.GetForCurrentView), which is a pattern that indicates a dependence on ApplicationView, and so are not supported in Desktop apps.

This document lists out these APIs - these are no longer supported in Desktop apps, and neither are any of its members or other APIs that depend on it. There are suggested replacements below, where available, to achieve the same functionality. 

This doc will continue to be updated as our community files bugs and as we identify more workarounds/replacements. If you are encountering issues with an API not listed here, please go ahead and [file a bug](https://github.com/microsoft/microsoft-ui-xaml/issues/new?assignees=&labels=&template=bug_report.md&title=) on this repo with the API in question and what you are trying to achieve by using it. 

## ApplicationView

- [`ApplicationViewTitleBar.ExtendViewIntoTitleBar`](https://docs.microsoft.com/en-us/uwp/api/windows.applicationmodel.core.coreapplicationviewtitlebar.extendviewintotitlebar?view=winrt-19041) 
    
    Use `Window.ExtendsContentIntoTitleBar` instead. This API was added in Preview 4 and is intended as a permanent replacement in WinUI 3. API reference documentation for this is coming soon.

## CoreWindow

- [`CoreWindow.GetKeyState()`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.core.corewindow.getkeystate?view=winrt-19041) 

    Use [`KeyboardInput.GetKeyStateForCurrentThread`](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.input.keyboardinput.getkeystateforcurrentthread?view=winui-3.0-preview) instead.

- [`CoreWindow.PointerCursor`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.core.corewindow.pointercursor?view=winrt-19041) 

    Use [`UIElement.ProtectedCursor`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.uielement.protectedcursor?view=winui-3.0-preview) instead. Note that you'll need to have a subclass of UIElement to access this API. 

## CoreDispatcher
Note too that the [Window.Dispatcher](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Window.Dispatcher) and [DependencyObject.Dispatcher](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.DependencyObject.Dispatcher) properties return null in a Desktop app.

- [`Window.Dispatcher`](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.window.dispatcher?view=winui-3.0-preview) or [`CoreDispatcher`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.core.coredispatcher?view=winrt-19041)

    Use [`DispatcherQueue`](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.window.dispatcherqueue?view=winui-3.0-preview) instead. `DispatcherQueue` is the permanent WinUI 3 replacement for `Dispatcher` and `CoreDispatcher`.

## CoreApplicationView
This API is no longer supported in Desktop apps, and neither are any of its members or other APIs that depend on it.

## DisplayInformation

- [`DisplayInformation.LogicalDpi`](https://docs.microsoft.com/en-us/uwp/api/windows.graphics.display.displayinformation.logicaldpi?view=winrt-19041)
    
    Use [`XamlRoot.RasterizationScale`](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.xamlroot.rasterizationscale?view=winui-3.0-preview) instead, and listen for changes on the [`XamlRoot.Changed`](https://docs.microsoft.com/uwp/api/windows.ui.xaml.xamlroot.changed?view=winrt-19041) event. 

- [`DisplayInformation.RawPixelsPerViewPixel`](https://docs.microsoft.com/uwp/api/windows.graphics.display.displayinformation.rawpixelsperviewpixel?view=winrt-19041)

    Use [`XamlRoot.RasterizationScale`](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.xamlroot.rasterizationscale?view=winui-3.0-preview) instead. 

## UIViewSettings

- [`UIViewSettings.GetForCurrentView()`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.viewmanagement.uiviewsettings.getforcurrentview?view=winrt-19041)

    No alternative - `UIViewSettings` is not intended to be supported in Desktop apps.

## SystemNavigationManager


- [`SystemNavigationManager.GetForCurrentView()`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.core.systemnavigationmanager.getforcurrentview?view=winrt-19041)

    No alternative - this functionality is not intended to be supported in Desktop apps. 

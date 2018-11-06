Windows UI Library
===========

The Windows UI Library (WinUI) provides backward-compatible implementations of the default Microsoft UWP XAML UI platform for Windows 10. 

It can be easily used with any standard Windows 10 UWP XAML app, or a Xamarin.Forms app running on Windows 10 using [native view embedding](https://docs.microsoft.com/xamarin/xamarin-forms/platform/native-views).

A prerelease preview of the WinUI Library is available via [NuGet](https://docs.microsoft.com/nuget/what-is-nuget) packages, and we are working toward making it open source.

The WinUI Library provides a few key benefits when building apps for Windows 10:

1. Helps you stay up to date with the latest versions of key controls and features of [XAML](https://docs.microsoft.com/windows/uwp/xaml-platform/xaml-overview) and the [Fluent Design System](https://www.microsoft.com/design/fluent)
2. Lets you start building and shipping apps with new UWP XAML features immediately 
    * It's backward-compatible with a wider range of Windows 10 versions, so you don't have to wait for your users to update their OS before they can run your app with the latest features
3. Makes it simpler to build version adaptive apps
    * You don't need version checks or conditional XAML markup to use WinUI controls or features: they automatically adapt to the user's OS version

## Getting started
Please read the [Getting Started with the Windows UI Library](https://docs.microsoft.com/uwp/toolkits/winui/getting-started) page for more detailed information about using the WinUI Library.

## Documentation
Documentation for the library is available here:

https://docs.microsoft.com/uwp/toolkits/winui 

## Sample App
A sample app that provides an overview of the XAML UI framework, including the WinUI Library, is [available on GitHub](https://github.com/Microsoft/Windows-universal-samples/tree/dev/Samples/XamlUIBasics/cs/AppUIBasics).

The app is also available for download from the Microsoft Store: [Xaml Controls Gallery](https://www.microsoft.com/store/productId/9MSVH128X2ZT).

## Nuget Packages
NuGet is the standard package manager built into Visual Studio.

This repo provides information and issue tracking for the following NuGet packages:

 NuGet Package Name | Description |
| --- | --- |
| [Microsoft.UI.Xaml](https://www.nuget.org/packages/Microsoft.UI.Xaml) | Controls and features for building apps for Windows 10 |
| [Microsoft.UI.Xaml.Core.Direct](https://www.nuget.org/packages/Microsoft.UI.Xaml.Core.Direct) | Low-level APIs that can enable better performance when creating middleware components. Not intended or required for general application use |

## Supported OS versions
* Windows Insider Preview (17764+)
* October 2018 Update (17763)
* April 2018 Update (17134)
* Fall Creators Update (16299)
* Creators Update (15063)
* Anniversary Update (14393)

Note some features may have a reduced or slightly different user experience on older versions, particularly on builds before 15063. This should not impact overall usability.

The Microsoft.UI.Xaml NuGet package requires TargetPlatformVersion &gt;= 10.0.17134.0 and TargetPlatformMinVersion &gt;= 10.0.14393.0 for your project.

## Features

The prerelease preview packages contain the following features. 
Note this is not an exhaustive list.

### Fluent Design Elements
* Fluent Design control styles for all XAML controls (both in the Windows UI Library and the standard SDK)
* [Fluent Acrylic](https://docs.microsoft.com/windows/uwp/design/style/acrylic) features for custom Acrylic styling, including:
  * [AcrylicBrush](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.media.acrylicbrush)
* [Fluent Reveal](https://docs.microsoft.com/windows/uwp/design/style/reveal) features for custom Reveal styling, including:
  * [RevealBackgroundBrush](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.media.revealbackgroundbrush)
  * [RevealBorderBrush](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.media.revealborderbrush)

### Controls
* [ColorPicker](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.colorpicker)
* [CommandBarFlyout](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.commandbarflyout)
* [DropDownButton](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.dropdownbutton)
* [MenuBar](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.menubar)
* [NavigationView](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.navigationview)
* [ParallaxView](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.parallaxview)
* [PersonPicture](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.personpicture)
* [RatingControl](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.ratingcontrol)
* [RefreshContainer](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.refreshcontainer)
* [SplitButton](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.splitbutton)
* [SwipeControl](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.swipecontrol)
* [SymbolIconSource](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.symboliconsource)
* [TextCommandBarFlyout](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.textcommandbarflyout)
* [ToggleSplitButton](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.togglesplitbutton)
* [TreeView](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.treeview)

### Preview Features
These are early prerelease features and controls that we are still working on. 

They will be available in prerelease NuGet packages but not in the stable package versions until they are ready. They are subject to change without notice and may be removed.

* [LayoutPanel](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.layoutpanel) and related classes
* [Repeater](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.repeater) and related classes
* [Scroller](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.scroller) and related classes
* [ScrollerView](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.scrollerview) and related classes
* [ScrollBar2](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.scrollbar2)

### Middleware Support APIs

* [XamlDirect](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.core.direct) and related APIs to improve performance for middleware components

## Feedback and Requests
Please use [GitHub Issues](https://github.com/Microsoft/microsoft-ui-xaml/issues) for bug reports and feature requests.

For feature requests, please also vote or create en entry on our [Uservoice feedback site](https://wpdev.uservoice.com/forums/110705-universal-windows-platform?category_id=58517).


## Contributing
This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

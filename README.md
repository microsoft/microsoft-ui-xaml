# Windows UI Library

The Windows UI Library (WinUI) provides official native Microsoft UI controls and features for Windows [UWP apps](https://docs.microsoft.com/windows/uwp/index).

WinUI is the easiest way to build great [Fluent Design](https://docs.microsoft.com/windows/uwp/design/fluent-design-system/) experiences for Windows.

WinUI can be used in any Windows 10 UWP XAML app, or in a Xamarin.Forms app running on Windows 10 using [native view embedding](https://docs.microsoft.com/xamarin/xamarin-forms/platform/native-views).

## Using WinUI
You can download and use WinUI packages in your app using the NuGet package manager: see the [Getting Started with the Windows UI Library](https://docs.microsoft.com/uwp/toolkits/winui/getting-started) page for more information.

### Packages

| NuGet Package | Build Status | Latest Versions |
| --- | --- | --- |
| [Microsoft.UI.Xaml](https://www.nuget.org/packages/Microsoft.UI.Xaml) <br /> Controls and Fluent Design for UWP apps | [![Build Status](https://dev.azure.com/ms/microsoft-ui-xaml/_apis/build/status/WinUI-Public-MUX-CI?branchName=master)](https://dev.azure.com/ms/microsoft-ui-xaml/_build/latest?definitionId=20?branchName=master) | [![latest stable version](https://img.shields.io/nuget/v/Microsoft.UI.Xaml.svg)](https://www.nuget.org/packages/Microsoft.UI.Xaml) <br /> [![latest prerelease version](https://img.shields.io/nuget/vpre/Microsoft.UI.Xaml.svg)](https://www.nuget.org/packages/Microsoft.UI.Xaml/2.1.181217001-prerelease) |
| [Microsoft.UI.Xaml.Core.Direct](https://www.nuget.org/packages/Microsoft.UI.Xaml.Core.Direct) <br /> Low-level APIs for middleware components | | [![latest prerelease version](https://img.shields.io/nuget/vpre/Microsoft.UI.Xaml.Core.Direct.svg)](https://www.nuget.org/packages/Microsoft.UI.Xaml.Core.Direct/2.0.180924001-prerelease) |

You can also build a WinUI package yourself from source. See [Contributing to the Windows UI Library](CONTRIBUTING.md) for more information on building and contributing to WinUI.

## Documentation

WinUI usage documentation can be found here:

https://docs.microsoft.com/uwp/toolkits/winui

## Contributing to WinUI
The WinUI team welcomes feedback and contributions!

For information on how to contribute please see [Contributing to the Windows UI Library](CONTRIBUTING.md).

## WinUI features

### Benefits

The WinUI Library provides some useful benefits when building apps for Windows 10:

1. **Helps you stay up to date**  
WinUI helps keep your app up to date with the latest versions of key controls and features of [UWP XAML](https://docs.microsoft.com/windows/uwp/xaml-platform/xaml-overview) and the [Fluent Design System](https://www.microsoft.com/design/fluent)

2. **Provides backward compatibility**  
WinUI is backward-compatible with a wide range of Windows 10 versions: you can start building and shipping apps with new XAML features immediately as soon as they're released, even if your users aren't on the latest version of Windows 10

3. **Makes it simpler to build version adaptive apps**  
You don't need version checks or conditional XAML markup to use WinUI controls or features: WinUI automatically adapts to the user's OS version

### Version support

The Microsoft.UI.Xaml NuGet package requires your project to have TargetPlatformVersion &gt;= 10.0.17134.0 and TargetPlatformMinVersion &gt;= 10.0.14393.0 when building. 

Your app's users can be on any of the following supported Windows versions:
* Windows Insider Previews
* October 2018 Update (17763)
* April 2018 Update (17134)
* Fall Creators Update (16299)
* Creators Update (15063)
* Anniversary Update (14393)

Some features may have a reduced or slightly different user experience on older versions, particularly on builds before 15063. This should not impact overall usability.

### Contents

The **Microsoft.UI.Xaml** 2.0 stable package includes:

* Fluent Design control styles for all XAML controls (from both WinUI and the standard Windows 10 SDK)
* Fluent materials and styles including [Fluent Acrylic](https://docs.microsoft.com/windows/uwp/design/style/acrylic) and [Fluent Reveal](https://docs.microsoft.com/windows/uwp/design/style/reveal)
* Many new and popular controls like 
[ColorPicker](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.colorpicker),
[MenuBar](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.menubar),
[NavigationView](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.navigationview),
[ParallaxView](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.parallaxview),
[PersonPicture](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.personpicture),
[RatingControl](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.ratingcontrol),
[RefreshContainer](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.refreshcontainer),
[SplitButton](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.splitbutton),
[SwipeControl](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.swipecontrol),
and
[TreeView](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.treeview)

Prerelease Microsoft.UI.Xaml packages also include previews of features and controls that we are still working on, like:
* [LayoutPanel](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.layoutpanel),
[Repeater](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.repeater),
[Scroller](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.scroller),
[ScrollerView](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.scrollerview),
and
[ScrollBar2](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.controls.scrollbar2)

The **Microsoft.UI.Xaml.Core.Direct** package includes:

* [XamlDirect](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.core.direct) and related APIs to improve performance for middleware components

## Roadmap

For info on the WinUI release schedule and high level plans please see the [Windows UI Library Roadmap](docs/roadmap.md).

## Data/Telemetry

This project collects usage data and sends it to Microsoft to help improve our products and services. Read Microsoft's [privacy statement to learn more](https://privacy.microsoft.com/privacystatement).

For more information on telemetry implementation see the [developer guide](docs/developer_guide.md#Telemetry).

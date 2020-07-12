
# Windows UI Library

[![Follow WinUI on Twitter](https://img.shields.io/twitter/follow/windowsui.svg?label=Follow%20WinUI%20on%20Twitter&style=social)](https://twitter.com/intent/follow?screen_name=windowsui)

WinUI is a user interface layer that contains modern controls and styles for building Windows apps. </span> As the native UI layer in Windows it embodies <a href="https://www.microsoft.com/design/fluent/#/">Fluent Design</a>, giving each Windows app the polished feel that customers expect.

WinUI 2 is a library of controls that provides official native Microsoft UI controls and features for Windows [UWP apps](https://docs.microsoft.com/windows/uwp/index). WinUI 2 can be used in any Windows 10 UWP XAML app, or in a Xamarin. Forms app running on Windows 10 using [native view embedding](https://docs.microsoft.com/xamarin/xamarin-forms/platform/native-views).

WinUI 3 is the next version of the WinUI framework, shipping later this year. It dramatically expands WinUI into a full UX framework, making WinUI available for all types of Windows apps – from Win32 to UWP – for use as the UI layer.
 
## WinUI Community Calls

The WinUI community call is your monthly opportunity to learn about native UX development for Windows with WinUI.

In these calls we’ll discuss the WinUI roadmap, our status and your feedback.

You can watch them online here on YouTube at the [Windows Developer channel](https://www.youtube.com/channel/UCzLbHrU7U3cUDNQWWAqjceA).

Add the event to your calendar: [ICS calendar file](https://aka.ms/winuicommunitycall)

## WinUI 3.0 Preview 1 (May 2020)

As outlined in the [roadmap](docs/roadmap.md) we're currently working on WinUI 3.0, which will greatly expand the scope of WinUI to include the full native Windows UI platform.

You can now [download an early build of WinUI 3.0 Preview 1](https://docs.microsoft.com/uwp/toolkits/winui3/) to try out - we'd love your feedback!

For more info see the [discussion issue #2472](https://github.com/microsoft/microsoft-ui-xaml/issues/2472).

## Using WinUI
You can download and use WinUI packages in your app using the NuGet package manager: see the [Getting Started with the Windows UI Library](https://docs.microsoft.com/uwp/toolkits/winui/getting-started) page for more information.

### Packages

| NuGet Package | Build Status | Latest Versions | Documentation |
| --- | --- | --- | --- |
| [Microsoft.UI.Xaml](https://www.nuget.org/packages/Microsoft.UI.Xaml) <br /> Controls and Fluent Design for UWP apps | [![Build Status](https://dev.azure.com/ms/microsoft-ui-xaml/_apis/build/status/WinUI-Public-MUX-CI?branchName=master)](https://dev.azure.com/ms/microsoft-ui-xaml/_build/latest?definitionId=20?branchName=master) | [![latest stable version](https://img.shields.io/nuget/v/Microsoft.UI.Xaml.svg)](https://www.nuget.org/packages/Microsoft.UI.Xaml) <br /> [![latest prerelease version](https://img.shields.io/nuget/vpre/Microsoft.UI.Xaml.svg)](https://www.nuget.org/packages/Microsoft.UI.Xaml/absoluteLatest) | [2.4 release](https://docs.microsoft.com/windows/apps/winui/winui2/release-notes/winui-2.4) |
| [Microsoft.UI.Xaml.Core.Direct](https://www.nuget.org/packages/Microsoft.UI.Xaml.Core.Direct) <br /> Low-level APIs for middleware components | | [![latest prerelease version](https://img.shields.io/nuget/vpre/Microsoft.UI.Xaml.Core.Direct.svg)](https://www.nuget.org/packages/Microsoft.UI.Xaml.Core.Direct/absoluteLatest) | [2.0 prerelease](https://docs.microsoft.com/uwp/api/microsoft.ui.xaml.core.direct) |

You can also build a WinUI package yourself from source. See [Contributing to the Windows UI Library](CONTRIBUTING.md) for more information on building and contributing to WinUI.

## Documentation

**WinUI usage documentation**:  
https://docs.microsoft.com/uwp/toolkits/winui

**Release notes**:  
https://docs.microsoft.com/uwp/toolkits/winui/release-notes/

**Sample code**:  
To view the WinUI controls in an interactive format, check out the Xaml Controls Gallery:
* Get the XAML Controls Gallery app from the [Microsoft Store](https://www.microsoft.com/store/productId/9MSVH128X2ZT)
* Get the source code on [GitHub](https://github.com/Microsoft/Xaml-Controls-Gallery)

[WinUI](https://microsoft.github.io/microsoft-ui-xaml/) also has its own website where you can learn more about it.

## Contributing to WinUI
The WinUI team welcomes feedback and contributions!

For information on how to contribute please see [Contributing to the Windows UI Library](CONTRIBUTING.md).

## WinUI features

### Benefits

WinUI 2 provides some useful benefits when building apps for Windows 10:

1. **Helps you stay up to date**  
WinUI helps keep your app up to date with the latest versions of key controls and features of [UWP XAML](https://docs.microsoft.com/windows/uwp/xaml-platform/xaml-overview) and the [Fluent Design System](https://www.microsoft.com/design/fluent)

2. **Provides backward compatibility**  
WinUI is backward-compatible with a wide range of Windows 10 versions: you can start building and shipping apps with new XAML features immediately as soon as they're released, even if your users aren't on the latest version of Windows 10

3. **Makes it simpler to build version adaptive apps**  
You don't need version checks or conditional XAML markup to use WinUI controls or features: WinUI automatically adapts to the user's OS version

### Version support

The Microsoft.UI.Xaml 2.4 NuGet package requires your project to have TargetPlatformVersion &gt;= 10.0.18362.0 and TargetPlatformMinVersion &gt;= 10.0.15063.0 when building. 

Your app's users can be on any of the following supported Windows 10 versions:

* Windows 10 1703 - Build 15063 (Creators Update aka "Redstone 2") and newer (including Windows Insider Previews)

Some features may have a reduced or slightly different user experience on older versions.

## Roadmap

For info on the WinUI release schedule and high level plans please see the [Windows UI Library Roadmap](docs/roadmap.md).

## WinUI is a part of the Project Reunion family
[Project Reunion](https://github.com/microsoft/ProjectReunion) is a set of libraries, frameworks, components, and tools that you can use in your apps to access powerful Windows platform functionality from all kinds of apps on many versions of Windows. Project Reunion combines the powers of Win32 native applications alongside modern API usage techniques, so your apps light up everywhere your users are. 
 
Other Project Reunion components are: [WebView2](https://docs.microsoft.com/microsoft-edge/webview2/),  [MSIX (MSIX-Core)](https://docs.microsoft.com/windows/msix/overview), [C++/WinRT](https://github.com/microsoft/cppwinrt), [Rust/WinRT](https://github.com/microsoft/winrt-rs), and [C#/WinRT](https://github.com/microsoft/cswinrt). If you'd like to learn more and contribute to Project Reunion, or have **UWP/app model related questions**, visit our [Github repo](https://github.com/microsoft/ProjectReunion). 

## Data/Telemetry

This project collects usage data and sends it to Microsoft to help improve our products and services. See the [privacy statement](privacy.md) for more details.

For more information on telemetry implementation see the [developer guide](docs/developer_guide.md#Telemetry).

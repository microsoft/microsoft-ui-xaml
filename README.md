
# Windows UI Library

[![Follow WinUI on Twitter](https://img.shields.io/twitter/follow/windowsui.svg?label=Follow%20WinUI%20on%20Twitter&style=social)](https://twitter.com/intent/follow?screen_name=windowsui)

WinUI is a user interface layer that contains modern controls and styles for building Windows apps. </span> As the native UI layer in Windows it embodies <a href="https://www.microsoft.com/design/fluent/#/">Fluent Design</a>, giving each Windows app the polished feel that customers expect.

WinUI 3 is the next generation of the WinUI framework. It dramatically expands WinUI into a full UX framework, making WinUI available for Win32 Windows apps for use as the UI layer.
 
## WinUI Community Calls

The WinUI community call is your monthly opportunity to learn about native UX development for Windows with WinUI.

In these calls we’ll discuss the WinUI [roadmap](https://github.com/microsoft/microsoft-ui-xaml/blob/main/docs/roadmap.md), our status and your feedback.

You can watch them online here on YouTube at the [Windows Developer channel](https://www.youtube.com/channel/UCzLbHrU7U3cUDNQWWAqjceA).


## Using WinUI 3

You can build new Windows apps using WinUI 3, which ships as a part of the Windows App SDK. The latest available stable release is the Windows App SDK 1.4 (previously called Project Reunion). With this release, you can ship production Desktop apps to the Microsoft Store.

See the [installation instructions](https://docs.microsoft.com/windows/apps/windows-app-sdk/set-up-your-development-environment), and guidelines on [creating your first WinUI 3 app](https://docs.microsoft.com/windows/apps/winui/winui3/create-your-first-winui3-app). 


## Debugging WinUI 3

Full symbols for WinUI 3 starting with version 1.4.1 are available on the [Microsoft public symbol server](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/microsoft-public-symbols). If you wish to debug the WinUI 3 source code, then you can do so by cloning this repository and checking out the [tag corresponding to the version of WinUI 3](https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/downloads) that you are using, adding the Microsoft public symbol server to your debugger's [symbol path](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/symbol-path), and adding your clone of this repository to your debugger's [source path](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/source-path). If you are using Visual Studio as your debugger, please ensure that you enable 'Native debugging' and disable 'Just My Code'.

## Documentation

To find resources for Windows UI 3, like the [Figma design toolkit](https://aka.ms/WinUI/3.0-figma-toolkit), Segoe UI Variable Font, and samples, visit [Design toolkits and samples for Windows apps](https://docs.microsoft.com/windows/apps/design/downloads/)

If you find any issues with the Windows UI toolkit, you can file a bug [here](https://aka.ms/WinUIToolkitBug)

**WinUI usage documentation**:  
https://docs.microsoft.com/windows/apps/winui/

**WinUI 3 Release notes**:
https://docs.microsoft.com/windows/apps/windows-app-sdk/stable-channel

**Sample code**:  
To view the WinUI 3 controls in an interactive format, get the WinUI 3 Gallery app from the [Microsoft Store](https://www.microsoft.com/p/winui-3-controls-gallery/9p3jfpwwdzrc) or get the source code on  [GitHub](https://github.com/microsoft/Xaml-Controls-Gallery/tree/winui3).

[WinUI 3](https://aka.ms/winui3) also has its own website where you can learn more about it.

## Contributing to WinUI
The WinUI team welcomes your feedback!

To understand how we handle incoming feature requests and bugs, please see our [contribution handling](CONTRIBUTION_HANDLING.md) guidelines.

For information on how to contribute, please see [Contributing to the Windows UI Library](CONTRIBUTING.md). **Please note that although the source code for WinUI 3 is available in this repository it cannot be compiled by non-Microsoft developers at this time.**

For guidelines on making an impact on WinUI through non-code contributions, please see [Contributing ideas, feedback, and requests](CONTRIBUTING_feedback_and_requests.md).

## WinUI features

### Benefits

WinUI 3 provides some useful benefits when building apps for Windows:

1. **Modernizing existing apps**
    * Enabling you to extend existing Win32 (WPF, WinForms, MFC...) apps with modern Windows UI at your own pace using 
    [Xaml Islands](https://docs.microsoft.com/windows/apps/desktop/modernize/xaml-islands). 
    Developers who currently use WinUI 2 for their app UX will be able to easily move to WinUI 3, as their syntax and
    capabilities are very similar
2. **Creating new Windows apps**
    * Enabling you to easily create new modern Windows apps with the flexibility offered by the [Windows App SDK](https://docs.microsoft.com/windows/apps/windows-app-sdk/)
3. **Enabling other frameworks**
    * Providing the native implementation for other frameworks like [React Native](https://github.com/Microsoft/react-native-windows) and .NET [MAUI](https://docs.microsoft.com/dotnet/maui/what-is-maui) when running on Windows.

WinUI 3 is available as a part of the [Windows App SDK](https://docs.microsoft.com/windows/apps/windows-app-sdk) for building stable and supported desktop/Win32 apps for production scenarios. The latest release is the Windows App SDK 1.4, which you can download and read more about at the documentation linked below:

[Stable release channel for the Windows App SDK](https://docs.microsoft.com/windows/apps/windows-app-sdk/stable-channel)

We also ship experimental features as we develop them. You can read more about the Windows App SDK Experimental releases at the following documentation. Note that Experimental releases have limitations and known issues, so they are not equipped for production apps.

[Experimental release channel for the Windows App SDK](https://docs.microsoft.com/windows/apps/windows-app-sdk/experimental-channel).

### Version support

WinUI 3 is part of Windows App SDK and so supports the same OS versions: down to build 17763 (version 1809/October 2018 Update) of Windows 10.

Some features may have a reduced or slightly different user experience on older versions.


## Roadmap

For info on the WinUI release schedule and high level plans please see the [Windows UI Library Roadmap](https://github.com/microsoft/microsoft-ui-xaml/blob/main/docs/roadmap.md).

## WinUI 3 is a part of the Windows App SDK family
The [Windows App SDK](https://github.com/microsoft/ProjectReunion) is a set of libraries, frameworks, components, and tools that you can use in your apps to access powerful Windows platform functionality from all kinds of apps on many versions of Windows. The Windows App SDK combines the powers of Win32 native applications alongside modern API usage techniques, so your apps light up everywhere your users are. 
 
Other Windows App SDK components are: [WebView2](https://docs.microsoft.com/microsoft-edge/webview2/),  [MSIX (MSIX-Core)](https://docs.microsoft.com/windows/msix/overview), [C++/WinRT](https://github.com/microsoft/cppwinrt), [Rust/WinRT](https://github.com/microsoft/winrt-rs), and [C#/WinRT](https://github.com/microsoft/cswinrt). If you'd like to learn more and contribute to Windows App SDK, or have **UWP/app model related questions**, visit our [Github repo](https://github.com/microsoft/WindowsAppSDK). 

For a detailed look at the features we're planning on releasing in WinAppSDK check out the [Windows App SDK feature roadmap](https://github.com/microsoft/WindowsAppSDK/blob/main/docs/roadmap.md).

## Data/Telemetry

This project collects usage data and sends it to Microsoft to help improve our products and services. See the [privacy statement](PRIVACY.md) for more details.

# Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).

For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.

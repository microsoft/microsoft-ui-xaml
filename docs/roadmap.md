# Windows UI Library Overview

WinUI is the modern native UI platform for Windows with two active generations:

1. **WinUI 3**: The latest, 3rd generation of WinUI that ships the entire WinUI stack decoupled from the operating system as a part of the [Windows App SDK](https://docs.microsoft.com/windows/apps/windows-app-sdk/).

2. **WinUI 2**: The previous generation of the WinUI stack for UWP apps, consisting of a XAML and Visual Layer built directly into the Windows 10 operating system, and a controls library built on top of the OS, delivered via NuGet, and hosted at this repository. WinUI 2 will continue to be supported with bug, reliability, and security fixes.

**For a detailed look on the difference between WinUI 2 & 3, please view our [comparison table](https://docs.microsoft.com/windows/apps/winui/#comparison-of-winui-3-and-winui-2)

### Benefits of WinUI 

WinUI provides a number of benefits which makes it the best way to create user interfaces for Windows apps:

1. **The native UI platform of Windows**  
WinUI is the highly-optimized native UI platform used to create Windows itself, now made more broadly available for all developers to use to reach Windows. It's a thoroughly tested and proven UI platform that powers the operating system environment and essential experiences of 1 billion+ Windows 10 & 11 PC, XBox One, HoloLens, Surface Hub and other devices.

2. **The latest advancements in Fluent Design**  
WinUI is Microsoft's main focus for native, accessible Windows UI and controls and is the definitive source for the [Fluent Design System](https://www.microsoft.com/design/fluent/) on Windows.  
It will also support the latest composition and rendering innovations like vector animations, effects, shadows and lighting.

3. **Backward compatibility for new features**  
New WinUI features will continue to be backward-compatible with a wide range of Windows 10 and 11 versions. With WinUI 3, you can start building and shipping apps with new features immediately as soon as they're released, without having to wait for your users to be running the latest update of Windows.

4. **Native development support**  
WinUI can be used with .NET, but doesn't depend on .NET: WinUI is 100% C++ and can be used in unmanaged Windows apps, for example using standard C++17 via [C++/WinRT](https://docs.microsoft.com/windows/uwp/cpp-and-winrt-apis/).

5. **More frequent updates**  
WinUI is planned to ship roughly every 6 months, with at least two preview builds per stable release.  This is more of a guideline than a rule, but that is what the team strives for.

6. **Open source development and community engagement**  
 The WinUI 2 Controls Library is already open source on GitHub, and we're planning to add the full WinUI 3 framework into this repo as well. You can engage directly with Microsoft's core engineering team and contribute bug reports, feature ideas, and even code: see the [Contribution Guide](../CONTRIBUTING.md) for more info.  You can also try out the pre-release builds to see new in-development features and help shape their final form.  

## WinUI 3

**[WinUI 3](https://docs.microsoft.com/windows/apps/winui/winui3/)** is the latest generation of native Windows UI, consisting of all the major UX layers of Windows decoupled and shipping as a standalone solution for you to use.

It focuses on enabling three main use cases:

1. **Modernizing existing apps**
    * Enabling you to extend existing Win32 (WPF, WinForms, MFC...) apps with modern Windows UI at your own pace using the upcoming release of [Xaml Islands](https://docs.microsoft.com/windows/apps/desktop/modernize/xaml-islands). Developers who currently use WinUI 2 for their app UX will be able to easily move to WinUI 3, as their syntax and capabilities are very similar
2. **Creating new Windows apps**
    * Enabling you to easily create new modern Windows apps with the flexibility offered by the [Windows App SDK](https://docs.microsoft.com/windows/apps/windows-app-sdk/)
3. **Enabling other frameworks**
    * Providing the native implementation for other frameworks like [React Native](https://github.com/Microsoft/react-native-windows) and .NET [MAUI](https://docs.microsoft.com/dotnet/maui/what-is-maui) when running on Windows.

WinUI 3 is available as a part of the [Windows App SDK](https://docs.microsoft.com/windows/apps/windows-app-sdk) for building stable and supported desktop/Win32 apps for production scenarios. The latest release is the Windows App SDK 1.0, which you can download and read more about at the documentation linked below:

[Stable release channel for the Windows App SDK](https://docs.microsoft.com/windows/apps/windows-app-sdk/stable-channel)

We also ship experimental features as we develop them. You can read more about the Windows App SDK Experimental releases at the following documentation. Note that Experimental releases have limitations and known issues, so they are not equipped for production apps.

[Experimental release channel for the Windows App SDK](https://docs.microsoft.com/windows/apps/windows-app-sdk/experimental-channel).

### Features by release
Legend:

&#128994; - Available as supported feature

&#128311; - Available as experimental feature  

Blank -  Not available

**WinUI 3 - Windows App SDK 1.0** is our latest release. For a detailed look at the features we're targeting in WinUI 3 in 2022, check out the [Windows App SDK feature roadmap](https://github.com/microsoft/WindowsAppSDK/blob/main/docs/roadmap.md).

| Customer Capability | Project Reunion 0.5 <br/>(March 2021) | Windows App SDK 0.8 <br/>(June 2021) | Windows App SDK 1.0 <br/> (November 2021) |
| ------------------- | -------------------------------- | ------------------------------- | ----------------------------- |
| Supported in any app using the Windows App SDK         | &#128994;          | &#128994;          | &#128994;                       |
| Contains new Windows 11 controls/styles from WinUI 2.6 |                    |                    | &#128994;                       |
| Supports MSIX Deployment                               | &#128994;          | &#128994;          | &#128994;                       |
| Supports Unpackaged (non-MSIX) Deployment              |                    |                    | &#128994;                       |
| Works downlevel to Windows 10 version 1809 and above   | &#128994;          | &#128994;          | &#128994;                       |
| Supports the latest .NET                               | &#128994;          | &#128994;          | &#128994;                       |
| ARM64 support                                          | &#128994;          | &#128994;          | &#128994;                       |
| `<SwapChainPanel> `                                    | &#128994;          | &#128994;          | &#128994;                       |
| Intellisense, Hot Reload, Live Visual Tree             | &#128311;          | &#128311;          | &#128994;[^1]                   |
| Chromium-based WebView2                                | &#128994;          | &#128994;          | &#128994;                       |
| Title bar customization                                | &#128994;          | &#128994;          | &#128994;                       |
| Fluent Shadows                                         | &#128994;          | &#128994;          | &#128994;                       |
| Input validation for data fields                       | &#128311;          | &#128311;          | &#128311;                       |
| Supports multiple top-level windows on the same thread | &#128311;          | &#128311;          | &#128311;                       |
| Support multiple top-level windows on separate threads | &#128311;          | &#128311;          | &#128311;                       |
| Drag and drop                                          | &#128994;          | &#128994;          | &#128994;                       |
| RenderTargetBitmap                                     | &#128994;          | &#128994;          | &#128994;                       |
| Mouse cursor customization                             | &#128994;          | &#128994;          | &#128994;                       |
| Animated Gif support                                   | &#128994;          | &#128994;          | &#128994;                       |
| VirtualSurfaceImageSource (VSIS) support               | &#128994;          | &#128994;          | &#128994;                       |
| In-app acrylic                                         | &#128994;          | &#128994;          | &#128994;                       |
| XAML Islands                                           |                    | &#128311;          | &#128311;                       |

[^1]: Supported in packaged apps only

## WinUI 2 Controls Library

WinUI 2 is a native user experience (UX) framework for both Windows desktop and UWP applications tightly integrated with Windows 10 and above SDKs. 

The next release of the **WinUI 2 Controls Library** will be v2.8, aiming to release in mid-2022.

WinUI 2.8 will include WebView2 & bug, reliability, and security fixes for UWP apps.  You can find a list of currently planned work in the [WinUI 2.8 milestone](https://github.com/microsoft/microsoft-ui-xaml/milestone/14).

For installation instructions see [Getting started with the Windows UI Library](https://docs.microsoft.com/windows/apps/winui/winui2/getting-started).

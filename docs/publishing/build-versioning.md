# Understanding all the Versions in the Xaml Build

## Table of Contents

- [Package configuration](#package-configuration)
- [Package sources](#package-sources)
- [Windows SDK](#windows-sdk)
  - [Targeting RS5](#targeting-rs5)
  - [Light-up Code](#light-up-code)
- [Versions.props](#versionsprops)
- [.Net and cs/winrt versions used by sample apps](#net-and-cswinrt-versions-used-by-sample-apps)

## Package configuration

All of Xaml’s dependencies are listed in package config files or solutions.

> Note: several of the versions in [packages.config](../../packages.config) must be kept in sync with [eng\versions.props](../../eng/versions.props)

This is mostly in the [packages.config](../../packages.config) file:

```xml
<packages>
  <package id="Microsoft.UI.DCPP.Dependencies" version="18362.210128.1" />
  <package id="Microsoft.UI.DCPP.Dependencies.Edge" version="95.0.1020.30" />
  <package id="Microsoft.Windows.SDK.cpp" version="10.0.22621.755" />

  ...
```

There are also per-architecture packages.*.config files. For example packages.x64.config:

```xml
<packages>
  <package id="Microsoft.Windows.SDK.cpp.x64" version="10.0.22621.755" />
</packages>
```

The complete list of all package dependency lists (restore files):
* [packages.config](../../packages.config)
* packages.[arch].config ([x86](../packages.x86.config), [x64](../../packages.x64.config), [arm64](../../packages.arm64.config))
* [controls\dev\dll\packages.config](../../controls/dev/dll/packages.config) (contains reference to Microsoft.Web.WebView2)
* [src\BuildTools.sln](../../src/BuildTools.sln)

There are scripts in the scripts directory to update the reference to IXP and to CSWinRT:
* UpdateIxp (updates packages*.config)
* UpdateCSWinRT (updates packages.config and eng\versions.props)

There is also a script in the scripts directory to update the reference to the WebView2 SDK and Edge version:
* [UpdateWebView2](../../controls/dev/WebView2/WebView2-update.md)
    * Updates SDK reference in [controls\dev\dll\packages.config](../../controls/dev/dll/packages.config) and 
    [eng\versions.props](../../eng/versions.props)
    * Updates Edge version in [packages.config](../../packages.config), 
    [Microsoft.UI.DCPP.Dependencies.Edge.nuspec](../../dxaml/external/Microsoft.UI.DCPP.Dependencies.Edge.nuspec), and 
    [TaefHostAppManaged.csproj](../../dxaml/test/infra/taefhostappmanaged/TaefHostAppManaged.csproj)

## Package sources

The primary package source to build WinUI is `WinUI.Dependencies`, configured in [NuGet.config](../../NuGet.config).
This source has the WinUI package but also references/caches other feeds for other dependent packages, in the form of upstream sources.

The Azdo Upstream Sources feature doesn't support all sources (must be Azdo or public), so the nuget.config
does have some additional sources, and a `packageStore` local directory source to help with inner loop.

## Windows SDK

The Windows SDK can be consumed via the MSI distro (installed under "C:\Program Files (x86)\Windows Kits\...")
or via the Nuget distro (installed under "%RepoRoot%\packages\Microsoft.Windows.SDK.cpp").  Projects may differ
on this technique, but generally, the WinUI repo should be built with the latest version of the Windows SDK.  

### Targeting RS5

While the latest Windows SDK toolset is preferred, all WinUI binaries must target the downlevel limit of the
Windows App SDK: Windows 10 1809 "Redstone 5", corresponding to NTDDI_WIN10_RS5 (0x0A000006), UAP contract v7.0,
and Windows SDK 10.0.17763.0.  By default, all C++ Win32, C++ ABI, C++/WinRT, C#/WinRT, and IDL sources in the
build must target these versions.  Xaml.Cpp.Targets and lightup.targets enforce this, pulling metadata only from
%RepoRoot%\packages\Microsoft.Windows.SDK.Contracts.10.0.17763.1000\ref\netstandard2.0.

### Light-up Code

For light-up scenarios such as the Auto-Hide ScrollBar functionality introduced with 19H1 (10.0.18632.0, UAP v8.0),
projects can define a **XamlLightup** project property to bypass the normal downlevel targeting described above.
Because this property enables code to target metadata from the current Windows SDK, it should be used sparingly,
and only in isolated source files that are clearly identified as light-up code.

## Versions.props

> Note: several of the versions in [eng\versions.props](../../eng/versions.props) must be kept in sync with [packages.config](../../packages.config)

TBD

## .Net and cs/winrt versions used by sample apps

The way a C# app (such as the samples and XCG) picks its version of .Net, of the Windows SDK, 
and of CSWinRT, is by setting the 
[TFM](https://docs.microsoft.com/en-us/dotnet/standard/frameworks) in the project file:

```xml
<TargetFramework>net6.0-windows10.0.18362.0</TargetFramework>
```

The Windows SDK version can be overridden though by FrameworkReference items:

```xml
<ItemGroup>
    <FrameworkReference Update="Microsoft.Windows.SDK.NET.Ref" RuntimeFrameworkVersion="10.0.18362.8-preview" />
    <FrameworkReference Update="Microsoft.Windows.SDK.NET.Ref" TargetingPackVersion="10.0.18362.8-preview" />
</ItemGroup>
```

In the lifted repo, the FrameworkReference override mechanism is used as follows:

* In the root of the lifted repo is a `Directory.Build.Targets` that gets included by all projects in the repo, 
* this imports `eng\sdkconfig.targets`, 
* and this sets the `<FrameworkReference>` overrides to 10.0.`TargetPlatformVersion`.`MicrosoftWindowsSDKNetRefPackVersionSuffixOverride`. 
* That value is defined in [eng\versions.props](../../eng/versions.props), for example

```xml
<MicrosoftWindowsSDKNetRefPackVersionSuffixOverride>8-preview</MicrosoftWindowsSDKNetRefPackVersionSuffixOverride>
```

`Version.props` also set the SDK version for .Net runtime, which is used by the 
[DownloadDotNetCoreSdk.ps1](../../build/DownloadDotNetCoreSdk.ps1) script (which is called by `init.cmd`):

```xml
<DotNetCoreSdkVersion>5.0.100-rc.2.20480.7</DotNetCoreSdkVersion>
```

But that doesn’t guarantee that that’s the .Net that will be used at runtime. To ensure that we specify 
the .Net version a global.json file:

```json
"sdk": {
    "version": "5.0.100-rc.2.20480.7"
},
```

| Now that we're on Net5 GA, how much of an issue will this continue to be?

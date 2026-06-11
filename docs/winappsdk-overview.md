# Windows App SDK and WinUI 3

This document provides an overview of the relationship between Windows App SDK (formerly Project Reunion) and WinUI 3
artifacts, and the processes that connect them.

## Table of Contents

- [WinUI 3 Nuget Package](#winui-3-nuget-package)
  - [WinUI 3 "Transport Package"](#winui-3-transport-package)
  - [Windows App SDK Aggregation](#windows-app-sdk-aggregation)
  - [Windows App SDK Nuget Package](#windows-app-sdk-nuget-package)
  - [Windows App SDK Self-Contained Configuration](#windows-app-sdk-self-contained-configuration)
- [Windows App SDK VSIX](#windows-app-sdk-vsix)
- [See Also](#see-also)

## WinUI 3 Nuget Package

Until WinUI 3 Preview 5, the library was distributed as a standalone nuget package that deposited all binaries directly
into the referencing project's build output. With GA, WinUI 3 is now a component of Windows App SDK, for which it is
considered a ***"feeder repo"***. Distribution is split between build time concerns, via the Windows App SDK nuget
package, and runtime concerns, via the Windows App SDK framework package. The standalone WinUI 3 nuget package is still
being produced for inner loop and testing purposes, but will be decommissioned at some point.  

### WinUI 3 "Transport Package"

The WinUI 3 build now produces a
[Microsoft.WinAppSDK.WinUI.TransportPackage](../build/nuspecs/Microsoft.WinAppSDK.WinUI.TransportPackage.nuspec)
nupkg, which has substantially similar content to the standalone Microsoft.WinUI nupkg. The purpose of the transport
package is to use nuget packaging and publishing solely to convey WinUI 3 artifacts to Windows App SDK. The WinUI 3
build publishes the transport package as a pipeline artifact (under \packaging), and also pushes it to an internal feed
where it is ingested by the WindowsAppSDK aggregation pipeline.

The transport package:

* excludes the MRM and ApplicationModel artifacts included in the Microsoft.WinUI nuget package  
* includes an [AppxManifest.xml](../packaging/AppxManifest.xml)
  file to convey ActivatableClass entries  

### Windows App SDK Aggregation

All transport packages, including WinUI's, are aggregated by the
WindowsAppSDK aggregation pipeline. The version
of each transport package is conveyed in the
[eng/Version.Details.xml](../eng/Version.Details.xml)
file. The aggregation process splits each incoming transport package in two. The ***runtime*** folder contents are
consolidated from all transport packages, and used to produce the Windows App SDK framework package. All other folders
contain strictly build-time artifacts, which are repackaged into corresponding "thin packages", such as
Microsoft.WinAppSDK.WinUI.nupkg.

### Windows App SDK Nuget Package

The user-visible result of the Windows App SDK build is the
[Microsoft.WindowsAppSDK](https://www.nuget.org/packages/Microsoft.WindowsAppSDK) package, which is published to
 nuget.org. (Note: this package was previously a nuget "metapackage", containing dependencies on "thin" packages
 produced from each transport package. Now, all transport packages are merged into a single monolithic package.)

### Windows App SDK Self-Contained Configuration

Some partners have requested the Windows App SDK nuget package to support in-app deployment, as the standalone legacy
WinUI 3 nuget package did. This support is enabled by defining a **WindowsAppSDKSelfContained** project property.  

## See Also

* [Windows App SDK on GitHub](https://github.com/microsoft/WindowsAppSDK)

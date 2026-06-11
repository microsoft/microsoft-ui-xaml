# WinUI Sample Apps

## Table of Contents

- [Building](#building)
  - [Building apps against a WinUI component package or a published Microsoft.WindowsAppSDK nuget package](#building-apps-against-a-winui-component-package-or-a-published-microsoftwindowsappsdk-nuget-package)
- [WinUIGallery](#winuigallery)

We have a number of Sample Apps in this repo under the 'Samples' directory.

The WinUIGallery (XCG) project is maintained in [a public GitHub repo](https://github.com/microsoft/WinUI-Gallery),
where it can be built standalone. It is also included as a submodule in the current repo, under the Samples directory.

There are also a number of simple apps that cover the different types of projects that we ship Visual Studio templates
for (e.g. WinUICsDesktopSampleApp, etc.).

There are some apps under the Samples directory that are not building regularly in the build Pipelines. What is said in
this doc does not apply to them. This doc covers:

* WinUIGallery
* WinUICppDesktopSampleApp
* WinUICsDesktopSampleApp

In addition to the apps themselves, there is also test automation that is used to validate their functionality. See
the Sample App Tests section of [testing-FAQ.md](../testing/testing-FAQ.md) for more details.

## Building

The easiest way to build these apps is to run buildsamples.cmd after doing a full local build (build.cmd)

These apps are configured to be able to build in multiple different ways:

1. Against a WinUI component package containing local build output
2. Against a published Microsoft.WindowsAppSDK nuget package

### Building apps against a WinUI component package or a published Microsoft.WindowsAppSDK nuget package

Customers build WinUI apps by consuming a published Microsoft.WindowsAppSDK package. The sample apps can also be built
against either a locally-produced WinUI component package or a published Microsoft.WindowsAppSDK package.

The WinUI component package embeds the version of the WinUI transport package it contains.

* For local dev builds, the package is always named Microsoft.WindowsAppSDK.999.0.0-mock-3.0.0-dev.nupkg
* For pipeline builds, the package will be named something like Microsoft.WindowsAppSDK.999.0.0-mock-3.0.0-zmain.230101.1-CI.nupkg

In any case, the procedure is the same, supplying the package version to the buildSample.cmd script.

**WinUI component package from a local build:**

```shell
init.cmd 
scripts\buildSample WinUIGallery 999.0.0-mock-3.0.0-dev
```

**WinUI component package from a pipeline run:**

```shell
init.cmd 
scripts\buildSample WinUIGallery 999.0.0-mock-3.0.0-zmain.230101.1-CI
```

When building the Sample Apps with this method, you do not need to build WinUI locally.
You can get a WinUI component package from the CI or Nightly Pipelines.

The WinUI-CI and WinUI-Nightly pipelines use this approach to build the Sample Apps.

**Published WindowsAppSDK:**

```shell
init.cmd 
scripts\buildSample WinUIGallery 1.2-stable
```

There are also switches to create unpackaged or self-contained apps, for example

```shell
scripts\buildSample WinUIGallery 1.2-stable -unpackaged -selfcontained
```

Note, there is no guarantee that a particular version of WindowsAppSDK is in sync with the WinUI repo, so a matching
version must be chosen.

The WinUI-ValidateWindowsAppSDK pipeline uses this approach to build the Sample Apps. This pipeline currently is only run manually.

## WinUIGallery

As mentioned, the WinUIGallery is both a [standalone public repo](https://github.com/microsoft/WinUI-Gallery/),
as well as a submodule in this repo.

WinUIGallery.slnx builds WinUIGallery as a Desktop app with Single-project MSIX Packaging support.

As mentioned above, the default way to build this is to build it against your local build with buildsamples.cmd.

Like other samples, it also supports using a NuGet instead of a local build.  Just modify the WinUITransportPackageVersion
property in [Directory.Build.props](../../Samples/Directory.Build.props) under the Samples folder.

# Developer Guide

This guide provides instructions on how to build the repo and implement 
improvements.

## Source code structure

#### /build, /tools

These folders contain scripts and other support machinery that you shouldn't 
need to edit for most changes.

In particular:

* **/build/NuSpecs** enables .nupkg generation
* **/build/FrameworkPackage** enables .appx generation

Note that here and in various parts of the codebase you will see references to 
`BUILD_WINDOWS`. WinUI operates as a standalone package for Xaml apps but is 
also a way that new controls migrate into [Windows.UI.Xaml.Controls](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls) 
as part of the Windows build system. The places where the WinUI source needs to 
differ for this different environment are specified under `BUILD_WINDOWS`. It's 
expected that it is the responsibility of the Microsoft team members to 
maintain this part of WinUI, and other community members should be able to 
ignore it.

#### /dev

Under dev is a separate folder for each of our controls.

Each control is composed of a Shared Item using the Shared Item Template in 
Visual Studio. It is then included into the respective projects. This gives us 
flexibility in the future if we need to decompose our other projects into 
smaller projects, move to a Git submodule model on a per control basis, or 
create different DLLs of our solution. Currently the project is small enough 
that the Shared Item Template gives us enough flexibility to add/remove 
controls to/from the subsequent projects easily.

Also under dev is the actual Microsoft.UI.Xaml project, which is the main DLL 
that contains all the controls and other solutions which will be packaged and 
deployed. At this time we believe the Microsoft.UI.Xaml.dll is 
small enough to include all controls into one DLL. As we increase the number of 
controls we will revisit this decision and may decompose it into different DLLs 
in the future. Also we will adjust based on developer feedback if we start to see 
usage patterns where teams use just a few controls vs. the whole library.

This project also includes the necessary definitions to package the DLL into a 
NuGet package.

#### /docs

This is where the repo documentation lives, including this document.

Note that developer usage documentation can be found separately on docs.microsoft.com.

#### /test

Our test library and test app (the app that the test library interacts with 
when executing the tests) are here.

MUXControls.Test is a [MSTest](https://docs.microsoft.com/dotnet/api/microsoft.visualstudio.testtools.unittesting) 
DLL using MITALite that contains all of the test code for the various controls 
by automating the MUXControlsTestApp.

MUXControlsTestApp is a UWP app that exercises all the controls. This is just a 
manual testing playground which can be driven by the automated tests for 
automated verification as well as [TestMethod] control API verification. Note 
this applications references the MUXControls DLL rather than including the 
Shared Items.

## Code style and conventions

* C++: [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md)
* C#: Follow the .NET Core team's [C# coding style](https://github.com/dotnet/corefx/blob/master/Documentation/coding-guidelines/coding-style.md)

For all languages respect the [.editorconfig](https://editorconfig.org/) file 
specified in the source tree. Many IDEs natively support this or can with a 
plugin.

### File headers

The following file header is the used for WinUI. Please use it for new files.

#### C++/C#
```
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
```

#### XAML/proj

```
<!-- Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project root for license information. -->
```

### C++/WinRT

## Building the repository

Generally you will want to set your configuration to **Debug**, **x86**, and 
select **MUXControlsTestApp** as your startup project in Visual Studio.

### Prerequisites
#### Visual Studio

Install latest VS2017 (15.9 or later) from here: http://visualstudio.com/downloads

#### SDK

While WinUI is designed to work against many versions of Windows, you will need 
a fairly recent SDK in order to build WinUI. It's required that you install the 
16299, 17134 and 17763 SDKs. You can download these from here: https://developer.microsoft.com/en-US/windows/downloads/windows-10-sdk

### Creating a NuGet package

> More information will be coming on this soon

## Testing

### Validating UI and accessibility requirements

> More information will be coming on this soon

### Manual testing

#### Test app

The WinUI solution project has a test app that is useful for validating common 
scenarios affected by the presence of the WinUI package.  Set 
**MUXControlsTestApp** as your startup project in Visual Studio and you can F5 
to start debugging that app and validate your changes.

##### Organization

MUXControlsTestApp is a no frills test app built for on demand developer 
scenarios and also to be automation friendly. As such it's perhaps not as 
friendly to navigate as something similarly control-centric like the 
[Xaml Controls Gallery](https://github.com/Microsoft/Xaml-Controls-Gallery/). 
That's a great potential area for future improvement, although 
MUXControlsTestApp must continue to function as an automation test target. 

#### Standalone app targeting custom NuGet package

In the end developers will consume WinUI as a NuGet package in their own apps. 
So it's important to keep in mind that scenario when validating changes.

To enable **automated** NuGet package testing there is a separate solution at 
test\MUXControlsReleaseTest\MUXControlsReleaseTest.sln, which contains
MUXControls.ReleaseTest, NugetPackageTestApp (C#) and NugetPackageTestAppCX 
(C++).

Test classes for this are in MUXControls.ReleaseTest, and they share test 
infrastructure with MUX so you can write tests in the same way as in MUX. 
The only difference is you’ll have to specify the TestType in ClassInitialize 
and TestCleanup (TestType.Nuget for NugetPackageTestApp and TestType.NugetCX 
for NugetPackageTestAppCX). 
```
public static void ClassInitialize(TestContext testContext)
{
    TestEnvironment.Initialize(testContext, TestType.Nuget);
}

public void TestCleanup()
{
    TestEnvironment.AssemblyCleanup(TestType.Nuget);
}
```
The test apps are using released versions of MUX NuGet package locally. In CI, 
the test pipeline will generate a NuGet package for each build, and there’s a 
separate pipeline configured to consume the generated package from latest 
build and run MUXControl.ReleaseTest.

#### Downlevel testing

One of the core values of WinUI is the way that it brings controls to a wide 
variety of versions of Windows, handling version compatibility differences so 
that developers using WinUI don't have to. As such, testing WinUI changes on 
different versions is sometimes necessary. To accomplish this you will need to 
at times set up older versions of Windows for testing. To get these earlier 
versions you can make use of a Visual Studio subscription [as described here](https://docs.microsoft.com/azure/virtual-machines/windows/client-images).

### Automated testing

You can run the test suite from within Visual Studio by using the Test top 
level menu. For targeting indivual tests you can use [Test Explorer](https://docs.microsoft.com/en-us/visualstudio/test/run-unit-tests-with-test-explorer?view=vs-2017) 
(found under the Test->Windows sub menu).

This same suite of tests will be run as part of your Pull Request validation 
[check](contribution_workflow.md#Checks).

#### Creating a new test

For your test to be discovered it needs to be a method tagged as [\[TestMethod\]](https://docs.microsoft.com/en-us/dotnet/api/microsoft.visualstudio.testtools.unittesting.testmethodattribute?view=mstest-net-1.2.0) 
on a class tagged with as [\[TestClass\]](https://docs.microsoft.com/en-us/dotnet/api/microsoft.visualstudio.testtools.unittesting.testclassattribute?view=mstest-net-1.2.0). 
With that metadata in place your new test method will be picked up by Test 
Explorer.

There are two types of tests you can use to validate your scenarios:
* **API Tests**: Run in the context of an app and validate the behaviors of our 
APIs. 
* **Interaction Tests**: Drive the UI on an external app and validate the 
results using UI Automation.

Keep in mind that your test will be executed on many different versions of 
Windows, not just the most recent version. Your tests may need version or 
[IsApiPresent](https://docs.microsoft.com/en-us/uwp/api/windows.foundation.metadata.apiinformation.istypepresent) 
checks in order to pass on all versions.

## Telemetry

This project collects usage data and sends it to Microsoft to help improve our 
products and services.

If desired you can disable logging when building the project by following these 
steps:

1. In Microsoft Visual Studio's Solution Explorer window, right-click the 
"Microsoft.UI.Xaml (Universal Windows)" project. 
2. Select the "Properties" menu.
3. Select "All Configurations" in the Configuration dropdown.
4. Select "All Platforms" in the Platform dropdown.
5. Select "Configuration Properties", then "C/C++", then "Preprocessor" in the 
left tree structure.
6. In the entry called "Preprocessor Definitions":
    * Add "DISABLE_TELEMETRY_TRACELOGGING;" to disable Microsoft telemetry 
    logging alone. 
    * Add "DISABLE_PERF_TRACELOGGING;" to disable performance logging alone.
    * Add "DISABLE_DEBUG_TRACELOGGING;" to disable debug logging alone.
    * Or simply add "DISABLE_ALL_TRACELOGGING;" to disable all three types of logging.
7. Click the "Apply" button.
8. Recompile the project.

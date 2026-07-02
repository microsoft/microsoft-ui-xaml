# Test Code in WinUI

## Table of Contents

- [Presentation Recording](#presentation-recording)
- [High-level Overview](#high-level-overview)
- [TAEF](#taef)
- [MUX.dll tests - dxaml\test](#muxdll-tests---dxamltest)
  - [Packaged. Runs using TAEF's RunAs:UAP feature:](#packaged-runs-using-taefs-runasuap-feature)
  - [UWP or WPF host](#uwp-or-wpf-host)
  - [Host app](#host-app)
  - [Win32Explicit](#win32explicit)
  - [Test Infra: "Private.Infrastructure"](#test-infra-privateinfrastructure)
  - [Client component](#client-component)
  - [Server Component](#server-component)
  - [Input injection](#input-injection)
  - [IXamlTestHooks](#ixamltesthooks)
  - [C++ Tests are written in C++/CX](#c-tests-are-written-in-ccx)
  - [Limitations](#limitations)
  - [Debugging](#debugging)
- [MUXC.dll tests - controls\test](#muxcdll-tests---controlstest)
  - [MUXControlsTestApp](#muxcontrolstestapp)
  - [Interaction Test vs API Test](#interaction-test-vs-api-test)
    - [API Tests](#api-tests)
    - [Interaction Tests](#interaction-tests)
    - [UIAutomation](#uiautomation)
    - [MITA/Microsoft.Windows.Apps.Test](#mitamicrosoftwindowsappstest)
  - [MUXTestInfra/AppTestAutomationHelpers](#muxtestinfraapptestautomationhelpers)
    - [MUXTestInfra](#muxtestinfra)
    - [AppTestAutomationHelpers](#apptestautomationhelpers)
  - [Debugging](#debugging-1)
  - [Tips](#tips)
- [Sample App tests](#sample-app-tests)
  - [Goals](#goals)
  - [MUXTestInfra](#muxtestinfra-1)
  - [Building Sample Apps](#building-sample-apps)
- [WindowsAppSDK Aggregator Tests](#windowsappsdk-aggregator-tests)

## Presentation Recording

There was a presentation given that covers some of the content in this doc.

## High-level Overview

There are three main 'buckets' of tests in the WinUI repo:
1. MUX.dll tests (under `dxaml\test` dir)
2. MUXC.dll tests (under `controls` dir)
3. Scenario Tests (Sample App tests under `Samples` dir)

We also have a set of integration tests that exists outside of the WinUI repo in the
WindowsAppSDK aggregator repo.

More details on these sets of tests are below.

## TAEF

All of these tests are executed via TAEF. More info on TAEF can be found at https://learn.microsoft.com/en-us/windows-hardware/drivers/taef.

## MUX.dll tests - dxaml\test

This test code originated from the OS repo (along with the bulk of the product code).

This is the main bulk of tests that cover `Microsoft.UI.Xaml.dll` and `Microsoft.UI.Xaml.Phone.dll`.

We have test code both in C++ and C# (but see [below](#c-tests-are-written-in-ccx) for caveats on C++).

The test code executes in a heavily customized environment. For example, most tests do not use the Frame/Page navigation
model. Most tests do not use the "xaml page + code behind" model. And there are some other differences in how the
runtime executes. But it is a good environment for executing tests.

### Packaged. Runs using TAEF's RunAs:UAP feature:

The MUX tests always run as a packaged app. Either a UWP app or in the case of the WPF host, as a packaged Centennial app.

TAEF is able to handle packaged tests. We use TAEF's **RunAs:UAP** feature.

We do NOT create an .appx/.msix for the tests. The test package is deployed as loose files as a sparse package
(similar to F5 from Visual Studio). We place the loose files in a directory containing an AppxManifest.xml file with the
same layout as an installed app would have. The test gets executed from there.

The test package uses one of the appxmanifests from:
`dxaml\test\packages\appx\`
The specific manifest used depends on the test metadata (specifically `UAP:AppXManifest` property).

### UWP or WPF host

The tests can execute in either a UWP or a WPF host. The WPF host consists of a single Xaml Island contained inside a
WPF application.

`Hosting:Mode` allows a test to declare which hosting mode to use and can be set to either `"UAP"`, or `"WPF"`. 
There is also the option of `"Win32Explicit"` for Xaml islands tests.

> Note: "UAP" is an old name for "UWP".

> Note: `"Hosting:Mode"` is not something that is built into TAEF or that it understands. This is custom metadata that
> we use when creating queries to execute tests.

> Note: WinUI3 no longer supports UWP mode. But it can be re-enabled with the `EnableUWPWindow` registry key. This is not something that is publicly supported.

By default, when a test is executed in TAEF, we will use the UAP host.
To run a test in the WPF host, pass `/p:HostingMode=WPF` as an argument to `te.exe`.
The `/p` switch to `te.exe` allows you to pass extra runtime parameters to the test code from the command line.

> Note: `/p:HostingMode=WPF` is NOT something that TAEF understands. It is something that our test infrastructure
> consumes at runtime that controls whether it creates a UAP host or a WPF host.

So there are two pieces to this, the `Hosting:Mode` test metadata which allows a test to declare which hosting modes it
supports and the `/p:HostingMode` runtime parameter to te.exe which controls which host to run in.

### Host app

The test code could get executed in one of three possible processes:
* WPF: `te.processhost.exe` (built into TAEF)
* UWP: (our own custom host app)
    * C++: `taefhostapp.exe`
        `dxaml\test\infra\taefhostapp`
    * C#: `TaefHostAppManaged.exe`
        `dxaml\test\infra\taefhostappmanaged`

### Win32Explicit

Most Xaml tests test the internals of Xaml. These tests have pretty simple requirements as far as the test host goes -
give the test a place to attach a Xaml element and the test can go from there. TestServices takes care of all this
setup to make things convenient for Xaml tests. During `TestServicesStatics::InitializeHost` it creates a test host
(UWP, WPF, or WinForms), sets up Xaml by putting in a temporary tree, and saves the DispatcherQueue so that the test can run
code on the UI thread.

However, this convenience becomes a limitation for Xaml island tests. Island tests need to set up multiple UI threads,
windows, and DesktopWindowXamlSources. These tests use the `Win32Explicit` hosting mode, and they explicitly create
their own hosting environments. Note that `Win32Explicit` isn't interoperable with `UAP` or `WPF`. A test written for
other hosting modes won't have code to set up its DWXS, and can't run in `Win32Explicit`.

### Test Infra: "Private.Infrastructure"

TAEF executes the test, but we also have a set of libraries that constitute a test infrastructure for the MUX tests.
This infra is responsible for creating and destroying the Xaml host and providing useful services to the test code
(such as input injection). The code is all under: [`dxaml\test\infra`](../../dxaml/test/infra).

### Client component

The main component of the test infra is the client component:
* **Implementation:** Private.Infrastructure.Client.dll
* **WinRT interface:** Private.Infrastructure.winmd
* **CsWinRT projection:** Private.Infrastructure.CsWinRT.dll

Creates execution environment for the test code.

Provides test helper functions such as:
* [Input Injection](#input-injection):
  * InputHelper: Mouse/Pen/Touch
  * KeyboardHelper: Text input
  * The implementation of this depends on `InputManagerXaml.dll` from a
    separate internal repo
* UIElement Tree XML dump
* DComp tree xml dump
* WindowHelper
* WaitForIdle

> Tip: Open Private.Infrastructure.winmd in [ILSpy](https://github.com/icsharpcode/ILSpy) to see all available functions.

Most private API usage has been removed from Private.Infrastructure. It depends on an internal package for input injection.

### Server Component

Most of the test infra executes in the main test process. However, for some things we want to execute in a different
process (e.g. for permissions purposes). For example, input injection. For this, we have a server component that the
client can execute RPC calls to. This runs in a separate `te.processhost.exe` process and runs outside the context of
the app package.

This is mostly hidden from the test code itself. The client library provides an API to the test code and the RPC call
to the server component is an implementation detail.

Code is under: [`dxaml\test\infra\server\`](../../dxaml/test/infra/server)

This architecture was originally created with UWP in mind, since UWP processes run in a sandbox and so are prevented from
doing many things that a regular full-trust process can do.

### Input injection

Xaml gets its pointer input injection functionality from `InputManagerXaml.dll` methods like
`IMInjectPress`,
and keyboard injection is done with
`IMInjectKeyInput`.

The client side call stack looks like:
```
Private_Infrastructure_Client!RpcInjectPress
Private_Infrastructure_Client!Private::Infrastructure::InputHelper::TapOnPoint
Private_Infrastructure_Client!Private::Infrastructure::InputHelper::TapAtPercent+0xf7
Private_Infrastructure_Client!Private::Infrastructure::InputHelper::Tap+0x4e
Microsoft_UI_Xaml_Tests_External_Foundation!Private::Infrastructure::IInputHelper::Tap+0x3e
Microsoft_UI_Xaml_Tests_External_Foundation!Microsoft::UI::Xaml::Tests::Foundation::Input::Pointer::BasicPointerTests::TouchFuzzyHitTest+0x66a
[more test code]
```

And the server side looks like:
```
InputManagerXaml!IMInjectPress+0x25
Private_Infrastructure_Server!Microsoft::UI::Xaml::Tests::Common::InputRoutineHelper::InjectPress+0x337
Private_Infrastructure_Server!RpcInjectPress+0xb5
RPCRT4!Invoke+0x73
[more RPC code]
```

Note that the client side relies in InputHelper, which isn't available when running in `Win32Explicit` mode, but the
server side of input injection still runs. We can take advantage of this to call directly to the server to do input
injection in island tests.

### IXamlTestHooks

Most of the test code is based on testing the public API surface of the product code. However, occasionally it is
necessary to call into private test hooks in the product code. This is done via the `IXamlTestHooks` interface which is
not a part of WinUI's public API, but is available to test code.

Definition: [`dxaml/xcp/inc/IXamlTestHooks-win.h`](../../dxaml/xcp/inc/IXamlTestHooks-win.h)

### C++ Tests are written in C++/CX

Most of the MUX tests are in C++. More specifically, they are written in a Microsoft extension of C++ called
[C++/CX](https://docs.microsoft.com/en-us/cpp/cppcx/visual-c-language-reference-c-cx?view=msvc-170)
which includes functionality to directly call WinRT APIs from C++ code. C++/CX has been deprecated in favor of C++/WinRT
(which is a library/toolchain that can be used with standard C++). C++/CX is not officially supported for WinAppSDK, but
it still works and our test code has not yet been ported over to C++/WinRT.

### Limitations

* No regular 'Win32' host
Right now, we run the tests either in a UWP host or a WPF-Xaml-Islands host. We do not have a non-islands Win32 host.

* C# tests ONLY run in WPF host.
It is not currently possible to run the C# tests in the UWP host. They only run in the WPF host. This is because the
managed UWP host is based on .net native, but the test code is compiled as .net 5.

* Many tests are running in UWP mode even though this is not supported. See [uap-tests.md](uap-tests.md) for more details.

### Debugging

To aid debugging, Private.Infrastructure supports a runtime flag that can be passed to TAEF: `/p:WaitForDebugger`. This
will cause the test host to wait until a debugger has been attached before executing.


## MUXC.dll tests - controls\test

There is a separate set of tests under the `controls\test` dir. These are tests that target
`Microsoft.UI.Xaml.Controls.dll`.

These tests do not use Private.Infrastructure to run. Instead they have their own set of test helpers. The reason for
the bifurcation is that everything under the 'controls' dir originally came from a separate repo that is on GitHub
(https://github.com/microsoft/microsoft-ui-xaml). Since the 'controls' dll went OSS before everything else, it could
only rely on public technologies and so was unable to use Private.Infrastructure.

### MUXControlsTestApp

The MUXC tests are all hosted in this test app. This test app can be run as a manual test app and is also used by test
automation.

This test is a standard Win32 Desktop WinUI app. In contrast to the MUX tests, it does not run as a UWP app nor as a
WPF-xaml-islands app.

### Interaction Test vs API Test

There Two types of MUXC Test: **Interaction Tests** and **API Tests**.

#### API Tests

Tests are compiled directly into MUXControlsTestApp.

They are executed by pointing TAEF directly at the .appx file:

```
te.exe MUXControlsTestApp.appx
```

(This might be unexpected to some, since TAEF is usually pointed to a .dll rather than to an appx.)

TAEF installs the app, launches it, executes the test, and uninstalls the app when it is done. This functionality is all
built in to TAEF.

The test code executes in the app process (`MUXControlsTestApp.exe`).

This model of test execution is fairly similar to the MUX tests, with the difference being that we are using a real appx
for the app, instead of a 'sparse' package of loose files.

#### Interaction Tests

These tests work quite differently to the API Tests or the MUX.dll tests.

In this style of tests, the test code is in its own process and it simulates the actions of user. It launches the test
app and interacts with it. The test code does not call directly into any WinUI code (WinUI dlls are not even loaded into
the test process).

**Test Code:** MUXControls.Test.dll (loaded by TAEF in `te.processhost.exe`)
**Code under test:** `MUXControlsTestApp.exe`

#### UIAutomation

The test interacts with the app via UIAutomation.
https://docs.microsoft.com/en-us/windows/win32/winauto/uiauto-uiautomationoverview

UIA is a Microsoft technology that allows programmatic access to UI. It provides an API that is neutral with respect to
the specific UI framework being targeted (e.g. Win32, WPF, Xaml, HTML, etc.). The two main uses of UIA are for test
automation and for accessibility tools such as Narrator.

#### MITA/Microsoft.Windows.Apps.Test

MITA is a library of test helpers that provides an abstraction over UIA to make it more convenient to use in the context
of writing tests. It also provides input injection functionality.

**Microsoft.Windows.Apps.Test** is a public variant of MITA:
https://github.com/microsoft/Microsoft.Windows.Apps.Test

**Remember:**, a `UIObject` such as `Button` in the test code is a MITA object, it is NOT a Xaml object, so you cannot
call Xaml APIs on it. The MITA object represents a UIElement in a different process (the test app).

### MUXTestInfra/AppTestAutomationHelpers

Two 'infrastructure' components to ease test authoring.

#### MUXTestInfra

Used by the test-process. Provides functionality to:
* Install and launch test app
* Navigate test app to test page
* WaitForIdle
* Helpers to find UIObjects

#### AppTestAutomationHelpers

Used by the test-app.
MUXTestInfra communicates with the test app via 'hidden' UIElements.
E.g. there is a CheckBox named 'IdleStateEnteredCheckBox' that gets checked when the app goes idle. The test process can
monitor the state of this checkbox to determine when to continue test execution after a call to WaitForIdle.

These are shared outside of the repo for partner teams:
* The WindowsAppSDK aggregator repo (see below)

### Debugging

With the MUXC interaction tests, there are two processes at play during test execution: there is the test process that
is executing the test code (`te.processhost.exe`) and there is the app process which has the UI and which has loaded Xaml
(`MUXControlsTestApp.exe`). Since there are two processes, when debugging you need to know which process you want to
attach to. The MUXC test infra supports two flags to allow test execution to wait until a debugger has been attached to
either the test process or the app process. These are `/p:WaitForDebugger` and `/p:WaitForAppDebugger` respectively.

### Tips

When writing an interaction test, it is useful to be able to look at the UIA tree of the test app so that you know what
things are available to you. A tool that is useful for this is Inspect.exe. From a Visual Studio Developer Command
prompt, run `inspect.exe` and it will launch. You can use this to view the entire UIA tree of every app running on your
machine.


## Sample App tests

Sample Apps are under 'Samples' dir

Test automation for:
* WinUIGallery
* WinUICppDesktopSampleApp
* WinUICsDesktopSampleApp

There are some other apps in this directory that do not have test automation associated with them.

Test automation code for these apps lives under
[controls\test\MUXControls.Test\ScenarioAppTests](../../controls/test/MuxControls.Test/ScenarioAppTests)
and compiles into `MUXControls.Test.dll`.
(Not really the most appropriate location for these tests. But it is where they live for now).

### Goals

The goal of these sample app tests is to have the apps consume WinUI the same way as a customer would and so that we
execute similar to the real world scenario that a customer would experience.

This is required because we were regularly hitting bugs that were not caught by the main functional suite of tests due
to the fact that these tests do not consume WinUI the way a customer would.

When WinUI was its own independent product that shipped as a nuget package, these apps built against that nuget package
and so were exercising the true customer scenario.

Now that WinUI is a part of the larger WindowsAppSDK these apps instead consume the local WinUI component package that is built
from the lifted xaml repo. This is not idential to the public WinAppSDK nuget package, but it is close.

So these sample app tests are no longer exercising true customer scenarios. But they still provide
value since the scenario is very close to the real world scenario. 
(See [WindowsAppSDK Aggregator Tests](#windowsappsdk-aggregator-tests) below for more info on true customer
scenario tests).

### MUXTestInfra

This set of tests re-uses the same set of helper libraries as the MUXC tests as described above.

### Building Sample Apps

More info on building the sample apps can be found here: [building-sample-apps.md](../building/building-sample-apps.md)


## WindowsAppSDK Aggregator Tests

The WindowsAppSDK aggregator repo is where
we produce the final bits of WinAppSDK that get shipped to customers. This includes the WinAppSDK framework package and
the nuget packages. All of the different components of WASDK feed into this repo (IXP, MRTCore, DWrite, WinUI, etc.).

Since this is the repo where the final product bits are assembled, it is the first opportunity that we have to test the
true customer scenario.

We have a set of test apps:
* WindowsAppSDKCsApp (C#)
* WindowsAppSDKCppApp (C++)

These compile against the Microsoft.WindowsAppSDK nuget package and run using the WASDK framework package.

We have a set of test automation for these apps:
* WindowsAppSDKCsAppTests
* WindowsAppSDKCppAppTests

These tests follow the same model as the MUXC Interaction tests and the WinUI Sample App tests.
They consume the MUXTestInfra nuget packages that are built in the WinUI repo.

These tests run for every PR into the aggregator repo and prevent an update in one of the feeder repos (e.g. MRTCore)
from breaking WinUI scenarios. This happens as part of the
WinAppSDK-Build-PR Pipeline.
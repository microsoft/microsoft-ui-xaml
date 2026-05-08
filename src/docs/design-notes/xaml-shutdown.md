# Xaml Shutdown

## Table of Contents

- [Overview](#overview)
- [Background](#background)
  - [App Lifecycle](#app-lifecycle)
  - [Desktop app Lifecycle](#desktop-app-lifecycle)
  - [Shutdown changes in WinAppSDK 1.5](#shutdown-changes-in-winappsdk-15)
- [Problems in System XAML Islands Lifetime / Shutdown](#problems-in-system-xaml-islands-lifetime--shutdown)
  - [First WindowsXamlManager on a thread must be the last one destroyed: FIXED](#first-windowsxamlmanager-on-a-thread-must-be-the-last-one-destroyed-fixed)
  - [WindowsXamlManager shutdown is async: MITIGATED by requiring DispatcherQueue](#windowsxamlmanager-shutdown-is-async-mitigated-by-requiring-dispatcherqueue)
  - [Thread poisoning: FIXED FOR XAML](#thread-poisoning-fixed-for-xaml)
  - [Process poisoning: Xaml cannot be restart in a process: NOT FIXED](#process-poisoning-xaml-cannot-be-restart-in-a-process-not-fixed)
  - [Application lifetime: FIXED](#application-lifetime-fixed)
  - [Application / WindowsXamlManager startup entanglement: NOT FIXED](#application--windowsxamlmanager-startup-entanglement-not-fixed)
- [FAQ](#faq)
- [Open Issues](#open-issues)
  - [API changes for Application and WindowsXamlManager objects](#api-changes-for-application-and-windowsxamlmanager-objects)
  - [Fake XAML shutdown during test runs](#fake-xaml-shutdown-during-test-runs)
  - [Leak Testing](#leak-testing)
  - [Microsoft.ui.xaml.dll can't unload quite yet](#microsoftuixamldll-cant-unload-quite-yet)
  - [Specific Scenarios](#specific-scenarios)
  - [What is the value-add here for the end developer?](#what-is-the-value-add-here-for-the-end-developer)

## Overview
This document describes how the Xaml framework shuts down.

Goals:
* Make it easy for callers to clean up XAML on a thread without incremental / per-instance leak.  (more info about this
in the section below about "Leak Testing")
* Integrate with organized shutdown plans where appropriate.  

Non-Goals / not-yet Goals:
* We don't support fully unloading microsoft.ui.xaml.dll and other WinUI3 DLLs.
* We don't promise that memory use will be net zero after using XAML and then running it down.

## Background
There are two kinds of WinUI apps relevant to this document:
* **WinUI Desktop Application** -- in this app mode, XAML owns and manages the top-level window, as well as the message
pump.  The app is calling **Application.Start** to run the app.
* **Islands Application** -- in this mode, the app (or some other framework) owns the windows and the message pumps,
and XAML is used for some subset of the GUI.  The app is using the **DesktopWindowXamlSource** API or the **XamlIsland** API.

When an app uses Xaml Islands, there are three key objects involved:
* **Application** -- the app typically creates a type derived from `Microsoft.UI.Xaml.Application`, usually called "App"
in the templates.
  * It's necessary to create a custom one for the tooling to hook up `IXamlMetadataProvider` correctly.  If an App doesn't
 supply its own Application object, XAML will use the default one.  When creating a custom Application object, the app
 must create it before the first `WindowsXamlManager` is instantiated, becuase the `WindowsXamlManager` creates a
 default Application object if there's not already one available.
  * The Application object is process-global, there can only be one in the process.
  * The threading model of this object is unusual.  Many of its APIs will operate on whatever DXamlCore is on the
  current thread.
* **WindowsXamlManager** -- represents the instance of the XAML runtime on that thread
  * As of WinAppSDK 1.5, there's zero or one WindowsXamlManager objects per thread, and Xaml stays active until
  the DispatcherQueue shutdown process.  WindowsXamlManager.Close is now a no-op.
* **XamlIsland** -- new in WinAppSDK 1.7, an instance of this is a "Xaml Island", a rectangle of Xaml content you
can put inside another framework.  It replaces **DesktopWindowXamlSource**
  * It implicitly creates a **WindowsXamlManager**.
* **DesktopWindowXamlSource** -- this is a Xaml Island that can only be connected to a child HWND.  The **XamlIsland**
type replaces **DesktopWindowXamlSource** because it is more general.
  * It has a method called `Initialize` that allows the app to attach it to an HWND

### App Lifecycle
Here's a rough timeline of object lifetimes as an app creates and cleans up Xaml Islands:

* App calls DispatcherQueueController.CreateOnCurrentThread()
* App creates custom Application object
  * App ctor
    * App creates initial WindowsXamlManager
      * DXamlCore::Initialize()
* Message Pump
  * App creates XamlIsland
    * CXamlIslandRoot::Initialize()
      * InteropCompositor created
      * ContentIsland created
  * App calls XamlIsland.Initialize()
  * App calls XamlIsland.Close()
* App calls DispatcherQueueController.ShutdownQueue()
  * DispatcherQueue.ShutdownStarting fires
    * (Application performs its own cleanup)
  * DispatcherQueue.FrameworkShutdownStarting fires
    * Xaml closes all outstanding `XamlIsland` objects
      * Results in related `ContentIslands` and related objects being closed
    * Xaml drains its remaining async work.  (this includes firing pending UIElement.Unloaded messages).
    * Xaml cleans up its per-thread state (Xaml's TLS slot is cleared, so DXamlCore::GetCurrent will fail.)
  * DispatcherQueue.PlatformShutdownStarting fires
    * Any active WinAppSDK platform object closes itself, including the Compositor, islands, and input objects

### Desktop app Lifecycle
In a WinUI Desktop app, the app creates a Xaml Window object, which owns and maintains a top-level window with a
DesktopWindowXamlSource inside it, which itself configures a XamlIsland.  In a WinUI Desktop app built using the default template, the timeline looks like
this:

* App (generated wWinMain) calls Application.Start()
  * microsoft_ui_xaml!FrameworkApplication::StartDesktop
    * Xaml calls DispatcherQueueController.CreateOnCurrentThread()
    * Xaml calls Application.Start() callback, where user's App is created
    * Xaml creates WindowsXamlManager
      * DXamlCore initialized
      * Xaml raises Application.OnLaunched
        * App creates Window
          * Window creates DesktopWindowXamlSource, attaches as child window
            * Under the hood, DesktopWindowXamlSource creates a XamlIsland to do this work
    * Xaml runs message loop
      * Eventually, the final Xaml Window closes
        * Window.Close is raised
        * Xaml closes the internal DesktopWindowXamlSource and XamlIsland object owned by the final Xaml Window
          * Results in the ChildSiteBridge, ContentIsland, and related objects being closed
      * App runs until message loop exits (WM_QUIT, etc)
    * Xaml calls DispatcherQueueController.ShutdownQueue()
      * DispatcherQueue.ShutdownStarting fires
        * (Application performs its own cleanup)
      * DispatcherQueue.FrameworkShutdownStarting fires
        * Xaml closes all outstanding XamlIsland objects explicitly created by the app (the "internal" one was already closed)
          * Results in the ChildSiteBridge, ContentIsland, and related objects being closed
        * Xaml drains its remaining async work.  (this includes firing pending UIElement.Unloaded messages).
        * Xaml calls ShutdownAllPeers(), which disconnects Xaml objects and often breaks ref graphs, often
        triggering Release() calls to app objects and destructors.
        * Xaml cleans up its per-thread state (Xaml's TLS slot is cleared, so DXamlCore::GetCurrent will fail.)
        * Xaml releases its ref to the app's App object (typically the last one)
          * App's App::~App called
            * App::window is cleared (typically this is the last ref on the window object)
              * App's Window::~Window called
        * Xaml raises the XamlShutdownCompletedOnThread event.  This signals to the app that Xaml is now finished on the thread.
      * DispatcherQueue.PlatformShutdownStarting fires
        * Any active WinAppSDK platform object closes itself, including the Compositor, islands, and input objects

### Shutdown changes in WinAppSDK 1.5
In WinAppSDK 1.5 we made a change to keep Xaml running on the thread until the DispatcherQueue shuts down.  

We've still kept the previous codepath alive.  See the private API **IFrameworkApplicationPrivate.ShutdownModel** -- when this
property is set to **Version1**, Xaml uses the old model: when all the WindowsXamlManager and DesktopWindowXamlSource objects
on the thread are closed, the runtime shuts down asynchronously.


## Problems in System XAML Islands Lifetime / Shutdown
See [xaml-islands](xaml-islands.md) for more background on some of these issues.

Below is a list of some of the biggest Xaml Islands problems and our progress/plans for them.

### First WindowsXamlManager on a thread must be the last one destroyed: FIXED
This is fixed in both System XAML and Lifted XAML now.  `WindowsXamlManager` now maintains a per-thread count and a
per-process count, and doesn't clean up XAML on the thread until the per-thread count goes to zero.

This was fixed in WinUI3.

### WindowsXamlManager shutdown is async: MITIGATED by requiring DispatcherQueue
In System Xaml, the app must drain the message queue (e.g. using PeekMessage/DispatchMessage) to make sure all the work
is complete.  If the app fails to do this before the thread exits, lots of objects will get leaked.

In Lifted Xaml, the Xaml framework now cleans itself up automatically during the DispatcherQueue shutdown process.
For islands-based apps, the app must use **DispatcherQueueController.CreateForCurrentThread()** to create a
DispatcherQueue on the thread before using Xaml.  For WinUI Dekstop apps, the framework does this automatically.

More detail at [xaml-islands-and-dispatcherqueue.md](xaml-islands/xaml-islands-and-dispatcherqueue.md).

### Thread poisoning: FIXED FOR XAML
In System XAML Islands, once a `WindowsXamlManager` is used on a thread, and then closed, a XAML can't be used on that
thread anymore.  (We added an explicit block for this).  Lifted Xaml now supports this scenario.

Note there may be other WinAppSDK objects that don't yet behave well in this scenario, so we don't yet claim that thread
poisoning is fixed end-to-end for WinAppSDK scenarios generally.

### Process poisoning: Xaml cannot be restart in a process: NOT FIXED

Once all WindowsXamlManagers in a process are released, XAML will unload its metadata.  If the app tries to use XAML
again in the process, it won't work -- the MUXC metadata has been unloaded. 

### Application lifetime: FIXED
System XAML holds a reference to the app's Application object, and doesn't do the final release until the DLL is
unloaded at process shutdown.  This is generally not a good time to clean things up, it's too late.

In Lifted XAML:
* XAML will Release its references to the app's Application object when all the WindowsXamlManager objects are gone
* XAML will hold a weak-ref to the app's Application object, so that if the app spins up a new WindowsXamlManager while still
keeping its Application object alive, XAML is able to keep using that Application object.
* The `MUX::Application::Current()` API will return null in this state

### Application / WindowsXamlManager startup entanglement: NOT FIXED
In System XAML, if an app wants to use its own custom Application type, it must perform these operations in order:
1. Create custom Application object
2. Call WindowsXamlManager.InitializeForCurrentThread()
3. Call MUX.Application.LoadComponent (if needed)

One common way to do this is to do (2) inside the custom Application's ctor.  This approach has been recommended in the past.

## FAQ
* **Is this just for islands-based apps or also for desktop apps?** A WinUI destkop app is really just a top-level
HWND with a DesktopWindowXamlSource taking up the entire client area.  So, there's not really much difference between
a WinUI Desktop app and a win32 app using islands.  The key difference is that the WinUI3 desktop app will call
`Application.Start()`, and this will create a WindowsXamlManager and run a message pump automatically.
Win32 apps need to do this themselves.
* **Why do we need a DispatcherQueue?** See [xaml-islands-and-dispatcherqueue.md](./xaml-islands/xaml-islands-and-dispatcherqueue.md)

## Open Issues

### API changes for Application and WindowsXamlManager objects
We do eventually plan for the shape of the `WindowsXamlManager` and `Application` APIs to change for island scenarios.
This doc uses the System XAML API shape.

### Fake XAML shutdown during test runs
In XAML's core tests, we do a fake shutdown between tests.  We do have some new tests, in the `XamlIslandTests`
suite, that do "real" shutdown.

### Leak Testing
Since we're not yet going to support unloading all WinUI binaries, there will be a tax an app permanently pays when
it first uses Xaml Islands.  In some sense, this is a "by-design leak".  But, we still want to ensure that we don't
have leaks that cause memory use to soar over time as islands come and go.  As a thought experiemnt, if an app creates
and destroys an island 100 times, the memory use should be the same as when 99th island was destroyed.  The working
set should reach a plateau.

We have just one test that runs a leak check on a basic island scenario, `XamlIslandTests::ShutdownWithLeakDetection`.
We should build this out to validate more complex scenarios.

In the past we have found this to be difficult because there are lots of IXP objects also involved.  As part of the the
"Organized Shutdown" effort, let's talk with IXP about the best way to do leak testing between our two teams.

### Microsoft.ui.xaml.dll can't unload quite yet
We don't yet support XAML getting completely cleaned up in a thread or process yet -- e.g., the memory use doesn't need
to go down to exactly what it was before XAML was used.  We don't yet support fully-unloading microsoft.ui.xaml.dll.

### Specific Scenarios
ASK: It would be nice if we went over specific real-world scenarios to explain things here.  Example: when File Explorer
opens a Context Menu, how does it do this?  Does it create a new XamlIslandRoot or reuse one?  Does it create a new thread
or reuse one?

### What is the value-add here for the end developer?
We're trying to move toward a world where a component can spin XAML up and down on arbitrary threads, doing minimal (or
zero) coordination with other components in the process.  Today, the app must do a lot 
of its own bookkeeping to be able to use Xaml Islands on multiple threads, with various lifetimes.

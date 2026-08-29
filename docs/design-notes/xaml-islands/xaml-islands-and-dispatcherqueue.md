# XAML Islands and DispatcherQueue

## Table of Contents

- [Overview](#overview)
- [Background: System XAML](#background-system-xaml)
- [Problems](#problems)
- [Fix for WinUI 3: Require a DispatcherQueue on the thread for Xaml Islands](#fix-for-winui-3-require-a-dispatcherqueue-on-the-thread-for-xaml-islands)
  - [More About DispatcherQueues](#more-about-dispatcherqueues)
  - [During XAML Shutdown: All Outstanding XAML Async Events are Fired](#during-xaml-shutdown-all-outstanding-xaml-async-events-are-fired)
  - [DispatcherQueueController in WinUI Desktop apps](#dispatcherqueuecontroller-in-winui-desktop-apps)
  - [Nested Message Pumps](#nested-message-pumps)
  - [Why can't XAML create its own DispatcherQueueController like it does today?](#why-cant-xaml-create-its-own-dispatcherqueuecontroller-like-it-does-today)
  - [Why does XAML return RPC_E_WRONG_THREAD?](#why-does-xaml-return-rpc_e_wrong_thread)
  - [Why does XAML close objects in respose to DispatcherQueue.FrameworkShutdownStarting rather than another ShutdownStarting event?](#why-does-xaml-close-objects-in-respose-to-dispatcherqueueframeworkshutdownstarting-rather-than-another-shutdownstarting-event)
- [Appendix](#appendix)
  - [Downsides to manually running a PeekMessage loop](#downsides-to-manually-running-a-peekmessage-loop)

## Overview
*This is part of XAML's strategy for organized shutdown.  See
[xaml-shutdown.md](../xaml-shutdown.md) for more information.*

This page describes the design for Lifted XAML's dependency on DispatcherQueue, specifically for XAML Islands scenarios.

Goals:
* Make it easy for apps to properly clean up the thread without crashing or leaking.  (System XAML suffers from these)
* Participate in IXP's "Organized Shutdown" effort.  

## Background: System XAML
An app using System XAML islands does roughly this work:
``` cpp
// System XAML

auto wxm = Windows::UI::Xaml::WindowsXamlManager::InitializeForCurrentThread();

// Run the message pump
MSG msg; 
while (GetMessage(&msg, nullptr, 0, 0)) 
{ 
    if (!CallPreTranslateMessageHelper()) // Hiding details of calling DesktopWindowXamlSource.PreTranslateMessage
    { 
        TranslateMesasge(&msg); 
        DispatchMessage(&msg); 
    } 
}

// When the last WindowsXamlManager on the thread is released, XAML enqueues a task on the DispatcherQueue to do final
// cleanup.
wxm.Close();

// We need to drain the DispatcherQueue to process the async work scheduled when that last reference to WindowsXamlManager
// went away.
while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) { DispatchMessage(&msg); }

// If the app already created a DispatcherQueueController, it *should* shut it down before the thread exits.  If the app
// didn't make a DispatcherQueueController, XAML creates one (CXcpDispatcher::Init), and *doesn't* shut it down correctly.
// (In practice, probably no XAML islands app does this today)
ShutdownDispatcherQueueController(); // Hiding details

// Most XAML APIs (on thread-affinitive objects) will now return RPC_E_WRONG_THREAD.

// Thread exits
```

## Problems
1. If XAML creates the DispatcherQueueController, the DispatcherQueue is not cleaned up correctly, potentially leading
   to leaks.  [See Below](#why-cant-xaml-create-its-own-dispatcherqueuecontroller-like-it-does-today)
2. The app is required to run a message loop to drain the queue.
3. Even with the message loop, the thread shutdown story still isn't great.  It doesn't allow for deferals, there's no
way to components to know a shutdown is happening, and there's not a guarantee all the work on the DispatcherQueue
will actually get dispatched.  (See "Downsides to manually running a PeekMessage loop" in the appendix.)

## Fix for WinUI 3: Require a DispatcherQueue on the thread for Xaml Islands
A feature was added to make it simpler to clean up a DispatcherQueue correctly.

Here are the significant changes for WinUI 3:
1.  **WinUI 3 Requires a DispatcherQueue**.  For WinUI 3 islands apps (where the app uses `DesktopWindowXamlSource` API), the app
is required to manage a `DispatcherQueue` on the thread before using XAML.
2.  **WinUI 3 Automatically Stays Running until the DispatcherQueue Shuts Down**.  Xaml will keep running on a thread until the
DispatcherQueue.FrameworkShutdownStarting event is raised.
3.  **WinUI 3 Desktop Apps are Unaffected**.  WinUI 3 Desktop apps (where WinUI 3 owns and manages the top-level window) will continue
to work as they have been.  (UWP unaffected as well, but that's not supported, only used for testing today)

Here's example code with these changes:

``` cpp
// Lifted XAML

// Create a lifted DispatcherQueueController.  This will cause DispatcherQueue.GetForCurrentThread to return a valid
// DispatcherQueue when called on this thread.
auto dqc = Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread(); // <-- NEW REQUIREMENT

// Create the WindowsXamlManager by calling WindowsXamlManager.InitializeForCurrentThread.
// This returns an error if there's no lifted DispatcherQueue running on the thread.
// This will initialize XAML for the thread if it's the first caller.  Note that if the app is going to use a custom
// Application object, it will need to be created (or be in the process of being created) before this call.
auto wxm = Microsoft::UI::Xaml::WindowsXamlManager::InitializeForCurrentThread();

// If the app wants to, it could run a custom message pump instead -- just make sure to call ContentPreTranslateMessage.
dqc.DispatcherQueue().RunEventLoop();

// (while the event loop is running, the app creates DesktopWindowXamlSoure objects to use Xaml Islands)

// When thread is about to exit, call ShutdownQueue (new API) to give components get a chance to clean up.
// The Xaml runtime will run down completely during this call.
dqc.ShutdownQueue();

// Most XAML APIs (on thread-affinitive objects) will now return RPC_E_WRONG_THREAD.

// Thread exits
```
### More About DispatcherQueues
* Think of the DispatcherQueue as an enhancement to a Windows thread and Windows message pump.  It allows apps to
safely post messages to other threads and shut down in a deterministic and organized way.
* It's OK to run a custom message pump rather than calling DispactcherQueue.RunEventLoop, and it's OK to create the
DispatcherQueueController after the custom message pump has started.  The example shows it being created before the pump
only for clarity.  But it is definitely required that DispatcherQueueController is created before WindowsXamlManager.
* It's supported to shutdown a DispatcherQueue and then start a new one on the same thread.  But the app must call
ShutdownQueue on the first DispatcherQueue first.
* The app MUST call ShutdownQueue on the DispatcherQueue in order to give each participating component a chance to
clean up.  If the app fails to call ShutdownQueue, the process will likely leak memory and possibly crash later on.

### During XAML Shutdown: All Outstanding XAML Async Events are Fired
When the XAML runtime runs down on a thread (which only happens when the `DispatcherQueueController` is being
shut down), it will fire off all its remaining async events before closing.  This means that `FrameworkElement.Unloaded`
will fire at this time for any XAML elements that were removed from the live tree as part of the shutdown process.

### DispatcherQueueController in WinUI Desktop apps
In WinUI Desktop apps, the XAML runtime owns the message pump, so it will also own the `DispatcherQueueController`.
XAML will create the DQC right before it runs the message pump.

In System XAML, in `CXcpDispatcher::Init` we create the `DispatcherQueueController` in the XAML startup path if there's
not already a DispatcherQueue on the thread.  In WinUI 3, we no longer do this -- rather, `CXcpDispatcher::Init`
will fail if there's no current DispatcherQueue.

For WinUI Desktop Apps, this change will be relatively unobservable by the app code -- a `DispatcherQueue` will still
exist on the thread, but will be created slightly earlier, and run down after the PostQuitMessage comes through
and shuts down the message pump.  There may be some risk that during shutdown, app code may run after the DispatcherQueue
is run down.

### Nested Message Pumps
*Note: This is somewhat academic right now because we don't yet support nested message pumps with WinUI 3.*

What if a component wants to run a _nested_ message pump on a thread that may or may not already have an associated
`DispatcherQueue`?   The app can check to see if a `DispatcherQueue` is on the thread -- if so, just use it.  If not,
it can create it's own `DispatcherQueueController`.

``` cpp
// Psuedocode
Microsoft::UI::Dispatching::DispatcherQueueController dqc {nullptr};

// For a nested message pump, only start a DispatcherQueue if there's not already one on the thread.
if (!Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread())
{
    dqc = Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();
}

// Run the NESTED message pump.  Could also use dqc.DispatcherQueue().RunEventLoop()
MSG msg; 
while (GetMessage(&msg, nullptr, 0, 0)) 
{ 
    if (!ContentPreTranslateMessage(&msg))  
    { 
        TranslateMesasge(&msg); 
        DispatchMessage(&msg); 
    } 
}

if (dqc)
{
    dqc.ShutdownQueue();
}
```

### Why can't XAML create its own DispatcherQueueController like it does today?
Does the app _really_ need to create a DispatcherQueueController first?  Ideally we'd rather not put more requirements
on the app code to support Xaml islands.  

Conceptually, DispatcherQueue is an enhancement to the message pump, so should be owned and managed by the component
that owns and manages the message pump.  For Xaml Desktop apps, Xaml will manage the DispatcherQueueController itself,
because it owns the message pump.  But for Xaml Islands apps, the app will be required to create the
DispatcherQueueController because _it_ owns the message pump.

If Xaml were to create, manage, and shutdown the DispatcherQueue in islands scenarios, the app would find that
`DispatcherQueue.TryEnqueue` mysteriously stops working after XAML is shut down on the thread.  Better to just make the
explicit requirement that a DispatcherQueue already exist up front.

For WinUI 3 Desktop apps, the framework will indeed create the DispatcherQueueController, and the app won't have to
worry about it.

We get these benefits (from benwest):
* **Less fragile.**  No need for apps to worry about timing the full release ahead of message loop shutdown/thread exit.
* **Allows decoupling.** Allows WindowsXamlManager owner to decouple itself from the message pump. I think it's likely
we'll have app components creating islands that don't want to or can't collude with whoever owns the message pump.
Without this feature, they have no way to guarantee Xaml shuts down cleanly.

### Why does XAML return RPC_E_WRONG_THREAD?
XAML has historically returned `RPC_E_WRONG_THREAD` when a thread-affinitive object is called on a thread for which
there is no running `DXamlCore` object.  

### Why does XAML close objects in respose to DispatcherQueue.FrameworkShutdownStarting rather than another ShutdownStarting event?
The `DispatcherQueue` object now supports three different ShutdownStarting events:
* `DispatcherQueue.ShutdownStarting` -- the app subscribes to this event, and this gets fired first when shutdown starts.
* `DispatcherQueue.FrameworkShutdownStarting` -- the framework (XAML, for instance) subscribes to this event, it's
fired second.
* `DispatcherQueue.PlatformShutdownStarting` -- this is private, and WinAppSDK platform objects such as the Compositor,
input objects, and island objects subscribe to this event.  It gets fired last.

The shutdown sequence roughly matches the layers in the system.  The highest-level layer, the app can clean up its
objects without worrying the framework objects have already cleaned up.  Similarly, XAML can clean up its objects
without worrying that the Compositor (for example) has been closed first.

## Appendix

### Downsides to manually running a PeekMessage loop
From BenWest:

There a few downsides to manually running a PeekMessage loop vs. the new ShutdownQueue API.
1.	PeekMessage doesn't always drain the queue after returning FALSE. The details are complicated, but in short a
"qevent" dispatch that adds a posted message will not be considered by the return value of PeekMessage. Shell hit this
last year and had to use ShutdownQueueAsync to drain reliably.
2.	A PeekMessage drain doesn't allow for shutdown deferral for things not contained in the Win32 message queue. That is, with
the new ShutdownQueue, there's a means for components to defer the shutdown until some external event completes. If Xaml
promotes the PeekMessage drain as a usage pattern, that would break anyone using DQ's deferral APIs.
3.	A PeekMessage drain doesn't fire DQ events, so components on the thread won't know the message pump is done. They
might continue to post work that never dispatches.
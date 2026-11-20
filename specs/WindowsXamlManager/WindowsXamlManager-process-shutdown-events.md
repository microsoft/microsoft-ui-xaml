# WindowsXamlManager process shutdown events

===

# Background

Shutting down the final WinUI thread does not by itself guarantee that all state from that WinUI
generation (the period from WinUI's first initialization in a process until its final WinUI thread
shuts down) has been released. WinUI and controls frameworks can retain process-wide state such as
dependency property registrations and WinUI metadata objects. If this state survives shutdown, a
later attempt to initialize WinUI can encounter objects associated with the previous generation
and fail to restart cleanly.

Frameworks need a process-wide hook at the generation shutdown boundary so they can release state
that must not survive into the next WinUI generation. Apps also need a notification after that
cleanup has completed so they know when it is safe to attempt a restart.

The existing
[`WindowsXamlManager.XamlShutdownCompletedOnThread`](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.hosting.windowsxamlmanager.xamlshutdowncompletedonthread)
event cannot provide this boundary because it describes only one thread. A process can host WinUI
on multiple threads, and clearing process-wide state when one thread shuts down could invalidate
state still in use by another.

A single process-wide event is also not sufficient because event handler ordering is unspecified.
An app could attempt to restart WinUI before a controls framework's cleanup handler had run.

This feature adds two static events to `WindowsXamlManager`:

* `WinUIProcessShutdownStarting` gives frameworks a synchronous cleanup phase.
* `WinUIProcessShutdownCompleted` tells apps that process-wide cleanup has completed.

> These APIs are **experimental** and are gated behind `Feature_ExperimentalApi`.

# Conceptual pages (How To)

## Responding to process-wide WinUI shutdown

Subscribe to `WinUIProcessShutdownStarting` when a framework or an app has process-wide state that
is valid only for the current WinUI generation. Release that state synchronously in the event handler.

Subscribe to `WinUIProcessShutdownCompleted` when an app needs to know when process-wide cleanup
has finished. After this event is raised, the app can attempt to initialize a new WinUI generation
on a thread with a usable `DispatcherQueue`.

The events are raised in this order:

1. WinUI shuts down on the final thread using WinUI in the process.
2. `WindowsXamlManager.XamlShutdownCompletedOnThread` is raised for that final WinUI thread.
3. `WindowsXamlManager.WinUIProcessShutdownStarting` is raised.
4. All `WinUIProcessShutdownStarting` handlers return.
5. `WindowsXamlManager.WinUIProcessShutdownCompleted` is raised.

Both process events are raised synchronously on the thread that shuts down the final WinUI
runtime instance. They are raised once for each complete process-wide WinUI shutdown. If WinUI is
initialized again, a later shutdown of that generation raises the events again.

The process events have no meaningful instance sender or event data. Both `sender` and `args` are
`null`.

`WinUIProcessShutdownStarting` handlers must finish their cleanup before returning. The event does
not support deferrals. A handler should not initialize WinUI or create new WinUI objects.

`WinUIProcessShutdownCompleted` indicates that all process shutdown cleanup handlers have
completed. It does not guarantee that a particular thread or `DispatcherQueue` is suitable for
initialization. For example, the thread that receives the event can still be in its
`DispatcherQueue` shutdown sequence. `WindowsXamlManager.InitializeForCurrentThread` remains the
authoritative check and can return `ERROR_INVALID_STATE`.

When WinUI is active on multiple threads, there is no ordering guarantee between a
`WinUIProcessShutdown*` event and `XamlShutdownCompletedOnThread` on a thread other than the one
that shuts down the final WinUI runtime instance. Use `XamlShutdownCompletedOnThread` for
thread-specific cleanup and the process events only for process-wide state.

# Examples

## Clear framework state before WinUI process shutdown completes

This example shows a controls framework subscribing once and clearing state associated with the
current WinUI generation.

```csharp
internal static class FrameworkLifetime
{
    private static bool isSubscribed;

    internal static void EnsureInitialized()
    {
        if (!isSubscribed)
        {
            WindowsXamlManager.WinUIProcessShutdownStarting += OnProcessShutdownStarting;
            isSubscribed = true;
        }

        EnsureGenerationState();
    }

    private static void OnProcessShutdownStarting(object? sender, object? args)
    {
        ClearDependencyPropertyReferences();
        ClearWinUIMetadataCache();
        ClearGenerationState();
    }
}
```

The subscription can remain active for the lifetime of the process. The handler runs again when a
later WinUI generation shuts down.

## Wait until process-wide cleanup is complete

This example tracks when an app can attempt to start another WinUI generation. The app performs the
actual restart after its current shutdown operation has returned and after it has a usable
`DispatcherQueue`.

```csharp
private const int ErrorInvalidState = unchecked((int)0x8007139F);
private bool canStartWinUI = true;

private void RegisterProcessShutdownEvents()
{
    WindowsXamlManager.WinUIProcessShutdownStarting += (_, _) =>
    {
        canStartWinUI = false;
    };

    WindowsXamlManager.WinUIProcessShutdownCompleted += (_, _) =>
    {
        canStartWinUI = true;
    };
}

private void StartWinUI()
{
    if (!canStartWinUI)
    {
        return;
    }

    try
    {
        // The current thread must have a usable DispatcherQueue.
        WindowsXamlManager manager = WindowsXamlManager.InitializeForCurrentThread();
    }
    catch (COMException exception) when (exception.HResult == ErrorInvalidState)
    {
        // Another WinUI generation may have started and begun shutting down.
        // Wait for the next WinUIProcessShutdownCompleted event before retrying.
    }
}
```

# API Pages

## WindowsXamlManager.WinUIProcessShutdownStarting event

Occurs when the final WinUI thread in the process has shut down and the app and process-wide
frameworks must release state associated with the previous WinUI generation.

```csharp
public static event EventHandler<object> WinUIProcessShutdownStarting;
```

The event is raised synchronously on the thread that shuts down the final WinUI runtime instance.
Frameworks must release their generation-bound state before returning from the handler. This can
include dependency property references, WinUI metadata caches, and other process-static objects that
cannot be reused by a later WinUI generation.

The event is raised after `WindowsXamlManager.XamlShutdownCompletedOnThread` on the thread that
shuts down the final WinUI runtime instance. At that point all WinUI runtime instances have shut
down, but `WindowsXamlManager.XamlShutdownCompletedOnThread` might not yet have been delivered on
other threads because no cross-thread ordering is guaranteed.

The `sender` and `args` values are `null`. The event does not support deferrals, and it is not
delayed by deferrals obtained from `WindowsXamlManager.XamlShutdownCompletedOnThread`. Those
deferrals delay the `DispatcherQueue` shutdown sequence, not the WinUI process shutdown events.

Use this event handler only to release references to objects and state associated with the
previous WinUI generation. Do not otherwise access WinUI state, initialize WinUI, or create WinUI
objects. Wait for `WinUIProcessShutdownCompleted` before attempting to initialize another WinUI
generation.

## WindowsXamlManager.WinUIProcessShutdownCompleted event

Occurs after all `WinUIProcessShutdownStarting` handlers have returned and WinUI process-wide
shutdown has completed.

```csharp
public static event EventHandler<object> WinUIProcessShutdownCompleted;
```

After this event is raised, an app can attempt to initialize another WinUI generation. The target
thread must still meet the normal requirements for WinUI initialization, including having a usable
`DispatcherQueue`.

The event is raised synchronously on the thread that shut down the final WinUI runtime instance.
The `sender` and `args` values are `null`.

Another WinUI generation can start and begin shutting down before code waiting on this notification
runs. `WindowsXamlManager.InitializeForCurrentThread` can therefore still return
`ERROR_INVALID_STATE`.

# API Details

```csharp (but really MIDL3)
namespace Microsoft.UI.Xaml.Hosting
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 5)]
    [webhosthidden]
    runtimeclass WindowsXamlManager : Windows.Foundation.IClosable
    {
        // Existing members omitted.

        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        [feature(Feature_ExperimentalApi)]
        {
            /// Occurs when the final WinUI thread in the process has shut down and
            /// process-wide frameworks must synchronously release state associated
            /// with the previous WinUI generation.
            static event Windows.Foundation.EventHandler<Object>
                WinUIProcessShutdownStarting;

            /// Occurs after all WinUIProcessShutdownStarting handlers have returned
            /// and WinUI process-wide shutdown has completed.
            static event Windows.Foundation.EventHandler<Object>
                WinUIProcessShutdownCompleted;
        }
    }
}
```

# Appendix

## Why are there two events?

Event handler ordering is unspecified. With one event, an app's restart handler could run before a
framework's cleanup handler. Separate starting and completed phases guarantee that all synchronous
framework cleanup finishes before the app receives the completion notification.

## Why not use XamlShutdownCompletedOnThread?

`XamlShutdownCompletedOnThread` describes one WinUI thread. It can be raised while WinUI remains
active on another thread, so clearing process-wide framework state from that event could break the
remaining WinUI thread.

## Why are the events static?

The events describe the lifetime of a process-wide WinUI generation rather than the lifetime of one
`WindowsXamlManager`. The final manager can already be closed when process-wide cleanup occurs.

## Why are the sender and event arguments null?

There is no single `WindowsXamlManager` that represents process-wide WinUI state, and the events do
not currently expose additional data. Using `EventHandler<Object>` avoids introducing event
argument types with no meaningful members.

## Why does WinUIProcessShutdownStarting not support deferrals?

WinUI shutdown cannot be deferred by any of the three `WindowsXamlManager` shutdown events.
`WinUIProcessShutdownStarting` is raised after WinUI has shut down, and its handlers provide a
synchronous opportunity to release state from the previous WinUI generation.

The deferral available from `XamlShutdownCompletedOnThread` delays the `DispatcherQueue` shutdown
sequence. It does not delay WinUI shutdown, `XamlShutdownCompletedOnThread`, or either
`WinUIProcessShutdown*` event.

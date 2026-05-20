WindowsXamlManager Shutdown Improvements Spec
===

# Background

> This section will not be part of public docs.

Design Document: Xaml Shutdown Improvements.

Some WinUI apps want a specific place to run their cleanup code.  They'd like to run code when:
* The Xaml live tree(s) have been fully disassembled, and all the Unloaded events have been raised.
* The DispatherQueue is still available to take new work.

Ideally the app could also take a deferral so that the DispatcherQueue doesn't finish shutting down until the app is ready.

This document introduces a new Xaml event to give these apps a good hook for this kind of cleanup.

Note that Xaml has process-wide state and also thread-specific state. Each have a distinct shutdown process, and in this doc
when we discuss "Xaml shutdown", we're specifically talking about Xaml cleaning up it's per-thread state.

## Example Sceanrio: A Model-View-ViewModel (MVVM) Xaml App

Consider an app with "View" objects that are part of the Xaml scene, and "Model" objects that contain app logic.  It's
not meaningful for a View object to outlive the Model objects, so it's reasonable for a View object to take a hard
dependency on the app Model objects and always expect them to exist. In this case, it's helpful to apps to have a place
they can run code where they know that all the View objects have been unloaded and aren't using the Model objects
anymore.  


## Changes for the WindowsXamlManager class
> Let's add these sections to the WindowsXamlManager class documentation.

Changes from WinAppSDK 1.4 to WinAppSDK 1.5
* In WinAppSDK 1.4, the Xaml runtime shuts down asynchronously on a thread when all the **WindowsXamlManager** and
**DesktopWindowXamlSource** objects on that thread have been Closed or destroyed.  In WinAppSDK 1.5, the Xaml runtime
continues to run until the **DispatcherQueue** on the thread shuts down.
* In WinAppSDK 1.4, **WindowsXamlManager.InitializeForCurrentThread** returns a new object each time it is called.  In
WinAppSDK 1.5, it will return the same **WindowsXamlManager** instance until the **DispatcherQueue** on the thread shuts
down.


# Conceptual pages (how to)

> No conceptual pages right now.

# API Pages

## WindowsXamlManager class

> The **WindowsXamlManager** class was already introduced in WinAppSDK 1.4, only the new APIs are shown here.

Represents a handle to the instance of the Xaml runtime that's running on a specific thread.

### Methods

| Name | Description |
|-|-|
| GetForCurrentThread | Returns the WindowsXamlManager object associated with the current thread.  Returns null if the Xaml runtime isn't running on the thread.  (in this case, most APIs in the Xaml namespace will return error.) |

### Events

| Name | Description |
|-|-|
| XamlShutdownCompletedOnThread | Raised when the Xaml runtime has finished its shutdown process on the current thread. |

## WindowsXamlManager.XamlShutdownCompletedOnThread Event

Raised when the Xaml runtime has finished its shutdown process for the current thread.

Xaml's shutdown is tied to the shutdown sequence of the DispatcherQueue running on the thread.  See [DispatcherQueue documentation](https://learn.microsoft.com/en-us/windows/apps/develop/dispatcherqueue).

When a **DispatcherQueue** on a thread using Xaml is shutdown, these events are raised in order:
* The **DispatcherQueue.ShutdownStarting** event is raised.  Intended for apps to handle.
* The **DispatcherQueue.FrameworkShutdownStarting** event is raised.  Intended for frameworks to handle. 
    * The **WindowsXamlManager.XamlShutdownCompletedOnThread** event is raised in response to FrameworkShutdownStarting.  Intended for apps to handle.
* The **DispacherQueue.FrameworkShutdownCompleted** event is raised.  The DispatcherQueue is not available for new work at this point.
* The **DispacherQueue.ShutdownCompleted** event is raised.  The DispatcherQueue is not available for new work at this point.

At the time the **WindowsXamlManager.XamlShutdownCompletedOnThread** event is raised:
* Xaml has unloaded all the live Xaml objects and raised the **Unloaded** event for each object.
* Xaml no longer has any state associated with the current thread.  **WindowsXamlManager.GetForCurrentThread()** returns **null** at this time.
* The **DispatcherQueue** on the current thread is still available and usable.  It is in its shutdown sequence, so note the **ShutdownStarting** event has already been raised and will not be raised again.

Note that even if you call **Close** on the **WindowsXamlManager** object or release all your references to it, this
event will still be raised.

Here's a sample that shows you how you might subscribe to the event to clean up your objects when Xaml's no longer
running on the thread:

```c++
// In this sample we have some different types of objects we want to clean up as the app shuts down. Xaml has some
// indirect references to some of these objects (for example, "Model" objects like in an Model-View-ViewModel app), so
// we don't want to destroy these until Xaml is finished with its work. We also have some objects that are getting
// cleaned up in another process, and we want to wait for those as well.
//
// So, we use the XamlShutdownCompletedOnThread event and its deferral to organize our shutdown process. When this event
// is raised, we know Xaml is done using our objects on this thread. We can also take a deferral so that shutdown won't
// complete until our remote operations are finished.
WindowsXamlManager manager = WindowsXamlManager::GetForCurrentThread();
manager.XamlShutdownCompletedOnThread([](
    const WindowsXamlManager& sender,
    const XamlShutdownCompletedOnThreadEventArgs& args) -> IAsyncAction
    {
        // Once we get this deferral, the DispatcherQueue shutdown process won't continue until Complete is called.
        // Until we call deferral.Complete(), we can still use the DispatcherQueue and give it new work.
        auto deferral = args.GetDispatcherQueueDeferral();

        // Now that Xaml has shutdown, we can clean up any of our objects that Xaml might have been using.
        CleanupUIThreadObjects();

        // Capture the UI thread context.
        winrt::apartment_context ui_thread;

        // We can also do cleanup work that might take a while. For example, we can wait for work in other processes
        // to finish.
        co_await CleanupRemoteObjects();

        // Switch back to the UI thread, in case we have any UI-thread work left to do.
        // It will still be running because we took the deferral.
        co_await ui_thread;

        // Done! Shutdown may continue.
        deferral.Complete();
    });

```

## XamlShutdownCompletedOnThreadEventArgs class

This is the event args type used by the **XamlShutdownCompletedOnThread** event.

### Methods

| Name | Description |
|-|-|
| GetDispatcherQueueDeferral | Returns a deferral object.  Call the **Complete** method on that object when you're ready for the DispatcherQueue's shutdown sequence to continue.   |

# API Details

## WindowsXamlManager class

```cs (but really MIDL3)
namespace Microsoft.UI.Xaml.Hosting
{
    runtimeclass WindowsXamlManager
        : Windows.Foundation.IClosable
    {
        // Existing APIs:
        static WindowsXamlManager InitializeForCurrentThread();

        // NEW APIs:
        static Microsoft.UI.Xaml.Hosting.WindowsXamlManager GetForCurrentThread();
        event Windows.Foundation.TypedEventHandler<WindowsXamlManager,XamlShutdownCompletedOnThreadEventArgs> XamlShutdownCompletedOnThread;
    }
}
```

## XamlShutdownCompletedOnThreadEventArgs class

```cs (but really MIDL3)
namespace Microsoft.UI.Xaml.Hosting
{
    runtimeclass XamlShutdownCompletedOnThreadEventArgs
    {
        Windows.Foundation.Deferral GetDispatcherQueueDeferral();
    }
}
```

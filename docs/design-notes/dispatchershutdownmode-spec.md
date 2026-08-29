Application.DispatcherShutdownMode
===

# Release Notes
There is a behavioral difference between WinAppSDK 1.4 and WinAppSDK 1.5 for Xaml Islands-based apps:
* In WinAppSDK 1.4, the Xaml runtime always exits the thread's event loop when the last Xaml window on a thread is closed.
* In WinAppSDK 1.5:
  * If your app is a WinUI Desktop app, the default behavior is still the same as in WinAppSDK 1.4.
  * If you're using Xaml for the DesktopWindowXamlSource ("Xaml Islands") API, the default behavior is now that Xaml does
  _not_ automatically exit the thread's event loop.
  * In both modes, you can change this behavior by setting the **Application.DispatcherShutdownMode** property.

For more information, see the documentation for the **Application.DispatcherShutdownMode** property.

# Conceptual pages (How To)

> No conceptual pages right now.

# API Pages

## DispatcherShutdownMode enum

**DispatcherShutdownMode** can be one of these values:

| Name | Description |
|-|-|
| OnLastWindowClose | When the last **Microsoft.UI.Xaml.Window** object on the thread is closed, the **DispatcherQueue.RunEventLoop** call or message pump on the thread will exit. |
| OnExplicitShutdown  | When the last **Microsoft.UI.Xaml.Window** object on the thread is closed, the Xaml runtime will take no action to stop the event loop or message pump. |

### DispatcherShutdownMode.OnExplicitShutdown

When you use **OnExplicitShutdown**, you'll need to exit the event loop yourself when you're ready for the event loop on
that thread to exit.  To properly exit the event loop, call **DispatcherQueue.EnqueueEventLoopExit()**.   Alternatively,
you can call **PostQuitMessage()**.

## Application class

> This is an existing class.  Existing APIs are not shown here.

### Properties

| Name | Description |
|-|-|
| DispatcherShutdownMode | Gets or sets the **DispatcherShutdownMode** to use for the current thread.  The default value depends on context (see details). |

## Application.DispatcherShutdownMode

When the **Application.Start** API is called (as it is at startup for WinUI Desktop apps), the Xaml runtime sets
**DispatcherShutdownMode** to **OnLastWindowClose** for the current thread.  This causes the DispatcherQueue's event
loop to exit when all the Xaml Windows on the thread are closed.

If the application does not call **Application.Start** (as is generally the case for Xaml islands-based applications)
this property will remain at its default value: **OnExplicitShutdown**. In this state, the Xaml runtime
doesn't take any action to exit the event loop when the Xaml Windows are closed.  This is a behavioral change between
WinAppSDK 1.4 and WinAppSDK 1.5.  In WinAppSDK 1.4, the Xaml runtime always exits the thread's event loop when the last
Xaml window on a thread is closed.

In a WinUI Desktop application, you may want your code to keep running even after all the Xaml windows on the thread
have closed.  To accomplish this, you can set this property to **OnExplicitShutdown**.  Now, after all the Xaml windows
have closed, the thread will keep running.  In this state, you can still schedule work on the **DispatcherQueue**, run
work on other threads, and display new Xaml windows.

When you use **OnExplicitShutdown**, you'll need to exit the event loop yourself when you're ready for the event loop on
that thread to exit.  To properly exit the event loop, call **DispatcherQueue.EnqueueEventLoopExit()**.   Alternatively,
you can call **PostQuitMessage()**.

This is a per-thread property.  When you set it, the property will only change for the current thread. It is possible to
have different values for this property on different threads in your application.

This property can be set at any time.  The Xaml runtime reads the property each time the last window on any thread is
closed.

# API Details

```cs (But really MIDL3)
namespace Microsoft.UI.Xaml {

    enum DispatcherShutdownMode
    {
        OnLastWindowClose,	// default for WinUI Desktop apps
        OnExplicitShutdown,	// default for WinAppSDK Xaml Island apps
    }

    runtimeclass Application // existing type
    {
        Microsoft.UI.Xaml.DispatcherShutdownMode DispatcherShutdownMode {get; set;}
    }

}
```


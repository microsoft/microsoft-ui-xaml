Providing a custom MRT Core `IResourceManager` for WinUI 3 to use
===

# Background

The WinUI 3 framework instantiates an MRT Core
[`ResourceManager`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.windows.applicationmodel.resources.resourcemanager?view=windows-app-sdk-1.2)
in order to resolve resource URIs. While this default `ResourceManager` is generally sufficient, 
there are some scenarios in which an app needs non-standard behavior in order to resolve a
particular resource URI. In order to address this requirement, this document describes a new
API that would allow an app to provide its own custom implementation of the
[`IResourceManager`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.windows.applicationmodel.resources.iresourcemanager?view=windows-app-sdk-1.2)
for WinUI 3 to use in lieu of its standard `ResourceManager`.

# API Pages

_(Each of the following L2 sections correspond to a page that will be on docs.microsoft.com)_

## Application.ResourceManagerRequested Event

Raised during startup of a new WinUI thread to give the app a chance to provide its own
[`IResourceManager`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.windows.applicationmodel.resources.iresourcemanager?view=windows-app-sdk-1.2)
implementation to be used by the framework for resolving resource URIs.

_Spec note: is there a version of the 'ResourceDictionary and XAML resource references' article that 
links to the Windows App SDK documentation for relevant APIs?_

```c#
public event TypedEventHandler<Application, ResourceManagerRequestedEventArgs> 
ResourceManagerRequested

```

### Remarks

It is strongly recommended that apps which need to listen for this event register their handler in their `App` 
class's constructor so that it is available during initial app launch. This event is raised once per WinUI
thread during initialization. Because the framework assumes that each `IResourceManager` instance returned
through the event is independent, it is your responsibility to ensure that the implementation is thread-safe
if you wish to share the same instance between multiple threads.



## ResourceManagerRequestedEventArgs Class

Provides event data for the `Application.ResourceManagerRequested` event.

```c#
public sealed class ResourceManagerRequestedEventArgs
```

### Remarks

`ResourceManagerRequestedEventArgs` is used to provide the WinUI 3 framework with a custom implementation
of [`IResourceManager`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.windows.applicationmodel.resources.iresourcemanager?view=windows-app-sdk-1.2)
to be used for resolving resource URIs rather than the default `ResourceManager` instance that the framework
creates. In the event handler, you should instantiate your custom `IResourceManager` and assign it to the
`ResourceManagerRequestedEventArgs.ResourceManager` property. The value of this property is initially
`null`, and it is only checked by the framework once per event raise after all registered event handlers have
been invoked. If the property value is still `null` then the framework will use the default `ResourceManager`
instance.


## ResourceManagerRequestedEventArgs.ResourceManager Property

Gets and sets the explanation of the XAML resource reference failure.

_Spec note: The type of this property should be `IResourceManager`,
but there's an issue with the cswinrt projection where it gets confused
about there being an existing internal interface by the same name behind the Windows
[ResourceManager](https://docs.microsoft.com/uwp/api/Windows.ApplicationModel.Resources.Core.ResourceManager)._

```c#
public object ResourceManager { get; set; }
```


# API Details
```c#
namespace Microsoft.UI.Xaml
{
  runtimeclass Application
  {
    // existing ...

    event Windows.Foundation.TypedEventHandler<object,Microsoft.UI.Xaml.ResourceManagerRequestedEventArgs> 
        ResourceManagerRequested;
  };

  runtimeclass ResourceManagerRequestedEventArgs
  {
    object ResourceManager { get; set; };
  };
}

```


# Appendix

- Currently, `ResourceManagerRequestedEventArgs.ResourceManager` is last writer wins; if there are multiple event handlers
registered for the event then the last one to set the property value is the one whose action is respected by the framework.
There is concern that this could lead to difficult-to-debug errors; should we explicitly block this possibility by throwing an
exception if the developer attempts to register more than one event handler?

- Should `ResourceManagerRequestedEventArgs.ResourceManager` be pre-populated with the default `ResourceManager` instance
rather than `null`? This would simplify things a little bit for developers who wish to handle the `ResourceManager.ResourceNotFound`
event but otherwise rely on the default resource URI resolution behavior but does add a very small amount of overhead (allocation of
the default `ResourceManager`) even if the developer ultimately decides to provide their own `IResourceManager`.

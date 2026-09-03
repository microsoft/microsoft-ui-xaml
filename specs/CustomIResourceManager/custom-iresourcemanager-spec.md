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
interface for WinUI to use in lieu of its default `ResourceManager`.

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

It is strongly recommended that apps which need to listen for this event register their handler in 
their `App` class's constructor so that it is available during initial app launch as in the 
following example.

```c#
public App()
{
   this.InitializeComponent();

   ResourceManagerRequested += (_, e) =>
   {
      // CreateResourceManager() is a custom method that returns an instance of
      // Microsoft.Windows.ApplicationModel.Resources.IResourceManager.
      IResourceManager resourceManager = CreateResourceManager();
      e.ResourceManager = resourceManager;
   };
}
```

This event is raised once per WinUI thread during initialization. If you use the same 
`IResourceManager` for multiple threads, then the `IResourceManager` must be thread-safe.



## ResourceManagerRequestedEventArgs Class

Provides event data for the `Application.ResourceManagerRequested` event.

```c#
public sealed class ResourceManagerRequestedEventArgs
```

### Remarks

`ResourceManagerRequestedEventArgs` is used to provide the WinUI 3 framework with a custom 
implementation of the 
[`IResourceManager`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.windows.applicationmodel.resources.iresourcemanager?view=windows-app-sdk-1.2)
interface to be used for resolving resource URIs rather than the default `ResourceManager` that the 
framework creates. In the event handler, you should instantiate your custom `IResourceManager` and 
assign it to the `ResourceManagerRequestedEventArgs.CustomResourceManager` property. The value of this 
property is initially `null`, and it is only checked by the framework once per event raise after 
all registered event handlers have been invoked. If the property value is still `null` then the 
framework will use the default `ResourceManager`.


## ResourceManagerRequestedEventArgs.CustomResourceManager Property

Sets the custom `IResourceManager` that should be used by WinUI to resolve MRT resources for the 
current thread. If you leave the value `null` then the default `ResourceManager` will be used.

```c#
public IResourceManager ResourceManager { get; set; }
```


# API Details
```c#
namespace Microsoft.UI.Xaml
{
  runtimeclass Application
  {
    // existing ...

    event Windows.Foundation.TypedEventHandler<Application,Microsoft.UI.Xaml.ResourceManagerRequestedEventArgs> 
        ResourceManagerRequested;
  };

  runtimeclass ResourceManagerRequestedEventArgs
  {
    IResourceManager CustomResourceManager;
  };
}

```


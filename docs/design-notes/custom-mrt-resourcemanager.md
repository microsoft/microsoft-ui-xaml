# Custom MRT ResourceManager for apps

## Table of Contents

- [Overview](#overview)
- [Design](#design)
  - [Why define the new event on Application?](#why-define-the-new-event-on-application)
- [Alternative approaches](#alternative-approaches)
  - [A property that apps populate with their ResourceManager](#a-property-that-apps-populate-with-their-resourcemanager)
  - [A virtual callback method that apps can override](#a-virtual-callback-method-that-apps-can-override)

## Overview

File Explorer, and Shell in general, has resource management scenarios that mandate the use of 
System MRT rather than MRT Core as the latter does not have the requisite capabilities (primarily 
the inability to resolve resources that live in siblings of the main resource map). In order 
to address this need, a translation layer between the MRT Core and System MRT API surfaces was 
added to the OS which would allow System MRT to be used under the hood while presenting a surface 
that was compatible with MRT Core consumers. In order to take advantage of this, however, it is 
necessary to modify WinUI 3 to consume this translation layer shim in lieu of creating its own
MRT Core `ResourceManager`.

To enable this, this document proposes that a new event, `ResourceManagerRequested`, be added
to the `Application` class. This event will be raised immediately prior to a XAML core 
[lazily constructing](/dxaml/xcp/components/mrt/ModernResourceProvider.cpp) a `ResourceManager` 
instance in order to resolve resource URIs, and will allow WinUI to ask for, and utilize, a custom 
`IResourceManager` implementation from the app that should be used instead of the standard MRT Core
`ResourceManager`. While File Explorer is the initial and primary user of this feature, it is
expected that other apps will also be interested in taking advantage of this.

## Design

The new API appears below:

```C#
runtimeclass ResourceManagerRequestedEventArgs
{
    Microsoft.Windows.ApplicationModel.Resources.IResourceManager CustomResourceManager;
};

unsealed runtimeclass Application
{
    // pre-existing API
    // ...

    event Windows.Foundation.TypedEventHandler<Object,Microsoft.UI.Xaml.ResourceManagerRequestedEventArgs> ResourceManagerRequested;
};

```

The logic implementing this new API is very straightforward:

```
raise event with eventargs args
if args.CustomResourceManager != null
then
    resourceManager = args.CustomResourceManager
else
    resourceManager = new Microsoft.Windows.ApplicationModel.Resources.ResourceManager()

```

An app simply needs to register an event handler for the `Application.ResourceManagerRequested` 
event that populates the passed in event args object with an `IResourceManager` implementation. 
Because WinUI will need a `ResourceManager` fairly early during initialization (when the framework 
automatically loads `App.xaml`) this event subscription should be performed in the constructor for 
the app's custom `Application` object.

While there is no hard requirement that `IResourceManager` instances must not be shared between 
threads, to match WinUI's default semantics each thread that receives the 
`Application.ResourceManagerRequested` event should return its own `IResourceManager` instance.

### Why define the new event on `Application`?

`Application` is an appropriate declaring type for the new event because it is where our other 
app-wide APIs and services (e.g. `DebugSettings`, `Application.Resources`, `IXMP` implementation, etc.) 
are located, and an instance of the class (`Application.Current`) is available very early during 
startup. There is future work planned to allow apps to utilize XAML islands without needing to go 
through the `Application` class;
once that feature is designed this event should be considered for migration to the new pattern.

## Alternative approaches

### A property that apps populate with their `ResourceManager`

The primary downside to this approach is that the window of opportunity for providing a 
`ResourceManager` to the framework is not an explicit part of the API. While in principle the
framework could reject a late attempt to provide a `ResourceManager`, this is suboptimal API design 
compared to simply not allowing the app to take that action to begin with.

### A virtual callback method that apps can override

This is technically similar to subscribing to an event but it locks the developer into implementing
a custom class derived from `Application` which is a pattern we are trying to move away from for new
code.
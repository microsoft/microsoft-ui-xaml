Tracing XAML resource reference lookup failures
===

# Background

A XAML resource is an item in a 
[ResourceDictionary](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.ResourceDictionary)
that can be referenced in markup with a `{StaticResource}` or `{ThemeResource}` reference.
For example:

```xml
<StackPanel >
    <StackPanel.Resources>
        <SolidColorBrush x:Key="myBrush" Color="Red"/>
    </StackPanel.Resources>

    <Rectangle Fill="{StaticResource myBrush}"/>
```

`ResourceDictionary`s can be defined in multiple places, so the resource reference is resolved
as a search.

Failure to resolve a XAML resource reference is one of 
the most common causes of app crashes. Such failures manifest in the _native code_ debug output with the 
message "Cannot find a Resource with the Name/Key *foo*", and generally fall into one of two 
buckets:

1. The referenced resource legitimately does not exist, e.g. the referenced key is misspelled or 
the intended matching resource has not been added to the app.
2. The referenced resource *does* exist in the app, but it is not reachable from the reference 
at run-time.

Unfortunately, the stowed exception message is the *only* information provided about the error, 
which makes it difficult or, more often, outright impossible to debug.

This spec describes a new 
API on the `DebugSettings` class that developers can use to access more detailed information about 
the resource reference lookup failure.
The new APIs here are patterned after the existing
[DebugSettings.BindingFailed](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.DebugSettings.BindingFailed)
and
[DebugSettings.IsBindingTracingEnabled](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.DebugSettings.IsBindingTracingEnabled)
APIs.


# API Pages

_(Updates to the 
[DebugSettings](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.DebugSettings)
class)_

## DebugSettings.XamlResourceReferenceFailed Event

Occurs when a
[XAML resource reference](https://learn.microsoft.com/en-us/windows/apps/design/style/xaml-resource-dictionary)
cannot be resolved.

_Spec note: This API is similar to the `DebugSettings.BindingFailed` event. Like XAML resource references, 
Bindings cannot be modeled solely through static evaluation, e.g. at compile-time, and so a run-time 
event when a failure occurs is the best approach for providing developers with a means of determining the 
root cause._

_Spec note: is there a version of the 'ResourceDictionary and XAML resource references' article that 
links to the Windows App SDK documentation for relevant APIs?_

```c#
public event TypedEventHandler<DebugSettings,XamlResourceReferenceFailedEventArgs> 
XamlResourceReferenceFailed

```

### Remarks

`IsXamlResourceReferenceTracingEnabled` must be `true` in order for this event to be raised.

Error information is also logged to the native debug output when the event is raised,
so attaching a `XamlResourceReferenceFailed` handler yourself is an advanced scenario for 
getting the raw message programmatically.

## DebugSettings.IsXamlResourceReferenceTracingEnabled Property

Gets or sets a value indicating that when a XAML resource reference error occurs,
the `XamlResourceReferenceFailed` event should be raised,
and error information should be logged in the native debug output.

```c# 
public bool IsXamlResourceReferenceTracingEnabled { get; set; }
```

### Remarks

This property is `true` by default. When XAML resource reference tracing is enabled and you 
run your app with the native debugger attached, any  XAML resource reference errors appear 
in the **Output** window in Microsoft Visual Studio.


## XamlResourceReferenceFailedEventArgs Class

Provides event data for the `DebugSettings.XamlResourceReferenceFailed` event.

```c#
public sealed class XamlResourceReferenceFailedEventArgs
```

### Remarks

`XamlResourceReferenceFailedEventArgs` is used for debugging XAML resource references. Register the event 
handler using `DebugSettings`. It will receive a reference to this class. You'll mainly be 
interested in the `Message` value, which you could log or send to **Debug** output.

The message in the event data contains the following information about the failed XAML resource 
reference:

* The URI of the XAML page containing each `ResourceDictionary` that was searched
* The order in which the `ResourceDictionary`s were searched

You can use this information to investigate why the XAML resource reference could not be resolved; 
perhaps the `ResourceDictionary` it is contained in was not in the list of searched 
`ResourceDictionary`s, or perhaps that `ResourceDictionary` was searched which could indicate that 
an incorrect resource key was specified.

Below is an example message from the WinUI Gallery sample app after an incorrect resource reference
(`OutputTextBlockStyl` rather than `OutputTextBlockStyle`) was deliberately inserted. If you know
that the desired resource is defined in `App.xaml`, then the failure to locate it in there is a
strong indicator of an erroneous reference.

Note: The below example output is for illustrative purposes only. The precise format of the message is 
implementation-defined and may change in the future. Applications should not attempt to parse the message.

```
Beginning search for resource with key 'OutputTextBlockStyl'.
  Searching dictionary 'ms-appx:///Controls/ControlExample.xaml' for resource with key 'OutputTextBlockStyl'.
  Finished searching dictionary 'ms-appx:///Controls/ControlExample.xaml'.
  Searching dictionary 'Framework-defined colors' for resource with key 'OutputTextBlockStyl'.
  Finished searching dictionary 'Framework-defined colors'.
  Searching dictionary 'Framework ThemeResources.xbf' for resource with key 'OutputTextBlockStyl'.
    Searching theme dictionary (active theme: 'Light') for resource with key 'OutputTextBlockStyl'.
      Searching dictionary '<anonymous dictionary>' for resource with key 'OutputTextBlockStyl'.
      Finished searching dictionary '<anonymous dictionary>'.
    Finished searching theme dictionary (active theme: 'Light').
  Finished searching dictionary 'Framework ThemeResources.xbf'.
  Searching dictionary 'ms-appx:///App.xaml' for resource with key 'OutputTextBlockStyl'.
    Searching merged dictionary with index '1' for resource with key 'OutputTextBlockStyl'.
      Searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/themeresources.xaml' for resource with key 'OutputTextBlockStyl'.
        Searching theme dictionary (active theme: 'Light') for resource with key 'OutputTextBlockStyl'.
          Searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/themeresources.xaml' for resource with key 'OutputTextBlockStyl'.
          Finished searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/themeresources.xaml'.
        Finished searching theme dictionary (active theme: 'Light').
      Finished searching dictionary 'ms-appx:///Microsoft.UI.Xaml/Themes/themeresources.xaml'.
    Finished searching merged dictionary with index '1'.
    Searching merged dictionary with index '0' for resource with key 'OutputTextBlockStyl'.
      Searching dictionary 'ms-appx:///ItemTemplates.xaml' for resource with key 'OutputTextBlockStyl'.
      Finished searching dictionary 'ms-appx:///ItemTemplates.xaml'.
    Finished searching merged dictionary with index '0'.
    Searching theme dictionary (active theme: 'Light') for resource with key 'OutputTextBlockStyl'.
      Searching dictionary 'ms-appx:///App.xaml' for resource with key 'OutputTextBlockStyl'.
      Finished searching dictionary 'ms-appx:///App.xaml'.
    Finished searching theme dictionary (active theme: 'Light').
  Finished searching dictionary 'ms-appx:///App.xaml'.
Finished search for resource with key 'OutputTextBlockStyl'.
```


## XamlResourceReferenceFailedEventArgs.Message Property

A human-readable explanation (in English) of the XAML resource reference failure.

```c#
public string Message { get; }
```


# API Details
```c#
namespace Microsoft.UI.Xaml
{
  runtimeclass DebugSettings
  {
    // existing ...

    Boolean IsXamlResourceReferenceTracingEnabled;
    event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.DebugSettings,Microsoft.UI.Xaml.XamlResourceReferenceFailedEventArgs> XamlResourceReferenceFailed;
  };

  runtimeclass XamlResourceReferenceFailedEventArgs
  {
    String Message{ get; };
  };
}

```

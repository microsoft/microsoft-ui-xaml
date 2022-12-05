Tracing XAML resource reference lookup failures
===

# Background

Failure to resolve a XAML resource reference (`{StaticResource}` or `{ThemeResource}`) is one of 
the most common causes of app crashes. Such failures manifest as a stowed exception with the 
message "Cannot find a Resource with the Name/Key *foo*", and generally fall into one of two 
buckets:

1. The referenced resource legitimately does not exist, e.g. the referenced key is misspelled or 
the intended matching resource has not been added to the app.
2. The referenced resource *does* exist in the app, but it is not reachable from the reference 
at run-time.

Unfortunately, the stowed exception message is the *only* information provided about the error, 
which makes it difficult or, more often, outright impossible to debug. This spec describes a new 
API on the `DebugSettings` class that developers can use to access more detailed information about 
the resource reference lookup failure.

# API Pages

_(Each of the following L2 sections correspond to a page that will be on docs.microsoft.com)_

## DebugSettings.XamlResourceReferenceFailed Event

Occurs when a [XAML resource reference](https://learn.microsoft.com/en-us/windows/apps/design/style/xaml-resource-dictionary) cannot be resolved.

_Spec note: is there a version of the 'ResourceDictionary and XAML resource references' article that 
links to the Windows App SDK documentation for relevant APIs?_

```c#
public event TypedEventHandler<DebugSettings,XamlResourceReferenceFailedEventArgs> 
XamlResourceReferenceFailed

```

### Remarks

`IsXamlResourceReferenceTracingEnabled` must be `true` and there must be a debugger attached to 
the app process in order for `XamlResourceReferenceFailed` to fire and for tracing to appear in 
debugger output. You don't need to handle the event in order to see tracing appear in a debugger. 
The debugger output contains message information that goes to the **Output** window of the 
debugger. Attaching a `XamlResourceReferenceFailed` handler yourself is an advanced scenario for 
when you want to see the raw message.



## DebugSettings.IsXamlResourceReferenceTracingEnabled Property

Gets or sets a value that indicates whether to engage the XAML resource reference tracing feature 
of Microsoft Visual Studio when the app runs.

```c# 
public bool IsXamlResourceReferenceTracingEnabled { get; set; }
```

### Remarks

This property is `true` by default, but for XAML resource reference tracing to work, you must also 
enable **Native debugging** in Microsoft Visual Studio on the **Debug** page of the project designer.

When XAML resource reference tracing is enabled and you run your app with the debugger attached, any 
XAML resource reference errors appear in the **Output** window in Microsoft Visual Studio.


## XamlResourceReferenceFailedEventArgs Class

Provides event data for the `DebugSettings.XamlResourceReferenceFailed` event.

```c#
public sealed class XamlResourceReferenceFailedEventArgs
```

### Remarks

`XamlResourceReferenceFailedEventArgs` is used for debugging XAML resource references, using a 
technique that you shouldn't include in production code. Wire the event handler using 
`DebugSettings`, and use this data class as the result in your handler. You'll mainly be interested
in the `Message` value, which you could log or send to **Debug** output.

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

Gets the explanation of the XAML resource reference failure.

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
    XamlResourceReferenceFailedEventArgs();
    String Message{ get; };
  };
}

```


# Appendix

This API is modeled on the existing `DebugSettings.BindingFailed` event. Like XAML resource references, classic Bindings cannot be modeled solely through static evaluation, e.g. at compile-time, and so a run-time event when a failure occurs is the best approach for providing developers with a means of determining the root cause.
# Spec: WebView2 Support for Custom Environment/Options

## Background
Edge `CoreWebView2` exposes considerable browser-level configuratbility through the constructors of
`CoreWebView2Environment` and `CoreWebView2Controller`  APIs, providing programmatic access to
important scenarios such as launching alternative WebView2Runtime installs, specifying a custom
UserDataFolder, managing Profiles, opting into InPrivate browsing, and much more. Today, WinAppSDK
(in constrast to WPF) does not support selecting these customizations programatically, since the
WebView2 API does not provide any way to incorporate custom envrionment/controller settings into
WebView2 intialization. Developers can still achieve most configurability scenarios by using reg
keys and/or environment variable overrides for associated properties, but this roundabout approach
is inconvenient. In addition, some newer CoreWebView2 APIs (e.g. Profiles) are being added without
reg key/env. variable support, making API exposure of these configuraiton knobs essential. The
accompanying feature proposal adds a variant of WebView2 initialization API that accepts customized
environments and controller settings, giving app developers programmatic access to the full range of
customized browser experiences supported by CoreWebView2/WebView2Runtime.

_Spec notes: Partial parity with WPF_

_This functionality has long been available in WPF, where it is exposed in two ways:_

_1.Programmatically, by means of [EnsureCoreWebView2Async()](https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.winforms.webview2.ensurecorewebview2async?view=webview2-dotnet-1.0.2151.40) overloads that allows developers to pass in customized
`CoreWebView2Environment` and `CoreWebView2ControllerOptions` objects._ _It gives developers full
programmatic control over WV2Runtime configurability, but does not support markup manipulation or
Binding._

_2. In markup, by means of a  Xaml "property bag" type
[CoreWebView2CreationProperties](https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.wpf.corewebview2creationproperties?view=webview2-dotnet-1.0.2151.40),
that exposes several of the commonly used CWV2 creation parameters._ _This allows customized
WebView2s to be declared in markup, and supports targeting the properties via Bindings._
_CoreWebView2CreationProperties has five Properties:_
_* BrowserExecutableFolder (normally passed into Environment creation)_
_* IsInPrivateModeEnabled (normally property on ControllerOptions)_
_* Language (normally property on EnvironmentOptions)_
_* ProfileName (normally property on ControllerOptions)_
_* UserDataFolder (normally passed into Environment creation)_

_For WinAppSDK, we will implement only (1) as this provides full access to browser configurability
from the app. We will not implement (2), as it is a conveneince wrapper which has additional design
concerns with respect to future knobs among other things._

## Conceptual Pages (How To)

We will enable programmatic browser configurability by exposing a new overload of the explicit
CoreWebView2 initialization API
[WebView2.EnsureCoreWebView2Async()](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.webview2.ensurecorewebview2async?view=windows-app-sdk-1.4)
that takes arguments specifying how to configure underlying CoreWebView2 objects. Specifically, it
takes the (possibly customized)
[CoreWebView2Environment](https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/winrt/microsoft_web_webview2_core/corewebview2environment?view=webview2-winrt-1.0.2151.40)
and
[CoreWebView2ControllerOptions](https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/winrt/microsoft_web_webview2_core/corewebview2controlleroptions?view=webview2-winrt-1.0.2151.40)
parameters.

For example, developers may want to specify an alternate path from which to load the WebView2Runtime
("Bring your Own" deployment model), or choose to use a different [UserDataFolder](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/user-data-folder?tabs=win32) (implying a
separate WebView2Runtime process family). For these and other possible customizations, they will
instantiate the Xaml WebView2 using the new creation API, passing a non-null value for at least one
of the parameters:
```csharp
[overload("EnsureCoreWebView2Async")] Windows.Foundation.IAsyncAction EnsureCoreWebView2WithOptionsAsync(
    Microsoft.Web.WebView2.Core.CoreWebView2Environment environment,
    Microsoft.Web.WebView2.Core.CoreWebView2ControllerOptions controllerOptions)
```

Specifically, to customize
[browserExecutableFolder](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution#details-about-the-fixed-version-runtime-distribution-mode),
[UserDataFolder](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/user-data-folder?tabs=win32), and any configuration attribute set via
[CoreWebView2EnvironmentOptions](https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/winrt/microsoft_web_webview2_core/corewebview2environmentoptions?view=webview2-winrt-1.0.2151.40), developers will set the `environment` parameter to the `CoreWebView2Environment` instance they obtained by appropriately calling [CoreWebView2Environment.CreateWithOptionsAsync]((https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/winrt/microsoft_web_webview2_core/corewebview2environment?view=webview2-winrt-1.0.2151.40#createwithoptionsasync)):
```c++
static IAsyncOperation<CoreWebView2Environment> CreateWithOptionsAsync(
        string browserExecutableFolder,
        string userDataFolder,
        CoreWebView2EnvironmentOptions options)
```
If the environement is not being customized, `environment` parameter should be set to null.

Alternatively, to customize the settings managed by
[CoreWebView2ControllerOptions](https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/winrt/microsoft_web_webview2_core/corewebview2controlleroptions?view=webview2-winrt-1.0.2151.40)
∈ {`IsPrivateModeEnabled`, `ProfileName`, and `ScriptLocale`}, developers will set the
`controllerOptions` parameter to a `CoreWebView2ControllerOptions` instance they created using the
[CoreWebView2Environment.CreateCoreWebView2ControllerOptions()](https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/winrt/microsoft_web_webview2_core/corewebview2environment?view=webview2-winrt-1.0.2151.40#createcorewebview2controlleroptions) function: and set each property accordingly. 

Again, if none of the controller options are being customized, the `controllerOptions` parameter
should be set to null.

Note that `EnsureCoreWebView2Async(null, null)` is equivalent to the default form
[EnsureCoreWebView2Async()](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.webview2.ensurecorewebview2async?view=windows-app-sdk-1.4).

### Example
Create a WebView2 with custom UserDataFolder, "zh-Hans" language preference, and running in private
mode.

**XAML**
```xml
<Page
    x:Class="WV2Sample.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <StackPanel Orientation="Vertical">
        <WebView2 x:Name="MyWebView2" Width="640" Height="480"/>
    </StackPanel>
</Page>
```

**C#**
```csharp
    // Create a custom environment
    var exePath = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
    var userDataFolder = exePath + ".CustomUDF";

    CoreWebView2EnvironmentOptions envOptions = new CoreWebView2EnvironmentOptions();
    envOptions.Language = "zh-Hans";

    CoreWebView2Environment environment =
        await Microsoft.Web.WebView2.Core.CoreWebView2Environment.CreateWithOptionsAsync(
            "" /* browserExecutableFolder */, userDataFolder, envOptions);

    CoreWebView2ControllerOptions conOptions = environment.CreateCoreWebView2ControllerOptions();
    conOptions.IsInPrivateModeEnabled = true;

    // Create the WebView2 using the custom Environment
    await MyWebView2.EnsureCoreWebView2Async(environment, conOptions);
```
### Mixing Default and Custom Configurations
After a CoreWebView2 has been with initialized in some way (either implicitly by setting Source, or
explicitlty with one of the `EnsureCoreWebView2Async()` overloads), it is invalid to later try to
reinitialize the WebView2 instance to a different custom configuration - doing so will raise an
`ArgumentException``. An allowance is made for ensuring default confgurations - for convenience and
conceptual consistency with _Ensure*_ pattern, such calls succeed as long as some configuration was
successfully initialized previously.

_Spec notes: Comparing Configurations_

_**CoreWebView2Environment** objects are tested for pointer equality, since a CoreWebView2 instance is associated with a specific (and, for Xaml WebView2, unique) CWV2Environment instance._

_**CoreWebView2ControllerOptions** onjects are tested for member value equality, as this object is just a bag of (3) PODs specifying some creation settings._

**C#**
### Example 1:
```csharp
    myWebView2.Source = myURl; // First initialzation - default config
    // ... CoreWebViewe2IntializationCompleted fires
    await myWebView2.EnsureCoreWebView2Async(customEnvironment, null);  // raises ArgumentException
```
### Example 2:
```csharp
    await myWebView2.EnsureCoreWebView2Async(customEnvironment, null); // First initialzation
    // ... CoreWebViewe2IntializationCompleted fires
    await myWebView2.EnsureCoreWebView2Async();           // Success (returns immediately)
    await myWebView2.EnsureCoreWebView2Async(null, null); // Success (returns immediately)
    await myWebView2.EnsureCoreWebView2Async(customEnvironment, customControllerOptions); // raises ArgumentException
```

### Implications of customizing UserDataFolder
Creating WebView2s with different `UserDataFolder` configurations in an app has the added
implication that a distinct WebView2Runtime process group is spawned to back each UDF variation - a
significant performance penalty. While this may be useful in some circumstances, developers should
try to avoid this unless it truly serves the scenario (eg providing distinct persistent storage for
each user of the app). Further details on information on UDF management and associated performance
implications can be found in [Manage user data
folders](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/user-data-folder?tabs=win32#avoid-running-too-many-folders-at-once).

### Default Language setting
For consistency across the Xaml framework, the default configuration applies the Language
preference of the WebView2 element. This is done by hooking up
CoreWebViewEnvironmentOptions.Language to the first entry in the list obtained from
[Windows.Globalization.ApplicationLanguages.Languages](https://learn.microsoft.com/en-us/uwp/api/windows.globalization.applicationlanguages.languages?view=winrt-22621).
For custom configurations, consider setting language similarly if appropriate.

## API Pages

### EnsureCoreWebView2Async(CoreWebView2Environment, CoreWebView2ControllerOptions) overload
Initializes the WebView2 with a CoreWebView2 instance created from the provided
[CoreWebView2Environment](https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/winrt/microsoft_web_webview2_core/corewebview2environment?view=webview2-winrt-1.0.2151.40)
and
[CoreWebView2ControllerOptions](https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/winrt/microsoft_web_webview2_core/corewebview2controlleroptions?view=webview2-winrt-1.0.2151.40).

```csharp (but really MIDL3)
    // Initialize CoreWebView2 with custom CoreWebView2Environment/CoreWebView2ControllerOptions
    [overload("EnsureCoreWebView2Async")] Windows.Foundation.IAsyncAction EnsureCoreWebView2WithOptionsAsync(
         Microsoft.Web.WebView2.Core.CoreWebView2Environment environment,
         Microsoft.Web.WebView2.Core.CoreWebView2ControllerOptions controllerOptions);
```

### Example
Initialize WebView2 with the underlying CoreWebView2 in private browsing mode.

```csharp
    WebView2 MyWebView2 = new WebView2();
    MyWebView2.Width = 320;
    MyWebView2.Height = 200;

    CoreWebView2ControllerOptions conOptions = environment.CreateCoreWebView2ControllerOptions();
    conOptions.IsInPrivateModeEnabled = true;

    // Create the WebView2 in private mode
    await MyWebView2.EnsureCoreWebView2Async(null, conOptions);

    MyWebView2.NavigateToString("You've navigated to a string message in private mode.");
```

### Remarks
To initialize a WebView2 instance to a custom configuration,`EnsureCoreWebView2(customEnvironment,
customControllerOptions)` must be used for the first initialization call. In particular, if Source
has already been set (implicitly applying the default congiration), a custom configuration can no
longer be applied.

## API Details

```csharp (but really MIDL3)
[webhosthidden]
[MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
[MUX_PROPERTY_CHANGED_CALLBACK_METHODNAME("OnPropertyChanged")]
[MUX_PUBLIC]
unsealed runtimeclass WebView2 : Microsoft.UI.Xaml.FrameworkElement
{
    ...

    // Initialize CoreWebView2 with default configuration
    [overload("EnsureCoreWebView2Async")] Windows.Foundation.IAsyncAction EnsureCoreWebView2Async();

    // Initialize CoreWebView2 with custom CoreWebView2Environment/CoreWebView2ControllerOptions
    [overload("EnsureCoreWebView2Async")] Windows.Foundation.IAsyncAction EnsureCoreWebView2WithOptionsAsync(
         Microsoft.Web.WebView2.Core.CoreWebView2Environment environment,
         Microsoft.Web.WebView2.Core.CoreWebView2ControllerOptions controllerOptions);
    ...
}
```
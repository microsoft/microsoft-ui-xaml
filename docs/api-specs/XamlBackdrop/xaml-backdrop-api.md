

Xaml Backdrop APIs
===

# Background

Composition in WinAppSDK has a general
[ISystemBackdropController](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.ISystemBackdropController)
interface that's concretely implemented by
[MicaController](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.MicaController)
and [DesktopAcrylicController](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.DesktopAcrylicController).
A backdrop is a visual effect you can use as the background for your window (or island).
For example the Desktop Acrylic backdrop lets you set as the background of your window the desktop wallpaper behind it, but visually modified.
[More info](https://learn.microsoft.com/en-us/windows/apps/design/style/acrylic).

These existing Mica and Desktop Acrylic controllers allow you to set the backdrops onto a
[WindowId](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.WindowId),
and there's currently only an awkward way to get a `WindowId` from a Xaml object such as
[Window](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Window).

The awkwardness only begins there; you also need to work with a _system_
[DispatcherQueue](https://docs.microsoft.com/uwp/api/Windows.System.DispatcherQueue),
coordinate with the
[Loaded](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.FrameworkElement.Loaded)
event, and check
[IsSupported](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.MicaController.IsSupported)
to see if you need to provide a fallback (Mica isn't supported on Windows 10).

So it's a great feature and works, but it's difficult to use.
[Here's a full code example of the current solution](https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/system-backdrop-controller#example-use-mica-in-a-windows-appsdkwinui-3-app).

The purpose of the new APIs here are to make backdrops much easier to use in a Xaml app,
allowing you to simply set a backdrop as a property on a `Window`.


# API Pages

## Window.SystemBackdrop property

_This description will also apply to `SystemBackdrop` properties on `Popup`, `FlyoutBase`,
and `DesktopWindowXamlSource`._

Sets a `SystemBackdrop` to apply that backdrop to this `Window`.

This backdrop is what will render behind the content specified in `Window.Content`.
If all of the content is fully opaque, this backdrop will have no visible effect.

The following example creates a Window with a Mica backdrop.

```xml
<Window
    x:Class="App3.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >

    <Window.SystemBackdrop>
        <MicaBackdrop/>
    </Window.SystemBackdrop>

    <Grid ColumnDefinitions="Auto,*">

        // This area has a transparent background, so the Mica backdrop will be mostly visible
        // For example if there are buttons here, the backdrop will show up in
        // the margins and gaps between the button text
        <local:NavigationControls/>

        // This area has an opaque background, so the Mica backdrop won't be visible
        <local:ContentArea Background="White"/>

    </Grid>
</Window>
```


## MicaBackdrop class

_Same description applies to `DesktopAcrylicBackdrop` class_

_See also the base class: `SystemBackdrop`_

Use this class to set a backdrop on a
[Window](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Window),
[Popup](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.Primitives.Popup)\*,
or [FlyoutBase](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase)\* (such as a `Flyout` or `MenuFlyout`).
* `Popup`/`MenuFlyout` `SystemBackdrop` support is not  available in WinAppSDK 1.3.0, but is expected to light up in a subsequent release. Note also `SystemBackdrop` will only work in "windowed" popups, i.e. those with *ShouldConstrainToRootBounds=False*.

For example:

```xml
<Window x:Class="MyApp.MainWindow">

    <Window.SystemBackdrop>
        <MicaBackdrop MicaKind="BaseAlt"/>
    </Window.SystemBackdrop>

</Window>
```

Note that `MicaBackdrop` isn't supported on all systems.
In such cases a solid color will be used instead of the Mica effect.


## SystemBackdrop class

Use this class to create a custom system backdrop.
You don't create this class directly, but subclass it to add your custom support (note the protected constructor). This class is the base of built-in backdrop materials `MicaBackdrop` and `DesktopAcrylicBackdrop`.

Generally, the custom material overrides `OnTargetConnected` to create and configure a backing [ISystemBackdropController](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.composition.systembackdrops.isystembackdropcontroller?view=windows-app-sdk-1.3), which will manage the [CompositionBrush](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.composition.compositionbrush?view=windows-app-sdk-1.3) used to fill the backdrop region. The brush's ultimate pixel rendering is affected by the environment/policy (set via [SystemBackdropConfiguration](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.composition.systembackdrops.systembackdropconfiguration?view=windows-app-sdk-1.3)) and material's appearence parameters, exposed via general (eg `ICompositionControllerWithTargets.SystemBackdropState` ∈ { _`Active` / `Fallback` / `HighContast`_ }) and material-specfic (eg `MicaController.Kind` ∈ { _`Base` / `BaseAlt`_} _SystemBackdropController_ properties. 

* The controllers available today (`MicaController` and `DesktopAcrylicContoller`) support the following parameters for customizing their appearence: [FallbackColor](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.composition.systembackdrops.desktopacryliccontroller.fallbackcolor?view=windows-app-sdk-1.3), [LuminosityOpacity](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.composition.systembackdrops.desktopacryliccontroller.luminosityopacity?view=windows-app-sdk-1.3), [TintColor](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.composition.systembackdrops.micacontroller.tintcolor?view=windows-app-sdk-1.3), and [TintOpacity](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.composition.systembackdrops.micacontroller.tintopacity?view=windows-app-sdk-1.3).
* The provided Xaml backdrop materials `MicaBackdrop` and `DesktopAcrylicBackdrop` currently do not expose `FallbackColor` or the material customization properties (`TintOpacity`, etc), instead relying on the default Light/Dark configurations of the underlying `MicaController` and `DesktopAcrylicController`. To customize these properties, createa a custom material class deriving from SystemBackdrop that exposes the desired properties.

The following example shows a simple custom system backdrop class that's implemented using
[MicaController](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.MicaController).


```cs
public class MicaSystemBackdrop : SystemBackdrop
{
    MicaController _micaController;

    protected override void OnTargetConnected(ICompositionSupportsSystemBackdrop connectedTarget, XamlRoot xamlRoot)
    {
        // Calling the base OnTargetConnected initializes the default configuration object 
        // returned by GetDefaultSystemBackdropConfiguration() to reflect environment changes 
        // affecting this SystemBackdrop instance (specified by {connecteedTarget, XamlRoot}).
        base.OnTargetConnected(connectedTarget, xamlRoot);

        // Although this sample does not support sharing MicaSystemBackdrop instances 
        // (eg Window1.SystemBackdrop = Window2.SystemBackdrop = myMicaSystemBackdrop) 
        // such sharing is possible and is supported by the built-in DestkopAcrylicBackdrop and 
        // MicaBackdrop materials. To implement sharing in a custom material, vectorize the 
        // backing ICompositionController storage to keep a separate instance for each usage 
        // context (eg via map keyed on connectedTarget). Note that SystemBackdrop similarly 
        // vectorizes the associated default configuration objects returned by 
        // GetDefaultSystemBackdropConfiguration(connectedTarget, xamlRoot), ensuring 
        // the configuration reflects each usage context when SystemBacdkrop is shared.
        if(_micaController != null)
        {
            throw new Exception("This controller cannot be shared");
        }

        SetControllerConfig(connectedTarget, xamlRoot);

        _micaController = new MicaController();
        _micaController.AddSystemBackdropTarget(connectedTarget);
    }

    protected override void OnTargetDisconnected(ICompositionSupportsSystemBackdrop disconnectedTarget)
    {
        base.OnTargetDisconnected(disconnectedTarget);

        _micaController.RemoveSystemBackdropTarget(disconnectedTarget);
        _micaController = null;
    }

    void SetControllerConfig(ICompositionSupportsSystemBackdrop connectedTarget, XamlRoot xamlRoot)
    {
        var config = GetDefaultSystemBackdropConfiguration(connectedTarget, xamlRoot);
        _micaController.SetSystemBackdropConfiguration(config);
    }
}
```

## SystemBackdrop.GetDefaultSystemBackdropConfiguration method

Returns a default 
[SystemBackdropConfiguration](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.SystemBackdropConfiguration)
object that is set and maintained automatically for each `SystemBackdrop` usage context (i.e `connectedTarget`), and can be passed to
[ISystemBackdropControllerWithTargets.SetSystemBackdropConfiguration](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.ISystemBackdropControllerWithTargets.SetSystemBackdropConfiguration) to apply the default policy.

The properties on `SystemBackdropConfiguration` you receive may change over time:

* `Theme`: set based on [ElementTheme](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.ElementTheme) of the target element (obtained from xamlRoot.Content)
   * If `FallbackColor`, `LuminosityOpacity`, `TintColor`, or `TintOpacity` are modified by the material's implementation, changes to associated [SystemBackdropConfiguration.Theme](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.composition.systembackdrops.systembackdropconfiguration.theme?view=windows-app-sdk-1.3) will no longer automatically toggle the controller's theme (since  default Dark and Light configurations are no longer apopropriate). In this case, the material's appearence properties need to be manually updated to match the new Theme in `SystemBackdrop.OnDefaultSystemBackdropConfigurationChanged`.

* `IsInputActive`: true if the target of the `SystemBackdrop` is in the active window.

* `IsHighContrast`: true if the target of the `SystemBackdrop` is in high contrast mode.


## SystemBackdrop.OnDefaultSystemBackdropConfigurationChanged virtual protected method

Override this method to be called when one of the properrties (described above) on the object returned by `GetDefaultSystemBackdropConfiguration` has changed.

* This is useful when implementing a custom `SystemBackdropConfiguration` that incorporates some of the tracked property states but is different in some way from the default policy. First, obtain a  `SystemBackdropConfiguration` object from `GetDefaultSystemBackdropConfiguration` as usual, but do not apply it via `CompositionContoller.SetSystemBackdropConfiguration`. Instead  create an additional `SystemBackdropConfiguration` object that is updated on `OnDefaultSystemBackdropConfigurationChanged` by applying custom logic to the tracked property (eg ignoring Theme change). The second `SystemBackdropConfiguration` object is hooked up to `CompositionContoller.SetSystemBackdropConfiguration`, applying the custom policy.

## Other `SystemBackdrop` members

| Member | Description |
| - | - |
| `OnTargetConnected` virtual method | Called when this object is attached to a valid container, for example when set on `Window.SystemBackdrop`. Override to create and configure the underlying `CompositionController` and its `SystemBackdropConfiguration`. |
| `OnTargetDisconnected` virtual method | Called when this object is cleared from its container |



# API Details

```c# (but really MIDL3)
namespace Microsoft.UI.Xaml.Media
{
  [contract(Microsoft.UI.Xaml.WinUIContract, 4)]
  [webhosthidden]
  unsealed runtimeclass MicaBackdrop : Microsoft.UI.Xaml.Media.SystemBackdrop
  {
      MicaBackdrop();

      Microsoft.UI.Composition.SystemBackdrops.MicaKind Kind { get; set; };

      static Microsoft.UI.Xaml.DependencyProperty KindProperty { get; };
  }

  [contract(Microsoft.UI.Xaml.WinUIContract, 4)]
  [webhosthidden]
  unsealed runtimeclass DesktopAcrylicBackdrop : Microsoft.UI.Xaml.Media.SystemBackdrop
  {
      DesktopAcrylicBackdrop();
  }

  [contract(Microsoft.UI.Xaml.WinUIContract, 4)]
  [webhosthidden]
  [constructor_name("Microsoft.UI.Xaml.Media.ISystemBackdropFactory")]
  unsealed runtimeclass SystemBackdrop
      : Microsoft.UI.Xaml.DependencyObject
  {
      [method_name("CreateInstance")] protected SystemBackdrop();
      
      overridable void OnTargetConnected(
        Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop connectedTarget,
        Microsoft.UI.Xaml.XamlRoot xamlRoot);

      overridable void OnTargetDisconnected(
        Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop disconnectedTarget);

      Microsoft.UI.Composition.SystemBackdrops.SystemBackdropConfiguration GetDefaultSystemBackdropConfiguration(
        Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop target, 
        Microsoft.UI.Xaml.XamlRoot xamlRoot);

      overridable void OnDefaultSystemBackdropConfigurationChanged(
        Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop target, 
        Microsoft.UI.Xaml.XamlRoot xamlRoot);
  };
}

namespace Microsoft.UI.Xaml
{
  [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
  [webhosthidden]
  [contentproperty("Content")]
  unsealed runtimeclass Window
  {
    // ... existing ...

      [contract(Microsoft.UI.Xaml.WinUIContract, 4)]
      {
          Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop;
      }    
  }
}

namespace Microsoft.UI.Xaml.Controls.Primitives
{
  [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
  [webhosthidden]
  [contentproperty("Child")]
  runtimeclass Popup
      : Microsoft.UI.Xaml.FrameworkElement
  {
    // ... Existing ...

    [contract(Microsoft.UI.Xaml.WinUIContract, 4)]
    {
        Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop;
        static Microsoft.UI.Xaml.DependencyProperty SystemBackdropProperty{ get; };
    }  
  }

  [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
  [webhosthidden]
  [constructor_name("Microsoft.UI.Xaml.Controls.Primitives.IFlyoutBaseFactory")]
  unsealed runtimeclass FlyoutBase
      : Microsoft.UI.Xaml.DependencyObject
  {
    // ... existing ...

    [contract(Microsoft.UI.Xaml.WinUIContract, 4)]
    {
        Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop;
        static Microsoft.UI.Xaml.DependencyProperty SystemBackdropProperty{ get; };
    }  
  }

}
```


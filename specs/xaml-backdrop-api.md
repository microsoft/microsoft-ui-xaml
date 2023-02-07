

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

_This description also applies to `Popup.SystemBackdrop` and `FlyoutBase.SystemBackdrop` properties_

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
[Popup](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.Primitives.Popup),
or [FlyoutBase](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase)
(such as a `Flyout` or `MenuFlyout`).

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
You don't create this class directly, but subclass it to add your custom support.

This class is the base of system backdrop classes: `MicaBackdrop` and `DesktopAcrylicBackdrop`.

The following example shows a custom system backdrop class that's implemented using
[MicaController](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.MicaController).


```cs
public class MicaSystemBackdrop : SystemBackdrop
{
    MicaController _micaController;

    protected override void OnTargetConnected(ICompositionSupportsSystemBackdrop connectedTarget, XamlRoot xamlRoot)
    {
        base.OnTargetConnected(connectedTarget, xamlRoot);

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

## SystemBackdrop.GetDefaultSystemBackdropConfiguration protected method

Returns a
[SystemBackdropConfiguration](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.SystemBackdropConfiguration)
object that is set and maintained automatically, and can be passed to
[ISystemBackdropControllerWithTargets.SetSystemBackdropConfiguration](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.ISystemBackdropControllerWithTargets.SetSystemBackdropConfiguration).

The properties on `SystemBackdropConfiguration` you receive may change over time:

* `Theme`: set based on the target element's
[ElementTheme](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.ElementTheme)

* `IsInputActive`: true if the target of the `SystemBackdrop` is in the active window.

* `IsHighContrast`: true if the target of the `SystemBackdrop` is in high contrast mode.


## SystemBackdrop.OnDefaultSystemBackdropConfigurationChanged protected method

Override this method to be called when the object returned by `GetDefaultSystemBackdropConfiguration` has changed.

This is useful if you're using a custom `SystemBackdropConfiguration`.
The following example shows a `MicaBackdrop` similar to the previous example,
but always forces the theme to be light.

```cs
protected override void OnSystemDefaultBackdropConfigurationChanged(SystemDefaultBackdropChangedEventArgs e)
{
    SetControllerConfig(e.ConnectedTarget, e.AssociatedRoot);
}

void SetControllerConfig(ICompositionSupportsSystemBackdrop connectedTarget, XamlRoot xamlRoot)
{
    var config = GetDefaultSystemBackdropConfiguration(connectedTarget, xamlRoot);
    config.Theme = SystemBackdropTheme.Light;
    _micaController.SetSystemBackdropConfiguration(config);
}

```



## Other `SystemBackdrop` members

| Member | Description |
| - | - |
| `OnTargetConnected` virtual method | Called when this object is attached to an object, for example when set on `Window.SystemBackdrop` |
| `OnTargetDisconnected` virtual method | Called when this object is cleared from an object |



## SystemDefaultBackdropChangedEventArgs class

Event args for the `OnSystemDefaultBackdropConfigurationChanged` virtual method.
Provides an updated `ICompositionSupportsSystemBackdrop` and `XamlRoot`.



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
  unsealed runtimeclass SystemBackdrop
      : Microsoft.UI.Xaml.DependencyObject
  {
      protected SystemBackdrop();
      
      overridable void OnTargetConnected(
        Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop connectedTarget,
        Microsoft.UI.Xaml.XamlRoot xamlRoot);
      overridable void OnTargetDisconnected(
        Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop disconnectedTarget);

      protected Microsoft.UI.Composition.SystemBackdrops.SystemBackdropConfiguration 
        GetDefaultSystemBackdropConfiguration(
          Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop target, 
          Microsoft.UI.Xaml.XamlRoot xamlRoot);

        protected virtual void OnSystemDefaultBackdropConfigurationChanged(
            SystemDefaultBackdropChangedEventArgs e)
  };

  [contract(Microsoft.UI.Xaml.WinUIContract, 4)]
  [webhosthidden]
  unsealed runtimeclass SystemBackdropChangedEventArgs
  {
      Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop ConnectedTarget{ get; };
      Microsoft.UI.Xaml.XamlRoot AssociatedRoot{ get; };
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

# Appendix


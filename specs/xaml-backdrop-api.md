

Xaml Backdrop APIs
===

# Background

Composition in WinAppSDK has a general
[ISystemBackdropController](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.ISystemBackdropController)
API that's concretely implemented by
[MicaController](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.MicaController)
and [DesktopAcrylicController](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.DesktopAcrylicController).
A backdrop is a visual effect you can use as the background for your window (or island).
For example the Desktop Acrylic backdrop lets you set as the background of your window the desktop wallpaper behind it, but visually modified.
[More info](https://learn.microsoft.com/en-us/windows/apps/design/style/acrylic).


These existing Mica and Desktop Acrylic controllers allow you to set the backdrops onto a
[WindowId](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.WindowId),
and there's currently only an awkward way to get a `WindowId` from a Xaml
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

_This also applies to `Popup.SystemBackdrop` and `FlyoutBase.SystemBackdrop` properties_

Set a `SystemBackdrop` to apply that backdrop to this `Window`.

This backdrop is what will render behind the content specified in `Window.Content`.
If the content is opaque, this backdrop will have no visible effect.

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
        // For example if there are buttons here, the backdrop will show up in the margins and gaps between the buttons
        <local:NavigationControls/>

        // This area has an opaque background, so the Mica backdrop won't be visible
        <local:ContentArea Background="White"/>

    </Grid>
</Window>
```


## MicaBackdrop class

_Same description applies to `DesktopAcrylicBackdrop` class_

_See also the base class: `SystemBackdrop`_

Use this class to set a backdrop on your `Window` or similar objects.

For example:

```xml
<Window x:Class="MyApp.MainWindow">

    <Window.SystemBackdrop>
        <MicaBackdrop MicaKind="BaseAlt"/>
    </Window.SystemBackdrop>

</Window>
```


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

    // When set as the value of a backdrop property, such as `Window.SystemBackdrop`,
    // create and set up a `MicaController` 
    protected override void OnTargetConnected(ICompositionSupportsSystemBackdrop connectedTarget, XamlRoot xamlRoot)
    {
        base.OnTargetConnected(connectedTarget, xamlRoot);

        // This implementation doesn't support sharing -- using the same instance on multiple elements.
        // If it did, it would maintain multiple Mica controllers, one for each target.
        if(_micaController != null)
        {
            throw new Exception("This controller cannot be shared");
        }

        // Get a `SystemBackdropConfiguration` from the pass class that's required for the `MicaController`
        var config = GetSystemBackdropConfiguration(connectedTarget, xamlRoot);

        //Create, configure, and set the `MicaController`
        _micaController = new MicaController();
        _micaController.SetSystemBackdropConfiguration(config);
        _micaController.AddSystemBackdropTarget(connectedTarget);
    }

    // When cleared from the backdrop property, clean up the MicaController
    protected override void OnTargetDisconnected(ICompositionSupportsSystemBackdrop disconnectedTarget)
    {
        base.OnTargetDisconnected(disconnectedTarget);

        _micaController.RemoveSystemBackdropTarget(disconnectedTarget);
        _micaController = null;
    }
}

```

## SystemBackdrop members

| Member | Description |
| - | - |
| `OnTargetConnected` virtual method | Called when this object is attached to an object, for example when set on `Window.SystemBackdrop` |
| `OnTargetDisconnected` virtual method | Called when this object is cleared from an object |
| `Changed` event | Raised when the configuration has changed, indicating that the `GetSystemBackdropConfiguration` method will return a new value |


## SystemBackdropChangedEventArgs

Event args for the `SystemBackrop.Changed` event.
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
      
      event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Media.SystemBackdrop,Microsoft.UI.Xaml.Media.SystemBackdropChangedEventArgs> Changed;

      Microsoft.UI.Composition.SystemBackdrops.SystemBackdropConfiguration 
        GetSystemBackdropConfiguration(
          Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop target, 
          Microsoft.UI.Xaml.XamlRoot xamlRoot);

      overridable void OnTargetConnected(Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop connectedTarget, Microsoft.UI.Xaml.XamlRoot xamlRoot);

      overridable void OnTargetDisconnected(Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop disconnectedTarget);
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


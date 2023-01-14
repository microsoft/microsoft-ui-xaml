

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


These Mica and Desktop Acrylic controllers allow you to set the backdrops onto a
[WindowId](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.WindowId),
and there's currently only an awkward way to get a `WindowId` from a Xaml
[Window](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Window).

The awkwardness only begins there; you also need to work with a _system_
[DispatcherQueue](https://docs.microsoft.com/uwp/api/Windows.System.DispatcherQueue),
coordinate with the
[Loaded](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.FrameworkElement.Loaded)
event, and check
[IsSupported](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Composition.SystemBackdrops.MicaController.IsSupported)
to see if you need to provide a fallback.

So it's a great feature and works, but it's difficult to use.
[Here's a full code example of the current solution](https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/system-backdrop-controller#example-use-mica-in-a-windows-appsdkwinui-3-app).

The purpose of the new APIs here are to make backdrops much easier to use in a Xaml app,
allowing you to simply set a backdrop as a property on a `Window`.

# Conceptual pages




# API Pages

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

This class is the base of system backdrop classes,
such as `MicaBackdrop` and `DesktopAcrylicBackdrop`.




## SystemBackdropChangedEventArgs

## Window.SystemBackdrop property

_This also applies to `Popup.SystemBackdrop` and `FlyoutBase.SystemBackdrop` properties_





## MyExample class

Brief description of this class.

Introduction to one or more example usages of a MyExample class:

```c#
void SampleMethod() 
{
  var show = new MyExample();
  show.SomeMembers = AndWhyItMight(be, interesting)
}
```
Remarks about the MyExample class. For example,
APIs should only throw exceptions in exceptional conditions; basically,
only when there's a bug in the caller, such as argument exception.  But if for some
reason it's necessary for a caller to catch an exception from an API, call that
out with an explanation either here or in the Examples

## MyExample.PropertyOne property

Brief description of the MyExample.PropertyOne property.

Paragraph with more detail about the property.

_Spec note: internal comment about this property that won't go into the public docs._

Introduction to one or more usages of the MyExample.PropertyOne property.

```c#
...
```

## Other MyExample members

| Name | Description |
|-|-|
| PropertyTwo | Brief description of the PropertyTwo property (defaults to ...) |
| MethodOne | Brief description of the MethodOne method |

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


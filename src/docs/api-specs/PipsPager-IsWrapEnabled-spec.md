
PipsPager.WrapMode
===

# Background
The WinUI [PipsPager](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.PipsPager) control is used to let the user move through items in a list.

![alt text](PipsPager.png)

Today when a user navigates to the first or last index in a PipsPager, the navigation button in that direction disappears.
Let us say the user is on the first pip and wants to get to the last pip. Under this implementation, they would have to navigate through
all the pips in order to get to their destination.

This spec adds the WrapMode property which, when set to PipsPagerWrapMode::Wrap, allows the user to jump between the first and last index with one click of a button. When set to PipsPagerWrapMode::Wrap, the navigation buttons do not disappear and instead the PipsPager navigates in a wrap around manner to the next logical pip.

![alt text](PipsPager-WrapAround.gif)

# API Pages

## PipsPager.WrapMode property

Gets or sets the WrapMode property which controls whether wrap around behavior in PipsPager. 
This property is set to PipsPagerWrapMode::None by default.

You should set this property to PipsPagerWrapMode::Wrap when you need to display a large, non-infinite, amount of pips.
This will allow for faster navigation between pips that are far apart.

Setting this property to PipsPagerWrapMode::Wrap changes the behavior of the navigation button visibility. Below is the
behavior for the possible values of [PreviousButtonVisibility](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.pipspager.previousbuttonvisibility?view=windows-app-sdk-1.5) and [NextButtonVisibility](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.pipspager.nextbuttonvisibility?view=windows-app-sdk-1.5):

- **Collapsed:** The button is not visible to the user and does not take up layout space. (Default)
- **Visible:** The button is visible and enabled. If WrapMode is set to PipsPagerWrapMode::None (Default), then each button is automatically hidden when the PipsPager is at the first or last pip. For example, if the current page is the first page then the previous button is hidden; if the current page is the last page then the next button is hidden. When hidden, the button is not visible but does take up layout space. If WrapMode is set to PipsPagerWrapMode::Wrap, then the button is always visible.
- **VisibleOnPointerOver:** The behavior is the same as Visible except that the button is only displayed when the user hovers the pointer cursor over the PipsPager UI, or the user sets keyboard focus on the PipsPager.

_Spec note: PipsPager documentation should be updated to reflect two changes. First is to update the part which describes
what happens when setting different navigation button visibility options (see above for new content). Second is to remove the line that says
"Wrapping between the first and last items is not supported."_

The only change to keyboarding behavior happens when at the first or last index. When using the keyboard to 
interact with the navigation buttons, in scenarios where the buttons do not disappear (see above), focus stays on the navigation button
instead of moving to the first or last pip. 

The following shows an app enabling the wrap around behavior.

```xml
<PipsPager WrapMode="Wrap"/>
```

# API Details

```c# (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{
  enum PipsPagerWrapMode
  {
      None,
      Wrap
  };

  unsealed runtimeclass PipsPager : Microsoft.UI.Xaml.Controls.Control
  {
    // ...
    PipsPagerWrapMode WrapMode;
    static Microsoft.UI.Xaml.DependencyProperty WrapModeProperty{ get; };
  }
}
```
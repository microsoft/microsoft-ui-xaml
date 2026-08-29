# ContentDialogPlacement.UnconstrainedPopup

## Table of Contents

- [Background](#background)
- [API Pages](#api-pages)
  - [ContentDialogPlacement.UnconstrainedPopup](#contentdialogplacementunconstrainedpopup-1)
- [API Details](#api-details)

## Background

The Xaml
[ContentDialog](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.ContentDialog)
is similar to a modal dialog by default; centered in the app window and requires action before returning to the window.
The behavior is configurable though in the
[ShowAsync](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.ContentDialog.ShowAsync) method. The
[ContentDialogPlacement](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.ContentDialogPlacement)
parameter on that method lets you specify if the dialog is shown in a popup (default) or InPlace, in which case it 
behaves like any other Xaml element and takes up layout space wherever it is in the element tree.

What this spec adds is a new mode of `ContentDialogPlacement`: `UnconstrainedPopup`.
This mode is like the existing `Popup` mode, but the current popup is constrained to stay
within the host window. The new mode is not constrained (internally it creates a new top-level hwnd).

This mode of the `ContentDialog.Show()` is similar to the
[Popup.ShouldConstrainToRootBounds](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.Primitives.Popup.ShouldConstrainToRootBounds)
mode, and in fact that's how it's implemented. The new `ContentDialog` API is not exposed as a similar 
`ShouldConstrainToRootBounds` property, because that would create a confusion conflict with the existing
`ContentDialogPlacement` mode.

## API Pages

### ContentDialogPlacement.UnconstrainedPopup

> All fields shown here, the last one is new

| Field | Description |
| - | - |
| Popup	| (**improved description**) The dialog is shown as a popup that's centered in the [XamlRoot](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.XamlRoot). |
| InPlace	| If the dialog has a parent element, the dialog is rooted in the parent's visual tree. Otherwise, it falls back to the Popup behavior. |
| UnconstrainedPopup | (**new**) The dialog is shown as a popup that's centered in the [XamlRoot](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.XamlRoot), but might be larger than it. |

## API Details

```cs
[contract(Microsoft.UI.Xaml.WinUIContract, 1)]
[webhosthidden]
enum ContentDialogPlacement
{
  Popup,
  InPlace,

  // New
  UnconstrainedPopup
}
```

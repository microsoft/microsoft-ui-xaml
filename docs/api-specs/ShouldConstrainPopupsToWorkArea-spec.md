DesktopWindowXamlSource.ShouldConstrainPopupsToWorkArea
===

# Background

This is an API that lets apps specify that their popup-like controls should not be constrained to the work area. This
was an API added to WinUI 2/system Xaml as a private API, and now WinAppSDK developers are needing the same
functionality.

Alternatives considered include:
* Add a bool per popup-like control, which was rejected because an app will likely want to set them all anyways. The
  fact that a popup-like control should be allowed to escape the work area isn't a property of the control or how the
  app is using it, but is a property of the app.
* Use heuristics to determine whether an app lives outside the work area, and automatically mark its popup-like
  controls, which was rejected for being fragile. The work area can change during the lifetime of the app and can be
  modified with a public API. Any heuristics based on window size or positioning also have a chance of getting it wrong,
  in which case the app has no workarounds. The app knows for sure whether it can display outside the work area, so it's
  better to be explicit and have it tell us.

Note: Later when the `XamlIsland` type is introduced, it will need this property as well.

**Note for doc writers**: Add this blurb to Popup, FlyoutBase, ToolTip, ComboBox:
```
Note: By default, this control will automatically be constrained within the [work area](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.windowing.displayarea.workarea) of its display. To change this behavior, set the DesktopWindowXamlSource.ShouldConstrainPopupsToWorkArea property of the Xaml island that contains this control.
```

# API Pages

The "[work
area](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.windowing.displayarea.workarea)"
of a display is the desktop area of the display excluding taskbars, docked windows, and docked tool bars. Typical apps
will constrain themselves to the work area, and this also includes any potential popup-like controls (e.g. Popup,
Flyout, ToolTip, ComboBox dropdown) that are opened by the app.

If an app window is intended to be positioned outside the work area (e.g. a docked tool bar), then its popups should
_not_ constrain themselves to the work area, otherwise they would be nudged to be too far from the UI elements that
they're associated with, creating situations like ToolTips that open far away from the control they describe or
ComboBoxes that open far away from the ComboBox button.

The `ShouldConstrainPopupsToWorkArea` API gives an app a way to specify whether its popups should be constrained to the
work area. The default value is "true", and apps that live outside the work area should specify "false".

## DesktopWindowXamlSource.ShouldConstrainPopupsToWorkArea property

Gets or sets whether popup-like controls (e.g. Popup, Flyout, ToolTip, ComboBox dropdown) should constrain themselves to
the work area. This does not apply retroactively to popup-like controls that are already open.

Note: If a control is constrained to root bounds (by setting its ShouldConstraintToRootBounds property to true), then
the root bounds constraint takes priority over the work area constraint.

<table>
<tr>
  <th>DesktopWindowXamlSource.ShouldConstrainPopupsToWorkArea</th>
  <th colspan=2>Control.ShouldConstrainToRootBounds</th>
</tr>
<tr>
  <td></td>
  <th>false</th>
  <th>true</th>
</tr>
<tr>
  <th>false</th>
  <td>display bounds</td>
  <td>Root bounds</td>
</tr>
<tr>
  <th>true</th>
  <td>Work area</td>
  <td>Root bounds</td>
</tr>
</table>

# API Details

```cs
namespace Microsoft.UI.Xaml.Hosting
{
  runtimeclass DesktopWindowXamlSource
  {
    bool ShouldConstrainPopupsToWorkArea { get; set; }
  }
}
```

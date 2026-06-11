DesktopWindowXamlSource.IsBackgroundTransparent
===

# Background

A XAML island hosted in a Win32 app (via
[`DesktopWindowXamlSource`](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.hosting.desktopwindowxamlsource))
composites its content onto a DirectComposition surface. Historically WinUI 3
enforced an **opaque, theme-colored background** (black or white depending on
the app theme) on that surface, so the pixels that the XAML tree leaves
transparent were *not* transparent in the composition output: the island
rendered as an opaque rectangle that occluded whatever was behind it (the parent
`HWND`'s content or the host window's system backdrop).

This is tracked by [microsoft-ui-xaml#11134](https://github.com/microsoft/microsoft-ui-xaml/issues/11134)
and is the WinUI 3 analogue of the UWP-era
`Windows.UI.Xaml.Hosting.IXamlSourceTransparency.IsBackgroundTransparent`,
which had no Windows App SDK counterpart.

The internal capability to suppress that enforced background already exists
(`CXamlIslandRoot::SetHasTransparentBackground`): the rendering path in
`BaseContentRenderer` skips drawing the enforced root background when the island
root reports a transparent background, and the `XamlIsland` type already enables
it unconditionally so that a `SystemBackdrop` set on the island is not occluded.
However, there was no **public, documented, opt-in property** for an app hosting
a `DesktopWindowXamlSource` to control this directly. Apps therefore had to rely
on indirect or undocumented behavior:

* Setting a per-island `SystemBackdrop` (e.g. `MicaBackdrop`) just to get
  transparency. For a host that overlays **many small islands** on one window
  (one island per native control), this means *N* backdrop controllers for a
  window that conceptually has a single backdrop, an audible/visible
  per-island re-composite "wave" on theme changes, and a deadlock risk if the
  backdrop is set synchronously during island creation.
* `WS_EX_LAYERED` / `SetLayeredWindowAttributes` on the bridge `HWND`, which has
  no effect because the island renders through the composition engine, not GDI.
* Compositor VMT hooks to null out the background visual — unsupported and
  version-fragile.

The concrete motivating scenario is an experimental **wxWidgets → WinUI 3
backend**, where every native control keeps its own Win32 child `HWND` and is
overlaid with one `DesktopWindowXamlSource`, while the top-level window uses a
single Mica backdrop that should show through all of the islands.

Alternatives considered:
* **Make islands always transparent (no API).** Rejected: it would silently
  change the rendering of existing `DesktopWindowXamlSource` apps that rely on
  the opaque background, and it would not be discoverable or documented. An
  explicit, opt-in property lets apps state their intent and keeps the behavior
  controllable.
* **Expose it only through `SystemBackdrop`.** Rejected: tying output
  transparency to having a backdrop controller per island is exactly the
  overhead/"wave"/deadlock problem described above; transparency and backdrops
  are separate concerns.

Note: When the `XamlIsland` type is the recommended hosting primitive, it should
expose the same property (today it hard-codes transparency on).

# API Pages

## DesktopWindowXamlSource.IsBackgroundTransparent property

Gets or sets a value that indicates whether the island composites with output
transparency, i.e. whether the regions that the hosted XAML tree leaves
transparent are transparent in the composition output.

When `true` (the default), the island does **not** draw an enforced
theme-colored background, so the content behind the island — the parent `HWND`'s
pixels, and therefore a single system backdrop applied to the host window — shows
through the transparent regions of the XAML tree. Opaque XAML elements (the
controls themselves) render normally on top.

When `false`, the island fills its background with the opaque, theme-colored
brush, matching the legacy behavior; the island occludes whatever is behind it.

This property only affects *output* transparency (what the island composites);
it does not change input/hit-testing behavior. Setting it does not retroactively
change islands that have already been torn down, and it can be changed at any
time after the source has been initialized.

```cs
// Host the island over a Win32 window that has a single Mica system backdrop,
// and let that backdrop show through every control island.
var source = new DesktopWindowXamlSource();
source.Initialize(myWindowId);
source.IsBackgroundTransparent = true;   // default; transparent regions show the host backdrop
source.Content = myControl;              // e.g. a Button whose own pixels stay opaque
```

# API Details

```cs
namespace Microsoft.UI.Xaml.Hosting
{
    runtimeclass DesktopWindowXamlSource
    {
        // ...existing members...

        // Added in WinUIContract v10.
        Boolean IsBackgroundTransparent { get; set; };
    }
}
```

# Remarks

* The default value is `true`. This matches the current behavior of the island
  root used by `DesktopWindowXamlSource` (and the `XamlIsland` type), which
  composites with output transparency so that a `SystemBackdrop` is not
  occluded. Apps that want the legacy opaque rectangle can set the property to
  `false`.
* With `IsBackgroundTransparent = true` and a single `SystemBackdrop`
  (e.g. Mica) applied to the **host window**, a theme change updates the whole
  window at once: there is no per-island backdrop controller and therefore no
  top-to-bottom re-composite "wave", and there is no need to defer enabling it
  past island creation to avoid a dispatcher-queue deadlock.
* This works for the "many small islands over one Win32 window" hosting model,
  not only for a single full-window island.

# Implementation notes

`IsBackgroundTransparent` is a thin, documented wrapper over the existing
`CXamlIslandRoot::SetHasTransparentBackground` / `HasTransparentBackground`
mechanism:

* `put_IsBackgroundTransparent(value)` calls
  `CXamlIslandRoot::SetHasTransparentBackground(value)` on the source's island
  root.
* `get_IsBackgroundTransparent()` returns
  `CXamlIslandRoot::HasTransparentBackground()`.

The rendering path already honors this flag: `BaseContentRenderer` skips drawing
the enforced root-visual background when the `CXamlIslandRoot` reports a
transparent background, so no rendering changes are required beyond exposing the
property.

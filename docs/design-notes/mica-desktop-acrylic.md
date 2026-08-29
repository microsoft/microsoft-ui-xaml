# Mica/Desktop Acrylic

## Table of Contents

- [Problem](#problem)
- [Lifted Composition API](#lifted-composition-api)
- [Xaml Public API](#xaml-public-api)
  - [SystemBackdrop](#systembackdrop)
- [Interaction with other features](#interaction-with-other-features)
  - [Custom title bar](#custom-title-bar)
  - [TabView and File Explorer's tabbed shell](#tabview-and-file-explorers-tabbed-shell)

## Problem

Acrylic and Mica are materials that can be used as backgrounds in Windows. Acrylic provides a glass-like blur of the
content underneath it. This can be in-app content (e.g. other controls covered up by a Grid that has Acrylic on it),
refered to as "in-app Acrylic", or content in other processes (e.g. other windows covered up by a window with Acrylic on
it), refered to as "desktop Acrylic". Mica provides a glass-like blur of the desktop background, ignoring any other
content underneath. Note that there's no such thing as "in-app Mica", because by design Mica cares about only the
desktop background and no other covered-up content.

When everything was rendered with system Composition, this didn't pose any additional problems - Acrylic & Mica are just
effect brushes and are passed to system DWM along with the rest of the `Visual` tree. The system DWM has access to
everything, including the contents of this window, the contents of all other windows, and the desktop background, so it
can sample and blur from wherever it needs when rendering the Acrylic & Mica effect brushes.

But lifted Xaml uses lifted Composition, which creates a problem. The lifted (i.e. in-proc) compositor isn't system DWM.
It knows about the content in the same window (or, more precisely, the same island), but it doesn't have access to
content of other processes, including the contents of any other window and including the desktop background. It can't
apply a blur to content that it can't access. This means it works fine for the Acrylic Grid that covers up a Button in
the same app, but it can't draw an Acrylic window background that covers other windows or a Mica window background that
blurs the desktop wallpaper. It must rely on system Composition for Mica & desktop Acrylic scenarios.

This means that in the lifted world (i.e. WinAppSDK), Mica and system Acrylic require lifted Xaml, lifted Composition,
and system Composition to all be involved.


## Lifted Composition API

Lifted Composition's API for Mica & desktop Acrylic (collectively referred to as "system backdrops") is the
`SystemBackdropController`. See the SystemBackdropController reference documentation for details.

`SystemBackdropController`'s
`AddSystemBackdropTarget`
method is the entrypoint for setting up system backdrops. It accepts an object that implements the
`Windows::UI::Composition::ICompositionSupportsSystemBackdrop`, does a QI, and sets its SystemBackdrop property.
`ICompositionSupportsSystemBackdrop` is an interface that's implemented by a couple of Xaml objects:
* `Window`, for desktop apps
* `DesktopWindowXamlSource`, for island apps

Since system backdrops are implemented in the system Composition, they require system effect brushes (from
`Windows::UI::Composition`), as opposed to lifted effect brushes (from `Microsoft::UI::Composition`). The
`SystemBackdropController` (itself a lifted type) creates the necessary system effect brushes under the covers. It
passes the system brush into the `ICompositionSupportsSystemBackdrop` parameter of `AddSystemBackdropTarget`.

The Xaml object implementing `ICompositionSupportsSystemBackdrop` receives the system brush and just needs to plumb the
brush through to the Composition `IContentSiteBridge` object. There are two cases:
1. Island apps have a `IContentSiteBridge` directly inside the `DesktopWindowXamlSource`. They plumb straight through.
2. Desktop apps are a special case of island apps where the `DesktopWindowXamlSource` is hidden inside a
   `DesktopWindow`, so accordingly this is just a special case of 1. `DesktopWindow` plumbs through that
   `DesktopWindowXamlSource`.

So, to recap:
1. Xaml calls the Composition `SystemBackdropController::AddSystemBackdropTarget` API, passing in a `Window` or
   `DesktopWindowXamlSource` Xaml object as a param.
2. Composition creates a `Windows::UI::Composition` brush, and calls the Xaml object back via
   `ICompositionSupportsSystemBackdrop::SystemBackdrop` to set it.
3. The Xaml object passes the `Windows::UI::Composition` brush back into Composition's `IContentSiteBridge` object.

This seemingly roundabout sequence, where Xaml calls Composition, who calls Xaml back, who calls Composition back, is
done to avoid having Xaml deal with the `Windows::UI::Composition` brush. That brush is created in Composition in step 2
and is given to Xaml. Xaml does nothing with that brush and passes it straight back to Composition in step 3. This puts
management of that brush entirely in Composition, which is beneficial for Xaml beyond not having to worry about the
brush's lifetime. The effect brush that implements system backdrop also comes with policies such as when to fall back to
a solid color, which can happen when the active window loses focus or when the theme changes to a high-contrast theme.
The fallback transition itself also has a tree of objects and animations that need to be managed. Pushing the brush into
Composition frees up Xaml from having to deal with any of these details. Instead Xaml can just hold a
`SystemBackdropConfiguration` object and tell it when to transition between active and fallback states.


## Xaml Public API

The "prior art" for Acrylic is the WinUI 2
[`AcrylicBrush`](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.media.acrylicbrush?view=winui-2.8)
class (WinUI 3 docs
[here](https://docs.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.media.acrylicbrush?view=windows-app-sdk-1.1)).
The main difference between the WinUI 2 and WinUI 3 types is that WinUI 2 is system Xaml and is backed by system
Composition, so it can be applied anywhere in the tree. In WinUI 3 things are more restricted as described above. While
in-app Acrylic can be applied anywhere in the tree (with the restriction of not being able to blur anything outside its
own island), desktop Acrylic can only be applied to `Window` and `DesktopWindowXamlSource` objects. This creates
problems because the `AcrylicBrush` type is just a Xaml brush and can be put anywhere in the tree, so an app will not be
aware of these restrictions.

There is no "prior art" for Mica. The Mica material was added after WinUI 3 was created, and we didn't add a
corresponding WinUI 2 type for it.

Since WinUI 3 hidden restrictions to desktop Acrylic and no prior art for Mica, we want to define a new API to expose
these features. This includes a new `Microsoft.UI.Xaml.Media.SystemBackdrop` type and new fields on `Window`,
`DesktopWindowXamlSource`, and `Popup` to accept an instance of a `MUX::Media::SystemBackdrop`. The Xaml object then
calls the Composition `SystemBackdropController` API under the covers.

For Acrylic, another option is to reuse the existing `AcrylicBrush`, and give it the ability to call into Composition's
`SystemBackdropController` in desktop Acrylic mode (`BackgroundSource="HostBackdrop"`). While this can work for an
`AcrylicBrush` on the root of the tree, it leads to problems pretty quickly:
* Putting an `AcrylicBrush` with `BackgroundSource="HostBackdrop"` lower down in the tree will have it no-op or fallback
  because it cannot be supported. It's not clear to developers why this would be the case.
* The root can have multiple elements sized to the entire island, and each can have a different `AcrylicBrush` object
  set on it. But there's only one `DesktopWindowXamlSource` to apply desktop Acrylic, so the brushes will have to fight
  to take control of it.
* This doesn't solve the problem for Mica at all. Adding a `MicaBrush` creates even more cases where the brush doesn't
  work, since in-app Mica isn't a thing.

Another option is to expose Composition's `SystemBackdropController` itself from Xaml. This has the advantage of not
introducing any new types and allows adding new types of backdrops with no overhead. However, it doesn't
support sharing scenarios. We want apps to be able to define a single backdrop object in a `ResourceDictionary` and bind
to it from multiple windows, islands, and popups. Each usage of this object has its own unique context, including the
theme and the input activation state, which means they must all have separate Composition `SystemBackdropConfiguration`
objects that carry this information. A `SystemBackdropController` can only take a single `SystemBackdropConfiguration`
object, which means it can't be shared among multiple windows, islands, or popups. The `SystemBackdrop` wrapper in Xaml
can internally keep a map of multiple `SystemBackdropController` and `SystemBackdropConfiguration` objects, keyed by the
place where they're attached, and support this scenario.


### SystemBackdrop

`Microsoft.UI.Xaml.Media.SystemBackdrop` is the base class for the Xaml exposure of Composition's
`SystemBackdropController` and for the easy Mica/desktop Acrylic feature.

* Internally handles theme changed, high contrast changed, and input activation changed events
* Internally keeps a config object up-to-date
* Internally broadcasts a StateChanged event when theme/high contrast/activation changes
  * Apps can listen for this event and get the new state from the config object
* Attached to the tree via `Window`/`DesktopWindowXamlSource`/`Popup`'s `SystemBackdrop` property. Calls virtual
  `OnTargetConnected`/`OnTargetDisconnected` nethods in response to entering/leaving the tree.
* Not responsible for the `SystemBackdropController` itself - that's on the derived class

Benefits:
* Easy to attach to the tree to get Mica/desktop Acrylic behavior
* Easy to add a new backdrop type - the default behavior is all implemented in base so you just create the controller.
  Corollary - easy to add new types of controllers and expose through Xaml
* Possible to add custom behaviors - create a separate config object and set it on the controller. Read state from our
  config object if they want to copy any default policy.

Future:
* Make SystemBackdrop shareable - easier to use from a ThemeDictionary

Issues:
* Multiple places where theme is implemented - IXP's config has a policy that automatically changes colors, and Xaml
  will have another (more explicit) policy that uses {ThemeResource} to bind to different SystemBackdrop objects, which
  themselves will stay light/dark theme
  * Actually, is there a race condition here? A Popup's SystemBackdrop is bound to a ThemeResource, which resolves to
    SystemBackdrop object A on light theme. When the theme changes, the ThemeResource will resolve to SystemBackdrop
    object B for dark theme. Meanwhile, A itself is listening for theme changed, and will flip its config object to dark
    theme. Now there's a race. If A gets the notification first, then it flips its config to dark. Then the
    ThemeResource gets updated to point to object B, which leaves A's config in dark theme. This can be a problem if we
    switch back to light theme and the ThemeResource resolves back to A - its config should be light theme but is
    actually dark.
    * This can be moot if Xaml explicitly sets colors/opacities on the IXP controllers, which removes the automatic
      color switching behavior of the IXP config object. It's still a subtlety that should be called out - if we ever
      use default-value controllers, then we can hit this race condition that causes the SystemBackdrop to be stuck in
      the wrong theme and look wrong when the theme changes.


## Interaction with other features

### Custom title bar

Easy Mica/desktop Acrylic is expected to work with custom title bars. Xaml's custom title bar API extends the client
area of the window into the non-client area. Easy Mica fills the client area, which then extends into the non-client
title bar area.

A test for Mica + custom title bar should verify this scenario works correctly.

### TabView and File Explorer's tabbed shell

TabView has a unique requirement of needing two different looking types of Mica. There's a "thin Mica" that goes in the
background, and regular Mica that goes on the selected tab. Currently File Explorer is implementing this by doing Mica
manually. It sets
`DWMWA_SYSTEMBACKDROP_TYPE`
via `DwmSetWindowAttribute`, which puts thin Mica behind the hwnd. The TabView control itself has a transparent
background to let that thin Mica show through. FE then draws the active tab with a half transparent brush which blends
with the thin Mica underneath to mimic the look of regular Mica. Easy Mica does not break this approach.

It is difficult for Xaml to implement two different types of proper Mica in the TabView control. Specifically, because
the Mica backdrop requires a Composition content bridge, having multiple instances of proper Mica being used means
having multiple islands. The Xaml TabView control would need to be re-implemented with each tab in a separate island,
which is a huge rearchitecture. The approach that FE is taking, with a 50% opaque brush on top of thin Mica to simulate
real Mica, is the only feasible option for TabView.

Currently FE is perfectly fine with this workaround. It introduced some polish problems which were fixed (thin line
under active tab, rendering artifacts near bottom rounded corner of tab that no longer repro).

The plan is to use the 50% opacity as the official solution for TabView. File Explorer itself will continue using
`DWMWA_SYSTEMBACKDROP_TYPE` on its main hwnd and put invisible islands inside, because the main hwnd of FE isn't a Xaml
window so FE can't use Xaml APIs on it.

Make sure TabView works with easy Mica for future File Explorer scenarios.

See the Mica in Tabbed Title Bars design documentation for more details.

# Inking in Xaml

## Table of Contents

- [Overview](#overview)
- [Ink-related Classes](#ink-related-classes)
  - [InkPresenter](#inkpresenter)
  - [InkCanvas](#inkcanvas)
  - [InkToolbar](#inktoolbar)
- [System Xaml Inking](#system-xaml-inking)
- [Lifted Xaml Inking](#lifted-xaml-inking)

## Overview

This document gives an overview of inking in Xaml.

Spoilers: It's DirectInk and Composition that does all of the heavy lifting. Xaml just needs to set things up correctly
and not get in the way.

## Ink-related Classes

### InkPresenter

[`InkPresenter`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.input.inking.inkpresenter?view=winrt-22000) is a
class in the `Windows.UI.Input.Inking` namespace (i.e. outside Xaml) that captures and renders the ink strokes.

`InkPresenter` supports a couple of ways of custom rendering. Secondary input (e.g. pen strokes with the barrel button
pressed) can be processed by the InkPresenter to draw like primary input, erase ink strokes, or be passed unprocessed to
the app. This is controlled by the [`InkPresenter.InputProcessingConfiguration`
property](https://docs.microsoft.com/en-us/uwp/api/windows.ui.input.inking.inkpresenter.inputprocessingconfiguration?view=winrt-22000).
The app can also specify a custom renderer for dry ink strokes by calling
[`InkPresenter.ActivateCustomDrying`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.input.inking.inkpresenter.activatecustomdrying?view=winrt-22000).
See the [Pen interactions and Windows Ink in Windows
apps](https://docs.microsoft.com/en-us/windows/apps/design/input/pen-and-stylus-interactions) article for more details.

### InkCanvas

[`InkCanvas`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.controls.inkcanvas?view=winrt-22000) is a
lightweight Xaml element (deriving from `FrameworkElement`) that serves to plug an `InkPresenter` into the rest of the
Xaml tree. InkCanvas also sizes the InkPresenter by participating in layout with the rest of the Xaml tree.

### InkToolbar

[`InkToolbar`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.controls.inktoolbar?view=winrt-22000) is a Xaml
`Control` that can configure an `InkCanvas`/`InkPresenter`. It can be placed anywhere in the tree and must be explicitly
pointed at an `InkCanvas`/`InkPresenter` via the
[TargetInkCanvas](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.controls.inktoolbar.targetinkcanvas?view=winrt-22000)
and
[TargetInkPresenter](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.controls.inktoolbar.targetinkpresenter?view=winrt-22000)
properties.

For WinUI3 1.2, we're lifting InkCanvas but not InkPresenter. We should avoid exposing the system InkPresenter, which
means the lifted InkToolbar should just point at a lifted InkCanvas.

## System Xaml Inking

System Xaml's entrypoint to inking is the
`InkPresenterInterop::SetRootVisual`
method, which marks a (system) `IDCompositionVisual2` as the root of the ink area. Note that this expects a legacy
`IDCompositionVisual2`, not a WinRT `Visual`. Once that's set up, all input and rendering happen outside Xaml, in other
processes.

System Xaml talks to InkPresenterInterop via a comp node, the
`HWCompInkCanvasNode`.
It has an `IDCompositionVisual2` inside, like all comp nodes, and it passes that visual into
`InkPresenterInterop::SetRootVisual` to plug the InkPresenter into the tree. It also passes the InkCanvas's layout size
down to the `IDCompositionVisual2` acting as the root visual of the InkPresenter.

## Lifted Xaml Inking

Since the entrypoint to inking involves system visuals, we can't do this for lifted. We cut all of ink for WinUI3. Now
that there are customers of WinUI3 that need inking, we're bringing it back. There are a couple of ways to go about
this:
1. Lift DirectInk and InkPresenter. This is expensive, so this is not the way to go to get inking support in 1.2.
2. Consume system DirectInk from lifted Xaml/IXP. IXP is prototyping this to figure out the work involved & the
   feasibility. This is what we're doing for 1.2.

In between Xaml and IXP, there will need to be a way to associate a system InkPresenter with a lifted (WinRT, hopefully)
Visual. We'll need some API from IXP to enable this.

> George's wild guess
> 
> Some method on a lifted Visual that takes an InkPresenter?

There is also an airspace concern to make input work. Over in system Xaml, the entire visual tree lives in dwm.exe,
including the visual with ink enabled and the visuals for the rest of the app's UI. Input goes straight to dwm.exe,
where it can be hit-tested against the tree. If it hits the ink-enabled visual, then it can be used to draw low-latency
ink in the DWM.

Lifted Xaml doesn't work like this. The tree of lifted visuals lives in the app process and is rendered by the in-proc
compositor. The tree that the dwm.exe sees typically has a single visual with a CompositionSurface that has the lifted
visuals drawn into it. Input goes to dwm.exe, hits this single visual, then gets routed to the app where the lifted
compositor does a hit test against the lifted tree that it sees. Once input is routed to the app process, it cannot be
passed back to the dwm for inking. The tree that the dwm sees must contain a system visual already set up for ink. This
visual also can't be obstructed by anything, including other transparent visuals, which can block hit testing and
prevent input from reaching the ink visual.

This means putting an InkCanvas in lifted Xaml should effectively "splay" the lifted Xaml tree, which produces one
section of content underneath the InkCanvas, the InkCanvas itself, and one section of content above the InkCanvas. The
lifted compositor can then turn these into three separate system Visuals to send over to the DWM, and the DWM will be
able to hit test the ink visual.

In practice there are a couple of problems with doing this in 1.2:
1. We don't have the time to implement it. Currently nothing in lifted Xaml splays the tree, and everything renders into
   a single content bridge corresponding to the island.
2. Automatic tree splaying can produce large transparent visuals that block input. If we automatically put everything
   above the InkCanvas into a separate rendering surface, then having something like a button at (0, 0) and another
   button at (1800, 900) will create a large and mostly transparent surface with a button at the top left and another at
   the bottom right. When this turns into a system Visual, it will cover up the ink-enabled system Visual and prevent
   ink from working.

To solve these problems, Xaml can take a different policy for 1.2:
1. InkCanvas goes on top of all Xaml content in the island, aside from windowed popups.
2. Windowed popups will create their own content bridge and render inside.

This allows ink to live in a separate system Visual from the rest of the tree. It also allows apps to render content
above the InkCanvas via windowed popups. Each popup is expected to be small, so we shouldn't have any scenarios where
disjoint pieces of UI get merged and produce a large transparent surface. Xaml will provide explicit sizes for windowed
popup content so we don't unnecessarily block input. This also covers many of the scenarios where we want content to
render on top of ink, including ToolTips, menus, and floating InkToolbars.

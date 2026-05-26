# Xaml Popups

## Table of Contents

- [Requirements](#requirements)
- [Additional constraints](#additional-constraints)
- [Lifted Visual tree](#lifted-visual-tree)
  - [ContentExternalOutputLink](#contentexternaloutputlink)
  - [Proposed IXP API](#proposed-ixp-api)
- [Deliverables & Tasks](#deliverables--tasks)
- [Future](#future)
- [Alternative - Hwnd shadows](#alternative---hwnd-shadows)
- [Alternative - system Visuals](#alternative---system-visuals)

## Requirements

Windowed `Popup` elements used for context menus combine rounded corners, shadows, system backdrops, and animations.
This is a difficult mix of properties to make work. Each requirement adds a set of constraints to the problem.

1. System backdrops can only be implemented in the system DWM because they may need to sample content outside the
process using them (e.g. for desktop Acrylic). This means the Visual with the backdrop effect brush must be a system
Visual, and lifted Xaml has no notion of system Visuals, system Compositors, or system legacy DComp devices. The content
bridge on the lifted IXP side can manage such a system Visual.

2. The addition of rounded corners means the system backdrop Visual must be clipped with a rounded corner clip.
Otherwise it will have square corners that poke out behind the rounded borders of the popup.

3. The addition of shadows means an additional Visual is needed to render the shadow. For performance reasons we
pre-render the shadow into a bitmap and draw it as a Visual, rather than use Composition's Shadow property directly.
This shadow Visual must not be clipped by the rounded corner clip applied to the popup contents/background because it
needs to show up outside the popup's borders.

   Note that there is a repercussion here - since the shadow is a Visual in the tree under the content bridge, it will
be hit tested, and clicks that land on the shadow will be forwarded to the process instead of to whatever is rendering
underneath the shadow. This was a limitation of system Xaml shadows as well.

4. The addition of animations means the three Visuals involved (content, system backdrop, and shadow) must stay
synchronized, otherwise we'll have tearing in the tree where one piece animates ahead of or behind the other pieces.
This means we cannot mix lifted and system Visuals, since lifted Visuals animate in-proc in the lifted Compositor and
system Visuals animate in dwm.exe in the system Compositor. Either all three are drawn by the lifted compositor or all
three are drawn by the system compositor.

## Additional constraints

5. The lifted compositor produces a single surface filled with rendered lifted Visuals. It then displays this surface in
a system Visual. It doesn't have capabilities to splay the tree and render content into two or more surfaces attached to
multiple different system Visuals.

   This creates a conflict with shadows, which need to be clipped to a larger rect than the popup's contents or system
backdrop. It means we either render the shadow (as a lifted Visual) and the content into the same lifted surface, which
means they can only be animated using a lifted animation, which means we have to animate the system backdrop system
Visual using a lifted animation. Luckily this is possible - the lifted Compositor can evaluate the lifted animation and
set a new static position on the system visual every frame.

   Alternatively, we render only the lifted content into the lifted surface, then use a system Visual to render the
shadow, which means we animate everything using system animations.

This creates the two different Visual trees that we can use to solve this problem. We are opting for a tree of lifted
Visuals, outlined below. The system Visual alternative is also included for completeness.

## Lifted Visual tree

The lifted Visual tree animates everything using lifted Composition animations. The content and shadow are rendered into
the same lifted Composition surface. We also use a `ContentExternalOutputLink.PlacementVisual` to connect a lifted
Visual to the system backdrop system Visual, which allows us to animate the system Visual with a lifted animation. This
system Visual is an existing Visual under the content bridge's render target and is exposed to Xaml via a property.

![A windowed popup with rounded corners, shadows, system backdrops, and animations, and its associated lifted Visual
tree](images/windowed-popup-visual-tree-lifted.png)

This is a simplified visual tree that shows only the important Visuals:

1. The PopupWindowSiteBridge internally contains a legacy `IDCompositionTarget` connected to the popup's hwnd. Note that
   every visual under this target is implicitly clipped to the bounds of the hwnd, which also serves as the viewport
   clip for any entrance animations that the popup may have (e.g. sliding in from above).

2. This is the root system Visual for the tree of system Visuals that Composition builds, including both Visuals from
   the content bridge itself as well as Visuals from the render target.

3. This is the system Visual inside the content bridge that is connected to the output of the lifted Compositor. The
   lifted Compositor renders all lifted Visuals into a system Composition surface, which is hooked up to a system
   `CompositionSurfaceBrush` and shown by this system Visual.

4. This is the lifted Visual corresponding to the Composition ContentSite. It's actually composed of a chain of multiple
   visuals. One important function of the ContentSite is that this is where the DPI scale is applied. In future
   cross-proc scenarios, this Visual may live in another process.

5. This is the lifted Visual corresponding to the Composition ContentIsland.

6. This is the root lifted Visual inside Xaml's Visual tree for the popup. This visual is aligned to the bounds of the
   hwnd; its origin is the top-left corner of the shadow region, as opposed to the top-left corner of the popup's
   UIElement content. It's the ancestor that contains the popup contents, the popup shadow, and the popup system backdrop
   proxy visual (explained below in 9 and 10). This is also the visual that will eventually be targeted by any popup
   entrance or exit animations, so that all three pieces move in unison.

7. This is a lifted Visual, generated by Xaml, that contains the shadow around the popup. Xaml draws the shadow as a
   nine grid image of a shadow, rather than use a composition Visual's Shadow property directly, to avoid the lifted
   Compositor re-rendering it every frame where something changes.

8. This is the output of Xaml rendering the popup's content. It's drawn as a single visual here but it will be a tree of
   lifted Visuals, containing text, images, and shapes. It's important that this tree of visuals have a transparent
   background to allow the system backdrop to go through. In practice that means the Xaml UIElement tree can't have a
   UIElement covering the whole popup with an opaque background.

9. This is a system Visual created inside Composition's render target, and is where the system brush with the system
   backdrop is connected. This is the system Visual that draws pixels for the system backdrop (Mica or Acrylic). It is
   connected to a lifted Visual via a `ContentExternalOutputLink`. By default it covers the entire bounds of the hwnd,
   which in this case includes the area drawing the shadow, therefore it needs to have a rounded corner clip set on it
   to limit it to the rounded corner content produced by Xaml.

10. This is the lifted Visual connected to visual 7 above. Xaml gets it from the content bridge and places it in the
   lifted Visual tree where the backdrop should go, above the shadow and below the Xaml content. The
   `ContentExternalOutputLink` then pushes its world transform and opacity to the system Visual every frame, which keeps
   the backdrop locked to the rest of the tree during the popup's entrance and exit animations. Xaml also sets a rounded
   corner clip on this Visual so that the `ContentExternalOutputLink` can set a matching rounded corner clip on the
   system Visual.

The lifted Visual tree has the benefit of reusing lots of existing code:
* Lifted Xaml already knows how to build the lifted Composition animations for a popup's entrance/exit animations.
* Lifted Xaml already has code to render a shadow as a lifted Visual.

Shadow hit testing is the main drawback of this approach - it's impossible to get correct given this tree structure. By
keeping the shadow in the same surface as the content, it will be hit testable, and there's no way to solve that without
rearchitecting the tree. Note that this is parity with system Xaml - there Xaml also draws its own shadows that eat
clicks. To solve this we'll need to separate the shadow into its own system Visual as well as some hit testing changes
in system Composition to allow that system visual to be invisible to the initial hit test walk.

### ContentExternalOutputLink

`ContentExternalOutputLink` punches a hole through the lifted surface at the top to reveal the system Visual that
the lifted surface covers up. It's capable of punching rounded corners as well.

`ContentExternalOutputLink` has a system Visual end and a lifted Visual end. The lifted Visual is the `PlacementVisual`
and must be created by the link itself (i.e. it cannot be specified by the caller). Xaml also uses
`ContentExternalOutputLink` for `WebView2`, and uses the same underlying code for `MediaPlayerElement` as well. This
creates a potential z-order issue among the system Visuals for the system backdrop, `WebView2` content, and media swap
chains. Luckily the lifted Compositor parents the system Visuals of `ContentExternalOutputLink`s in the tree based on
the relative order of their `PlacementVisuals` in the lifted tree. For Xaml this produces the effect that we want. The
`PlacementVisual` of the system backdrop will be at the bottom of Xaml's lifted Visual tree, below the
`PlacementVisual`s of `WebView2` or `MediaPlayerElement` elements, which then means the system Visual for the system
backdrop shows up underneath any system Visuals hosting `WebView2` content or media swap chains.

`ContentExternalOutputLink` also copies the world transform and opacity from its lifted `PlacementVisual` to its system
Visual every frame. Xaml will put the system backdrop's `PlacementVisual` under its animated root, so transform and
opacity will work automatically based on its location in the tree. Xaml will need to set the rounded corner clip on the
`PlacementVisual` explicitly, based on the rounded corner of the popup contents. The Visual's size doesn't matter for
Xaml's usage. Visual size affects inset clips but here the clip will be an explicit rect. It also affects things like
the brush stretch but Xaml doesn't set any of those properties on the `PlacementVisual` either.

### Proposed IXP API

The main API needed on the IXP side is a way for the app to provide properties on the system backdrop's system Visual.
Specifically, what its offset is relative to the hwnd, what its clip should be, and how it should be animated. This is
most directly done by exposing the `ContentExternalOutputLink.Placement` directly from the bridge as a
`PopupWindowSiteBridge.SystemBackdropPlacementVisual` property. Xaml can then get the visual, set a rounded clip on it,
and position it in the lifted Visual tree so that it picks up the proper transforms and opacities.

No new API is needed on the Xaml side. We do need to infer a few things from the popup's UIElement tree:
* The animation needs to be fished out so that it can be applied on the root of the lifted tree, above the shadow and
  the system backdrop proxy Visual.
* The rounded corner needs to be fished out so that it can be explicitly set on the system backdrop proxy Visual.

This seems like implicit behavior that's easy to get wrong, but it's enough to get FE working.

## Deliverables & Tasks

Composition deliverable: Support Mica + Rounded Corners + Shadows + Animations for
file explorer with islands
* Windowed popups support Mica + Rounded Corners + Shadows
* Lifted Compositor changes needed for windowed popup + system backdrop +
  rounded corners + shadows + animations

Xaml deliverable: Desktop Acrylic works for windowed popups part 1, FE's
ContextMenus
* Consume updated PopupWindowSiteBridge for rounded corners, shadows, and
  entrance animations

## Future

The change as described in this doc is to get File Explorer unblocked. In the long term we'll want to move the system
backdrop system Visual into Xaml, which requires lifted Xaml to create a system Compositor again. The pieces involved
include:
* Xaml creating and managing a system Compositor
* Xaml creating a system Visual for the system backdrop, and setting the WUC brush from
  ICompositionSupportsSystemBackdrop on that Visual
* Xaml creating a ContentExternalOutputLink to animate the system backdrop system Visual
* MicaController and DesktopAcrylicController add API to be given a system Compositor, and Xaml passing one in

This is the first step in getting system IXP reintegrated into lifted Xaml, and this path leads to solving layering
issues with swap chain content. The full details are out-of-scope for this document and are best left for a separate
document, but include:
* Xaml creating system Visuals for swap chain content
* SystemVisualSiteBridge, that allows hosting lifted content in arbitrary system Visuals and allows Xaml to create a
  mixed system/lifted Visual tree
* Solving the problem of shadows eating clicks
* Solving the problem of animations & animation tearing in a mixed system/lifted Visual tree
* Solving the problem of brushes and surfaces used in a mixed system/lifted Visual tree
* Solving multi-island hit testing

## Alternative - Hwnd shadows

The system Compositor is capable of drawing rounded corners and shadows around hwnds. The main benefit here is that
these hwnd shadows are transparent for hit testing and will not eat clicks meant for content underneath. There are
drawbacks that prevent us from going with this approach though:

1. No support for animations. This is the deal breaker. There is an
   [`AnimateWindow`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-animatewindow) API that has
   a few animation options, but this API blocks the UI
   thread while it updates per-frame values, and this is not acceptable from a app responsiveness perspective.
   There is a potential replacement that animates asynchronously but only offers baked-in animations.

2. No support for rounded corners or shadows downlevel. This is not a problem for File Explorer, which doesn't ship
   down-level, but is a problem for WinUI 3 in general, which does.

3. No support for custom values. The radius for rounded corners and depths for shadows are baked into the window styles,
   and anything different is not supported. This is a problem for Xaml in general, which supports app-provided values
   for rounded corners and shadow depths.

## Alternative - system Visuals

The system Visual tree animates everything using system Composition animations. Only the popup's content is rendered
into the lifted Composition surface. The shadow is drawn exclusively using system Visuals. The system backdrop Visual
has no lifted counterpart either.

The drawback here is that it needs a lot of changes in the content bridge code. The two biggest pieces are:
* It will need a way to translate lifted Composition animations (built by lifted Xaml) to system Composition animations
  (applied to the system Visual tree)
* It will need code to render a shadow using only system Visuals. Lifted Xaml currently builds the shadow using lifted
  Visuals, and this code can be adapted. Alternatively, system Xaml renders the shadow using system Visuals, and that
  can be copied over.

Because of these drawbacks, we are not going with this option.

The system Visual tree leaves the door open to address proper hit testing. Since the shadow is separated into its own
system Visual, we can make changes later to make this Visual skip the hit testing walk. This will need either converting
to SystemVisualBridge to avoid the hwnd, or it will need additional hit testing work in the system Compositor.

![A windowed popup with rounded corners, shadows, system backdrops, and animations, and its associated system Visual
tree](images/windowed-popup-visual-tree-system.png)

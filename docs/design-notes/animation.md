# XAML Animations

## Table of Contents

- [Overview](#overview)
  - [Storyboards](#storyboards)
    - [Transitions](#transitions)
  - [ElementCompositionPreview.GetElementVisual](#elementcompositionpreviewgetelementvisual)
  - [UIElement.StartAnimation](#uielementstartanimation)
  - [ElementCompositionPreview.SetImplicitShow/HideAnimation](#elementcompositionpreviewsetimplicitshowhideanimation)
  - [Motion system](#motion-system)
  - [Connected animations](#connected-animations)
  - [Implicit Composition Animations](#implicit-composition-animations)

## Overview

This document gives a high-level overview of the various animation APIs in Xaml.


### Storyboards

These are the oldest form of animation APIs in Xaml. The API is carried over from WPF and is the most integrated with
the rest of Xaml. In particular `Storyboards` can be used from markup and from `VisualStateManager`.

Wishlist:
* converting Storyboards to Composition animations
* setting Composition animations on the targeted object
* kicking off the Composition animation via `CTimeManager::StartWUCAnimation`
* animation control (pause/seek/resume)


#### Transitions

[Transitions](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.media.animation.transition?view=windows-app-sdk-1.2)
are a feature built on top of `Storyboard`s that let apps use predefined animations in well-known scenarios, such as items
entering or leaving a collection or a menu opening and closing.

There is a `CLayoutTransitionElement` associated with each transition that does redirected rendering of the element with
the transition. LTEs allow for elements to escape clips and allows for an element to be rendered multiple times, both of
which may be required for a transition. The transition itself creates this LTE in `CTransition::SetupTransition`.

Internally transition will create `Storyboard`s in their various `CreateStoryboardImpl` overrides. When created, this
`Storyboard` targets the element with the transition set on it, but the transition will retarget it to be the
`CLayoutTransitionElement` associated with the transition in `RewriteTransitionTargets`.


### ElementCompositionPreview.GetElementVisual

The
[`GetElementVisual`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.hosting.elementcompositionpreview.getelementvisual?view=windows-app-sdk-1.2)
API allows the app to directly access a Composition `Visual` behind a Xaml `UIElement`. The app is then free to use
Composition APIs to set properties or animate that `Visual` directly. Xaml subscribes to property update notifications
from Composition so we can keep track of where the element is on screen for hit testing.

There is plenty of code in the controls layer (e.g., `controls/dev/ScrollPresenter/ScrollPresenter.cpp`)
that calls this API.


### UIElement.StartAnimation

The
[`UIElement.StartAnimation`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.uielement.startanimation?view=windows-app-sdk-1.2)
API allows Composition animations to animate Xaml objects. It works by having `UIElement` implement Composition's
`IAnimationObject` interface (see `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.cs`).
The interface allows `UIElement` to map a property name to a real Composition object wrapped inside the
`UIElement` to be animated by the Composition animation. The interface
does this via a AnimationPropertyInfo class that has the
`GetResolvedCompositionObject`/`GetResolvedCompositionObjectProperty` methods to complete the mapping.

There is plenty of code in the controls layer (e.g., `controls/dev/ScrollPresenter/ScrollPresenter.cpp`)
that calls this API.

Wishlist:
* mechanism for resolving property mappings
* mechanism for tracking backing Composition objects
* Test mock lookup (`MockCompositionObjectFactory::StartAnimationWithIAnimationObject`)


### ElementCompositionPreview.SetImplicitShow/HideAnimation

The
[`SetImplicitShowAnimation`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.hosting.elementcompositionpreview.setimplicitshowanimation?view=windows-app-sdk-1.2)
API lets the app provide a Composition animation that will play automatically when a UIElement is shown on screen. This
is kicked off from Xaml's render walk, so it can take into account the full ancestor chain and properties set on that
chain. Similarly, there's a `SetImplicitHideAnimation` API for when a UIElement is hidden. Note that for hide animations
we need to keep the element visible and in the tree as long as the animation is in progress. This is done via
`SetTrackKeepVisible` (see `dxaml/xcp/components/comptree/DCompTreeHost.cpp`).

Wishlist:
* SetTrackKeepVisible details
* integration points with the render walk


### Connected animations

Connected animations are a way to animate an element across a page transition. It's set up by the
[`ConnectedAnimationService`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.media.animation.connectedanimationservice?view=windows-app-sdk-1.3).


### Implicit Composition Animations

IXP's `CompositionObject` also has an
[`ImplicitAnimations`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.composition.compositionobject.implicitanimations?view=windows-app-sdk-1.3#microsoft-ui-composition-compositionobject-implicitanimations)
collection. Apps can put animations in this collection to be automatically kicked off when properties change.

Xaml has some interaction with the ImplicitAnimations collection - we turn off implicit
animations (see `dxaml/xcp/components/comptree/HWCompNodeWinRT.cpp`)
during rendering when we first update the primary visual.

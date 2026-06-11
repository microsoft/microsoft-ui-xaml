# Lifted XAML and OneCoreTransforms
**April 2020**

## Table of Contents

- [Executive Summary (too long, didn't read)](#executive-summary-too-long-didnt-read)
- [Background](#background)
  - [What is OneCoreTransforms?](#what-is-onecoretransforms)
  - [Terms](#terms)
  - [Visual-relative coordinates: the north star for XAML scaling](#visual-relative-coordinates-the-north-star-for-xaml-scaling)
  - [Scaling inside the XAML runtime](#scaling-inside-the-xaml-runtime)
  - [Text Scaling](#text-scaling)
  - [Lightweight Compositing, OCT, and UIA](#lightweight-compositing-oct-and-uia)
  - [DirectManipulation](#directmanipulation)
  - [Test Code](#test-code)
- [Proposed Roadmap: Keep OCT paths, iteratively converge them](#proposed-roadmap-keep-oct-paths-iteratively-converge-them)
- [Other Lightweight Compositing Considerations](#other-lightweight-compositing-considerations)
  - [Linking](#linking)
- [Appendix](#appendix)
  - [XAML's uses of OCT mode (April 2020)](#xamls-uses-of-oct-mode-april-2020)
    - [UIA](#uia)
    - [Direct Manipulation](#direct-manipulation)
    - [TouchHitTesting](#touchhittesting)
    - [Coordinate-space differences (Scaling/Offsets)](#coordinate-space-differences-scalingoffsets)
    - [Apply scale differently when in island](#apply-scale-differently-when-in-island)
    - [Hook up to CoreWindow's comp island](#hook-up-to-corewindows-comp-island)
    - [Use WinRT API instead of win32 API](#use-winrt-api-instead-of-win32-api)
    - [Software Keyboard](#software-keyboard)
    - [Other](#other)

## Executive Summary (too long, didn't read)

XAML has some diverged code to support lightweight compositing-based Windows editions.
We call this "OneCoreTransforms mode". This mode
is currently always off, but when we support similar versions
of Windows this code may help guide us. We cannot yet
test WinUI 3+ on lightweight compositing mode, but we may want to support it in the future.  

## Background

### What is OneCoreTransforms?

In the RS4 timeframe, we started work toward a new standard coordinate
space for windows. We created a mode called
"OneCoreTransforms" that, when enabled, would tell enlightened
components (like XAML) to operate in visual-relative coordinates with
each other. Instead of using screen space and calling Win32 functions
like ::GetClientRect and ::ClientToScreen, coordinates would instead be
defined by a composition visual and X,Y offset from the origin of that
visual. At the time, the goal was to eventually use the visual-relative
coordinate space for all versions of Windows, but due to shifting
priorities it is now only on for certain lightweight compositing SKUs.

In System XAML, we also connect the visual tree differently in
OneCoreTransforms mode. When system XAML is running on Desktop edition,
we retrieve a composition target based on the CoreWindow's HWND; in
lightweight compositing mode we get a Composition Island from the CoreWindow, and parent our
visual tree to that Composition Island.

We now have many "if (OneCoreTransforms)" switches across various
parts of the codebase, for various reasons. (see appendix for a list,
there are around 50). As we churn lifted XAML without testing lightweight compositing, it's
likely we'll not update these codepaths correctly and accumulate debt
for when we do turn on lightweight compositing. The aim of this document is to discuss those
differences and choose a strategy for how and if we deal with this debt.

### Terms

In this document, the terms OneCoreTransforms (OCT), lightweight compositing, and
visual-relative coordinates can almost always be substituted for one
another. This is because OCT mode is enabled only in lightweight compositing-based editions
today.

### Visual-relative coordinates: the north star for XAML scaling

Our north-star in XAML is still to use visual-relative coordinates as
much as possible, and convert to another coordinate space only right
when it's needed. Within the XAML framework, the term "visual-relative
coordinates" is generally the coordinate space where 0,0 is the top
left of XAML's display area. The scale is what we call "logical"
pixels, which is usually the current display scale.

### Scaling inside the XAML runtime

Today there are multiple ways XAML sets up the visual tree, and this
affects various internal XAML calls differently. The below table shows
the various ways the scale will be applied, and how various calls within
XAML will receive a scale. The differences make it hard to write correct
code today.

*Table: Scales are Inconsistent*

|**Scenario**|**How XAML applies the scale**|**PresentTarget size scale**|**GetGlobalBounds and TransformToWorldSpace**|**CUIElement::TransformToRoot**|
|------------|------------------------------|----------------------------|---------------------------------------------|-------------------------------|
| System XAML Desktop CoreWindow      | ScaleRenderTransformset on XamlRootElement                | Physical | Physical  | Physical |
| System XAML Lightweight Compositing CoreWindow         | Scale set on comp visual above XAML, XAML inherits scale  | Logical  | Logical   | Logical  |
| System XAML DesktopWindowXamlSource | Scale set on comp visual above XAML, XAML inherits scale  | n/a      | Physical  | Logical  |
| Lifted XAML Desktop CoreWindow | Scale set on comp visual above XAML, XAML inherits scale. XAML RasterizationScale set on XAML root element | Physical | Physical | Logical |
| Lifted XAML Lightweight Compositing CoreWindow         | Not designed                                              | TBD      | TBD       | TBD      |
| Lifted XAML DesktopWindowXamlSource | Scale set on comp visual above XAML, XAML inherits scale  | n/a      | Physical  | Logical  |

Inside XAML code, you can find OCT switches by looking for these
functions:

* **XamlOneCoreTransforms::IsEnabled** -- True for lightweight compositing mode
* **ShouldUseVisualRelativePixels** -- XAML's CContentRoot representation of this
* **ShouldUseVisualPixels** -- XAML's CTextBoxBase representation of this
* **TxGetShouldUseVisualPixels** -- yet another representation in XAML

### Text Scaling

TODO: More investigation needed for coordinate space handling in text code — specifically which coordinates are passed to TSF and what coordinate space it expects. Text code could be simplified by resolving differences in coordinate transform APIs (GetGlobalBounds, TransformToWorldSpace, TransformToRoot) noted in the "Scaling Today" section.

### Lightweight Compositing, OCT, and UIA

In lightweight compositing mode, XAML uses a special private UIA interface to communicate in visual-relative
coordinates.

### DirectManipulation

According to code comments, DirectManipulation operates in screen
coordinates on Desktop edition, and visual-relative on OCT/lightweight compositing-based
editions.

### Test Code

In XAML's test infrastructure, we call some private APIs (e.g.
MapVisualRelativePoints) to convert to a screen location for input
injection. Some specific tests expect different coordinates when in OCT
mode (specifically, accessibility), and other tests simply exit out when
OCT is either off or on.


## Proposed Roadmap: Keep OCT paths, iteratively converge them

For WinUI 3.0, the proposal is that **we leave in XAML's
"OneCoreTransforms" codepaths** and **opportunistically converge
toward them** when possible. That is -- where we do converge, we want to
use the OCT path rather than the non-OCT path if possible. Yes, this
will leave us with some dead code for the release. This is because we
intend to ship 3.0 before we're able to test platforms with OCT mode,
but we want to quickly turn around and support those platforms as soon
as possible. Since right now we can only test platforms where
OneCoreTransforms is OFF, we can't trust that any refactoring we would
do would be sufficient to support platforms where OCT is ON.

Here's what we plan to do in the future:

* When communicating with lifted components, use "client-logical"
  coordinates wherever possible to set us up for using visual-relative
  coordinates more and more in the future. Requires internal
  coordination. This may not be possible when running in a win32 context
  * DirectManipulation
  * TouchHitTesting

* Opportunistically prefer OCT code paths where appropriate. In some
  cases, the OCT path is an equivalent WinRT function, and we kept the
  old code for reasons that don't matter so much anymore
  (XamlPresenter support, subtle compat concerns, etc)
  * Example: In OCT we call CoreWindow.Bounds to get the
    CoreWindow's bounds, but we call ::GetClientRect on desktop. We
    can converge this by just always using CoreWindow.Bounds
* Opportunistically reduce XAML's use of non-converged scaling
  functions, from the "Scales are inconsistent" table above.
  Internally within the XAML runtime, use logical pixels everywhere we
  can
  * Stop using GetGlobalBounds/TransformToWorldSpace, use something
    like GetGlobalBoundsLogical instead
  * Stop using CUIElement::TransformToRoot
  * Investigate changing the WindowPresentTarget size to logical
    instead of physical pixels
  * Review recent DPI bugs to find other problematic functions
* UIA: Remove direct use of the private UIA interfaces, create
  equivalents with different names
  * Verify internally whether keeping copies of the private
    interfaces in our code (possibly with different GUIDs), unused
    for now, is acceptable
  * Rather than delete the code that supports the UIA interfaces, we
    just create our own contract with a slightly different name that
    happens to match the current UIA interface 1:1
  * Update automation tests that expect different coordinate results in OCT mode.
* Test code
  * As we explore using public APIs for input injection, consider
    cleaner ways to stay in a visual-relative coordinate space

## Other Lightweight Compositing Considerations

### Linking

Today XAML links against libs that don't work in lightweight compositing mode.  (e.g., user32.lib)

## Appendix

### XAML's uses of OCT mode (April 2020)

This section breaks down XAML's use of OneCoreTransforms mode, with
some rough categories.

#### UIA

*Not yet exposed publicly; verify internally before relying on this.*

Use visual-relative UIA interfaces

#### Direct Manipulation

*Lifted DirectManipulation is expected to run in strict mode by
default; verify internally before deleting the code.*

Tell DirectManipulation to use strict mode
(IDirectManipulationManagerPartner::EnableOneCoreTransforms)

Tell DirectManipulation to use strict mode
(CInputServices::CreateViewportInteractionForRootVisual)

#### TouchHitTesting

TouchHitTesting uses different coordinate space (now that we use lifted,
what should we do?)

#### Coordinate-space differences (Scaling/Offsets)

*There are some things we can tease apart here. Which of these are
because of other components operating in a different coordinate space,
and which are because of XAML arranging the tree differently? For the
cases where XAML is arranging the tree differently, shouldn't this be
the same between OCT and islands modes?*

Handle scaling differently (CJupiterControl::UpdateHdr)

Convert to correct scale
(ListViewBaseHeaderItemAutomationPeer_partial.cpp)

Handle scaling differently (Text)
(CTextRangeAdapter::GetBoundingRectangles)

Handle scaling differently (Text)
(CRichEditGripperChild::UpdateCenterWorldCoordinate)

Handle scaling differently (Text) (CTextServicesHost::ShowGripper)

Handle scaling differently (Text) (CTextBoxView::TxGetViewportRect)

Handle scaling differently (Text) (CTextBox::GetRectFromCharacterIndex)

Handle scaling differently (Text)
(HyperlinkAutomationPeer::GetBoundingRectangleCore)

Handle scaling differently (UIA)
(FrameworkElementAutomationPeer::GetBoundingRectangleCore)

Handle scaling differently (UIA)
(CUIElement::GetClickablePointRasterizedClient)

Handle scaling differently, configure root scale
(VisualTree::VisualTree)

Handle scaling differently
(CUIElement::GetRedirectionTransformsAndParentCompNode)

Don't get desktop offset (AutoSuggestBox::GetAdjustedLayoutBounds)

Don't apply offset for windowed popups
(DXamlCore::GetTranslationFromTargetWindowToRootWindow)

Handle scaling differently (Dmanip)
(CInputServices::UpdateManipulationViewport)

Handle scaling differently (soft keyboard)
(CInputPaneHandler::BringFocusedElementIntoView)

#### Apply scale differently when in island

Register for scale change notifications (DXamlCore)

Fix OCT scale/bounds sync problem (DXamlCore::UpdateScaleFactor)

Set correct scale on root element (HWWalk::RenderProperties)

#### Hook up to CoreWindow's comp island

Get/Connect to CoreWindow's island (CJupiterWindow::SetCoreWindow)

Get/Connect to CoreWindow's island (DCompTreeHost)

#### Use WinRT API instead of win32 API

*It looks like we could move to the WinRT option for most of these,
couldn't we? It's possible we were afraid of compat breaks and the
WinRT one is fine (if it exists down to RS4)*

Use CoreWindow bounds for size rather than GetClientRect
(CJupiterWindow::GetJupiterWindowPhysicalSize)

Subscribe to CoreWindow.Activated (on desktop we use WM_ACTIVATE)
(CJupiterWindow::RegisterCoreWindowEvents)

Use DisplayInformation for HDR (CJupiterControl::UpdateHdr)

Use WinRT PointerPoint rather than Win32 POINTER_INFO
(CInputServices::ProcessPointerExitedEventByPointerEnteredElementStateChange)

Decide to use WinRT DManip hit testing
(CJupiterWindow::UseDirectManipulationHitTestEvent)

#### Software Keyboard

Use FrameworkInputView instead of InputPane for soft keyboard
(CInputPaneInteractionHelper::RegisterInputPaneHandler)

Init CInputPaneHandler with useVisualRelativePixels=true
(InputPaneProcessor)

Disable detecting occlusion with SIP
(ContentDialog::AdjustVisualStateForInputPan)

#### Other

Test hook (ShrinkApplicationViewVisibleBounds)

Use it to check for win32 (xaml islands) hosting
(SemanticZoom_partial.cpp)

Ensure OCT not on when on desktop (Page_partial.cpp)

Do NOT Configure CoreWindow bounds behavior (DXamlCore)

Check to see if full-screen (CMediaElement::InvokeImpl)

Different way to SetCapture/ReleaseCapture (PointerInputProcessor)

Force disable in win32 mode (WindowXamlManager_partial.cpp)

Drag/drop (XamlMobileDragOperation.cpp)

CTextBoxBase::ShouldUseVisualPixels called by text code to handle
scaling differently

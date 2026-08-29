// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "Focusable.h"
#include "RevealFocusSource.h"
#include "RevealFocusAnimator.h"

struct FocusRectangleOptions;
class IContentRenderer;
class CTransitionRoot;
class CLayoutTransitionElement;
class CBorder;
class CCanvas;

namespace DirectUI {
    enum class FocusNavigationDirection : uint8_t;
}

class CFocusRectManager
{
public:
    ~CFocusRectManager();

    // Render the "legacy" dotted-line focus rectangle for the given element
    static void RenderFocusRectForElement(
        _In_ CUIElement* element,
        _In_ IContentRenderer* renderer);

    // Returns true if we're showing the "high-visibility" solid-line focus rectangles,
    // false if we're showing the "legacy" dotted-line focus rectangles. Returns true
    // if RevealFocus mode is enabled as well, as that just adds an extra effect around the existing
    // high-visibility focus rect
    static bool AreHighVisibilityFocusRectsEnabled();

    // Returns true if we're showing the "reveal" focus rectangles,
    // false if we're showing the "legacy" dotted-line focus rectangles or regular
    // "high-visibility" focus rectangles
    static bool AreRevealFocusRectsEnabled();

    // Ensure the focus rectangle is positioned properly in the visual tree
    void UpdateFocusRect(
        _In_ CCoreServices* core,
        _In_opt_ CDependencyObject* focusedObject,
        _In_opt_ CDependencyObject* focusTargetObject,
        _In_ DirectUI::FocusNavigationDirection focusNavigationDirection,
        _In_ bool cleanOnly = false);

    void OnFocusedElementKeyPressed() const;
    void OnFocusedElementKeyReleased() const;

    void ReleaseResources(_In_ bool isDeviceLost, _In_ bool cleanupDComp, _In_ bool clearRenderData);

    static XTHICKNESS GetFocusVisualMargin(
        _In_ CDependencyObject* element,
        _In_opt_ CDependencyObject* target);

private:
    void ApplyRevealFocusToBorder(
        _In_ CBorder* revealBorder,
        _In_ const Focusable& focusTarget,
        _In_ FocusRectangleOptions& options,
        _In_ XRECTF outerBounds,
        _In_ DirectUI::FocusNavigationDirection focusNavigationDirection);

    static void CallCustomizationFunction(
        _In_ CDependencyObject* focusTarget,
        _Inout_ FocusRectangleOptions& options,
        _Inout_ bool* shouldDrawFocusRect);

    static void DetermineRenderOptions(
        _In_ const Focusable& focusedElement,
        _In_ const Focusable& focusTarget,
        _In_ const FocusRectHost& focusHost,
        _In_ const Focusable& previousFocusTarget,
        _In_ const bool useRevealFocus,
        _Inout_ FocusRectangleOptions& options,
        _Inout_ std::vector<XRECTF>& boundsVector);

    static bool SetLegacyRenderOptions(
        _In_ const Focusable& focusTarget,
        _Inout_ FocusRectangleOptions* options);

    static void GetFocusOptionsForElement(
        _In_ const Focusable& element,
        _In_ const Focusable& focusTarget,
        _In_ bool hasMultipleFocusRects,
        _In_ bool useRevealFocus,
        _Out_ FocusRectangleOptions* options,
        _Out_ XTHICKNESS* margin);

    static void GetDefaultFocusOptions(
        _In_ CCoreServices* core,
        _Inout_ FocusRectangleOptions* options);

    static void GetDefaultFocusOptionsForLink(
        _In_ CCoreServices* core,
        _Inout_ FocusRectangleOptions* options);

    static void AddRevealFocusOptions(
        FocusRectangleOptions& options,
        const Focusable& focusTarget);

    enum class FocusElementType { Outer, Inner };

    static bool ConfigureFocusElement(
        _In_ CBorder* border,
        _In_ FocusElementType type,
        _In_ FocusRectangleOptions& options,
        _In_ XRECTF outerBounds);

    static FocusRectHost FindFocusRectHost(
        _In_ const Focusable& target,
        _In_ const XRECTF& focusRect);

/*
    High-visibility focus rectangles are difficult to get exactly right because we want them to sometimes get clipped
    but not always, sometimes be obscured in zorder but not always, and definitely always be positioned exactly
    where the focused element is, even if it's being animated, in an LTE, getting transformed, etc.

    High-level notes:
    * The "focused element" is the element that logically has focus in the tree.
    * The "focus target" is the element on which we're drawing the focus rect.  It's often the same as the focused element,
        but is different if an element in the template sets Control.IsTemplateFocusTarget = true.
    * The "focus rect host" is an element we choose in the tree (heuristically) that's going to "sponsor" or "host" the focus
        rect by being responsible for drawing the focus rect on behalf of the focus target.  The main thing to note is that
        the focus rect will be clipped by the focus host's clip.
    * "Nudging" is the term we use for making adjustments to the focus rect, usually to keep it visible within a particular
        container element or rectangle.  See FocusRectNudging.h.
    * "High-visibility" focus rects were added in RS1.  They make things complicated because they can be wider than 1 pixel, before
        that focus rects always had a width of 1.  High-visibility focus rects are two rectangles, configurable by the FocusVisual*
        properties on FrameworkElement

    This implementation uses LTEs to place the focus rects in the tree in such a way that they will pick up the transforms and
    animations.  Here's what the tree looks like:

                                             (nodes)
                                                |
                                         focus rect host
                                         /              \
                                      (nodes)           m_transitionRoot
                                       /                            \
                                 focused element                    m_focusLte
                                      |                                 |
                                 focus target                (targets m_canvasForFocusRects)
                                      |
                               (secret child)
                             m_canvasForFocusRects
                                      |
                          (borders for focus rects)

    We place m_canvasForFocusRects in the tree as a "secret child" of the focus target.  m_canvasForFocusRects knows it's a child
    of the focus target, but it's not in focus target's child collection, so it won't be part of its layout, and it won't be
    visible by VisualTreeHelper and stuff.  But during the render walk it will pick up the transforms and animations between
    "focus rect host" and "focus target", which is what we want.  That's the magic of the LTE -- it allows us to inherit the
    clip and zorder from focus rect host, while still honoring the transforms all the way down to the focus target.
*/

    CLayoutTransitionElement* m_focusLte = nullptr;
    CTransitionRoot* m_transitionRoot = nullptr;
    CCanvas* m_canvasForFocusRects = nullptr;
    FocusRect::RevealFocusSource m_revealSource;
    std::unique_ptr<FocusRect::RevealFocusAnimator> m_revealAnimator;
};



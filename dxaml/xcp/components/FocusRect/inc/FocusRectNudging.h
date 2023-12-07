// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <GeneralTransformHelper.h>
#include <FocusProperties.h>
#include <FxCallbacks.h>

namespace FocusRect {

enum class NudgeMode
{
    Always,
    OnlyWhenElementIsOnContainerBoundary
};

CScrollViewer* GetAncestorScrollViewer(
    _In_ CDependencyObject* element)
{
    CDependencyObject* parent = element;
    while (parent)
    {
        if (parent->OfTypeByIndex<KnownTypeIndex::ScrollViewer>())
        {
            return static_cast<CScrollViewer*>(parent);
        }
        parent = parent->GetParent();
    }
    return nullptr;
}

// Directions a ScrollViewer can scroll
struct ScrollDirection
{
    bool Horizontal = false;
    bool Vertical = false;

    bool IsScrollable() const
    { return Vertical || Horizontal; }
};

// Code to get scrolling direction from ScrollViewer
ScrollDirection GetScrollDirection(
    _In_ CDependencyObject* element)
{
    CScrollViewer* scrollViewer = GetAncestorScrollViewer(element);
    if (!scrollViewer)
    {
        return ScrollDirection();
    }

    ScrollDirection direction;
    if (scrollViewer->m_extentHeight > scrollViewer->m_viewportHeight)
    {
        direction.Vertical = true;
    }
    if (scrollViewer->m_extentWidth > scrollViewer->m_viewportWidth)
    {
        direction.Horizontal = true;
    }
    return direction;
}

// Nudge the given rect by the given amounts
void NudgeRect(
    _Inout_ XRECTF& r,
    _In_ const XPOINTF& topLeftNudge,
    _In_ const XPOINTF& bottomRightNudge)
{
    r.X += topLeftNudge.x;
    r.Y += topLeftNudge.y;
    r.Width = MAX(0.0f, (r.Width - topLeftNudge.x) + bottomRightNudge.x);
    r.Height = MAX(0.0f, (r.Height - topLeftNudge.y) + bottomRightNudge.y);
}

// The spec calls for nudging to only happen when the element is right on the edge of the
// containing element, defined as within 0.5 pixels
bool AreNearEnoughToNudge(float a, float b, NudgeMode mode)
{
    if (mode == NudgeMode::Always)
    {
        return true;
    }
    else if (mode == NudgeMode::OnlyWhenElementIsOnContainerBoundary)
    {
        return abs(a - b) < 0.5f;
    }
    ASSERT(false); // unrecognized nudge mode
    return false;
}

void NudgeInsideContainer(
    _Inout_ XRECTF& focusRectBounds,
    _In_ const XRECTF& elementBounds,
    _In_ const XRECTF& containerBounds,
    _In_ NudgeMode nudgeMode,
    _In_ bool nudgeHorizontalSides = true,
    _In_ bool nudgeVerticalSides = true)
{
    XPOINTF topLeftNudge = {};
    XPOINTF bottomRightNudge = {};

    if (nudgeHorizontalSides)
    {
        if (AreNearEnoughToNudge(containerBounds.X, elementBounds.X, nudgeMode) && focusRectBounds.X < containerBounds.X)
        {
            topLeftNudge.x = containerBounds.X - focusRectBounds.X;
        }
        if (AreNearEnoughToNudge(containerBounds.Right(), elementBounds.Right(), nudgeMode) && containerBounds.Right() < focusRectBounds.Right())
        {
            bottomRightNudge.x = containerBounds.Right() - focusRectBounds.Right();
        }
    }

    if (nudgeVerticalSides)
    {
        if (AreNearEnoughToNudge(containerBounds.Y, elementBounds.Y, nudgeMode) && focusRectBounds.Y < containerBounds.Y)
        {
            topLeftNudge.y = containerBounds.Y - focusRectBounds.Y;
        }
        if (AreNearEnoughToNudge(containerBounds.Bottom(), elementBounds.Bottom(), nudgeMode) && containerBounds.Bottom() < focusRectBounds.Bottom())
        {
            bottomRightNudge.y = containerBounds.Bottom() - focusRectBounds.Bottom();
        }
    }

    if (   bottomRightNudge.x == 0.0f
        && bottomRightNudge.y == 0.0f
        && topLeftNudge.x == 0.0f
        && topLeftNudge.y == 0.0f)
    {
        return;
    }

    NudgeRect(focusRectBounds, topLeftNudge, bottomRightNudge);
}

// Get an elements bounds, with its margins tacked on, in element space.
XRECTF GetElementLayoutBoundsWithMargins(_In_ CUIElement* e)
{
    XRECTF bounds = { 0.0f, 0.0f, e->GetActualWidth(), e->GetActualHeight() };
    CFrameworkElement* fe = do_pointer_cast<CFrameworkElement>(e);

    if (fe)
    {
        if (fe->m_pLayoutProperties)
        {
            bounds = EnlargeRectByThickness(bounds, fe->m_pLayoutProperties->m_margin);
        }
    }

    return bounds;
}

bool NudgeFocusRectInsideHorizontalClip(_In_ CUIElement* const element)
{
    bool nudge = true;

    if (element->IsScroller())
    {
        bool isHorizontallyScrollable = false;
        bool isVerticallyScrollableIgnored = false;
        FocusProperties::IsScrollable(element, &isHorizontallyScrollable, &isVerticallyScrollableIgnored);
        nudge = !isHorizontallyScrollable;
    }

    // Sticky headers in ItemsStackPanel and ItemsWrapGrid are not used
    // when the panel is in the horizontal orientation.

    return nudge;
}

bool NudgeFocusRectInsideVerticalClip(_In_ CUIElement* const element)
{
    bool nudge = true;

    if (element->IsScroller())
    {
        bool isHorizontallyScrollableIgnored = false;
        bool isVerticallyScrollable = false;
        FocusProperties::IsScrollable(element, &isHorizontallyScrollableIgnored, &isVerticallyScrollable);
        nudge = !isVerticallyScrollable;
    }
    else if (
        (element->OfTypeByIndex<KnownTypeIndex::ItemsStackPanel>() || element->OfTypeByIndex<KnownTypeIndex::ItemsWrapGrid>()) &&
        element->GetClip())
    {
        // If the element is a modern panel (ISP or IWG) and there is a sticky header clip, we should not nudge
        // against it in the vertical direction and allow it to clip the focus rectangle.
        nudge = false;
    }

    return nudge;
}

XRECTF GetEffectiveNudgingClip(
    _In_ CUIElement* const element,
    _In_ XRECTF clip)
{
    const XRECTF infiniteClip = GetInfiniteClip();

    if (!NudgeFocusRectInsideHorizontalClip(element))
    {
        clip.X = infiniteClip.X;
        clip.Width = infiniteClip.Width;
    }

    if (!NudgeFocusRectInsideVerticalClip(element))
    {
        clip.Y = infiniteClip.Y;
        clip.Height = infiniteClip.Height;
    }

    return clip;
}

// Collect rectangular clips up to the root.  Uses bounding boxes if a clip is transformed.
_Check_return_ HRESULT GetAncestorClipsInClientSpace(
    _In_ CDependencyObject* node,
    _In_ const FocusRectHost& focusRectHost,
    _In_ bool isNodeAncestorOfFocusRectHost,
    _Out_ XRECTF& transformedClip)
{
    CUIElement* element = do_pointer_cast<CUIElement>(node);
    const bool isLteTarget =
        element
        && element->IsHiddenForLayoutTransition()
        && element->GetLayoutTransitionElements()->size() >= 1;

    // As we walk up the tree, when we hit an LTE target we need to follow the LTE ancestors
    // up the tree instead of the LTE target's ancestors, because those are the clips that
    // will apply
    CDependencyObject* parent = isLteTarget
        ? (*element->GetLayoutTransitionElements())[0]
        : node->GetParentInternal(false /*publicOnly*/);

    XRECTF clipsOfAncestors;

    if (node == focusRectHost.Element)
    {
        isNodeAncestorOfFocusRectHost = true;
    }

    if (focusRectHost.type == FocusRectHost::Type::ScrollContentPresenterPeer && node == focusRectHost.Element->GetParent())
    {
        isNodeAncestorOfFocusRectHost = true;
    }

    // Make sure not to clip against the Island root collection.
    if (parent && !parent->OfTypeByIndex<KnownTypeIndex::XamlIslandRootCollection>())
    {
        IFC_RETURN(GetAncestorClipsInClientSpace(parent, focusRectHost, isNodeAncestorOfFocusRectHost, clipsOfAncestors));
    }
    else
    {
        clipsOfAncestors = GetInfiniteClip();
    }

    bool shouldAddClip = element != nullptr;

    // We need to be careful about how we deal with clips here.  If we accumulate the clip we find here,
    // that means we'll be nudging the focus rect to stay inside of that clip.  Sometimes we want to
    // nudge inside the clip, but sometimes we want the clip to clip the focus rect just like it's
    // clipping the focus target.
    //
    //  (A) Focus rect is clipped just like the button is clipped:
    //  -------------------- clip
    //   ||        ||
    //   || button ||
    //   ||--------||
    //   |----------|
    //
    //  (B) Focus rect is nudged to stay inside of the clip:
    //  -------------------- clip
    //   |----------|
    //   || button ||
    //   ||--------||
    //   |----------|
    //
    // If the focus target is part of scrolling content inside a ScrollViewer, we generally don't want to
    // nudge against that clip (we want (A) rather than (B) above) for two reasons:
    //  1. It's not visually what users expect.  As a focused element scrolls out of view, we don't expect the focus
    //     rect to change its size as it leaves.
    //  2. We update the focus rect on the UI thread, but scrolling happens off-thread.  This means the focus rect's
    //     shape isn't in perfect sync with the focus target, which is visually jarring.

    if (shouldAddClip
        && element->OfTypeByIndex<KnownTypeIndex::ScrollContentPresenter>())
    {
        if (!isNodeAncestorOfFocusRectHost)
        {
            // This ScrollContentPresenter is between the focus rect host and the focus target.  This means we skipped it
            // on the way up because it wasn't scrollable.  Since we're drawing the focus rect at a place
            // in the tree that is not clipped by this clip, there's no need to nudge inside of it.
            shouldAddClip = false;
        }
        else if (focusRectHost.type == FocusRectHost::Type::ElementContainsFocusRect && GetScrollDirection(element).IsScrollable())
        {
            // Here, "element" is a ScrollContentPresenter that's a an ancestor of the focus rect host.
            // So, the ScrollContentPresenter's clip will clip the focus rect just like it will clip the focus target.  If the
            // ScrollViewer is scrollable, we want the (A) behavior above.  Sometimes apps have non-scrolling ScrollViewers in the
            // tree-- these really just act like parents that clip their children.  For these, we want (B) behavior, so we
            // only set shouldAddClip to false if the IsScrollable call above returns true.

            // MSFT:10874921 Focus rects sometimes nudge to stay within a ScrollViewer viewport
            // We shouldn't strictly need the restriction that this is "ElementContainsFocusRect", we've added it to reduce
            // risk in RS2 endgame.  We'd actually have better behavior for ScrollContentPresenterChild hosts if we went
            // down this path; today, we get (B) behavior above for ScrollContentPresenterChild hosts, when we really want (A).
            shouldAddClip = false;
        }
    }

    if (shouldAddClip &&
        (NudgeFocusRectInsideHorizontalClip(element) || NudgeFocusRectInsideVerticalClip(element)))
    {
        CRectangleGeometry* rectangle = do_pointer_cast<CRectangleGeometry>(element->GetClip());
        if (rectangle)
        {
            xref_ptr<CGeneralTransform> xform;
            element->TransformToVisual(nullptr, &xform);

            XRECTF clipRect = rectangle->m_rc;
            XRECTF transformedClipRect = {};
            if (rectangle->m_pTransform)
            {
                IFC_RETURN(rectangle->m_pTransform->TransformRect(clipRect, &transformedClipRect))
                clipRect = transformedClipRect;
            }

            transformedClipRect = {};
            IFC_RETURN(xform->TransformRect(clipRect, &transformedClipRect));

            const XRECTF clipRectInClientSpace = GetEffectiveNudgingClip(element, transformedClipRect);
            IntersectRect(&clipsOfAncestors, &clipRectInClientSpace);
        }

        // A layout clip may exist due to a parent element having smaller bounds than its descendants.  E.g.,
        // a StackPanel that has a Height of 100 and elements that extend down past 100.
        if (element->HasLayoutClip())
        {
            // The layout clip could be in element space or its parent's space.  ShouldApplyLayoutClipAsAncestorClip
            // tells us which.
            CUIElement* elementDefiningClipSpace =
                element->ShouldApplyLayoutClipAsAncestorClip() ? do_pointer_cast<CUIElement>(element->GetParent()) : element;
            CRectangleGeometry* layoutClipRect = element->LayoutClipGeometry;

            if (elementDefiningClipSpace)
            {
                xref_ptr<CGeneralTransform> xform;
                elementDefiningClipSpace->TransformToVisual(nullptr, &xform);

                XRECTF clipRect = layoutClipRect->m_rc;
                XRECTF transformedClipRect = {};
                if (layoutClipRect->m_pTransform)
                {
                    IFC_RETURN(layoutClipRect->m_pTransform->TransformRect(clipRect, &transformedClipRect));
                    clipRect = transformedClipRect;
                }

                transformedClipRect = {};
                IFC_RETURN(xform->TransformRect(clipRect, &transformedClipRect));

                const XRECTF clipRectInClientSpace = GetEffectiveNudgingClip(element, transformedClipRect);
                IntersectRect(&clipsOfAncestors, &clipRectInClientSpace);
            }
        }

    }

    transformedClip = clipsOfAncestors;
    return S_OK;
}

// Does all the focus rect nudging, and then returns the focus rect bounds in
// element space. When using reveal focus, if there isn't a border, we never want
// to nudge because the glow will then be pushed over the content and the content will
// be visible underneath
_Check_return_ HRESULT GetFocusRectWithNudging(
    _In_ CUIElement* element,
    _In_ XRECTF elementBounds,
    _In_ const FocusRectHost& focusRectHost,
    _In_ const XTHICKNESS& focusRectMargin,
    _In_ const bool isBorderlessRevealFocus,
    _Out_ XRECTF& transformedClientToElementRect)
{
    xref_ptr<CGeneralTransform> elementToClientTransform;

    const XRECTF focusRectBounds = ShrinkRectByThickness(elementBounds, focusRectMargin);

    element->TransformToVisual(nullptr, &elementToClientTransform);

    XRECTF elementBoundsInClientSpace = {};
    IFC_RETURN(elementToClientTransform->TransformRect(elementBounds, &elementBoundsInClientSpace));

    XRECTF focusRectBoundsInClientSpace = {};
    IFC_RETURN(elementToClientTransform->TransformRect(focusRectBounds, &focusRectBoundsInClientSpace));

    {
        wf::Rect visibleBounds = {};
        HRESULT hr = FxCallbacks::DXamlCore_GetVisibleContentBoundsForElement(element, &visibleBounds);
        if (SUCCEEDED(hr) && visibleBounds.Width > 0.0f && visibleBounds.Height > 0.0f)
        {
            const bool nudgeHorizontalSides = !isBorderlessRevealFocus;
            const bool nudgeVerticalSides = !isBorderlessRevealFocus;
            NudgeInsideContainer(
                focusRectBoundsInClientSpace,
                elementBoundsInClientSpace,
                ConvertRectToXRectF(visibleBounds),
                NudgeMode::OnlyWhenElementIsOnContainerBoundary,
                nudgeHorizontalSides,
                nudgeVerticalSides);
        }
    }

    // For a scroller or content under a scroll clip, nudge the focus rect inside of the content's bounds.
    // See UpdateFocusRect for high-level strategy. We don't do this if using reveal focus and we are borderless.
    // Otherwise you see the content behind the reveal focus glow. In this scenario the focus will be clipped, but it's
    // better than the alternative

    bool nudgeOnHorizontalEdgesOfScrollClip = false;
    bool nudgeOnVerticalEdgesOfScrollClip = false;

    if (focusRectHost.ScrollContentPresenter && !isBorderlessRevealFocus)
    {
        // We allow the focus rect to escape the SCP clip in this case, so we want to make sure
        // not to nudge
        ScrollDirection direction = GetScrollDirection(focusRectHost.ScrollContentPresenter);
        nudgeOnHorizontalEdgesOfScrollClip = direction.Horizontal;
        nudgeOnVerticalEdgesOfScrollClip = direction.Vertical;
    }
    else if (focusRectHost.Element && !isBorderlessRevealFocus)
    {
        // Even if the host element is expected to clip the content (i.e. no nudging inside the clip), we
        // still need to nudge on the *edges*. In other words, we won't want the focus rect to be clipped in
        // the overpan areas.
        nudgeOnHorizontalEdgesOfScrollClip = !NudgeFocusRectInsideHorizontalClip(focusRectHost.Element);
        nudgeOnVerticalEdgesOfScrollClip = !NudgeFocusRectInsideVerticalClip(focusRectHost.Element);
    }

    // Collect rectangular clips all the way to the root.  If part of the element is clipped out,
    // we don't want to draw the focus rect on the space where it's invisible.  Instead nudge the
    // focus rect to be around the visible portion of the element.
    XRECTF clipRect = {};
    IFC_RETURN(GetAncestorClipsInClientSpace(element, focusRectHost, /*isNodeAncestorOfFocusRectHost*/ false, clipRect));

    if (!IsInfiniteRectF(clipRect))
    {
        const bool nudgeHorizontalSides = !isBorderlessRevealFocus;
        const bool nudgeVerticalSides = !isBorderlessRevealFocus;
        NudgeInsideContainer(
            focusRectBoundsInClientSpace,
            elementBoundsInClientSpace,
            clipRect,
            NudgeMode::Always,
            nudgeHorizontalSides,
            nudgeVerticalSides);
    }

    if (nudgeOnHorizontalEdgesOfScrollClip || nudgeOnVerticalEdgesOfScrollClip)
    {
        // If the host is a scrolling surface, the content is expected to be their child.
        // If it's not, then we assume the host is itself the host of some scrolling surface (e.g. root ScrollViewer).
        const bool isScroller =
            focusRectHost.ScrollContentPresenter ||
            (focusRectHost.Element && focusRectHost.Element->IsScroller());

        CUIElement* scrollContent =
            isScroller ?
            (focusRectHost.ScrollContentPresenter ? focusRectHost.ScrollContentPresenter : focusRectHost.Element)->GetFirstChildNoAddRef() :
            focusRectHost.Element;

        xref_ptr<CGeneralTransform> scrollContentToClientTransform;
        scrollContent->TransformToVisual(nullptr, &scrollContentToClientTransform);

        XRECTF scrollContentBounds = GetElementLayoutBoundsWithMargins(scrollContent);

        // The scroll content may have an explicit clip on top of the layout clip.
        CRectangleGeometry* rectangle = do_pointer_cast<CRectangleGeometry>(scrollContent->GetClip());
        if (rectangle)
        {
            XRECTF rectangleClipRect = rectangle->m_rc;

            if (rectangle->m_pTransform)
            {
                XRECTF transformedClipRect = {};
                IFC_RETURN(rectangle->m_pTransform->TransformRect(rectangleClipRect, &transformedClipRect));

                rectangleClipRect = transformedClipRect;
            }

            IntersectRect(&scrollContentBounds, &rectangleClipRect);
        }

        XRECTF scrollContentInClientSpace = {};
        IFC_RETURN(scrollContentToClientTransform->TransformRect(scrollContentBounds, &scrollContentInClientSpace));

        NudgeInsideContainer(
            focusRectBoundsInClientSpace,
            elementBoundsInClientSpace,
            scrollContentInClientSpace,
            NudgeMode::OnlyWhenElementIsOnContainerBoundary,
            nudgeOnHorizontalEdgesOfScrollClip,
            nudgeOnVerticalEdgesOfScrollClip);
    }

    // invert Client elementTransform and xform focusRectInClientSpace back to element space
    xref_ptr<CGeneralTransform> clientToElementTransform(GetInverseTransform(elementToClientTransform));

    // ClientToElementTransform can be null if elementToClientTransform is not invertable
    if (!clientToElementTransform)
    {
        IFC_RETURN(E_FAIL);
    }
    IFC_RETURN(clientToElementTransform->TransformRect(focusRectBoundsInClientSpace, &transformedClientToElementRect));

    return S_OK;
}

} //namespace



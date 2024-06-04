// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "uielement.h"

#include <corep.h>
#include <GeneralTransform.h>
#include <host.h>
#include <uielementcollection.h>
#include <LayoutManager.h>
#include "LayoutCycleDebugSettings.h"

void ComputeUnidimensionalEffectiveViewport(
    _In_ const std::vector<CUIElement::UnidimensionalViewportInformation>& viewports,
    _Out_ float& visibleOffset,
    _Out_ float& visibleLength)
{
    if (viewports.empty())
    {
        visibleOffset = std::numeric_limits<float>::infinity();
        visibleLength = -std::numeric_limits<float>::infinity();
    }
    else
    {
        float visibleNear = 0.0f;
        float visibleFar = 0.0f;

        for (auto it = viewports.begin(); it != viewports.end(); it++)
        {
            auto& info = *it;
            float viewportNear = info.GetGlobalOffset();
            float viewportFar = viewportNear + info.GetLength();

            if (it == viewports.begin())
            {
                // The visible length matches the first viewport if there are no
                // other viewports to consider.
                visibleNear = viewportNear;
                visibleFar = viewportFar;
                visibleLength = info.GetLength();
            }
            else
            {
                // Obtain the new visible length by intersecting the new
                // viewport with the current visible rect.
                // Note: If these do not intersect, the resulting visible
                // length will be <= 0, and the visible near and far will now
                // be invalid.
                visibleLength = std::max(0.0f, (std::min(viewportFar, visibleFar) - std::max(viewportNear, visibleNear)));
                visibleNear = std::max(viewportNear, visibleNear);
                visibleFar = visibleNear + visibleLength;
            }

            if (visibleLength <= 0.0f)
            {
                break;
            }
        }

        if (visibleLength <= 0.0f)
        {
            // There is no visible rect.
            visibleOffset = std::numeric_limits<float>::infinity();
            visibleLength = -std::numeric_limits<float>::infinity();
        }
        else
        {
            visibleOffset = visibleNear;
        }
    }
}

void ComputeUnidimensionalMaxViewport(
    _In_ const std::vector<CUIElement::UnidimensionalViewportInformation>& viewports,
    _Out_ float& maxOffset,
    _Out_ float& maxLength)
{
    if (viewports.empty())
    {
        maxOffset = std::numeric_limits<float>::infinity();
        maxLength = -std::numeric_limits<float>::infinity();
    }
    else
    {
        float maxNear = 0.0f;
        float maxFar = 0.0f;

        for (auto it = viewports.begin(); it != viewports.end(); it++)
        {
            auto& info = *it;
            float viewportNear = info.GetGlobalOffset();
            float viewportFar = viewportNear + info.GetLength();

            if (it == viewports.begin() || (maxLength >= info.GetLength()))
            {
                // The max length matches the first viewport if there are no
                // other viewports to consider. Or if we find a smaller viewport
                // in the way, we start again with that one.
                maxNear = viewportNear;
                maxFar = viewportFar;
                maxLength = info.GetLength();
            }
            else
            {
                float nearToNear = viewportNear - maxNear;
                float farToFar = viewportFar - maxFar;

                // If the current max rect is not within the rect of the new
                // viewport, we need to project the former onto the latter.
                if (!(nearToNear <= 0.0f && farToFar >= 0.0f))
                {
                    if (std::abs(nearToNear) < std::abs(farToFar))
                    {
                        maxNear = viewportNear;
                        maxFar = viewportNear + maxLength;
                    }
                    else
                    {
                        maxNear = viewportFar - maxLength;
                        maxFar = viewportFar;
                    }
                }
            }
        }

        maxOffset = maxNear;
    }
}

void ComputeUnidimensionalBringIntoViewDistance(
    _In_ const float elementOffset,
    _In_ const float elementLength,
    _In_ const std::vector<CUIElement::UnidimensionalViewportInformation>& viewports,
    _Out_ float& distance)
{
    if (viewports.empty())
    {
        distance = std::numeric_limits<float>::infinity();
    }
    else
    {
        float tempOffset = elementOffset;
        float tempLength = elementLength;
        distance = 0.0f;

        for (auto it = viewports.rbegin(); it != viewports.rend(); it++)
        {
            auto& info = *it;
            float viewportNear = info.GetGlobalOffset();
            float viewportFar = viewportNear + info.GetLength();
            float nearToNear = info.GetGlobalOffset() - tempOffset;
            float farToFar = info.GetGlobalOffset() + info.GetLength() - (tempOffset + tempLength);

            tempLength = std::min(tempLength, info.GetLength());

            if (nearToNear <= 0.0f && farToFar >= 0.0f)
            {
                // This element is entirely within the viewport and can be seen
                // entirely through it; no correction is needed.
            }
            else if (nearToNear > 0.0f && farToFar < 0.0f)
            {
                // The element is larger than the viewport and the max possible
                // area is visible; no correction is needed.
                tempOffset = viewportNear;
            }
            else if (std::abs(nearToNear) <= std::abs(farToFar))
            {
                distance += std::abs(nearToNear);
                tempOffset = viewportNear;
            }
            else
            {
                distance += abs(farToFar);
                tempOffset = viewportFar - tempLength;
            }
        }
    }
}


// Invalidate the content and outer bounds of the element.
void CUIElement::InvalidateElementBounds()
{
    m_contentInnerboundsDirty = TRUE;
    m_combinedInnerBoundsDirty = TRUE;
    m_outerBoundsDirty = TRUE;
}

// Invalidate the child, combined and outer bounds of the element.
void CUIElement::InvalidateChildBounds()
{
    m_childBoundsDirty = TRUE;
    m_combinedInnerBoundsDirty = TRUE;
    m_outerBoundsDirty = TRUE;
}

bool CUIElement::AreBoundsDirty()
{
    return (AreInnerBoundsDirty() || AreOuterBoundsDirty());
}

bool CUIElement::AreInnerBoundsDirty()
{
    return m_combinedInnerBoundsDirty;
}

bool CUIElement::AreContentInnerBoundsDirty()
{
    return m_contentInnerboundsDirty;
}

bool CUIElement::AreOuterBoundsDirty()
{
    return m_outerBoundsDirty;
}

bool CUIElement::AreChildBoundsDirty()
{
    return m_childBoundsDirty;
}

void CUIElement::CleanContentInnerBounds()
{
    m_contentInnerboundsDirty = FALSE;
}

void CUIElement::CleanOuterBounds()
{
    m_outerBoundsDirty = FALSE;
}

void CUIElement::CleanChildBounds()
{
    m_childBoundsDirty = FALSE;
}

void CUIElement::CleanInnerBounds()
{
    m_combinedInnerBoundsDirty = FALSE;
}

bool CUIElement::GetRequiresMeasure() const
{
    return !!GetLayoutFlagsOr(LF_MEASURE_DIRTY | LF_ON_MEASURE_DIRTY_PATH);
}

bool CUIElement::GetRequiresArrange() const
{
    return !!GetLayoutFlagsOr(LF_ARRANGE_DIRTY | LF_ON_ARRANGE_DIRTY_PATH);
}

bool CUIElement::GetRequiresLayout() const
{
    return !!GetLayoutFlagsOr(LF_MEASURE_DIRTY | LF_ON_MEASURE_DIRTY_PATH | LF_ARRANGE_DIRTY | LF_ON_ARRANGE_DIRTY_PATH);
}

bool CUIElement::GetIsViewportDirtyOrOnViewportDirtyPath() const
{
    return !!GetLayoutFlagsOr(LF_VIEWPORT_DIRTY | LF_ON_VIEWPORT_DIRTY_PATH);
}

bool CUIElement::GetWantsViewportOrContributesToViewport() const
{
    return !!GetLayoutFlagsOr(LF_WANTS_VIEWPORT | LF_CONTRIBUTES_TO_VIEWPORT);
}

void CUIElement::PropagateOnPathInternal(XUINT32 pathFlag, XUINT32 dirtyFlag, XUINT32 pendingFlag, bool stopAtLayoutSuspended)
{
    CUIElement *pParent = GetUIElementParentInternal();
    CUIElement *pChild = this;
    while (pParent)
    {
        // Due to explicit calls to measure/arrange it is possible that we could end up processing and
        // element that is in a collpased part of the tree.  If so (and we are requested to do so), don't
        // propagate the flag into the suspended element.
        if (stopAtLayoutSuspended & pParent->GetIsLayoutSuspended())
        {
            return;
        }
        if (pParent->GetLayoutFlagsAnd(pathFlag))
        {
            return;
        }

        pParent->SetLayoutFlags(pathFlag, TRUE);

        if (pParent->GetLayoutFlagsAnd(pendingFlag))
        {
            pParent->SetLayoutFlags(dirtyFlag, TRUE);
            pParent->SetLayoutFlags(pendingFlag, FALSE);
        }

        pChild = pParent;
        pParent = pParent->GetUIElementParentInternal();
    }

    // If we propagated these flags all the way up to the root of the visual tree without
    // early exiting because the flags were redundantly set, ensure another UI thread
    // frame is scheduled to process the changes.
    ASSERT(pParent == NULL);
    if (pChild->OfTypeByIndex<KnownTypeIndex::RootVisual>())
    {
        // The browser host and/or frame scheduler can be NULL during shutdown.
        IXcpBrowserHost *pBH = GetContext()->GetBrowserHost();
        if (pBH != NULL)
        {
            ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

            // There's no need to schedule another tick for tree changes during ticking.
            // Layout happens right before rendering after all other processing in Tick,
            // and the render walk itself should never invalidate layout again.  That means
            // any layout changes during ticking will be processed in the current frame and
            // no additional frame needs to be scheduled.
            if (pFrameScheduler != NULL && !pFrameScheduler->IsInTick())
            {
                VERIFYHR(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::RootVisualDirty));
            }
        }
    }
}

void CUIElement::PropagateOnMeasureDirtyPath()
{
    PropagateOnPathInternal(LF_ON_MEASURE_DIRTY_PATH, LF_MEASURE_DIRTY, LF_MEASURE_DIRTY_PENDING);
}

void CUIElement::PropagateOnArrangeDirtyPath(bool stopAtLayoutSuspended)
{
    PropagateOnPathInternal(LF_ON_ARRANGE_DIRTY_PATH, LF_ARRANGE_DIRTY, LF_ARRANGE_DIRTY_PENDING, stopAtLayoutSuspended);
}

void CUIElement::PropagateOnAutomationPeerDirtyPath()
{
    // Because the pending flag is '0', this call propagates
    // the path flag, but also the dirty flag to all its ancestors. Is this intended?
    PropagateOnPathInternal(LF_ON_AUTOMATION_PEER_DIRTY_PATH, LF_AUTOMATION_PEER_DIRTY, 0);
}

void CUIElement::PropagateOnViewportDirtyPath()
{
    PropagateOnPathInternal(LF_ON_VIEWPORT_DIRTY_PATH, 0, 0);
}

void CUIElement::PropagateOnContributesToViewport()
{
    PropagateOnPathInternal(LF_CONTRIBUTES_TO_VIEWPORT, 0, 0);
}

void CUIElement::InvalidateViewport()
{
    if (!GetIsViewportDirty())
    {
        if (GetWantsViewportOrContributesToViewport())
        {
            InvalidateViewportInternal();
        }
    }
}

void CUIElement::InvalidateViewportInternal()
{
    CLayoutManager* layoutManager = VisualTree::GetLayoutManagerForElement(this);
    if (layoutManager && layoutManager->StoreLayoutCycleWarningContexts())
    {
        StoreLayoutCycleWarningContext(layoutManager);
        if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Low))
        {
            __debugbreak();
        }
    }

    SetIsViewportDirty(TRUE);
    PropagateOnViewportDirtyPath();
}

// Strictly, this function only handles the inspection and manipulation of the
// the layout flags in order to perform the walk. All other logic should be
// implemented in CUIElement::EffectiveViewportWalkCore.
_Check_return_ HRESULT CUIElement::EffectiveViewportWalk(
    const bool dirtyFound,
    _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
    _In_ std::vector<UnidimensionalViewportInformation>& horizontalViewports,
    _In_ std::vector<UnidimensionalViewportInformation>& verticalViewports)
{
    if (GetIsLayoutSuspended() || !HasLayoutStorage())
    {
        return S_OK;
    }

    // Prevent the tree from changing while we're walking it.
    auto scopedParentLock = LockParent();

    if (dirtyFound && GetWantsViewport())
    {
        IFC_RETURN(ComputeEffectiveViewportChangedEventArgsAndNotifyLayoutManager(
            transformsToViewports,
            horizontalViewports,
            verticalViewports));
    }

    // Walk down the visual tree through the viewport dirty path, until we find
    // the first element that is viewport dirty. Once found, keep walking down
    // through the elements marked as listeners or contributors.
    if (GetIsViewportDirtyOrOnViewportDirtyPath() || (dirtyFound && GetContributesToViewport()))
    {
        bool newDirtyFound = dirtyFound || GetIsViewportDirty();

        // The following is an important consideration:
        // When an element is removed from the tree, we do not know to clear the 'contributing' flag
        // because there might have been other listeners as well. So the idea of the tree walk is to
        // clear everything on the way down and then do another invalidation for the listeners that are
        // still on the tree. This guarantees that the flags will always be correct after a walk.
        SetContributesToViewport(false);
        SetIsViewportDirty(false);
        SetIsOnViewportDirtyPath(false);

        bool addedViewports = false;
        IFC_RETURN(EffectiveViewportWalkCore(
            transformsToViewports,
            horizontalViewports,
            verticalViewports,
            addedViewports));

        auto children = GetUnsortedChildren();
        XUINT32 childrenCount = children.GetCount();

        for (XUINT32 childIndex = 0; childIndex < childrenCount; childIndex++)
        {
            CUIElement* currentChild = children[childIndex];
            ASSERT(currentChild);

            if (currentChild->GetIsViewportDirtyOrOnViewportDirtyPath()
                || (newDirtyFound && currentChild->GetWantsViewportOrContributesToViewport()))
            {
                IFC_RETURN(EffectiveViewportWalkToChild(
                    currentChild,
                    newDirtyFound,
                    transformsToViewports,
                    horizontalViewports,
                    verticalViewports));
            }

            // If at least one of the children still has the viewport
            // flags, reconstruct the path of 'contributing' flags.
            if (currentChild->GetContributesToViewport() || currentChild->GetWantsViewport())
            {
                SetContributesToViewport(true);
            }
        }

        // Remove the added viewports so that we can reuse these vectors while
        // traversing the rest of the visual tree.
        if (addedViewports)
        {
            // This one is a little tricky. CXamlIslandRoot::EffectiveViewportWalkCore will push two viewports without
            // pushing a transform, so we technically can't blindly pop all three vectors. But given CXamlIslandRoot is
            // always the first element to push anything, the transform vector will be empty anyway when we get back up
            // to CXamlIslandRoot to pop things.
            transformsToViewports.pop_back();

            horizontalViewports.pop_back();
            verticalViewports.pop_back();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CUIElement::EffectiveViewportWalkToChild(
    _In_ CUIElement* child,
    const bool dirtyFound,
    _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
    _In_ std::vector<UnidimensionalViewportInformation>& horizontalViewports,
    _In_ std::vector<UnidimensionalViewportInformation>& verticalViewports)
{
    IFC_RETURN(child->EffectiveViewportWalk(
        dirtyFound,
        transformsToViewports,
        horizontalViewports,
        verticalViewports));

    return S_OK;
}
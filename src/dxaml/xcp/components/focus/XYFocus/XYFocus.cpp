// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XYFocus.h>

#include <corep.h>
#include <FocusProperties.h>

#include <Popup.h>

#include <Bubbling.h>
#include <TreeWalker.h>

#include <XYFocusAlgorithms.h>
#include <ProximityStrategy.h>

#include <DoPointerCast.h>
#include <TextElement.h>

#include "Focusability.h"

#include "MUX-ETWEvents.h"

using namespace Focus;

XYFocusAlgorithms m_heuristic;

XYFocus::Manifolds XYFocus::ResetManifolds()
{
    XYFocus::Manifolds manifolds = m_manifolds;
    m_manifolds.Reset();

    return manifolds;
}

void XYFocus::SetManifolds(_In_ XYFocus::Manifolds& manifolds)
{
    m_manifolds.vManifold = manifolds.vManifold;
    m_manifolds.hManifold = manifolds.hManifold;
}

void XYFocus::ClearCache()
{
    m_exploredList.clear();
}

CDependencyObject* XYFocus::GetNextFocusableElement(
    _In_ DirectUI::FocusNavigationDirection direction,
    _In_opt_ CDependencyObject* element,
    _In_opt_ CDependencyObject* engagedControl,
    _In_ VisualTree* visualTree,
    _In_ bool updateManifolds,
    _In_ XYFocusOptions& xyFocusOptions)
{
    if (element == nullptr) { return nullptr; }

    size_t hash = 0;
    if (m_exploredList.empty() == false)
    {
        hash = ExploredListHash(direction, element, engagedControl, xyFocusOptions);
        if (std::find(m_exploredList.begin(), m_exploredList.end(), hash) != m_exploredList.end())
        {
            CacheHitTrace(direction);
            return nullptr;
        }
    }

    const auto root = visualTree->GetRootOrIslandForElement(element);
    const bool isRightToLeft = element->IsRightToLeft();

    const DirectUI::XYFocusNavigationStrategy mode = XYFocusPrivate::GetStrategy(element, direction, xyFocusOptions.navigationStrategyOverride);

    XRECTF_RB rootBounds;
    XRECTF_RB focusedElementBounds = xyFocusOptions.focusedElementBounds;

    CDependencyObject* nextFocusableElement = XYFocusPrivate::GetDirectionOverride(element, xyFocusOptions.searchRoot, direction, true /*ignoreFocusability*/);

    if (nextFocusableElement)
    {
        return nextFocusableElement;
    }

    const CDependencyObject* activeScroller = GetActiveScrollerForScrollInput(direction, element);
    const bool isProcessingInputForScroll = (activeScroller != nullptr);

    if (xyFocusOptions.focusHintRectangle != nullptr)
    {
        // Because we have a focus hint rectangle, we should not have the focused element have any role in what elements are chosen as a candidate
        focusedElementBounds = *xyFocusOptions.focusHintRectangle;
        element = nullptr;
    }

    if (engagedControl)
    {
        rootBounds = XYFocusPrivate::GetBoundsForRanking(engagedControl, xyFocusOptions.ignoreClipping);
    }
    else if (xyFocusOptions.searchRoot)
    {
        rootBounds = XYFocusPrivate::GetBoundsForRanking(xyFocusOptions.searchRoot, xyFocusOptions.ignoreClipping);
    }
    else
    {
        rootBounds = XYFocusPrivate::GetBoundsForRanking(root, xyFocusOptions.ignoreClipping);
    }

    auto candidateList = GetAllValidFocusableChildren(root, direction, element, engagedControl, xyFocusOptions.searchRoot, visualTree, activeScroller, xyFocusOptions.ignoreClipping, xyFocusOptions.shouldConsiderXYFocusKeyboardNavigation);

    if (!candidateList.empty())
    {
        double maxRootBoundsDistance = std::max(rootBounds.right - rootBounds.left, rootBounds.bottom - rootBounds.top);
        maxRootBoundsDistance = std::max(maxRootBoundsDistance, GetMaxRootBoundsDistance(candidateList, focusedElementBounds, direction, xyFocusOptions.ignoreClipping));

        RankElements(candidateList, direction, &focusedElementBounds, maxRootBoundsDistance, mode, xyFocusOptions.exclusionRect, xyFocusOptions.ignoreClipping, xyFocusOptions.ignoreCone);

#ifdef XYFOCUS_DBG
        for (const Focus::XYFocus::XYFocusParams& it : candidateList)
        {
            RAWTRACE(TraceAlways, L"Candidate: %x %f,%f %f,%f rank %f",
                it.element,
                it.bounds.left,
                it.bounds.top,
                it.bounds.right,
                it.bounds.bottom,
                it.score);
        }
#endif // XYFOCUS_DBG
        const bool ignoreOcclusivity = xyFocusOptions.ignoreOcclusivity || isProcessingInputForScroll;

        //Choose the best candidate, after testing for occlusivity, if we're currently scrolling, the test has been done already, skip it.
        nextFocusableElement = ChooseBestFocusableElementFromList(candidateList, direction, visualTree, &focusedElementBounds, xyFocusOptions.ignoreClipping, ignoreOcclusivity, isRightToLeft, xyFocusOptions.updateManifold && updateManifolds);
        nextFocusableElement = XYFocusPrivate::TryXYFocusBubble(element, nextFocusableElement, xyFocusOptions.searchRoot, direction);
    }

    // Store the result in the explored list if the candidate is null.
    if (nextFocusableElement == nullptr)
    {
        if (hash == 0)
        {
            hash = ExploredListHash(direction, element, engagedControl, xyFocusOptions);
        }

        m_exploredList.push_back(hash);
    }

    return nextFocusableElement;
}

CDependencyObject* XYFocus::ChooseBestFocusableElementFromList(
    _In_ std::vector<XYFocusParams>& scoreList,
    _In_ DirectUI::FocusNavigationDirection direction,
    _In_ VisualTree* visualTree,
    _In_ const XRECTF_RB* bounds,
    _In_ bool ignoreClipping,
    _In_ bool ignoreOcclusivity,
    _In_ bool isRightToLeft,
    _In_ bool updateManifolds)
{
    CDependencyObject* bestElement = nullptr;
    std::stable_sort(scoreList.begin(), scoreList.end(), [&](const XYFocusParams& elementA, const XYFocusParams& elementB)
    {
        if (elementA.score == elementB.score)
        {
            const XRECTF_RB firstBounds = elementA.bounds;
            const XRECTF_RB secondBounds = elementB.bounds;

            // In the case of a tie, we want to choose the element furthest top or left (depending on FocusNavigation and FlowDirection)
            if (direction == DirectUI::FocusNavigationDirection::Up || direction == DirectUI::FocusNavigationDirection::Down)
            {
                if (isRightToLeft)
                {
                    return firstBounds.left > secondBounds.left;
                }

                return firstBounds.left < secondBounds.left;
            }
            else
            {
                return firstBounds.top < secondBounds.top;
            }
        }

        return elementA.score > elementB.score;
    });

    for (const auto& param : scoreList)
    {
        if (param.score <= 0) { break; }

        // When passing in the bounds for OcclusivityTesting, we want to ensure that we are using the non clipped bounds. Therefore, if ignoreClipping is
        // set to true, that means that our cached bounds are invalid for OcclusivityTesting.
        const XRECTF_RB boundsForOccTesting = ignoreClipping ? XYFocusPrivate::GetBoundsForRanking(param.element, false) : param.bounds;

        // Don't check for occlusivity if we've already covered occlusivity scenarios for scrollable content or have been asked
        // to ignore occlusivity by the caller.
        if (!IsInvalidRectF(param.bounds) && (ignoreOcclusivity || !XYFocusPrivate::IsOccluded(param.element, boundsForOccTesting)))
        {
            bestElement = param.element;

            if (updateManifolds)
            {
                //Update the manifolds with the newly selected focus
                m_heuristic.UpdateManifolds(direction, *bounds, param.bounds, m_manifolds.hManifold, m_manifolds.vManifold);
            }

            break;
        }
    }

    return bestElement;
}

void XYFocus::UpdateManifolds(
    _In_ DirectUI::FocusNavigationDirection direction,
    _In_ const XRECTF_RB& elementBounds,
    _In_ CDependencyObject* const candidate,
    _In_ bool ignoreClipping)
{
    const XRECTF_RB candidateBounds = XYFocusPrivate::GetBoundsForRanking(candidate, ignoreClipping);
    m_heuristic.UpdateManifolds(direction, elementBounds, candidateBounds, m_manifolds.hManifold, m_manifolds.vManifold);
}

std::vector<Focus::XYFocus::XYFocusParams> XYFocus::GetAllValidFocusableChildren(
    _In_ CDependencyObject* startRoot,
    _In_ DirectUI::FocusNavigationDirection direction,
    _In_ const CDependencyObject* currentElement,
    _In_ CDependencyObject* engagedControl,
    _In_ CDependencyObject* searchScope,
    _In_ VisualTree* visualTree,
    _In_ const CDependencyObject* activeScroller,
    _In_ bool ignoreClipping,
    _In_ bool shouldConsiderXYFocusKeyboardNavigation)
{
    CDependencyObject* rootForTreeWalk = startRoot;
    std::vector<Focus::XYFocus::XYFocusParams> candidateList;
    FocusWalkTraceBegin(direction);

    //If asked to scope the search within the given container, honor it without any exceptions
    if (searchScope != nullptr)
    {
        rootForTreeWalk = searchScope;
    }

    if (engagedControl == nullptr)
    {
        candidateList = XYFocusPrivate::FindElements(rootForTreeWalk, currentElement, activeScroller, ignoreClipping, shouldConsiderXYFocusKeyboardNavigation);
    }
    else
    {
        //Only run through this when you are an engaged element. Being an engaged element means that you should only
        //look at the children of the engaged element and any children of popups that were opened during engagement
        //TODO: engagement only happens on Popup root and public root, but should happen on all roots
        const auto& popupChildrenDuringEngagement = CPopupRoot::GetPopupChildrenOpenedDuringEngagement(engagedControl);
        candidateList = XYFocusPrivate::FindElements(engagedControl, currentElement, activeScroller, ignoreClipping, shouldConsiderXYFocusKeyboardNavigation);

        // Iterate though the popups and add their children to the list
        for (const auto& popup : popupChildrenDuringEngagement)
        {
            auto subCandidateList = XYFocusPrivate::FindElements(popup, currentElement, activeScroller, ignoreClipping, shouldConsiderXYFocusKeyboardNavigation);
            candidateList.insert(candidateList.end(), subCandidateList.begin(), subCandidateList.end());
        }

        if (currentElement != engagedControl)
        {
            auto bounds = XYFocusPrivate::GetBoundsForRanking(engagedControl, ignoreClipping);

            Focus::XYFocus::XYFocusParams params;
            params.element = engagedControl;
            params.bounds = bounds;
            candidateList.push_back(params);
        }
    }

    TraceXYFocusWalkEnd();
    return candidateList;
}

void XYFocus::RankElements(
    _Inout_ std::vector<XYFocusParams>& candidateList,
    _In_ DirectUI::FocusNavigationDirection direction,
    _In_ const XRECTF_RB* bounds,
    _In_ const double maxRootBoundsDistance,
    _In_ DirectUI::XYFocusNavigationStrategy mode,
    _In_ XRECTF_RB* exclusionRect,
    _In_opt_ bool ignoreClipping,
    _In_opt_ bool ignoreCone)
{
    XRECTF_RB exclusionBounds;
    EmptyRectF(&exclusionBounds);
    if (exclusionRect)
    {
        exclusionBounds = *exclusionRect;
    }

    for (auto& candidate : candidateList)
    {
        const XRECTF_RB candidateBounds = candidate.bounds;

        if (!( DoRectsIntersect(exclusionBounds, candidateBounds)
            || DoesRectContainRect(/*container*/&exclusionBounds, /*contained element*/&candidateBounds)))
        {
            if (mode == DirectUI::XYFocusNavigationStrategy::Projection &&
                m_heuristic.ShouldCandidateBeConsideredForRanking(*bounds, candidateBounds, maxRootBoundsDistance, direction, exclusionBounds, ignoreCone))
            {
                candidate.score = m_heuristic.GetScore(direction, *bounds, candidateBounds, m_manifolds.hManifold, m_manifolds.vManifold, maxRootBoundsDistance);
            }
            else if (mode == DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance || mode == DirectUI::XYFocusNavigationStrategy::RectilinearDistance)
            {
                candidate.score = XYFocusPrivate::ProximityStrategy::GetScore(direction, *bounds, candidateBounds, maxRootBoundsDistance, mode == DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            }
        }
    }
}

double XYFocus::GetMaxRootBoundsDistance(
    _In_ const std::vector<XYFocusParams>& list,
    _In_ const XRECTF_RB& bounds,
    _In_ DirectUI::FocusNavigationDirection direction,
    _In_opt_ bool ignoreClipping)
{
    auto max = std::max_element(list.begin(), list.end(), [&](XYFocusParams paramA, XYFocusParams paramB)
    {
        const XRECTF_RB candidateBounds = paramA.bounds;
        const XRECTF_RB candidateBBounds = paramB.bounds;

        if (direction == DirectUI::FocusNavigationDirection::Left) { return candidateBounds.left < candidateBBounds.left; }
        else if (direction == DirectUI::FocusNavigationDirection::Right) { return candidateBounds.right > candidateBBounds.right; }
        else if (direction == DirectUI::FocusNavigationDirection::Up) { return candidateBounds.top < candidateBBounds.top; }
        else if (direction == DirectUI::FocusNavigationDirection::Down) { return candidateBounds.bottom > candidateBBounds.bottom; }

        return false;
    });

    XRECTF_RB maxBounds = max->bounds;

    if (direction == DirectUI::FocusNavigationDirection::Left) { return std::abs(maxBounds.right - bounds.left); }
    else if (direction == DirectUI::FocusNavigationDirection::Right) { return std::abs(bounds.right - maxBounds.left); }
    else if (direction == DirectUI::FocusNavigationDirection::Up) { return std::abs(bounds.bottom - maxBounds.top); }
    else if (direction == DirectUI::FocusNavigationDirection::Down) { return std::abs(maxBounds.bottom - bounds.top); }

    return 0;
}

const CDependencyObject* const XYFocus::GetActiveScrollerForScrollInput(
    _In_ const DirectUI::FocusNavigationDirection direction,
    _In_opt_ CDependencyObject* const focusedElement)
{
    CDependencyObject* parent = nullptr;
    CTextElement* textElement = do_pointer_cast<CTextElement>(focusedElement);
    if (textElement)
    {
        parent = textElement->GetContainingFrameworkElement();
    }
    else
    {
        parent = focusedElement;
    }

    while (parent)
    {
        auto element = do_pointer_cast<CUIElement>(parent);
        if (element && element->IsScroller())
        {
            bool isHorizontallyScrollable = false;
            bool isVerticallyScrollable = false;
            FocusProperties::IsScrollable(element, &isHorizontallyScrollable, &isVerticallyScrollable);

            const bool isHorizontallyScrollableForDirection = IsHorizontalNavigationDirection(direction) && isHorizontallyScrollable;
            const bool isVerticallyScrollableForDirection = IsVerticalNavigationDirection(direction) && isVerticallyScrollable;

            ASSERT(!(isHorizontallyScrollableForDirection && isVerticallyScrollableForDirection));

            if (isHorizontallyScrollableForDirection || isVerticallyScrollableForDirection)
            {
                return element;
            }
        }
        parent = parent->GetParent();
    }
    return nullptr;
}

bool XYFocus::IsHorizontalNavigationDirection(_In_ const DirectUI::FocusNavigationDirection direction)
{
    return (direction == DirectUI::FocusNavigationDirection::Left || direction == DirectUI::FocusNavigationDirection::Right);
}

bool XYFocus::IsVerticalNavigationDirection(_In_ const DirectUI::FocusNavigationDirection direction)
{
    return (direction == DirectUI::FocusNavigationDirection::Up || direction == DirectUI::FocusNavigationDirection::Down);
}

void XYFocus::SetPrimaryAxisDistanceWeight(int primaryAxisDistanceWeight) { m_heuristic.SetPrimaryAxisDistanceWeight(primaryAxisDistanceWeight); }
void XYFocus::SetSecondaryAxisDistanceWeight(int secondaryAxisDistanceWeight) { m_heuristic.SetSecondaryAxisDistanceWeight(secondaryAxisDistanceWeight); }
void XYFocus::SetPercentInManifoldShadowWeight(int percentInManifoldShadowWeight) { m_heuristic.SetPercentInManifoldShadowWeight(percentInManifoldShadowWeight); }
void XYFocus::SetPercentInShadowWeight(int percentInShadowWeight) { m_heuristic.SetPercentInShadowWeight(percentInShadowWeight); }

std::size_t XYFocus::ExploredListHash(
    _In_ DirectUI::FocusNavigationDirection direction,
    _In_opt_ CDependencyObject* element,
    _In_opt_ CDependencyObject* engagedControl,
    _In_ const XYFocusOptions& xyFocusOptions)
{
    std::size_t hash = 0;
    CommonUtilities::hash_combine(hash, direction);
    CommonUtilities::hash_combine(hash, element);
    CommonUtilities::hash_combine(hash, engagedControl);
    CommonUtilities::hash_combine(hash, xyFocusOptions.hash());

    return hash;
}

void XYFocus::FocusWalkTraceBegin(_In_ DirectUI::FocusNavigationDirection direction)
{
    switch (direction)
    {
    case DirectUI::FocusNavigationDirection::Next:
        TraceXYFocusWalkBegin(L"Next");
        break;
    case DirectUI::FocusNavigationDirection::Previous:
        TraceXYFocusWalkBegin(L"Previous");
        break;
    case DirectUI::FocusNavigationDirection::Up:
        TraceXYFocusWalkBegin(L"Up");
        break;
    case DirectUI::FocusNavigationDirection::Down:
        TraceXYFocusWalkBegin(L"Down");
        break;
    case DirectUI::FocusNavigationDirection::Left:
        TraceXYFocusWalkBegin(L"Left");
        break;
    case DirectUI::FocusNavigationDirection::Right:
        TraceXYFocusWalkBegin(L"Right");
        break;
    default:
        TraceXYFocusWalkBegin(L"Invalid");
    }
}

void XYFocus::CacheHitTrace(_In_ DirectUI::FocusNavigationDirection direction)
{
    switch (direction)
    {
    case DirectUI::FocusNavigationDirection::Next:
        TraceXYFocusCandidateCacheHit(L"Next");
        break;
    case DirectUI::FocusNavigationDirection::Previous:
        TraceXYFocusCandidateCacheHit(L"Previous");
        break;
    case DirectUI::FocusNavigationDirection::Up:
        TraceXYFocusCandidateCacheHit(L"Up");
        break;
    case DirectUI::FocusNavigationDirection::Down:
        TraceXYFocusCandidateCacheHit(L"Down");
        break;
    case DirectUI::FocusNavigationDirection::Left:
        TraceXYFocusCandidateCacheHit(L"Left");
        break;
    case DirectUI::FocusNavigationDirection::Right:
        TraceXYFocusCandidateCacheHit(L"Right");
        break;
    default:
        TraceXYFocusCandidateCacheHit(L"Invalid");
    }
}
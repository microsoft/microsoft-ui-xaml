// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ScrollPresenter.h"
#include "DoubleUtil.h"
#include "ScrollPresenterTestHooks.h"

// Used when ScrollPresenter.HorizontalAnchorRatio or ScrollPresenter.VerticalAnchorRatio is 0.0 or 1.0 to determine whether the Content is scrolled to an edge.
// It is declared at an edge if it's within 1/10th of a pixel.
const double c_edgeDetectionTolerance = 0.1;

void ScrollPresenter::RaiseConfigurationChanged()
{
    if (m_configurationChanged)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        m_configurationChanged(*this);
    }
}

void ScrollPresenter::RaisePostArrange()
{
    if (m_postArrange)
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_postArrange(*this);
    }
}

void ScrollPresenter::RaiseViewportChanged(const bool isFinal)
{
    if (m_viewportChanged)
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_viewportChanged(*this, isFinal);
    }
}

void ScrollPresenter::RaiseAnchorRequested()
{
    if (m_anchorRequestedEventSource)
    {
        SCROLLPRESENTER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        if (!m_anchorRequestedEventArgs)
        {
            m_anchorRequestedEventArgs = tracker_ref<winrt::ScrollingAnchorRequestedEventArgs>(this, winrt::make<ScrollingAnchorRequestedEventArgs>(*this));
        }

        com_ptr<ScrollingAnchorRequestedEventArgs> anchorRequestedEventArgs = winrt::get_self<ScrollingAnchorRequestedEventArgs>(m_anchorRequestedEventArgs.get())->get_strong();

        anchorRequestedEventArgs->SetAnchorElement(nullptr);
        anchorRequestedEventArgs->SetAnchorCandidates(m_anchorCandidates);
        m_anchorRequestedEventSource(*this, *anchorRequestedEventArgs);
    }
}

// Computes the type of anchoring to perform, if any, based on ScrollPresenter.HorizontalAnchorRatio, ScrollPresenter.VerticalAnchorRatio, 
// the current offsets, zoomFactor, viewport size, content size and state.
// When all 4 returned booleans are False, no element anchoring is performed, no far edge anchoring is performed. There may still be anchoring at near edges.
void ScrollPresenter::IsAnchoring(
    _Out_ bool* isAnchoringElementHorizontally,
    _Out_ bool* isAnchoringElementVertically,
    _Out_opt_ bool* isAnchoringFarEdgeHorizontally,
    _Out_opt_ bool* isAnchoringFarEdgeVertically)
{
    *isAnchoringElementHorizontally = false;
    *isAnchoringElementVertically = false;
    if (isAnchoringFarEdgeHorizontally)
    {
        *isAnchoringFarEdgeHorizontally = false;
    }
    if (isAnchoringFarEdgeVertically)
    {
        *isAnchoringFarEdgeVertically = false;
    }

    // Mouse wheel comes in as a custom animation, and we are currently not 
    // anchoring because of the check below. Unfortunately, I cannot validate that
    // removing the check is the correct fix due to dcomp bug 17523225. I filed a 
    // tracking bug to follow up once the dcomp bug is fixed.
    // Bug 17523266: ScrollPresenter is not anchoring during mouse wheel
    if (!m_interactionTracker || m_state == winrt::InteractionState::Animation)
    {
        // Skip calls to SetContentLayoutOffsetX / SetContentLayoutOffsetY when the InteractionTracker has not been set up yet,
        // or when it is performing a custom animation because if would result in a visual flicker.
        return;
    }

    const double horizontalAnchorRatio = HorizontalAnchorRatio();
    const double verticalAnchorRatio = VerticalAnchorRatio();

    // For edge anchoring, the near edge is considered when HorizontalAnchorRatio or VerticalAnchorRatio is 0.0. 
    // When the property is 1.0, the far edge is considered.
    if (!isnan(horizontalAnchorRatio))
    {
        MUX_ASSERT(horizontalAnchorRatio >= 0.0);
        MUX_ASSERT(horizontalAnchorRatio <= 1.0);

        if (horizontalAnchorRatio == 0.0 || horizontalAnchorRatio == 1.0)
        {
            if (horizontalAnchorRatio == 1.0 && m_zoomedHorizontalOffset + m_viewportWidth - m_unzoomedExtentWidth * m_zoomFactor > -c_edgeDetectionTolerance)
            {
                if (isAnchoringFarEdgeHorizontally)
                {
                    *isAnchoringFarEdgeHorizontally = true;
                }
            }
            else if (!(horizontalAnchorRatio == 0.0 && m_zoomedHorizontalOffset < c_edgeDetectionTolerance))
            {
                *isAnchoringElementHorizontally = true;
            }
        }
        else
        {
            *isAnchoringElementHorizontally = true;
        }
    }

    if (!isnan(verticalAnchorRatio))
    {
        MUX_ASSERT(verticalAnchorRatio >= 0.0);
        MUX_ASSERT(verticalAnchorRatio <= 1.0);

        if (verticalAnchorRatio == 0.0 || verticalAnchorRatio == 1.0)
        {
            if (verticalAnchorRatio == 1.0 && m_zoomedVerticalOffset + m_viewportHeight - m_unzoomedExtentHeight * m_zoomFactor > -c_edgeDetectionTolerance)
            {
                if (isAnchoringFarEdgeVertically)
                {
                    *isAnchoringFarEdgeVertically = true;
                }
            }
            else if (!(verticalAnchorRatio == 0.0 && m_zoomedVerticalOffset < c_edgeDetectionTolerance))
            {
                *isAnchoringElementVertically = true;
            }
        }
        else
        {
            *isAnchoringElementVertically = true;
        }
    }
}

// Returns:
// - viewportAnchorPointHorizontalOffset: unzoomed horizontal offset of the anchor point within the ScrollPresenter.Content. NaN if there is no horizontal anchoring.
// - viewportAnchorPointVerticalOffset: unzoomed vertical offset of the anchor point within the ScrollPresenter.Content. NaN if there is no vertical anchoring.
void ScrollPresenter::ComputeViewportAnchorPoint(
    double viewportWidth,
    double viewportHeight,
    _Out_ double* viewportAnchorPointHorizontalOffset,
    _Out_ double* viewportAnchorPointVerticalOffset)
{
    *viewportAnchorPointHorizontalOffset = DoubleUtil::NaN;
    *viewportAnchorPointVerticalOffset = DoubleUtil::NaN;

    const winrt::Rect viewportAnchorBounds{
        static_cast<float>(m_zoomedHorizontalOffset / m_zoomFactor),
        static_cast<float>(m_zoomedVerticalOffset / m_zoomFactor),
        static_cast<float>(viewportWidth / m_zoomFactor),
        static_cast<float>(viewportHeight / m_zoomFactor)
    };

    ComputeAnchorPoint(viewportAnchorBounds, viewportAnchorPointHorizontalOffset, viewportAnchorPointVerticalOffset);

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, *viewportAnchorPointHorizontalOffset, *viewportAnchorPointVerticalOffset);
}

// Returns:
// - elementAnchorPointHorizontalOffset: unzoomed horizontal offset of the anchor element's anchor point within the ScrollPresenter.Content. NaN if there is no horizontal anchoring.
// - elementAnchorPointVerticalOffset: unzoomed vertical offset of the anchor element's point within the ScrollPresenter.Content. NaN if there is no vertical anchoring.
void ScrollPresenter::ComputeElementAnchorPoint(
    bool isForPreArrange,
    _Out_ double* elementAnchorPointHorizontalOffset,
    _Out_ double* elementAnchorPointVerticalOffset)
{
    *elementAnchorPointHorizontalOffset = DoubleUtil::NaN;
    *elementAnchorPointVerticalOffset = DoubleUtil::NaN;

    MUX_ASSERT(!m_isAnchorElementDirty);

    if (m_anchorElement.get())
    {
        winrt::Rect anchorElementBounds = isForPreArrange ? m_anchorElementBounds : GetDescendantBounds(Content(), m_anchorElement.get());

        ComputeAnchorPoint(anchorElementBounds, elementAnchorPointHorizontalOffset, elementAnchorPointVerticalOffset);

        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, *elementAnchorPointHorizontalOffset, *elementAnchorPointVerticalOffset);
    }
}

void ScrollPresenter::ComputeAnchorPoint(
    const winrt::Rect& anchorBounds,
    _Out_ double* anchorPointX,
    _Out_ double* anchorPointY)
{
    if (isnan(HorizontalAnchorRatio()))
    {
        *anchorPointX = DoubleUtil::NaN;
    }
    else
    {
        MUX_ASSERT(HorizontalAnchorRatio() >= 0.0);
        MUX_ASSERT(HorizontalAnchorRatio() <= 1.0);

        *anchorPointX = anchorBounds.X + HorizontalAnchorRatio() * anchorBounds.Width;
    }

    if (isnan(VerticalAnchorRatio()))
    {
        *anchorPointY = DoubleUtil::NaN;
    }
    else
    {
        MUX_ASSERT(VerticalAnchorRatio() >= 0.0);
        MUX_ASSERT(VerticalAnchorRatio() <= 1.0);

        *anchorPointY = anchorBounds.Y + VerticalAnchorRatio() * anchorBounds.Height;
    }
}

// Computes the distance between the viewport's anchor point and the anchor element's anchor point.
winrt::Size ScrollPresenter::ComputeViewportToElementAnchorPointsDistance(
    double viewportWidth,
    double viewportHeight,
    bool isForPreArrange)
{
    if (m_anchorElement.get())
    {
        MUX_ASSERT(!isForPreArrange || IsElementValidAnchor(m_anchorElement.get()));

        if (!isForPreArrange && !IsElementValidAnchor(m_anchorElement.get()))
        {
            return winrt::Size{ FloatUtil::NaN, FloatUtil::NaN };
        }

        double elementAnchorPointHorizontalOffset{ 0.0 };
        double elementAnchorPointVerticalOffset{ 0.0 };
        double viewportAnchorPointHorizontalOffset{ 0.0 };
        double viewportAnchorPointVerticalOffset{ 0.0 };

        ComputeElementAnchorPoint(
            isForPreArrange, 
            &elementAnchorPointHorizontalOffset, 
            &elementAnchorPointVerticalOffset);
        ComputeViewportAnchorPoint(
            viewportWidth,
            viewportHeight,
            &viewportAnchorPointHorizontalOffset,
            &viewportAnchorPointVerticalOffset);

        MUX_ASSERT(!isnan(viewportAnchorPointHorizontalOffset) || !isnan(viewportAnchorPointVerticalOffset));
        MUX_ASSERT(isnan(viewportAnchorPointHorizontalOffset) == isnan(elementAnchorPointHorizontalOffset));
        MUX_ASSERT(isnan(viewportAnchorPointVerticalOffset) == isnan(elementAnchorPointVerticalOffset));

        // Rounding the distance to 6 precision digits to avoid layout cycles due to float/double conversions.
        const winrt::Size viewportToElementAnchorPointsDistance = winrt::Size{
            isnan(viewportAnchorPointHorizontalOffset) ? 
                FloatUtil::NaN : static_cast<float>(round((elementAnchorPointHorizontalOffset - viewportAnchorPointHorizontalOffset) * 1000000) / 1000000),
            isnan(viewportAnchorPointVerticalOffset) ? 
                FloatUtil::NaN : static_cast<float>(round((elementAnchorPointVerticalOffset - viewportAnchorPointVerticalOffset) * 1000000) / 1000000)
        };

        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, viewportToElementAnchorPointsDistance.Width, viewportToElementAnchorPointsDistance.Height);

        return viewportToElementAnchorPointsDistance;
    }
    else
    {
        return winrt::Size{ FloatUtil::NaN, FloatUtil::NaN };
    }
}

void ScrollPresenter::ClearAnchorCandidates()
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_anchorCandidates.clear();
    m_isAnchorElementDirty = true;
}

void ScrollPresenter::ResetAnchorElement()
{
    if (m_anchorElement.get())
    {
        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_anchorElement.set(nullptr);
        m_anchorElementBounds = winrt::Rect{};
        m_isAnchorElementDirty = false;

        com_ptr<ScrollPresenterTestHooks> globalTestHooks = ScrollPresenterTestHooks::GetGlobalTestHooks();

        if (globalTestHooks && globalTestHooks->AreAnchorNotificationsRaised())
        {
            globalTestHooks->NotifyAnchorEvaluated(*this, nullptr /*anchorElement*/, DoubleUtil::NaN /*viewportAnchorPointHorizontalOffset*/, DoubleUtil::NaN /*viewportAnchorPointVerticalOffset*/);
        }
    }
}

// Raises the ScrollPresenter.AnchorRequested event. If no anchor element was specified, selects an anchor among the candidates vector that may have been altered 
// in the AnchorRequested event handler.
void ScrollPresenter::EnsureAnchorElementSelection()
{
    if (!m_isAnchorElementDirty)
    {
        return;
    }

    m_anchorElement.set(nullptr);
    m_anchorElementBounds = winrt::Rect{};
    m_isAnchorElementDirty = false;

    com_ptr<ScrollPresenterTestHooks> globalTestHooks = ScrollPresenterTestHooks::GetGlobalTestHooks();
    double viewportAnchorPointHorizontalOffset{ 0.0 };
    double viewportAnchorPointVerticalOffset{ 0.0 };

    ComputeViewportAnchorPoint(
        m_viewportWidth,
        m_viewportHeight,
        &viewportAnchorPointHorizontalOffset,
        &viewportAnchorPointVerticalOffset);

    MUX_ASSERT(!isnan(viewportAnchorPointHorizontalOffset) || !isnan(viewportAnchorPointVerticalOffset));

    RaiseAnchorRequested();

    auto anchorRequestedEventArgs = winrt::get_self<ScrollingAnchorRequestedEventArgs>(m_anchorRequestedEventArgs.get());
    winrt::UIElement requestedAnchorElement{ nullptr };
    winrt::IVector<winrt::UIElement> anchorCandidates{ nullptr };
    const winrt::UIElement content = Content();

    if (anchorRequestedEventArgs)
    {
        requestedAnchorElement = anchorRequestedEventArgs->GetAnchorElement();
        anchorCandidates = anchorRequestedEventArgs->GetAnchorCandidates();
    }

    if (requestedAnchorElement)
    {
        m_anchorElement.set(requestedAnchorElement);
        m_anchorElementBounds = GetDescendantBounds(content, requestedAnchorElement);

        if (globalTestHooks && globalTestHooks->AreAnchorNotificationsRaised())
        {
            globalTestHooks->NotifyAnchorEvaluated(*this, requestedAnchorElement, viewportAnchorPointHorizontalOffset, viewportAnchorPointVerticalOffset);
        }

        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::RectToString(m_anchorElementBounds).c_str());

        return;
    }

    winrt::Rect bestAnchorCandidateBounds{};
    winrt::UIElement bestAnchorCandidate{ nullptr };
    double bestAnchorCandidateDistance = std::numeric_limits<float>::max();
    const winrt::Rect viewportAnchorBounds{
        static_cast<float>(m_zoomedHorizontalOffset / m_zoomFactor),
        static_cast<float>(m_zoomedVerticalOffset / m_zoomFactor),
        static_cast<float>(m_viewportWidth / m_zoomFactor),
        static_cast<float>(m_viewportHeight / m_zoomFactor)
    };

    MUX_ASSERT(content);

    if (anchorCandidates)
    {
        for (winrt::UIElement anchorCandidate : anchorCandidates)
        {
            ProcessAnchorCandidate(
                anchorCandidate,
                content,
                viewportAnchorBounds,
                viewportAnchorPointHorizontalOffset,
                viewportAnchorPointVerticalOffset,
                &bestAnchorCandidateDistance,
                &bestAnchorCandidate,
                &bestAnchorCandidateBounds);
        }
    }
    else
    {
        for (tracker_ref<winrt::UIElement> anchorCandidateTracker : m_anchorCandidates)
        {
            const winrt::UIElement anchorCandidate = anchorCandidateTracker.get();

            ProcessAnchorCandidate(
                anchorCandidate,
                content,
                viewportAnchorBounds,
                viewportAnchorPointHorizontalOffset,
                viewportAnchorPointVerticalOffset,
                &bestAnchorCandidateDistance,
                &bestAnchorCandidate,
                &bestAnchorCandidateBounds);
        }
    }

    if (bestAnchorCandidate)
    {
        m_anchorElement.set(bestAnchorCandidate);
        m_anchorElementBounds = bestAnchorCandidateBounds;

        SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::RectToString(m_anchorElementBounds).c_str());
    }

    if (globalTestHooks && globalTestHooks->AreAnchorNotificationsRaised())
    {
        globalTestHooks->NotifyAnchorEvaluated(*this, m_anchorElement.get(), viewportAnchorPointHorizontalOffset, viewportAnchorPointVerticalOffset);
    }
}

// Checks if the provided anchor candidate is better than the current best, based on its distance to the viewport anchor point,
// and potentially updates the best candidate and its bounds.
void ScrollPresenter::ProcessAnchorCandidate(
    const winrt::UIElement& anchorCandidate,
    const winrt::UIElement& content,
    const winrt::Rect& viewportAnchorBounds,
    double viewportAnchorPointHorizontalOffset,
    double viewportAnchorPointVerticalOffset,
    _Inout_ double* bestAnchorCandidateDistance,
    _Inout_ winrt::UIElement* bestAnchorCandidate,
    _Inout_ winrt::Rect* bestAnchorCandidateBounds) const
{
    MUX_ASSERT(anchorCandidate);
    MUX_ASSERT(content);

    if (!IsElementValidAnchor(anchorCandidate, content))
    {
        // Ignore candidates that are collapsed or do not belong to the Content element and are not the Content itself. 
        return;
    }

    winrt::Rect anchorCandidateBounds = GetDescendantBounds(content, anchorCandidate);

    if (!SharedHelpers::DoRectsIntersect(viewportAnchorBounds, anchorCandidateBounds))
    {
        // Ignore candidates that do not intersect with the viewport in order to favor those that do.
        return;
    }

    // Using the distances from the viewport anchor point to the four corners of the anchor candidate.
    double anchorCandidateDistance{ 0.0 };

    if (!isnan(viewportAnchorPointHorizontalOffset))
    {
        anchorCandidateDistance += std::pow(viewportAnchorPointHorizontalOffset - anchorCandidateBounds.X, 2);
        anchorCandidateDistance += std::pow(viewportAnchorPointHorizontalOffset - (anchorCandidateBounds.X + anchorCandidateBounds.Width), 2);
    }

    if (!isnan(viewportAnchorPointVerticalOffset))
    {
        anchorCandidateDistance += std::pow(viewportAnchorPointVerticalOffset - anchorCandidateBounds.Y, 2);
        anchorCandidateDistance += std::pow(viewportAnchorPointVerticalOffset - (anchorCandidateBounds.Y + anchorCandidateBounds.Height), 2);
    }

    if (anchorCandidateDistance <= *bestAnchorCandidateDistance)
    {
        *bestAnchorCandidate = anchorCandidate;
        *bestAnchorCandidateBounds = anchorCandidateBounds;
        *bestAnchorCandidateDistance = anchorCandidateDistance;
    }
}

// Returns the bounds of a ScrollPresenter.Content descendant in respect to that content.
winrt::Rect ScrollPresenter::GetDescendantBounds(
    const winrt::UIElement& content,
    const winrt::UIElement& descendant)
{
    MUX_ASSERT(content);
    MUX_ASSERT(IsElementValidAnchor(descendant, content));

    const winrt::FrameworkElement descendantAsFE = descendant.as<winrt::FrameworkElement>();
    const winrt::Rect descendantRect{
        0.0f,
        0.0f,
        descendantAsFE ? static_cast<float>(descendantAsFE.ActualWidth()) : 0.0f,
        descendantAsFE ? static_cast<float>(descendantAsFE.ActualHeight()) : 0.0f
    };

    return GetDescendantBounds(content, descendant, descendantRect);
}

bool ScrollPresenter::IsElementValidAnchor(const winrt::UIElement& element, const winrt::UIElement& content)
{
    MUX_ASSERT(element);
    MUX_ASSERT(content);

    return element.Visibility() == winrt::Visibility::Visible && (element == content || SharedHelpers::IsAncestor(element, content));
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenter.h"
#include "DoubleUtil.h"

bool ScrollPresenter::IsHorizontallyScrollable()
{
    return m_contentOrientation != winrt::ScrollingContentOrientation::Vertical;
}

bool ScrollPresenter::IsVerticallyScrollable()
{
    return m_contentOrientation != winrt::ScrollingContentOrientation::Horizontal;
}

winrt::event_token ScrollPresenter::ViewportChanged(winrt::ViewportChangedEventHandler const& value)
{
    return m_viewportChanged.add(value);
}

void ScrollPresenter::ViewportChanged(winrt::event_token const& token)
{
    m_viewportChanged.remove(token);
}

winrt::event_token ScrollPresenter::PostArrange(winrt::PostArrangeEventHandler const& value)
{
    return m_postArrange.add(value);
}

void ScrollPresenter::PostArrange(winrt::event_token const& token)
{
    m_postArrange.remove(token);
}

winrt::event_token ScrollPresenter::ConfigurationChanged(winrt::ConfigurationChangedEventHandler const& value)
{
    return m_configurationChanged.add(value);
}

void ScrollPresenter::ConfigurationChanged(winrt::event_token const& token)
{
    m_configurationChanged.remove(token);
}

winrt::Rect ScrollPresenter::GetRelativeViewport(
    winrt::UIElement const& child)
{
    // The commented out code is expected to work but somehow the child.TransformToVisual(*this)
    // transform returns unexpected values shortly after a ScrollPresenter.Content layout offset change.
    // Bug 14999031 is tracking this issue. For now the m_contentLayoutOffsetX/Y, m_zoomedHorizontalOffset,
    // m_zoomedVerticalOffset usage below mitigates the problem.

    //const winrt::GeneralTransform transform = child.TransformToVisual(*this);
    const winrt::GeneralTransform transform = child.TransformToVisual(Content());
    const winrt::Point elementOffset = transform.TransformPoint(winrt::Point{});
    const float viewportWidth = static_cast<float>(m_viewportWidth / m_zoomFactor);
    const float viewportHeight = static_cast<float>(m_viewportHeight / m_zoomFactor);

    //winrt::Rect result = { -elementOffset.X / m_zoomFactor,
    //                       -elementOffset.Y / m_zoomFactor,
    //                       viewportWidth, viewportHeight };

    winrt::float2 minPosition{};

    ComputeMinMaxPositions(m_zoomFactor, &minPosition, nullptr);

    winrt::Rect result = { (minPosition.x - m_contentLayoutOffsetX + static_cast<float>(m_zoomedHorizontalOffset) - elementOffset.X) / m_zoomFactor,
        (minPosition.y - m_contentLayoutOffsetY + static_cast<float>(m_zoomedVerticalOffset) - elementOffset.Y) / m_zoomFactor,
        viewportWidth, viewportHeight };

    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this, child, TypeLogging::RectToString(result).c_str());

    return result;
}

winrt::UIElement ScrollPresenter::CurrentAnchor()
{
    return AnchorElement();
}

winrt::UIElement ScrollPresenter::AnchorElement()
{
    bool isAnchoringElementHorizontally = false;
    bool isAnchoringElementVertically = false;

    IsAnchoring(&isAnchoringElementHorizontally, &isAnchoringElementVertically);

    if (isAnchoringElementHorizontally || isAnchoringElementVertically)
    {
        EnsureAnchorElementSelection();
    }

    auto value = m_anchorElement.get();
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, value);

    return value;
}

void ScrollPresenter::RegisterAnchorCandidate(winrt::UIElement const& element)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, element);

    if (!element)
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }

    if (HorizontalAnchorRatio() != DoubleUtil::NaN || VerticalAnchorRatio() != DoubleUtil::NaN)
    {
#ifdef _DEBUG
        // We should not be registring the same element twice. Even through it is functionally ok,
        // we will end up spending more time during arrange than we must.
        // However checking if an element is already in the list every time a new element is registered is worse for perf.
        // So, I'm leaving an assert here to catch regression in our code but in release builds we run without the check.
        const winrt::UIElement anchorCandidate = element;
        const auto it = std::find_if(m_anchorCandidates.cbegin(), m_anchorCandidates.cend(), [&anchorCandidate](const tracker_ref<winrt::UIElement>& a) { return a.get() == anchorCandidate; });
        if (it != m_anchorCandidates.cend())
        {
            MUX_ASSERT(false);
        }
#endif // _DEBUG

        m_anchorCandidates.push_back(tracker_ref<winrt::UIElement>{ this, element });
        m_isAnchorElementDirty = true;
    }
}

void ScrollPresenter::UnregisterAnchorCandidate(winrt::UIElement const& element)
{
    SCROLLPRESENTER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, element);

    if (!element)
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }

    const winrt::UIElement anchorCandidate = element;
    const auto it = std::find_if(m_anchorCandidates.cbegin(), m_anchorCandidates.cend(), [&anchorCandidate](const tracker_ref<winrt::UIElement>& a) { return a.get() == anchorCandidate; });
    if (it != m_anchorCandidates.cend())
    {
        m_anchorCandidates.erase(it);
        m_isAnchorElementDirty = true;
    }
}

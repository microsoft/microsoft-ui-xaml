// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ScrollPresenterAutomationPeer.h"
#include "ResourceAccessor.h"
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>

double ScrollPresenterAutomationPeer::s_minimumPercent{ 0.0 };
double ScrollPresenterAutomationPeer::s_maximumPercent{ 100.0 };
double ScrollPresenterAutomationPeer::s_noScroll{ -1.0 };

#include "ScrollPresenterAutomationPeer.properties.cpp"

ScrollPresenterAutomationPeer::ScrollPresenterAutomationPeer(winrt::ScrollPresenter const& owner)
    : ReferenceTracker(owner)
{
    SCROLLPRESENTER_TRACE_VERBOSE(owner, TRACE_MSG_METH_PTR, METH_NAME, this, owner);
}

// IAutomationPeerOverrides implementation

winrt::AutomationControlType ScrollPresenterAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Pane;
}

winrt::IInspectable ScrollPresenterAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    return GetPatternCoreImpl(patternInterface);
}

// IScrollProvider implementation

// Request to scroll horizontally and vertically by the specified amount.
// The ability to call this method and simultaneously scroll horizontally
// and vertically provides simple panning support.
void ScrollPresenterAutomationPeer::Scroll(winrt::ScrollAmount const& horizontalAmount, winrt::ScrollAmount const& verticalAmount)
{
    SCROLLPRESENTER_TRACE_VERBOSE(Owner(), TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::ScrollAmountToString(horizontalAmount).c_str(), TypeLogging::ScrollAmountToString(verticalAmount).c_str());

    if (!IsEnabled())
    {
        throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
    }

    const bool scrollHorizontally = horizontalAmount != winrt::ScrollAmount::NoAmount;
    const bool scrollVertically = verticalAmount != winrt::ScrollAmount::NoAmount;

    const bool isHorizontallyScrollable = HorizontallyScrollable();
    const bool isVerticallyScrollable = VerticallyScrollable();

    if (!(scrollHorizontally && !isHorizontallyScrollable) && !(scrollVertically && !isVerticallyScrollable))
    {
        auto scrollPresenter = winrt::get_self<ScrollPresenter>(GetScrollPresenter());
        bool isInvalidOperation = false;

        switch (horizontalAmount)
        {
        case winrt::ScrollAmount::LargeDecrement:
            scrollPresenter->PageLeft();
            break;
        case winrt::ScrollAmount::LargeIncrement:
            scrollPresenter->PageRight();
            break;
        case winrt::ScrollAmount::SmallDecrement:
            scrollPresenter->LineLeft();
            break;
        case winrt::ScrollAmount::SmallIncrement:
            scrollPresenter->LineRight();
            break;
        case winrt::ScrollAmount::NoAmount:
            break;
        default:
            isInvalidOperation = true;
            break;
        }

        if (!isInvalidOperation)
        {
            switch (verticalAmount)
            {
            case winrt::ScrollAmount::LargeDecrement:
                scrollPresenter->PageUp();
                return;
            case winrt::ScrollAmount::SmallDecrement:
                scrollPresenter->LineUp();
                return;
            case winrt::ScrollAmount::SmallIncrement:
                scrollPresenter->LineDown();
                return;
            case winrt::ScrollAmount::LargeIncrement:
                scrollPresenter->PageDown();
                return;
            case winrt::ScrollAmount::NoAmount:
                return;
            }
        }
    }

    throw winrt::hresult_error(UIA_E_INVALIDOPERATION, L"Cannot perform the operation.");
}

// Request to set the current horizontal and Vertical scroll position by percent (0-100).
// Passing in the value of "-1", represented by the constant "NoScroll", will indicate that scrolling
// in that direction should be ignored.
// The ability to call this method and simultaneously scroll horizontally and vertically provides simple panning support.
void ScrollPresenterAutomationPeer::SetScrollPercent(double horizontalPercent, double verticalPercent)
{
    SCROLLPRESENTER_TRACE_VERBOSE(Owner(), TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalPercent, verticalPercent);

    if (!IsEnabled())
    {
        throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
    }

    const bool scrollHorizontally = horizontalPercent != s_noScroll;
    const bool scrollVertically = verticalPercent != s_noScroll;

    if (!scrollHorizontally && !scrollVertically)
    {
        return;
    }

    const bool isHorizontallyScrollable = HorizontallyScrollable();
    const bool isVerticallyScrollable = VerticallyScrollable();

    if ((scrollHorizontally && !isHorizontallyScrollable) || (scrollVertically && !isVerticallyScrollable))
    {
        throw winrt::hresult_error(UIA_E_INVALIDOPERATION, L"Cannot perform the operation.");
    }

    if ((scrollHorizontally && (horizontalPercent < s_minimumPercent || horizontalPercent > s_maximumPercent)) ||
        (scrollVertically && (verticalPercent < s_minimumPercent || verticalPercent > s_maximumPercent)))
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }

    auto scrollPresenter = winrt::get_self<ScrollPresenter>(GetScrollPresenter());

    if (scrollHorizontally && !scrollVertically)
    {
        const double maxOffset = scrollPresenter->ScrollableWidth();

        scrollPresenter->ScrollToHorizontalOffset(maxOffset * horizontalPercent / s_maximumPercent);
    }
    else if (scrollVertically && !scrollHorizontally)
    {
        const double maxOffset = scrollPresenter->ScrollableHeight();

        scrollPresenter->ScrollToVerticalOffset(maxOffset * verticalPercent / s_maximumPercent);
    }
    else
    {
        const double maxHorizontalOffset = scrollPresenter->ScrollableWidth();
        const double maxVerticalOffset = scrollPresenter->ScrollableHeight();

        scrollPresenter->ScrollToOffsets(
            maxHorizontalOffset * horizontalPercent / s_maximumPercent, maxVerticalOffset * verticalPercent / s_maximumPercent);
    }
}

double ScrollPresenterAutomationPeer::HorizontalScrollPercent()
{
    MUX_ASSERT(m_horizontalScrollPercent == get_HorizontalScrollPercentImpl());

    return m_horizontalScrollPercent;
}

double ScrollPresenterAutomationPeer::VerticalScrollPercent()
{
    MUX_ASSERT(m_verticalScrollPercent == get_VerticalScrollPercentImpl());

    return m_verticalScrollPercent;
}

// Returns the horizontal percentage of the entire extent that is currently viewed.
double ScrollPresenterAutomationPeer::HorizontalViewSize()
{
    MUX_ASSERT(m_horizontalViewSize == get_HorizontalViewSizeImpl());

    return m_horizontalViewSize;
}

// Returns the vertical percentage of the entire extent that is currently viewed.
double ScrollPresenterAutomationPeer::VerticalViewSize()
{
    MUX_ASSERT(m_verticalViewSize == get_VerticalViewSizeImpl());

    return m_verticalViewSize;
}

bool ScrollPresenterAutomationPeer::HorizontallyScrollable()
{
    MUX_ASSERT(m_horizontallyScrollable == get_HorizontallyScrollableImpl());
    
    return m_horizontallyScrollable;
}

bool ScrollPresenterAutomationPeer::VerticallyScrollable()
{
    MUX_ASSERT(m_verticallyScrollable == get_VerticallyScrollableImpl());

    return m_verticallyScrollable;
}

// Raise relevant Scroll Pattern events for UIAutomation clients.
void ScrollPresenterAutomationPeer::UpdateScrollPatternProperties()
{
    SCROLLPRESENTER_TRACE_VERBOSE(Owner(), TRACE_MSG_METH, METH_NAME, this);

    const double newHorizontalScrollPercent = get_HorizontalScrollPercentImpl();
    const double newVerticalScrollPercent = get_VerticalScrollPercentImpl();
    const double newHorizontalViewSize = get_HorizontalViewSizeImpl();
    const double newVerticalViewSize = get_VerticalViewSizeImpl();
    const bool newHorizontallyScrollable = get_HorizontallyScrollableImpl();
    const bool newVerticallyScrollable = get_VerticallyScrollableImpl();

    if (newHorizontallyScrollable != m_horizontallyScrollable)
    {
        const bool oldHorizontallyScrollable = m_horizontallyScrollable;
        m_horizontallyScrollable = newHorizontallyScrollable;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::HorizontallyScrollableProperty(),
            box_value(oldHorizontallyScrollable).as<winrt::IReference<bool>>(),
            box_value(newHorizontallyScrollable).as<winrt::IReference<bool>>());
    }

    if (newVerticallyScrollable != m_verticallyScrollable)
    {
        const bool oldVerticallyScrollable = m_verticallyScrollable;
        m_verticallyScrollable = newVerticallyScrollable;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::HorizontallyScrollableProperty(),
            box_value(oldVerticallyScrollable).as<winrt::IReference<bool>>(),
            box_value(newVerticallyScrollable).as<winrt::IReference<bool>>());
    }

    if (newHorizontalViewSize != m_horizontalViewSize)
    {
        const double oldHorizontalViewSize = m_horizontalViewSize;
        m_horizontalViewSize = newHorizontalViewSize;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::HorizontalViewSizeProperty(),
            box_value(oldHorizontalViewSize).as<winrt::IReference<double>>(),
            box_value(newHorizontalViewSize).as<winrt::IReference<double>>());
    }

    if (newVerticalViewSize != m_verticalViewSize)
    {
        const double oldVerticalViewSize = m_verticalViewSize;
        m_verticalViewSize = newVerticalViewSize;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::VerticalViewSizeProperty(),
            box_value(oldVerticalViewSize).as<winrt::IReference<double>>(),
            box_value(newVerticalViewSize).as<winrt::IReference<double>>());
    }

    if (newHorizontalScrollPercent != m_horizontalScrollPercent)
    {
        const double oldHorizontalScrollPercent = m_horizontalScrollPercent;
        m_horizontalScrollPercent = newHorizontalScrollPercent;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::HorizontalScrollPercentProperty(),
            box_value(oldHorizontalScrollPercent).as<winrt::IReference<double>>(),
            box_value(newHorizontalScrollPercent).as<winrt::IReference<double>>());
    }

    if (newVerticalScrollPercent != m_verticalScrollPercent)
    {
        const double oldVerticalScrollPercent = m_verticalScrollPercent;
        m_verticalScrollPercent = newVerticalScrollPercent;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::VerticalScrollPercentProperty(),
            box_value(oldVerticalScrollPercent).as<winrt::IReference<double>>(),
            box_value(newVerticalScrollPercent).as<winrt::IReference<double>>());
    }
}

double ScrollPresenterAutomationPeer::get_HorizontalScrollPercentImpl()
{
    com_ptr<ScrollPresenter> scrollPresenter = winrt::get_self<ScrollPresenter>(GetScrollPresenter())->get_strong();

    return GetScrollPercent(
        scrollPresenter->GetZoomedExtentWidth(),
        scrollPresenter->ViewportWidth(),
        GetScrollPresenter().HorizontalOffset());
}

double ScrollPresenterAutomationPeer::get_VerticalScrollPercentImpl()
{
    com_ptr<ScrollPresenter> scrollPresenter = winrt::get_self<ScrollPresenter>(GetScrollPresenter())->get_strong();

    return GetScrollPercent(
        scrollPresenter->GetZoomedExtentHeight(),
        scrollPresenter->ViewportHeight(),
        GetScrollPresenter().VerticalOffset());
}

// Returns the horizontal percentage of the entire extent that is currently viewed.
double ScrollPresenterAutomationPeer::get_HorizontalViewSizeImpl()
{
    com_ptr<ScrollPresenter> scrollPresenter = winrt::get_self<ScrollPresenter>(GetScrollPresenter())->get_strong();

    return GetViewPercent(
        scrollPresenter->GetZoomedExtentWidth(),
        scrollPresenter->ViewportWidth());
}

// Returns the vertical percentage of the entire extent that is currently viewed.
double ScrollPresenterAutomationPeer::get_VerticalViewSizeImpl()
{
    com_ptr<ScrollPresenter> scrollPresenter = winrt::get_self<ScrollPresenter>(GetScrollPresenter())->get_strong();

    return GetViewPercent(
        scrollPresenter->GetZoomedExtentHeight(),
        scrollPresenter->ViewportHeight());
}

bool ScrollPresenterAutomationPeer::get_HorizontallyScrollableImpl()
{
    com_ptr<ScrollPresenter> scrollPresenter = winrt::get_self<ScrollPresenter>(GetScrollPresenter())->get_strong();

    return scrollPresenter->ScrollableWidth() > 0.0;
}

bool ScrollPresenterAutomationPeer::get_VerticallyScrollableImpl()
{
    com_ptr<ScrollPresenter> scrollPresenter = winrt::get_self<ScrollPresenter>(GetScrollPresenter())->get_strong();

    return scrollPresenter->ScrollableHeight() > 0.0;
}

winrt::IInspectable ScrollPresenterAutomationPeer::GetPatternCoreImpl(winrt::PatternInterface patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Scroll)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

winrt::ScrollPresenter ScrollPresenterAutomationPeer::GetScrollPresenter()
{
    winrt::UIElement owner = Owner();
    return owner.as<winrt::ScrollPresenter>();
}

double ScrollPresenterAutomationPeer::GetViewPercent(double zoomedExtent, double viewport)
{
    MUX_ASSERT(zoomedExtent >= 0.0);
    MUX_ASSERT(viewport >= 0.0);

    if (zoomedExtent == 0.0)
    {
        return s_maximumPercent;
    }

    return std::min(s_maximumPercent, (viewport / zoomedExtent * s_maximumPercent));
}

double ScrollPresenterAutomationPeer::GetScrollPercent(double zoomedExtent, double viewport, double offset)
{
    MUX_ASSERT(zoomedExtent >= 0.0);
    MUX_ASSERT(viewport >= 0.0);

    if (viewport >= zoomedExtent)
    {
        return winrt::ScrollPatternIdentifiers::NoScroll();
    }

    double scrollPercent = offset / (zoomedExtent - viewport) * s_maximumPercent;

    scrollPercent = std::max(scrollPercent, s_minimumPercent);
    scrollPercent = std::min(scrollPercent, s_maximumPercent);

    return scrollPercent;
}

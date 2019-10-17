// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollingPresenterTypeLogging.h"
#include "ScrollingPresenterAutomationPeer.h"
#include "ResourceAccessor.h"
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>

double ScrollingPresenterAutomationPeer::s_minimumPercent{ 0.0 };
double ScrollingPresenterAutomationPeer::s_maximumPercent{ 100.0 };
double ScrollingPresenterAutomationPeer::s_noScroll{ -1.0 };

CppWinRTActivatableClassWithBasicFactory(ScrollingPresenterAutomationPeer);

ScrollingPresenterAutomationPeer::ScrollingPresenterAutomationPeer(winrt::ScrollingPresenter const& owner)
    : ReferenceTracker(owner)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(owner, TRACE_MSG_METH_PTR, METH_NAME, this, owner);
}

// IAutomationPeerOverrides implementation

winrt::AutomationControlType ScrollingPresenterAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Pane;
}

winrt::IInspectable ScrollingPresenterAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    return GetPatternCoreImpl(patternInterface);
}

// IScrollProvider implementation

// Request to scroll horizontally and vertically by the specified amount.
// The ability to call this method and simultaneously scroll horizontally
// and vertically provides simple panning support.
void ScrollingPresenterAutomationPeer::Scroll(winrt::ScrollAmount const& horizontalAmount, winrt::ScrollAmount const& verticalAmount)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(Owner(), TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::ScrollAmountToString(horizontalAmount).c_str(), TypeLogging::ScrollAmountToString(verticalAmount).c_str());

    if (!IsEnabled())
    {
        throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
    }

    bool scrollHorizontally = horizontalAmount != winrt::ScrollAmount::NoAmount;
    bool scrollVertically = verticalAmount != winrt::ScrollAmount::NoAmount;

    bool isHorizontallyScrollable = HorizontallyScrollable();
    bool isVerticallyScrollable = VerticallyScrollable();

    if (!(scrollHorizontally && !isHorizontallyScrollable) && !(scrollVertically && !isVerticallyScrollable))
    {
        auto scrollingPresenter = winrt::get_self<ScrollingPresenter>(GetScrollingPresenter());
        bool isInvalidOperation = false;

        switch (horizontalAmount)
        {
        case winrt::ScrollAmount::LargeDecrement:
            scrollingPresenter->PageLeft();
            break;
        case winrt::ScrollAmount::LargeIncrement:
            scrollingPresenter->PageRight();
            break;
        case winrt::ScrollAmount::SmallDecrement:
            scrollingPresenter->LineLeft();
            break;
        case winrt::ScrollAmount::SmallIncrement:
            scrollingPresenter->LineRight();
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
                scrollingPresenter->PageUp();
                return;
            case winrt::ScrollAmount::SmallDecrement:
                scrollingPresenter->LineUp();
                return;
            case winrt::ScrollAmount::SmallIncrement:
                scrollingPresenter->LineDown();
                return;
            case winrt::ScrollAmount::LargeIncrement:
                scrollingPresenter->PageDown();
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
void ScrollingPresenterAutomationPeer::SetScrollPercent(double horizontalPercent, double verticalPercent)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(Owner(), TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalPercent, verticalPercent);

    if (!IsEnabled())
    {
        throw winrt::hresult_error(UIA_E_ELEMENTNOTENABLED);
    }

    bool scrollHorizontally = horizontalPercent != s_noScroll;
    bool scrollVertically = verticalPercent != s_noScroll;

    if (!scrollHorizontally && !scrollVertically)
    {
        return;
    }

    bool isHorizontallyScrollable = HorizontallyScrollable();
    bool isVerticallyScrollable = VerticallyScrollable();

    if ((scrollHorizontally && !isHorizontallyScrollable) || (scrollVertically && !isVerticallyScrollable))
    {
        throw winrt::hresult_error(UIA_E_INVALIDOPERATION, L"Cannot perform the operation.");
    }

    if ((scrollHorizontally && (horizontalPercent < s_minimumPercent || horizontalPercent > s_maximumPercent)) ||
        (scrollVertically && (verticalPercent < s_minimumPercent || verticalPercent > s_maximumPercent)))
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }

    auto scrollingPresenter = winrt::get_self<ScrollingPresenter>(GetScrollingPresenter());

    if (scrollHorizontally && !scrollVertically)
    {
        double maxOffset = scrollingPresenter->ScrollableWidth();

        scrollingPresenter->ScrollToHorizontalOffset(maxOffset * horizontalPercent / s_maximumPercent);
    }
    else if (scrollVertically && !scrollHorizontally)
    {
        double maxOffset = scrollingPresenter->ScrollableHeight();

        scrollingPresenter->ScrollToVerticalOffset(maxOffset * verticalPercent / s_maximumPercent);
    }
    else
    {
        double maxHorizontalOffset = scrollingPresenter->ScrollableWidth();
        double maxVerticalOffset = scrollingPresenter->ScrollableHeight();

        scrollingPresenter->ScrollToOffsets(
            maxHorizontalOffset * horizontalPercent / s_maximumPercent, maxVerticalOffset * verticalPercent / s_maximumPercent);
    }
}

double ScrollingPresenterAutomationPeer::HorizontalScrollPercent()
{
    MUX_ASSERT(m_horizontalScrollPercent == get_HorizontalScrollPercentImpl());

    return m_horizontalScrollPercent;
}

double ScrollingPresenterAutomationPeer::VerticalScrollPercent()
{
    MUX_ASSERT(m_verticalScrollPercent == get_VerticalScrollPercentImpl());

    return m_verticalScrollPercent;
}

// Returns the horizontal percentage of the entire extent that is currently viewed.
double ScrollingPresenterAutomationPeer::HorizontalViewSize()
{
    MUX_ASSERT(m_horizontalViewSize == get_HorizontalViewSizeImpl());

    return m_horizontalViewSize;
}

// Returns the vertical percentage of the entire extent that is currently viewed.
double ScrollingPresenterAutomationPeer::VerticalViewSize()
{
    MUX_ASSERT(m_verticalViewSize == get_VerticalViewSizeImpl());

    return m_verticalViewSize;
}

bool ScrollingPresenterAutomationPeer::HorizontallyScrollable()
{
    MUX_ASSERT(m_horizontallyScrollable == get_HorizontallyScrollableImpl());
    
    return m_horizontallyScrollable;
}

bool ScrollingPresenterAutomationPeer::VerticallyScrollable()
{
    MUX_ASSERT(m_verticallyScrollable == get_VerticallyScrollableImpl());

    return m_verticallyScrollable;
}

// Raise relevant Scroll Pattern events for UIAutomation clients.
void ScrollingPresenterAutomationPeer::UpdateScrollPatternProperties()
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(Owner(), TRACE_MSG_METH, METH_NAME, this);

    double newHorizontalScrollPercent = get_HorizontalScrollPercentImpl();
    double newVerticalScrollPercent = get_VerticalScrollPercentImpl();
    double newHorizontalViewSize = get_HorizontalViewSizeImpl();
    double newVerticalViewSize = get_VerticalViewSizeImpl();
    bool newHorizontallyScrollable = get_HorizontallyScrollableImpl();
    bool newVerticallyScrollable = get_VerticallyScrollableImpl();

    if (newHorizontallyScrollable != m_horizontallyScrollable)
    {
        bool oldHorizontallyScrollable = m_horizontallyScrollable;
        m_horizontallyScrollable = newHorizontallyScrollable;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::HorizontallyScrollableProperty(),
            box_value(oldHorizontallyScrollable).as<winrt::IReference<bool>>(),
            box_value(newHorizontallyScrollable).as<winrt::IReference<bool>>());
    }

    if (newVerticallyScrollable != m_verticallyScrollable)
    {
        bool oldVerticallyScrollable = m_verticallyScrollable;
        m_verticallyScrollable = newVerticallyScrollable;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::HorizontallyScrollableProperty(),
            box_value(oldVerticallyScrollable).as<winrt::IReference<bool>>(),
            box_value(newVerticallyScrollable).as<winrt::IReference<bool>>());
    }

    if (newHorizontalViewSize != m_horizontalViewSize)
    {
        double oldHorizontalViewSize = m_horizontalViewSize;
        m_horizontalViewSize = newHorizontalViewSize;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::HorizontalViewSizeProperty(),
            box_value(oldHorizontalViewSize).as<winrt::IReference<double>>(),
            box_value(newHorizontalViewSize).as<winrt::IReference<double>>());
    }

    if (newVerticalViewSize != m_verticalViewSize)
    {
        double oldVerticalViewSize = m_verticalViewSize;
        m_verticalViewSize = newVerticalViewSize;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::VerticalViewSizeProperty(),
            box_value(oldVerticalViewSize).as<winrt::IReference<double>>(),
            box_value(newVerticalViewSize).as<winrt::IReference<double>>());
    }

    if (newHorizontalScrollPercent != m_horizontalScrollPercent)
    {
        double oldHorizontalScrollPercent = m_horizontalScrollPercent;
        m_horizontalScrollPercent = newHorizontalScrollPercent;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::HorizontalScrollPercentProperty(),
            box_value(oldHorizontalScrollPercent).as<winrt::IReference<double>>(),
            box_value(newHorizontalScrollPercent).as<winrt::IReference<double>>());
    }

    if (newVerticalScrollPercent != m_verticalScrollPercent)
    {
        double oldVerticalScrollPercent = m_verticalScrollPercent;
        m_verticalScrollPercent = newVerticalScrollPercent;
        RaisePropertyChangedEvent(
            winrt::ScrollPatternIdentifiers::VerticalScrollPercentProperty(),
            box_value(oldVerticalScrollPercent).as<winrt::IReference<double>>(),
            box_value(newVerticalScrollPercent).as<winrt::IReference<double>>());
    }
}

double ScrollingPresenterAutomationPeer::get_HorizontalScrollPercentImpl()
{
    com_ptr<ScrollingPresenter> scrollingPresenter = winrt::get_self<ScrollingPresenter>(GetScrollingPresenter())->get_strong();

    return GetScrollPercent(
        scrollingPresenter->GetZoomedExtentWidth(),
        scrollingPresenter->ViewportWidth(),
        GetScrollingPresenter().HorizontalOffset());
}

double ScrollingPresenterAutomationPeer::get_VerticalScrollPercentImpl()
{
    com_ptr<ScrollingPresenter> scrollingPresenter = winrt::get_self<ScrollingPresenter>(GetScrollingPresenter())->get_strong();

    return GetScrollPercent(
        scrollingPresenter->GetZoomedExtentHeight(),
        scrollingPresenter->ViewportHeight(),
        GetScrollingPresenter().VerticalOffset());
}

// Returns the horizontal percentage of the entire extent that is currently viewed.
double ScrollingPresenterAutomationPeer::get_HorizontalViewSizeImpl()
{
    com_ptr<ScrollingPresenter> scrollingPresenter = winrt::get_self<ScrollingPresenter>(GetScrollingPresenter())->get_strong();

    return GetViewPercent(
        scrollingPresenter->GetZoomedExtentWidth(),
        scrollingPresenter->ViewportWidth());
}

// Returns the vertical percentage of the entire extent that is currently viewed.
double ScrollingPresenterAutomationPeer::get_VerticalViewSizeImpl()
{
    com_ptr<ScrollingPresenter> scrollingPresenter = winrt::get_self<ScrollingPresenter>(GetScrollingPresenter())->get_strong();

    return GetViewPercent(
        scrollingPresenter->GetZoomedExtentHeight(),
        scrollingPresenter->ViewportHeight());
}

bool ScrollingPresenterAutomationPeer::get_HorizontallyScrollableImpl()
{
    com_ptr<ScrollingPresenter> scrollingPresenter = winrt::get_self<ScrollingPresenter>(GetScrollingPresenter())->get_strong();

    return scrollingPresenter->ScrollableWidth() > 0.0;
}

bool ScrollingPresenterAutomationPeer::get_VerticallyScrollableImpl()
{
    com_ptr<ScrollingPresenter> scrollingPresenter = winrt::get_self<ScrollingPresenter>(GetScrollingPresenter())->get_strong();

    return scrollingPresenter->ScrollableHeight() > 0.0;
}

winrt::IInspectable ScrollingPresenterAutomationPeer::GetPatternCoreImpl(winrt::PatternInterface patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Scroll)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

winrt::ScrollingPresenter ScrollingPresenterAutomationPeer::GetScrollingPresenter()
{
    winrt::UIElement owner = Owner();
    return owner.as<winrt::ScrollingPresenter>();
}

double ScrollingPresenterAutomationPeer::GetViewPercent(double zoomedExtent, double viewport)
{
    MUX_ASSERT(zoomedExtent >= 0.0);
    MUX_ASSERT(viewport >= 0.0);

    if (zoomedExtent == 0.0)
    {
        return s_maximumPercent;
    }

    return std::min(s_maximumPercent, (viewport / zoomedExtent * s_maximumPercent));
}

double ScrollingPresenterAutomationPeer::GetScrollPercent(double zoomedExtent, double viewport, double offset)
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

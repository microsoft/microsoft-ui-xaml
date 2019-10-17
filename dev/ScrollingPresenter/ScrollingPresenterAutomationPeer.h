// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"

#include "ScrollerAutomationPeer.g.h"

class ScrollerAutomationPeer :
    public ReferenceTracker<ScrollerAutomationPeer, winrt::implementation::ScrollerAutomationPeerT, winrt::IScrollProvider>
{
public:
    ScrollerAutomationPeer(winrt::Scroller const& owner);

    ~ScrollerAutomationPeer()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IAutomationPeerOverrides methods 
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);

    // IScrollProvider properties and methods
    void Scroll(winrt::ScrollAmount const& horizontalAmount, winrt::ScrollAmount const& verticalAmount);
    void SetScrollPercent(double horizontalPercent, double verticalPercent);
    double HorizontalScrollPercent();
    double VerticalScrollPercent();
    double HorizontalViewSize();
    double VerticalViewSize();
    bool HorizontallyScrollable();
    bool VerticallyScrollable();

    void UpdateScrollPatternProperties();

private:
    double get_HorizontalScrollPercentImpl();
    double get_VerticalScrollPercentImpl();
    double get_HorizontalViewSizeImpl();
    double get_VerticalViewSizeImpl();
    bool get_HorizontallyScrollableImpl();
    bool get_VerticallyScrollableImpl();

    winrt::IInspectable GetPatternCoreImpl(winrt::PatternInterface patternInterface);
    winrt::Scroller GetScroller();
    
    static double GetViewPercent(double zoomedExtent, double viewport);
    static double GetScrollPercent(double zoomedExtent, double viewport, double offset);

private:
    double m_horizontalScrollPercent{ winrt::ScrollPatternIdentifiers::NoScroll() };
    double m_verticalScrollPercent{ winrt::ScrollPatternIdentifiers::NoScroll() };
    double m_horizontalViewSize{ s_maximumPercent };
    double m_verticalViewSize{ s_maximumPercent };
    bool m_horizontallyScrollable{ false };
    bool m_verticallyScrollable{ false };

    static double s_minimumPercent;
    static double s_maximumPercent;
    static double s_noScroll;
};

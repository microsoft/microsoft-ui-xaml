﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"

class ScrollView;

using namespace std;

class ScrollBarController:
    public winrt::implements<ScrollBarController, winrt::IScrollController>
{
public:
    ScrollBarController();
    ~ScrollBarController();

    void SetScrollBar(const winrt::ScrollBar& scrollBar);

#pragma region IScrollController
    winrt::IScrollControllerPanningInfo PanningInfo();

    bool CanScroll();
    bool IsScrollingWithMouse();

    void SetIsScrollable(
        bool isScrollable);

    void SetValues(
        double minOffset,
        double maxOffset,
        double offset,
        double viewportLength);

    winrt::CompositionAnimation GetScrollAnimation(
        int correlationId,
        winrt::float2 const& startPosition,
        winrt::float2 const& endPosition,
        winrt::CompositionAnimation const& defaultAnimation);

    void NotifyRequestedScrollCompleted(
        int correlationId);

    winrt::event_token ScrollToRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollToRequestedEventArgs> const& value);
    void ScrollToRequested(winrt::event_token const& token);

    winrt::event_token ScrollByRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollByRequestedEventArgs> const& value);
    void ScrollByRequested(winrt::event_token const& token);

    winrt::event_token AddScrollVelocityRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerAddScrollVelocityRequestedEventArgs> const& value);
    void AddScrollVelocityRequested(winrt::event_token const& token);

    winrt::event_token CanScrollChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value);
    void CanScrollChanged(winrt::event_token const& token);

    winrt::event_token IsScrollingWithMouseChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value);
    void IsScrollingWithMouseChanged(winrt::event_token const& token);
#pragma endregion

private:
    void UpdateCanScroll();

    void HookScrollBarEvent();
    void UnhookScrollBarEvent();
    void HookScrollBarPropertyChanged();
    void UnhookScrollBarPropertyChanged();

    void OnScrollBarPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);
    void OnScroll(
        const winrt::IInspectable& sender,
        const winrt::ScrollEventArgs& args);

    bool RaiseScrollToRequested(
        double offset);
    bool RaiseScrollByRequested(
        double offsetChange);
    bool RaiseAddScrollVelocityRequested(
        double offsetChange);
    void RaiseCanScrollChanged();
    void RaiseIsScrollingWithMouseChanged();

private:
    // Private constants

    // Default amount to scroll when hitting the SmallIncrement/SmallDecrement buttons: 1/8 of the viewport size.
    // This amount can be overridden by setting the ScrollBar.SmallChange property to something else than double.NaN.
    static constexpr double s_defaultViewportToSmallChangeRatio{ 8.0 };

    // Inertia decay rate for SmallChange / LargeChange animated Value changes.
    static constexpr float s_inertiaDecayRate = 0.9995f;

    // Additional velocity required with decay s_inertiaDecayRate to move Position by one pixel.
    static constexpr double s_velocityNeededPerPixel{ 7.600855902349023 };

    // Additional velocity at Minimum and Maximum positions to ensure hitting the extreme Value.
    static constexpr double s_minMaxEpsilon{ 0.001 };

    winrt::ScrollBar m_scrollBar;
    int32_t m_lastOffsetChangeCorrelationIdForScrollTo{ -1 };
    int32_t m_lastOffsetChangeCorrelationIdForScrollBy{ -1 };
    int32_t m_lastOffsetChangeCorrelationIdForAddScrollVelocity{ -1 };
    int m_operationsCount{ 0 };
    double m_lastScrollBarValue{ 0.0 };
    double m_lastOffset{ 0.0 };
    bool m_canScroll{ false };
    bool m_isScrollingWithMouse{ false };
    bool m_isScrollable{ false };

    // Event Sources
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollToRequestedEventArgs>> m_scrollToRequested { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollByRequestedEventArgs>> m_scrollByRequested { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerAddScrollVelocityRequestedEventArgs>> m_addScrollVelocityRequested { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable>> m_canScrollChanged { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable>> m_isScrollingWithMouseChanged { };

    // Event Tokens
    winrt::event_token m_scrollBarScrollToken{};
    winrt::event_token m_visibilityChangedToken{};
    winrt::event_token m_scrollBarIsEnabledChangedToken{};
#ifdef DBG
    // For testing purposes only
    winrt::event_token m_scrollBarIndicatorModeChangedToken{};
    winrt::event_token m_scrollBarVisibilityChangedToken{};
#endif //DBG
};


// Copyright (c) Microsoft Corporation. All rights reserved.
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
    bool AreScrollControllerInteractionsAllowed();

    bool AreScrollerInteractionsAllowed();

    bool IsInteracting();

    bool IsInteractionVisualRailEnabled();

    winrt::Visual InteractionVisual();

    winrt::Orientation InteractionVisualScrollOrientation();

    void SetExpressionAnimationSources(
        winrt::CompositionPropertySet const& propertySet,
        winrt::hstring const& minOffsetPropertyName,
        winrt::hstring const& maxOffsetPropertyName,
        winrt::hstring const& offsetPropertyName,
        winrt::hstring const& multiplierPropertyName);

    void SetScrollMode(
        winrt::ScrollMode const& scrollMode);

    void SetValues(
        double minOffset,
        double maxOffset,
        double offset,
        double viewport);

    winrt::CompositionAnimation GetScrollAnimation(
        winrt::ScrollInfo info,
        winrt::float2 const& currentPosition,
        winrt::CompositionAnimation const& defaultAnimation);

    void OnScrollCompleted(
        winrt::ScrollInfo info);

    winrt::event_token ScrollToRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollToRequestedEventArgs> const& value);
    void ScrollToRequested(winrt::event_token const& token);

    winrt::event_token ScrollByRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollByRequestedEventArgs> const& value);
    void ScrollByRequested(winrt::event_token const& token);

    winrt::event_token ScrollFromRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollFromRequestedEventArgs> const& value);
    void ScrollFromRequested(winrt::event_token const& token);

    winrt::event_token InteractionRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerInteractionRequestedEventArgs> const& value);
    void InteractionRequested(winrt::event_token const& token);

    winrt::event_token InteractionInfoChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value);
    void InteractionInfoChanged(winrt::event_token const& token);
#pragma endregion

private:
    void UpdateAreScrollControllerInteractionsAllowed();

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
    bool RaiseScrollFromRequested(
        double offsetChange);
    void RaiseInteractionInfoChanged();

private:
    // Private constants

    // Default amount to scroll when hitting the SmallIncrement/SmallDecrement buttons: 1/8 of the viewport size.
    // This amount can be overridden by setting the ScrollBar.SmallChange property to something else than double.NaN.
    static constexpr double s_defaultViewportToSmallChangeRatio{ 8.0 };

    // Minimum initial velocity required by InteractionTracker.TryUpdatePositionWithAdditionalVelocity to affect the Position.
    static constexpr double s_minimumVelocity{ 30.0 };

    // Inertia decay rate for SmallChange / LargeChange animated Value changes.
    static constexpr float s_inertiaDecayRate = 0.9995f;

    // Additional velocity required with decay s_inertiaDecayRate to move Position by one pixel.
    static constexpr double s_velocityNeededPerPixel{ 7.600855902349023 };

    // Additional velocity at Minimum and Maximum positions to ensure hitting the extreme Value.
    static constexpr double s_minMaxEpsilon{ 0.001 };

    winrt::ScrollBar m_scrollBar;
    winrt::ScrollMode m_scrollMode{ winrt::ScrollMode::Disabled };
    int32_t m_lastOffsetChangeIdForScrollTo{ -1 };
    int32_t m_lastOffsetChangeIdForScrollBy{ -1 };
    int32_t m_lastOffsetChangeIdForScrollFrom{ -1 };
    int m_operationsCount{ 0 };
    double m_lastScrollBarValue{ 0.0 };
    double m_lastOffset{ 0.0 };
    bool m_areScrollerInteractionsAllowed{ true };
    bool m_isInteracting{ false };
    bool m_areScrollControllerInteractionsAllowed{ false };

    // Event Sources
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollToRequestedEventArgs>> m_scrollToRequested { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollByRequestedEventArgs>> m_scrollByRequested { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollFromRequestedEventArgs>> m_scrollFromRequested { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable>> m_interactionInfoChanged{ };

    // Event Tokens
    winrt::event_token m_scrollBarScrollToken{};
    winrt::event_token m_visibilityChangedToken{};
    winrt::event_token m_scrollBarIsEnabledChangedToken{};
#ifdef _DEBUG
    // For testing purposes only
    winrt::event_token m_scrollBarIndicatorModeChangedToken{};
    winrt::event_token m_scrollBarVisibilityChangedToken{};
#endif //_DEBUG
};


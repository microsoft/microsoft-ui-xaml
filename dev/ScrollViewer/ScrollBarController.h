// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"

class ScrollViewer;

using namespace std;

class ScrollBarController final:
    public winrt::implements<ScrollBarController, winrt::IScrollController>
{
public:
    ScrollBarController(ScrollViewer* owner);
    ~ScrollBarController();

    void SetScrollBar(const winrt::ScrollBar& scrollBar);

#pragma region IScrollController
    bool AreInteractionsEnabled();

    bool AreScrollerInteractionsAllowed();

    bool IsInteracting();

    bool IsInteractionVisualRailEnabled();

    winrt::Visual InteractionVisual();

    winrt::Orientation InteractionVisualScrollOrientation();

    void AllowInteractions(bool allowInteractions);

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
        INT32 offsetChangeId,
        winrt::float2 const& currentPosition,
        winrt::CompositionAnimation const& defaultAnimation);

    void OnScrollCompleted(
        INT32 offsetChangeId,
        winrt::ScrollerViewChangeResult const& result);

    winrt::event_token OffsetChangeRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerOffsetChangeRequestedEventArgs> const& value);
    void OffsetChangeRequested(winrt::event_token const& token);

    winrt::event_token OffsetChangeWithAdditionalVelocityRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs> const& value);
    void OffsetChangeWithAdditionalVelocityRequested(winrt::event_token const& token);

    winrt::event_token InteractionRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerInteractionRequestedEventArgs> const& value);
    void InteractionRequested(winrt::event_token const& token);

    winrt::event_token InteractionInfoChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value);
    void InteractionInfoChanged(winrt::event_token const& token);
#pragma endregion

private:
    void UpdateAreInteractionsEnabled();

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

    bool RaiseOffsetChangeRequested(
        double offset);
    bool RaiseOffsetChangeWithAdditionalVelocityRequested(
        double offsetChange);
    void RaiseInteractionInfoChanged();

private:
    // Private constants

    // Default amount to scroll when hitting the SmallIncrement/SmallDecrement buttons: 1/8 of the viewport size.
    // This amount can be overridden by setting the ScrollBar2.SmallChange property to something else than double.NaN.
    static constexpr double s_defaultViewportToSmallChangeRatio{ 8.0 };

    // Minimum initial velocity required by InteractionTracker.TryUpdatePositionWithAdditionalVelocity to affect the Position.
    static constexpr double s_minimumVelocity{ 30.0 };

    // Inertia decay rate for SmallChange / LargeChange animated Value changes.
    static constexpr float s_inertiaDecayRate = 0.9995f;

    // Additional velocity required with decay s_inertiaDecayRate to move Position by one pixel.
    static constexpr double s_velocityNeededPerPixel{ 7.600855902349023 };

    // Additional velocity at Minimum and Maximum positions to ensure hitting the extreme Value.
    static constexpr double s_minMaxEpsilon{ 0.001 };

    tracker_ref<winrt::ScrollBar> m_scrollBar;
    winrt::ScrollMode m_scrollMode{ winrt::ScrollMode::Disabled };
    int32_t m_lastViewChangeIdForOffsetChange{ -1 };
    int32_t m_lastViewChangeIdForOffsetChangeWithAdditionalVelocity{ -1 };
    int m_operationsCount{ 0 };
    double m_lastScrollBarValue{ 0.0 };
    double m_lastOffset{ 0.0 };
    bool m_areScrollerInteractionsAllowed{ true };
    bool m_isInteracting{ false };
    bool m_areInteractionsEnabled{ false };
    bool m_areInteractionsAllowed{ false };

    // Event Sources
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerOffsetChangeRequestedEventArgs>> m_offsetChangeRequested{ };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs>> m_offsetChangeWithAdditionalVelocityRequested{ };
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


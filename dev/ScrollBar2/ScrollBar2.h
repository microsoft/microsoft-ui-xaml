// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollBar2Trace.h"

#include "ScrollBar2.g.h"
#include "ScrollBar2.properties.h"

// Note about UIA: Automation clients will discover and use the inner ScrollBar's appropriate ScrollBarAutomationPeer. 
// Thus this FrameworkElement does not need to expose a separate AutomationPeer type or instance.

class ScrollBar2 :
    public ReferenceTracker<ScrollBar2, DeriveFromPanelHelper_base, winrt::ScrollBar2, winrt::IScrollController>,
    public ScrollBar2Properties
{
public:
    ScrollBar2();
    ~ScrollBar2();

    // Property names
    static constexpr std::wstring_view s_SmallChangePropertyName{ L"SmallChange"sv };
    static constexpr std::wstring_view s_LargeChangePropertyName{ L"LargeChange"sv };
    static constexpr std::wstring_view s_IndicatorModePropertyName{ L"IndicatorMode"sv };
    static constexpr std::wstring_view s_IsEnabledPropertyName{ L"IsEnabled"sv };
    static constexpr std::wstring_view s_OrientationPropertyName{ L"Orientation"sv };
    static constexpr std::wstring_view s_ScrollBarStylePropertyName{ L"ScrollBarStyle"sv };

    // Properties' default values
    static constexpr double s_defaultMinOffset{ 0.0 };
    static constexpr double s_defaultMaxOffset{ 100.0 };
    static constexpr double s_defaultOffset{ 0.0 };
    static constexpr double s_defaultViewport{ 10.0 };
    static constexpr double s_defaultSmallChange{ std::numeric_limits<double>::quiet_NaN() };
    static constexpr double s_defaultLargeChange{ std::numeric_limits<double>::quiet_NaN() };
    static constexpr bool s_defaultIsEnabled{ true };
    static constexpr winrt::Orientation s_defaultOrientation{ winrt::Orientation::Vertical };
    static constexpr winrt::ScrollingIndicatorMode s_defaultIndicatorMode{ winrt::ScrollingIndicatorMode::None };

#pragma region IFrameworkElementOverridesHelper
    // IFrameworkElementOverrides (unoverridden methods provided by FrameworkElementOverridesHelper)
    winrt::Size MeasureOverride(winrt::Size const& availableSize); // not actually final for 'derived' classes
    winrt::Size ArrangeOverride(winrt::Size const& finalSize); // not actually final for 'derived' classes
#pragma endregion

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

    // Invoked when a dependency property of this ScrollBar2 has changed.
    void OnPropertyChanged(
        const winrt::DependencyPropertyChangedEventArgs& args);

private:
#ifdef _DEBUG
    static winrt::hstring DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty);
#endif

private:
    void CreateAndInitializeScrollBar();
    void HookPropertyChanged();
    void UnhookPropertyChanged();
    void HookScrollBarEvent();
    void UnhookScrollBarEvent();
#ifdef _DEBUG
    // For testing purposes only
    void HookScrollBarPropertyChanged();
    void UnhookScrollBarPropertyChanged();
    void OnScrollBarPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);
#endif //_DEBUG
    void OnScrollBar2PropertyChanged(
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
    tracker_ref<winrt::ScrollBar>  m_scrollBar{ this };
    winrt::ScrollMode m_scrollMode{ winrt::ScrollMode::Disabled };
    int32_t m_lastViewChangeIdForOffsetChange{ -1 };
    int32_t m_lastViewChangeIdForOffsetChangeWithAdditionalVelocity{ -1 };
    int m_operationsCount{ 0 };
    double m_lastScrollBarValue{ 0.0 };
    double m_lastOffset{ 0.0 };
    bool m_areScrollerInteractionsAllowed{ true };
    bool m_isInteracting{ false };
    bool m_areInteractionsAllowed{ false };

    // Event Sources
    event_source<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerOffsetChangeRequestedEventArgs>> m_offsetChangeRequested{ this };
    event_source<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs>> m_offsetChangeWithAdditionalVelocityRequested{ this };
    event_source<winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable>> m_interactionInfoChanged{ this };

    // Event Tokens
    winrt::event_token m_scrollBarScrollToken{};
    winrt::event_token m_visibilityChangedToken{};
#ifdef _DEBUG
    // For testing purposes only
    winrt::event_token m_scrollBarIndicatorModeChangedToken{};
    winrt::event_token m_scrollBarVisibilityChangedToken{};
#endif //_DEBUG

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
};

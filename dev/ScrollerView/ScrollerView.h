// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollerViewTrace.h"

#include "ScrollerView.g.h"
#include "ScrollerView.properties.h"

class ScrollerView :
    public ReferenceTracker<ScrollerView, winrt::implementation::ScrollerViewT>,
    public ScrollerViewProperties
{
public:
    ScrollerView();
    ~ScrollerView();

    // Properties' default values.
    static const winrt::ScrollerViewScrollControllerVisibility s_defaultHorizontalScrollControllerVisibility{ winrt::ScrollerViewScrollControllerVisibility::Auto };
    static const winrt::ScrollerViewScrollControllerVisibility s_defaultVerticalScrollControllerVisibility{ winrt::ScrollerViewScrollControllerVisibility::Auto };
    static const winrt::ScrollerChainingMode s_defaultHorizontalScrollChainingMode{ winrt::ScrollerChainingMode::Auto };
    static const winrt::ScrollerChainingMode s_defaultVerticalScrollChainingMode{ winrt::ScrollerChainingMode::Auto };
    static const winrt::ScrollerRailingMode s_defaultHorizontalScrollRailingMode{ winrt::ScrollerRailingMode::Enabled };
    static const winrt::ScrollerRailingMode s_defaultVerticalScrollRailingMode{ winrt::ScrollerRailingMode::Enabled };
    static const winrt::ScrollerScrollMode s_defaultHorizontalScrollMode{ winrt::ScrollerScrollMode::Auto };
    static const winrt::ScrollerScrollMode s_defaultVerticalScrollMode{ winrt::ScrollerScrollMode::Auto };
    static const winrt::ScrollerScrollMode s_defaultComputedHorizontalScrollMode{ winrt::ScrollerScrollMode::Disabled };
    static const winrt::ScrollerScrollMode s_defaultComputedVerticalScrollMode{ winrt::ScrollerScrollMode::Disabled };
    static const winrt::ScrollerChainingMode s_defaultZoomChainingMode{ winrt::ScrollerChainingMode::Auto };
    static const winrt::ScrollerZoomMode s_defaultZoomMode{ winrt::ScrollerZoomMode::Disabled };
    static const winrt::ScrollerInputKind s_defaultInputKind{ winrt::ScrollerInputKind::All };
    static constexpr bool s_defaultIsChildAvailableWidthConstrained{ true };
    static constexpr bool s_defaultIsChildAvailableHeightConstrained{ false };
    static constexpr double s_defaultMinZoomFactor{ 0.1 };
    static constexpr double s_defaultMaxZoomFactor{ 10.0 };
    static constexpr bool s_defaultAnchorAtExtent{ true };
    static constexpr double s_defaultAnchorRatio{ 0.0 };

#pragma region IScrollerView

    winrt::CompositionPropertySet ExpressionAnimationSources();

    double HorizontalOffset();

    double VerticalOffset();

    float ZoomFactor();

    double ExtentWidth();

    double ExtentHeight();

    winrt::ScrollerState State();

    winrt::ScrollerInputKind InputKind();
    void InputKind(winrt::ScrollerInputKind const& value);

    int32_t ChangeOffsets(winrt::ScrollerChangeOffsetsOptions const& options);
    int32_t ChangeOffsetsWithAdditionalVelocity(winrt::ScrollerChangeOffsetsWithAdditionalVelocityOptions const& options);
    int32_t ChangeZoomFactor(winrt::ScrollerChangeZoomFactorOptions const& options);
    int32_t ChangeZoomFactorWithAdditionalVelocity(winrt::ScrollerChangeZoomFactorWithAdditionalVelocityOptions const& options);
#pragma endregion

    // Invoked by ScrollerViewTestHooks
    winrt::Scroller GetScrollerPart();

    static void ValidateAnchorRatio(double value);
    static void ValidateZoomFactoryBoundary(double value);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

#ifdef _DEBUG
    static winrt::hstring DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty);
#endif

public:
#pragma region IFrameworkElementOverrides
    // unoverridden methods provided by FrameworkElementOverridesHelper
    void OnApplyTemplate(); // not actually final for 'derived' classes
#pragma endregion

#pragma region IControlOverrides
    void OnGotFocus(winrt::RoutedEventArgs const& args);
    void OnKeyDown(winrt::KeyRoutedEventArgs const& args);
#pragma endregion

private:
    void OnScrollerViewGettingFocus(
        const winrt::IInspectable& /*sender*/,
        const winrt::GettingFocusEventArgs& args);
    void OnScrollerViewIsEnabledChanged(
        const winrt::IInspectable& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);
    void OnScrollerViewUnloaded(
        const winrt::IInspectable& sender,
        const winrt::RoutedEventArgs& args);
    void OnScrollerViewPointerEntered(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollerViewPointerMoved(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollerViewPointerExited(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollerViewPointerPressed(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollerViewPointerReleased(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollerViewPointerCanceled(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);

    void OnHorizontalScrollControllerPointerEntered(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnHorizontalScrollControllerPointerExited(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnVerticalScrollControllerPointerEntered(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnVerticalScrollControllerPointerExited(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnNoIndicatorStateStoryboardCompleted(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnIndicatorStateStoryboardCompleted(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnScrollControllerInteractionInfoChanged(
        const winrt::IScrollController& sender,
        const winrt::IInspectable& args);
    void OnHideIndicatorsTimerTick(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);

    // Internal event handlers
    void OnScrollerExtentChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnScrollerStateChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnScrollerChangingOffsets(
        const winrt::IInspectable& sender,
        const winrt::ScrollerChangingOffsetsEventArgs& args);
    void OnScrollerChangingZoomFactor(
        const winrt::IInspectable& sender,
        const winrt::ScrollerChangingZoomFactorEventArgs& args);
    void OnScrollerViewChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnScrollerPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);
    void OnScrollerViewChangeCompleted(
        const winrt::IInspectable& sender,
        const winrt::ScrollerViewChangeCompletedEventArgs& args);
    void OnScrollerBringingIntoView(
        const winrt::IInspectable& sender,
        const winrt::ScrollerBringingIntoViewEventArgs& args);
    void OnScrollerAnchorRequested(
        const winrt::IInspectable& sender,
        const winrt::ScrollerAnchorRequestedEventArgs& args);

    void StopHideIndicatorsTimer(bool isForDestructor);

    void HookScrollerViewEvents();
    void UnhookScrollerViewEvents();
    void HookScrollerEvents();
    void UnhookScrollerEvents(bool isForDestructor);
    void HookHorizontalScrollControllerEvents();
    void UnhookHorizontalScrollControllerEvents();
    void HookVerticalScrollControllerEvents();
    void UnhookVerticalScrollControllerEvents();

    void UpdateScroller(const winrt::Scroller& scroller);
    void UpdateHorizontalScrollController(const winrt::IScrollController& horizontalScrollController);
    void UpdateVerticalScrollController(const winrt::IScrollController& verticalScrollController);
    void UpdateScrollControllersSeparator(const winrt::IUIElement& scrollControllersSeparator);
    void UpdateScrollerHorizontalScrollController(const winrt::IScrollController& horizontalScrollController);
    void UpdateScrollerVerticalScrollController(const winrt::IScrollController& verticalScrollController);
    void UpdateScrollControllersVisibility(bool horizontalChange, bool verticalChange);

    bool IsLoaded();

    bool AreAllScrollControllersCollapsed();
    bool AreBothScrollControllersVisible();
    void ShowIndicators();
    void HideIndicators(bool useTransitions);
    void HideIndicatorsAfterDelay();
    
    void HandleKeyDownForStandardScroll(winrt::KeyRoutedEventArgs);
    void HandleKeyDownForXYNavigation(winrt::KeyRoutedEventArgs);

    bool DoScrollForKey(winrt::VirtualKey key, double scrollProportion);
    void DoScroll(double offsetAmount, winrt::Orientation orientation);

    bool CanScrollDown();
    bool CanScrollUp();
    bool CanScrollRight();
    bool CanScrollLeft();
    bool CanScrollVerticallyInDirection(bool inPositiveDirection);
    bool CanScrollHorizontallyInDirection(bool inPositiveDirection);
    bool CanScrollInDirection(winrt::FocusNavigationDirection drection);

    winrt::DependencyObject GetNextFocusCandidate(winrt::FocusNavigationDirection direction, bool isPageNavigation);

    static constexpr std::wstring_view s_rootPartName{ L"PART_Root"sv };
    static constexpr std::wstring_view s_scrollerPartName{ L"PART_Scroller"sv };
    static constexpr std::wstring_view s_horizontalScrollControllerPartName{ L"PART_HorizontalScrollController"sv };
    static constexpr std::wstring_view s_verticalScrollControllerPartName{ L"PART_VerticalScrollController"sv };
    static constexpr std::wstring_view s_scrollControllersSeparatorPartName{ L"PART_ScrollControllersSeparator"sv };

    tracker_ref<winrt::IUIElement> m_horizontalScrollControllerElement{ this };
    tracker_ref<winrt::IUIElement> m_verticalScrollControllerElement{ this };
    tracker_ref<winrt::IUIElement> m_scrollControllersSeparatorElement{ this };
    tracker_ref<winrt::IScroller>  m_scroller{ this };
    tracker_ref<winrt::DispatcherTimer> m_hideIndicatorsTimer{ this };

    // Event Tokens
    winrt::event_token m_gettingFocusToken{};
    winrt::event_token m_isEnabledChangedToken{};
    winrt::event_token m_unloadedToken{};

    winrt::event_token m_scrollerExtentChangedToken{};
    winrt::event_token m_scrollerStateChangedToken{};
    winrt::event_token m_scrollerChangingOffsetsToken{};
    winrt::event_token m_scrollerChangingZoomFactorToken{};
    winrt::event_token m_scrollerViewChangedToken{};
    winrt::event_token m_scrollerViewChangeCompletedToken{};
    winrt::event_token m_scrollerBringingIntoViewToken{};
    winrt::event_token m_scrollerAnchorRequestedToken{};
    winrt::event_token m_scrollerComputedHorizontalScrollModeChangedToken{};
    winrt::event_token m_scrollerComputedVerticalScrollModeChangedToken{};

    winrt::event_token m_horizontalScrollControllerInteractionInfoChangedToken{};
    winrt::event_token m_verticalScrollControllerInteractionInfoChangedToken{};

    winrt::IInspectable m_onPointerEnteredEventHandler{ nullptr };
    winrt::IInspectable m_onPointerMovedEventHandler{ nullptr };
    winrt::IInspectable m_onPointerExitedEventHandler{ nullptr };
    winrt::IInspectable m_onPointerPressedEventHandler{ nullptr };
    winrt::IInspectable m_onPointerReleasedEventHandler{ nullptr };
    winrt::IInspectable m_onPointerCanceledEventHandler{ nullptr };
    
    winrt::IInspectable m_onHorizontalScrollControllerPointerEnteredHandler{ nullptr };
    winrt::IInspectable m_onHorizontalScrollControllerPointerExitedHandler{ nullptr };
    winrt::IInspectable m_onVerticalScrollControllerPointerEnteredHandler{ nullptr };
    winrt::IInspectable m_onVerticalScrollControllerPointerExitedHandler{ nullptr };

    winrt::FocusInputDeviceKind m_focusInputDeviceKind{ winrt::FocusInputDeviceKind::None };
    
    bool m_isLeftMouseButtonPressedForFocus{ false };
    
    // Set to True when the mouse scrolling indicators are currently showing.
    bool m_showingMouseIndicators{ false };

    // Set to True to prevent the normal fade-out of the scrolling indicators.
    bool m_keepIndicatorsShowing{ false };

    // Set to True to favor mouse indicators over panning indicators for the scroll controllers.
    bool m_preferMouseIndicators{ false };

    // Set to the values of IScrollController::IsInteracting.
    bool m_isHorizontalScrollControllerInteracting{ false };
    bool m_isVerticalScrollControllerInteracting{ false };

    // Set to True when the pointer is over the optional scroll controllers.
    bool m_isPointerOverHorizontalScrollController{ false };
    bool m_isPointerOverVerticalScrollController{ false };

    // Private constants    
    // 2 seconds delay used to hide the indicators for example when OS animations are turned off.
    static constexpr int64_t s_noIndicatorCountdown = 2000 * 10000; 

    static constexpr std::wstring_view s_noIndicatorStateName{ L"NoIndicator"sv };
    static constexpr std::wstring_view s_touchIndicatorStateName{ L"TouchIndicator"sv };
    static constexpr std::wstring_view s_mouseIndicatorStateName{ L"MouseIndicator"sv };
    static constexpr std::wstring_view s_mouseIndicatorFullStateName{ L"MouseIndicatorFull"sv };

    int m_verticalScrollWithKeyboardDirection = 0;
    int m_verticalScrollWithKeyboardViewChangeId = -1;

    int m_horizontalScrollWithKeyboardDirection = 0;
    int m_horizontalScrollWithKeyboardViewChangeId = -1;
};

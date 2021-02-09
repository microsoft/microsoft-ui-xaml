// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollBarController.h"
#include "ScrollViewTrace.h"
#include "ScrollView.g.h"
#include "ScrollView.properties.h"
#include "ScrollViewBringIntoViewOperation.h"

class ScrollView :
    public ReferenceTracker<ScrollView, winrt::implementation::ScrollViewT>,
    public ScrollViewProperties
{
public:
    ScrollView();
    ~ScrollView();

    // Properties' default values.
    static const winrt::Microsoft::UI::Xaml::Controls::ScrollingScrollBarVisibility s_defaultHorizontalScrollBarVisibility{ winrt::Microsoft::UI::Xaml::Controls::ScrollingScrollBarVisibility::Auto };
    static const winrt::Microsoft::UI::Xaml::Controls::ScrollingScrollBarVisibility s_defaultVerticalScrollBarVisibility{ winrt::Microsoft::UI::Xaml::Controls::ScrollingScrollBarVisibility::Auto };
    static const winrt::ScrollingChainMode s_defaultHorizontalScrollChainMode{ winrt::ScrollingChainMode::Auto };
    static const winrt::ScrollingChainMode s_defaultVerticalScrollChainMode{ winrt::ScrollingChainMode::Auto };
    static const winrt::ScrollingRailMode s_defaultHorizontalScrollRailMode{ winrt::ScrollingRailMode::Enabled };
    static const winrt::ScrollingRailMode s_defaultVerticalScrollRailMode{ winrt::ScrollingRailMode::Enabled };
    static const winrt::Visibility s_defaultComputedHorizontalScrollBarVisibility{ winrt::Visibility::Collapsed };
    static const winrt::Visibility s_defaultComputedVerticalScrollBarVisibility{ winrt::Visibility::Collapsed };
#ifdef USE_SCROLLMODE_AUTO
    static const winrt::ScrollingScrollMode s_defaultHorizontalScrollMode{ winrt::ScrollingScrollMode::Auto };
    static const winrt::ScrollingScrollMode s_defaultVerticalScrollMode{ winrt::ScrollingScrollMode::Auto };
    static const winrt::ScrollingScrollMode s_defaultComputedHorizontalScrollMode{ winrt::ScrollingScrollMode::Disabled };
    static const winrt::ScrollingScrollMode s_defaultComputedVerticalScrollMode{ winrt::ScrollingScrollMode::Disabled };
#else
    static const winrt::ScrollingScrollMode s_defaultHorizontalScrollMode{ winrt::ScrollingScrollMode::Enabled };
    static const winrt::ScrollingScrollMode s_defaultVerticalScrollMode{ winrt::ScrollingScrollMode::Enabled };
#endif
    static const int32_t s_noOpCorrelationId;
    static const winrt::ScrollingChainMode s_defaultZoomChainMode{ winrt::ScrollingChainMode::Auto };
    static const winrt::ScrollingZoomMode s_defaultZoomMode{ winrt::ScrollingZoomMode::Disabled };
    static const winrt::ScrollingInputKinds s_defaultIgnoredInputKinds{ winrt::ScrollingInputKinds::None };
    static const winrt::ScrollingContentOrientation s_defaultContentOrientation{ winrt::ScrollingContentOrientation::Vertical };
    static constexpr double s_defaultMinZoomFactor{ 0.1 };
    static constexpr double s_defaultMaxZoomFactor{ 10.0 };
    static constexpr bool s_defaultAnchorAtExtent{ true };
    static constexpr double s_defaultAnchorRatio{ 0.0 };

#pragma region IScrollView

    winrt::UIElement CurrentAnchor();
    winrt::CompositionPropertySet ExpressionAnimationSources();

    double HorizontalOffset();
    double VerticalOffset();
    float ZoomFactor();
    double ExtentWidth();
    double ExtentHeight();
    double ViewportWidth();
    double ViewportHeight();
    double ScrollableWidth();
    double ScrollableHeight();

    winrt::ScrollingInteractionState State();

    winrt::ScrollingInputKinds IgnoredInputKinds();
    void IgnoredInputKinds(winrt::ScrollingInputKinds const& value);

    void RegisterAnchorCandidate(winrt::UIElement const& element);
    void UnregisterAnchorCandidate(winrt::UIElement const& element);

    int32_t ScrollTo(double horizontalOffset, double verticalOffset);
    int32_t ScrollTo(double horizontalOffset, double verticalOffset, winrt::ScrollingScrollOptions const& options);
    int32_t ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta);
    int32_t ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta, winrt::ScrollingScrollOptions const& options);
    int32_t AddScrollVelocity(winrt::float2 offsetsVelocity, winrt::IReference<winrt::float2> inertiaDecayRate);
    int32_t ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint);
    int32_t ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint, winrt::ScrollingZoomOptions const& options);
    int32_t ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint);
    int32_t ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint, winrt::ScrollingZoomOptions const& options);
    int32_t AddZoomVelocity(float zoomFactorVelocity, winrt::IReference<winrt::float2> centerPoint, winrt::IReference<float> inertiaDecayRate);
#pragma endregion

    // Invoked by ScrollViewTestHooks
    void ScrollControllersAutoHidingChanged();
    winrt::ScrollPresenter GetScrollPresenterPart() const;

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
    void OnScrollViewGettingFocus(
        const winrt::IInspectable& sender,
        const winrt::GettingFocusEventArgs& args);
    void OnScrollViewIsEnabledChanged(
        const winrt::IInspectable& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);
    void OnScrollViewUnloaded(
        const winrt::IInspectable& sender,
        const winrt::RoutedEventArgs& args);
    void OnScrollViewPointerEntered(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollViewPointerMoved(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollViewPointerExited(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollViewPointerPressed(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollViewPointerReleased(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollViewPointerCanceled(
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
    void OnAutoHideScrollBarsChanged(
        winrt::UISettings const& uiSettings,
        winrt::UISettingsAutoHideScrollBarsChangedEventArgs const& args);

    // Internal event handlers
    void OnScrollPresenterExtentChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnScrollPresenterStateChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnScrollAnimationStarting(
        const winrt::IInspectable& sender,
        const winrt::ScrollingScrollAnimationStartingEventArgs& args);
    void OnZoomAnimationStarting(
        const winrt::IInspectable& sender,
        const winrt::ScrollingZoomAnimationStartingEventArgs& args);
    void OnScrollPresenterViewChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
#ifdef USE_SCROLLMODE_AUTO
    void OnScrollPresenterPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);
#endif
    void OnScrollPresenterScrollCompleted(
        const winrt::IInspectable& sender,
        const winrt::ScrollingScrollCompletedEventArgs& args);
    void OnScrollPresenterZoomCompleted(
        const winrt::IInspectable& sender,
        const winrt::ScrollingZoomCompletedEventArgs& args);
    void OnScrollPresenterBringingIntoView(
        const winrt::IInspectable& sender,
        const winrt::ScrollingBringingIntoViewEventArgs& args);
    void OnScrollPresenterAnchorRequested(
        const winrt::IInspectable& sender,
        const winrt::ScrollingAnchorRequestedEventArgs& args);
    void OnCompositionTargetRendering(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);

    void ResetHideIndicatorsTimer(bool isForDestructor = false, bool restart = false);

    void HookUISettingsEvent();
    void HookCompositionTargetRendering();
    void UnhookCompositionTargetRendering();
    void HookScrollViewEvents();
    void UnhookScrollViewEvents();
    void HookScrollPresenterEvents();
    void UnhookScrollPresenterEvents(bool isForDestructor);
    void HookHorizontalScrollControllerEvents();
    void UnhookHorizontalScrollControllerEvents();
    void HookVerticalScrollControllerEvents();
    void UnhookVerticalScrollControllerEvents();

    void UpdateScrollPresenter(const winrt::ScrollPresenter& scrollPresenter);
    void UpdateHorizontalScrollController(
        const winrt::IScrollController& horizontalScrollController,
        const winrt::IUIElement& horizontalScrollControllerElement);
    void UpdateVerticalScrollController(
        const winrt::IScrollController& verticalScrollController,
        const winrt::IUIElement& verticalScrollControllerElement);
    void UpdateScrollControllersSeparator(const winrt::IUIElement& scrollControllersSeparator);
    void UpdateScrollPresenterHorizontalScrollController(const winrt::IScrollController& horizontalScrollController);
    void UpdateScrollPresenterVerticalScrollController(const winrt::IScrollController& verticalScrollController);
    void UpdateScrollControllersVisibility(bool horizontalChange, bool verticalChange);

    bool IsInputKindIgnored(winrt::ScrollingInputKinds const& inputKind);

    bool AreAllScrollControllersCollapsed() const;
    bool AreBothScrollControllersVisible() const;
    bool AreScrollControllersAutoHiding();
    bool IsScrollControllersSeparatorVisible() const;
    void HideIndicators(bool useTransitions = true);
    void HideIndicatorsAfterDelay();
    void UpdateScrollControllersAutoHiding(bool forceUpdate = false);
    void UpdateVisualStates(
        bool useTransitions = true,
        bool showIndicators = false,
        bool hideIndicators = false,
        bool scrollControllersAutoHidingChanged = false,
        bool updateScrollControllersAutoHiding = false,
        bool onlyForAutoHidingScrollControllers = false);
    void UpdateScrollControllersVisualState(bool useTransitions = true, bool showIndicators = false, bool hideIndicators = false);
    void UpdateScrollControllersSeparatorVisualState(bool useTransitions = true, bool scrollControllersAutoHidingChanged = false);
    void GoToState(std::wstring_view const& stateName, bool useTransitions = true);

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
    static constexpr std::wstring_view s_scrollPresenterPartName{ L"PART_ScrollPresenter"sv };
    static constexpr std::wstring_view s_horizontalScrollBarPartName{ L"PART_HorizontalScrollBar"sv };
    static constexpr std::wstring_view s_verticalScrollBarPartName{ L"PART_VerticalScrollBar"sv };
    static constexpr std::wstring_view s_scrollBarsSeparatorPartName{ L"PART_ScrollBarsSeparator"sv };
    static constexpr std::wstring_view s_IScrollAnchorProviderNotImpl{ L"Template part named PART_ScrollPresenter does not implement IScrollAnchorProvider."sv };
    static constexpr std::wstring_view s_noScrollPresenterPart{ L"No template part named PART_ScrollPresenter was loaded."sv };

    winrt::com_ptr<ScrollBarController> m_horizontalScrollBarController{ nullptr };
    winrt::com_ptr<ScrollBarController> m_verticalScrollBarController{ nullptr };

    tracker_ref<winrt::IScrollController> m_horizontalScrollController{ this };
    tracker_ref<winrt::IScrollController> m_verticalScrollController{ this };
    tracker_ref<winrt::IUIElement> m_horizontalScrollControllerElement{ this };
    tracker_ref<winrt::IUIElement> m_verticalScrollControllerElement{ this };
    tracker_ref<winrt::IUIElement> m_scrollControllersSeparatorElement{ this };
    tracker_ref<winrt::IScrollPresenter>  m_scrollPresenter{ this };
    tracker_ref<winrt::DispatcherTimer> m_hideIndicatorsTimer{ this };

    // Event Tokens
    winrt::event_token m_gettingFocusToken{};
    winrt::event_token m_isEnabledChangedToken{};
    winrt::event_token m_unloadedToken{};

    winrt::event_token m_scrollPresenterExtentChangedToken{};
    winrt::event_token m_scrollPresenterStateChangedToken{};
    winrt::event_token m_scrollPresenterScrollAnimationStartingToken{};
    winrt::event_token m_scrollPresenterZoomAnimationStartingToken{};
    winrt::event_token m_scrollPresenterViewChangedToken{};
    winrt::event_token m_scrollPresenterScrollCompletedToken{};
    winrt::event_token m_scrollPresenterZoomCompletedToken{};
    winrt::event_token m_scrollPresenterBringingIntoViewToken{};
    winrt::event_token m_scrollPresenterAnchorRequestedToken{};
#ifdef USE_SCROLLMODE_AUTO
    winrt::event_token m_scrollPresenterComputedHorizontalScrollModeChangedToken{};
    winrt::event_token m_scrollPresenterComputedVerticalScrollModeChangedToken{};
#endif

    winrt::event_token m_horizontalScrollControllerInteractionInfoChangedToken{};
    winrt::event_token m_verticalScrollControllerInteractionInfoChangedToken{};

    winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendering_revoker m_renderingToken{};

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

    // Used to detect changes for UISettings.AutoHiScrollBars.
    winrt::IUISettings5 m_uiSettings5{ nullptr };
    winrt::IUISettings5::AutoHideScrollBarsChanged_revoker m_autoHideScrollBarsChangedRevoker{};

    bool m_autoHideScrollControllersValid{ false };
    bool m_autoHideScrollControllers{ false };

    bool m_isLeftMouseButtonPressedForFocus{ false };
    
    // Set to True when the mouse scrolling indicators are currently showing.
    bool m_showingMouseIndicators{ false };

    // Set to True to prevent the normal fade-out of the scrolling indicators.
    bool m_keepIndicatorsShowing{ false };

    // Set to True to favor mouse indicators over panning indicators for the scroll controllers.
    bool m_preferMouseIndicators{ false };

    // Indicates whether the NoIndicator visual state has a Storyboard for which a completion event was hooked up.
    bool m_hasNoIndicatorStateStoryboardCompletedHandler{ false };

    // Set to the values of IScrollController::IsInteracting.
    bool m_isHorizontalScrollControllerInteracting{ false };
    bool m_isVerticalScrollControllerInteracting{ false };

    // Set to True when the pointer is over the optional scroll controllers.
    bool m_isPointerOverHorizontalScrollController{ false };
    bool m_isPointerOverVerticalScrollController{ false };

    int m_verticalAddScrollVelocityDirection{ 0 };
    int32_t m_verticalAddScrollVelocityOffsetChangeCorrelationId{ -1 };

    int m_horizontalAddScrollVelocityDirection{ 0 };
    int32_t m_horizontalAddScrollVelocityOffsetChangeCorrelationId{ -1 };

    // List of temporary ScrollViewBringIntoViewOperation instances used to track expected
    // ScrollPresenter::BringingIntoView occurrences due to navigation.
    std::list<std::shared_ptr<ScrollViewBringIntoViewOperation>> m_bringIntoViewOperations;

    // Private constants    
    // 2 seconds delay used to hide the indicators for example when OS animations are turned off.
    static constexpr int64_t s_noIndicatorCountdown = 2000 * 10000; 

    static constexpr std::wstring_view s_noIndicatorStateName{ L"NoIndicator"sv };
    static constexpr std::wstring_view s_touchIndicatorStateName{ L"TouchIndicator"sv };
    static constexpr std::wstring_view s_mouseIndicatorStateName{ L"MouseIndicator"sv };

    static constexpr std::wstring_view s_scrollBarsSeparatorExpanded{ L"ScrollBarsSeparatorExpanded"sv };
    static constexpr std::wstring_view s_scrollBarsSeparatorCollapsed{ L"ScrollBarsSeparatorCollapsed"sv };
    static constexpr std::wstring_view s_scrollBarsSeparatorCollapsedDisabled{ L"ScrollBarsSeparatorCollapsedDisabled"sv };
    static constexpr std::wstring_view s_scrollBarsSeparatorCollapsedWithoutAnimation{ L"ScrollBarsSeparatorCollapsedWithoutAnimation"sv };
    static constexpr std::wstring_view s_scrollBarsSeparatorDisplayedWithoutAnimation{ L"ScrollBarsSeparatorDisplayedWithoutAnimation"sv };
    static constexpr std::wstring_view s_scrollBarsSeparatorExpandedWithoutAnimation{ L"ScrollBarsSeparatorExpandedWithoutAnimation"sv };
};

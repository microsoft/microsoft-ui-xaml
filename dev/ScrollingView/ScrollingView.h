// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollBarController.h"
#include "ScrollingViewTrace.h"
#include "ScrollingView.g.h"
#include "ScrollingView.properties.h"
#include "ScrollingViewBringIntoViewOperation.h"

class ScrollingView :
    public ReferenceTracker<ScrollingView, winrt::implementation::ScrollingViewT>,
    public ScrollingViewProperties
{
public:
    ScrollingView();
    ~ScrollingView();

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
    static const winrt::ScrollingScrollInfo s_noOpScrollInfo;
    static const winrt::ScrollingZoomInfo s_noOpZoomInfo;
    static const winrt::ScrollingChainMode s_defaultZoomChainMode{ winrt::ScrollingChainMode::Auto };
    static const winrt::ScrollingZoomMode s_defaultZoomMode{ winrt::ScrollingZoomMode::Disabled };
    static const winrt::ScrollingInputKinds s_defaultIgnoredInputKind{ winrt::ScrollingInputKinds::None };
    static const winrt::ScrollingContentOrientation s_defaultContentOrientation{ winrt::ScrollingContentOrientation::Vertical };
    static constexpr double s_defaultMinZoomFactor{ 0.1 };
    static constexpr double s_defaultMaxZoomFactor{ 10.0 };
    static constexpr bool s_defaultAnchorAtExtent{ true };
    static constexpr double s_defaultAnchorRatio{ 0.0 };

#pragma region IScrollingView

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

    winrt::ScrollingInputKinds IgnoredInputKind();
    void IgnoredInputKind(winrt::ScrollingInputKinds const& value);

    void RegisterAnchorCandidate(winrt::UIElement const& element);
    void UnregisterAnchorCandidate(winrt::UIElement const& element);

    winrt::ScrollingScrollInfo ScrollTo(double horizontalOffset, double verticalOffset);
    winrt::ScrollingScrollInfo ScrollTo(double horizontalOffset, double verticalOffset, winrt::ScrollingScrollOptions const& options);
    winrt::ScrollingScrollInfo ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta);
    winrt::ScrollingScrollInfo ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta, winrt::ScrollingScrollOptions const& options);
    winrt::ScrollingScrollInfo ScrollFrom(winrt::float2 offsetsVelocity, winrt::IReference<winrt::float2> inertiaDecayRate);
    winrt::ScrollingZoomInfo ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint);
    winrt::ScrollingZoomInfo ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint, winrt::ScrollingZoomOptions const& options);
    winrt::ScrollingZoomInfo ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint);
    winrt::ScrollingZoomInfo ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint, winrt::ScrollingZoomOptions const& options);
    winrt::ScrollingZoomInfo ZoomFrom(float zoomFactorVelocity, winrt::IReference<winrt::float2> centerPoint, winrt::IReference<float> inertiaDecayRate);
#pragma endregion

    // Invoked by ScrollingViewTestHooks
    void ScrollControllersAutoHidingChanged();
    winrt::ScrollingPresenter GetScrollingPresenterPart() const;

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
    void OnScrollingViewGettingFocus(
        const winrt::IInspectable& sender,
        const winrt::GettingFocusEventArgs& args);
    void OnScrollingViewIsEnabledChanged(
        const winrt::IInspectable& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);
    void OnScrollingViewUnloaded(
        const winrt::IInspectable& sender,
        const winrt::RoutedEventArgs& args);
    void OnScrollingViewPointerEntered(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollingViewPointerMoved(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollingViewPointerExited(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollingViewPointerPressed(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollingViewPointerReleased(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollingViewPointerCanceled(
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
    void OnScrollingPresenterExtentChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnScrollingPresenterStateChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnScrollAnimationStarting(
        const winrt::IInspectable& sender,
        const winrt::ScrollingScrollAnimationStartingEventArgs& args);
    void OnZoomAnimationStarting(
        const winrt::IInspectable& sender,
        const winrt::ScrollingZoomAnimationStartingEventArgs& args);
    void OnScrollingPresenterViewChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
#ifdef USE_SCROLLMODE_AUTO
    void OnScrollingPresenterPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);
#endif
    void OnScrollingPresenterScrollCompleted(
        const winrt::IInspectable& sender,
        const winrt::ScrollingScrollCompletedEventArgs& args);
    void OnScrollingPresenterZoomCompleted(
        const winrt::IInspectable& sender,
        const winrt::ScrollingZoomCompletedEventArgs& args);
    void OnScrollingPresenterBringingIntoView(
        const winrt::IInspectable& sender,
        const winrt::ScrollingBringingIntoViewEventArgs& args);
    void OnScrollingPresenterAnchorRequested(
        const winrt::IInspectable& sender,
        const winrt::ScrollingAnchorRequestedEventArgs& args);
    void OnCompositionTargetRendering(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);

    void ResetHideIndicatorsTimer(bool isForDestructor = false, bool restart = false);

    void HookUISettingsEvent();
    void HookCompositionTargetRendering();
    void UnhookCompositionTargetRendering();
    void HookScrollingViewEvents();
    void UnhookScrollingViewEvents();
    void HookScrollingPresenterEvents();
    void UnhookScrollingPresenterEvents(bool isForDestructor);
    void HookHorizontalScrollControllerEvents();
    void UnhookHorizontalScrollControllerEvents();
    void HookVerticalScrollControllerEvents();
    void UnhookVerticalScrollControllerEvents();

    void UpdateScrollingPresenter(const winrt::ScrollingPresenter& scrollingPresenter);
    void UpdateHorizontalScrollController(
        const winrt::IScrollController& horizontalScrollController,
        const winrt::IUIElement& horizontalScrollControllerElement);
    void UpdateVerticalScrollController(
        const winrt::IScrollController& verticalScrollController,
        const winrt::IUIElement& verticalScrollControllerElement);
    void UpdateScrollControllersSeparator(const winrt::IUIElement& scrollControllersSeparator);
    void UpdateScrollingPresenterHorizontalScrollController(const winrt::IScrollController& horizontalScrollController);
    void UpdateScrollingPresenterVerticalScrollController(const winrt::IScrollController& verticalScrollController);
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
    static constexpr std::wstring_view s_scrollingPresenterPartName{ L"PART_ScrollingPresenter"sv };
    static constexpr std::wstring_view s_horizontalScrollBarPartName{ L"PART_HorizontalScrollBar"sv };
    static constexpr std::wstring_view s_verticalScrollBarPartName{ L"PART_VerticalScrollBar"sv };
    static constexpr std::wstring_view s_scrollBarsSeparatorPartName{ L"PART_ScrollBarsSeparator"sv };
    static constexpr std::wstring_view s_IScrollAnchorProviderNotImpl{ L"Template part named PART_ScrollingPresenter does not implement IScrollAnchorProvider."sv };
    static constexpr std::wstring_view s_noScrollingPresenterPart{ L"No template part named PART_ScrollingPresenter was loaded."sv };

    winrt::com_ptr<ScrollBarController> m_horizontalScrollBarController{ nullptr };
    winrt::com_ptr<ScrollBarController> m_verticalScrollBarController{ nullptr };

    tracker_ref<winrt::IScrollController> m_horizontalScrollController{ this };
    tracker_ref<winrt::IScrollController> m_verticalScrollController{ this };
    tracker_ref<winrt::IUIElement> m_horizontalScrollControllerElement{ this };
    tracker_ref<winrt::IUIElement> m_verticalScrollControllerElement{ this };
    tracker_ref<winrt::IUIElement> m_scrollControllersSeparatorElement{ this };
    tracker_ref<winrt::IScrollingPresenter>  m_scrollingPresenter{ this };
    tracker_ref<winrt::DispatcherTimer> m_hideIndicatorsTimer{ this };

    // Event Tokens
    winrt::event_token m_gettingFocusToken{};
    winrt::event_token m_isEnabledChangedToken{};
    winrt::event_token m_unloadedToken{};

    winrt::event_token m_scrollingPresenterExtentChangedToken{};
    winrt::event_token m_scrollingPresenterStateChangedToken{};
    winrt::event_token m_scrollingPresenterScrollAnimationStartingToken{};
    winrt::event_token m_scrollingPresenterZoomAnimationStartingToken{};
    winrt::event_token m_scrollingPresenterViewChangedToken{};
    winrt::event_token m_scrollingPresenterScrollCompletedToken{};
    winrt::event_token m_scrollingPresenterZoomCompletedToken{};
    winrt::event_token m_scrollingPresenterBringingIntoViewToken{};
    winrt::event_token m_scrollingPresenterAnchorRequestedToken{};
#ifdef USE_SCROLLMODE_AUTO
    winrt::event_token m_scrollingPresenterComputedHorizontalScrollModeChangedToken{};
    winrt::event_token m_scrollingPresenterComputedVerticalScrollModeChangedToken{};
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

    int m_verticalScrollFromDirection{ 0 };
    int m_verticalScrollFromOffsetChangeId{ -1 };

    int m_horizontalScrollFromDirection{ 0 };
    int m_horizontalScrollFromOffsetChangeId{ -1 };

    // List of temporary ScrollingViewBringIntoViewOperation instances used to track expected
    // ScrollingPresenter::BringingIntoView occurrences due to navigation.
    std::list<std::shared_ptr<ScrollingViewBringIntoViewOperation>> m_bringIntoViewOperations;

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

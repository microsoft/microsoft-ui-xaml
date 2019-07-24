// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollBarController.h"
#include "ScrollViewerTrace.h"
#include "ScrollViewer.g.h"
#include "ScrollViewer.properties.h"
#include "ScrollViewerBringIntoViewOperation.h"

class ScrollViewer :
    public ReferenceTracker<ScrollViewer, winrt::implementation::ScrollViewerT>,
    public ScrollViewerProperties
{
public:
    ScrollViewer();
    ~ScrollViewer();

    // Properties' default values.
    static const winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility s_defaultHorizontalScrollBarVisibility{ winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility::Auto };
    static const winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility s_defaultVerticalScrollBarVisibility{ winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility::Auto };
    static const winrt::ChainingMode s_defaultHorizontalScrollChainingMode{ winrt::ChainingMode::Auto };
    static const winrt::ChainingMode s_defaultVerticalScrollChainingMode{ winrt::ChainingMode::Auto };
    static const winrt::RailingMode s_defaultHorizontalScrollRailingMode{ winrt::RailingMode::Enabled };
    static const winrt::RailingMode s_defaultVerticalScrollRailingMode{ winrt::RailingMode::Enabled };    
    static const winrt::Visibility s_defaultComputedHorizontalScrollBarVisibility{ winrt::Visibility::Collapsed };
    static const winrt::Visibility s_defaultComputedVerticalScrollBarVisibility{ winrt::Visibility::Collapsed };
#ifdef USE_SCROLLMODE_AUTO
    static const winrt::ScrollMode s_defaultHorizontalScrollMode{ winrt::ScrollMode::Auto };
    static const winrt::ScrollMode s_defaultVerticalScrollMode{ winrt::ScrollMode::Auto };
    static const winrt::ScrollMode s_defaultComputedHorizontalScrollMode{ winrt::ScrollMode::Disabled };
    static const winrt::ScrollMode s_defaultComputedVerticalScrollMode{ winrt::ScrollMode::Disabled };
#else
    static const winrt::ScrollMode s_defaultHorizontalScrollMode{ winrt::ScrollMode::Enabled };
    static const winrt::ScrollMode s_defaultVerticalScrollMode{ winrt::ScrollMode::Enabled };
#endif
    static const winrt::ScrollInfo s_noOpScrollInfo;
    static const winrt::ZoomInfo s_noOpZoomInfo;
    static const winrt::ChainingMode s_defaultZoomChainingMode{ winrt::ChainingMode::Auto };
    static const winrt::ZoomMode s_defaultZoomMode{ winrt::ZoomMode::Disabled };
    static const winrt::InputKind s_defaultIgnoredInputKind{ winrt::InputKind::None };
    static const winrt::ContentOrientation s_defaultContentOrientation{ winrt::ContentOrientation::Vertical };
    static constexpr double s_defaultMinZoomFactor{ 0.1 };
    static constexpr double s_defaultMaxZoomFactor{ 10.0 };
    static constexpr bool s_defaultAnchorAtExtent{ true };
    static constexpr double s_defaultAnchorRatio{ 0.0 };

#pragma region IScrollViewer

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

    winrt::InteractionState State();

    winrt::InputKind IgnoredInputKind();
    void IgnoredInputKind(winrt::InputKind const& value);

    void RegisterAnchorCandidate(winrt::UIElement const& element);
    void UnregisterAnchorCandidate(winrt::UIElement const& element);

    winrt::ScrollInfo ScrollTo(double horizontalOffset, double verticalOffset);
    winrt::ScrollInfo ScrollTo(double horizontalOffset, double verticalOffset, winrt::ScrollOptions const& options);
    winrt::ScrollInfo ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta);
    winrt::ScrollInfo ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta, winrt::ScrollOptions const& options);
    winrt::ScrollInfo ScrollFrom(winrt::float2 offsetsVelocity, winrt::IReference<winrt::float2> inertiaDecayRate);
    winrt::ZoomInfo ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint);
    winrt::ZoomInfo ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint, winrt::ZoomOptions const& options);
    winrt::ZoomInfo ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint);
    winrt::ZoomInfo ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint, winrt::ZoomOptions const& options);
    winrt::ZoomInfo ZoomFrom(float zoomFactorVelocity, winrt::IReference<winrt::float2> centerPoint, winrt::IReference<float> inertiaDecayRate);
#pragma endregion

    // Invoked by ScrollViewerTestHooks
    void ScrollControllersAutoHidingChanged();
    winrt::Scroller GetScrollerPart() const;

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
    void OnScrollViewerGettingFocus(
        const winrt::IInspectable& sender,
        const winrt::GettingFocusEventArgs& args);
    void OnScrollViewerIsEnabledChanged(
        const winrt::IInspectable& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);
    void OnScrollViewerUnloaded(
        const winrt::IInspectable& sender,
        const winrt::RoutedEventArgs& args);
    void OnScrollViewerPointerEntered(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollViewerPointerMoved(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollViewerPointerExited(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollViewerPointerPressed(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollViewerPointerReleased(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);
    void OnScrollViewerPointerCanceled(
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
    void OnScrollerExtentChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnScrollerStateChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
    void OnScrollAnimationStarting(
        const winrt::IInspectable& sender,
        const winrt::ScrollAnimationStartingEventArgs& args);
    void OnZoomAnimationStarting(
        const winrt::IInspectable& sender,
        const winrt::ZoomAnimationStartingEventArgs& args);
    void OnScrollerViewChanged(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);
#ifdef USE_SCROLLMODE_AUTO
    void OnScrollerPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);
#endif
    void OnScrollerScrollCompleted(
        const winrt::IInspectable& sender,
        const winrt::ScrollCompletedEventArgs& args);
    void OnScrollerZoomCompleted(
        const winrt::IInspectable& sender,
        const winrt::ZoomCompletedEventArgs& args);
    void OnScrollerBringingIntoView(
        const winrt::IInspectable& sender,
        const winrt::ScrollerBringingIntoViewEventArgs& args);
    void OnScrollerAnchorRequested(
        const winrt::IInspectable& sender,
        const winrt::ScrollerAnchorRequestedEventArgs& args);
    void OnCompositionTargetRendering(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);

    void ResetHideIndicatorsTimer(bool isForDestructor = false, bool restart = false);

    void HookUISettingsEvent();
    void HookCompositionTargetRendering();
    void UnhookCompositionTargetRendering();
    void HookScrollViewerEvents();
    void UnhookScrollViewerEvents();
    void HookScrollerEvents();
    void UnhookScrollerEvents(bool isForDestructor);
    void HookHorizontalScrollControllerEvents();
    void UnhookHorizontalScrollControllerEvents();
    void HookVerticalScrollControllerEvents();
    void UnhookVerticalScrollControllerEvents();

    void UpdateScroller(const winrt::Scroller& scroller);
    void UpdateHorizontalScrollController(
        const winrt::IScrollController& horizontalScrollController,
        const winrt::IUIElement& horizontalScrollControllerElement);
    void UpdateVerticalScrollController(
        const winrt::IScrollController& verticalScrollController,
        const winrt::IUIElement& verticalScrollControllerElement);
    void UpdateScrollControllersSeparator(const winrt::IUIElement& scrollControllersSeparator);
    void UpdateScrollerHorizontalScrollController(const winrt::IScrollController& horizontalScrollController);
    void UpdateScrollerVerticalScrollController(const winrt::IScrollController& verticalScrollController);
    void UpdateScrollControllersVisibility(bool horizontalChange, bool verticalChange);

    bool IsInputKindIgnored(winrt::InputKind const& inputKind);

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
    static constexpr std::wstring_view s_scrollerPartName{ L"PART_Scroller"sv };
    static constexpr std::wstring_view s_horizontalScrollBarPartName{ L"PART_HorizontalScrollBar"sv };
    static constexpr std::wstring_view s_verticalScrollBarPartName{ L"PART_VerticalScrollBar"sv };
    static constexpr std::wstring_view s_scrollBarsSeparatorPartName{ L"PART_ScrollBarsSeparator"sv };
    static constexpr std::wstring_view s_IScrollAnchorProviderNotImpl{ L"Template part named PART_Scroller does not implement IScrollAnchorProvider."sv };
    static constexpr std::wstring_view s_noScrollerPart{ L"No template part named PART_Scroller was loaded."sv };

    winrt::com_ptr<ScrollBarController> m_horizontalScrollBarController{ nullptr };
    winrt::com_ptr<ScrollBarController> m_verticalScrollBarController{ nullptr };

    tracker_ref<winrt::IScrollController> m_horizontalScrollController{ this };
    tracker_ref<winrt::IScrollController> m_verticalScrollController{ this };
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
    winrt::event_token m_scrollerScrollAnimationStartingToken{};
    winrt::event_token m_scrollerZoomAnimationStartingToken{};
    winrt::event_token m_scrollerViewChangedToken{};
    winrt::event_token m_scrollerScrollCompletedToken{};
    winrt::event_token m_scrollerZoomCompletedToken{};
    winrt::event_token m_scrollerBringingIntoViewToken{};
    winrt::event_token m_scrollerAnchorRequestedToken{};
#ifdef USE_SCROLLMODE_AUTO
    winrt::event_token m_scrollerComputedHorizontalScrollModeChangedToken{};
    winrt::event_token m_scrollerComputedVerticalScrollModeChangedToken{};
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

    // List of temporary ScrollViewerBringIntoViewOperation instances used to track expected
    // Scroller::BringingIntoView occurrences due to navigation.
    std::list<std::shared_ptr<ScrollViewerBringIntoViewOperation>> m_bringIntoViewOperations;

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

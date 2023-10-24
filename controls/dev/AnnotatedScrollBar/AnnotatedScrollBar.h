// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AnnotatedScrollBarTrace.h"
#include "AnnotatedScrollBar.g.h"
#include "AnnotatedScrollBar.properties.h"
#include "AnnotatedScrollBarPanningInfo.h"

class AnnotatedScrollBar :
    public ReferenceTracker<AnnotatedScrollBar, winrt::implementation::AnnotatedScrollBarT, winrt::cloaked<winrt::IScrollController>>,
    public AnnotatedScrollBarProperties
{

public:
    AnnotatedScrollBar();
    ~AnnotatedScrollBar();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnApplyTemplate();

    winrt::IScrollController ScrollController();

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

    winrt::event_token CanScrollChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value);
    void CanScrollChanged(winrt::event_token const& token);

    winrt::event_token IsScrollingWithMouseChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value);
    void IsScrollingWithMouseChanged(winrt::event_token const& token);

    winrt::event_token ScrollToRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollToRequestedEventArgs> const& value);
    void ScrollToRequested(winrt::event_token const& token);

    winrt::event_token ScrollByRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollByRequestedEventArgs> const& value);
    void ScrollByRequested(winrt::event_token const& token);

    winrt::event_token AddScrollVelocityRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerAddScrollVelocityRequestedEventArgs> const& value);
    void AddScrollVelocityRequested(winrt::event_token const& token);
#pragma endregion

private:

    static constexpr std::wstring_view s_verticalThumbPartName { L"PART_VerticalThumb"sv };
    static constexpr std::wstring_view s_verticalThumbGhostPartName { L"PART_VerticalThumbGhost"sv };
    static constexpr std::wstring_view s_verticalDecrementRepeatButtonPartName { L"PART_VerticalDecrementRepeatButton"sv };
    static constexpr std::wstring_view s_verticalIncrementRepeatButtonPartName { L"PART_VerticalIncrementRepeatButton"sv };
    static constexpr std::wstring_view s_verticalGridPartName { L"PART_VerticalGrid"sv };
    static constexpr std::wstring_view s_labelsGridPartName { L"PART_LabelsGrid"sv };
    static constexpr std::wstring_view s_tooltipContentPresenterPartName { L"PART_TooltipContentPresenter"sv };
    static constexpr std::wstring_view s_detailLabelToolTipPartName { L"PART_DetailLabelToolTip"sv };

    void ViewportSize(double viewportSize);
    double ViewportSize();
    double m_viewportSize{ 0.0 };
    void Maximum(double maximum);
    double Maximum();
    double m_maximum{ 0.0 };
    void Minimum(double minimum);
    double Minimum();
    double m_minimum{ 0.0 };
    void Value(double value);
    double Value();
    double m_value{ 0.0 };

    // Visual Components
    tracker_ref<winrt::Border> m_verticalThumb{ this };
    tracker_ref<winrt::Border> m_verticalThumbGhost{ this };
    tracker_ref<winrt::RepeatButton> m_verticalDecrementRepeatButton{ this };
    tracker_ref<winrt::RepeatButton> m_verticalIncrementRepeatButton{ this };
    tracker_ref<winrt::Grid> m_verticalGrid{ this };
    tracker_ref<winrt::Grid> m_labelsGrid{ this };
    tracker_ref<winrt::ContentPresenter> m_tooltipContentPresenter{ this };
    tracker_ref<winrt::ToolTip> m_detailLabelToolTip{ this };

    tracker_com_ref<AnnotatedScrollBarPanningInfo> m_panningInfo{ this };

    // Visual state tracking
    winrt::Pointer m_capturedPointer{ nullptr };
    uint32_t m_verticalGridTrackedPointerId{ 0 };
    bool m_isPressed{ false };
    bool m_isPointerOver{ false };
    double m_lastVerticalGridPointerMovedOffset{ 0.0 };

    bool m_detailLabelTemplateApplied{ false };

    bool m_canScroll{ false };
    bool m_isScrollingWithMouse{ false };
    bool m_isScrollable{ false };
    int m_operationsCount{ 0 };
    double m_lastOffset{ 0.0 };
    int32_t m_lastOffsetChangeCorrelationIdForAddScrollVelocity{ -1 };
    int32_t m_lastOffsetChangeCorrelationIdForScrollBy{ -1 };
    int32_t m_lastOffsetChangeCorrelationIdForScrollTo{ -1 };

    std::vector<float> m_labelSizes{};
    std::atomic_flag m_labelsDebounce{};

    // Event Sources for IScrollController
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollToRequestedEventArgs>> m_scrollToRequested { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollByRequestedEventArgs>> m_scrollByRequested { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerAddScrollVelocityRequestedEventArgs>> m_addScrollVelocityRequested { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable>> m_canScrollChanged { };
    event<winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable>> m_isScrollingWithMouseChanged { };

    // Event Revokers
    winrt::UIElement::PointerEntered_revoker m_verticalGridPointerEnteredRevoker{};
    winrt::UIElement::PointerMoved_revoker m_verticalGridPointerMovedRevoker{};
    RoutedEventHandler_revoker m_verticalGridPointerExitedRevoker{};
    winrt::UIElement::PointerPressed_revoker  m_verticalGridPointerPressedRevoker{};
    RoutedEventHandler_revoker m_verticalGridPointerReleasedRevoker{};
    RoutedEventHandler_revoker m_verticalGridPointerCanceledRevoker{};
    RoutedEventHandler_revoker m_verticalGridPointerCaptureLostRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_sizeChangedRevoker{};
#ifdef DBG
    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};
#endif
    winrt::Control::IsEnabledChanged_revoker m_isEnabledRevoker{};
    winrt::IObservableVector<winrt::AnnotatedScrollBarLabel>::VectorChanged_revoker m_labelsVectorChangedRevoker{};
    winrt::RepeatButton::Click_revoker m_verticalIncrementRepeatButtonClickRevoker{};
    winrt::RepeatButton::Click_revoker m_verticalDecrementRepeatButtonClickRevoker{};
    winrt::ToolTip::Opened_revoker m_detailLabelToolTipOpenedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_labelsGridSizeChangedRevoker{};

    void RaiseCanScrollChanged();
    void RaiseIsScrollingWithMouseChanged();
    bool RaiseScrolling(double offset, winrt::AnnotatedScrollBarScrollingEventKind scrollingEventKind);
    double RaiseScrollOffsetFromValueRequested(double value);
    double RaiseValueFromScrollOffsetRequested(double offset);
    winrt::IInspectable RaiseDetailLabelRequested(double offsetFromTop);
    void RaiseScrollToRequested(double offset);
    void RaiseScrollByRequested(double offsetDelta);
    void RaiseAddScrollVelocityRequested(double offsetDelta);
    void ScrollTo(double offset, winrt::AnnotatedScrollBarScrollingEventKind scrollingEventKind);

    void ResetTrackedPointerId();
    bool IgnorePointerId(const winrt::PointerRoutedEventArgs& args);

    void OnVerticalGridPointerEntered(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnVerticalGridPointerMoved(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnVerticalGridPointerExited(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnVerticalGridPointerPressed(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnVerticalGridPointerReleased(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnVerticalGridPointerCanceled(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnVerticalGridPointerCaptureLost(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void ProcessVerticalGridPointerCanceled(const winrt::PointerRoutedEventArgs& args);
    bool IsOutOfVerticalGridBounds(const winrt::Point& point);

    void OnIncrementRepeatButtonClick(const winrt::IInspectable& sender,  const winrt::RoutedEventArgs& args);
    void OnDecrementRepeatButtonClick(const winrt::IInspectable& sender,  const winrt::RoutedEventArgs& args);

    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
#ifdef DBG
    void OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
#endif
    void OnIsEnabledChanged(const winrt::IInspectable& sender, const winrt::DependencyPropertyChangedEventArgs& args);
    void OnLabelsCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnLabelsGridSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);

    void UnhookHandlers(bool isFromDestructor);
    void HookInputEvents();
    void SetUpInteractionElements();
    void UpdatePositionsOfAnchoredElements();
    void QueueLayoutLabels(unsigned int millisecondWait);
    void LayoutLabels();
    void CreateLabelContainers();
    void UpdateLabelContainersOffsets();
    void CollapseCollidingAndOutOfBoundsLabels();
    void UpdateScrollOffsetToLabelOffsetFactor();
    double GetLabelVerticalOffset(winrt::AnnotatedScrollBarLabel label);
    void UpdateCanScroll();
    void EnsurePanningInfo();
    void EnsureSmallChangeValue();

    // Hover visuals logic (ToolTip and ThumbGhost)
    void OnDetailLabelToolTipOpened(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void SetToolTipIsOpen(bool isOpen);
    void ShowHoverVisuals(double offsetFromTopToolTip, double offsetFromTopThumbGhost);
    void HideHoverVisuals();
    void ShowToolTipAtOffset(double offsetFromTop);
    void ShowThumbGhostAtOffset(double offsetFromTop);
    void SetThumbGhostVisibility(winrt::Visibility visibility);

    // Default amount to scroll when hitting the PART_VerticalIncrementRepeatButton/PART_VerticalDecrementRepeatButton buttons: 1/8 of the viewport size.
    static constexpr double s_defaultViewportToSmallChangeRatio{ 8.0 };
    // Additional velocity required with decay s_smallChangeInertiaDecayRate to move Position by one pixel.
    static constexpr double s_velocityNeededPerPixel{ 3.688880455092886 };
    // Inertia decay rate for SmallChange animated Value changes.
    static constexpr float s_smallChangeInertiaDecayRate{ 0.975f};
    // Vertical offset of the tooltip during touch interactions.
    static constexpr double s_toolTipTouchOffset{ 22.0 };
};

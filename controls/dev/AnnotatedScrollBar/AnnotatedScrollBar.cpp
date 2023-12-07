// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AnnotatedScrollBar.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ScrollingScrollOptions.h"
#include "ScrollControllerScrollToRequestedEventArgs.h"
#include "ScrollControllerScrollByRequestedEventArgs.h"
#include "ScrollControllerAddScrollVelocityRequestedEventArgs.h"
#include "ScrollControllerPanRequestedEventArgs.h"
#include "AnnotatedScrollBarScrollingEventArgs.h"
#include "AnnotatedScrollBarDetailLabelRequestedEventArgs.h"
#include "Vector.h"

// Change to 'true' to turn on debugging outputs in Output window
bool AnnotatedScrollBarTrace::s_IsDebugOutputEnabled{ false };
bool AnnotatedScrollBarTrace::s_IsVerboseDebugOutputEnabled{ false };

static winrt::Size c_infSize{ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };

AnnotatedScrollBar::AnnotatedScrollBar()
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    __RP_Marker_ClassById(RuntimeProfiler::ProfId_AnnotatedScrollBar);

    EnsureProperties();
    SetDefaultStyleKey(this);

    m_sizeChangedRevoker = SizeChanged(winrt::auto_revoke, { this, &AnnotatedScrollBar::OnSizeChanged });
#ifdef DBG
    m_loadedRevoker = Loaded(winrt::auto_revoke, { this, &AnnotatedScrollBar::OnLoaded });
#endif
    m_isEnabledRevoker = IsEnabledChanged(winrt::auto_revoke, { this, &AnnotatedScrollBar::OnIsEnabledChanged });

    SetValue(s_LabelsProperty, winrt::make<ObservableVector<winrt::AnnotatedScrollBarLabel>>());

    auto observableVector = Labels().try_as<winrt::IObservableVector<winrt::AnnotatedScrollBarLabel>>();
    m_labelsVectorChangedRevoker = observableVector.VectorChanged(winrt::auto_revoke,{ this, &AnnotatedScrollBar::OnLabelsCollectionChanged });

    SmallChange(0);
}

AnnotatedScrollBar::~AnnotatedScrollBar()
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookHandlers(true /*isFromDestructor*/);
}

winrt::IScrollController AnnotatedScrollBar::ScrollController()
{
    return static_cast<winrt::IScrollController>(*this);
}

void AnnotatedScrollBar::ViewportSize(double viewportSize)
{
    if (viewportSize != m_viewportSize)
    {
        m_viewportSize = viewportSize;
        UpdatePositionsOfAnchoredElements();
    }
}

double AnnotatedScrollBar::ViewportSize()
{
    return m_viewportSize;
}

void AnnotatedScrollBar::Maximum(double maximum)
{
    if (maximum != m_maximum)
    {
        m_maximum = maximum;
        UpdatePositionsOfAnchoredElements();
    }
}

double AnnotatedScrollBar::Maximum()
{
    return m_maximum;
}

void AnnotatedScrollBar::Minimum(double minimum)
{
    if (minimum != m_minimum)
    {
        m_minimum = minimum;
        UpdatePositionsOfAnchoredElements();
    }
}

double AnnotatedScrollBar::Minimum()
{
    return m_minimum;
}

void AnnotatedScrollBar::Value(double value)
{
    if (value != m_value)
    {
        m_value = value;
    }
}

double AnnotatedScrollBar::Value()
{
    return m_value;
}

void AnnotatedScrollBar::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_LabelTemplateProperty)
    {
        QueueLayoutLabels(50 /*millisecondWait*/);
    }
    else if (property == s_DetailLabelTemplateProperty)
    {
        m_detailLabelTemplateApplied = false;
    }
    else if (property == s_LabelsProperty)
    {
        if (auto labelsGrid = m_labelsGrid.get())
        {
            labelsGrid.Children().Clear();
        }
        m_labelSizes.clear();
        QueueLayoutLabels(50 /*millisecondWait*/);
    }
}

void AnnotatedScrollBar::OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdatePositionsOfAnchoredElements();
}

#ifdef DBG
void AnnotatedScrollBar::OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);
}
#endif

void AnnotatedScrollBar::OnIsEnabledChanged(const winrt::IInspectable& sender, const winrt::DependencyPropertyChangedEventArgs& args)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!IsEnabled())
    {
        m_isPressed = false;
        m_isPointerOver = false;

        if (m_capturedPointer)
        {
            if (auto verticalGrid = m_verticalGrid.get())
            {
                verticalGrid.ReleasePointerCapture(m_capturedPointer);
                m_capturedPointer = nullptr;
            }
        }
        ResetTrackedPointerId();
    }
    UpdateCanScroll();
}

void AnnotatedScrollBar::OnLabelsCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    if (auto labelsGrid = m_labelsGrid.get())
    {
        labelsGrid.Children().Clear();
    }
    m_labelSizes.clear();
    QueueLayoutLabels(50 /*millisecondWait*/);
}

void AnnotatedScrollBar::OnApplyTemplate()
{
    UnhookHandlers(false /*isFromDestructor*/);

    __super::OnApplyTemplate();

    winrt::IControlProtected controlProtected = *this;

    m_verticalThumb.set(GetTemplateChildT<winrt::Border>(s_verticalThumbPartName, controlProtected));
    m_verticalThumbGhost.set(GetTemplateChildT<winrt::Border>(s_verticalThumbGhostPartName, controlProtected));
    m_verticalDecrementRepeatButton.set(GetTemplateChildT<winrt::RepeatButton>(s_verticalDecrementRepeatButtonPartName, controlProtected));
    m_verticalIncrementRepeatButton.set(GetTemplateChildT<winrt::RepeatButton>(s_verticalIncrementRepeatButtonPartName, controlProtected));
    m_verticalGrid.set(GetTemplateChildT<winrt::Grid>(s_verticalGridPartName, controlProtected));
    m_labelsGrid.set(GetTemplateChildT<winrt::Grid>(s_labelsGridPartName, controlProtected));
    m_tooltipContentPresenter.set(GetTemplateChildT<winrt::ContentPresenter>(s_tooltipContentPresenterPartName, controlProtected));
    m_detailLabelToolTip.set(GetTemplateChildT<winrt::ToolTip>(s_detailLabelToolTipPartName, controlProtected));

    SetUpInteractionElements();
    HookInputEvents();
}

void AnnotatedScrollBar::UnhookHandlers(bool isFromDestructor)
{
    m_verticalGridPointerEnteredRevoker.revoke();
    m_verticalGridPointerMovedRevoker.revoke();
    m_verticalGridPointerExitedRevoker.revoke();
    m_verticalGridPointerPressedRevoker.revoke();
    m_verticalGridPointerReleasedRevoker.revoke();
    m_verticalGridPointerCanceledRevoker.revoke();
    m_verticalGridPointerCaptureLostRevoker.revoke();
    m_verticalIncrementRepeatButtonClickRevoker.revoke();
    m_verticalDecrementRepeatButtonClickRevoker.revoke();
    m_detailLabelToolTipOpenedRevoker.revoke();

    if (isFromDestructor)
    {
        m_sizeChangedRevoker.revoke();
#ifdef DBG
        m_loadedRevoker.revoke();
#endif
        m_isEnabledRevoker.revoke();
        m_labelsVectorChangedRevoker.revoke();
    }
}

 void AnnotatedScrollBar::HookInputEvents()
{
    if (auto const verticalGrid = m_verticalGrid.get())
    {
        m_verticalGridPointerEnteredRevoker = verticalGrid.PointerEntered(winrt::auto_revoke, { this, &AnnotatedScrollBar::OnVerticalGridPointerEntered });
        m_verticalGridPointerMovedRevoker = verticalGrid.PointerMoved(winrt::auto_revoke, { this, &AnnotatedScrollBar::OnVerticalGridPointerMoved });
        m_verticalGridPointerPressedRevoker = verticalGrid.PointerPressed(winrt::auto_revoke, { this, &AnnotatedScrollBar::OnVerticalGridPointerPressed });

        m_verticalGridPointerExitedRevoker = AddRoutedEventHandler<RoutedEventType::PointerExited>(
        *this,
        { this, &AnnotatedScrollBar::OnVerticalGridPointerExited },
        true /*handledEventsToo*/);
        m_verticalGridPointerReleasedRevoker = AddRoutedEventHandler<RoutedEventType::PointerReleased>(
        *this,
        { this, &AnnotatedScrollBar::OnVerticalGridPointerReleased },
        true /*handledEventsToo*/);
        m_verticalGridPointerCanceledRevoker = AddRoutedEventHandler<RoutedEventType::PointerCanceled>(
        *this,
        { this, &AnnotatedScrollBar::OnVerticalGridPointerCanceled },
        true /*handledEventsToo*/);
        m_verticalGridPointerCaptureLostRevoker = AddRoutedEventHandler<RoutedEventType::PointerCaptureLost>(
        *this,
        { this, &AnnotatedScrollBar::OnVerticalGridPointerCaptureLost },
        true /*handledEventsToo*/);
    }

    if (auto detailLabelToolTip = m_detailLabelToolTip.get())
    {
        m_detailLabelToolTipOpenedRevoker = detailLabelToolTip.Opened(winrt::auto_revoke, { this, &AnnotatedScrollBar::OnDetailLabelToolTipOpened });
    }
}

void AnnotatedScrollBar::SetUpInteractionElements()
{
    if (auto verticalIncrementRepeatButton = m_verticalIncrementRepeatButton.get())
    {
        m_verticalIncrementRepeatButtonClickRevoker = verticalIncrementRepeatButton.Click(winrt::auto_revoke, { this, &AnnotatedScrollBar::OnIncrementRepeatButtonClick });
    }

    if (auto verticalDecrementRepeatButton = m_verticalDecrementRepeatButton.get())
    {
        m_verticalDecrementRepeatButtonClickRevoker = verticalDecrementRepeatButton.Click(winrt::auto_revoke, { this, &AnnotatedScrollBar::OnDecrementRepeatButtonClick });
    }

    EnsurePanningInfo();

    m_panningInfo.get()->PanningFrameworkElement(m_verticalThumb.get());
    m_panningInfo.get()->PanningElementAncestor(m_verticalGrid.get());
}

// This function updates components that rely on Minimum, Maximum, ViewportSize & LabelsGrid.
void AnnotatedScrollBar::UpdatePositionsOfAnchoredElements()
{
    ANNOTATEDSCROLLBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    // Run the math for the mapping factor again
    UpdateScrollOffsetToLabelOffsetFactor();

    // Update thumb animation multiplier
    EnsurePanningInfo();

    m_panningInfo.get()->PanningElementOffsetMultiplier(static_cast<float>(-1.0 / m_scrollOffsetToLabelOffsetFactor));

    QueueLayoutLabels(500 /*millisecondWait*/);
}

void AnnotatedScrollBar::QueueLayoutLabels(unsigned int millisecondWait)
{
    ANNOTATEDSCROLLBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);
    
    if (!m_labelsDebounce.test_and_set())
    {
        auto strongThis = get_strong(); // ensure object lifetime during coroutines
        auto runLayoutLabelsAction = [&, strongThis]()
        {
            strongThis->m_labelsDebounce.clear();
            strongThis->LayoutLabels();
        };

        SharedHelpers::ScheduleActionAfterWait(runLayoutLabelsAction, millisecondWait);
    }
}

void AnnotatedScrollBar::LayoutLabels()
{
    auto labelsGrid = m_labelsGrid.get();
    if (!labelsGrid)
    {
        return;
    }
    
    if (labelsGrid.Children().Size() == 0)
    {
        CreateLabelContainers();
    }
    else
    {
        UpdateLabelContainersOffsets();
    }

    CollapseCollidingAndOutOfBoundsLabels();
}

void AnnotatedScrollBar::CreateLabelContainers()
{
    auto labelsGrid = m_labelsGrid.get();
    MUX_ASSERT(labelsGrid);
    MUX_ASSERT(m_labelSizes.size() == 0);

    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    for (auto label : Labels())
    {
        auto labelContentPresenter = winrt::ContentPresenter();
        labelContentPresenter.HorizontalAlignment(winrt::HorizontalAlignment::Right);
        labelContentPresenter.Content(label);

        if (auto dataElementFactory = LabelTemplate())
        {
            if (auto dataTemplate = dataElementFactory.try_as<winrt::DataTemplate>())
            {
                labelContentPresenter.ContentTemplate(dataTemplate);
            }
            else if (auto dataTemplateSelector = dataElementFactory.try_as<winrt::DataTemplateSelector>())
            {
                labelContentPresenter.ContentTemplateSelector(dataTemplateSelector);
            }
        }

        float const labelHeight = LayoutUtils::MeasureAndGetDesiredHeightFor(labelContentPresenter, c_infSize);
        m_labelSizes.push_back(labelHeight);

        auto const labelVerticalOffset = GetLabelVerticalOffset(label);
        labelContentPresenter.Margin({ 0,labelVerticalOffset,0,0 });
        labelsGrid.Children().Append(labelContentPresenter);
    }
}

void AnnotatedScrollBar::UpdateLabelContainersOffsets()
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    auto labelsGrid = m_labelsGrid.get();
    MUX_ASSERT(labelsGrid);

    for (auto const labelContainer : labelsGrid.Children())
    {
        auto const labelContentPresenter = labelContainer.as<winrt::ContentPresenter>();
        auto const label = labelContentPresenter.Content().as<winrt::AnnotatedScrollBarLabel>();
        auto const labelVerticalOffset = GetLabelVerticalOffset(label);
        labelContentPresenter.Margin({ 0,labelVerticalOffset,0,0 });
    }
}

void AnnotatedScrollBar::CollapseCollidingAndOutOfBoundsLabels()
{
    auto const labelsGrid = m_labelsGrid.get();
    MUX_ASSERT(labelsGrid);

    auto const labelsGridHeight = labelsGrid.ActualSize().y;
    auto const labelsSize = Labels().Size();
    if (labelsSize == 0 ||
        labelsGridHeight == 0)
    {
        return;
    }

    MUX_ASSERT(labelsSize == labelsGrid.Children().Size());
    MUX_ASSERT(labelsSize == m_labelSizes.size());

    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    // Collapse colliding or out of bounds labels
    double previousLabelTopPosition = -1;
    int previousLabelIndex = (int)labelsGrid.Children().Size();
    for (int currentLabelIndex = previousLabelIndex - 1; currentLabelIndex >= 0; currentLabelIndex--)
    {
       auto const currentLabel = labelsGrid.Children().GetAt(currentLabelIndex).as<winrt::FrameworkElement>();
       auto const currentLabelTopPosition = currentLabel.Margin().Top;
       double const currentLabelBottomPosition = currentLabelTopPosition + m_labelSizes[currentLabelIndex];

       auto const isLabelOutOfBounds = currentLabelTopPosition < 0 ||
                                       currentLabelBottomPosition > labelsGridHeight;

       auto const isLabelColliding = previousLabelTopPosition >= 0 &&
                                     previousLabelTopPosition <= currentLabelBottomPosition;

       if (isLabelOutOfBounds)
       {
           currentLabel.Visibility(winrt::Visibility::Collapsed);
       }
       else if (isLabelColliding)
       {
           // The bottom item displayed should always stay shown in any collision.
           // The first item should also be always displayed with an exception
           // in the case that it is colliding with the last item.
           int indexToCollapse = currentLabelIndex;
           if (currentLabelIndex == 0 &&
               previousLabelIndex < (int)labelsGrid.Children().Size() - 1)
           {
               indexToCollapse = previousLabelIndex;
           }

           auto const labelToCollapse = labelsGrid.Children().GetAt(indexToCollapse).as<winrt::FrameworkElement>();
           labelToCollapse.Visibility(winrt::Visibility::Collapsed);
       }
       else
       {
           currentLabel.Visibility(winrt::Visibility::Visible);
           previousLabelTopPosition = currentLabelTopPosition;
           previousLabelIndex = currentLabelIndex;
       }
    }
}

double AnnotatedScrollBar::GetLabelVerticalOffset(winrt::AnnotatedScrollBarLabel label)
{
    return label.ScrollOffset() * m_scrollOffsetToLabelOffsetFactor;
}

void AnnotatedScrollBar::UpdateScrollOffsetToLabelOffsetFactor()
{
    double scrollOffsetToLabelOffsetFactor = 1;
    auto labelsGrid = m_labelsGrid.get();
    auto interactionFrameworkElement = m_verticalThumb.get();

    if (labelsGrid && interactionFrameworkElement)
    {
        double scrollViewScrollableHeight = Maximum() - Minimum();
        scrollViewScrollableHeight = scrollViewScrollableHeight == 0 ? 1 : scrollViewScrollableHeight;
        scrollOffsetToLabelOffsetFactor = (labelsGrid.ActualHeight() - interactionFrameworkElement.ActualHeight()) / scrollViewScrollableHeight * (1 - ViewportSize() / (ViewportSize() + scrollViewScrollableHeight));
    }
    m_scrollOffsetToLabelOffsetFactor = scrollOffsetToLabelOffsetFactor;
}

void AnnotatedScrollBar::EnsurePanningInfo()
{
    if (!m_panningInfo)
    {
        m_panningInfo.set(winrt::make_self<AnnotatedScrollBarPanningInfo>());
    }
}

void AnnotatedScrollBar::EnsureSmallChangeValue()
{
    if (SmallChange() == 0.0)
    {
       SmallChange(ViewportSize()/s_defaultViewportToSmallChangeRatio);
    }
}

void AnnotatedScrollBar::SetThumbGhostVisibility(winrt::Visibility visibility)
{
    if (auto verticalThumbGhost = m_verticalThumbGhost.get())
    {
        verticalThumbGhost.Visibility(visibility);
    }
}

void AnnotatedScrollBar::ShowHoverVisuals(double offsetFromTopToolTip, double offsetFromTopThumbGhost)
{    
    ShowToolTipAtOffset(offsetFromTopToolTip);
    ShowThumbGhostAtOffset(offsetFromTopThumbGhost);
}

void AnnotatedScrollBar::HideHoverVisuals()
{
    SetToolTipIsOpen(false);
    SetThumbGhostVisibility(winrt::Visibility::Collapsed);
}

void AnnotatedScrollBar::SetToolTipIsOpen(bool isOpen)
{
    if (!m_detailLabelRequestedEventSource)
    {
        return;
    }

    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto detailLabelToolTip = m_detailLabelToolTip.get())
    {
        detailLabelToolTip.IsOpen(isOpen);
    }
}

void AnnotatedScrollBar::ShowToolTipAtOffset(double offsetFromTop)
{
    auto data = RaiseDetailLabelRequested(offsetFromTop);

    auto detailLabelToolTip = m_detailLabelToolTip.get();
    auto tooltipContentPresenter = m_tooltipContentPresenter.get();
    if (data && 
        detailLabelToolTip &&
        tooltipContentPresenter)
    {
        tooltipContentPresenter.Content(data);

        if (!m_detailLabelTemplateApplied || 
            (!tooltipContentPresenter.ContentTemplate() && !tooltipContentPresenter.ContentTemplateSelector()))
        {
            if (auto dataElementFactory = DetailLabelTemplate())
            {
                if (auto dataTemplate = dataElementFactory.try_as<winrt::DataTemplate>())
                {
                    tooltipContentPresenter.ContentTemplateSelector(nullptr);
                    tooltipContentPresenter.ContentTemplate(dataTemplate);
                    m_detailLabelTemplateApplied = true;
                }
                else if (auto dataTemplateSelector = dataElementFactory.try_as<winrt::DataTemplateSelector>())
                {
                    tooltipContentPresenter.ContentTemplate(nullptr);
                    tooltipContentPresenter.ContentTemplateSelector(dataTemplateSelector);
                    m_detailLabelTemplateApplied = true;
                }
            }
        }

        auto const detailLabelToolTipWidth = LayoutUtils::MeasureAndGetDesiredWidthFor(tooltipContentPresenter, c_infSize);
        // +2 extra pixels to offset the tooltip border for proper alignment
        auto const horizontalPosition = static_cast<float>((-1*detailLabelToolTipWidth/2) + 2);
        const winrt::Rect placementRect(horizontalPosition,
                                        static_cast<float>(offsetFromTop),
                                        0, 
                                        0);
        detailLabelToolTip.PlacementRect(placementRect);
        SetToolTipIsOpen(true);
    }
}

void AnnotatedScrollBar::ShowThumbGhostAtOffset(double offsetFromTop)
{
    if (auto verticalThumbGhost = m_verticalThumbGhost.get())
    {
        verticalThumbGhost.Margin({verticalThumbGhost.Margin().Left, 
                                    offsetFromTop, 
                                    verticalThumbGhost.Margin().Right, 
                                    verticalThumbGhost.Margin().Bottom});
        SetThumbGhostVisibility(winrt::Visibility::Visible);
    }
}

void AnnotatedScrollBar::UpdateCanScroll()
{
    const bool oldCanScroll = m_canScroll;

    m_canScroll = m_isScrollable && IsEnabled();

    if (oldCanScroll != m_canScroll)
    {
        RaiseCanScrollChanged();
    }
}

void AnnotatedScrollBar::OnDetailLabelToolTipOpened(const winrt::IInspectable& sender,  const winrt::RoutedEventArgs& args)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);
    // Circumvent tooltip opening after hover delay when it is not in use
    if (!m_detailLabelRequestedEventSource)
    {
        SetToolTipIsOpen(false);
    }
}

void AnnotatedScrollBar::OnIncrementRepeatButtonClick(const winrt::IInspectable& sender,  const winrt::RoutedEventArgs& args)
{
    // We pass in 0 for offset here because it is determined in ScrollTo()
    ScrollTo(0, winrt::AnnotatedScrollBarScrollingEventKind::IncrementButton);
}

void AnnotatedScrollBar::OnDecrementRepeatButtonClick(const winrt::IInspectable& sender,  const winrt::RoutedEventArgs& args)
{
    // We pass in 0 for offset here because it is determined in ScrollTo()
    ScrollTo(0, winrt::AnnotatedScrollBarScrollingEventKind::DecrementButton);
}

void AnnotatedScrollBar::ResetTrackedPointerId()
{
    m_verticalGridTrackedPointerId = 0;
}

bool AnnotatedScrollBar::IgnorePointerId(const winrt::PointerRoutedEventArgs& args)
{
    uint32_t pointerId = args.Pointer().PointerId();

    if (m_verticalGridTrackedPointerId == 0)
    {
        m_verticalGridTrackedPointerId = pointerId;
    }
    else if (m_verticalGridTrackedPointerId != pointerId)
    {
        return true;
    }
    return false;
}

void AnnotatedScrollBar::OnVerticalGridPointerEntered(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    m_isPointerOver = true;

    MUX_ASSERT(m_verticalGrid.get() != nullptr);

    const auto offsetFromTop = args.GetCurrentPoint(m_verticalGrid.get()).Position().Y;
    auto offsetFromTopToolTip = offsetFromTop;
    const auto offsetFromTopThumbGhost = offsetFromTop;

    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offsetFromTop);

    auto const& pointerDeviceType = args.Pointer().PointerDeviceType();
    if (pointerDeviceType == winrt::PointerDeviceType::Touch)
    {
        offsetFromTopToolTip -= s_toolTipTouchOffset;
    }

    ShowHoverVisuals(offsetFromTopToolTip, offsetFromTopThumbGhost);
}

void AnnotatedScrollBar::OnVerticalGridPointerMoved(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    m_isPointerOver = true;

    auto verticalGrid = m_verticalGrid.get();
    MUX_ASSERT(verticalGrid != nullptr);

    auto offsetFromTop = args.GetCurrentPoint(verticalGrid).Position().Y;

    offsetFromTop = std::max(0.0f, offsetFromTop);
    offsetFromTop = std::min((float)verticalGrid.ActualHeight(), offsetFromTop);

    auto offsetFromTopToolTip = offsetFromTop;
    const auto offsetFromTopThumbGhost = offsetFromTop;

    ANNOTATEDSCROLLBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offsetFromTop);

    // If pointer hasn't moved we do nothing
    auto detailLabelToolTip = m_detailLabelToolTip.get();
    if (detailLabelToolTip && detailLabelToolTip.IsOpen())
    {
         if (offsetFromTop == m_lastVerticalGridPointerMovedOffset)
         {
             return;
         }
    }
    m_lastVerticalGridPointerMovedOffset = offsetFromTop;

    auto const& pointerDeviceType = args.Pointer().PointerDeviceType();
    if (pointerDeviceType == winrt::PointerDeviceType::Touch)
    {
        offsetFromTopToolTip -= s_toolTipTouchOffset;
    }

    ShowHoverVisuals(offsetFromTopToolTip, offsetFromTopThumbGhost);

    if (m_isPressed || m_capturedPointer)
    {
        double const newScrollOffset = offsetFromTop / m_scrollOffsetToLabelOffsetFactor;
        ScrollTo(newScrollOffset, winrt::AnnotatedScrollBarScrollingEventKind::Drag);
    }
}

void AnnotatedScrollBar::OnVerticalGridPointerExited(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    const bool isTouch = args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Touch;
    if (isTouch && !m_capturedPointer)
    {
        // The hiding of elements for non-mouse input events is handled in OnVerticalGridPointerReleased.
        // The PointerReleased and PointerExited events can happen in any order so we need to make sure to reset the tracked pointer id here as well.
        ResetTrackedPointerId();
    }
    if (IsOutOfVerticalGridBounds(args.GetCurrentPoint(m_verticalGrid.get()).Position()) && !isTouch)
    {
        // We do the vertical bounds check because PointerExited can sometimes happen when the mouse has hovered over the open tooltip.
        // In these cases we need to test if the pointer is over the vertical grid to maintain the proper state.
        m_isPressed = false;
        m_isPointerOver = false;

        if (!m_capturedPointer)
        {
            ResetTrackedPointerId();
        }

        HideHoverVisuals();
    }
}

void AnnotatedScrollBar::OnVerticalGridPointerPressed(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    m_isPressed = true;

    const double newScrollOffset = args.GetCurrentPoint(m_verticalGrid.get()).Position().Y / m_scrollOffsetToLabelOffsetFactor;

    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, newScrollOffset);

    ScrollTo(newScrollOffset, winrt::AnnotatedScrollBarScrollingEventKind::Click);

    auto pointer = args.Pointer();
    if (auto verticalGrid = m_verticalGrid.get())
    {
        if (verticalGrid.CapturePointer(pointer))
        {
            m_capturedPointer = pointer;
        }
    }

    switch (pointer.PointerDeviceType())
    {
        case winrt::PointerDeviceType::Touch:
        case winrt::PointerDeviceType::Pen:
        {
            if (m_panningInfo)
            {
                // Attempt an UI-thread-independent pan.
                //TODO as part of bug fix 45076500. Start an independent pan.
                //m_panningInfo.get()->RaisePanRequested(args.GetCurrentPoint(nullptr));
            }
            break;
        }
        case winrt::PointerDeviceType::Mouse:
        {
            if (!m_isScrollingWithMouse)
            {
                m_isScrollingWithMouse = true;
                RaiseIsScrollingWithMouseChanged();
            }
            break;
        }
    }
}

void AnnotatedScrollBar::OnVerticalGridPointerReleased(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_isPressed = false;
    auto verticalGrid = m_verticalGrid.get();

    if (m_capturedPointer && verticalGrid)
    {
        verticalGrid.ReleasePointerCapture(m_capturedPointer);
        m_capturedPointer = nullptr;

        // OnVerticalGridPointerCaptureLost does not get called in the case of Touch.
        // In that case we want to make sure to hide the required elements here.
        auto const& pointerDeviceType = args.Pointer().PointerDeviceType();

        switch (pointerDeviceType)
        {
            case winrt::PointerDeviceType::Touch:
            {
                m_isPointerOver = false;
                HideHoverVisuals();
                ResetTrackedPointerId();
                break;
            }
            case winrt::PointerDeviceType::Mouse:
            {
                if (m_isScrollingWithMouse)
                {
                    m_isScrollingWithMouse = false;
                    RaiseIsScrollingWithMouseChanged();
                }
                break;
            }
        }
    }
}

void AnnotatedScrollBar::OnVerticalGridPointerCanceled(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ProcessVerticalGridPointerCanceled(args);
}

void AnnotatedScrollBar::OnVerticalGridPointerCaptureLost(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ProcessVerticalGridPointerCanceled(args);
}

void AnnotatedScrollBar::ProcessVerticalGridPointerCanceled(const winrt::PointerRoutedEventArgs& args)
{
    if (IgnorePointerId(args))
    {
        return;
    }

    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_isPressed = false;
    // We do this check because PointerCaptureLost can sometimes take the place of PointerReleased events.
    // In these cases we need to test if the pointer is over the vertical grid to maintain the proper state.
    if (IsOutOfVerticalGridBounds(args.GetCurrentPoint(m_verticalGrid.get()).Position()))
    {
        m_isPointerOver = false;
        HideHoverVisuals();
    }

    if (m_isScrollingWithMouse && args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Mouse)
    {
        m_isScrollingWithMouse = false;
        RaiseIsScrollingWithMouseChanged();
    }

    m_capturedPointer = nullptr;
    ResetTrackedPointerId();
}

bool AnnotatedScrollBar::IsOutOfVerticalGridBounds(const winrt::Point& point) {

    if (auto verticalGrid = m_verticalGrid.get())
    {
        // This is a conservative check. It is okay to say we are
        // out of the bounds when close to the edge to account for rounding.
        const auto tolerance = 1.0;
        const auto actualWidth = verticalGrid.ActualWidth();
        const auto actualHeight = verticalGrid.ActualHeight();
        return point.X < tolerance ||
            point.X > actualWidth - tolerance ||
            point.Y < tolerance ||
            point.Y > actualHeight - tolerance;
    }
    return true;
}

void AnnotatedScrollBar::ScrollTo(double offset, winrt::AnnotatedScrollBarScrollingEventKind scrollingEventKind)
{
    ANNOTATEDSCROLLBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_INT, METH_NAME, this, offset, scrollingEventKind);

    auto scrollOffset = offset;
    // In the case of increment buttons, calculate the new offset
    double changeAmount = 0;

    switch (scrollingEventKind)
    {
        case winrt::AnnotatedScrollBarScrollingEventKind::IncrementButton:
        {
            EnsureSmallChangeValue();
            changeAmount = -1 * SmallChange();
            scrollOffset = Value() + changeAmount;
            scrollOffset = std::max(scrollOffset, Minimum());
            break;
        }
        case winrt::AnnotatedScrollBarScrollingEventKind::DecrementButton:
        {
            EnsureSmallChangeValue();
            changeAmount = SmallChange();
            scrollOffset = Value() + changeAmount;
            scrollOffset = std::min(scrollOffset, Maximum());
            break;
        }
    }

    if (RaiseScrolling(scrollOffset, scrollingEventKind))
    {
        return;
    }

    switch (scrollingEventKind)
    {
        case winrt::AnnotatedScrollBarScrollingEventKind::Click:
        {
            RaiseScrollToRequested(scrollOffset);
            break;
        }
        case winrt::AnnotatedScrollBarScrollingEventKind::Drag:
        {
            // TODO as part of bug fix 45076500.
            // RaiseScrollToRequested must not be called during a touch-driven independent pan.
            //if (m_isScrollingWithMouse)
            {
                RaiseScrollToRequested(scrollOffset);
            }
            break;
        }
        case winrt::AnnotatedScrollBarScrollingEventKind::IncrementButton:
        case winrt::AnnotatedScrollBarScrollingEventKind::DecrementButton:
        {
            if (SharedHelpers::IsAnimationsEnabled())
            {
                RaiseAddScrollVelocityRequested(changeAmount);
            }
            else
            {
                RaiseScrollByRequested(changeAmount);
            }
            break;
        }
    }
}

bool AnnotatedScrollBar::RaiseScrolling(double offset, winrt::AnnotatedScrollBarScrollingEventKind scrollingEventKind)
{
    ANNOTATEDSCROLLBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_INT, METH_NAME, this, offset, scrollingEventKind);

    if (!m_scrollingEventSource)
    {
        return false;
    }

    auto scrollingEventArgs = winrt::make_self<AnnotatedScrollBarScrollingEventArgs>(offset, scrollingEventKind);

    m_scrollingEventSource(*this, *scrollingEventArgs);

    return scrollingEventArgs.as<winrt::AnnotatedScrollBarScrollingEventArgs>().Cancel();
}

winrt::IInspectable AnnotatedScrollBar::RaiseDetailLabelRequested(double offsetFromTop)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offsetFromTop);

    if (!m_detailLabelRequestedEventSource)
    {
        return nullptr;
    }

    const auto value = std::min(offsetFromTop / m_scrollOffsetToLabelOffsetFactor, Maximum() + ViewportSize());
    auto detailLabelRequestedEventArgs = winrt::make_self<AnnotatedScrollBarDetailLabelRequestedEventArgs>(value);

    m_detailLabelRequestedEventSource(*this, *detailLabelRequestedEventArgs);
    return detailLabelRequestedEventArgs.as<winrt::AnnotatedScrollBarDetailLabelRequestedEventArgs>().Content();
}

void AnnotatedScrollBar::RaiseCanScrollChanged()
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!m_canScrollChanged)
    {
        return;
    }

    m_canScrollChanged(*this, nullptr);
}

void AnnotatedScrollBar::RaiseIsScrollingWithMouseChanged()
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!m_isScrollingWithMouseChanged)
    {
        return;
    }

    m_isScrollingWithMouseChanged(*this, nullptr);
}

void AnnotatedScrollBar::RaiseScrollToRequested(double offset)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offset);

    if (!m_scrollToRequested)
    {
        return;
    }

    auto options = winrt::make_self<ScrollingScrollOptions>(
        winrt::ScrollingAnimationMode::Disabled,
        winrt::ScrollingSnapPointsMode::Ignore);

    auto scrollToRequestedEventArgs = winrt::make_self<ScrollControllerScrollToRequestedEventArgs>(
        offset,
        *options);

    m_scrollToRequested(*this, *scrollToRequestedEventArgs);

    int32_t offsetChangeCorrelationId = scrollToRequestedEventArgs.as<winrt::ScrollControllerScrollToRequestedEventArgs>().CorrelationId();

    // Only increment m_operationsCount when the returned OffsetsChangeCorrelationId represents a new request that was not coalesced with a pending request. 
    if (offsetChangeCorrelationId != -1 && offsetChangeCorrelationId != m_lastOffsetChangeCorrelationIdForScrollTo)
    {
        m_lastOffsetChangeCorrelationIdForScrollTo = offsetChangeCorrelationId;
        m_operationsCount++;
    }
}

void AnnotatedScrollBar::RaiseScrollByRequested(double offsetDelta)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offsetDelta);

    if (!m_scrollByRequested)
    {
        return;
    }

    auto options = winrt::make_self<ScrollingScrollOptions>(
        winrt::ScrollingAnimationMode::Disabled,
        winrt::ScrollingSnapPointsMode::Ignore);

    auto scrollByRequestedEventArgs = winrt::make_self<ScrollControllerScrollByRequestedEventArgs>(
        offsetDelta,
        *options);

    m_scrollByRequested(*this, *scrollByRequestedEventArgs);

    int32_t offsetChangeCorrelationId = scrollByRequestedEventArgs.as<winrt::ScrollControllerScrollByRequestedEventArgs>().CorrelationId();

    // Only increment m_operationsCount when the returned OffsetsChangeCorrelationId represents a new request that was not coalesced with a pending request. 
    if (offsetChangeCorrelationId != -1 && offsetChangeCorrelationId != m_lastOffsetChangeCorrelationIdForScrollBy)
    {
        m_lastOffsetChangeCorrelationIdForScrollBy = offsetChangeCorrelationId;
        m_operationsCount++;
    }
}

void AnnotatedScrollBar::RaiseAddScrollVelocityRequested(double offsetDelta)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offsetDelta);

    if (!m_addScrollVelocityRequested)
    {
        return;
    }

    double const offsetVelocity = offsetDelta * s_velocityNeededPerPixel;

    auto addScrollVelocityRequestedEventArgs = winrt::make_self<ScrollControllerAddScrollVelocityRequestedEventArgs>(
        static_cast<float>(offsetVelocity),
        s_smallChangeInertiaDecayRate);

    m_addScrollVelocityRequested(*this, *addScrollVelocityRequestedEventArgs);

    int32_t offsetChangeCorrelationId = addScrollVelocityRequestedEventArgs.as<winrt::ScrollControllerAddScrollVelocityRequestedEventArgs>().CorrelationId();

    // Only increment m_operationsCount when the returned OffsetsChangeCorrelationId represents a new request that was not coalesced with a pending request. 
    if (offsetChangeCorrelationId != -1 && offsetChangeCorrelationId != m_lastOffsetChangeCorrelationIdForAddScrollVelocity)
    {
        m_lastOffsetChangeCorrelationIdForAddScrollVelocity = offsetChangeCorrelationId;
        m_operationsCount++;
    }
}


#pragma region IScrollController

winrt::IScrollControllerPanningInfo AnnotatedScrollBar::PanningInfo()
{
    EnsurePanningInfo();

    return m_panningInfo.try_as<winrt::IScrollControllerPanningInfo>();
}

bool AnnotatedScrollBar::CanScroll()
{
    return m_canScroll;
}

bool AnnotatedScrollBar::IsScrollingWithMouse()
{
    return m_isScrollingWithMouse;
}

void AnnotatedScrollBar::SetIsScrollable(
    bool isScrollable)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(
        *this,
        TRACE_MSG_METH_INT,
        METH_NAME,
        this,
        isScrollable);

    m_isScrollable = isScrollable;
    UpdateCanScroll();
}

void AnnotatedScrollBar::SetValues(
    double minOffset,
    double maxOffset,
    double offset,
    double viewportLength)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(
        *this,
        L"%s[0x%p](minOffset:%lf, maxOffset:%lf, offset:%lf, viewportLength:%lf, operationsCount:%d)\n",
        METH_NAME,
        this,
        minOffset,
        maxOffset,
        offset,
        viewportLength,
        m_operationsCount);

    if (maxOffset < minOffset)
    {
        throw winrt::hresult_invalid_argument(L"maxOffset cannot be smaller than minOffset.");
    }

    if (viewportLength < 0.0)
    {
        throw winrt::hresult_invalid_argument(L"viewportLength cannot be negative.");
    }

    offset = std::max(minOffset, offset);
    offset = std::min(maxOffset, offset);
    m_lastOffset = offset;

    Minimum(minOffset);
    Maximum(maxOffset);
    ViewportSize(viewportLength);

    if (m_operationsCount == 0)
    {
        Value(offset);
    }
}

winrt::CompositionAnimation AnnotatedScrollBar::GetScrollAnimation(
    int correlationId,
    winrt::float2 const& startPosition,
    winrt::float2 const& endPosition,
    winrt::CompositionAnimation const& defaultAnimation)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, correlationId);

    // Using the consumer's default animation.
    return defaultAnimation;
}

void AnnotatedScrollBar::NotifyRequestedScrollCompleted(
    int correlationId)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(
        *this,
        TRACE_MSG_METH_INT,
        METH_NAME,
        this,
        correlationId);

    MUX_ASSERT(m_operationsCount > 0);
    m_operationsCount--;

    if (m_operationsCount == 0 && Value() != m_lastOffset)
    {
        Value(m_lastOffset);
    }
}

winrt::event_token AnnotatedScrollBar::ScrollToRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollToRequestedEventArgs> const& value)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    return m_scrollToRequested.add(value);
}

void AnnotatedScrollBar::ScrollToRequested(winrt::event_token const& token)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_scrollToRequested.remove(token);
}

winrt::event_token AnnotatedScrollBar::ScrollByRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollByRequestedEventArgs> const& value)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    return m_scrollByRequested.add(value);
}

void AnnotatedScrollBar::ScrollByRequested(winrt::event_token const& token)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_scrollByRequested.remove(token);
}

winrt::event_token AnnotatedScrollBar::AddScrollVelocityRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerAddScrollVelocityRequestedEventArgs> const& value)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    return m_addScrollVelocityRequested.add(value);
}

void AnnotatedScrollBar::AddScrollVelocityRequested(winrt::event_token const& token)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_addScrollVelocityRequested.remove(token);
}

winrt::event_token AnnotatedScrollBar::CanScrollChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    return m_canScrollChanged.add(value);
}

void AnnotatedScrollBar::CanScrollChanged(winrt::event_token const& token)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_canScrollChanged.remove(token);
}

winrt::event_token AnnotatedScrollBar::IsScrollingWithMouseChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    return m_isScrollingWithMouseChanged.add(value);
}

void AnnotatedScrollBar::IsScrollingWithMouseChanged(winrt::event_token const& token)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_isScrollingWithMouseChanged.remove(token);
}

#pragma endregion

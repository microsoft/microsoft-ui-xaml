// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "Scroller.h"
#include "RuntimeProfiler.h"
#include "FocusHelper.h"
#include "ScrollerViewTestHooks.h"

// Change to 'true' to turn on debugging outputs in Output window
bool ScrollerViewTrace::s_IsDebugOutputEnabled{ false };
bool ScrollerViewTrace::s_IsVerboseDebugOutputEnabled{ false };

ScrollerView::ScrollerView()
{
    SCROLLERVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    EnsureProperties();
    SetDefaultStyleKey(this);
    HookScrollerViewEvents();
}

ScrollerView::~ScrollerView()
{
    SCROLLERVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollerEvents(true /*isForDestructor*/);
    UnhookScrollerViewEvents();
    StopHideIndicatorsTimer(true /*isForDestructor*/);
}

#pragma region IScrollerView

winrt::CompositionPropertySet ScrollerView::ExpressionAnimationSources()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ExpressionAnimationSources();
    }

    return nullptr;
}

double ScrollerView::HorizontalOffset()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.HorizontalOffset();
    }

    return 0.0;
}

double ScrollerView::VerticalOffset()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.VerticalOffset();
    }

    return 0.0;
}

float ScrollerView::ZoomFactor()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ZoomFactor();
    }

    return 0.0f;
}

double ScrollerView::ExtentWidth()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ExtentWidth();
    }

    return 0.0;
}

double ScrollerView::ExtentHeight()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ExtentHeight();
    }

    return 0.0;
}

winrt::ScrollerState ScrollerView::State()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.State();
    }

    return winrt::ScrollerState::Idle;
}

winrt::ScrollerInputKind ScrollerView::InputKind()
{
    // Workaround for Bug 17377013: XamlCompiler codegen for Enum CreateFromString always returns boxed int which is wrong for [flags] enums (should be uint)
    // Check if the boxed InputKind is an IReference<int> first in which case we unbox as int.
    auto boxedKind = GetValue(s_InputKindProperty);
    if (auto boxedInt = boxedKind.try_as<winrt::IReference<int32_t>>())
    {
        return winrt::ScrollerInputKind{ static_cast<uint32_t>(unbox_value<int32_t>(boxedInt)) };
    }

    return auto_unbox(boxedKind);
}

void ScrollerView::InputKind(winrt::ScrollerInputKind const& value)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerInputKindToString(value).c_str());
    SetValue(s_InputKindProperty, box_value(value));
}

int32_t ScrollerView::ChangeOffsets(
    winrt::ScrollerChangeOffsetsOptions const& options)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerChangeOffsetsOptionsToString(options).c_str());

    if (auto scroller = m_scroller.get())
    {
        return scroller.ChangeOffsets(options);
    }

    return -1;
}

int32_t ScrollerView::ChangeOffsetsWithAdditionalVelocity(
    winrt::ScrollerChangeOffsetsWithAdditionalVelocityOptions const& options)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerChangeOffsetsWithAdditionalVelocityOptionsToString(options).c_str());

    if (auto scroller = m_scroller.get())
    {
        return scroller.ChangeOffsetsWithAdditionalVelocity(options);
    }

    return -1;
}

int32_t ScrollerView::ChangeZoomFactor(
    winrt::ScrollerChangeZoomFactorOptions const& options)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerChangeZoomFactorOptionsToString(options).c_str());

    if (auto scroller = m_scroller.get())
    {
        return scroller.ChangeZoomFactor(options);
    }

    return -1;
}

int32_t ScrollerView::ChangeZoomFactorWithAdditionalVelocity(
    winrt::ScrollerChangeZoomFactorWithAdditionalVelocityOptions const& options)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerChangeZoomFactorWithAdditionalVelocityOptionsToString(options).c_str());

    if (auto scroller = m_scroller.get())
    {
        return scroller.ChangeZoomFactorWithAdditionalVelocity(options);
    }

    return -1;
}

#pragma endregion

#pragma region IFrameworkElementOverrides

void ScrollerView::OnApplyTemplate()
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnApplyTemplate();

    m_keepIndicatorsShowing = false;

    winrt::IControlProtected thisAsControlProtected = *this;

    winrt::Scroller scroller = GetTemplateChildT<winrt::Scroller>(s_scrollerPartName, thisAsControlProtected);

    UpdateScroller(scroller);

    winrt::IScrollController horizontalScrollController = GetTemplateChildT<winrt::IScrollController>(s_horizontalScrollControllerPartName, thisAsControlProtected);

    UpdateHorizontalScrollController(horizontalScrollController);

    winrt::IScrollController verticalScrollController = GetTemplateChildT<winrt::IScrollController>(s_verticalScrollControllerPartName, thisAsControlProtected);

    UpdateVerticalScrollController(verticalScrollController);

    winrt::IUIElement scrollControllersSeparator = GetTemplateChildT<winrt::IUIElement>(s_scrollControllersSeparatorPartName, thisAsControlProtected);

    UpdateScrollControllersSeparator(scrollControllersSeparator);

    UpdateScrollControllersVisibility(true /*horizontalChange*/, true /*verticalChange*/);

    winrt::FrameworkElement root = GetTemplateChildT<winrt::FrameworkElement>(s_rootPartName, thisAsControlProtected);

    if (root)
    {        
        winrt::IVector<winrt::VisualStateGroup> rootVisualStateGroups = winrt::VisualStateManager::GetVisualStateGroups(root);

        if (rootVisualStateGroups)
        {
            uint32_t groupCount = rootVisualStateGroups.Size();

            for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
            {
                winrt::VisualStateGroup group = rootVisualStateGroups.GetAt(groupIndex);

                if (group)
                {
                    winrt::IVector<winrt::VisualState> visualStates = group.States();

                    if (visualStates)
                    {
                        uint32_t stateCount = visualStates.Size();

                        for (uint32_t stateIndex = 0; stateIndex < stateCount; ++stateIndex)
                        {
                            winrt::VisualState state = visualStates.GetAt(stateIndex);

                            if (state)
                            {
                                auto stateName = state.Name();
                                winrt::Storyboard stateStoryboard = state.Storyboard();

                                if (stateStoryboard)
                                {
                                    if (stateName == s_noIndicatorStateName)
                                    {
                                        winrt::event_token noIndicatorStateStoryboardCompletedToken = stateStoryboard.Completed({ this, &ScrollerView::OnNoIndicatorStateStoryboardCompleted });
                                    }
                                    else if (stateName == s_touchIndicatorStateName || stateName == s_mouseIndicatorStateName || stateName == s_mouseIndicatorFullStateName)
                                    {
                                        winrt::event_token indicatorStateStoryboardCompletedToken = stateStoryboard.Completed({ this, &ScrollerView::OnIndicatorStateStoryboardCompleted });
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    HideIndicators(false /*useTransitions*/);
}

#pragma endregion

#pragma region IControlOverrides

void ScrollerView::OnGotFocus(winrt::RoutedEventArgs const& args)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnGotFocus(args);

    m_preferMouseIndicators = 
        m_focusInputDeviceKind == winrt::FocusInputDeviceKind::Mouse ||
        m_focusInputDeviceKind == winrt::FocusInputDeviceKind::Pen;

    ShowIndicators();
}

#pragma endregion

void ScrollerView::OnScrollerViewGettingFocus(
    const winrt::IInspectable& /*sender*/,
    const winrt::GettingFocusEventArgs& args)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_focusInputDeviceKind = args.InputDevice();
}

void ScrollerView::OnScrollerViewIsEnabledChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!IsEnabled())
    {
        HideIndicators(true /*useTransitions*/);
    }
}

void ScrollerView::OnScrollerViewUnloaded(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_showingMouseIndicators = false;
    m_keepIndicatorsShowing = false;
}

void ScrollerView::OnScrollerViewPointerEntered(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Pointer().PointerDeviceType() != winrt::PointerDeviceType::Touch)
    {
        // Mouse/Pen inputs dominate. If touch panning indicators are shown, switch to mouse indicators.
        m_preferMouseIndicators = true;
        ShowIndicators();
    }
}

void ScrollerView::OnScrollerViewPointerMoved(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    // Don't process if this is a generated replay of the event.
    if (SharedHelpers::IsRS3OrHigher() && args.IsGenerated())
    {
        return;
    }

    if (args.Pointer().PointerDeviceType() != winrt::PointerDeviceType::Touch)
    {
        // Mouse/Pen inputs dominate. If touch panning indicators are shown, switch to mouse indicators.
        m_preferMouseIndicators = true;
        ShowIndicators();

        if (!SharedHelpers::IsAnimationsEnabled() &&
            m_hideIndicatorsTimer &&
            (m_isPointerOverHorizontalScrollController || m_isPointerOverVerticalScrollController))
        {
            StopHideIndicatorsTimer(false /*isForDestructor*/);
        }
    }
}

void ScrollerView::OnScrollerViewPointerExited(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Pointer().PointerDeviceType() != winrt::PointerDeviceType::Touch)
    {
        // Mouse/Pen inputs dominate. If touch panning indicators are shown, switch to mouse indicators.
        m_isPointerOverHorizontalScrollController = false;
        m_isPointerOverVerticalScrollController = false;
        m_preferMouseIndicators = true;
        ShowIndicators();

        HideIndicatorsAfterDelay();
    }
}

void ScrollerView::OnScrollerViewPointerPressed(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Handled())
    {
        return;
    }
    
    if (args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Mouse)
    {
        const winrt::PointerPoint pointerPoint = args.GetCurrentPoint(nullptr);
        const winrt::PointerPointProperties pointerPointProperties = pointerPoint.Properties();

        m_isLeftMouseButtonPressedForFocus = pointerPointProperties.IsLeftButtonPressed();
    }

    // Show the scroll controller indicators as soon as a pointer is pressed on the ScrollerView.
    ShowIndicators();
}

void ScrollerView::OnScrollerViewPointerReleased(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    bool takeFocus = false;

    if (args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Mouse && m_isLeftMouseButtonPressedForFocus)
    {
        m_isLeftMouseButtonPressedForFocus = false;
        takeFocus = true;
    }

    if (args.Handled())
    {
        return;
    }

    if (takeFocus)
    {
        SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_METH, METH_NAME, this, L"Focus");

        bool tookFocus = Focus(winrt::FocusState::Pointer);
        args.Handled(tookFocus);
    }
}

void ScrollerView::OnScrollerViewPointerCanceled(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Mouse)
    {
        m_isLeftMouseButtonPressedForFocus = false;
    }
}

void ScrollerView::OnHorizontalScrollControllerPointerEntered(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverHorizontalScrollController = true;
}

void ScrollerView::OnHorizontalScrollControllerPointerExited(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverHorizontalScrollController = false;
}

void ScrollerView::OnVerticalScrollControllerPointerEntered(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverVerticalScrollController = true;

    if (!SharedHelpers::IsAnimationsEnabled())
    {
        HideIndicatorsAfterDelay();
    }
}

void ScrollerView::OnVerticalScrollControllerPointerExited(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverVerticalScrollController = false;

    HideIndicatorsAfterDelay();
}

// Handler for when the NoIndicator state's storyboard completes animating.
void ScrollerView::OnNoIndicatorStateStoryboardCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_showingMouseIndicators = false;
}

// Handler for when a TouchIndicator, MouseIndicator or MouseIndicatorFull state's storyboard completes animating.
void ScrollerView::OnIndicatorStateStoryboardCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    // If the cursor is currently directly over either scroll controller then do not automatically hide the indicators
    if (!m_keepIndicatorsShowing &&
        !m_isPointerOverVerticalScrollController &&
        !m_isPointerOverHorizontalScrollController)
    {
        // Go to the NoIndicator state using transitions.
        if (SharedHelpers::IsAnimationsEnabled())
        {
            // By default there is a delay before the NoIndicator state actually shows.
            HideIndicators(true /*useTransitions*/);
        }
        else
        {
            // Since OS animations are turned off, use a timer to delay the indicators' hiding.
            HideIndicatorsAfterDelay();
        }
    }
}

// Invoked by ScrollerViewTestHooks
winrt::Scroller ScrollerView::GetScrollerPart()
{
    return safe_cast<winrt::Scroller>(m_scroller.get());
}

void ScrollerView::ValidateAnchorRatio(double value)
{
    Scroller::ValidateAnchorRatio(value);
}

void ScrollerView::ValidateZoomFactoryBoundary(double value)
{
    Scroller::ValidateZoomFactoryBoundary(value);
}


// Invoked when a dependency property of this ScrollerView has changed.
void ScrollerView::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto dependencyProperty = args.Property();

#ifdef _DEBUG
    SCROLLERVIEW_TRACE_VERBOSE(nullptr, L"%s(property: %s)\n", METH_NAME, DependencyPropertyToString(dependencyProperty).c_str());
#endif

    if (dependencyProperty == s_HorizontalScrollControllerVisibilityProperty ||
        dependencyProperty == s_ComputedHorizontalScrollModeProperty)
    {
        UpdateScrollControllersVisibility(true /*horizontalChange*/, false /*verticalChange*/);
    }
    else if (dependencyProperty == s_VerticalScrollControllerVisibilityProperty ||
        dependencyProperty == s_ComputedVerticalScrollModeProperty)
    {
        UpdateScrollControllersVisibility(false /*horizontalChange*/, true /*verticalChange*/);
    }
}

void ScrollerView::OnScrollControllerInteractionInfoChanged(
    const winrt::IScrollController& sender,
    const winrt::IInspectable& /*args*/)
{
    if (m_horizontalScrollControllerElement &&
        m_horizontalScrollControllerElement.get().try_as<winrt::IScrollController>() == sender)
    {
        bool isHorizontalScrollControllerInteracting = sender.IsInteracting();

        if (m_isHorizontalScrollControllerInteracting != isHorizontalScrollControllerInteracting)
        {
            SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"HorizontalScrollController.IsInteracting changed: ", m_isHorizontalScrollControllerInteracting, isHorizontalScrollControllerInteracting);

            m_isHorizontalScrollControllerInteracting = isHorizontalScrollControllerInteracting;

            if (isHorizontalScrollControllerInteracting)
            {
                // Prevent the vertical scroll controller from fading out while the user is interacting with the horizontal one.
                m_keepIndicatorsShowing = true;

                ShowIndicators();
            }
            else
            {
                // Make the scroll controllers fade out, after the normal delay.
                m_keepIndicatorsShowing = false;

                HideIndicators(true /*useTransitions*/);
            }
        }
    }
    else if (m_verticalScrollControllerElement &&
             m_verticalScrollControllerElement.get().try_as<winrt::IScrollController>() == sender)
    {
        bool isVerticalScrollControllerInteracting = sender.IsInteracting();

        if (m_isVerticalScrollControllerInteracting != isVerticalScrollControllerInteracting)
        {
            SCROLLERVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"VerticalScrollController.IsInteracting changed: ", m_isVerticalScrollControllerInteracting, isVerticalScrollControllerInteracting);

            m_isVerticalScrollControllerInteracting = isVerticalScrollControllerInteracting;

            if (isVerticalScrollControllerInteracting)
            {
                // Prevent the horizontal scroll controller from fading out while the user is interacting with the vertical one.
                m_keepIndicatorsShowing = true;

                ShowIndicators();
            }
            else
            {
                // Make the scroll controllers fade out, after the normal delay.
                m_keepIndicatorsShowing = false;

                HideIndicators(true /*useTransitions*/);
            }
        }
    }
}

void ScrollerView::OnHideIndicatorsTimerTick(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    StopHideIndicatorsTimer(false /*isForDestructor*/);
    HideIndicators(true /*useTransitions*/);
}

void ScrollerView::OnScrollerExtentChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    if (m_extentChangedEventSource)
    {
        SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_extentChangedEventSource(*this, args);
    }
}

void ScrollerView::OnScrollerStateChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scroller = m_scroller.get())
    {
        if (scroller.State() == winrt::ScrollerState::Interacting)
        {
            m_preferMouseIndicators = false;
        }
    }

    if (m_stateChangedEventSource)
    {
        m_stateChangedEventSource(*this, args);
    }
}

void ScrollerView::OnScrollerChangingOffsets(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollerChangingOffsetsEventArgs& args)
{
    if (m_changingOffsetsEventSource)
    {
        SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_changingOffsetsEventSource(*this, args);
    }
}

void ScrollerView::OnScrollerChangingZoomFactor(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollerChangingZoomFactorEventArgs& args)
{
    if (m_changingZoomFactorEventSource)
    {
        SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_changingZoomFactorEventSource(*this, args);
    }
}

void ScrollerView::OnScrollerViewChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    // Unless the control is still loading, show the scroll controller indicators when the view changes. For example,
    // when using Ctrl+/- to zoom, mouse-wheel to scroll or zoom, or any other input type. Keep the existing indicator type.
    if (IsLoaded())
    {
        ShowIndicators();
    }

    if (m_viewChangedEventSource)
    {
        SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_viewChangedEventSource(*this, args);
    }
}

void ScrollerView::OnScrollerViewChangeCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollerViewChangeCompletedEventArgs& args)
{
    if (args.ViewChangeId() == m_horizontalScrollWithKeyboardViewChangeId)
    {
        m_horizontalScrollWithKeyboardViewChangeId = -1;
    }
    else if (args.ViewChangeId() == m_verticalScrollWithKeyboardViewChangeId)
    {
        m_verticalScrollWithKeyboardViewChangeId = -1;
    }

    if (m_viewChangeCompletedEventSource)
    {
        SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_viewChangeCompletedEventSource(*this, args);
    }
}

void ScrollerView::OnScrollerBringingIntoView(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollerBringingIntoViewEventArgs& args)
{
    if (m_bringingIntoViewEventSource)
    {
        SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_bringingIntoViewEventSource(*this, args);
    }
}

void ScrollerView::OnScrollerAnchorRequested(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollerAnchorRequestedEventArgs& args)
{
    if (m_anchorRequestedEventSource)
    {
        SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_anchorRequestedEventSource(*this, args);
    }
}

void ScrollerView::OnScrollerPropertyChanged(
    const winrt::DependencyObject& /*sender*/,
    const winrt::DependencyProperty& args)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (args == winrt::Scroller::ComputedHorizontalScrollModeProperty())
    {
        SetValue(s_ComputedHorizontalScrollModeProperty, box_value(m_scroller.get().ComputedHorizontalScrollMode()));
    }
    else if (args == winrt::Scroller::ComputedVerticalScrollModeProperty())
    {
        SetValue(s_ComputedVerticalScrollModeProperty, box_value(m_scroller.get().ComputedVerticalScrollMode()));
    }
}

void ScrollerView::StopHideIndicatorsTimer(bool isForDestructor)
{
    winrt::DispatcherTimer hideIndicatorsTimer = m_hideIndicatorsTimer.safe_get(isForDestructor /*useSafeGet*/);

    if (hideIndicatorsTimer && hideIndicatorsTimer.IsEnabled())
    {
        hideIndicatorsTimer.Stop();
    }
}

void ScrollerView::HookScrollerViewEvents()
{
    MUX_ASSERT(!m_onPointerEnteredEventHandler);
    MUX_ASSERT(!m_onPointerMovedEventHandler);
    MUX_ASSERT(!m_onPointerExitedEventHandler);
    MUX_ASSERT(!m_onPointerPressedEventHandler);
    MUX_ASSERT(!m_onPointerReleasedEventHandler);
    MUX_ASSERT(!m_onPointerCanceledEventHandler);
    MUX_ASSERT(m_gettingFocusToken.value == 0);
    MUX_ASSERT(m_isEnabledChangedToken.value == 0);
    MUX_ASSERT(m_unloadedToken.value == 0);

    m_gettingFocusToken = GettingFocus({ this, &ScrollerView::OnScrollerViewGettingFocus });
    m_isEnabledChangedToken = IsEnabledChanged({ this, &ScrollerView::OnScrollerViewIsEnabledChanged });
    m_unloadedToken = Unloaded({ this, &ScrollerView::OnScrollerViewUnloaded });

    m_onPointerEnteredEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollerView::OnScrollerViewPointerEntered });
    AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onPointerEnteredEventHandler, false);

    m_onPointerMovedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollerView::OnScrollerViewPointerMoved });
    AddHandler(winrt::UIElement::PointerMovedEvent(), m_onPointerMovedEventHandler, false);

    m_onPointerExitedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollerView::OnScrollerViewPointerExited });
    AddHandler(winrt::UIElement::PointerExitedEvent(), m_onPointerExitedEventHandler, false);

    m_onPointerPressedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollerView::OnScrollerViewPointerPressed });
    AddHandler(winrt::UIElement::PointerPressedEvent(), m_onPointerPressedEventHandler, false);

    m_onPointerReleasedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollerView::OnScrollerViewPointerReleased });
    AddHandler(winrt::UIElement::PointerReleasedEvent(), m_onPointerReleasedEventHandler, true);

    m_onPointerCanceledEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollerView::OnScrollerViewPointerCanceled });
    AddHandler(winrt::UIElement::PointerCanceledEvent(), m_onPointerCanceledEventHandler, true);
}

void ScrollerView::UnhookScrollerViewEvents()
{
    if (m_gettingFocusToken.value != 0)
    {
        GettingFocus(m_gettingFocusToken);
        m_gettingFocusToken.value = 0;
    }

    if (m_isEnabledChangedToken.value != 0)
    {
        IsEnabledChanged(m_isEnabledChangedToken);
        m_isEnabledChangedToken.value = 0;
    }

    if (m_unloadedToken.value != 0)
    {
        Unloaded(m_unloadedToken);
        m_unloadedToken.value = 0;
    }

    if (m_onPointerEnteredEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerEnteredEvent(), m_onPointerEnteredEventHandler);
        m_onPointerEnteredEventHandler = nullptr;
    }

    if (m_onPointerMovedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerMovedEvent(), m_onPointerMovedEventHandler);
        m_onPointerMovedEventHandler = nullptr;
    }

    if (m_onPointerExitedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerExitedEvent(), m_onPointerExitedEventHandler);
        m_onPointerExitedEventHandler = nullptr;
    }

    if (m_onPointerPressedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerPressedEvent(), m_onPointerPressedEventHandler);
        m_onPointerPressedEventHandler = nullptr;
    }

    if (m_onPointerReleasedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerReleasedEvent(), m_onPointerReleasedEventHandler);
        m_onPointerReleasedEventHandler = nullptr;
    }

    if (m_onPointerCanceledEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerCanceledEvent(), m_onPointerCanceledEventHandler);
        m_onPointerCanceledEventHandler = nullptr;
    }
}

void ScrollerView::HookScrollerEvents()
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_scrollerExtentChangedToken.value == 0);
    MUX_ASSERT(m_scrollerStateChangedToken.value == 0);
    MUX_ASSERT(m_scrollerChangingOffsetsToken.value == 0);
    MUX_ASSERT(m_scrollerChangingZoomFactorToken.value == 0);
    MUX_ASSERT(m_scrollerViewChangedToken.value == 0);
    MUX_ASSERT(m_scrollerViewChangeCompletedToken.value == 0);
    MUX_ASSERT(m_scrollerBringingIntoViewToken.value == 0);
    MUX_ASSERT(m_scrollerAnchorRequestedToken.value == 0);
    MUX_ASSERT(m_scrollerComputedHorizontalScrollModeChangedToken.value == 0);
    MUX_ASSERT(m_scrollerComputedVerticalScrollModeChangedToken.value == 0);

    if (auto scroller = m_scroller.get())
    {
        m_scrollerExtentChangedToken = scroller.ExtentChanged({ this, &ScrollerView::OnScrollerExtentChanged });
        m_scrollerStateChangedToken = scroller.StateChanged({ this, &ScrollerView::OnScrollerStateChanged });
        m_scrollerChangingOffsetsToken = scroller.ChangingOffsets({ this, &ScrollerView::OnScrollerChangingOffsets });
        m_scrollerChangingZoomFactorToken = scroller.ChangingZoomFactor({ this, &ScrollerView::OnScrollerChangingZoomFactor });
        m_scrollerViewChangedToken = scroller.ViewChanged({ this, &ScrollerView::OnScrollerViewChanged });
        m_scrollerViewChangeCompletedToken = scroller.ViewChangeCompleted({ this, &ScrollerView::OnScrollerViewChangeCompleted });
        m_scrollerBringingIntoViewToken = scroller.BringingIntoView({ this, &ScrollerView::OnScrollerBringingIntoView });
        m_scrollerAnchorRequestedToken = scroller.AnchorRequested({ this, &ScrollerView::OnScrollerAnchorRequested });

        const winrt::DependencyObject scrollerAsDO = scroller.try_as<winrt::DependencyObject>();

        m_scrollerComputedHorizontalScrollModeChangedToken.value = scrollerAsDO.RegisterPropertyChangedCallback(
            winrt::Scroller::ComputedHorizontalScrollModeProperty(), { this, &ScrollerView::OnScrollerPropertyChanged });

        m_scrollerComputedVerticalScrollModeChangedToken.value = scrollerAsDO.RegisterPropertyChangedCallback(
            winrt::Scroller::ComputedVerticalScrollModeProperty(), { this, &ScrollerView::OnScrollerPropertyChanged });
    }
}

void ScrollerView::UnhookScrollerEvents(bool isForDestructor)
{
    if (isForDestructor)
    {
        SCROLLERVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }
    else
    {
        SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);
    }

    if (auto scroller = m_scroller.safe_get())
    {
        if (m_scrollerExtentChangedToken.value != 0)
        {
            scroller.ExtentChanged(m_scrollerExtentChangedToken);
            m_scrollerExtentChangedToken.value = 0;
        }

        if (m_scrollerStateChangedToken.value != 0)
        {
            scroller.StateChanged(m_scrollerStateChangedToken);
            m_scrollerStateChangedToken.value = 0;
        }

        if (m_scrollerChangingOffsetsToken.value != 0)
        {
            scroller.ChangingOffsets(m_scrollerChangingOffsetsToken);
            m_scrollerChangingOffsetsToken.value = 0;
        }

        if (m_scrollerChangingZoomFactorToken.value != 0)
        {
            scroller.ChangingZoomFactor(m_scrollerChangingZoomFactorToken);
            m_scrollerChangingZoomFactorToken.value = 0;
        }

        if (m_scrollerViewChangedToken.value != 0)
        {
            scroller.ViewChanged(m_scrollerViewChangedToken);
            m_scrollerViewChangedToken.value = 0;
        }

        if (m_scrollerViewChangeCompletedToken.value != 0)
        {
            scroller.ViewChangeCompleted(m_scrollerViewChangeCompletedToken);
            m_scrollerViewChangeCompletedToken.value = 0;
        }

        if (m_scrollerBringingIntoViewToken.value != 0)
        {
            scroller.BringingIntoView(m_scrollerBringingIntoViewToken);
            m_scrollerBringingIntoViewToken.value = 0;
        }

        if (m_scrollerAnchorRequestedToken.value != 0)
        {
            scroller.AnchorRequested(m_scrollerAnchorRequestedToken);
            m_scrollerAnchorRequestedToken.value = 0;
        }

        const winrt::DependencyObject scrollerAsDO = scroller.try_as<winrt::DependencyObject>();

        if (m_scrollerComputedHorizontalScrollModeChangedToken.value != 0)
        {
            scrollerAsDO.UnregisterPropertyChangedCallback(winrt::Scroller::ComputedHorizontalScrollModeProperty(), m_scrollerComputedHorizontalScrollModeChangedToken.value);
            m_scrollerComputedHorizontalScrollModeChangedToken.value = 0;
        }

        if (m_scrollerComputedVerticalScrollModeChangedToken.value != 0)
        {
            scrollerAsDO.UnregisterPropertyChangedCallback(winrt::Scroller::ComputedVerticalScrollModeProperty(), m_scrollerComputedVerticalScrollModeChangedToken.value);
            m_scrollerComputedVerticalScrollModeChangedToken.value = 0;
        }
    }
}

void ScrollerView::HookHorizontalScrollControllerEvents()
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_horizontalScrollControllerInteractionInfoChangedToken.value == 0);
    MUX_ASSERT(!m_onHorizontalScrollControllerPointerEnteredHandler);
    MUX_ASSERT(!m_onHorizontalScrollControllerPointerExitedHandler);

    if (winrt::IUIElement horizontalScrollControllerElement = m_horizontalScrollControllerElement.get())
    {
        m_horizontalScrollControllerInteractionInfoChangedToken = 
            horizontalScrollControllerElement.try_as<winrt::IScrollController>().InteractionInfoChanged({ this, &ScrollerView::OnScrollControllerInteractionInfoChanged });

        m_onHorizontalScrollControllerPointerEnteredHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollerView::OnHorizontalScrollControllerPointerEntered });
        horizontalScrollControllerElement.AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onHorizontalScrollControllerPointerEnteredHandler, true);

        m_onHorizontalScrollControllerPointerExitedHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollerView::OnHorizontalScrollControllerPointerExited });
        horizontalScrollControllerElement.AddHandler(winrt::UIElement::PointerExitedEvent(), m_onHorizontalScrollControllerPointerExitedHandler, true);
    }
}

void ScrollerView::UnhookHorizontalScrollControllerEvents()
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (winrt::IUIElement horizontalScrollControllerElement = m_horizontalScrollControllerElement.safe_get())
    {
        if (m_horizontalScrollControllerInteractionInfoChangedToken.value != 0)
        {
            horizontalScrollControllerElement.try_as<winrt::IScrollController>().InteractionInfoChanged(m_horizontalScrollControllerInteractionInfoChangedToken);
        }
        m_horizontalScrollControllerInteractionInfoChangedToken.value = 0;

        if (m_onHorizontalScrollControllerPointerEnteredHandler)
        {
            horizontalScrollControllerElement.RemoveHandler(winrt::UIElement::PointerEnteredEvent(), m_onHorizontalScrollControllerPointerEnteredHandler);
            m_onHorizontalScrollControllerPointerEnteredHandler = nullptr;
        }

        if (m_onHorizontalScrollControllerPointerExitedHandler)
        {
            horizontalScrollControllerElement.RemoveHandler(winrt::UIElement::PointerExitedEvent(), m_onHorizontalScrollControllerPointerExitedHandler);
            m_onHorizontalScrollControllerPointerExitedHandler = nullptr;
        }
    }
}

void ScrollerView::HookVerticalScrollControllerEvents()
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_verticalScrollControllerInteractionInfoChangedToken.value == 0);
    MUX_ASSERT(!m_onVerticalScrollControllerPointerEnteredHandler);
    MUX_ASSERT(!m_onVerticalScrollControllerPointerExitedHandler);

    if (winrt::IUIElement verticalScrollControllerElement = m_verticalScrollControllerElement.get())
    {
        m_verticalScrollControllerInteractionInfoChangedToken =
            verticalScrollControllerElement.try_as<winrt::IScrollController>().InteractionInfoChanged({ this, &ScrollerView::OnScrollControllerInteractionInfoChanged });

        m_onVerticalScrollControllerPointerEnteredHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollerView::OnVerticalScrollControllerPointerEntered });
        verticalScrollControllerElement.AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onVerticalScrollControllerPointerEnteredHandler, true);

        m_onVerticalScrollControllerPointerExitedHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollerView::OnVerticalScrollControllerPointerExited });
        verticalScrollControllerElement.AddHandler(winrt::UIElement::PointerExitedEvent(), m_onVerticalScrollControllerPointerExitedHandler, true);
    }
}

void ScrollerView::UnhookVerticalScrollControllerEvents()
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (winrt::IUIElement verticalScrollControllerElement = m_verticalScrollControllerElement.safe_get())
    {
        if (m_verticalScrollControllerInteractionInfoChangedToken.value != 0)
        {
            verticalScrollControllerElement.try_as<winrt::IScrollController>().InteractionInfoChanged(m_verticalScrollControllerInteractionInfoChangedToken);
        }
        m_verticalScrollControllerInteractionInfoChangedToken.value = 0;

        if (m_onVerticalScrollControllerPointerEnteredHandler)
        {
            verticalScrollControllerElement.RemoveHandler(winrt::UIElement::PointerEnteredEvent(), m_onVerticalScrollControllerPointerEnteredHandler);
            m_onVerticalScrollControllerPointerEnteredHandler = nullptr;
        }

        if (m_onVerticalScrollControllerPointerExitedHandler)
        {
            verticalScrollControllerElement.RemoveHandler(winrt::UIElement::PointerExitedEvent(), m_onVerticalScrollControllerPointerExitedHandler);
            m_onVerticalScrollControllerPointerExitedHandler = nullptr;
        }
    }
}

void ScrollerView::UpdateScroller(const winrt::Scroller& scroller)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollerEvents(false /*isForDestructor*/);
    m_scroller.set(nullptr);

    SetValue(s_ScrollerProperty, scroller);

    if (scroller)
    {
        m_scroller.set(scroller);
        HookScrollerEvents();
    }
}

void ScrollerView::UpdateHorizontalScrollController(const winrt::IScrollController& horizontalScrollController)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookHorizontalScrollControllerEvents();

    SetValue(s_HorizontalScrollControllerProperty, horizontalScrollController);

    winrt::IUIElement horizontalScrollControllerElement{ nullptr };

    if (horizontalScrollController)
    {
        horizontalScrollControllerElement = horizontalScrollController.try_as<winrt::IUIElement>();
    }

    m_horizontalScrollControllerElement.set(horizontalScrollControllerElement);
    HookHorizontalScrollControllerEvents();
    UpdateScrollerHorizontalScrollController(horizontalScrollController);    
}

void ScrollerView::UpdateScrollerHorizontalScrollController(const winrt::IScrollController& horizontalScrollController)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scroller = m_scroller.get())
    {
        scroller.HorizontalScrollController(horizontalScrollController);
    }
}

void ScrollerView::UpdateVerticalScrollController(const winrt::IScrollController& verticalScrollController)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookVerticalScrollControllerEvents();

    SetValue(s_VerticalScrollControllerProperty, verticalScrollController);

    winrt::IUIElement verticalScrollControllerElement{ nullptr };

    if (verticalScrollController)
    {
        verticalScrollControllerElement = verticalScrollController.try_as<winrt::IUIElement>();
    }

    m_verticalScrollControllerElement.set(verticalScrollControllerElement);
    HookVerticalScrollControllerEvents();
    UpdateScrollerVerticalScrollController(verticalScrollController);
}

void ScrollerView::UpdateScrollerVerticalScrollController(const winrt::IScrollController& verticalScrollController)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scroller = m_scroller.get())
    {
        scroller.VerticalScrollController(verticalScrollController);
    }
}

void ScrollerView::UpdateScrollControllersSeparator(const winrt::IUIElement& scrollControllersSeparator)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_scrollControllersSeparatorElement.set(scrollControllersSeparator);
}

void ScrollerView::UpdateScrollControllersVisibility(
    bool horizontalChange, bool verticalChange)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(horizontalChange || verticalChange);

    bool isHorizontalScrollControllerVisible = false;

    if (m_horizontalScrollControllerElement)
    {
        if (horizontalChange)
        {
            winrt::ScrollerViewScrollControllerVisibility scrollControllerVisibility = 
                unbox_value<winrt::ScrollerViewScrollControllerVisibility>(GetValue(s_HorizontalScrollControllerVisibilityProperty));

            if (scrollControllerVisibility == winrt::ScrollerViewScrollControllerVisibility::Auto &&
                ComputedHorizontalScrollMode() == winrt::ScrollerScrollMode::Enabled)
            {
                isHorizontalScrollControllerVisible = true;
            }
            else
            {
                isHorizontalScrollControllerVisible = (scrollControllerVisibility == winrt::ScrollerViewScrollControllerVisibility::Visible);
            }

            m_horizontalScrollControllerElement.get().Visibility(
                isHorizontalScrollControllerVisible ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
        }
        else
        {
            isHorizontalScrollControllerVisible = (m_horizontalScrollControllerElement.get().Visibility() == winrt::Visibility::Visible);
        }
    }

    bool isVerticalScrollControllerVisible = false;

    if (m_verticalScrollControllerElement)
    {
        if (verticalChange)
        {
            winrt::ScrollerViewScrollControllerVisibility scrollControllerVisibility = 
                unbox_value<winrt::ScrollerViewScrollControllerVisibility>(GetValue(s_VerticalScrollControllerVisibilityProperty));

            if (scrollControllerVisibility == winrt::ScrollerViewScrollControllerVisibility::Auto &&
                ComputedVerticalScrollMode() == winrt::ScrollerScrollMode::Enabled)
            {
                isVerticalScrollControllerVisible = true;
            }
            else
            {
                isVerticalScrollControllerVisible = (scrollControllerVisibility == winrt::ScrollerViewScrollControllerVisibility::Visible);
            }

            m_verticalScrollControllerElement.get().Visibility(
                isVerticalScrollControllerVisible ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
        }
        else
        {
            isVerticalScrollControllerVisible = (m_verticalScrollControllerElement.get().Visibility() == winrt::Visibility::Visible);
        }
    }

    if (m_scrollControllersSeparatorElement)
    {
        m_scrollControllersSeparatorElement.get().Visibility(isHorizontalScrollControllerVisible && isVerticalScrollControllerVisible ?
            winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }
}

bool ScrollerView::IsLoaded()
{
    return winrt::VisualTreeHelper::GetParent(*this) != nullptr;
}

bool ScrollerView::AreAllScrollControllersCollapsed()
{
    return (!m_horizontalScrollControllerElement || m_horizontalScrollControllerElement.get().Visibility() == winrt::Visibility::Collapsed) &&
           (!m_verticalScrollControllerElement || m_verticalScrollControllerElement.get().Visibility() == winrt::Visibility::Collapsed);
}

bool ScrollerView::AreBothScrollControllersVisible()
{
    return m_horizontalScrollControllerElement && m_horizontalScrollControllerElement.get().Visibility() == winrt::Visibility::Visible &&
           m_verticalScrollControllerElement && m_verticalScrollControllerElement.get().Visibility() == winrt::Visibility::Visible;
}

// Show the appropriate scrolling indicators.
void ScrollerView::ShowIndicators()
{
    if (!AreAllScrollControllersCollapsed())
    {
        if (m_hideIndicatorsTimer)
        {
            winrt::DispatcherTimer hideIndicatorsTimer = m_hideIndicatorsTimer.get();

            if (hideIndicatorsTimer.IsEnabled())
            {
                hideIndicatorsTimer.Stop();
                hideIndicatorsTimer.Start();
            }
        }

        // Mouse indicators dominate if they are already showing or if we have set the flag to prefer them.
        if (m_preferMouseIndicators || m_showingMouseIndicators)
        {
            if (AreBothScrollControllersVisible() && (m_isPointerOverHorizontalScrollController || m_isPointerOverVerticalScrollController))
            {
                winrt::VisualStateManager::GoToState(*this, s_mouseIndicatorFullStateName, true /*useTransitions*/);

                SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_mouseIndicatorFullStateName);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, s_mouseIndicatorStateName, true /*useTransitions*/);

                SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_mouseIndicatorStateName);
            }

            m_showingMouseIndicators = true;
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, s_touchIndicatorStateName, true /*useTransitions*/);

            SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_touchIndicatorStateName);
        }
    }
}

void ScrollerView::HideIndicators(
    bool useTransitions)
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, s_noIndicatorStateName, useTransitions, m_keepIndicatorsShowing);

    if (!m_keepIndicatorsShowing)
    {
        winrt::VisualStateManager::GoToState(*this, s_noIndicatorStateName, useTransitions);
    }
}

void ScrollerView::HideIndicatorsAfterDelay()
{
    SCROLLERVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, m_keepIndicatorsShowing);

    if (!m_keepIndicatorsShowing)
    {
        winrt::DispatcherTimer hideIndicatorsTimer = nullptr;

        if (m_hideIndicatorsTimer)
        {
            hideIndicatorsTimer = m_hideIndicatorsTimer.get();
            if (hideIndicatorsTimer.IsEnabled())
            {
                hideIndicatorsTimer.Stop();
            }
        }
        else
        {
            hideIndicatorsTimer = winrt::DispatcherTimer();
            hideIndicatorsTimer.Interval(winrt::TimeSpan::duration(s_noIndicatorCountdown));
            hideIndicatorsTimer.Tick({ this, &ScrollerView::OnHideIndicatorsTimerTick });
            m_hideIndicatorsTimer.set(hideIndicatorsTimer);
        }

        hideIndicatorsTimer.Start();
    }
}

void ScrollerView::OnKeyDown(winrt::KeyRoutedEventArgs const& e)
{
    __super::OnKeyDown(e);
    
    m_preferMouseIndicators = false;

    if (m_scroller)
    {
        winrt::KeyRoutedEventArgs eventArgs = e.as<winrt::KeyRoutedEventArgs>();
        if (!eventArgs.Handled())
        {
            auto scroller = m_scroller.get().as<winrt::Scroller>();
            auto originalKey = eventArgs.OriginalKey();

            bool isGamepadKey = FocusHelper::IsGamepadNavigationDirection(originalKey) || FocusHelper::IsGamepadPageNavigationDirection(originalKey);
            bool isXYFocusEnabledForKeyboard = XYFocusKeyboardNavigation() == winrt::XYFocusKeyboardNavigationMode::Enabled;
            bool doXYFocusScrolling = isGamepadKey || isXYFocusEnabledForKeyboard;

            if (doXYFocusScrolling)
            {
                HandleKeyDownForXYNavigation(eventArgs);
            }
            else
            {
                HandleKeyDownForStandardScroll(eventArgs);
            }
        }
    }
}

void ScrollerView::HandleKeyDownForStandardScroll(winrt::KeyRoutedEventArgs args)
{
    // Up/Down/Left/Right will scroll by 15% the size of the viewport.
    static const double smallScrollProportion = 0.15;

    MUX_ASSERT(!args.Handled());
    MUX_ASSERT(m_scroller != nullptr);

    bool isHandled = DoScrollForKey(args.Key(), smallScrollProportion);

    args.Handled(isHandled);
}

void ScrollerView::HandleKeyDownForXYNavigation(winrt::KeyRoutedEventArgs args)
{
    MUX_ASSERT(!args.Handled());
    MUX_ASSERT(m_scroller != nullptr);

    bool isHandled = false;
    auto originalKey = args.OriginalKey();
    auto scroller = m_scroller.get().as<winrt::Scroller>();
    bool isPageNavigation = FocusHelper::IsGamepadPageNavigationDirection(originalKey);
    double scrollAmountProportion = isPageNavigation ? 1.0 : 0.5;
    bool shouldProcessKeyEvent = true;
    winrt::FocusNavigationDirection navigationDirection;

    if (isPageNavigation)
    {
        navigationDirection = FocusHelper::GetPageNavigationDirection(originalKey);

        // We should only handle page navigation if we can scroll in that direction.
        // Note: For non-paging navigation, we might want to move focus even if we cannot scroll.
        shouldProcessKeyEvent = CanScrollInDirection(navigationDirection);
    }
    else
    {
        navigationDirection = FocusHelper::GetNavigationDirection(originalKey);
    }

    if (shouldProcessKeyEvent)
    {
        bool shouldScroll = false;
        bool shouldMoveFocus = false;
        winrt::DependencyObject nextElement = nullptr;

        if (navigationDirection != winrt::FocusNavigationDirection::None)
        {
            nextElement = GetNextFocusCandidate(navigationDirection, isPageNavigation);
        }

        if (nextElement && nextElement != winrt::FocusManager::GetFocusedElement())
        {
            winrt::UIElement nextElementAsUIE = FocusHelper::GetUIElementForFocusCandidate(nextElement);
            MUX_ASSERT(nextElementAsUIE != nullptr);

            auto nextElementAsFe = nextElementAsUIE.as<winrt::FrameworkElement>();
            auto rect = winrt::Rect{ 0, 0, static_cast<float>(nextElementAsFe.ActualWidth()), static_cast<float>(nextElementAsFe.ActualHeight()) };
            auto elementBounds = nextElementAsUIE.TransformToVisual(scroller).TransformBounds(rect);
            auto viewport = winrt::Rect{ 0, 0, static_cast<float>(scroller.ActualWidth()), static_cast<float>(scroller.ActualHeight()) };

            // Extend the viewport in the direction we are moving:
            winrt::Rect extendedViewport = viewport;
            switch (navigationDirection)
            {
            case winrt::FocusNavigationDirection::Down:
                extendedViewport.Height += viewport.Height;
                break;
            case winrt::FocusNavigationDirection::Up:
                extendedViewport.Y -= viewport.Height;
                extendedViewport.Height += viewport.Height;
                break;
            case winrt::FocusNavigationDirection::Left:
                extendedViewport.X -= viewport.Width;
                extendedViewport.Width += viewport.Width;
                break;
            case winrt::FocusNavigationDirection::Right:
                extendedViewport.Width += viewport.Width;
                break;
            }

            bool isElementInExtendedViewport = winrt::RectHelper::Intersect(elementBounds, extendedViewport) != winrt::RectHelper::Empty();
            bool isElementFullyInExtendedViewport = winrt::RectHelper::Union(elementBounds, extendedViewport) == extendedViewport;

            if (isElementInExtendedViewport)
            {
                if (isPageNavigation)
                {
                    // Always scroll for page navigation
                    shouldScroll = true;

                    if (isElementFullyInExtendedViewport)
                    {
                        // Move focus:
                        shouldMoveFocus = true;
                    }
                }
                else
                {
                    // Non-paging scroll allows partial candidates
                    shouldMoveFocus = true;
                }
            }
            else
            {
                // Element is outside extended viewport - scroll but don't focus.
                shouldScroll = true;
            }
        }
        else
        {
            // No focus candidate: scroll
            shouldScroll = true;
        }

        if (shouldMoveFocus)
        {
            winrt::FocusManager::TryFocusAsync(nextElement, winrt::FocusState::Keyboard);
            isHandled = true;
        }

        if (shouldScroll)
        {
            if (navigationDirection == winrt::FocusNavigationDirection::None)
            {
                isHandled = DoScrollForKey(args.Key(), scrollAmountProportion);
            }
            else
            {
                if (navigationDirection == winrt::FocusNavigationDirection::Down && CanScrollDown())
                {
                    isHandled = true;
                    DoScroll(scroller.ActualHeight() * scrollAmountProportion, winrt::Orientation::Vertical);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Up && CanScrollUp())
                {
                    isHandled = true;
                    DoScroll(-scroller.ActualHeight() * scrollAmountProportion, winrt::Orientation::Vertical);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Right && CanScrollRight())
                {
                    isHandled = true;
                    DoScroll(scroller.ActualWidth() * scrollAmountProportion * (FlowDirection() == winrt::FlowDirection::RightToLeft ? -1 : 1), winrt::Orientation::Horizontal);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Left && CanScrollLeft())
                {
                    isHandled = true;
                    DoScroll(-scroller.ActualWidth() * scrollAmountProportion * (FlowDirection() == winrt::FlowDirection::RightToLeft ? -1 : 1), winrt::Orientation::Horizontal);
                }
            }
        }
    }

    args.Handled(isHandled);
}

winrt::DependencyObject ScrollerView::GetNextFocusCandidate(winrt::FocusNavigationDirection navigationDirection, bool isPageNavigation)
{
    MUX_ASSERT(m_scroller != nullptr);
    MUX_ASSERT(navigationDirection != winrt::FocusNavigationDirection::None);
    auto scroller = m_scroller.get().as<winrt::Scroller>();

    winrt::FocusNavigationDirection focusDirection = navigationDirection;

    winrt::FindNextElementOptions findNextElementOptions;
    findNextElementOptions.SearchRoot(scroller.Child());

    if (isPageNavigation)
    {
        auto localBounds = winrt::Rect{ 0, 0, static_cast<float>(scroller.ActualWidth()), static_cast<float>(scroller.ActualHeight()) };
        auto globalBounds = scroller.TransformToVisual(nullptr).TransformBounds(localBounds);
        const int numPagesLookAhead = 2;

        auto hintRect = globalBounds;
        switch (navigationDirection)
        {
        case winrt::FocusNavigationDirection::Down:
            hintRect.Y += globalBounds.Height * numPagesLookAhead;
            break;
        case winrt::FocusNavigationDirection::Up:
            hintRect.Y -= globalBounds.Height * numPagesLookAhead;
            break;
        case winrt::FocusNavigationDirection::Left:
            hintRect.X -= globalBounds.Width * numPagesLookAhead;
            break;
        case winrt::FocusNavigationDirection::Right:
            hintRect.X += globalBounds.Width * numPagesLookAhead;
            break;
        default:
            MUX_ASSERT(false);
            break;
        }
        
        findNextElementOptions.HintRect(hintRect);
        findNextElementOptions.ExclusionRect(hintRect);
        focusDirection = FocusHelper::GetOppositeDirection(navigationDirection);
    }

    return winrt::FocusManager::FindNextElement(focusDirection, findNextElementOptions);
}

bool ScrollerView::DoScrollForKey(winrt::VirtualKey key, double scrollProportion)
{
    MUX_ASSERT(m_scroller != nullptr);

    bool isScrollTriggered = false;
    auto scroller = m_scroller.get().as<winrt::Scroller>();

    if (key == winrt::VirtualKey::Down && CanScrollDown())
    {
        isScrollTriggered = true;
        DoScroll(scroller.ActualHeight() * scrollProportion, winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::Up && CanScrollUp())
    {
        isScrollTriggered = true;
        DoScroll(scroller.ActualHeight() * -scrollProportion, winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::PageDown && CanScrollDown())
    {
        isScrollTriggered = true;
        DoScroll(scroller.ActualHeight(), winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::PageUp && CanScrollUp())
    {
        isScrollTriggered = true;
        DoScroll(-scroller.ActualHeight(), winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::Left || key == winrt::VirtualKey::Right)
    {
        double scrollAmount = scroller.ActualWidth() * scrollProportion;
        if (FlowDirection() == winrt::FlowDirection::RightToLeft)
        {
            scrollAmount *= -1;
        }

        if (key == winrt::VirtualKey::Right && CanScrollRight())
        {
            isScrollTriggered = true;
            DoScroll(scrollAmount, winrt::Orientation::Horizontal);
        }
        else if (key == winrt::VirtualKey::Left && CanScrollLeft())
        {
            isScrollTriggered = true;
            DoScroll(-scrollAmount, winrt::Orientation::Horizontal);
        }
    }
    else if (key == winrt::VirtualKey::Home)
    {
        bool canScrollUp = CanScrollUp();
        if (canScrollUp || (ComputedVerticalScrollMode() == winrt::ScrollerScrollMode::Disabled && CanScrollLeft()))
        {
            isScrollTriggered = true;
            auto horizontalOffset = canScrollUp ? scroller.HorizontalOffset() : 0.0;
            auto verticalOffset = canScrollUp ? 0.0 : scroller.VerticalOffset();

            if (!canScrollUp && FlowDirection() == winrt::FlowDirection::RightToLeft)
            {
                horizontalOffset = scroller.ExtentWidth() * scroller.ZoomFactor() - scroller.ActualWidth();
            }

            winrt::ScrollerChangeOffsetsOptions options(horizontalOffset, verticalOffset, winrt::ScrollerViewKind::Absolute, winrt::ScrollerViewChangeKind::AllowAnimation, winrt::ScrollerViewChangeSnapPointRespect::RespectSnapPoints);
            scroller.ChangeOffsets(options);
        }
    }
    else if (key == winrt::VirtualKey::End)
    {
        bool canScrollDown = CanScrollDown();
        if (canScrollDown || (ComputedVerticalScrollMode() == winrt::ScrollerScrollMode::Disabled && CanScrollRight()))
        {
            isScrollTriggered = true;
            auto zoomedExtent = (canScrollDown ? scroller.ExtentHeight() : scroller.ExtentWidth()) * scroller.ZoomFactor();
            auto horizontalOffset = canScrollDown ? scroller.HorizontalOffset() : zoomedExtent - scroller.ActualWidth();
            auto verticalOffset = canScrollDown ? zoomedExtent - scroller.ActualHeight() : scroller.VerticalOffset();

            if (!canScrollDown && FlowDirection() == winrt::FlowDirection::RightToLeft)
            {
                horizontalOffset = 0.0;
            }

            winrt::ScrollerChangeOffsetsOptions options(horizontalOffset, verticalOffset, winrt::ScrollerViewKind::Absolute, winrt::ScrollerViewChangeKind::AllowAnimation, winrt::ScrollerViewChangeSnapPointRespect::RespectSnapPoints);
            scroller.ChangeOffsets(options);
        }
    }

    return isScrollTriggered;
}

void ScrollerView::DoScroll(double offset, winrt::Orientation orientation)
{
    static const winrt::float2 inertiaDecayRate(0.9995f, 0.9995f);

    // A velocity less than or equal to this value has no effect.
    static const double minVelocity = 30.0;

    // We need to add this much velocity over minVelocity per pixel we want to move:
    static constexpr double s_velocityNeededPerPixel{ 7.600855902349023 };

    bool isVertical = orientation == winrt::Orientation::Vertical;

    if (auto scroller = m_scroller.get().as<winrt::Scroller>())
    {
        int scrollDir = offset > 0 ? 1 : -1;

        // The minimum velocity required to move in the given direction.
        double baselineVelocity = minVelocity * scrollDir;

        // If there is already a scroll animation running for a previous key press, we want to take that into account
        // for calculating the baseline velocity. 
        auto previousScrollViewChangeId = isVertical ? m_verticalScrollWithKeyboardViewChangeId : m_horizontalScrollWithKeyboardViewChangeId;
        if (previousScrollViewChangeId != -1)
        {
            auto directionOfPreviousScrollOperation = isVertical ? m_verticalScrollWithKeyboardDirection : m_horizontalScrollWithKeyboardDirection;
            if (directionOfPreviousScrollOperation == 1)
            {
                baselineVelocity -= minVelocity;
            }
            else if (directionOfPreviousScrollOperation == -1)
            {
                baselineVelocity += minVelocity;
            }
        }

        float velocity = static_cast<float>(baselineVelocity + (offset * s_velocityNeededPerPixel));

        if (isVertical)
        {
            winrt::float2 additionalVelocity(0.0f, velocity);
            winrt::ScrollerChangeOffsetsWithAdditionalVelocityOptions options(additionalVelocity, inertiaDecayRate);
            m_verticalScrollWithKeyboardViewChangeId = scroller.ChangeOffsetsWithAdditionalVelocity(options);
            m_verticalScrollWithKeyboardDirection = scrollDir;
        }
        else
        {
            winrt::float2 additionalVelocity(velocity, 0.0f);
            winrt::ScrollerChangeOffsetsWithAdditionalVelocityOptions options(additionalVelocity, inertiaDecayRate);
            m_horizontalScrollWithKeyboardViewChangeId = scroller.ChangeOffsetsWithAdditionalVelocity(options);
            m_horizontalScrollWithKeyboardDirection = scrollDir;
        }
    }
}

bool ScrollerView::CanScrollInDirection(winrt::FocusNavigationDirection direction)
{
    bool result = false;
    switch (direction)
    {
    case winrt::FocusNavigationDirection::Down:
        result = CanScrollDown();
        break;
    case winrt::FocusNavigationDirection::Up:
        result = CanScrollUp();
        break;
    case winrt::FocusNavigationDirection::Left:
        result = CanScrollLeft();
        break;
    case winrt::FocusNavigationDirection::Right:
        result = CanScrollRight();
        break;
    }

    return result;
}

bool ScrollerView::CanScrollDown()
{
    return CanScrollVerticallyInDirection(true /*inPositiveDirection*/);
}

bool ScrollerView::CanScrollUp()
{
    return CanScrollVerticallyInDirection(false /*inPositiveDirection*/);
}

bool ScrollerView::CanScrollRight()
{
    return CanScrollHorizontallyInDirection(true /*inPositiveDirection*/);
}

bool ScrollerView::CanScrollLeft()
{
    return CanScrollHorizontallyInDirection(false /*inPositiveDirection*/);
}

bool ScrollerView::CanScrollVerticallyInDirection(bool inPositiveDirection)
{
    bool canScrollInDirection = false;
    if (m_scroller)
    {
        auto scroller = m_scroller.get().as<winrt::Scroller>();
        if (ComputedVerticalScrollMode() == winrt::ScrollerScrollMode::Enabled)
        {
            auto zoomedExtentHeight = scroller.ExtentHeight() * scroller.ZoomFactor();
            auto viewportHeight = scroller.ActualHeight();
            if (zoomedExtentHeight > viewportHeight)
            {
                if (inPositiveDirection)
                {
                    auto maxVerticalOffset = zoomedExtentHeight - viewportHeight;
                    if (scroller.VerticalOffset() < maxVerticalOffset)
                    {
                        canScrollInDirection = true;
                    }
                }
                else
                {
                    if (scroller.VerticalOffset() > 0)
                    {
                        canScrollInDirection = true;
                    }
                }
            }
        }
    }

    return canScrollInDirection;
}

bool ScrollerView::CanScrollHorizontallyInDirection(bool inPositiveDirection)
{
    bool canScrollInDirection = false;

    if (FlowDirection() == winrt::FlowDirection::RightToLeft)
    {
        inPositiveDirection = !inPositiveDirection;
    }
    
    if (m_scroller)
    {
        auto scroller = m_scroller.get().as<winrt::Scroller>();
        if (ComputedHorizontalScrollMode() == winrt::ScrollerScrollMode::Enabled)
        {
            auto zoomedExtentWidth = scroller.ExtentWidth() * scroller.ZoomFactor();
            auto viewportWidth = scroller.ActualWidth();
            if (zoomedExtentWidth > viewportWidth)
            {
                if (inPositiveDirection)
                {
                    auto maxHorizontalOffset = zoomedExtentWidth - viewportWidth;
                    if (scroller.HorizontalOffset() < maxHorizontalOffset)
                    {
                        canScrollInDirection = true;
                    }
                }
                else
                {
                    if (scroller.HorizontalOffset() > 0)
                    {
                        canScrollInDirection = true;
                    }
                }
            }
        }
    }

    return canScrollInDirection;
}


#ifdef _DEBUG

winrt::hstring ScrollerView::DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty)
{
    if (dependencyProperty == s_ContentProperty)
    {
        return L"Content";
    }
    else if (dependencyProperty == s_ScrollerProperty)
    {
        return L"Scroller";
    }
    else if (dependencyProperty == s_HorizontalScrollControllerProperty)
    {
        return L"HorizontalScrollController";
    }
    else if (dependencyProperty == s_VerticalScrollControllerProperty)
    {
        return L"VerticalScrollController";
    }
    else if (dependencyProperty == s_HorizontalScrollControllerVisibilityProperty)
    {
        return L"HorizontalScrollControllerVisibility";
    }
    else if (dependencyProperty == s_VerticalScrollControllerVisibilityProperty)
    {
        return L"VerticalScrollControllerVisibility";
    }
    else if (dependencyProperty == s_IsChildAvailableWidthConstrainedProperty)
    {
        return L"IsChildAvailableWidthConstrained";
    }
    else if (dependencyProperty == s_IsChildAvailableHeightConstrainedProperty)
    {
        return L"IsChildAvailableHeightConstrained";
    }
    else if (dependencyProperty == s_VerticalScrollChainingModeProperty)
    {
        return L"VerticalScrollChainingMode";
    }
    else if (dependencyProperty == s_ZoomChainingModeProperty)
    {
        return L"ZoomChainingMode";
    }
    else if (dependencyProperty == s_HorizontalScrollRailingModeProperty)
    {
        return L"HorizontalScrollRailingMode";
    }
    else if (dependencyProperty == s_VerticalScrollRailingModeProperty)
    {
        return L"VerticalScrollRailingMode";
    }
    else if (dependencyProperty == s_HorizontalScrollModeProperty)
    {
        return L"HorizontalScrollMode";
    }
    else if (dependencyProperty == s_VerticalScrollModeProperty)
    {
        return L"VerticalScrollMode";
    }
    else if (dependencyProperty == s_ComputedHorizontalScrollModeProperty)
    {
        return L"ComputedHorizontalScrollMode";
    }
    else if (dependencyProperty == s_ComputedVerticalScrollModeProperty)
    {
        return L"ComputedVerticalScrollMode";
    }
    else if (dependencyProperty == s_ZoomModeProperty)
    {
        return L"ZoomMode";
    }
    else if (dependencyProperty == s_InputKindProperty)
    {
        return L"InputKind";
    }
    else if (dependencyProperty == s_MinZoomFactorProperty)
    {
        return L"MinZoomFactor";
    }
    else if (dependencyProperty == s_MaxZoomFactorProperty)
    {
        return L"MaxZoomFactor";
    }
    else if (dependencyProperty == s_IsAnchoredAtHorizontalExtentProperty)
    {
        return L"IsAnchoredAtHorizontalExtent";
    }
    else if (dependencyProperty == s_IsAnchoredAtVerticalExtentProperty)
    {
        return L"IsAnchoredAtVerticalExtent";
    }
    else if (dependencyProperty == s_HorizontalAnchorRatioProperty)
    {
        return L"HorizontalAnchorRatio";
    }
    else if (dependencyProperty == s_VerticalAnchorRatioProperty)
    {
        return L"VerticalAnchorRatio";
    }
    else
    {
        return L"UNKNOWN";
    }
}

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ScrollPresenter.h"
#include "ScrollView.h"
#include "RuntimeProfiler.h"
#include "FocusHelper.h"
#include "RegUtil.h"
#include "ScrollViewTestHooks.h"

// Change to 'true' to turn on debugging outputs in Output window
bool ScrollViewTrace::s_IsDebugOutputEnabled{ false };
bool ScrollViewTrace::s_IsVerboseDebugOutputEnabled{ false };

const int32_t ScrollView::s_noOpCorrelationId{ -1 };

ScrollView::ScrollView()
{
    SCROLLVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    EnsureProperties();
    SetDefaultStyleKey(this);
    HookUISettingsEvent();
    HookScrollViewEvents();
}

ScrollView::~ScrollView()
{
    SCROLLVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookCompositionTargetRendering();
    UnhookScrollPresenterEvents(true /*isForDestructor*/);
    UnhookScrollViewEvents();
    ResetHideIndicatorsTimer(true /*isForDestructor*/);
}

#pragma region IScrollView

winrt::CompositionPropertySet ScrollView::ExpressionAnimationSources()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ExpressionAnimationSources();
    }

    return nullptr;
}

double ScrollView::HorizontalOffset()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.HorizontalOffset();
    }

    return 0.0;
}

double ScrollView::VerticalOffset()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.VerticalOffset();
    }

    return 0.0;
}

float ScrollView::ZoomFactor()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ZoomFactor();
    }

    return 0.0f;
}

double ScrollView::ExtentWidth()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ExtentWidth();
    }

    return 0.0;
}

double ScrollView::ExtentHeight()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ExtentHeight();
    }

    return 0.0;
}

double ScrollView::ViewportWidth()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ViewportWidth();
    }

    return 0.0;
}

double ScrollView::ViewportHeight()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ViewportHeight();
    }

    return 0.0;
}

double ScrollView::ScrollableWidth()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ScrollableWidth();
    }

    return 0.0;
}

double ScrollView::ScrollableHeight()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ScrollableHeight();
    }

    return 0.0;
}

winrt::ScrollingInteractionState ScrollView::State()
{
    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.State();
    }

    return winrt::ScrollingInteractionState::Idle;
}

winrt::ScrollingInputKinds ScrollView::IgnoredInputKinds()
{
    // Workaround for Bug 17377013: XamlCompiler codegen for Enum CreateFromString always returns boxed int which is wrong for [flags] enums (should be uint)
    // Check if the boxed IgnoredInputKinds is an IReference<int> first in which case we unbox as int.
    auto boxedKind = GetValue(s_IgnoredInputKindsProperty);
    if (auto boxedInt = boxedKind.try_as<winrt::IReference<int32_t>>())
    {
        return winrt::ScrollingInputKinds{ static_cast<uint32_t>(unbox_value<int32_t>(boxedInt)) };
    }

    return auto_unbox(boxedKind);
}

void ScrollView::IgnoredInputKinds(winrt::ScrollingInputKinds const& value)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::InputKindToString(value).c_str());
    SetValue(s_IgnoredInputKindsProperty, box_value(value));
}

void ScrollView::RegisterAnchorCandidate(winrt::UIElement const& element)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, element);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        if (const auto scrollPresenterAsAnchorProvider = scrollPresenter.try_as<winrt::Controls::IScrollAnchorProvider>())
        {
            scrollPresenterAsAnchorProvider.RegisterAnchorCandidate(element);
            return;
        }
        throw winrt::hresult_error(E_INVALID_OPERATION, s_IScrollAnchorProviderNotImpl);
    }
    throw winrt::hresult_error(E_INVALID_OPERATION, s_noScrollPresenterPart);
}

void ScrollView::UnregisterAnchorCandidate(winrt::UIElement const& element)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, element);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        if (const auto scrollPresenterAsAnchorProvider = scrollPresenter.try_as<winrt::Controls::IScrollAnchorProvider>())
        {
            scrollPresenterAsAnchorProvider.UnregisterAnchorCandidate(element);
            return;
        }
        throw winrt::hresult_error(E_INVALID_OPERATION, s_IScrollAnchorProviderNotImpl);
    }
    throw winrt::hresult_error(E_INVALID_OPERATION, s_noScrollPresenterPart);
}


int32_t ScrollView::ScrollTo(double horizontalOffset, double verticalOffset)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalOffset, verticalOffset);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ScrollTo(horizontalOffset, verticalOffset);
    }

    return s_noOpCorrelationId;
}

int32_t ScrollView::ScrollTo(double horizontalOffset, double verticalOffset, winrt::ScrollingScrollOptions const& options)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        horizontalOffset, verticalOffset, TypeLogging::ScrollOptionsToString(options).c_str());

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ScrollTo(horizontalOffset, verticalOffset, options);
    }

    return s_noOpCorrelationId;
}

int32_t ScrollView::ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalOffsetDelta, verticalOffsetDelta);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ScrollBy(horizontalOffsetDelta, verticalOffsetDelta);
    }

    return s_noOpCorrelationId;
}

int32_t ScrollView::ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta, winrt::ScrollingScrollOptions const& options)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        horizontalOffsetDelta, verticalOffsetDelta, TypeLogging::ScrollOptionsToString(options).c_str());

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ScrollBy(horizontalOffsetDelta, verticalOffsetDelta, options);
    }

    return s_noOpCorrelationId;
}

int32_t ScrollView::AddScrollVelocity(winrt::float2 offsetsVelocity, winrt::IReference<winrt::float2> inertiaDecayRate)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::Float2ToString(offsetsVelocity).c_str(), TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str());

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.AddScrollVelocity(offsetsVelocity, inertiaDecayRate);
    }

    return s_noOpCorrelationId;
}

int32_t ScrollView::ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(), zoomFactor);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ZoomTo(zoomFactor, centerPoint);
    }

    return s_noOpCorrelationId;
}

int32_t ScrollView::ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint, winrt::ScrollingZoomOptions const& options)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str(),
        zoomFactor);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ZoomTo(zoomFactor, centerPoint, options);
    }

    return s_noOpCorrelationId;
}

int32_t ScrollView::ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        zoomFactorDelta);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ZoomBy(zoomFactorDelta, centerPoint);
    }

    return s_noOpCorrelationId;
}

int32_t ScrollView::ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint, winrt::ScrollingZoomOptions const& options)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str(),
        zoomFactorDelta);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.ZoomBy(zoomFactorDelta, centerPoint, options);
    }

    return s_noOpCorrelationId;
}

int32_t ScrollView::AddZoomVelocity(float zoomFactorVelocity, winrt::IReference<winrt::float2> centerPoint, winrt::IReference<float> inertiaDecayRate)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::NullableFloatToString(inertiaDecayRate).c_str(),
        zoomFactorVelocity);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.AddZoomVelocity(zoomFactorVelocity, centerPoint, inertiaDecayRate);
    }

    return s_noOpCorrelationId;
}

#pragma endregion

#pragma region IFrameworkElementOverrides

void ScrollView::OnApplyTemplate()
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnApplyTemplate();

    m_hasNoIndicatorStateStoryboardCompletedHandler = false;
    m_keepIndicatorsShowing = false;

    winrt::IControlProtected thisAsControlProtected = *this;

    winrt::ScrollPresenter scrollPresenter = GetTemplateChildT<winrt::ScrollPresenter>(s_scrollPresenterPartName, thisAsControlProtected);

    UpdateScrollPresenter(scrollPresenter);

    winrt::IUIElement horizontalScrollControllerElement = GetTemplateChildT<winrt::IUIElement>(s_horizontalScrollBarPartName, thisAsControlProtected);
    winrt::IScrollController horizontalScrollController = horizontalScrollControllerElement.try_as<winrt::IScrollController>();
    winrt::ScrollBar horizontalScrollBar = nullptr;

    if (horizontalScrollControllerElement && !horizontalScrollController)
    {
        horizontalScrollBar = horizontalScrollControllerElement.try_as<winrt::ScrollBar>();

        if (horizontalScrollBar)
        {
            if (!m_horizontalScrollBarController)
            {
                m_horizontalScrollBarController = winrt::make_self<ScrollBarController>();
            }
            horizontalScrollController = m_horizontalScrollBarController.as<winrt::IScrollController>();
        }
    }

    if (horizontalScrollBar)
    {
        m_horizontalScrollBarController->SetScrollBar(horizontalScrollBar);
    }
    else
    {
        m_horizontalScrollBarController = nullptr;
    }

    UpdateHorizontalScrollController(horizontalScrollController, horizontalScrollControllerElement);

    winrt::IUIElement verticalScrollControllerElement = GetTemplateChildT<winrt::IUIElement>(s_verticalScrollBarPartName, thisAsControlProtected);
    winrt::IScrollController verticalScrollController = verticalScrollControllerElement.try_as<winrt::IScrollController>();
    winrt::ScrollBar verticalScrollBar = nullptr;

    if (verticalScrollControllerElement && !verticalScrollController)
    {
        verticalScrollBar = verticalScrollControllerElement.try_as<winrt::ScrollBar>();

        if (verticalScrollBar)
        {
            if (!m_verticalScrollBarController)
            {
                m_verticalScrollBarController = winrt::make_self<ScrollBarController>();
            }
            verticalScrollController = m_verticalScrollBarController.as<winrt::IScrollController>();
        }
    }

    if (verticalScrollBar)
    {
        m_verticalScrollBarController->SetScrollBar(verticalScrollBar);
    }
    else
    {
        m_verticalScrollBarController = nullptr;
    }

    UpdateVerticalScrollController(verticalScrollController, verticalScrollControllerElement);

    winrt::IUIElement scrollControllersSeparator = GetTemplateChildT<winrt::IUIElement>(s_scrollBarsSeparatorPartName, thisAsControlProtected);

    UpdateScrollControllersSeparator(scrollControllersSeparator);

    UpdateScrollControllersVisibility(true /*horizontalChange*/, true /*verticalChange*/);

    winrt::FrameworkElement root = GetTemplateChildT<winrt::FrameworkElement>(s_rootPartName, thisAsControlProtected);

    if (root)
    {
        winrt::IVector<winrt::VisualStateGroup> rootVisualStateGroups = winrt::VisualStateManager::GetVisualStateGroups(root);

        if (rootVisualStateGroups)
        {
            const uint32_t groupCount = rootVisualStateGroups.Size();

            for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
            {
                winrt::VisualStateGroup group = rootVisualStateGroups.GetAt(groupIndex);

                if (group)
                {
                    winrt::IVector<winrt::VisualState> visualStates = group.States();

                    if (visualStates)
                    {
                        const uint32_t stateCount = visualStates.Size();

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
                                        const winrt::event_token noIndicatorStateStoryboardCompletedToken = stateStoryboard.Completed({ this, &ScrollView::OnNoIndicatorStateStoryboardCompleted });
                                        m_hasNoIndicatorStateStoryboardCompletedHandler = true;
                                    }
                                    else if (stateName == s_touchIndicatorStateName || stateName == s_mouseIndicatorStateName)
                                    {
                                        const winrt::event_token indicatorStateStoryboardCompletedToken = stateStoryboard.Completed({ this, &ScrollView::OnIndicatorStateStoryboardCompleted });
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    UpdateVisualStates(false /*useTransitions*/);
}

#pragma endregion

#pragma region IControlOverrides

void ScrollView::OnGotFocus(winrt::RoutedEventArgs const& args)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnGotFocus(args);

    m_preferMouseIndicators =
        m_focusInputDeviceKind == winrt::FocusInputDeviceKind::Mouse ||
        m_focusInputDeviceKind == winrt::FocusInputDeviceKind::Pen;

    UpdateVisualStates(
        true  /*useTransitions*/,
        true  /*showIndicators*/,
        false /*hideIndicators*/,
        false /*scrollControllersAutoHidingChanged*/,
        true  /*updateScrollControllersAutoHiding*/,
        true  /*onlyForAutoHidingScrollControllers*/);
}

#pragma endregion

void ScrollView::OnScrollViewGettingFocus(
    const winrt::IInspectable& /*sender*/,
    const winrt::GettingFocusEventArgs& args)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_focusInputDeviceKind = args.InputDevice();
}

void ScrollView::OnScrollViewIsEnabledChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateVisualStates(
        true  /*useTransitions*/,
        false /*showIndicators*/,
        false /*hideIndicators*/,
        false /*scrollControllersAutoHidingChanged*/,
        true  /*updateScrollControllersAutoHiding*/);
}

void ScrollView::OnScrollViewUnloaded(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_showingMouseIndicators = false;
    m_keepIndicatorsShowing = false;
    m_bringIntoViewOperations.clear();

    UnhookCompositionTargetRendering();
    ResetHideIndicatorsTimer();
}

void ScrollView::OnScrollViewPointerEntered(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Pointer().PointerDeviceType() != winrt::PointerDeviceType::Touch)
    {
        // Mouse/Pen inputs dominate. If touch panning indicators are shown, switch to mouse indicators.
        m_preferMouseIndicators = true;

        UpdateVisualStates(
            true  /*useTransitions*/,
            true  /*showIndicators*/,
            false /*hideIndicators*/,
            false /*scrollControllersAutoHidingChanged*/,
            true  /*updateScrollControllersAutoHiding*/,
            true  /*onlyForAutoHidingScrollControllers*/);
    }
}

void ScrollView::OnScrollViewPointerMoved(
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

        UpdateVisualStates(
            true  /*useTransitions*/,
            true  /*showIndicators*/,
            false /*hideIndicators*/,
            false /*scrollControllersAutoHidingChanged*/,
            false /*updateScrollControllersAutoHiding*/,
            true  /*onlyForAutoHidingScrollControllers*/);

        if (AreScrollControllersAutoHiding() &&
            !SharedHelpers::IsAnimationsEnabled() &&
            m_hideIndicatorsTimer &&
            (m_isPointerOverHorizontalScrollController || m_isPointerOverVerticalScrollController))
        {
            ResetHideIndicatorsTimer();
        }
    }
}

void ScrollView::OnScrollViewPointerExited(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Pointer().PointerDeviceType() != winrt::PointerDeviceType::Touch)
    {
        // Mouse/Pen inputs dominate. If touch panning indicators are shown, switch to mouse indicators.
        m_isPointerOverHorizontalScrollController = false;
        m_isPointerOverVerticalScrollController = false;
        m_preferMouseIndicators = true;

        UpdateVisualStates(
            true  /*useTransitions*/,
            true  /*showIndicators*/,
            false /*hideIndicators*/,
            false /*scrollControllersAutoHidingChanged*/,
            true  /*updateScrollControllersAutoHiding*/,
            true  /*onlyForAutoHidingScrollControllers*/);

        if (AreScrollControllersAutoHiding())
        {
            HideIndicatorsAfterDelay();
        }
    }
}

void ScrollView::OnScrollViewPointerPressed(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

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

    // Show the scroll controller indicators as soon as a pointer is pressed on the ScrollView.
    UpdateVisualStates(
        true  /*useTransitions*/,
        true  /*showIndicators*/,
        false /*hideIndicators*/,
        false /*scrollControllersAutoHidingChanged*/,
        true  /*updateScrollControllersAutoHiding*/,
        true  /*onlyForAutoHidingScrollControllers*/);
}

void ScrollView::OnScrollViewPointerReleased(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

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
        SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_METH, METH_NAME, this, L"Focus");

        const bool tookFocus = Focus(winrt::FocusState::Pointer);
        args.Handled(tookFocus);
    }
}

void ScrollView::OnScrollViewPointerCanceled(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Mouse)
    {
        m_isLeftMouseButtonPressedForFocus = false;
    }
}

void ScrollView::OnHorizontalScrollControllerIsEnabledChanged(
    const winrt::IInspectable& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateAreHorizontalScrollControllerInteractionsAllowed();
}

void ScrollView::OnHorizontalScrollControllerPointerEntered(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverHorizontalScrollController = true;
}

void ScrollView::OnHorizontalScrollControllerPointerExited(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverHorizontalScrollController = false;
}

void ScrollView::OnVerticalScrollControllerIsEnabledChanged(
    const winrt::IInspectable& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateAreVerticalScrollControllerInteractionsAllowed();
}

void ScrollView::OnVerticalScrollControllerPointerEntered(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverVerticalScrollController = true;

    UpdateScrollControllersAutoHiding();
    if (AreScrollControllersAutoHiding() && !SharedHelpers::IsAnimationsEnabled())
    {
        HideIndicatorsAfterDelay();
    }
}

void ScrollView::OnVerticalScrollControllerPointerExited(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverVerticalScrollController = false;

    UpdateScrollControllersAutoHiding();
    if (AreScrollControllersAutoHiding())
    {
        HideIndicatorsAfterDelay();
    }
}

// Handler for when the NoIndicator state's storyboard completes animating.
void ScrollView::OnNoIndicatorStateStoryboardCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_hasNoIndicatorStateStoryboardCompletedHandler);

    m_showingMouseIndicators = false;
}

// Handler for when a TouchIndicator or MouseIndicator state's storyboard completes animating.
void ScrollView::OnIndicatorStateStoryboardCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    // If the cursor is currently directly over either scroll controller then do not automatically hide the indicators
    if (AreScrollControllersAutoHiding() &&
        !m_keepIndicatorsShowing &&
        !m_isPointerOverVerticalScrollController &&
        !m_isPointerOverHorizontalScrollController)
    {
        UpdateScrollControllersVisualState(true /*useTransitions*/, false /*showIndicators*/, true /*hideIndicators*/);
    }
}

// Invoked by ScrollViewTestHooks
void ScrollView::ScrollControllersAutoHidingChanged()
{
    if (SharedHelpers::IsRS4OrHigher())
    {
        UpdateScrollControllersAutoHiding(true /*forceUpdate*/);
    }
}

winrt::ScrollPresenter ScrollView::GetScrollPresenterPart() const
{
    return m_scrollPresenter.get().as<winrt::ScrollPresenter>();
}

void ScrollView::ValidateAnchorRatio(double value)
{
    ScrollPresenter::ValidateAnchorRatio(value);
}

void ScrollView::ValidateZoomFactoryBoundary(double value)
{
    ScrollPresenter::ValidateZoomFactoryBoundary(value);
}

// Invoked when a dependency property of this ScrollView has changed.
void ScrollView::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto dependencyProperty = args.Property();

#ifdef _DEBUG
    SCROLLVIEW_TRACE_VERBOSE(nullptr, L"%s(property: %s)\n", METH_NAME, DependencyPropertyToString(dependencyProperty).c_str());
#endif

    bool horizontalChange = dependencyProperty == s_HorizontalScrollBarVisibilityProperty;
    bool verticalChange = dependencyProperty == s_VerticalScrollBarVisibilityProperty;

    if (horizontalChange || verticalChange)
    {
        UpdateScrollControllersVisibility(horizontalChange, verticalChange);
        UpdateVisualStates(
            true  /*useTransitions*/,
            false /*showIndicators*/,
            false /*hideIndicators*/,
            false /*scrollControllersAutoHidingChanged*/,
            true  /*updateScrollControllersAutoHiding*/);
    }
}

// Invoked by an IScrollController implementation when one or more of its characteristics has changed:
// IsScrolling, InteractionElement, InteractionElementScrollOrientation
// #ifdef USE_SCROLLCONTROLLER_ISINTERACTIONELEMENTRAILENABLED IsInteractionElementRailEnabled
// #ifdef USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED AreScrollControllerInteractionsAllowed
// #ifdef USE_SCROLLCONTROLLER_ARESCROLLERINTERACTIONSALLOWED AreScrollerInteractionsAllowed
void ScrollView::OnScrollControllerInteractionInfoChanged(
    const winrt::IScrollController& sender,
    const winrt::IInspectable& /*args*/)
{
    const bool isScrollControllerScrolling = sender.IsScrolling();
    bool showIndicators = false;
    bool hideIndicators = false;

    if (m_horizontalScrollController && m_horizontalScrollController == sender)
    {
        UpdateScrollControllersAutoHiding();

        if (m_isHorizontalScrollControllerScrolling != isScrollControllerScrolling)
        {
            SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"HorizontalScrollController.IsScrolling changed: ", m_isHorizontalScrollControllerScrolling, isScrollControllerScrolling);

            m_isHorizontalScrollControllerScrolling = isScrollControllerScrolling;

            if (isScrollControllerScrolling)
            {
                // Prevent the vertical scroll controller from fading out while the user is interacting with the horizontal one.
                m_keepIndicatorsShowing = true;
                showIndicators = true;
            }
            else
            {
                // Make the scroll controllers fade out, after the normal delay, if they are auto-hiding.
                m_keepIndicatorsShowing = false;
                hideIndicators = AreScrollControllersAutoHiding();
            }
        }

#ifdef USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
        // IScrollController::AreScrollControllerInteractionsAllowed might have changed and affect the scroll controller's visibility
        // when its visibility mode is Auto.
        UpdateScrollControllersVisibility(true /*horizontalChange*/, false /*verticalChange*/);
#endif
        UpdateVisualStates(true /*useTransitions*/, showIndicators, hideIndicators);
    }
    else if (m_verticalScrollController && m_verticalScrollController == sender)
    {
        UpdateScrollControllersAutoHiding();

        if (m_isVerticalScrollControllerScrolling != isScrollControllerScrolling)
        {
            SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"VerticalScrollController.IsScrolling changed: ", m_isVerticalScrollControllerScrolling, isScrollControllerScrolling);

            m_isVerticalScrollControllerScrolling = isScrollControllerScrolling;

            if (isScrollControllerScrolling)
            {
                // Prevent the horizontal scroll controller from fading out while the user is interacting with the vertical one.
                m_keepIndicatorsShowing = true;
                showIndicators = true;
            }
            else
            {
                // Make the scroll controllers fade out, after the normal delay, if they are auto-hiding.
                m_keepIndicatorsShowing = false;
                hideIndicators = AreScrollControllersAutoHiding();
            }
        }

#ifdef USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
        // IScrollController::AreScrollControllerInteractionsAllowed might have changed and affect the scroll controller's visibility
        // when its visibility mode is Auto.
        UpdateScrollControllersVisibility(false /*horizontalChange*/, true /*verticalChange*/);
#endif
        UpdateVisualStates(true /*useTransitions*/, showIndicators, hideIndicators);
    }
}

void ScrollView::OnHideIndicatorsTimerTick(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    ResetHideIndicatorsTimer();

    if (AreScrollControllersAutoHiding())
    {
        HideIndicators();
    }
}

void ScrollView::OnAutoHideScrollBarsChanged(
    winrt::UISettings const& uiSettings,
    winrt::UISettingsAutoHideScrollBarsChangedEventArgs const& args)
{
    MUX_ASSERT(SharedHelpers::Is19H1OrHigher());

    // OnAutoHideScrollBarsChanged is called on a non-UI thread, process notification on the UI thread using a dispatcher.
    m_dispatcherHelper.RunAsync([strongThis = get_strong()]()
    {
        strongThis->m_autoHideScrollControllersValid = false;
        strongThis->UpdateVisualStates(
            true  /*useTransitions*/,
            false /*showIndicators*/,
            false /*hideIndicators*/,
            true  /*scrollControllersAutoHidingChanged*/);
    });
}

void ScrollView::OnScrollPresenterSizeChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateAreHorizontalScrollControllerInteractionsAllowed();
    UpdateAreVerticalScrollControllerInteractionsAllowed();
}

void ScrollView::OnScrollPresenterExtentChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    UpdateAreHorizontalScrollControllerInteractionsAllowed();
    UpdateAreVerticalScrollControllerInteractionsAllowed();

    if (m_extentChangedEventSource)
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_extentChangedEventSource(*this, args);
    }
}

void ScrollView::OnScrollPresenterStateChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        if (scrollPresenter.State() == winrt::ScrollingInteractionState::Interaction)
        {
            m_preferMouseIndicators = false;
        }
    }

    if (m_stateChangedEventSource)
    {
        m_stateChangedEventSource(*this, args);
    }
}

void ScrollView::OnScrollAnimationStarting(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingScrollAnimationStartingEventArgs& args)
{
    if (m_scrollAnimationStartingEventSource)
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_scrollAnimationStartingEventSource(*this, args);
    }
}

void ScrollView::OnZoomAnimationStarting(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingZoomAnimationStartingEventArgs& args)
{
    if (m_zoomAnimationStartingEventSource)
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_zoomAnimationStartingEventSource(*this, args);
    }
}

void ScrollView::OnScrollPresenterViewChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    // Unless the control is still loading, show the scroll controller indicators when the view changes. For example,
    // when using Ctrl+/- to zoom, mouse-wheel to scroll or zoom, or any other input type. Keep the existing indicator type.
    if (SharedHelpers::IsFrameworkElementLoaded(*this))
    {
        UpdateVisualStates(
            true  /*useTransitions*/,
            true  /*showIndicators*/,
            false /*hideIndicators*/,
            false /*scrollControllersAutoHidingChanged*/,
            false /*updateScrollControllersAutoHiding*/,
            true  /*onlyForAutoHidingScrollControllers*/);
    }

    if (m_viewChangedEventSource)
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_viewChangedEventSource(*this, args);
    }
}

void ScrollView::OnScrollPresenterScrollCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingScrollCompletedEventArgs& args)
{
    if (args.CorrelationId() == m_horizontalAddScrollVelocityOffsetChangeCorrelationId)
    {
        m_horizontalAddScrollVelocityOffsetChangeCorrelationId = s_noOpCorrelationId;
    }
    else if (args.CorrelationId() == m_verticalAddScrollVelocityOffsetChangeCorrelationId)
    {
        m_verticalAddScrollVelocityOffsetChangeCorrelationId = s_noOpCorrelationId;
    }

    if (m_scrollCompletedEventSource)
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_scrollCompletedEventSource(*this, args);
    }
}

void ScrollView::OnScrollPresenterZoomCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingZoomCompletedEventArgs& args)
{
    if (m_zoomCompletedEventSource)
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_zoomCompletedEventSource(*this, args);
    }
}

void ScrollView::OnScrollPresenterBringingIntoView(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingBringingIntoViewEventArgs& args)
{
    if (!m_bringIntoViewOperations.empty())
    {
        auto requestEventArgs = args.RequestEventArgs();

        for (auto operationsIter = m_bringIntoViewOperations.begin(); operationsIter != m_bringIntoViewOperations.end(); operationsIter++)
        {
            auto& bringIntoViewOperation = *operationsIter;

            if (requestEventArgs.TargetElement() == bringIntoViewOperation->TargetElement())
            {
                // This ScrollPresenter::BringingIntoView notification results from a FocusManager::TryFocusAsync call in ScrollView::HandleKeyDownForXYNavigation.
                // Its BringIntoViewRequestedEventArgs::AnimationDesired property is set to True in order to animate to the target element rather than jumping.
                SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR_INT, METH_NAME, this, bringIntoViewOperation->TargetElement(), bringIntoViewOperation->TicksCount());

                requestEventArgs.AnimationDesired(true);
                break;
            }
        }
    }

    if (m_bringingIntoViewEventSource)
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_bringingIntoViewEventSource(*this, args);
    }
}

void ScrollView::OnScrollPresenterAnchorRequested(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingAnchorRequestedEventArgs& args)
{
    if (m_anchorRequestedEventSource)
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_anchorRequestedEventSource(*this, args);
    }
}

void ScrollView::OnCompositionTargetRendering(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!m_bringIntoViewOperations.empty())
    {
        for (auto operationsIter = m_bringIntoViewOperations.begin(); operationsIter != m_bringIntoViewOperations.end();)
        {
            auto& bringIntoViewOperation = *operationsIter;
            operationsIter++;

            SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR_INT, METH_NAME, this, bringIntoViewOperation->TargetElement(), bringIntoViewOperation->TicksCount());

            if (bringIntoViewOperation->HasMaxTicksCount())
            {
                // This ScrollView is no longer expected to receive BringingIntoView notifications from its ScrollPresenter,
                // resulting from a FocusManager::TryFocusAsync call in ScrollView::HandleKeyDownForXYNavigation.
                m_bringIntoViewOperations.remove(bringIntoViewOperation);
            }
            else
            {
                // Increment the number of ticks ellapsed since the FocusManager::TryFocusAsync call, and continue to wait for BringingIntoView notifications.
                bringIntoViewOperation->TickOperation();
            }
        }
    }

    if (m_bringIntoViewOperations.empty())
    {
        UnhookCompositionTargetRendering();
    }
}

void ScrollView::OnScrollPresenterPropertyChanged(
    const winrt::DependencyObject& /*sender*/,
    const winrt::DependencyProperty& args)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (args == winrt::ScrollPresenter::HorizontalScrollModeProperty())
    {
        UpdateAreHorizontalScrollControllerInteractionsAllowed();
    }
    else if (args == winrt::ScrollPresenter::VerticalScrollModeProperty())
    {
        UpdateAreVerticalScrollControllerInteractionsAllowed();
    }

#ifdef USE_SCROLLMODE_AUTO
    if (args == winrt::ScrollPresenter::ComputedHorizontalScrollModeProperty())
    {
        SetValue(s_ComputedHorizontalScrollModeProperty, box_value(m_scrollPresenter.get().ComputedHorizontalScrollMode()));
    }
    else if (args == winrt::ScrollPresenter::ComputedVerticalScrollModeProperty())
    {
        SetValue(s_ComputedVerticalScrollModeProperty, box_value(m_scrollPresenter.get().ComputedVerticalScrollMode()));
    }
#endif
}

void ScrollView::ResetHideIndicatorsTimer(bool isForDestructor, bool restart)
{
    auto hideIndicatorsTimer = m_hideIndicatorsTimer.safe_get(isForDestructor /*useSafeGet*/);

    if (hideIndicatorsTimer && hideIndicatorsTimer.IsEnabled())
    {
        hideIndicatorsTimer.Stop();
        if (restart)
        {
            hideIndicatorsTimer.Start();
        }
    }
}

void ScrollView::HookUISettingsEvent()
{
    // Introduced in 19H1, IUISettings5 exposes the AutoHideScrollBars property and AutoHideScrollBarsChanged event.
    if (!m_uiSettings5)
    {
        winrt::UISettings uiSettings;

        m_uiSettings5 = uiSettings.try_as<winrt::IUISettings5>();
        if (m_uiSettings5)
        {
            m_autoHideScrollBarsChangedRevoker = m_uiSettings5.AutoHideScrollBarsChanged(
                winrt::auto_revoke,
                { this, &ScrollView::OnAutoHideScrollBarsChanged });
        }
    }
}

void ScrollView::HookCompositionTargetRendering()
{
    if (!m_renderingToken)
    {
        winrt::Windows::UI::Xaml::Media::CompositionTarget compositionTarget{ nullptr };
        m_renderingToken = compositionTarget.Rendering(winrt::auto_revoke, { this, &ScrollView::OnCompositionTargetRendering });
    }
}

void ScrollView::UnhookCompositionTargetRendering()
{
    m_renderingToken.revoke();
}

void ScrollView::HookScrollViewEvents()
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

    m_gettingFocusToken = GettingFocus({ this, &ScrollView::OnScrollViewGettingFocus });
    m_isEnabledChangedToken = IsEnabledChanged({ this, &ScrollView::OnScrollViewIsEnabledChanged });
    m_unloadedToken = Unloaded({ this, &ScrollView::OnScrollViewUnloaded });

    m_onPointerEnteredEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollView::OnScrollViewPointerEntered });
    AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onPointerEnteredEventHandler, false);

    m_onPointerMovedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollView::OnScrollViewPointerMoved });
    AddHandler(winrt::UIElement::PointerMovedEvent(), m_onPointerMovedEventHandler, false);

    m_onPointerExitedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollView::OnScrollViewPointerExited });
    AddHandler(winrt::UIElement::PointerExitedEvent(), m_onPointerExitedEventHandler, false);

    m_onPointerPressedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollView::OnScrollViewPointerPressed });
    AddHandler(winrt::UIElement::PointerPressedEvent(), m_onPointerPressedEventHandler, false);

    m_onPointerReleasedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollView::OnScrollViewPointerReleased });
    AddHandler(winrt::UIElement::PointerReleasedEvent(), m_onPointerReleasedEventHandler, true);

    m_onPointerCanceledEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollView::OnScrollViewPointerCanceled });
    AddHandler(winrt::UIElement::PointerCanceledEvent(), m_onPointerCanceledEventHandler, true);
}

void ScrollView::UnhookScrollViewEvents()
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

void ScrollView::HookScrollPresenterEvents()
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_scrollPresenterSizeChangedToken.value == 0);
    MUX_ASSERT(m_scrollPresenterExtentChangedToken.value == 0);
    MUX_ASSERT(m_scrollPresenterStateChangedToken.value == 0);
    MUX_ASSERT(m_scrollPresenterScrollAnimationStartingToken.value == 0);
    MUX_ASSERT(m_scrollPresenterZoomAnimationStartingToken.value == 0);
    MUX_ASSERT(m_scrollPresenterViewChangedToken.value == 0);
    MUX_ASSERT(m_scrollPresenterScrollCompletedToken.value == 0);
    MUX_ASSERT(m_scrollPresenterZoomCompletedToken.value == 0);
    MUX_ASSERT(m_scrollPresenterBringingIntoViewToken.value == 0);
    MUX_ASSERT(m_scrollPresenterAnchorRequestedToken.value == 0);
    MUX_ASSERT(m_scrollPresenterHorizontalScrollModeChangedToken.value == 0);
    MUX_ASSERT(m_scrollPresenterVerticalScrollModeChangedToken.value == 0);
#ifdef USE_SCROLLMODE_AUTO
    MUX_ASSERT(m_scrollPresenterComputedHorizontalScrollModeChangedToken.value == 0);
    MUX_ASSERT(m_scrollPresenterComputedVerticalScrollModeChangedToken.value == 0);
#endif

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        auto scrollPresenterFE = scrollPresenter.try_as<winrt::FrameworkElement>();

        m_scrollPresenterSizeChangedToken = scrollPresenterFE.SizeChanged({ this, &ScrollView::OnScrollPresenterSizeChanged });
        m_scrollPresenterExtentChangedToken = scrollPresenter.ExtentChanged({ this, &ScrollView::OnScrollPresenterExtentChanged });
        m_scrollPresenterStateChangedToken = scrollPresenter.StateChanged({ this, &ScrollView::OnScrollPresenterStateChanged });
        m_scrollPresenterScrollAnimationStartingToken = scrollPresenter.ScrollAnimationStarting({ this, &ScrollView::OnScrollAnimationStarting });
        m_scrollPresenterZoomAnimationStartingToken = scrollPresenter.ZoomAnimationStarting({ this, &ScrollView::OnZoomAnimationStarting });
        m_scrollPresenterViewChangedToken = scrollPresenter.ViewChanged({ this, &ScrollView::OnScrollPresenterViewChanged });
        m_scrollPresenterScrollCompletedToken = scrollPresenter.ScrollCompleted({ this, &ScrollView::OnScrollPresenterScrollCompleted });
        m_scrollPresenterZoomCompletedToken = scrollPresenter.ZoomCompleted({ this, &ScrollView::OnScrollPresenterZoomCompleted });
        m_scrollPresenterBringingIntoViewToken = scrollPresenter.BringingIntoView({ this, &ScrollView::OnScrollPresenterBringingIntoView });
        m_scrollPresenterAnchorRequestedToken = scrollPresenter.AnchorRequested({ this, &ScrollView::OnScrollPresenterAnchorRequested });

        const winrt::DependencyObject scrollPresenterAsDO = scrollPresenter.try_as<winrt::DependencyObject>();

        m_scrollPresenterHorizontalScrollModeChangedToken.value = scrollPresenterAsDO.RegisterPropertyChangedCallback(
            winrt::ScrollPresenter::HorizontalScrollModeProperty(), { this, &ScrollView::OnScrollPresenterPropertyChanged });

        m_scrollPresenterVerticalScrollModeChangedToken.value = scrollPresenterAsDO.RegisterPropertyChangedCallback(
            winrt::ScrollPresenter::VerticalScrollModeProperty(), { this, &ScrollView::OnScrollPresenterPropertyChanged });

#ifdef USE_SCROLLMODE_AUTO
        m_scrollPresenterComputedHorizontalScrollModeChangedToken.value = scrollPresenterAsDO.RegisterPropertyChangedCallback(
            winrt::ScrollPresenter::ComputedHorizontalScrollModeProperty(), { this, &ScrollView::OnScrollPresenterPropertyChanged });

        m_scrollPresenterComputedVerticalScrollModeChangedToken.value = scrollPresenterAsDO.RegisterPropertyChangedCallback(
            winrt::ScrollPresenter::ComputedVerticalScrollModeProperty(), { this, &ScrollView::OnScrollPresenterPropertyChanged });
#endif
    }
}

void ScrollView::UnhookScrollPresenterEvents(bool isForDestructor)
{
    if (isForDestructor)
    {
        SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }
    else
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);
    }

    if (auto scrollPresenter = m_scrollPresenter.safe_get())
    {
        if (m_scrollPresenterSizeChangedToken.value != 0)
        {
            auto scrollPresenterFE = scrollPresenter.try_as<winrt::FrameworkElement>();

            scrollPresenterFE.SizeChanged(m_scrollPresenterSizeChangedToken);
            m_scrollPresenterSizeChangedToken.value = 0;
        }

        if (m_scrollPresenterExtentChangedToken.value != 0)
        {
            scrollPresenter.ExtentChanged(m_scrollPresenterExtentChangedToken);
            m_scrollPresenterExtentChangedToken.value = 0;
        }

        if (m_scrollPresenterStateChangedToken.value != 0)
        {
            scrollPresenter.StateChanged(m_scrollPresenterStateChangedToken);
            m_scrollPresenterStateChangedToken.value = 0;
        }

        if (m_scrollPresenterScrollAnimationStartingToken.value != 0)
        {
            scrollPresenter.ScrollAnimationStarting(m_scrollPresenterScrollAnimationStartingToken);
            m_scrollPresenterScrollAnimationStartingToken.value = 0;
        }

        if (m_scrollPresenterZoomAnimationStartingToken.value != 0)
        {
            scrollPresenter.ZoomAnimationStarting(m_scrollPresenterZoomAnimationStartingToken);
            m_scrollPresenterZoomAnimationStartingToken.value = 0;
        }

        if (m_scrollPresenterViewChangedToken.value != 0)
        {
            scrollPresenter.ViewChanged(m_scrollPresenterViewChangedToken);
            m_scrollPresenterViewChangedToken.value = 0;
        }

        if (m_scrollPresenterScrollCompletedToken.value != 0)
        {
            scrollPresenter.ScrollCompleted(m_scrollPresenterScrollCompletedToken);
            m_scrollPresenterScrollCompletedToken.value = 0;
        }

        if (m_scrollPresenterZoomCompletedToken.value != 0)
        {
            scrollPresenter.ZoomCompleted(m_scrollPresenterZoomCompletedToken);
            m_scrollPresenterZoomCompletedToken.value = 0;
        }

        if (m_scrollPresenterBringingIntoViewToken.value != 0)
        {
            scrollPresenter.BringingIntoView(m_scrollPresenterBringingIntoViewToken);
            m_scrollPresenterBringingIntoViewToken.value = 0;
        }

        if (m_scrollPresenterAnchorRequestedToken.value != 0)
        {
            scrollPresenter.AnchorRequested(m_scrollPresenterAnchorRequestedToken);
            m_scrollPresenterAnchorRequestedToken.value = 0;
        }

        const winrt::DependencyObject scrollPresenterAsDO = scrollPresenter.try_as<winrt::DependencyObject>();

        if (m_scrollPresenterHorizontalScrollModeChangedToken.value != 0)
        {
            scrollPresenterAsDO.UnregisterPropertyChangedCallback(winrt::ScrollPresenter::HorizontalScrollModeProperty(), m_scrollPresenterHorizontalScrollModeChangedToken.value);
            m_scrollPresenterHorizontalScrollModeChangedToken.value = 0;
        }

        if (m_scrollPresenterVerticalScrollModeChangedToken.value != 0)
        {
            scrollPresenterAsDO.UnregisterPropertyChangedCallback(winrt::ScrollPresenter::VerticalScrollModeProperty(), m_scrollPresenterVerticalScrollModeChangedToken.value);
            m_scrollPresenterVerticalScrollModeChangedToken.value = 0;
        }

#ifdef USE_SCROLLMODE_AUTO
        if (m_scrollPresenterComputedHorizontalScrollModeChangedToken.value != 0)
        {
            scrollPresenterAsDO.UnregisterPropertyChangedCallback(winrt::ScrollPresenter::ComputedHorizontalScrollModeProperty(), m_scrollPresenterComputedHorizontalScrollModeChangedToken.value);
            m_scrollPresenterComputedHorizontalScrollModeChangedToken.value = 0;
        }

        if (m_scrollPresenterComputedVerticalScrollModeChangedToken.value != 0)
        {
            scrollPresenterAsDO.UnregisterPropertyChangedCallback(winrt::ScrollPresenter::ComputedVerticalScrollModeProperty(), m_scrollPresenterComputedVerticalScrollModeChangedToken.value);
            m_scrollPresenterComputedVerticalScrollModeChangedToken.value = 0;
        }
#endif
    }
}

void ScrollView::HookHorizontalScrollControllerEvents()
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_horizontalScrollControllerIsEnabledChangedToken.value == 0);
    MUX_ASSERT(m_horizontalScrollControllerInteractionInfoChangedToken.value == 0);
    MUX_ASSERT(!m_onHorizontalScrollControllerPointerEnteredHandler);
    MUX_ASSERT(!m_onHorizontalScrollControllerPointerExitedHandler);

    if (winrt::IScrollController horizontalScrollController = m_horizontalScrollController.get())
    {
        m_horizontalScrollControllerInteractionInfoChangedToken =
            horizontalScrollController.InteractionInfoChanged({ this, &ScrollView::OnScrollControllerInteractionInfoChanged });
    }

    if (winrt::IUIElement horizontalScrollControllerElement = m_horizontalScrollControllerElement.get())
    {
        if (auto horizontalScrollControllerControl = horizontalScrollControllerElement.try_as<winrt::Control>())
        {
            m_horizontalScrollControllerIsEnabledChangedToken =
                horizontalScrollControllerControl.IsEnabledChanged({ this, &ScrollView::OnHorizontalScrollControllerIsEnabledChanged });
        }

        m_onHorizontalScrollControllerPointerEnteredHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollView::OnHorizontalScrollControllerPointerEntered });
        horizontalScrollControllerElement.AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onHorizontalScrollControllerPointerEnteredHandler, true);

        m_onHorizontalScrollControllerPointerExitedHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollView::OnHorizontalScrollControllerPointerExited });
        horizontalScrollControllerElement.AddHandler(winrt::UIElement::PointerExitedEvent(), m_onHorizontalScrollControllerPointerExitedHandler, true);
    }
}

void ScrollView::UnhookHorizontalScrollControllerEvents()
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (winrt::IScrollController horizontalScrollController = m_horizontalScrollController.safe_get())
    {
        if (m_horizontalScrollControllerInteractionInfoChangedToken.value != 0)
        {
            horizontalScrollController.InteractionInfoChanged(m_horizontalScrollControllerInteractionInfoChangedToken);
            m_horizontalScrollControllerInteractionInfoChangedToken.value = 0;
        }
    }

    if (winrt::IUIElement horizontalScrollControllerElement = m_horizontalScrollControllerElement.safe_get())
    {
        if (auto horizontalScrollControllerControl = horizontalScrollControllerElement.try_as<winrt::Control>())
        {
            if (m_horizontalScrollControllerIsEnabledChangedToken.value != 0)
            {
                horizontalScrollControllerControl.IsEnabledChanged(m_horizontalScrollControllerIsEnabledChangedToken);
                m_horizontalScrollControllerIsEnabledChangedToken.value = 0;
            }
        }

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

void ScrollView::HookVerticalScrollControllerEvents()
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_verticalScrollControllerIsEnabledChangedToken.value == 0);
    MUX_ASSERT(m_verticalScrollControllerInteractionInfoChangedToken.value == 0);
    MUX_ASSERT(!m_onVerticalScrollControllerPointerEnteredHandler);
    MUX_ASSERT(!m_onVerticalScrollControllerPointerExitedHandler);

    if (winrt::IScrollController verticalScrollController = m_verticalScrollController.get())
    {
        m_verticalScrollControllerInteractionInfoChangedToken =
            verticalScrollController.InteractionInfoChanged({ this, &ScrollView::OnScrollControllerInteractionInfoChanged });
    }

    if (winrt::IUIElement verticalScrollControllerElement = m_verticalScrollControllerElement.get())
    {
        if (auto verticalScrollControllerControl = verticalScrollControllerElement.try_as<winrt::Control>())
        {
            m_verticalScrollControllerIsEnabledChangedToken =
                verticalScrollControllerControl.IsEnabledChanged({ this, &ScrollView::OnVerticalScrollControllerIsEnabledChanged });
        }

        m_onVerticalScrollControllerPointerEnteredHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollView::OnVerticalScrollControllerPointerEntered });
        verticalScrollControllerElement.AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onVerticalScrollControllerPointerEnteredHandler, true);

        m_onVerticalScrollControllerPointerExitedHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollView::OnVerticalScrollControllerPointerExited });
        verticalScrollControllerElement.AddHandler(winrt::UIElement::PointerExitedEvent(), m_onVerticalScrollControllerPointerExitedHandler, true);
    }
}

void ScrollView::UnhookVerticalScrollControllerEvents()
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (winrt::IScrollController verticalScrollController = m_verticalScrollController.safe_get())
    {
        if (m_verticalScrollControllerInteractionInfoChangedToken.value != 0)
        {
            verticalScrollController.InteractionInfoChanged(m_verticalScrollControllerInteractionInfoChangedToken);
            m_verticalScrollControllerInteractionInfoChangedToken.value = 0;
        }
    }

    if (winrt::IUIElement verticalScrollControllerElement = m_verticalScrollControllerElement.safe_get())
    {
        if (auto verticalScrollControllerControl = verticalScrollControllerElement.try_as<winrt::Control>())
        {
            if (m_verticalScrollControllerIsEnabledChangedToken.value != 0)
            {
                verticalScrollControllerControl.IsEnabledChanged(m_verticalScrollControllerIsEnabledChangedToken);
                m_verticalScrollControllerIsEnabledChangedToken.value = 0;
            }
        }

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

void ScrollView::UpdateScrollPresenter(const winrt::ScrollPresenter& scrollPresenter)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollPresenterEvents(false /*isForDestructor*/);
    m_scrollPresenter.set(nullptr);

    SetValue(s_ScrollPresenterProperty, scrollPresenter);

    if (scrollPresenter)
    {
        m_scrollPresenter.set(scrollPresenter);
        HookScrollPresenterEvents();
    }
}

void ScrollView::UpdateHorizontalScrollController(
    const winrt::IScrollController& horizontalScrollController,
    const winrt::IUIElement& horizontalScrollControllerElement)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookHorizontalScrollControllerEvents();

    m_horizontalScrollController.set(horizontalScrollController);
    m_horizontalScrollControllerElement.set(horizontalScrollControllerElement);
    HookHorizontalScrollControllerEvents();
    UpdateScrollPresenterHorizontalScrollController(horizontalScrollController);
}

void ScrollView::UpdateScrollPresenterHorizontalScrollController(const winrt::IScrollController& horizontalScrollController)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        scrollPresenter.HorizontalScrollController(horizontalScrollController);
    }
}

void ScrollView::UpdateVerticalScrollController(
    const winrt::IScrollController& verticalScrollController,
    const winrt::IUIElement& verticalScrollControllerElement)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookVerticalScrollControllerEvents();

    m_verticalScrollController.set(verticalScrollController);
    m_verticalScrollControllerElement.set(verticalScrollControllerElement);
    HookVerticalScrollControllerEvents();
    UpdateScrollPresenterVerticalScrollController(verticalScrollController);
}

void ScrollView::UpdateAreHorizontalScrollControllerInteractionsAllowed()
{
    bool oldAreHorizontalScrollControllerInteractionsAllowed = m_areHorizontalScrollControllerInteractionsAllowed;

    if (!m_scrollPresenter ||
        m_scrollPresenter.get().HorizontalScrollMode() == winrt::ScrollingScrollMode::Disabled ||
        m_scrollPresenter.get().ScrollableWidth() <= 0.0)
    {
        m_areHorizontalScrollControllerInteractionsAllowed = false;
    }
    else if (auto horizontalScrollControllerControl = m_horizontalScrollControllerElement.try_as<winrt::Control>())
    {
        m_areHorizontalScrollControllerInteractionsAllowed = horizontalScrollControllerControl.IsEnabled();
    }
    else
    {
        m_areHorizontalScrollControllerInteractionsAllowed = true;
    }

    if (oldAreHorizontalScrollControllerInteractionsAllowed != m_areHorizontalScrollControllerInteractionsAllowed)
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        UpdateScrollControllersVisibility(true /*horizontalChange*/, false /*verticalChange*/);
    }
}

void ScrollView::UpdateAreVerticalScrollControllerInteractionsAllowed()
{
    bool oldAreVerticalScrollControllerInteractionsAllowed = m_areVerticalScrollControllerInteractionsAllowed;

    if (!m_scrollPresenter ||
        m_scrollPresenter.get().VerticalScrollMode() == winrt::ScrollingScrollMode::Disabled ||
        m_scrollPresenter.get().ScrollableHeight() <= 0.0)
    {
        m_areVerticalScrollControllerInteractionsAllowed = false;
    }
    else if (auto verticalScrollControllerControl = m_verticalScrollControllerElement.try_as<winrt::Control>())
    {
        m_areVerticalScrollControllerInteractionsAllowed = verticalScrollControllerControl.IsEnabled();
    }
    else
    {
        m_areVerticalScrollControllerInteractionsAllowed = true;
    }

    if (oldAreVerticalScrollControllerInteractionsAllowed != m_areVerticalScrollControllerInteractionsAllowed)
    {
        SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        UpdateScrollControllersVisibility(false /*horizontalChange*/, true /*verticalChange*/);
    }
}

void ScrollView::UpdateScrollPresenterVerticalScrollController(const winrt::IScrollController& verticalScrollController)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scrollPresenter = m_scrollPresenter.get())
    {
        scrollPresenter.VerticalScrollController(verticalScrollController);
    }
}

void ScrollView::UpdateScrollControllersSeparator(const winrt::IUIElement& scrollControllersSeparator)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_scrollControllersSeparatorElement.set(scrollControllersSeparator);
}

void ScrollView::UpdateScrollControllersVisibility(
    bool horizontalChange, bool verticalChange)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(horizontalChange || verticalChange);

    bool isHorizontalScrollControllerVisible = false;

    if (horizontalChange)
    {
        const auto scrollBarVisibility = HorizontalScrollBarVisibility();

        if (scrollBarVisibility == winrt::ScrollingScrollBarVisibility::Auto &&
#ifdef USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
            m_horizontalScrollController &&
            m_horizontalScrollController.get().AreScrollControllerInteractionsAllowed())
#else
            m_areHorizontalScrollControllerInteractionsAllowed)
#endif
        {
            isHorizontalScrollControllerVisible = true;
        }
        else
        {
            isHorizontalScrollControllerVisible = (scrollBarVisibility == winrt::ScrollingScrollBarVisibility::Visible);
        }

        SetValue(s_ComputedHorizontalScrollBarVisibilityProperty, box_value(isHorizontalScrollControllerVisible ? winrt::Visibility::Visible : winrt::Visibility::Collapsed));
    }
    else
    {
        isHorizontalScrollControllerVisible = ComputedHorizontalScrollBarVisibility() == winrt::Visibility::Visible;
    }

    bool isVerticalScrollControllerVisible = false;

    if (verticalChange)
    {
        const auto scrollBarVisibility = VerticalScrollBarVisibility();

        if (scrollBarVisibility == winrt::ScrollingScrollBarVisibility::Auto &&
#ifdef USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
            m_verticalScrollController &&
            m_verticalScrollController.get().AreScrollControllerInteractionsAllowed())
#else
            m_areVerticalScrollControllerInteractionsAllowed)
#endif
        {
            isVerticalScrollControllerVisible = true;
        }
        else
        {
            isVerticalScrollControllerVisible = (scrollBarVisibility == winrt::ScrollingScrollBarVisibility::Visible);
        }

        SetValue(s_ComputedVerticalScrollBarVisibilityProperty, box_value(isVerticalScrollControllerVisible ? winrt::Visibility::Visible : winrt::Visibility::Collapsed));
    }
    else
    {
        isVerticalScrollControllerVisible = ComputedVerticalScrollBarVisibility() == winrt::Visibility::Visible;
    }

    if (m_scrollControllersSeparatorElement)
    {
        m_scrollControllersSeparatorElement.get().Visibility(isHorizontalScrollControllerVisible && isVerticalScrollControllerVisible ?
            winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }
}

bool ScrollView::IsInputKindIgnored(winrt::ScrollingInputKinds const& inputKind)
{
    return (IgnoredInputKinds() & inputKind) == inputKind;
}

bool ScrollView::AreAllScrollControllersCollapsed() const
{
    return (!m_horizontalScrollControllerElement || m_horizontalScrollControllerElement.get().Visibility() == winrt::Visibility::Collapsed) &&
        (!m_verticalScrollControllerElement || m_verticalScrollControllerElement.get().Visibility() == winrt::Visibility::Collapsed);
}

bool ScrollView::AreBothScrollControllersVisible() const
{
    return m_horizontalScrollControllerElement && m_horizontalScrollControllerElement.get().Visibility() == winrt::Visibility::Visible &&
        m_verticalScrollControllerElement && m_verticalScrollControllerElement.get().Visibility() == winrt::Visibility::Visible;
}

bool ScrollView::AreScrollControllersAutoHiding()
{
    // Use the cached value unless it was invalidated.
    if (m_autoHideScrollControllersValid)
    {
        return m_autoHideScrollControllers;
    }

    m_autoHideScrollControllersValid = true;

    if (SharedHelpers::IsRS4OrHigher())
    {
        if (auto globalTestHooks = ScrollViewTestHooks::GetGlobalTestHooks())
        {
            winrt::IReference<bool> autoHideScrollControllers = globalTestHooks->GetAutoHideScrollControllers(*this);

            if (autoHideScrollControllers)
            {
                // Test hook takes precedence over UISettings and registry key settings.
                m_autoHideScrollControllers = autoHideScrollControllers.Value();
                return m_autoHideScrollControllers;
            }
        }
    }

    if (m_uiSettings5)
    {
        // Use the 19H1+ UISettings property.
        m_autoHideScrollControllers = m_uiSettings5.AutoHideScrollBars();
    }
    else if (SharedHelpers::IsRS4OrHigher())
    {
        // Use the RS4+ registry key HKEY_CURRENT_USER\Control Panel\Accessibility\DynamicScrollbars
        m_autoHideScrollControllers = RegUtil::UseDynamicScrollbars();
    }
    else
    {
        // ScrollBars are auto-hiding prior to RS4.
        m_autoHideScrollControllers = true;
    }

    return m_autoHideScrollControllers;
}

bool ScrollView::IsScrollControllersSeparatorVisible() const
{
    return m_scrollControllersSeparatorElement && m_scrollControllersSeparatorElement.get().Visibility() == winrt::Visibility::Visible;
}

void ScrollView::HideIndicators(
    bool useTransitions)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, useTransitions, m_keepIndicatorsShowing);

    MUX_ASSERT(AreScrollControllersAutoHiding());

    if (!AreAllScrollControllersCollapsed() && !m_keepIndicatorsShowing)
    {
        GoToState(s_noIndicatorStateName, useTransitions);

        if (!m_hasNoIndicatorStateStoryboardCompletedHandler)
        {
            m_showingMouseIndicators = false;
        }
    }
}

void ScrollView::HideIndicatorsAfterDelay()
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, m_keepIndicatorsShowing);

    MUX_ASSERT(AreScrollControllersAutoHiding());

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
            hideIndicatorsTimer.Tick({ this, &ScrollView::OnHideIndicatorsTimerTick });
            m_hideIndicatorsTimer.set(hideIndicatorsTimer);
        }

        hideIndicatorsTimer.Start();
    }
}

// On RS4 and RS5, update m_autoHideScrollControllers based on the DynamicScrollbars registry key value
// and update the visual states if the value changed.
void ScrollView::UpdateScrollControllersAutoHiding(
    bool forceUpdate)
{
    if ((forceUpdate || (!m_uiSettings5 && SharedHelpers::IsRS4OrHigher())) && m_autoHideScrollControllersValid)
    {
        m_autoHideScrollControllersValid = false;

        const bool oldAutoHideScrollControllers = m_autoHideScrollControllers;
        const bool newAutoHideScrollControllers = AreScrollControllersAutoHiding();

        if (oldAutoHideScrollControllers != newAutoHideScrollControllers)
        {
            UpdateVisualStates(
                true  /*useTransitions*/,
                false /*showIndicators*/,
                false /*hideIndicators*/,
                true  /*scrollControllersAutoHidingChanged*/);
        }
    }
}

void ScrollView::UpdateVisualStates(
    bool useTransitions,
    bool showIndicators,
    bool hideIndicators,
    bool scrollControllersAutoHidingChanged,
    bool updateScrollControllersAutoHiding,
    bool onlyForAutoHidingScrollControllers)
{
    if (updateScrollControllersAutoHiding)
    {
        UpdateScrollControllersAutoHiding();
    }

    if (onlyForAutoHidingScrollControllers && !AreScrollControllersAutoHiding())
    {
        return;
    }

    UpdateScrollControllersVisualState(useTransitions, showIndicators, hideIndicators);
    UpdateScrollControllersSeparatorVisualState(useTransitions, scrollControllersAutoHidingChanged);
}

// Updates the state for the ScrollingIndicatorStates state group.
void ScrollView::UpdateScrollControllersVisualState(
    bool useTransitions,
    bool showIndicators,
    bool hideIndicators)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, useTransitions);
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, showIndicators, hideIndicators);

    MUX_ASSERT(!(showIndicators && hideIndicators));

    const bool areScrollControllersAutoHiding = AreScrollControllersAutoHiding();

    MUX_ASSERT(!(!areScrollControllersAutoHiding && hideIndicators));

    if ((!areScrollControllersAutoHiding || showIndicators) && !hideIndicators)
    {
        if (AreAllScrollControllersCollapsed())
        {
            return;
        }

        ResetHideIndicatorsTimer(false /*isForDestructor*/, true /*restart*/);

        // Mouse indicators dominate if they are already showing or if we have set the flag to prefer them.
        if (m_preferMouseIndicators || m_showingMouseIndicators || !areScrollControllersAutoHiding)
        {
            GoToState(s_mouseIndicatorStateName, useTransitions);

            m_showingMouseIndicators = true;
        }
        else
        {
            GoToState(s_touchIndicatorStateName, useTransitions);
        }
    }
    else if (!m_keepIndicatorsShowing)
    {
        if (SharedHelpers::IsAnimationsEnabled())
        {
            // By default there is a delay before the NoIndicator state actually shows.
            HideIndicators();
        }
        else
        {
            // Since OS animations are turned off, use a timer to delay the indicators' hiding.
            HideIndicatorsAfterDelay();
        }
    }
}

// Updates the state for the ScrollBarsSeparatorStates state group.
void ScrollView::UpdateScrollControllersSeparatorVisualState(
    bool useTransitions,
    bool scrollControllersAutoHidingChanged)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, useTransitions, scrollControllersAutoHidingChanged);

    if (!IsScrollControllersSeparatorVisible())
    {
        return;
    }

    const bool isEnabled = IsEnabled();
    const bool areScrollControllersAutoHiding = AreScrollControllersAutoHiding();
    bool showScrollControllersSeparator = !areScrollControllersAutoHiding;

    if (!showScrollControllersSeparator &&
        AreBothScrollControllersVisible() &&
        (m_preferMouseIndicators || m_showingMouseIndicators) &&
        (m_isPointerOverHorizontalScrollController || m_isPointerOverVerticalScrollController))
    {
        showScrollControllersSeparator = true;
    }

    // Select the proper state for the scroll controllers separator within the ScrollBarsSeparatorStates group:
    if (SharedHelpers::IsAnimationsEnabled())
    {
        // When OS animations are turned on, show the separator when a scroll controller is shown unless the ScrollView is disabled, using an animation.
        if (showScrollControllersSeparator && isEnabled)
        {
            GoToState(s_scrollBarsSeparatorExpanded, useTransitions);
        }
        else if (isEnabled)
        {
            GoToState(s_scrollBarsSeparatorCollapsed, useTransitions);
        }
        else
        {
            GoToState(s_scrollBarsSeparatorCollapsedDisabled, useTransitions);
        }
    }
    else
    {
        // OS animations are turned off. Show or hide the separator depending on the presence of scroll controllers, without an animation.
        // When the ScrollView is disabled, hide the separator in sync with the ScrollBar(s).
        if (showScrollControllersSeparator)
        {
            if (isEnabled)
            {
                GoToState((areScrollControllersAutoHiding || scrollControllersAutoHidingChanged) ? s_scrollBarsSeparatorExpandedWithoutAnimation : s_scrollBarsSeparatorDisplayedWithoutAnimation, useTransitions);
            }
            else
            {
                GoToState(s_scrollBarsSeparatorCollapsed, useTransitions);
            }
        }
        else
        {
            GoToState(isEnabled ? s_scrollBarsSeparatorCollapsedWithoutAnimation : s_scrollBarsSeparatorCollapsed, useTransitions);
        }
    }
}

void ScrollView::GoToState(std::wstring_view const& stateName, bool useTransitions)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, stateName.data(), useTransitions);

    winrt::VisualStateManager::GoToState(*this, stateName, useTransitions);
}

void ScrollView::OnKeyDown(winrt::KeyRoutedEventArgs const& e)
{
    SCROLLVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::KeyRoutedEventArgsToString(e).c_str());

    __super::OnKeyDown(e);

    m_preferMouseIndicators = false;

    if (m_scrollPresenter)
    {
        winrt::KeyRoutedEventArgs eventArgs = e.as<winrt::KeyRoutedEventArgs>();
        if (!eventArgs.Handled())
        {
            const auto originalKey = eventArgs.OriginalKey();
            const bool isGamepadKey = FocusHelper::IsGamepadNavigationDirection(originalKey) || FocusHelper::IsGamepadPageNavigationDirection(originalKey);

            if (isGamepadKey)
            {
                if (IsInputKindIgnored(winrt::ScrollingInputKinds::Gamepad))
                {
                    return;
                }
            }
            else
            {
                if (IsInputKindIgnored(winrt::ScrollingInputKinds::Keyboard))
                {
                    return;
                }
            }

            const bool isXYFocusEnabledForKeyboard = XYFocusKeyboardNavigation() == winrt::XYFocusKeyboardNavigationMode::Enabled;
            const bool doXYFocusScrolling = isGamepadKey || isXYFocusEnabledForKeyboard;

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

void ScrollView::HandleKeyDownForStandardScroll(winrt::KeyRoutedEventArgs args)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::KeyRoutedEventArgsToString(args).c_str());

    // Up/Down/Left/Right will scroll by 15% the size of the viewport.
    static const double smallScrollProportion = 0.15;

    MUX_ASSERT(!args.Handled());
    MUX_ASSERT(m_scrollPresenter != nullptr);

    const bool isHandled = DoScrollForKey(args.Key(), smallScrollProportion);

    args.Handled(isHandled);
}

void ScrollView::HandleKeyDownForXYNavigation(winrt::KeyRoutedEventArgs args)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::KeyRoutedEventArgsToString(args).c_str());

    MUX_ASSERT(!args.Handled());
    MUX_ASSERT(m_scrollPresenter != nullptr);

    bool isHandled = false;
    const auto originalKey = args.OriginalKey();
    const auto scrollPresenter = m_scrollPresenter.get().as<winrt::ScrollPresenter>();
    const bool isPageNavigation = FocusHelper::IsGamepadPageNavigationDirection(originalKey);
    const double scrollAmountProportion = isPageNavigation ? 1.0 : 0.5;
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

            const auto nextElementAsFe = nextElementAsUIE.as<winrt::FrameworkElement>();
            const auto rect = winrt::Rect{ 0, 0, static_cast<float>(nextElementAsFe.ActualWidth()), static_cast<float>(nextElementAsFe.ActualHeight()) };
            const auto elementBounds = nextElementAsUIE.TransformToVisual(scrollPresenter).TransformBounds(rect);
            const auto viewport = winrt::Rect{ 0, 0, static_cast<float>(scrollPresenter.ActualWidth()), static_cast<float>(scrollPresenter.ActualHeight()) };

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

            const bool isElementInExtendedViewport = winrt::RectHelper::Intersect(elementBounds, extendedViewport) != winrt::RectHelper::Empty();
            const bool isElementFullyInExtendedViewport = winrt::RectHelper::Union(elementBounds, extendedViewport) == extendedViewport;

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
            SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH_INT, METH_NAME, this, L"FocusManager::TryFocusAsync", SharedHelpers::IsAnimationsEnabled());

            auto focusAsyncOperation = winrt::FocusManager::TryFocusAsync(nextElement, winrt::FocusState::Keyboard);

            if (SharedHelpers::IsAnimationsEnabled()) // When system animations are turned off, the bring-into-view operations are not turned into animations.
            {
                focusAsyncOperation.Completed(winrt::AsyncOperationCompletedHandler<winrt::FocusMovementResult>(
                    [strongThis = get_strong(), targetElement = nextElement.try_as<winrt::UIElement>()](winrt::IAsyncOperation<winrt::FocusMovementResult> asyncOperation, winrt::AsyncStatus asyncStatus)
                {
                    SCROLLVIEW_TRACE_VERBOSE(*strongThis, TRACE_MSG_METH_INT, METH_NAME, strongThis, static_cast<int>(asyncStatus));

                    if (asyncStatus == winrt::AsyncStatus::Completed && asyncOperation.GetResults())
                    {
                        // The focus change request was successful. One or a few ScrollPresenter::BringingIntoView notifications are likely to be raised in the coming ticks.
                        // For those, the BringIntoViewRequestedEventArgs::AnimationDesired property will be set to True in order to animate to the target element rather than jumping.
                        SCROLLVIEW_TRACE_VERBOSE(*strongThis, TRACE_MSG_METH_PTR, METH_NAME, strongThis, targetElement);

                        auto bringIntoViewOperation(std::make_shared<ScrollViewBringIntoViewOperation>(targetElement));

                        strongThis->m_bringIntoViewOperations.push_back(bringIntoViewOperation);
                        strongThis->HookCompositionTargetRendering();
                    }
                }));
            }

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
                    DoScroll(scrollPresenter.ActualHeight() * scrollAmountProportion, winrt::Orientation::Vertical);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Up && CanScrollUp())
                {
                    isHandled = true;
                    DoScroll(-scrollPresenter.ActualHeight() * scrollAmountProportion, winrt::Orientation::Vertical);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Right && CanScrollRight())
                {
                    isHandled = true;
                    DoScroll(scrollPresenter.ActualWidth() * scrollAmountProportion * (FlowDirection() == winrt::FlowDirection::RightToLeft ? -1 : 1), winrt::Orientation::Horizontal);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Left && CanScrollLeft())
                {
                    isHandled = true;
                    DoScroll(-scrollPresenter.ActualWidth() * scrollAmountProportion * (FlowDirection() == winrt::FlowDirection::RightToLeft ? -1 : 1), winrt::Orientation::Horizontal);
                }
            }
        }
    }

    args.Handled(isHandled);
}

winrt::DependencyObject ScrollView::GetNextFocusCandidate(winrt::FocusNavigationDirection navigationDirection, bool isPageNavigation)
{
    MUX_ASSERT(m_scrollPresenter != nullptr);
    MUX_ASSERT(navigationDirection != winrt::FocusNavigationDirection::None);
    auto scrollPresenter = m_scrollPresenter.get().as<winrt::ScrollPresenter>();

    winrt::FocusNavigationDirection focusDirection = navigationDirection;

    winrt::FindNextElementOptions findNextElementOptions;
    findNextElementOptions.SearchRoot(scrollPresenter.Content());

    if (isPageNavigation)
    {
        const auto localBounds = winrt::Rect{ 0, 0, static_cast<float>(scrollPresenter.ActualWidth()), static_cast<float>(scrollPresenter.ActualHeight()) };
        const auto globalBounds = scrollPresenter.TransformToVisual(nullptr).TransformBounds(localBounds);
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

bool ScrollView::DoScrollForKey(winrt::VirtualKey key, double scrollProportion)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_INT, METH_NAME, this, scrollProportion, static_cast<int>(key));

    MUX_ASSERT(m_scrollPresenter != nullptr);

    bool isScrollTriggered = false;
    auto scrollPresenter = m_scrollPresenter.get().as<winrt::ScrollPresenter>();

    if (key == winrt::VirtualKey::Down && CanScrollDown())
    {
        isScrollTriggered = true;
        DoScroll(scrollPresenter.ActualHeight() * scrollProportion, winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::Up && CanScrollUp())
    {
        isScrollTriggered = true;
        DoScroll(scrollPresenter.ActualHeight() * -scrollProportion, winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::PageDown && CanScrollDown())
    {
        isScrollTriggered = true;
        DoScroll(scrollPresenter.ActualHeight(), winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::PageUp && CanScrollUp())
    {
        isScrollTriggered = true;
        DoScroll(-scrollPresenter.ActualHeight(), winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::Left || key == winrt::VirtualKey::Right)
    {
        double scrollAmount = scrollPresenter.ActualWidth() * scrollProportion;
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
        const bool canScrollUp = CanScrollUp();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollingScrollMode verticalScrollMode = ComputedVerticalScrollMode();
#else
        const auto verticalScrollMode = VerticalScrollMode();
#endif

        if (canScrollUp || (verticalScrollMode == winrt::ScrollingScrollMode::Disabled && CanScrollLeft()))
        {
            isScrollTriggered = true;
            auto horizontalOffset = canScrollUp ? scrollPresenter.HorizontalOffset() : 0.0;
            const auto verticalOffset = canScrollUp ? 0.0 : scrollPresenter.VerticalOffset();

            if (!canScrollUp && FlowDirection() == winrt::FlowDirection::RightToLeft)
            {
                horizontalOffset = scrollPresenter.ExtentWidth() * scrollPresenter.ZoomFactor() - scrollPresenter.ActualWidth();
            }

            scrollPresenter.ScrollTo(horizontalOffset, verticalOffset);
        }
    }
    else if (key == winrt::VirtualKey::End)
    {
        const bool canScrollDown = CanScrollDown();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollingScrollMode verticalScrollMode = ComputedVerticalScrollMode();
#else
        const auto verticalScrollMode = VerticalScrollMode();
#endif

        if (canScrollDown || (verticalScrollMode == winrt::ScrollingScrollMode::Disabled && CanScrollRight()))
        {
            isScrollTriggered = true;
            const auto zoomedExtent = (canScrollDown ? scrollPresenter.ExtentHeight() : scrollPresenter.ExtentWidth()) * scrollPresenter.ZoomFactor();
            auto horizontalOffset = canScrollDown ? scrollPresenter.HorizontalOffset() : zoomedExtent - scrollPresenter.ActualWidth();
            const auto verticalOffset = canScrollDown ? zoomedExtent - scrollPresenter.ActualHeight() : scrollPresenter.VerticalOffset();

            if (!canScrollDown && FlowDirection() == winrt::FlowDirection::RightToLeft)
            {
                horizontalOffset = 0.0;
            }

            scrollPresenter.ScrollTo(horizontalOffset, verticalOffset);
        }
    }

    return isScrollTriggered;
}

void ScrollView::DoScroll(double offset, winrt::Orientation orientation)
{
    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_INT, METH_NAME, this, offset, static_cast<int>(orientation));

    const bool isVertical = orientation == winrt::Orientation::Vertical;

    if (auto scrollPresenter = m_scrollPresenter.get().as<winrt::ScrollPresenter>())
    {
        if (SharedHelpers::IsAnimationsEnabled())
        {
            static const winrt::float2 inertiaDecayRate(0.9995f, 0.9995f);

            // A velocity less than or equal to this value has no effect.
            static const double minVelocity = 30.0;

            // We need to add this much velocity over minVelocity per pixel we want to move:
            static constexpr double s_velocityNeededPerPixel{ 7.600855902349023 };

            const auto scrollDir = offset > 0 ? 1 : -1;

            // The minimum velocity required to move in the given direction.
            double baselineVelocity = minVelocity * scrollDir;

            // If there is already a scroll animation running for a previous key press, we want to take that into account
            // for calculating the baseline velocity. 
            const auto previousScrollViewChangeCorrelationId = isVertical ? m_verticalAddScrollVelocityOffsetChangeCorrelationId : m_horizontalAddScrollVelocityOffsetChangeCorrelationId;
            if (previousScrollViewChangeCorrelationId != s_noOpCorrelationId)
            {
                const auto directionOfPreviousScrollOperation = isVertical ? m_verticalAddScrollVelocityDirection : m_horizontalAddScrollVelocityDirection;
                if (directionOfPreviousScrollOperation == 1)
                {
                    baselineVelocity -= minVelocity;
                }
                else if (directionOfPreviousScrollOperation == -1)
                {
                    baselineVelocity += minVelocity;
                }
            }

            const auto velocity = static_cast<float>(baselineVelocity + (offset * s_velocityNeededPerPixel));

            if (isVertical)
            {
                const winrt::float2 offsetsVelocity(0.0f, velocity);
                m_verticalAddScrollVelocityOffsetChangeCorrelationId = scrollPresenter.AddScrollVelocity(offsetsVelocity, inertiaDecayRate);
                m_verticalAddScrollVelocityDirection = scrollDir;
            }
            else
            {
                const winrt::float2 offsetsVelocity(velocity, 0.0f);
                m_horizontalAddScrollVelocityOffsetChangeCorrelationId = scrollPresenter.AddScrollVelocity(offsetsVelocity, inertiaDecayRate);
                m_horizontalAddScrollVelocityDirection = scrollDir;
            }
        }
        else
        {
            if (isVertical)
            {
                // Any horizontal AddScrollVelocity animation recently launched should be ignored by a potential subsequent AddScrollVelocity call.
                m_verticalAddScrollVelocityOffsetChangeCorrelationId = s_noOpCorrelationId;

                scrollPresenter.ScrollBy(0.0 /*horizontalOffsetDelta*/, offset /*verticalOffsetDelta*/);
            }
            else
            {
                // Any vertical AddScrollVelocity animation recently launched should be ignored by a potential subsequent AddScrollVelocity call.
                m_horizontalAddScrollVelocityOffsetChangeCorrelationId = s_noOpCorrelationId;

                scrollPresenter.ScrollBy(offset /*horizontalOffsetDelta*/, 0.0 /*verticalOffsetDelta*/);
            }
        }
    }
}

bool ScrollView::CanScrollInDirection(winrt::FocusNavigationDirection direction)
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

bool ScrollView::CanScrollDown()
{
    return CanScrollVerticallyInDirection(true /*inPositiveDirection*/);
}

bool ScrollView::CanScrollUp()
{
    return CanScrollVerticallyInDirection(false /*inPositiveDirection*/);
}

bool ScrollView::CanScrollRight()
{
    return CanScrollHorizontallyInDirection(true /*inPositiveDirection*/);
}

bool ScrollView::CanScrollLeft()
{
    return CanScrollHorizontallyInDirection(false /*inPositiveDirection*/);
}

bool ScrollView::CanScrollVerticallyInDirection(bool inPositiveDirection)
{
    bool canScrollInDirection = false;
    if (m_scrollPresenter)
    {
        auto scrollPresenter = m_scrollPresenter.get().as<winrt::ScrollPresenter>();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollingScrollMode verticalScrollMode = ComputedVerticalScrollMode();
#else
        const auto verticalScrollMode = VerticalScrollMode();
#endif

        if (verticalScrollMode == winrt::ScrollingScrollMode::Enabled)
        {
            const auto zoomedExtentHeight = scrollPresenter.ExtentHeight() * scrollPresenter.ZoomFactor();
            const auto viewportHeight = scrollPresenter.ActualHeight();
            if (zoomedExtentHeight > viewportHeight)
            {
                if (inPositiveDirection)
                {
                    const auto maxVerticalOffset = zoomedExtentHeight - viewportHeight;
                    if (scrollPresenter.VerticalOffset() < maxVerticalOffset)
                    {
                        canScrollInDirection = true;
                    }
                }
                else
                {
                    if (scrollPresenter.VerticalOffset() > 0)
                    {
                        canScrollInDirection = true;
                    }
                }
            }
        }
    }

    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, inPositiveDirection, canScrollInDirection);

    return canScrollInDirection;
}

bool ScrollView::CanScrollHorizontallyInDirection(bool inPositiveDirection)
{
    bool canScrollInDirection = false;

    if (FlowDirection() == winrt::FlowDirection::RightToLeft)
    {
        inPositiveDirection = !inPositiveDirection;
    }

    if (m_scrollPresenter)
    {
        auto scrollPresenter = m_scrollPresenter.get().as<winrt::ScrollPresenter>();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollingScrollMode horizontalScrollMode = ComputedHorizontalScrollMode();
#else
        const auto horizontalScrollMode = HorizontalScrollMode();
#endif

        if (horizontalScrollMode == winrt::ScrollingScrollMode::Enabled)
        {
            const auto zoomedExtentWidth = scrollPresenter.ExtentWidth() * scrollPresenter.ZoomFactor();
            const auto viewportWidth = scrollPresenter.ActualWidth();
            if (zoomedExtentWidth > viewportWidth)
            {
                if (inPositiveDirection)
                {
                    const auto maxHorizontalOffset = zoomedExtentWidth - viewportWidth;
                    if (scrollPresenter.HorizontalOffset() < maxHorizontalOffset)
                    {
                        canScrollInDirection = true;
                    }
                }
                else
                {
                    if (scrollPresenter.HorizontalOffset() > 0)
                    {
                        canScrollInDirection = true;
                    }
                }
            }
        }
    }

    SCROLLVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, inPositiveDirection, canScrollInDirection);

    return canScrollInDirection;
}

#ifdef _DEBUG

winrt::hstring ScrollView::DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty)
{
    if (dependencyProperty == s_ContentProperty)
    {
        return L"Content";
    }
    else if (dependencyProperty == s_ScrollPresenterProperty)
    {
        return L"ScrollPresenter";
    }
    else if (dependencyProperty == s_HorizontalScrollBarVisibilityProperty)
    {
        return L"HorizontalScrollBarVisibility";
    }
    else if (dependencyProperty == s_VerticalScrollBarVisibilityProperty)
    {
        return L"VerticalScrollBarVisibility";
    }
    else if (dependencyProperty == s_ContentOrientationProperty)
    {
        return L"ContentOrientation";
    }
    else if (dependencyProperty == s_VerticalScrollChainModeProperty)
    {
        return L"VerticalScrollChainMode";
    }
    else if (dependencyProperty == s_ZoomChainModeProperty)
    {
        return L"ZoomChainMode";
    }
    else if (dependencyProperty == s_HorizontalScrollRailModeProperty)
    {
        return L"HorizontalScrollRailMode";
    }
    else if (dependencyProperty == s_VerticalScrollRailModeProperty)
    {
        return L"VerticalScrollRailMode";
    }
    else if (dependencyProperty == s_HorizontalScrollModeProperty)
    {
        return L"HorizontalScrollMode";
    }
    else if (dependencyProperty == s_VerticalScrollModeProperty)
    {
        return L"VerticalScrollMode";
    }
    else if (dependencyProperty == s_ComputedHorizontalScrollBarVisibilityProperty)
    {
        return L"ComputedHorizontalScrollBarVisibility";
    }
    else if (dependencyProperty == s_ComputedVerticalScrollBarVisibilityProperty)
    {
        return L"ComputedVerticalScrollBarVisibility";
    }
#ifdef USE_SCROLLMODE_AUTO
    else if (dependencyProperty == s_ComputedHorizontalScrollModeProperty)
    {
        return L"ComputedHorizontalScrollMode";
    }
    else if (dependencyProperty == s_ComputedVerticalScrollModeProperty)
    {
        return L"ComputedVerticalScrollMode";
    }
#endif
    else if (dependencyProperty == s_ZoomModeProperty)
    {
        return L"ZoomMode";
    }
    else if (dependencyProperty == s_IgnoredInputKindsProperty)
    {
        return L"IgnoredInputKinds";
    }
    else if (dependencyProperty == s_MinZoomFactorProperty)
    {
        return L"MinZoomFactor";
    }
    else if (dependencyProperty == s_MaxZoomFactorProperty)
    {
        return L"MaxZoomFactor";
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

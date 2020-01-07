// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollingPresenterTypeLogging.h"
#include "ScrollingPresenter.h"
#include "ScrollingView.h"
#include "RuntimeProfiler.h"
#include "FocusHelper.h"
#include "RegUtil.h"
#include "ScrollingViewTestHooks.h"

// Change to 'true' to turn on debugging outputs in Output window
bool ScrollingViewTrace::s_IsDebugOutputEnabled{ false };
bool ScrollingViewTrace::s_IsVerboseDebugOutputEnabled{ false };

const winrt::ScrollingScrollInfo ScrollingView::s_noOpScrollInfo{ -1 };
const winrt::ScrollingZoomInfo ScrollingView::s_noOpZoomInfo{ -1 };

ScrollingView::ScrollingView()
{
    SCROLLINGVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    EnsureProperties();
    SetDefaultStyleKey(this);
    HookUISettingsEvent();
    HookScrollingViewEvents();
}

ScrollingView::~ScrollingView()
{
    SCROLLINGVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookCompositionTargetRendering();
    UnhookScrollingPresenterEvents(true /*isForDestructor*/);
    UnhookScrollingViewEvents();
    ResetHideIndicatorsTimer(true /*isForDestructor*/);
}

#pragma region IScrollingView

winrt::CompositionPropertySet ScrollingView::ExpressionAnimationSources()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ExpressionAnimationSources();
    }

    return nullptr;
}

double ScrollingView::HorizontalOffset()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.HorizontalOffset();
    }

    return 0.0;
}

double ScrollingView::VerticalOffset()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.VerticalOffset();
    }

    return 0.0;
}

float ScrollingView::ZoomFactor()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ZoomFactor();
    }

    return 0.0f;
}

double ScrollingView::ExtentWidth()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ExtentWidth();
    }

    return 0.0;
}

double ScrollingView::ExtentHeight()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ExtentHeight();
    }

    return 0.0;
}

double ScrollingView::ViewportWidth()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ViewportWidth();
    }

    return 0.0;
}

double ScrollingView::ViewportHeight()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ViewportHeight();
    }

    return 0.0;
}

double ScrollingView::ScrollableWidth()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ScrollableWidth();
    }

    return 0.0;
}

double ScrollingView::ScrollableHeight()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ScrollableHeight();
    }

    return 0.0;
}

winrt::ScrollingInteractionState ScrollingView::State()
{
    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.State();
    }

    return winrt::ScrollingInteractionState::Idle;
}

winrt::ScrollingInputKinds ScrollingView::IgnoredInputKind()
{
    // Workaround for Bug 17377013: XamlCompiler codegen for Enum CreateFromString always returns boxed int which is wrong for [flags] enums (should be uint)
    // Check if the boxed IgnoredInputKind is an IReference<int> first in which case we unbox as int.
    auto boxedKind = GetValue(s_IgnoredInputKindProperty);
    if (auto boxedInt = boxedKind.try_as<winrt::IReference<int32_t>>())
    {
        return winrt::ScrollingInputKinds{ static_cast<uint32_t>(unbox_value<int32_t>(boxedInt)) };
    }

    return auto_unbox(boxedKind);
}

void ScrollingView::IgnoredInputKind(winrt::ScrollingInputKinds const& value)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::InputKindToString(value).c_str());
    SetValue(s_IgnoredInputKindProperty, box_value(value));
}

void ScrollingView::RegisterAnchorCandidate(winrt::UIElement const& element)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, element);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        if (const auto scrollingPresenterAsAnchorProvider = scrollingPresenter.try_as<winrt::Controls::IScrollAnchorProvider>())
        {
            scrollingPresenterAsAnchorProvider.RegisterAnchorCandidate(element);
            return;
        }
        throw winrt::hresult_error(E_INVALID_OPERATION, s_IScrollAnchorProviderNotImpl);
    }
    throw winrt::hresult_error(E_INVALID_OPERATION, s_noScrollingPresenterPart);
}

void ScrollingView::UnregisterAnchorCandidate(winrt::UIElement const& element)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, element);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        if (const auto scrollingPresenterAsAnchorProvider = scrollingPresenter.try_as<winrt::Controls::IScrollAnchorProvider>())
        {
            scrollingPresenterAsAnchorProvider.UnregisterAnchorCandidate(element);
            return;
        }
        throw winrt::hresult_error(E_INVALID_OPERATION, s_IScrollAnchorProviderNotImpl);
    }
    throw winrt::hresult_error(E_INVALID_OPERATION, s_noScrollingPresenterPart);
}


winrt::ScrollingScrollInfo ScrollingView::ScrollTo(double horizontalOffset, double verticalOffset)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalOffset, verticalOffset);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ScrollTo(horizontalOffset, verticalOffset);
    }

    return s_noOpScrollInfo;
}

winrt::ScrollingScrollInfo ScrollingView::ScrollTo(double horizontalOffset, double verticalOffset, winrt::ScrollingScrollOptions const& options)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        horizontalOffset, verticalOffset, TypeLogging::ScrollOptionsToString(options).c_str());

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ScrollTo(horizontalOffset, verticalOffset, options);
    }

    return s_noOpScrollInfo;
}

winrt::ScrollingScrollInfo ScrollingView::ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalOffsetDelta, verticalOffsetDelta);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ScrollBy(horizontalOffsetDelta, verticalOffsetDelta);
    }

    return s_noOpScrollInfo;
}

winrt::ScrollingScrollInfo ScrollingView::ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta, winrt::ScrollingScrollOptions const& options)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL_STR, METH_NAME, this,
        horizontalOffsetDelta, verticalOffsetDelta, TypeLogging::ScrollOptionsToString(options).c_str());

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ScrollBy(horizontalOffsetDelta, verticalOffsetDelta, options);
    }

    return s_noOpScrollInfo;
}

winrt::ScrollingScrollInfo ScrollingView::ScrollFrom(winrt::float2 offsetsVelocity, winrt::IReference<winrt::float2> inertiaDecayRate)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::Float2ToString(offsetsVelocity).c_str(), TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str());

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ScrollFrom(offsetsVelocity, inertiaDecayRate);
    }

    return s_noOpScrollInfo;
}

winrt::ScrollingZoomInfo ScrollingView::ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(), zoomFactor);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ZoomTo(zoomFactor, centerPoint);
    }

    return s_noOpZoomInfo;
}

winrt::ScrollingZoomInfo ScrollingView::ZoomTo(float zoomFactor, winrt::IReference<winrt::float2> centerPoint, winrt::ScrollingZoomOptions const& options)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str(),
        zoomFactor);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ZoomTo(zoomFactor, centerPoint, options);
    }

    return s_noOpZoomInfo;
}

winrt::ScrollingZoomInfo ScrollingView::ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        zoomFactorDelta);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ZoomBy(zoomFactorDelta, centerPoint);
    }

    return s_noOpZoomInfo;
}

winrt::ScrollingZoomInfo ScrollingView::ZoomBy(float zoomFactorDelta, winrt::IReference<winrt::float2> centerPoint, winrt::ScrollingZoomOptions const& options)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::ZoomOptionsToString(options).c_str(),
        zoomFactorDelta);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ZoomBy(zoomFactorDelta, centerPoint, options);
    }

    return s_noOpZoomInfo;
}

winrt::ScrollingZoomInfo ScrollingView::ZoomFrom(float zoomFactorVelocity, winrt::IReference<winrt::float2> centerPoint, winrt::IReference<float> inertiaDecayRate)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(),
        TypeLogging::NullableFloatToString(inertiaDecayRate).c_str(),
        zoomFactorVelocity);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        return scrollingPresenter.ZoomFrom(zoomFactorVelocity, centerPoint, inertiaDecayRate);
    }

    return s_noOpZoomInfo;
}

#pragma endregion

#pragma region IFrameworkElementOverrides

void ScrollingView::OnApplyTemplate()
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnApplyTemplate();

    m_hasNoIndicatorStateStoryboardCompletedHandler = false;
    m_keepIndicatorsShowing = false;

    winrt::IControlProtected thisAsControlProtected = *this;

    winrt::ScrollingPresenter scrollingPresenter = GetTemplateChildT<winrt::ScrollingPresenter>(s_scrollingPresenterPartName, thisAsControlProtected);

    UpdateScrollingPresenter(scrollingPresenter);

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
                                        winrt::event_token noIndicatorStateStoryboardCompletedToken = stateStoryboard.Completed({ this, &ScrollingView::OnNoIndicatorStateStoryboardCompleted });
                                        m_hasNoIndicatorStateStoryboardCompletedHandler = true;
                                    }
                                    else if (stateName == s_touchIndicatorStateName || stateName == s_mouseIndicatorStateName)
                                    {
                                        winrt::event_token indicatorStateStoryboardCompletedToken = stateStoryboard.Completed({ this, &ScrollingView::OnIndicatorStateStoryboardCompleted });
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

void ScrollingView::OnGotFocus(winrt::RoutedEventArgs const& args)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

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

void ScrollingView::OnScrollingViewGettingFocus(
    const winrt::IInspectable& /*sender*/,
    const winrt::GettingFocusEventArgs& args)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_focusInputDeviceKind = args.InputDevice();
}

void ScrollingView::OnScrollingViewIsEnabledChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateVisualStates(
        true  /*useTransitions*/,
        false /*showIndicators*/,
        false /*hideIndicators*/,
        false /*scrollControllersAutoHidingChanged*/,
        true  /*updateScrollControllersAutoHiding*/);
}

void ScrollingView::OnScrollingViewUnloaded(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_showingMouseIndicators = false;
    m_keepIndicatorsShowing = false;
    m_bringIntoViewOperations.clear();

    UnhookCompositionTargetRendering();
    ResetHideIndicatorsTimer();
}

void ScrollingView::OnScrollingViewPointerEntered(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

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

void ScrollingView::OnScrollingViewPointerMoved(
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

void ScrollingView::OnScrollingViewPointerExited(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

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

void ScrollingView::OnScrollingViewPointerPressed(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

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

    // Show the scroll controller indicators as soon as a pointer is pressed on the ScrollingView.
    UpdateVisualStates(
        true  /*useTransitions*/,
        true  /*showIndicators*/,
        false /*hideIndicators*/,
        false /*scrollControllersAutoHidingChanged*/,
        true  /*updateScrollControllersAutoHiding*/,
        true  /*onlyForAutoHidingScrollControllers*/);
}

void ScrollingView::OnScrollingViewPointerReleased(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

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
        SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_METH, METH_NAME, this, L"Focus");

        bool tookFocus = Focus(winrt::FocusState::Pointer);
        args.Handled(tookFocus);
    }
}

void ScrollingView::OnScrollingViewPointerCanceled(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Mouse)
    {
        m_isLeftMouseButtonPressedForFocus = false;
    }
}

void ScrollingView::OnHorizontalScrollControllerIsEnabledChanged(
    const winrt::IInspectable& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateAreHorizontalScrollControllerInteractionsAllowed();
}

void ScrollingView::OnHorizontalScrollControllerPointerEntered(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverHorizontalScrollController = true;
}

void ScrollingView::OnHorizontalScrollControllerPointerExited(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverHorizontalScrollController = false;
}

void ScrollingView::OnVerticalScrollControllerIsEnabledChanged(
    const winrt::IInspectable& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateAreVerticalScrollControllerInteractionsAllowed();
}

void ScrollingView::OnVerticalScrollControllerPointerEntered(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverVerticalScrollController = true;

    UpdateScrollControllersAutoHiding();
    if (AreScrollControllersAutoHiding() && !SharedHelpers::IsAnimationsEnabled())
    {
        HideIndicatorsAfterDelay();
    }
}

void ScrollingView::OnVerticalScrollControllerPointerExited(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverVerticalScrollController = false;

    UpdateScrollControllersAutoHiding();
    if (AreScrollControllersAutoHiding())
    {
        HideIndicatorsAfterDelay();
    }
}

// Handler for when the NoIndicator state's storyboard completes animating.
void ScrollingView::OnNoIndicatorStateStoryboardCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_hasNoIndicatorStateStoryboardCompletedHandler);

    m_showingMouseIndicators = false;
}

// Handler for when a TouchIndicator or MouseIndicator state's storyboard completes animating.
void ScrollingView::OnIndicatorStateStoryboardCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    // If the cursor is currently directly over either scroll controller then do not automatically hide the indicators
    if (AreScrollControllersAutoHiding() &&
        !m_keepIndicatorsShowing &&
        !m_isPointerOverVerticalScrollController &&
        !m_isPointerOverHorizontalScrollController)
    {
        UpdateScrollControllersVisualState(true /*useTransitions*/, false /*showIndicators*/, true /*hideIndicators*/);
    }
}

// Invoked by ScrollingViewTestHooks
void ScrollingView::ScrollControllersAutoHidingChanged()
{
    if (SharedHelpers::IsRS4OrHigher())
    {
        UpdateScrollControllersAutoHiding(true /*forceUpdate*/);
    }
}

winrt::ScrollingPresenter ScrollingView::GetScrollingPresenterPart() const
{
    return m_scrollingPresenter.get().as<winrt::ScrollingPresenter>();
}

void ScrollingView::ValidateAnchorRatio(double value)
{
    ScrollingPresenter::ValidateAnchorRatio(value);
}

void ScrollingView::ValidateZoomFactoryBoundary(double value)
{
    ScrollingPresenter::ValidateZoomFactoryBoundary(value);
}

// Invoked when a dependency property of this ScrollingView has changed.
void ScrollingView::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto dependencyProperty = args.Property();

#ifdef _DEBUG
    SCROLLINGVIEW_TRACE_VERBOSE(nullptr, L"%s(property: %s)\n", METH_NAME, DependencyPropertyToString(dependencyProperty).c_str());
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
// IsInteracting, InteractionElement, InteractionElementScrollOrientation
// #ifdef USE_SCROLLCONTROLLER_ISINTERACTIONELEMENTRAILENABLED IsInteractionElementRailEnabled
// #ifdef USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED AreScrollControllerInteractionsAllowed
// #ifdef USE_SCROLLCONTROLLER_ARESCROLLERINTERACTIONSALLOWED AreScrollerInteractionsAllowed
void ScrollingView::OnScrollControllerInteractionInfoChanged(
    const winrt::IScrollController& sender,
    const winrt::IInspectable& /*args*/)
{
    bool isScrollControllerInteracting = sender.IsInteracting();
    bool showIndicators = false;
    bool hideIndicators = false;

    if (m_horizontalScrollController && m_horizontalScrollController == sender)
    {
        UpdateScrollControllersAutoHiding();

        if (m_isHorizontalScrollControllerInteracting != isScrollControllerInteracting)
        {
            SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"HorizontalScrollController.IsInteracting changed: ", m_isHorizontalScrollControllerInteracting, isScrollControllerInteracting);

            m_isHorizontalScrollControllerInteracting = isScrollControllerInteracting;

            if (isScrollControllerInteracting)
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

        if (m_isVerticalScrollControllerInteracting != isScrollControllerInteracting)
        {
            SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"VerticalScrollController.IsInteracting changed: ", m_isVerticalScrollControllerInteracting, isScrollControllerInteracting);

            m_isVerticalScrollControllerInteracting = isScrollControllerInteracting;

            if (isScrollControllerInteracting)
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

void ScrollingView::OnHideIndicatorsTimerTick(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    ResetHideIndicatorsTimer();

    if (AreScrollControllersAutoHiding())
    {
        HideIndicators();
    }
}

void ScrollingView::OnAutoHideScrollBarsChanged(
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

void ScrollingView::OnScrollingPresenterSizeChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateAreHorizontalScrollControllerInteractionsAllowed();
    UpdateAreVerticalScrollControllerInteractionsAllowed();
}

void ScrollingView::OnScrollingPresenterExtentChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    UpdateAreHorizontalScrollControllerInteractionsAllowed();
    UpdateAreVerticalScrollControllerInteractionsAllowed();

    if (m_extentChangedEventSource)
    {
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_extentChangedEventSource(*this, args);
    }
}

void ScrollingView::OnScrollingPresenterStateChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        if (scrollingPresenter.State() == winrt::ScrollingInteractionState::Interaction)
        {
            m_preferMouseIndicators = false;
        }
    }

    if (m_stateChangedEventSource)
    {
        m_stateChangedEventSource(*this, args);
    }
}

void ScrollingView::OnScrollAnimationStarting(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingScrollAnimationStartingEventArgs& args)
{
    if (m_scrollAnimationStartingEventSource)
    {
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_scrollAnimationStartingEventSource(*this, args);
    }
}

void ScrollingView::OnZoomAnimationStarting(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingZoomAnimationStartingEventArgs& args)
{
    if (m_zoomAnimationStartingEventSource)
    {
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_zoomAnimationStartingEventSource(*this, args);
    }
}

void ScrollingView::OnScrollingPresenterViewChanged(
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
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_viewChangedEventSource(*this, args);
    }
}

void ScrollingView::OnScrollingPresenterScrollCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingScrollCompletedEventArgs& args)
{
    if (args.ScrollInfo().OffsetsChangeId == m_horizontalScrollFromOffsetChangeId)
    {
        m_horizontalScrollFromOffsetChangeId = -1;
    }
    else if (args.ScrollInfo().OffsetsChangeId == m_verticalScrollFromOffsetChangeId)
    {
        m_verticalScrollFromOffsetChangeId = -1;
    }

    if (m_scrollCompletedEventSource)
    {
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_scrollCompletedEventSource(*this, args);
    }
}

void ScrollingView::OnScrollingPresenterZoomCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingZoomCompletedEventArgs& args)
{
    if (m_zoomCompletedEventSource)
    {
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_zoomCompletedEventSource(*this, args);
    }
}

void ScrollingView::OnScrollingPresenterBringingIntoView(
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
                // This ScrollingPresenter::BringingIntoView notification results from a FocusManager::TryFocusAsync call in ScrollingView::HandleKeyDownForXYNavigation.
                // Its BringIntoViewRequestedEventArgs::AnimationDesired property is set to True in order to animate to the target element rather than jumping.
                SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR_INT, METH_NAME, this, bringIntoViewOperation->TargetElement(), bringIntoViewOperation->TicksCount());

                requestEventArgs.AnimationDesired(true);
                break;
            }
        }
    }

    if (m_bringingIntoViewEventSource)
    {
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_bringingIntoViewEventSource(*this, args);
    }
}

void ScrollingView::OnScrollingPresenterAnchorRequested(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollingAnchorRequestedEventArgs& args)
{
    if (m_anchorRequestedEventSource)
    {
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_anchorRequestedEventSource(*this, args);
    }
}

void ScrollingView::OnCompositionTargetRendering(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!m_bringIntoViewOperations.empty())
    {
        for (auto operationsIter = m_bringIntoViewOperations.begin(); operationsIter != m_bringIntoViewOperations.end();)
        {
            auto& bringIntoViewOperation = *operationsIter;
            operationsIter++;
            
            SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR_INT, METH_NAME, this, bringIntoViewOperation->TargetElement(), bringIntoViewOperation->TicksCount());

            if (bringIntoViewOperation->HasMaxTicksCount())
            {
                // This ScrollingView is no longer expected to receive BringingIntoView notifications from its ScrollingPresenter,
                // resulting from a FocusManager::TryFocusAsync call in ScrollingView::HandleKeyDownForXYNavigation.
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

void ScrollingView::OnScrollingPresenterPropertyChanged(
    const winrt::DependencyObject& /*sender*/,
    const winrt::DependencyProperty& args)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (args == winrt::ScrollingPresenter::HorizontalScrollModeProperty())
    {
        UpdateAreHorizontalScrollControllerInteractionsAllowed();
    }
    else if (args == winrt::ScrollingPresenter::VerticalScrollModeProperty())
    {
        UpdateAreVerticalScrollControllerInteractionsAllowed();
    }

#ifdef USE_SCROLLMODE_AUTO
    if (args == winrt::ScrollingPresenter::ComputedHorizontalScrollModeProperty())
    {
        SetValue(s_ComputedHorizontalScrollModeProperty, box_value(m_scrollingPresenter.get().ComputedHorizontalScrollMode()));
    }
    else if (args == winrt::ScrollingPresenter::ComputedVerticalScrollModeProperty())
    {
        SetValue(s_ComputedVerticalScrollModeProperty, box_value(m_scrollingPresenter.get().ComputedVerticalScrollMode()));
    }
#endif
}

void ScrollingView::ResetHideIndicatorsTimer(bool isForDestructor, bool restart)
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

void ScrollingView::HookUISettingsEvent()
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
                { this, &ScrollingView::OnAutoHideScrollBarsChanged });
        }
    }
}

void ScrollingView::HookCompositionTargetRendering()
{
    if (!m_renderingToken)
    {
        winrt::Windows::UI::Xaml::Media::CompositionTarget compositionTarget{ nullptr };
        m_renderingToken = compositionTarget.Rendering(winrt::auto_revoke, { this, &ScrollingView::OnCompositionTargetRendering });
    }
}

void ScrollingView::UnhookCompositionTargetRendering()
{
    m_renderingToken.revoke();
}

void ScrollingView::HookScrollingViewEvents()
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

    m_gettingFocusToken = GettingFocus({ this, &ScrollingView::OnScrollingViewGettingFocus });
    m_isEnabledChangedToken = IsEnabledChanged({ this, &ScrollingView::OnScrollingViewIsEnabledChanged });
    m_unloadedToken = Unloaded({ this, &ScrollingView::OnScrollingViewUnloaded });

    m_onPointerEnteredEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollingView::OnScrollingViewPointerEntered });
    AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onPointerEnteredEventHandler, false);

    m_onPointerMovedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollingView::OnScrollingViewPointerMoved });
    AddHandler(winrt::UIElement::PointerMovedEvent(), m_onPointerMovedEventHandler, false);

    m_onPointerExitedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollingView::OnScrollingViewPointerExited });
    AddHandler(winrt::UIElement::PointerExitedEvent(), m_onPointerExitedEventHandler, false);

    m_onPointerPressedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollingView::OnScrollingViewPointerPressed });
    AddHandler(winrt::UIElement::PointerPressedEvent(), m_onPointerPressedEventHandler, false);

    m_onPointerReleasedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollingView::OnScrollingViewPointerReleased });
    AddHandler(winrt::UIElement::PointerReleasedEvent(), m_onPointerReleasedEventHandler, true);

    m_onPointerCanceledEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollingView::OnScrollingViewPointerCanceled });
    AddHandler(winrt::UIElement::PointerCanceledEvent(), m_onPointerCanceledEventHandler, true);
}

void ScrollingView::UnhookScrollingViewEvents()
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

void ScrollingView::HookScrollingPresenterEvents()
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_scrollingPresenterSizeChangedToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterExtentChangedToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterStateChangedToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterScrollAnimationStartingToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterZoomAnimationStartingToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterViewChangedToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterScrollCompletedToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterZoomCompletedToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterBringingIntoViewToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterAnchorRequestedToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterHorizontalScrollModeChangedToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterVerticalScrollModeChangedToken.value == 0);
#ifdef USE_SCROLLMODE_AUTO
    MUX_ASSERT(m_scrollingPresenterComputedHorizontalScrollModeChangedToken.value == 0);
    MUX_ASSERT(m_scrollingPresenterComputedVerticalScrollModeChangedToken.value == 0);
#endif

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        auto scrollingPresenterFE = scrollingPresenter.try_as<winrt::FrameworkElement>();

        m_scrollingPresenterSizeChangedToken = scrollingPresenterFE.SizeChanged({ this, &ScrollingView::OnScrollingPresenterSizeChanged });
        m_scrollingPresenterExtentChangedToken = scrollingPresenter.ExtentChanged({ this, &ScrollingView::OnScrollingPresenterExtentChanged });
        m_scrollingPresenterStateChangedToken = scrollingPresenter.StateChanged({ this, &ScrollingView::OnScrollingPresenterStateChanged });
        m_scrollingPresenterScrollAnimationStartingToken = scrollingPresenter.ScrollAnimationStarting({ this, &ScrollingView::OnScrollAnimationStarting });
        m_scrollingPresenterZoomAnimationStartingToken = scrollingPresenter.ZoomAnimationStarting({ this, &ScrollingView::OnZoomAnimationStarting });
        m_scrollingPresenterViewChangedToken = scrollingPresenter.ViewChanged({ this, &ScrollingView::OnScrollingPresenterViewChanged });
        m_scrollingPresenterScrollCompletedToken = scrollingPresenter.ScrollCompleted({ this, &ScrollingView::OnScrollingPresenterScrollCompleted });
        m_scrollingPresenterZoomCompletedToken = scrollingPresenter.ZoomCompleted({ this, &ScrollingView::OnScrollingPresenterZoomCompleted });
        m_scrollingPresenterBringingIntoViewToken = scrollingPresenter.BringingIntoView({ this, &ScrollingView::OnScrollingPresenterBringingIntoView });
        m_scrollingPresenterAnchorRequestedToken = scrollingPresenter.AnchorRequested({ this, &ScrollingView::OnScrollingPresenterAnchorRequested });

        const winrt::DependencyObject scrollingPresenterAsDO = scrollingPresenter.try_as<winrt::DependencyObject>();

        m_scrollingPresenterHorizontalScrollModeChangedToken.value = scrollingPresenterAsDO.RegisterPropertyChangedCallback(
            winrt::ScrollingPresenter::HorizontalScrollModeProperty(), { this, &ScrollingView::OnScrollingPresenterPropertyChanged });

        m_scrollingPresenterVerticalScrollModeChangedToken.value = scrollingPresenterAsDO.RegisterPropertyChangedCallback(
            winrt::ScrollingPresenter::VerticalScrollModeProperty(), { this, &ScrollingView::OnScrollingPresenterPropertyChanged });

#ifdef USE_SCROLLMODE_AUTO
        m_scrollingPresenterComputedHorizontalScrollModeChangedToken.value = scrollingPresenterAsDO.RegisterPropertyChangedCallback(
            winrt::ScrollingPresenter::ComputedHorizontalScrollModeProperty(), { this, &ScrollingView::OnScrollingPresenterPropertyChanged });

        m_scrollingPresenterComputedVerticalScrollModeChangedToken.value = scrollingPresenterAsDO.RegisterPropertyChangedCallback(
            winrt::ScrollingPresenter::ComputedVerticalScrollModeProperty(), { this, &ScrollingView::OnScrollingPresenterPropertyChanged });
#endif
    }
}

void ScrollingView::UnhookScrollingPresenterEvents(bool isForDestructor)
{
    if (isForDestructor)
    {
        SCROLLINGVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }
    else
    {
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);
    }

    if (auto scrollingPresenter = m_scrollingPresenter.safe_get())
    {
        if (m_scrollingPresenterSizeChangedToken.value != 0)
        {
            auto scrollingPresenterFE = scrollingPresenter.try_as<winrt::FrameworkElement>();

            scrollingPresenterFE.SizeChanged(m_scrollingPresenterSizeChangedToken);
            m_scrollingPresenterSizeChangedToken.value = 0;
        }

        if (m_scrollingPresenterExtentChangedToken.value != 0)
        {
            scrollingPresenter.ExtentChanged(m_scrollingPresenterExtentChangedToken);
            m_scrollingPresenterExtentChangedToken.value = 0;
        }

        if (m_scrollingPresenterStateChangedToken.value != 0)
        {
            scrollingPresenter.StateChanged(m_scrollingPresenterStateChangedToken);
            m_scrollingPresenterStateChangedToken.value = 0;
        }

        if (m_scrollingPresenterScrollAnimationStartingToken.value != 0)
        {
            scrollingPresenter.ScrollAnimationStarting(m_scrollingPresenterScrollAnimationStartingToken);
            m_scrollingPresenterScrollAnimationStartingToken.value = 0;
        }

        if (m_scrollingPresenterZoomAnimationStartingToken.value != 0)
        {
            scrollingPresenter.ZoomAnimationStarting(m_scrollingPresenterZoomAnimationStartingToken);
            m_scrollingPresenterZoomAnimationStartingToken.value = 0;
        }

        if (m_scrollingPresenterViewChangedToken.value != 0)
        {
            scrollingPresenter.ViewChanged(m_scrollingPresenterViewChangedToken);
            m_scrollingPresenterViewChangedToken.value = 0;
        }

        if (m_scrollingPresenterScrollCompletedToken.value != 0)
        {
            scrollingPresenter.ScrollCompleted(m_scrollingPresenterScrollCompletedToken);
            m_scrollingPresenterScrollCompletedToken.value = 0;
        }

        if (m_scrollingPresenterZoomCompletedToken.value != 0)
        {
            scrollingPresenter.ZoomCompleted(m_scrollingPresenterZoomCompletedToken);
            m_scrollingPresenterZoomCompletedToken.value = 0;
        }

        if (m_scrollingPresenterBringingIntoViewToken.value != 0)
        {
            scrollingPresenter.BringingIntoView(m_scrollingPresenterBringingIntoViewToken);
            m_scrollingPresenterBringingIntoViewToken.value = 0;
        }

        if (m_scrollingPresenterAnchorRequestedToken.value != 0)
        {
            scrollingPresenter.AnchorRequested(m_scrollingPresenterAnchorRequestedToken);
            m_scrollingPresenterAnchorRequestedToken.value = 0;
        }

        const winrt::DependencyObject scrollingPresenterAsDO = scrollingPresenter.try_as<winrt::DependencyObject>();

        if (m_scrollingPresenterHorizontalScrollModeChangedToken.value != 0)
        {
            scrollingPresenterAsDO.UnregisterPropertyChangedCallback(winrt::ScrollingPresenter::HorizontalScrollModeProperty(), m_scrollingPresenterHorizontalScrollModeChangedToken.value);
            m_scrollingPresenterHorizontalScrollModeChangedToken.value = 0;
        }

        if (m_scrollingPresenterVerticalScrollModeChangedToken.value != 0)
        {
            scrollingPresenterAsDO.UnregisterPropertyChangedCallback(winrt::ScrollingPresenter::VerticalScrollModeProperty(), m_scrollingPresenterVerticalScrollModeChangedToken.value);
            m_scrollingPresenterVerticalScrollModeChangedToken.value = 0;
        }

#ifdef USE_SCROLLMODE_AUTO
        if (m_scrollingPresenterComputedHorizontalScrollModeChangedToken.value != 0)
        {
            scrollingPresenterAsDO.UnregisterPropertyChangedCallback(winrt::ScrollingPresenter::ComputedHorizontalScrollModeProperty(), m_scrollingPresenterComputedHorizontalScrollModeChangedToken.value);
            m_scrollingPresenterComputedHorizontalScrollModeChangedToken.value = 0;
        }

        if (m_scrollingPresenterComputedVerticalScrollModeChangedToken.value != 0)
        {
            scrollingPresenterAsDO.UnregisterPropertyChangedCallback(winrt::ScrollingPresenter::ComputedVerticalScrollModeProperty(), m_scrollingPresenterComputedVerticalScrollModeChangedToken.value);
            m_scrollingPresenterComputedVerticalScrollModeChangedToken.value = 0;
        }
#endif
    }
}

void ScrollingView::HookHorizontalScrollControllerEvents()
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_horizontalScrollControllerIsEnabledChangedToken.value == 0);
    MUX_ASSERT(m_horizontalScrollControllerInteractionInfoChangedToken.value == 0);
    MUX_ASSERT(!m_onHorizontalScrollControllerPointerEnteredHandler);
    MUX_ASSERT(!m_onHorizontalScrollControllerPointerExitedHandler);

    if (winrt::IScrollController horizontalScrollController = m_horizontalScrollController.get())
    {
        m_horizontalScrollControllerInteractionInfoChangedToken =
            horizontalScrollController.InteractionInfoChanged({ this, &ScrollingView::OnScrollControllerInteractionInfoChanged });
    }

    if (winrt::IUIElement horizontalScrollControllerElement = m_horizontalScrollControllerElement.get())
    {
        if (auto horizontalScrollControllerControl = horizontalScrollControllerElement.try_as<winrt::Control>())
        {
            m_horizontalScrollControllerIsEnabledChangedToken =
                horizontalScrollControllerControl.IsEnabledChanged({ this, &ScrollingView::OnHorizontalScrollControllerIsEnabledChanged });
        }

        m_onHorizontalScrollControllerPointerEnteredHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollingView::OnHorizontalScrollControllerPointerEntered });
        horizontalScrollControllerElement.AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onHorizontalScrollControllerPointerEnteredHandler, true);

        m_onHorizontalScrollControllerPointerExitedHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollingView::OnHorizontalScrollControllerPointerExited });
        horizontalScrollControllerElement.AddHandler(winrt::UIElement::PointerExitedEvent(), m_onHorizontalScrollControllerPointerExitedHandler, true);
    }
}

void ScrollingView::UnhookHorizontalScrollControllerEvents()
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

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

void ScrollingView::HookVerticalScrollControllerEvents()
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_verticalScrollControllerIsEnabledChangedToken.value == 0);
    MUX_ASSERT(m_verticalScrollControllerInteractionInfoChangedToken.value == 0);
    MUX_ASSERT(!m_onVerticalScrollControllerPointerEnteredHandler);
    MUX_ASSERT(!m_onVerticalScrollControllerPointerExitedHandler);

    if (winrt::IScrollController verticalScrollController = m_verticalScrollController.get())
    {
        m_verticalScrollControllerInteractionInfoChangedToken =
            verticalScrollController.InteractionInfoChanged({ this, &ScrollingView::OnScrollControllerInteractionInfoChanged });
    }

    if (winrt::IUIElement verticalScrollControllerElement = m_verticalScrollControllerElement.get())
    {
        if (auto verticalScrollControllerControl = verticalScrollControllerElement.try_as<winrt::Control>())
        {
            m_verticalScrollControllerIsEnabledChangedToken =
                verticalScrollControllerControl.IsEnabledChanged({ this, &ScrollingView::OnVerticalScrollControllerIsEnabledChanged });
        }

        m_onVerticalScrollControllerPointerEnteredHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollingView::OnVerticalScrollControllerPointerEntered });
        verticalScrollControllerElement.AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onVerticalScrollControllerPointerEnteredHandler, true);

        m_onVerticalScrollControllerPointerExitedHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollingView::OnVerticalScrollControllerPointerExited });
        verticalScrollControllerElement.AddHandler(winrt::UIElement::PointerExitedEvent(), m_onVerticalScrollControllerPointerExitedHandler, true);
    }
}

void ScrollingView::UnhookVerticalScrollControllerEvents()
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

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

void ScrollingView::UpdateScrollingPresenter(const winrt::ScrollingPresenter& scrollingPresenter)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollingPresenterEvents(false /*isForDestructor*/);
    m_scrollingPresenter.set(nullptr);

    SetValue(s_ScrollingPresenterProperty, scrollingPresenter);

    if (scrollingPresenter)
    {
        m_scrollingPresenter.set(scrollingPresenter);
        HookScrollingPresenterEvents();
    }
}

void ScrollingView::UpdateHorizontalScrollController(
    const winrt::IScrollController& horizontalScrollController,
    const winrt::IUIElement& horizontalScrollControllerElement)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookHorizontalScrollControllerEvents();

    m_horizontalScrollController.set(horizontalScrollController);
    m_horizontalScrollControllerElement.set(horizontalScrollControllerElement);
    HookHorizontalScrollControllerEvents();
    UpdateScrollingPresenterHorizontalScrollController(horizontalScrollController);    
}

void ScrollingView::UpdateScrollingPresenterHorizontalScrollController(const winrt::IScrollController& horizontalScrollController)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        scrollingPresenter.HorizontalScrollController(horizontalScrollController);
    }
}

void ScrollingView::UpdateVerticalScrollController(
    const winrt::IScrollController& verticalScrollController,
    const winrt::IUIElement& verticalScrollControllerElement)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookVerticalScrollControllerEvents();

    m_verticalScrollController.set(verticalScrollController);
    m_verticalScrollControllerElement.set(verticalScrollControllerElement);
    HookVerticalScrollControllerEvents();
    UpdateScrollingPresenterVerticalScrollController(verticalScrollController);
}

void ScrollingView::UpdateAreHorizontalScrollControllerInteractionsAllowed()
{
    bool oldAreHorizontalScrollControllerInteractionsAllowed = m_areHorizontalScrollControllerInteractionsAllowed;

    if (!m_scrollingPresenter ||
        m_scrollingPresenter.get().HorizontalScrollMode() == winrt::ScrollingScrollMode::Disabled ||
        m_scrollingPresenter.get().ScrollableWidth() <= 0.0)
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
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        UpdateScrollControllersVisibility(true /*horizontalChange*/, false /*verticalChange*/);
    }
}

void ScrollingView::UpdateAreVerticalScrollControllerInteractionsAllowed()
{
    bool oldAreVerticalScrollControllerInteractionsAllowed = m_areVerticalScrollControllerInteractionsAllowed;

    if (!m_scrollingPresenter ||
        m_scrollingPresenter.get().VerticalScrollMode() == winrt::ScrollingScrollMode::Disabled ||
        m_scrollingPresenter.get().ScrollableHeight() <= 0.0)
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
        SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        UpdateScrollControllersVisibility(false /*horizontalChange*/, true /*verticalChange*/);
    }
}

void ScrollingView::UpdateScrollingPresenterVerticalScrollController(const winrt::IScrollController& verticalScrollController)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scrollingPresenter = m_scrollingPresenter.get())
    {
        scrollingPresenter.VerticalScrollController(verticalScrollController);
    }
}

void ScrollingView::UpdateScrollControllersSeparator(const winrt::IUIElement& scrollControllersSeparator)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_scrollControllersSeparatorElement.set(scrollControllersSeparator);
}

void ScrollingView::UpdateScrollControllersVisibility(
    bool horizontalChange, bool verticalChange)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(horizontalChange || verticalChange);

    bool isHorizontalScrollControllerVisible = false;

    if (horizontalChange)
    {
        winrt::ScrollingScrollBarVisibility scrollBarVisibility = HorizontalScrollBarVisibility();

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
        winrt::ScrollingScrollBarVisibility scrollBarVisibility = VerticalScrollBarVisibility();

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

bool ScrollingView::IsInputKindIgnored(winrt::ScrollingInputKinds const& inputKind)
{
    return (IgnoredInputKind() & inputKind) == inputKind;
}

bool ScrollingView::AreAllScrollControllersCollapsed() const
{
    return (!m_horizontalScrollControllerElement || m_horizontalScrollControllerElement.get().Visibility() == winrt::Visibility::Collapsed) &&
           (!m_verticalScrollControllerElement || m_verticalScrollControllerElement.get().Visibility() == winrt::Visibility::Collapsed);
}

bool ScrollingView::AreBothScrollControllersVisible() const
{
    return m_horizontalScrollControllerElement && m_horizontalScrollControllerElement.get().Visibility() == winrt::Visibility::Visible &&
           m_verticalScrollControllerElement && m_verticalScrollControllerElement.get().Visibility() == winrt::Visibility::Visible;
}

bool ScrollingView::AreScrollControllersAutoHiding()
{
    // Use the cached value unless it was invalidated.
    if (m_autoHideScrollControllersValid)
    {
        return m_autoHideScrollControllers;
    }

    m_autoHideScrollControllersValid = true;

    if (SharedHelpers::IsRS4OrHigher())
    {
        if (auto globalTestHooks = ScrollingViewTestHooks::GetGlobalTestHooks())
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

bool ScrollingView::IsScrollControllersSeparatorVisible() const
{
    return m_scrollControllersSeparatorElement && m_scrollControllersSeparatorElement.get().Visibility() == winrt::Visibility::Visible;
}

void ScrollingView::HideIndicators(
    bool useTransitions)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, useTransitions, m_keepIndicatorsShowing);

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

void ScrollingView::HideIndicatorsAfterDelay()
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, m_keepIndicatorsShowing);

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
            hideIndicatorsTimer.Tick({ this, &ScrollingView::OnHideIndicatorsTimerTick });
            m_hideIndicatorsTimer.set(hideIndicatorsTimer);
        }

        hideIndicatorsTimer.Start();
    }
}

// On RS4 and RS5, update m_autoHideScrollControllers based on the DynamicScrollbars registry key value
// and update the visual states if the value changed.
void ScrollingView::UpdateScrollControllersAutoHiding(
    bool forceUpdate)
{
    if ((forceUpdate || (!m_uiSettings5 && SharedHelpers::IsRS4OrHigher())) && m_autoHideScrollControllersValid)
    {
        m_autoHideScrollControllersValid = false;

        bool oldAutoHideScrollControllers = m_autoHideScrollControllers;
        bool newAutoHideScrollControllers = AreScrollControllersAutoHiding();

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

void ScrollingView::UpdateVisualStates(
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
void ScrollingView::UpdateScrollControllersVisualState(
    bool useTransitions,
    bool showIndicators,
    bool hideIndicators)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, useTransitions);
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, showIndicators, hideIndicators);

    MUX_ASSERT(!(showIndicators && hideIndicators));

    bool areScrollControllersAutoHiding = AreScrollControllersAutoHiding();

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
void ScrollingView::UpdateScrollControllersSeparatorVisualState(
    bool useTransitions,
    bool scrollControllersAutoHidingChanged)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, useTransitions, scrollControllersAutoHidingChanged);

    if (!IsScrollControllersSeparatorVisible())
    {
        return;
    }

    bool isEnabled = IsEnabled();
    bool areScrollControllersAutoHiding = AreScrollControllersAutoHiding();
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
        // When OS animations are turned on, show the separator when a scroll controller is shown unless the ScrollingView is disabled, using an animation.
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
        // When the ScrollingView is disabled, hide the separator in sync with the ScrollBar(s).
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

void ScrollingView::GoToState(std::wstring_view const& stateName, bool useTransitions)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, stateName.data(), useTransitions);

    winrt::VisualStateManager::GoToState(*this, stateName, useTransitions);
}

void ScrollingView::OnKeyDown(winrt::KeyRoutedEventArgs const& e)
{
    SCROLLINGVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::KeyRoutedEventArgsToString(e).c_str());

    __super::OnKeyDown(e);
    
    m_preferMouseIndicators = false;

    if (m_scrollingPresenter)
    {
        winrt::KeyRoutedEventArgs eventArgs = e.as<winrt::KeyRoutedEventArgs>();
        if (!eventArgs.Handled())
        {
            auto originalKey = eventArgs.OriginalKey();
            bool isGamepadKey = FocusHelper::IsGamepadNavigationDirection(originalKey) || FocusHelper::IsGamepadPageNavigationDirection(originalKey);

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

void ScrollingView::HandleKeyDownForStandardScroll(winrt::KeyRoutedEventArgs args)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::KeyRoutedEventArgsToString(args).c_str());

    // Up/Down/Left/Right will scroll by 15% the size of the viewport.
    static const double smallScrollProportion = 0.15;

    MUX_ASSERT(!args.Handled());
    MUX_ASSERT(m_scrollingPresenter != nullptr);

    bool isHandled = DoScrollForKey(args.Key(), smallScrollProportion);

    args.Handled(isHandled);
}

void ScrollingView::HandleKeyDownForXYNavigation(winrt::KeyRoutedEventArgs args)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::KeyRoutedEventArgsToString(args).c_str());

    MUX_ASSERT(!args.Handled());
    MUX_ASSERT(m_scrollingPresenter != nullptr);

    bool isHandled = false;
    auto originalKey = args.OriginalKey();
    auto scrollingPresenter = m_scrollingPresenter.get().as<winrt::ScrollingPresenter>();
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
            auto elementBounds = nextElementAsUIE.TransformToVisual(scrollingPresenter).TransformBounds(rect);
            auto viewport = winrt::Rect{ 0, 0, static_cast<float>(scrollingPresenter.ActualWidth()), static_cast<float>(scrollingPresenter.ActualHeight()) };

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
            SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_METH_INT, METH_NAME, this, L"FocusManager::TryFocusAsync", SharedHelpers::IsAnimationsEnabled());

            auto focusAsyncOperation = winrt::FocusManager::TryFocusAsync(nextElement, winrt::FocusState::Keyboard);

            if (SharedHelpers::IsAnimationsEnabled()) // When system animations are turned off, the bring-into-view operations are not turned into animations.
            {
                focusAsyncOperation.Completed(winrt::AsyncOperationCompletedHandler<winrt::FocusMovementResult>(
                    [strongThis = get_strong(), targetElement = nextElement.try_as<winrt::UIElement>()](winrt::IAsyncOperation<winrt::FocusMovementResult> asyncOperation, winrt::AsyncStatus asyncStatus)
                    {
                        SCROLLINGVIEW_TRACE_VERBOSE(*strongThis, TRACE_MSG_METH_INT, METH_NAME, strongThis, static_cast<int>(asyncStatus));

                        if (asyncStatus == winrt::AsyncStatus::Completed && asyncOperation.GetResults())
                        {
                            // The focus change request was successful. One or a few ScrollingPresenter::BringingIntoView notifications are likely to be raised in the coming ticks.
                            // For those, the BringIntoViewRequestedEventArgs::AnimationDesired property will be set to True in order to animate to the target element rather than jumping.
                            SCROLLINGVIEW_TRACE_VERBOSE(*strongThis, TRACE_MSG_METH_PTR, METH_NAME, strongThis, targetElement);

                            auto bringIntoViewOperation(std::make_shared<ScrollingViewBringIntoViewOperation>(targetElement));

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
                    DoScroll(scrollingPresenter.ActualHeight() * scrollAmountProportion, winrt::Orientation::Vertical);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Up && CanScrollUp())
                {
                    isHandled = true;
                    DoScroll(-scrollingPresenter.ActualHeight() * scrollAmountProportion, winrt::Orientation::Vertical);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Right && CanScrollRight())
                {
                    isHandled = true;
                    DoScroll(scrollingPresenter.ActualWidth() * scrollAmountProportion * (FlowDirection() == winrt::FlowDirection::RightToLeft ? -1 : 1), winrt::Orientation::Horizontal);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Left && CanScrollLeft())
                {
                    isHandled = true;
                    DoScroll(-scrollingPresenter.ActualWidth() * scrollAmountProportion * (FlowDirection() == winrt::FlowDirection::RightToLeft ? -1 : 1), winrt::Orientation::Horizontal);
                }
            }
        }
    }

    args.Handled(isHandled);
}

winrt::DependencyObject ScrollingView::GetNextFocusCandidate(winrt::FocusNavigationDirection navigationDirection, bool isPageNavigation)
{
    MUX_ASSERT(m_scrollingPresenter != nullptr);
    MUX_ASSERT(navigationDirection != winrt::FocusNavigationDirection::None);
    auto scrollingPresenter = m_scrollingPresenter.get().as<winrt::ScrollingPresenter>();

    winrt::FocusNavigationDirection focusDirection = navigationDirection;

    winrt::FindNextElementOptions findNextElementOptions;
    findNextElementOptions.SearchRoot(scrollingPresenter.Content());

    if (isPageNavigation)
    {
        auto localBounds = winrt::Rect{ 0, 0, static_cast<float>(scrollingPresenter.ActualWidth()), static_cast<float>(scrollingPresenter.ActualHeight()) };
        auto globalBounds = scrollingPresenter.TransformToVisual(nullptr).TransformBounds(localBounds);
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

bool ScrollingView::DoScrollForKey(winrt::VirtualKey key, double scrollProportion)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_INT, METH_NAME, this, scrollProportion, static_cast<int>(key));

    MUX_ASSERT(m_scrollingPresenter != nullptr);

    bool isScrollTriggered = false;
    auto scrollingPresenter = m_scrollingPresenter.get().as<winrt::ScrollingPresenter>();

    if (key == winrt::VirtualKey::Down && CanScrollDown())
    {
        isScrollTriggered = true;
        DoScroll(scrollingPresenter.ActualHeight() * scrollProportion, winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::Up && CanScrollUp())
    {
        isScrollTriggered = true;
        DoScroll(scrollingPresenter.ActualHeight() * -scrollProportion, winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::PageDown && CanScrollDown())
    {
        isScrollTriggered = true;
        DoScroll(scrollingPresenter.ActualHeight(), winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::PageUp && CanScrollUp())
    {
        isScrollTriggered = true;
        DoScroll(-scrollingPresenter.ActualHeight(), winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::Left || key == winrt::VirtualKey::Right)
    {
        double scrollAmount = scrollingPresenter.ActualWidth() * scrollProportion;
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
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollingScrollMode verticalScrollMode = ComputedVerticalScrollMode();
#else
        winrt::ScrollingScrollMode verticalScrollMode = VerticalScrollMode();
#endif

        if (canScrollUp || (verticalScrollMode == winrt::ScrollingScrollMode::Disabled && CanScrollLeft()))
        {
            isScrollTriggered = true;
            auto horizontalOffset = canScrollUp ? scrollingPresenter.HorizontalOffset() : 0.0;
            auto verticalOffset = canScrollUp ? 0.0 : scrollingPresenter.VerticalOffset();

            if (!canScrollUp && FlowDirection() == winrt::FlowDirection::RightToLeft)
            {
                horizontalOffset = scrollingPresenter.ExtentWidth() * scrollingPresenter.ZoomFactor() - scrollingPresenter.ActualWidth();
            }

            scrollingPresenter.ScrollTo(horizontalOffset, verticalOffset);
        }
    }
    else if (key == winrt::VirtualKey::End)
    {
        bool canScrollDown = CanScrollDown();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollingScrollMode verticalScrollMode = ComputedVerticalScrollMode();
#else
        winrt::ScrollingScrollMode verticalScrollMode = VerticalScrollMode();
#endif

        if (canScrollDown || (verticalScrollMode == winrt::ScrollingScrollMode::Disabled && CanScrollRight()))
        {
            isScrollTriggered = true;
            auto zoomedExtent = (canScrollDown ? scrollingPresenter.ExtentHeight() : scrollingPresenter.ExtentWidth()) * scrollingPresenter.ZoomFactor();
            auto horizontalOffset = canScrollDown ? scrollingPresenter.HorizontalOffset() : zoomedExtent - scrollingPresenter.ActualWidth();
            auto verticalOffset = canScrollDown ? zoomedExtent - scrollingPresenter.ActualHeight() : scrollingPresenter.VerticalOffset();

            if (!canScrollDown && FlowDirection() == winrt::FlowDirection::RightToLeft)
            {
                horizontalOffset = 0.0;
            }

            scrollingPresenter.ScrollTo(horizontalOffset, verticalOffset);
        }
    }

    return isScrollTriggered;
}

void ScrollingView::DoScroll(double offset, winrt::Orientation orientation)
{
    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_INT, METH_NAME, this, offset, static_cast<int>(orientation));

    bool isVertical = orientation == winrt::Orientation::Vertical;

    if (auto scrollingPresenter = m_scrollingPresenter.get().as<winrt::ScrollingPresenter>())
    {
        if (SharedHelpers::IsAnimationsEnabled())
        {
            static const winrt::float2 inertiaDecayRate(0.9995f, 0.9995f);

            // A velocity less than or equal to this value has no effect.
            static const double minVelocity = 30.0;

            // We need to add this much velocity over minVelocity per pixel we want to move:
            static constexpr double s_velocityNeededPerPixel{ 7.600855902349023 };

            int scrollDir = offset > 0 ? 1 : -1;

            // The minimum velocity required to move in the given direction.
            double baselineVelocity = minVelocity * scrollDir;

            // If there is already a scroll animation running for a previous key press, we want to take that into account
            // for calculating the baseline velocity. 
            auto previousScrollViewChangeId = isVertical ? m_verticalScrollFromOffsetChangeId : m_horizontalScrollFromOffsetChangeId;
            if (previousScrollViewChangeId != -1)
            {
                auto directionOfPreviousScrollOperation = isVertical ? m_verticalScrollFromDirection : m_horizontalScrollFromDirection;
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
                winrt::float2 offsetsVelocity(0.0f, velocity);
                m_verticalScrollFromOffsetChangeId = scrollingPresenter.ScrollFrom(offsetsVelocity, inertiaDecayRate).OffsetsChangeId;
                m_verticalScrollFromDirection = scrollDir;
            }
            else
            {
                winrt::float2 offsetsVelocity(velocity, 0.0f);
                m_horizontalScrollFromOffsetChangeId = scrollingPresenter.ScrollFrom(offsetsVelocity, inertiaDecayRate).OffsetsChangeId;
                m_horizontalScrollFromDirection = scrollDir;
            }
        }
        else
        {
            if (isVertical)
            {
                // Any horizontal ScrollFrom animation recently launched should be ignored by a potential subsequent ScrollFrom call.
                m_verticalScrollFromOffsetChangeId = -1;

                scrollingPresenter.ScrollBy(0.0 /*horizontalOffsetDelta*/, offset /*verticalOffsetDelta*/).OffsetsChangeId;
            }
            else
            {
                // Any vertical ScrollFrom animation recently launched should be ignored by a potential subsequent ScrollFrom call.
                m_horizontalScrollFromOffsetChangeId = -1;

                scrollingPresenter.ScrollBy(offset /*horizontalOffsetDelta*/, 0.0 /*verticalOffsetDelta*/).OffsetsChangeId;
            }
        }
    }
}

bool ScrollingView::CanScrollInDirection(winrt::FocusNavigationDirection direction)
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

bool ScrollingView::CanScrollDown()
{
    return CanScrollVerticallyInDirection(true /*inPositiveDirection*/);
}

bool ScrollingView::CanScrollUp()
{
    return CanScrollVerticallyInDirection(false /*inPositiveDirection*/);
}

bool ScrollingView::CanScrollRight()
{
    return CanScrollHorizontallyInDirection(true /*inPositiveDirection*/);
}

bool ScrollingView::CanScrollLeft()
{
    return CanScrollHorizontallyInDirection(false /*inPositiveDirection*/);
}

bool ScrollingView::CanScrollVerticallyInDirection(bool inPositiveDirection)
{
    bool canScrollInDirection = false;
    if (m_scrollingPresenter)
    {
        auto scrollingPresenter = m_scrollingPresenter.get().as<winrt::ScrollingPresenter>();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollingScrollMode verticalScrollMode = ComputedVerticalScrollMode();
#else
        winrt::ScrollingScrollMode verticalScrollMode = VerticalScrollMode();
#endif

        if (verticalScrollMode == winrt::ScrollingScrollMode::Enabled)
        {
            auto zoomedExtentHeight = scrollingPresenter.ExtentHeight() * scrollingPresenter.ZoomFactor();
            auto viewportHeight = scrollingPresenter.ActualHeight();
            if (zoomedExtentHeight > viewportHeight)
            {
                if (inPositiveDirection)
                {
                    auto maxVerticalOffset = zoomedExtentHeight - viewportHeight;
                    if (scrollingPresenter.VerticalOffset() < maxVerticalOffset)
                    {
                        canScrollInDirection = true;
                    }
                }
                else
                {
                    if (scrollingPresenter.VerticalOffset() > 0)
                    {
                        canScrollInDirection = true;
                    }
                }
            }
        }
    }

    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, inPositiveDirection, canScrollInDirection);

    return canScrollInDirection;
}

bool ScrollingView::CanScrollHorizontallyInDirection(bool inPositiveDirection)
{
    bool canScrollInDirection = false;

    if (FlowDirection() == winrt::FlowDirection::RightToLeft)
    {
        inPositiveDirection = !inPositiveDirection;
    }

    if (m_scrollingPresenter)
    {
        auto scrollingPresenter = m_scrollingPresenter.get().as<winrt::ScrollingPresenter>();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollingScrollMode horizontalScrollMode = ComputedHorizontalScrollMode();
#else
        winrt::ScrollingScrollMode horizontalScrollMode = HorizontalScrollMode();
#endif

        if (horizontalScrollMode == winrt::ScrollingScrollMode::Enabled)
        {
            auto zoomedExtentWidth = scrollingPresenter.ExtentWidth() * scrollingPresenter.ZoomFactor();
            auto viewportWidth = scrollingPresenter.ActualWidth();
            if (zoomedExtentWidth > viewportWidth)
            {
                if (inPositiveDirection)
                {
                    auto maxHorizontalOffset = zoomedExtentWidth - viewportWidth;
                    if (scrollingPresenter.HorizontalOffset() < maxHorizontalOffset)
                    {
                        canScrollInDirection = true;
                    }
                }
                else
                {
                    if (scrollingPresenter.HorizontalOffset() > 0)
                    {
                        canScrollInDirection = true;
                    }
                }
            }
        }
    }

    SCROLLINGVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, inPositiveDirection, canScrollInDirection);

    return canScrollInDirection;
}

#ifdef _DEBUG

winrt::hstring ScrollingView::DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty)
{
    if (dependencyProperty == s_ContentProperty)
    {
        return L"Content";
    }
    else if (dependencyProperty == s_ScrollingPresenterProperty)
    {
        return L"ScrollingPresenter";
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
    else if (dependencyProperty == s_IgnoredInputKindProperty)
    {
        return L"IgnoredInputKind";
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

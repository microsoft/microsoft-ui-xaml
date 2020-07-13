// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollingScrollOptions.h"
#include "ScrollBarController.h"
#include "ScrollView.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ScrollControllerScrollToRequestedEventArgs.h"
#include "ScrollControllerScrollByRequestedEventArgs.h"
#include "ScrollControllerAddScrollVelocityRequestedEventArgs.h"

ScrollBarController::ScrollBarController()
{
    SCROLLVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

ScrollBarController::~ScrollBarController()
{
    SCROLLVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollBarEvent();
#if defined(USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED) || defined(_DEBUG)
    UnhookScrollBarPropertyChanged();
#endif
}

void ScrollBarController::SetScrollBar(const winrt::ScrollBar& scrollBar)
{
    SCROLLVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollBarEvent();

    m_scrollBar = scrollBar;

    HookScrollBarEvent();
#if defined(USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED) || defined(_DEBUG)
    HookScrollBarPropertyChanged();
#endif
}

#pragma region IScrollController

#if USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
bool ScrollBarController::AreScrollControllerInteractionsAllowed()
{
    return m_areScrollControllerInteractionsAllowed;
}
#endif

#if USE_SCROLLCONTROLLER_ARESCROLLERINTERACTIONSALLOWED
bool ScrollBarController::AreScrollerInteractionsAllowed()
{
    return m_areScrollerInteractionsAllowed;
}
#endif

#if USE_SCROLLCONTROLLER_ISINTERACTIONELEMENTRAILENABLED
bool ScrollBarController::IsInteractionElementRailEnabled()
{
    // Unused because InteractionElement returns null.
    return true;
}
#endif

bool ScrollBarController::IsScrolling()
{
    return m_isScrolling;
}

winrt::UIElement ScrollBarController::InteractionElement()
{
    // This IScrollController implementation has no touch-manipulatable element.
    return nullptr;
}

winrt::Orientation ScrollBarController::InteractionElementScrollOrientation()
{
    // Unused because InteractionElement returns null.
    MUX_ASSERT(m_scrollBar);
    return m_scrollBar.Orientation();
}

void ScrollBarController::SetExpressionAnimationSources(
    winrt::CompositionPropertySet const& propertySet,
    winrt::hstring const& offsetPropertyName,
    winrt::hstring const& scrollableExtentPropertyName,
    winrt::hstring const& multiplierPropertyName)
{
    // Unused because InteractionElement returns null.
    SCROLLVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

void ScrollBarController::SetScrollMode(
    winrt::ScrollingScrollMode const& scrollMode)
{
    SCROLLVIEW_TRACE_INFO(
        nullptr,
        TRACE_MSG_METH_STR,
        METH_NAME,
        this,
        TypeLogging::ScrollModeToString(scrollMode).c_str());
    m_scrollMode = scrollMode;

#ifdef USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
    UpdateAreScrollControllerInteractionsAllowed();
#endif
}

void ScrollBarController::SetDimensions(
    double offset,
    double scrollableExtent,
    double viewport)
{
    SCROLLVIEW_TRACE_INFO(
        nullptr,
        L"%s[0x%p](offset:%lf, scrollableExtent:%lf, viewport:%lf, operationsCount:%d)\n",
        METH_NAME,
        this,
        offset,
        scrollableExtent,
        viewport,
        m_operationsCount);

    if (scrollableExtent < 0.0)
    {
        throw winrt::hresult_invalid_argument(L"scrollableExtent cannot be negative.");
    }

    if (viewport < 0.0)
    {
        throw winrt::hresult_invalid_argument(L"viewport cannot be negative.");
    }

    offset = max(0.0, offset);
    offset = min(scrollableExtent, offset);
    m_lastOffset = offset;

    MUX_ASSERT(m_scrollBar);

    if (0.0 < m_scrollBar.Minimum())
    {
        m_scrollBar.Minimum(0.0);
    }

    if (scrollableExtent > m_scrollBar.Maximum())
    {
        m_scrollBar.Maximum(scrollableExtent);
    }

    if (0.0 != m_scrollBar.Minimum())
    {
        m_scrollBar.Minimum(0.0);
    }

    if (scrollableExtent != m_scrollBar.Maximum())
    {
        m_scrollBar.Maximum(scrollableExtent);
    }

    m_scrollBar.ViewportSize(viewport);
    m_scrollBar.LargeChange(viewport);
    m_scrollBar.SmallChange(max(1.0, viewport / s_defaultViewportToSmallChangeRatio));

    // The ScrollBar Value is only updated when there is no operation in progress.
    if (m_operationsCount == 0 || m_scrollBar.Value() < 0.0 || m_scrollBar.Value() > scrollableExtent)
    {
        m_scrollBar.Value(offset);
        m_lastScrollBarValue = offset;
    }

#ifdef USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
    // Potentially changed ScrollBar.Minimum / ScrollBar.Maximum value(s) may have an effect
    // on the read-only IScrollController.AreScrollControllerInteractionsAllowed property.
    UpdateAreScrollControllerInteractionsAllowed();
#endif
}

winrt::CompositionAnimation ScrollBarController::GetScrollAnimation(
    int info,
    winrt::float2 const& currentPosition,
    winrt::CompositionAnimation const& defaultAnimation)
{
    SCROLLVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, info);

    // Using the consumer's default animation.
    return nullptr;
}

void ScrollBarController::NotifyScrollCompleted(
    int info)
{
    SCROLLVIEW_TRACE_INFO(
        nullptr,
        TRACE_MSG_METH_INT,
        METH_NAME,
        this,
        info);

    MUX_ASSERT(m_operationsCount > 0);
    m_operationsCount--;

    if (m_operationsCount == 0 && m_scrollBar && m_scrollBar.Value() != m_lastOffset)
    {
        m_scrollBar.Value(m_lastOffset);
        m_lastScrollBarValue = m_lastOffset;
    }
}

winrt::event_token ScrollBarController::ScrollToRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollToRequestedEventArgs> const& value)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    return m_scrollToRequested.add(value);
}

void ScrollBarController::ScrollToRequested(winrt::event_token const& token)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_scrollToRequested.remove(token);
}

winrt::event_token ScrollBarController::ScrollByRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollByRequestedEventArgs> const& value)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    return m_scrollByRequested.add(value);
}

void ScrollBarController::ScrollByRequested(winrt::event_token const& token)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_scrollByRequested.remove(token);
}

winrt::event_token ScrollBarController::AddScrollVelocityRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerAddScrollVelocityRequestedEventArgs> const& value)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    return m_addScrollVelocityRequested.add(value);
}

void ScrollBarController::AddScrollVelocityRequested(winrt::event_token const& token)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_addScrollVelocityRequested.remove(token);
}

winrt::event_token ScrollBarController::InteractionRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerInteractionRequestedEventArgs> const& value)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    // Because this IScrollController implementation does not expose an InteractionElement, 
    // this InteractionRequested event is not going to be raised.
    return {};
}

void ScrollBarController::InteractionRequested(winrt::event_token const& token)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    // Because this IScrollController implementation does not expose an InteractionElement, 
    // this InteractionRequested event is not going to be raised.
}

winrt::event_token ScrollBarController::InteractionInfoChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    return m_interactionInfoChanged.add(value);
}

void ScrollBarController::InteractionInfoChanged(winrt::event_token const& token)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_interactionInfoChanged.remove(token);
}

#pragma endregion

#if defined(USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED) || defined(_DEBUG)
void ScrollBarController::HookScrollBarPropertyChanged()
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

#ifdef _DEBUG
    MUX_ASSERT(m_scrollBarIndicatorModeChangedToken.value == 0);
    MUX_ASSERT(m_scrollBarVisibilityChangedToken.value == 0);
#endif //_DEBUG
#if defined(USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED) || defined(_DEBUG)
    MUX_ASSERT(m_scrollBarIsEnabledChangedToken.value == 0);
#endif // USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED || _DEBUG

    if (m_scrollBar)
    {
#ifdef _DEBUG
        m_scrollBarIndicatorModeChangedToken.value = m_scrollBar.RegisterPropertyChangedCallback(
            winrt::ScrollBar::IndicatorModeProperty(), { this, &ScrollBarController::OnScrollBarPropertyChanged });

        m_scrollBarVisibilityChangedToken.value = m_scrollBar.RegisterPropertyChangedCallback(
            winrt::UIElement::VisibilityProperty(), { this, &ScrollBarController::OnScrollBarPropertyChanged });
#endif //_DEBUG

#if defined(USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED) || defined(_DEBUG)
        m_scrollBarIsEnabledChangedToken.value = m_scrollBar.RegisterPropertyChangedCallback(
            winrt::Control::IsEnabledProperty(), { this, &ScrollBarController::OnScrollBarPropertyChanged });
#endif // USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED || _DEBUG
    }
}

void ScrollBarController::UnhookScrollBarPropertyChanged()
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (m_scrollBar)
    {
#ifdef _DEBUG
        if (m_scrollBarIndicatorModeChangedToken.value != 0)
        {
            m_scrollBar.UnregisterPropertyChangedCallback(winrt::ScrollBar::IndicatorModeProperty(), m_scrollBarIndicatorModeChangedToken.value);
            m_scrollBarIndicatorModeChangedToken.value = 0;
        }

        if (m_scrollBarVisibilityChangedToken.value != 0)
        {
            m_scrollBar.UnregisterPropertyChangedCallback(winrt::UIElement::VisibilityProperty(), m_scrollBarVisibilityChangedToken.value);
            m_scrollBarVisibilityChangedToken.value = 0;
        }
#endif //_DEBUG

#if defined(USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED) || defined(_DEBUG)
        if (m_scrollBarIsEnabledChangedToken.value != 0)
        {
            m_scrollBar.UnregisterPropertyChangedCallback(winrt::Control::IsEnabledProperty(), m_scrollBarIsEnabledChangedToken.value);
            m_scrollBarIsEnabledChangedToken.value = 0;
        }
#endif // USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED || _DEBUG
    }
}
#endif // USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED || _DEBUG

#if USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
void ScrollBarController::UpdateAreScrollControllerInteractionsAllowed()
{
    const bool oldAreScrollControllerInteractionsAllowed = m_areScrollControllerInteractionsAllowed;

    m_areScrollControllerInteractionsAllowed =
        m_scrollBar &&
        m_scrollBar.IsEnabled() &&
        m_scrollBar.Maximum() > m_scrollBar.Minimum() &&
        m_scrollMode != winrt::ScrollingScrollMode::Disabled;

    if (oldAreScrollControllerInteractionsAllowed != m_areScrollControllerInteractionsAllowed)
    {
        RaiseInteractionInfoChanged();
    }
}
#endif

void ScrollBarController::HookScrollBarEvent()
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_scrollBarScrollToken.value == 0);

    if (m_scrollBar)
    {
        m_scrollBarScrollToken = m_scrollBar.Scroll({ this, &ScrollBarController::OnScroll });
    }
}

void ScrollBarController::UnhookScrollBarEvent()
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (m_scrollBar && m_scrollBarScrollToken.value != 0)
    {
        m_scrollBar.Scroll(m_scrollBarScrollToken);
        m_scrollBarScrollToken.value = 0;
    }
}

#if defined(USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED) || defined(_DEBUG)
void ScrollBarController::OnScrollBarPropertyChanged(
    const winrt::DependencyObject& /*sender*/,
    const winrt::DependencyProperty& args)
{
    MUX_ASSERT(m_scrollBar);

    if (args == winrt::Control::IsEnabledProperty())
    {
        SCROLLVIEW_TRACE_VERBOSE(
            nullptr,
            TRACE_MSG_METH_STR_INT,
            METH_NAME,
            this,
            L"IsEnabled",
            m_scrollBar.IsEnabled());

#ifdef USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
        // Potentially changed ScrollBar.IsEnabled value may have an effect
        // on the read-only IScrollController.AreScrollControllerInteractionsAllowed property.
        UpdateAreScrollControllerInteractionsAllowed();
#endif
    }
#ifdef _DEBUG
    else if (args == winrt::UIElement::VisibilityProperty())
    {
        SCROLLVIEW_TRACE_VERBOSE(
            nullptr,
            TRACE_MSG_METH_STR_INT,
            METH_NAME,
            this,
            L"Visibility",
            m_scrollBar.Visibility());
    }
    else if (args == winrt::ScrollBar::IndicatorModeProperty())
    {
        SCROLLVIEW_TRACE_VERBOSE(
            nullptr,
            TRACE_MSG_METH_STR_STR,
            METH_NAME,
            this,
            L"IndicatorMode",
            TypeLogging::ScrollingIndicatorModeToString(m_scrollBar.IndicatorMode()).c_str());
    }
#endif //_DEBUG
}
#endif

void ScrollBarController::OnScroll(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollEventArgs& args)
{
    const auto scrollEventType = args.ScrollEventType();

    SCROLLVIEW_TRACE_VERBOSE(
        nullptr,
        TRACE_MSG_METH_STR,
        METH_NAME,
        this,
        TypeLogging::ScrollEventTypeToString(scrollEventType).c_str());

    if (!m_scrollBar)
    {
        return;
    }

    if (m_scrollMode == winrt::ScrollingScrollMode::Disabled && scrollEventType != winrt::ScrollEventType::ThumbPosition)
    {
        // This ScrollBar is not interactive. Restore its previous Value.
        m_scrollBar.Value(m_lastScrollBarValue);
        return;
    }

    switch (scrollEventType)
    {
    case winrt::ScrollEventType::First:
    case winrt::ScrollEventType::Last:
    {
        break;
    }
    case winrt::ScrollEventType::EndScroll:
    {
#if USE_SCROLLCONTROLLER_ARESCROLLERINTERACTIONSALLOWED
        m_areScrollerInteractionsAllowed = true;
#endif

        if (m_isScrolling)
        {
            m_isScrolling = false;
            RaiseInteractionInfoChanged();
        }
        break;
    }
    case winrt::ScrollEventType::LargeDecrement:
    case winrt::ScrollEventType::LargeIncrement:
    case winrt::ScrollEventType::SmallDecrement:
    case winrt::ScrollEventType::SmallIncrement:
    case winrt::ScrollEventType::ThumbPosition:
    case winrt::ScrollEventType::ThumbTrack:
    {
        if (scrollEventType == winrt::ScrollEventType::ThumbTrack)
        {
#if USE_SCROLLCONTROLLER_ARESCROLLERINTERACTIONSALLOWED
            m_areScrollerInteractionsAllowed = false;
#endif

            if (!m_isScrolling)
            {
                m_isScrolling = true;
                RaiseInteractionInfoChanged();
            }
        }

        bool offsetChangeRequested = false;

        if (scrollEventType == winrt::ScrollEventType::ThumbPosition ||
            scrollEventType == winrt::ScrollEventType::ThumbTrack)
        {
            offsetChangeRequested = RaiseScrollToRequested(args.NewValue());
        }
        else
        {
            double offsetChange = 0.0;

            switch (scrollEventType)
            {
            case winrt::ScrollEventType::LargeDecrement:
                offsetChange = -min(m_lastScrollBarValue - m_scrollBar.Minimum(), m_scrollBar.LargeChange());
                break;
            case winrt::ScrollEventType::LargeIncrement:
                offsetChange = min(m_scrollBar.Maximum() - m_lastScrollBarValue, m_scrollBar.LargeChange());
                break;
            case winrt::ScrollEventType::SmallDecrement:
                offsetChange = -min(m_lastScrollBarValue - m_scrollBar.Minimum(), m_scrollBar.SmallChange());
                break;
            case winrt::ScrollEventType::SmallIncrement:
                offsetChange = min(m_scrollBar.Maximum() - m_lastScrollBarValue, m_scrollBar.SmallChange());
                break;
            }

            // When the requested Value is near the Mininum or Maximum, include a little additional velocity
            // to ensure the extreme value is reached.
            if (args.NewValue() - m_scrollBar.Minimum() < s_minMaxEpsilon)
            {
                MUX_ASSERT(offsetChange < 0.0);
                offsetChange -= s_minMaxEpsilon;
            }
            else if (m_scrollBar.Maximum() - args.NewValue() < s_minMaxEpsilon)
            {
                MUX_ASSERT(offsetChange > 0.0);
                offsetChange += s_minMaxEpsilon;
            }

            if (SharedHelpers::IsAnimationsEnabled())
            {
                offsetChangeRequested = RaiseAddScrollVelocityRequested(offsetChange);
            }
            else
            {
                offsetChangeRequested = RaiseScrollByRequested(offsetChange);
            }
        }

        if (!offsetChangeRequested)
        {
            // This request could not be requested, restore the previous Value.
            m_scrollBar.Value(m_lastScrollBarValue);
        }
        break;
    }
    }

    m_lastScrollBarValue = m_scrollBar.Value();
}

bool ScrollBarController::RaiseScrollToRequested(
    double offset)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, offset);

    if (!m_scrollToRequested)
    {
        return false;
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
        return true;
    }

    return false;
}

bool ScrollBarController::RaiseScrollByRequested(
    double offsetChange)
{
    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, offsetChange);

    if (!m_scrollByRequested)
    {
        return false;
    }

    auto options = winrt::make_self<ScrollingScrollOptions>(
        winrt::ScrollingAnimationMode::Disabled,
        winrt::ScrollingSnapPointsMode::Ignore);

    auto scrollByRequestedEventArgs = winrt::make_self<ScrollControllerScrollByRequestedEventArgs>(
        offsetChange,
        *options);

    m_scrollByRequested(*this, *scrollByRequestedEventArgs);

    int32_t offsetChangeCorrelationId = scrollByRequestedEventArgs.as<winrt::ScrollControllerScrollByRequestedEventArgs>().CorrelationId();

    // Only increment m_operationsCount when the returned OffsetsChangeCorrelationId represents a new request that was not coalesced with a pending request. 
    if (offsetChangeCorrelationId != -1 && offsetChangeCorrelationId != m_lastOffsetChangeCorrelationIdForScrollBy)
    {
        m_lastOffsetChangeCorrelationIdForScrollBy = offsetChangeCorrelationId;
        m_operationsCount++;
        return true;
    }

    return false;
}

bool ScrollBarController::RaiseAddScrollVelocityRequested(
    double offsetChange)
{
    if (!m_addScrollVelocityRequested)
    {
        return false;
    }

    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, offsetChange);

    double offsetVelocity = m_operationsCount == 0 ? s_minimumVelocity : 0.0;

    if (offsetChange < 0.0)
    {
        offsetVelocity *= -1;
    }
    offsetVelocity += offsetChange * s_velocityNeededPerPixel;

    winrt::IInspectable inertiaDecayRateAsInsp = box_value(s_inertiaDecayRate);
    winrt::IReference<float> inertiaDecayRate = inertiaDecayRateAsInsp.as<winrt::IReference<float>>();

    auto addScrollVelocityRequestedEventArgs = winrt::make_self<ScrollControllerAddScrollVelocityRequestedEventArgs>(
        static_cast<float>(offsetVelocity),
        inertiaDecayRate);

    m_addScrollVelocityRequested(*this, *addScrollVelocityRequestedEventArgs);

    int32_t offsetChangeCorrelationId = addScrollVelocityRequestedEventArgs.as<winrt::ScrollControllerAddScrollVelocityRequestedEventArgs>().CorrelationId();

    // Only increment m_operationsCount when the returned OffsetsChangeCorrelationId represents a new request that was not coalesced with a pending request. 
    if (offsetChangeCorrelationId != -1 && offsetChangeCorrelationId != m_lastOffsetChangeCorrelationIdForAddScrollVelocity)
    {
        m_lastOffsetChangeCorrelationIdForAddScrollVelocity = offsetChangeCorrelationId;
        m_operationsCount++;
        return true;
    }

    return false;
}

void ScrollBarController::RaiseInteractionInfoChanged()
{
    if (!m_interactionInfoChanged)
    {
        return;
    }

    SCROLLVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_interactionInfoChanged(*this, nullptr);
}

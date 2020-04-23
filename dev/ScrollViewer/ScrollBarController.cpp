// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollOptions.h"
#include "ScrollBarController.h"
#include "ScrollViewer.h"
#include "TypeLogging.h"
#include "ScrollerTypeLogging.h"
#include "ScrollControllerScrollToRequestedEventArgs.h"
#include "ScrollControllerScrollByRequestedEventArgs.h"
#include "ScrollControllerScrollFromRequestedEventArgs.h"

ScrollBarController::ScrollBarController()
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

ScrollBarController::~ScrollBarController()
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollBarEvent();
    UnhookScrollBarPropertyChanged();
}

void ScrollBarController::SetScrollBar(const winrt::ScrollBar& scrollBar)
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollBarEvent();

    m_scrollBar = scrollBar;

    HookScrollBarEvent();
    HookScrollBarPropertyChanged();
}

#pragma region IScrollController

bool ScrollBarController::AreInteractionsAllowed()
{
    return m_areInteractionsAllowed;
}

bool ScrollBarController::AreScrollerInteractionsAllowed()
{
    return m_areScrollerInteractionsAllowed;
}

bool ScrollBarController::IsInteracting()
{
    return m_isInteracting;
}

bool ScrollBarController::IsInteractionVisualRailEnabled()
{
    // Unused because InteractionVisual returns null.
    return true;
}

winrt::Visual ScrollBarController::InteractionVisual()
{
    // This IScrollController implementation has no touch-manipulatable element.
    return nullptr;
}

winrt::Orientation ScrollBarController::InteractionVisualScrollOrientation()
{
    // Unused because InteractionVisual returns null.
    MUX_ASSERT(m_scrollBar);
    return m_scrollBar.Orientation();
}

void ScrollBarController::SetExpressionAnimationSources(
    winrt::CompositionPropertySet const& propertySet,
    winrt::hstring const& minOffsetPropertyName,
    winrt::hstring const& maxOffsetPropertyName,
    winrt::hstring const& offsetPropertyName,
    winrt::hstring const& multiplierPropertyName)
{
    // Unused because InteractionVisual returns null.
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

void ScrollBarController::SetScrollMode(
    winrt::ScrollMode const& scrollMode)
{
    SCROLLVIEWER_TRACE_INFO(
        nullptr,
        TRACE_MSG_METH_STR,
        METH_NAME,
        this,
        TypeLogging::ScrollModeToString(scrollMode).c_str());
    m_scrollMode = scrollMode;

    UpdateAreInteractionsAllowed();
}

void ScrollBarController::SetValues(
    double minOffset,
    double maxOffset,
    double offset,
    double viewport)
{
    SCROLLVIEWER_TRACE_INFO(
        nullptr,
        L"%s[0x%p](minOffset:%lf, maxOffset:%lf, offset:%lf, viewport:%lf, operationsCount:%d)\n",
        METH_NAME,
        this,
        minOffset,
        maxOffset,
        offset,
        viewport,
        m_operationsCount);

    if (maxOffset < minOffset)
    {
        throw winrt::hresult_invalid_argument(L"maxOffset cannot be smaller than minOffset.");
    }

    if (viewport < 0.0)
    {
        throw winrt::hresult_invalid_argument(L"viewport cannot be negative.");
    }

    offset = max(minOffset, offset);
    offset = min(maxOffset, offset);
    m_lastOffset = offset;

    MUX_ASSERT(m_scrollBar);

    if (minOffset < m_scrollBar.Minimum())
    {
        m_scrollBar.Minimum(minOffset);
    }

    if (maxOffset > m_scrollBar.Maximum())
    {
        m_scrollBar.Maximum(maxOffset);
    }

    if (minOffset != m_scrollBar.Minimum())
    {
        m_scrollBar.Minimum(minOffset);
    }

    if (maxOffset != m_scrollBar.Maximum())
    {
        m_scrollBar.Maximum(maxOffset);
    }

    m_scrollBar.ViewportSize(viewport);
    m_scrollBar.LargeChange(viewport);
    m_scrollBar.SmallChange(max(1.0, viewport / s_defaultViewportToSmallChangeRatio));
 
    // The ScrollBar Value is only updated when there is no operation in progress.
    if (m_operationsCount == 0 || m_scrollBar.Value() < minOffset || m_scrollBar.Value() > maxOffset)
    {
        m_scrollBar.Value(offset);
        m_lastScrollBarValue = offset;
    }

    // Potentially changed ScrollBar.Minimum / ScrollBar.Maximum value(s) may have an effect
    // on the read-only IScrollController.AreInteractionsAllowed property.
    UpdateAreInteractionsAllowed();
}

winrt::CompositionAnimation ScrollBarController::GetScrollAnimation(
    winrt::ScrollInfo info,
    winrt::float2 const& currentPosition,
    winrt::CompositionAnimation const& defaultAnimation)
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, info.OffsetsChangeId);

    // Using the consumer's default animation.
    return nullptr;
}

void ScrollBarController::OnScrollCompleted(
    winrt::ScrollInfo info)
{
    SCROLLVIEWER_TRACE_INFO(
        nullptr,
        TRACE_MSG_METH_INT,
        METH_NAME,
        this,
        info.OffsetsChangeId);

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
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    return m_scrollToRequested.add(value);
}

void ScrollBarController::ScrollToRequested(winrt::event_token const& token)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_scrollToRequested.remove(token);
}

winrt::event_token ScrollBarController::ScrollByRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollByRequestedEventArgs> const& value)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    return m_scrollByRequested.add(value);
}

void ScrollBarController::ScrollByRequested(winrt::event_token const& token)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_scrollByRequested.remove(token);
}

winrt::event_token ScrollBarController::ScrollFromRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerScrollFromRequestedEventArgs> const& value)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    return m_scrollFromRequested.add(value);
}

void ScrollBarController::ScrollFromRequested(winrt::event_token const& token)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_scrollFromRequested.remove(token);
}

winrt::event_token ScrollBarController::InteractionRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerInteractionRequestedEventArgs> const& value)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    // Because this IScrollController implementation does not expose an InteractionVisual, 
    // this InteractionRequested event is not going to be raised.
    return {};
}

void ScrollBarController::InteractionRequested(winrt::event_token const& token)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    // Because this IScrollController implementation does not expose an InteractionVisual, 
    // this InteractionRequested event is not going to be raised.
}

winrt::event_token ScrollBarController::InteractionInfoChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    return m_interactionInfoChanged.add(value);
}

void ScrollBarController::InteractionInfoChanged(winrt::event_token const& token)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_interactionInfoChanged.remove(token);
}

#pragma endregion

void ScrollBarController::HookScrollBarPropertyChanged()
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

#ifdef _DEBUG
    MUX_ASSERT(m_scrollBarIndicatorModeChangedToken.value == 0);
    MUX_ASSERT(m_scrollBarVisibilityChangedToken.value == 0);
#endif //_DEBUG
    MUX_ASSERT(m_scrollBarIsEnabledChangedToken.value == 0);

    if (m_scrollBar)
    {
#ifdef _DEBUG
        m_scrollBarIndicatorModeChangedToken.value = m_scrollBar.RegisterPropertyChangedCallback(
            winrt::ScrollBar::IndicatorModeProperty(), { this, &ScrollBarController::OnScrollBarPropertyChanged });

        m_scrollBarVisibilityChangedToken.value = m_scrollBar.RegisterPropertyChangedCallback(
            winrt::UIElement::VisibilityProperty(), { this, &ScrollBarController::OnScrollBarPropertyChanged });
#endif //_DEBUG

        m_scrollBarIsEnabledChangedToken.value = m_scrollBar.RegisterPropertyChangedCallback(
            winrt::Control::IsEnabledProperty(), { this, &ScrollBarController::OnScrollBarPropertyChanged });
    }
}

void ScrollBarController::UnhookScrollBarPropertyChanged()
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

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

        if (m_scrollBarIsEnabledChangedToken.value != 0)
        {
            m_scrollBar.UnregisterPropertyChangedCallback(winrt::Control::IsEnabledProperty(), m_scrollBarIsEnabledChangedToken.value);
            m_scrollBarIsEnabledChangedToken.value = 0;
        }
    }
}

void ScrollBarController::UpdateAreInteractionsAllowed()
{
    const bool oldAreInteractionsAllowed = m_areInteractionsAllowed;

    m_areInteractionsAllowed =
        m_scrollBar &&
        m_scrollBar.IsEnabled() &&
        m_scrollBar.Maximum() > m_scrollBar.Minimum() &&
        m_scrollMode != winrt::ScrollMode::Disabled;

    if (oldAreInteractionsAllowed != m_areInteractionsAllowed)
    {
        RaiseInteractionInfoChanged();
    }
}

void ScrollBarController::HookScrollBarEvent()
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_scrollBarScrollToken.value == 0);

    if (m_scrollBar)
    {
        m_scrollBarScrollToken = m_scrollBar.Scroll({ this, &ScrollBarController::OnScroll });
    }
}

void ScrollBarController::UnhookScrollBarEvent()
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (m_scrollBar && m_scrollBarScrollToken.value != 0)
    {
        m_scrollBar.Scroll(m_scrollBarScrollToken);
        m_scrollBarScrollToken.value = 0;
    }
}

void ScrollBarController::OnScrollBarPropertyChanged(
    const winrt::DependencyObject& /*sender*/,
    const winrt::DependencyProperty& args)
{
    MUX_ASSERT(m_scrollBar);

    if (args == winrt::Control::IsEnabledProperty())
    {
        SCROLLVIEWER_TRACE_VERBOSE(
            nullptr,
            TRACE_MSG_METH_STR_INT,
            METH_NAME,
            this,
            L"IsEnabled",
            m_scrollBar.IsEnabled());

        // Potentially changed ScrollBar.Minimum / ScrollBar.Maximum value(s) may have an effect
        // on the read-only IScrollController.AreInteractionsAllowed property.
        UpdateAreInteractionsAllowed();
    }
#ifdef _DEBUG
    else if (args == winrt::UIElement::VisibilityProperty())
    {
        SCROLLVIEWER_TRACE_VERBOSE(
            nullptr,
            TRACE_MSG_METH_STR_INT,
            METH_NAME,
            this,
            L"Visibility",
            m_scrollBar.Visibility());
    }
    else if (args == winrt::ScrollBar::IndicatorModeProperty())
    {
        SCROLLVIEWER_TRACE_VERBOSE(
            nullptr,
            TRACE_MSG_METH_STR_STR,
            METH_NAME,
            this,
            L"IndicatorMode",
            TypeLogging::ScrollingIndicatorModeToString(m_scrollBar.IndicatorMode()).c_str());
    }
#endif //_DEBUG
}

void ScrollBarController::OnScroll(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollEventArgs& args)
{
    const auto scrollEventType = args.ScrollEventType();

    SCROLLVIEWER_TRACE_VERBOSE(
        nullptr,
        TRACE_MSG_METH_STR,
        METH_NAME,
        this,
        TypeLogging::ScrollEventTypeToString(scrollEventType).c_str());

    if (!m_scrollBar)
    {
        return;
    }

    if (m_scrollMode == winrt::ScrollMode::Disabled && scrollEventType != winrt::ScrollEventType::ThumbPosition)
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
        m_areScrollerInteractionsAllowed = true;

        if (m_isInteracting)
        {
            m_isInteracting = false;
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
            m_areScrollerInteractionsAllowed = false;

            if (!m_isInteracting)
            {
                m_isInteracting = true;
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
                offsetChangeRequested = RaiseScrollFromRequested(offsetChange);
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
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, offset);

    if (!m_scrollToRequested)
    {
        return false;
    }

    auto options = winrt::make_self<ScrollOptions>(
        winrt::AnimationMode::Disabled,
        winrt::SnapPointsMode::Ignore);

    auto scrollToRequestedEventArgs = winrt::make_self<ScrollControllerScrollToRequestedEventArgs>(
        offset,
        *options);

    m_scrollToRequested(*this, *scrollToRequestedEventArgs);

    int32_t offsetChangeId = scrollToRequestedEventArgs.as<winrt::ScrollControllerScrollToRequestedEventArgs>().Info().OffsetsChangeId;

    // Only increment m_operationsCount when the returned OffsetsChangeId represents a new request that was not coalesced with a pending request. 
    if (offsetChangeId != -1 && offsetChangeId != m_lastOffsetChangeIdForScrollTo)
    {
        m_lastOffsetChangeIdForScrollTo = offsetChangeId;
        m_operationsCount++;
        return true;
    }

    return false;
}

bool ScrollBarController::RaiseScrollByRequested(
    double offsetChange)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, offsetChange);

    if (!m_scrollByRequested)
    {
        return false;
    }

    auto options = winrt::make_self<ScrollOptions>(
        winrt::AnimationMode::Disabled,
        winrt::SnapPointsMode::Ignore);

    auto scrollByRequestedEventArgs = winrt::make_self<ScrollControllerScrollByRequestedEventArgs>(
        offsetChange,
        *options);

    m_scrollByRequested(*this, *scrollByRequestedEventArgs);

    int32_t offsetChangeId = scrollByRequestedEventArgs.as<winrt::ScrollControllerScrollByRequestedEventArgs>().Info().OffsetsChangeId;

    // Only increment m_operationsCount when the returned OffsetsChangeId represents a new request that was not coalesced with a pending request. 
    if (offsetChangeId != -1 && offsetChangeId != m_lastOffsetChangeIdForScrollBy)
    {
        m_lastOffsetChangeIdForScrollBy = offsetChangeId;
        m_operationsCount++;
        return true;
    }

    return false;
}

bool ScrollBarController::RaiseScrollFromRequested(
    double offsetChange)
{
    if (!m_scrollFromRequested)
    {
        return false;
    }

    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, offsetChange);

    double offsetVelocity = m_operationsCount == 0 ? s_minimumVelocity : 0.0;

    if (offsetChange < 0.0)
    {
        offsetVelocity *= -1;
    }
    offsetVelocity += offsetChange * s_velocityNeededPerPixel;

    winrt::IInspectable inertiaDecayRateAsInsp = box_value(s_inertiaDecayRate);
    winrt::IReference<float> inertiaDecayRate = inertiaDecayRateAsInsp.as<winrt::IReference<float>>();

    auto scrollFromRequestedEventArgs = winrt::make_self<ScrollControllerScrollFromRequestedEventArgs>(
        static_cast<float>(offsetVelocity),
        inertiaDecayRate);

    m_scrollFromRequested(*this, *scrollFromRequestedEventArgs);

    int32_t offsetChangeId = scrollFromRequestedEventArgs.as<winrt::ScrollControllerScrollFromRequestedEventArgs>().Info().OffsetsChangeId;

    // Only increment m_operationsCount when the returned OffsetsChangeId represents a new request that was not coalesced with a pending request. 
    if (offsetChangeId != -1 && offsetChangeId != m_lastOffsetChangeIdForScrollFrom)
    {
        m_lastOffsetChangeIdForScrollFrom = offsetChangeId;
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

    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_interactionInfoChanged(*this, nullptr);
}

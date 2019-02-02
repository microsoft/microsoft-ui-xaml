﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollBarController.h"
#include "ScrollViewer.h"
#include "TypeLogging.h"
#include "ScrollControllerOffsetChangeRequestedEventArgs.h"
#include "ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs.h"

ScrollBarController::ScrollBarController(ScrollViewer* owner)
    : m_scrollBar(owner)
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    MUX_ASSERT(owner);
}

ScrollBarController::~ScrollBarController()
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollBarEvent();
#ifdef _DEBUG
    UnhookScrollBarPropertyChanged();
#endif //_DEBUG
}

void ScrollBarController::SetScrollBar(const winrt::ScrollBar& scrollBar)
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollBarEvent();

    m_scrollBar.set(scrollBar);

    HookScrollBarEvent();
#ifdef _DEBUG
    HookScrollBarPropertyChanged();
#endif //_DEBUG
}

#pragma region IScrollController

bool ScrollBarController::AreInteractionsEnabled()
{
    return m_areInteractionsEnabled;
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
    return m_scrollBar.get().Orientation();
}

void ScrollBarController::AllowInteractions(bool allowInteractions)
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, allowInteractions);
    m_areInteractionsAllowed = allowInteractions;

    UpdateAreInteractionsEnabled();
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

    UpdateAreInteractionsEnabled();
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

    winrt::ScrollBar scrollBar = m_scrollBar.get();

    if (minOffset < scrollBar.Minimum())
    {
        scrollBar.Minimum(minOffset);
    }

    if (maxOffset > scrollBar.Maximum())
    {
        scrollBar.Maximum(maxOffset);
    }

    if (minOffset != scrollBar.Minimum())
    {
        scrollBar.Minimum(minOffset);
    }

    if (maxOffset != scrollBar.Maximum())
    {
        scrollBar.Maximum(maxOffset);
    }

    scrollBar.ViewportSize(viewport);
    scrollBar.LargeChange(viewport);
    scrollBar.SmallChange(max(1.0, viewport / s_defaultViewportToSmallChangeRatio));
 
    // The ScrollBar Value is only updated when there is no operation in progress.
    if (m_operationsCount == 0 || scrollBar.Value() < minOffset || scrollBar.Value() > maxOffset)
    {
        scrollBar.Value(offset);
        m_lastScrollBarValue = offset;
    }

    // Potentially changed ScrollBar.Minimum / ScrollBar.Maximum value(s) may have an effect
    // on the read-only IScrollController.AreInteractionsEnabled property.
    UpdateAreInteractionsEnabled();
}

winrt::CompositionAnimation ScrollBarController::GetScrollAnimation(
    INT32 offsetChangeId,
    winrt::float2 const& currentPosition,
    winrt::CompositionAnimation const& defaultAnimation)
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, offsetChangeId);

    // Using the consumer's default animation.
    return nullptr;
}

void ScrollBarController::OnScrollCompleted(
    INT32 offsetChangeId,
    winrt::ScrollerViewChangeResult const& result)
{
    SCROLLVIEWER_TRACE_INFO(
        nullptr,
        TRACE_MSG_METH_STR_INT,
        METH_NAME,
        this,
        TypeLogging::ScrollerViewChangeResultToString(result).c_str(),
        offsetChangeId);

    MUX_ASSERT(m_operationsCount > 0);
    m_operationsCount--;

    if (m_operationsCount == 0 && m_scrollBar && m_scrollBar.get().Value() != m_lastOffset)
    {
        m_scrollBar.get().Value(m_lastOffset);
        m_lastScrollBarValue = m_lastOffset;
    }
}

winrt::event_token ScrollBarController::OffsetChangeRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerOffsetChangeRequestedEventArgs> const& value)
{
    return m_offsetChangeRequested.add(value);
}

void ScrollBarController::OffsetChangeRequested(winrt::event_token const& token)
{
    m_offsetChangeRequested.remove(token);
}

winrt::event_token ScrollBarController::OffsetChangeWithAdditionalVelocityRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs> const& value)
{
    return m_offsetChangeWithAdditionalVelocityRequested.add(value);
}

void ScrollBarController::OffsetChangeWithAdditionalVelocityRequested(winrt::event_token const& token)
{
    m_offsetChangeWithAdditionalVelocityRequested.remove(token);
}

winrt::event_token ScrollBarController::InteractionRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerInteractionRequestedEventArgs> const& value)
{
    // Because this IScrollController implementation does not expose an InteractionVisual, 
    // this InteractionRequested event is not going to be raised.
    return {};
}

void ScrollBarController::InteractionRequested(winrt::event_token const& token)
{
    // Because this IScrollController implementation does not expose an InteractionVisual, 
    // this InteractionRequested event is not going to be raised.
}

winrt::event_token ScrollBarController::InteractionInfoChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value)
{
    return m_interactionInfoChanged.add(value);
}

void ScrollBarController::InteractionInfoChanged(winrt::event_token const& token)
{
    m_interactionInfoChanged.remove(token);
}

#pragma endregion

void ScrollBarController::HookScrollBarPropertyChanged()
{
#ifdef _DEBUG
    MUX_ASSERT(m_scrollBarIndicatorModeChangedToken.value == 0);
    MUX_ASSERT(m_scrollBarVisibilityChangedToken.value == 0);
#endif //_DEBUG
    MUX_ASSERT(m_scrollBarIsEnabledChangedToken.value == 0);

    winrt::ScrollBar scrollBar = m_scrollBar.get();

    if (scrollBar)
    {
#ifdef _DEBUG
        m_scrollBarIndicatorModeChangedToken.value = scrollBar.RegisterPropertyChangedCallback(
            winrt::ScrollBar::IndicatorModeProperty(), { this, &ScrollBarController::OnScrollBarPropertyChanged });

        m_scrollBarVisibilityChangedToken.value = scrollBar.RegisterPropertyChangedCallback(
            winrt::UIElement::VisibilityProperty(), { this, &ScrollBarController::OnScrollBarPropertyChanged });
#endif //_DEBUG

        m_scrollBarIsEnabledChangedToken.value = scrollBar.RegisterPropertyChangedCallback(
            winrt::Control::IsEnabledProperty(), { this, &ScrollBarController::OnScrollBarPropertyChanged });
    }
}

void ScrollBarController::UnhookScrollBarPropertyChanged()
{
    winrt::ScrollBar scrollBar = m_scrollBar.safe_get();

    if (scrollBar)
    {
#ifdef _DEBUG
        if (m_scrollBarIndicatorModeChangedToken.value != 0)
        {
            scrollBar.UnregisterPropertyChangedCallback(winrt::ScrollBar::IndicatorModeProperty(), m_scrollBarIndicatorModeChangedToken.value);
            m_scrollBarIndicatorModeChangedToken.value = 0;
        }

        if (m_scrollBarVisibilityChangedToken.value != 0)
        {
            scrollBar.UnregisterPropertyChangedCallback(winrt::UIElement::VisibilityProperty(), m_scrollBarVisibilityChangedToken.value);
            m_scrollBarVisibilityChangedToken.value = 0;
        }
#endif //_DEBUG

        if (m_scrollBarIsEnabledChangedToken.value != 0)
        {
            scrollBar.UnregisterPropertyChangedCallback(winrt::Control::IsEnabledProperty(), m_scrollBarIsEnabledChangedToken.value);
            m_scrollBarIsEnabledChangedToken.value = 0;
        }
    }
}

void ScrollBarController::UpdateAreInteractionsEnabled()
{
    bool oldAreInteractionsEnabled = m_areInteractionsEnabled;
    winrt::ScrollBar scrollBar = m_scrollBar.get();

    m_areInteractionsEnabled =
        scrollBar &&
        scrollBar.IsEnabled() &&
        scrollBar.Maximum() > scrollBar.Minimum() &&
        m_areInteractionsAllowed &&
        m_scrollMode == winrt::ScrollMode::Enabled;

    if (oldAreInteractionsEnabled != m_areInteractionsEnabled)
    {
        RaiseInteractionInfoChanged();
    }
}

void ScrollBarController::HookScrollBarEvent()
{
    MUX_ASSERT(m_scrollBarScrollToken.value == 0);

    winrt::ScrollBar scrollBar = m_scrollBar.get();

    if (scrollBar)
    {
        m_scrollBarScrollToken = scrollBar.Scroll({ this, &ScrollBarController::OnScroll });
    }
}

void ScrollBarController::UnhookScrollBarEvent()
{
    winrt::ScrollBar scrollBar = m_scrollBar.safe_get();

    if (scrollBar && m_scrollBarScrollToken.value != 0)
    {
        scrollBar.Scroll(m_scrollBarScrollToken);
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
            m_scrollBar.get().IsEnabled());

        // Potentially changed ScrollBar.Minimum / ScrollBar.Maximum value(s) may have an effect
        // on the read-only IScrollController.AreInteractionsEnabled property.
        UpdateAreInteractionsEnabled();
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
            m_scrollBar.get().Visibility());
    }
    else if (args == winrt::ScrollBar::IndicatorModeProperty())
    {
        SCROLLVIEWER_TRACE_VERBOSE(
            nullptr,
            TRACE_MSG_METH_STR_STR,
            METH_NAME,
            this,
            L"IndicatorMode",
            TypeLogging::ScrollingIndicatorModeToString(m_scrollBar.get().IndicatorMode()).c_str());
    }
#endif //_DEBUG
}

void ScrollBarController::OnScroll(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollEventArgs& args)
{
    winrt::ScrollEventType scrollEventType = args.ScrollEventType();

    SCROLLVIEWER_TRACE_VERBOSE(
        nullptr,
        TRACE_MSG_METH_STR,
        METH_NAME,
        this,
        TypeLogging::ScrollEventTypeToString(scrollEventType).c_str());

    winrt::ScrollBar scrollBar = m_scrollBar.get();

    if (!scrollBar)
    {
        return;
    }

    if (!m_areInteractionsAllowed && m_scrollMode == winrt::ScrollMode::Disabled && scrollEventType != winrt::ScrollEventType::ThumbPosition)
    {
        // This ScrollBar is not interactive. Restore its previous Value.
        scrollBar.Value(m_lastScrollBarValue);
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
            offsetChangeRequested = RaiseOffsetChangeRequested(args.NewValue());
        }
        else
        {
            double offsetChange = 0.0;

            switch (scrollEventType)
            {
            case winrt::ScrollEventType::LargeDecrement:
                offsetChange = -min(m_lastScrollBarValue - scrollBar.Minimum(), scrollBar.LargeChange());
                break;
            case winrt::ScrollEventType::LargeIncrement:
                offsetChange = min(scrollBar.Maximum() - m_lastScrollBarValue, scrollBar.LargeChange());
                break;
            case winrt::ScrollEventType::SmallDecrement:
                offsetChange = -min(m_lastScrollBarValue - scrollBar.Minimum(), scrollBar.SmallChange());
                break;
            case winrt::ScrollEventType::SmallIncrement:
                offsetChange = min(scrollBar.Maximum() - m_lastScrollBarValue, scrollBar.SmallChange());
                break;
            }

            // When the requested Value is near the Mininum or Maximum, include a little additional velocity
            // to ensure the extreme value is reached.
            if (args.NewValue() - scrollBar.Minimum() < s_minMaxEpsilon)
            {
                MUX_ASSERT(offsetChange < 0.0);
                offsetChange -= s_minMaxEpsilon;
            }
            else if (scrollBar.Maximum() - args.NewValue() < s_minMaxEpsilon)
            {
                MUX_ASSERT(offsetChange > 0.0);
                offsetChange += s_minMaxEpsilon;
            }

            offsetChangeRequested = RaiseOffsetChangeWithAdditionalVelocityRequested(offsetChange);
        }

        if (!offsetChangeRequested)
        {
            // This request could not be requested, restore the previous Value.
            scrollBar.Value(m_lastScrollBarValue);
        }
        break;
    }
    }

    m_lastScrollBarValue = scrollBar.Value();
}

bool ScrollBarController::RaiseOffsetChangeRequested(
    double offset)
{
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, offset);

    if (!m_offsetChangeRequested)
    {
        return false;
    }

    auto offsetChangeRequestedEventArgs = winrt::make_self<ScrollControllerOffsetChangeRequestedEventArgs>(
        offset,
        winrt::ScrollerViewKind::Absolute,
        winrt::ScrollerViewChangeKind::DisableAnimation);

    m_offsetChangeRequested(*this, *offsetChangeRequestedEventArgs);

    int32_t viewChangeId = offsetChangeRequestedEventArgs.as<winrt::ScrollControllerOffsetChangeRequestedEventArgs>().ViewChangeId();

    // Only increment m_operationsCount when the returned ViewChangeId represents a new request that was not coalesced with a pending request. 
    if (viewChangeId != -1 && viewChangeId != m_lastViewChangeIdForOffsetChange)
    {
        m_lastViewChangeIdForOffsetChange = viewChangeId;
        m_operationsCount++;
        return true;
    }

    return false;
}

bool ScrollBarController::RaiseOffsetChangeWithAdditionalVelocityRequested(
    double offsetChange)
{
    if (!m_offsetChangeWithAdditionalVelocityRequested)
    {
        return false;
    }

    SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, offsetChange);

    double additionalVelocity = m_operationsCount == 0 ? s_minimumVelocity : 0.0;

    if (offsetChange < 0.0)
    {
        additionalVelocity *= -1;
    }
    additionalVelocity += offsetChange * s_velocityNeededPerPixel;

    winrt::IInspectable inertiaDecayRateAsInsp = box_value(s_inertiaDecayRate);
    winrt::IReference<float> inertiaDecayRate = inertiaDecayRateAsInsp.as<winrt::IReference<float>>();

    auto offsetChangeWithAdditionalVelocityRequestedEventArgs = winrt::make_self<ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs>(
        static_cast<float>(additionalVelocity),
        inertiaDecayRate);

    m_offsetChangeWithAdditionalVelocityRequested(*this, *offsetChangeWithAdditionalVelocityRequestedEventArgs);

    int32_t viewChangeId = offsetChangeWithAdditionalVelocityRequestedEventArgs.as<winrt::ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs>().ViewChangeId();

    // Only increment m_operationsCount when the returned ViewChangeId represents a new request that was not coalesced with a pending request. 
    if (viewChangeId != -1 && viewChangeId != m_lastViewChangeIdForOffsetChangeWithAdditionalVelocity)
    {
        m_lastViewChangeIdForOffsetChangeWithAdditionalVelocity = viewChangeId;
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

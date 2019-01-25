// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "DoubleUtil.h"
#include "TypeLogging.h"
#include "ResourceAccessor.h"
#include "RuntimeProfiler.h"
#include "ScrollBar2.h"
#include "ScrollControllerOffsetChangeRequestedEventArgs.h"
#include "ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs.h"

using namespace std;

// Change to 'true' to turn on debugging outputs in Output window
bool ScrollBar2Trace::s_IsDebugOutputEnabled{ false };
bool ScrollBar2Trace::s_IsVerboseDebugOutputEnabled{ false };

ScrollBar2::~ScrollBar2()
{
    SCROLLBAR2_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollBarEvent();
    UnhookPropertyChanged();
#ifdef _DEBUG
    UnhookScrollBarPropertyChanged();
#endif //_DEBUG
}

ScrollBar2::ScrollBar2()
{
    SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);


    EnsureProperties();
    CreateAndInitializeScrollBar();
    HookScrollBarEvent();
    HookPropertyChanged();
#ifdef _DEBUG
    HookScrollBarPropertyChanged();
#endif //_DEBUG
}

#pragma region IScrollController

bool ScrollBar2::AreScrollerInteractionsAllowed()
{
    return m_areScrollerInteractionsAllowed;
}

bool ScrollBar2::IsInteracting()
{
    return m_isInteracting;
}

winrt::Visual ScrollBar2::InteractionVisual()
{
    // This IScrollController implementation has no touch-manipulatable element.
    return nullptr;
}

winrt::Orientation ScrollBar2::InteractionVisualScrollOrientation()
{
    // Unused because InteractionVisual returns null.
    return Orientation();
}

winrt::RailingMode ScrollBar2::InteractionVisualScrollRailingMode()
{
    // Unused because InteractionVisual returns null.
    return winrt::RailingMode::Enabled;
}

void ScrollBar2::SetExpressionAnimationSources(
    winrt::CompositionPropertySet const& propertySet,
    _In_ winrt::hstring const& minOffsetPropertyName,
    _In_ winrt::hstring const& maxOffsetPropertyName,
    _In_ winrt::hstring const& offsetPropertyName,
    _In_ winrt::hstring const& multiplierPropertyName)
{
    SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    // Unused because InteractionVisual returns null.
}

void ScrollBar2::SetValues(
    _In_ double minOffset,
    _In_ double maxOffset,
    _In_ double offset,
    _In_ double viewport)
{
    SCROLLBAR2_TRACE_INFO(*this, L"%s[0x%p](minOffset:%lf, maxOffset:%lf, offset:%lf, viewport:%lf, operationsCount:%d)\n", METH_NAME, this,
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

    SetValue(s_ViewportProperty, box_value(viewport));

    if (minOffset < unbox_value<double>(GetValue(s_MinOffsetProperty)))
    {
        SetValue(s_MinOffsetProperty, box_value(minOffset));
    }

    if (maxOffset < unbox_value<double>(GetValue(s_MaxOffsetProperty)))
    {
        SetValue(s_MaxOffsetProperty, box_value(maxOffset));
    }

    if (minOffset != unbox_value<double>(GetValue(s_MinOffsetProperty)))
    {
        SetValue(s_MinOffsetProperty, box_value(minOffset));
    }

    if (maxOffset != unbox_value<double>(GetValue(s_MaxOffsetProperty)))
    {
        SetValue(s_MaxOffsetProperty, box_value(maxOffset));
    }

    if (m_scrollBar)
    {
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

        if (isnan(unbox_value<double>(GetValue(s_LargeChangeProperty))))
        {
            scrollBar.LargeChange(viewport);
        }

        if (isnan(unbox_value<double>(GetValue(s_SmallChangeProperty))))
        {
            scrollBar.SmallChange(max(1.0, viewport / s_defaultViewportToSmallChangeRatio));
        }

        // The ScrollBar Value is only updated when there is no operation in progress.
        if (m_operationsCount == 0 || scrollBar.Value() < minOffset || scrollBar.Value() > maxOffset)
        {
            SetValue(s_OffsetProperty, box_value(offset));
            scrollBar.Value(offset);
            m_lastScrollBarValue = offset;
        }
    }
}

winrt::CompositionAnimation ScrollBar2::GetOffsetChangeAnimation(
    _In_ INT32 offsetChangeId,
    winrt::float2 const& currentPosition,
    winrt::CompositionAnimation const& defaultAnimation)
{
    SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, offsetChangeId);

    // Using the consumer's default animation.
    return nullptr;
}

void ScrollBar2::OnOffsetChangeCompleted(
    _In_ INT32 offsetChangeId,
    winrt::ScrollerViewChangeResult const& result)
{
    SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, 
        TypeLogging::ScrollerViewChangeResultToString(result).c_str(), offsetChangeId);

    MUX_ASSERT(m_operationsCount > 0);
    m_operationsCount--;

    if (m_operationsCount == 0 && m_scrollBar && m_scrollBar.get().Value() != m_lastOffset)
    {
        m_scrollBar.get().Value(m_lastOffset);
        m_lastScrollBarValue = m_lastOffset;
    }
}

winrt::event_token ScrollBar2::OffsetChangeRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerOffsetChangeRequestedEventArgs> const& value)
{
    return m_offsetChangeRequested.add(value);
}

void ScrollBar2::OffsetChangeRequested(winrt::event_token const& token)
{
    m_offsetChangeRequested.remove(token);
}

winrt::event_token ScrollBar2::OffsetChangeWithAdditionalVelocityRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs> const& value)
{
    return m_offsetChangeWithAdditionalVelocityRequested.add(value);
}

void ScrollBar2::OffsetChangeWithAdditionalVelocityRequested(winrt::event_token const& token)
{
    m_offsetChangeWithAdditionalVelocityRequested.remove(token);
}

winrt::event_token ScrollBar2::InteractionRequested(winrt::TypedEventHandler<winrt::IScrollController, winrt::ScrollControllerInteractionRequestedEventArgs> const& value)
{
    // Because this IScrollController implementation does not expose an InteractionVisual, 
    // this InteractionRequested event is not going to be raised.
    return {};
}

void ScrollBar2::InteractionRequested(winrt::event_token const& token)
{
    // Because this IScrollController implementation does not expose an InteractionVisual, 
    // this InteractionRequested event is not going to be raised.
}

winrt::event_token ScrollBar2::InteractionInfoChanged(winrt::TypedEventHandler<winrt::IScrollController, winrt::IInspectable> const& value)
{
    return m_interactionInfoChanged.add(value);
}

void ScrollBar2::InteractionInfoChanged(winrt::event_token const& token)
{
    m_interactionInfoChanged.remove(token);
}

#pragma endregion

#pragma region IFrameworkElementOverridesHelper

winrt::Size ScrollBar2::MeasureOverride(winrt::Size const& availableSize)
{
    SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, availableSize.Width, availableSize.Height);

    winrt::Size childDesiredSize{ 0.0f, 0.0f };

    if (m_scrollBar)
    {
        m_scrollBar.get().Measure(availableSize);
        childDesiredSize = m_scrollBar.get().DesiredSize();
    }
    
    return childDesiredSize;
}

winrt::Size ScrollBar2::ArrangeOverride(winrt::Size const& finalSize)
{
    SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, finalSize.Width, finalSize.Height);

    if (m_scrollBar)
    {
        winrt::Rect finalRect{ 0, 0, finalSize.Width, finalSize.Height };

        m_scrollBar.get().Arrange(finalRect);
    }

    return finalSize;
}

#pragma endregion

// Invoked when a dependency property of this ScrollBar2 has changed.
void ScrollBar2::OnPropertyChanged(
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    const auto dependencyProperty = args.Property();

    if (dependencyProperty == s_SmallChangeProperty)
    {
        SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_SmallChangePropertyName);

        if (m_scrollBar)
        {
            if (isnan(unbox_value<double>(args.NewValue())))
            {
                m_scrollBar.get().SmallChange(max(1.0, unbox_value<double>(GetValue(s_ViewportProperty)) / s_defaultViewportToSmallChangeRatio));
            }
            else
            {
                m_scrollBar.get().SmallChange(unbox_value<double>(args.NewValue()));
            }
        }
    }
    else if (dependencyProperty == s_LargeChangeProperty)
    {
        SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_LargeChangePropertyName);

        if (m_scrollBar)
        {
            if (isnan(unbox_value<double>(args.NewValue())))
            {
                m_scrollBar.get().LargeChange(unbox_value<double>(GetValue(s_ViewportProperty)));
            }
            else
            {
                m_scrollBar.get().LargeChange(unbox_value<double>(args.NewValue()));
            }
        }
    }
    else if (dependencyProperty == s_IndicatorModeProperty)
    {
        SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_IndicatorModePropertyName);

        if (m_scrollBar)
        {
            m_scrollBar.get().IndicatorMode(unbox_value<winrt::ScrollingIndicatorMode>(args.NewValue()));
        }
    }
    else if (dependencyProperty == s_IsEnabledProperty)
    {
        SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_IsEnabledPropertyName);

        if (m_scrollBar)
        {
            m_scrollBar.get().IsEnabled(unbox_value<bool>(args.NewValue()));
        }
    }
    else if (dependencyProperty == s_OrientationProperty)
    {
        SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_OrientationPropertyName);

        if (m_scrollBar)
        {
            m_scrollBar.get().Orientation(unbox_value<winrt::Orientation>(args.NewValue()));
        }
    }
    else if (dependencyProperty == s_ScrollBarStyleProperty)
    {
        SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_ScrollBarStylePropertyName);

        if (m_scrollBar)
        {
            m_scrollBar.get().Style(safe_cast<winrt::Style>(args.NewValue()));
        }
    }
    else if (dependencyProperty == s_ScrollModeProperty)
    {
        SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_ScrollModePropertyName);
        
#ifdef USE_SCROLLMODE_AUTO
        MUX_ASSERT(unbox_value<winrt::ScrollMode>(args.OldValue()) != winrt::ScrollMode::Auto);
#endif
    }
#ifdef _DEBUG
    else
    {
        SCROLLBAR2_TRACE_VERBOSE(nullptr, L"%s(property: %s)\n", METH_NAME, DependencyPropertyToString(dependencyProperty).c_str());
    }
#endif
}

void ScrollBar2::ValidateScrollMode(winrt::ScrollMode mode)
{
#ifdef USE_SCROLLMODE_AUTO
    if (mode == winrt::ScrollMode::Auto)
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }
#endif
}

void ScrollBar2::CreateAndInitializeScrollBar()
{
    winrt::ScrollBar scrollBar;

    scrollBar.IsEnabled(s_defaultIsEnabled);
    scrollBar.Orientation(s_defaultOrientation);

    auto children = Children();
    children.Append(scrollBar);

    m_scrollBar.set(scrollBar);
}

#ifdef _DEBUG
void ScrollBar2::HookScrollBarPropertyChanged()
{
    MUX_ASSERT(m_scrollBarIndicatorModeChangedToken.value == 0);
    MUX_ASSERT(m_scrollBarVisibilityChangedToken.value == 0);

    winrt::ScrollBar scrollBar = m_scrollBar.get();

    if (scrollBar)
    {
        m_scrollBarIndicatorModeChangedToken.value = scrollBar.RegisterPropertyChangedCallback(
            winrt::ScrollBar::IndicatorModeProperty(), { this, &ScrollBar2::OnScrollBarPropertyChanged });

        m_scrollBarVisibilityChangedToken.value = scrollBar.RegisterPropertyChangedCallback(
            winrt::UIElement::VisibilityProperty(), { this, &ScrollBar2::OnScrollBarPropertyChanged });
    }
}

void ScrollBar2::UnhookScrollBarPropertyChanged()
{
    winrt::ScrollBar scrollBar = m_scrollBar.safe_get();

    if (scrollBar)
    {
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
    }
}
#endif //_DEBUG

void ScrollBar2::HookPropertyChanged()
{
    MUX_ASSERT(m_visibilityChangedToken.value == 0);

    m_visibilityChangedToken.value = RegisterPropertyChangedCallback(
        winrt::UIElement::VisibilityProperty(), { this, &ScrollBar2::OnScrollBar2PropertyChanged });
}

void ScrollBar2::UnhookPropertyChanged()
{
    if (m_visibilityChangedToken.value != 0)
    {
        UnregisterPropertyChangedCallback(winrt::UIElement::VisibilityProperty(), m_visibilityChangedToken.value);
        m_visibilityChangedToken.value = 0;
    }
}

void ScrollBar2::HookScrollBarEvent()
{
    MUX_ASSERT(m_scrollBarScrollToken.value == 0);

    m_scrollBarScrollToken = m_scrollBar.get().Scroll({ this, &ScrollBar2::OnScroll });
}

void ScrollBar2::UnhookScrollBarEvent()
{
    winrt::ScrollBar scrollBar = m_scrollBar.safe_get();

    if (scrollBar && m_scrollBarScrollToken.value != 0)
    {
        scrollBar.Scroll(m_scrollBarScrollToken);
        m_scrollBarScrollToken.value = 0;
    }
}

#ifdef _DEBUG
void ScrollBar2::OnScrollBarPropertyChanged(
    const winrt::DependencyObject& /*sender*/,
    const winrt::DependencyProperty& args)
{
    if (args == winrt::UIElement::VisibilityProperty())
    {
        SCROLLBAR2_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Visibility");
    }
    else if (args == winrt::ScrollBar::IndicatorModeProperty())
    {
        SCROLLBAR2_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this,
            hstring{ ScrollBar2::s_IndicatorModePropertyName }.c_str(), TypeLogging::ScrollingIndicatorModeToString(m_scrollBar.get().IndicatorMode()).c_str());
    }
}
#endif //_DEBUG

void ScrollBar2::OnScrollBar2PropertyChanged(
    const winrt::DependencyObject& /*sender*/,
    const winrt::DependencyProperty& args)
{
    SCROLLBAR2_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (args == winrt::UIElement::VisibilityProperty() && m_scrollBar)
    {
        m_scrollBar.get().Visibility(Visibility());
    }
}

void ScrollBar2::OnScroll(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollEventArgs& args)
{
    winrt::ScrollEventType scrollEventType = args.ScrollEventType();

    SCROLLBAR2_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollEventTypeToString(scrollEventType).c_str());

    winrt::ScrollBar scrollBar = m_scrollBar.get();

    if (!scrollBar)
    {
        return;
    }

    if (ScrollMode() == winrt::ScrollMode::Disabled &&
        scrollEventType != winrt::ScrollEventType::ThumbPosition)
    {
        // This ScrollBar2 is not interactive. Restore its previous Value.
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
    SetValue(s_OffsetProperty, box_value(m_lastScrollBarValue));
}

bool ScrollBar2::RaiseOffsetChangeRequested(
    double offset)
{
    SCROLLBAR2_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offset);

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

bool ScrollBar2::RaiseOffsetChangeWithAdditionalVelocityRequested(
    double offsetChange)
{
    if (!m_offsetChangeWithAdditionalVelocityRequested)
    {
        return false;
    }

    SCROLLBAR2_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL, METH_NAME, this, offsetChange);

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

void ScrollBar2::RaiseInteractionInfoChanged()
{
    if (!m_interactionInfoChanged)
    {
        return;
    }

    SCROLLBAR2_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_interactionInfoChanged(*this, nullptr);
}

#ifdef _DEBUG

winrt::hstring ScrollBar2::DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty)
{
    if (dependencyProperty == s_MinOffsetProperty)
    {
        return L"MinOffset";
    }
    else if (dependencyProperty == s_MaxOffsetProperty)
    {
        return L"MaxOffset";
    }
    else if (dependencyProperty == s_OffsetProperty)
    {
        return L"Offset";
    }
    else if (dependencyProperty == s_ViewportProperty)
    {
        return L"Viewport";
    }
    else if (dependencyProperty == s_SmallChangeProperty)
    {
        return hstring{ ScrollBar2::s_SmallChangePropertyName };
    }
    else if (dependencyProperty == s_LargeChangeProperty)
    {
        return hstring{ ScrollBar2::s_LargeChangePropertyName };
    }
    else if (dependencyProperty == s_IndicatorModeProperty)
    {
        return hstring{ ScrollBar2::s_IndicatorModePropertyName };
    }
    else if (dependencyProperty == s_IsEnabledProperty)
    {
        return hstring{ ScrollBar2::s_IsEnabledPropertyName };
    }
    else if (dependencyProperty == s_OrientationProperty)
    {
        return hstring{ ScrollBar2::s_OrientationPropertyName };
    }
    else if (dependencyProperty == s_ScrollBarStyleProperty)
    {
        return hstring{ ScrollBar2::s_ScrollBarStylePropertyName };
    }
    else if (dependencyProperty == s_ScrollModeProperty)
    {
        return hstring{ ScrollBar2::s_ScrollModePropertyName };
    }
    else
    {
        return L"UNKNOWN";
    }
}

#endif

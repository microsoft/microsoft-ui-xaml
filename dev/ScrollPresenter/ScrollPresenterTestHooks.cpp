// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollPresenterTestHooksFactory.h"
#include "Vector.h"

com_ptr<ScrollPresenterTestHooks> ScrollPresenterTestHooks::s_testHooks{};

ScrollPresenterTestHooks::ScrollPresenterTestHooks()
{
    m_mouseWheelInertiaDecayRate = SharedHelpers::IsRS2OrHigher() ? ScrollPresenter::s_mouseWheelInertiaDecayRate : ScrollPresenter::s_mouseWheelInertiaDecayRateRS1;
}

com_ptr<ScrollPresenterTestHooks> ScrollPresenterTestHooks::EnsureGlobalTestHooks() 
{
    static bool s_initialized = []() {
        s_testHooks = winrt::make_self<ScrollPresenterTestHooks>();
        return true;
    }();
    return s_testHooks;
}

bool ScrollPresenterTestHooks::AreAnchorNotificationsRaised()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_areAnchorNotificationsRaised;
}

void ScrollPresenterTestHooks::AreAnchorNotificationsRaised(bool areAnchorNotificationsRaised)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_areAnchorNotificationsRaised = areAnchorNotificationsRaised;
}

bool ScrollPresenterTestHooks::AreInteractionSourcesNotificationsRaised()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_areInteractionSourcesNotificationsRaised;
}

void ScrollPresenterTestHooks::AreInteractionSourcesNotificationsRaised(bool areInteractionSourcesNotificationsRaised)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_areInteractionSourcesNotificationsRaised = areInteractionSourcesNotificationsRaised;
}

bool ScrollPresenterTestHooks::AreExpressionAnimationStatusNotificationsRaised()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_areExpressionAnimationStatusNotificationsRaised;
}

void ScrollPresenterTestHooks::AreExpressionAnimationStatusNotificationsRaised(bool areExpressionAnimationStatusNotificationsRaised)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_areExpressionAnimationStatusNotificationsRaised = areExpressionAnimationStatusNotificationsRaised;
}

bool ScrollPresenterTestHooks::IsInteractionTrackerPointerWheelRedirectionEnabled()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_isInteractionTrackerPointerWheelRedirectionEnabled;
}

void ScrollPresenterTestHooks::IsInteractionTrackerPointerWheelRedirectionEnabled(bool isInteractionTrackerPointerWheelRedirectionEnabled)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_isInteractionTrackerPointerWheelRedirectionEnabled = isInteractionTrackerPointerWheelRedirectionEnabled;
}

winrt::IReference<bool> ScrollPresenterTestHooks::IsAnimationsEnabledOverride()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_isAnimationsEnabledOverride;
}

void ScrollPresenterTestHooks::IsAnimationsEnabledOverride(winrt::IReference<bool> isAnimationsEnabledOverride)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_isAnimationsEnabledOverride = isAnimationsEnabledOverride;
}

int ScrollPresenterTestHooks::MouseWheelDeltaForVelocityUnit()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelDeltaForVelocityUnit;
}

void ScrollPresenterTestHooks::MouseWheelDeltaForVelocityUnit(int mouseWheelDeltaForVelocityUnit)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelDeltaForVelocityUnit = mouseWheelDeltaForVelocityUnit;
}

int ScrollPresenterTestHooks::MouseWheelScrollLines()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelScrollLines;
}

void ScrollPresenterTestHooks::MouseWheelScrollLines(int mouseWheelScrollLines)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelScrollLines = mouseWheelScrollLines;
}

int ScrollPresenterTestHooks::MouseWheelScrollChars()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelScrollChars;
}

void ScrollPresenterTestHooks::MouseWheelScrollChars(int mouseWheelScrollChars)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelScrollChars = mouseWheelScrollChars;
}

float ScrollPresenterTestHooks::MouseWheelInertiaDecayRate()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelInertiaDecayRate;
}

void ScrollPresenterTestHooks::MouseWheelInertiaDecayRate(float mouseWheelInertiaDecayRate)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelInertiaDecayRate = mouseWheelInertiaDecayRate;
}

void ScrollPresenterTestHooks::GetOffsetsChangeVelocityParameters(int& millisecondsPerUnit, int& minMilliseconds, int& maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    millisecondsPerUnit = hooks->m_offsetsChangeMsPerUnit;
    minMilliseconds = hooks->m_offsetsChangeMinMs;
    maxMilliseconds = hooks->m_offsetsChangeMaxMs;
}

void ScrollPresenterTestHooks::SetOffsetsChangeVelocityParameters(int millisecondsPerUnit, int minMilliseconds, int maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_offsetsChangeMsPerUnit = millisecondsPerUnit;
    hooks->m_offsetsChangeMinMs = minMilliseconds;
    hooks->m_offsetsChangeMaxMs = maxMilliseconds;
}

void ScrollPresenterTestHooks::GetZoomFactorChangeVelocityParameters(int& millisecondsPerUnit, int& minMilliseconds, int& maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    millisecondsPerUnit = hooks->m_zoomFactorChangeMsPerUnit;
    minMilliseconds = hooks->m_zoomFactorChangeMinMs;
    maxMilliseconds = hooks->m_zoomFactorChangeMaxMs;
}

void ScrollPresenterTestHooks::SetZoomFactorChangeVelocityParameters(int millisecondsPerUnit, int minMilliseconds, int maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_zoomFactorChangeMsPerUnit = millisecondsPerUnit;
    hooks->m_zoomFactorChangeMinMs = minMilliseconds;
    hooks->m_zoomFactorChangeMaxMs = maxMilliseconds;
}

void ScrollPresenterTestHooks::GetContentLayoutOffsetX(const winrt::ScrollPresenter& scrollPresenter, float& contentLayoutOffsetX)
{
    if (scrollPresenter)
    {
        contentLayoutOffsetX = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetContentLayoutOffsetX();
    }
    else
    {
        contentLayoutOffsetX = 0.0f;
    }
}

void ScrollPresenterTestHooks::SetContentLayoutOffsetX(const winrt::ScrollPresenter& scrollPresenter, float contentLayoutOffsetX)
{
    if (scrollPresenter)
    {
        winrt::get_self<ScrollPresenter>(scrollPresenter)->SetContentLayoutOffsetX(contentLayoutOffsetX);
    }
}

void ScrollPresenterTestHooks::GetContentLayoutOffsetY(const winrt::ScrollPresenter& scrollPresenter, float& contentLayoutOffsetY)
{
    if (scrollPresenter)
    {
        contentLayoutOffsetY = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetContentLayoutOffsetY();
    }
    else
    {
        contentLayoutOffsetY = 0.0f;
    }
}

void ScrollPresenterTestHooks::SetContentLayoutOffsetY(const winrt::ScrollPresenter& scrollPresenter, float contentLayoutOffsetY)
{
    if (scrollPresenter)
    {
        winrt::get_self<ScrollPresenter>(scrollPresenter)->SetContentLayoutOffsetY(contentLayoutOffsetY);
    }
}

winrt::float2 ScrollPresenterTestHooks::GetArrangeRenderSizesDelta(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetArrangeRenderSizesDelta();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::float2 ScrollPresenterTestHooks::GetMinPosition(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetMinPosition();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::float2 ScrollPresenterTestHooks::GetMaxPosition(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetMaxPosition();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::ScrollPresenterViewChangeResult ScrollPresenterTestHooks::GetScrollCompletedResult(const winrt::ScrollingScrollCompletedEventArgs& scrollCompletedEventArgs)
{
    if (scrollCompletedEventArgs)
    {
        const ScrollPresenterViewChangeResult result = winrt::get_self<ScrollingScrollCompletedEventArgs>(scrollCompletedEventArgs)->Result();
        return TestHooksViewChangeResult(result);
    }
    return winrt::ScrollPresenterViewChangeResult::Completed;
}

winrt::ScrollPresenterViewChangeResult ScrollPresenterTestHooks::GetZoomCompletedResult(const winrt::ScrollingZoomCompletedEventArgs& zoomCompletedEventArgs)
{
    if (zoomCompletedEventArgs)
    {
        const ScrollPresenterViewChangeResult result = winrt::get_self<ScrollingZoomCompletedEventArgs>(zoomCompletedEventArgs)->Result();
        return TestHooksViewChangeResult(result);
    }
    return winrt::ScrollPresenterViewChangeResult::Completed;
}

void ScrollPresenterTestHooks::NotifyAnchorEvaluated(
    const winrt::ScrollPresenter& sender,
    const winrt::UIElement& anchorElement,
    double viewportAnchorPointHorizontalOffset,
    double viewportAnchorPointVerticalOffset)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_anchorEvaluatedEventSource)
    {
        auto anchorEvaluatedEventArgs = winrt::make<ScrollPresenterTestHooksAnchorEvaluatedEventArgs>(
            anchorElement, viewportAnchorPointHorizontalOffset, viewportAnchorPointVerticalOffset);

        hooks->m_anchorEvaluatedEventSource(sender, anchorEvaluatedEventArgs);
    }
}

winrt::event_token ScrollPresenterTestHooks::AnchorEvaluated(winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::ScrollPresenterTestHooksAnchorEvaluatedEventArgs> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_anchorEvaluatedEventSource.add(value);
}

void ScrollPresenterTestHooks::AnchorEvaluated(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_anchorEvaluatedEventSource.remove(token);
}

void ScrollPresenterTestHooks::NotifyInteractionSourcesChanged(
    const winrt::ScrollPresenter& sender,
    const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_interactionSourcesChangedEventSource)
    {
        auto interactionSourcesChangedEventArgs = winrt::make<ScrollPresenterTestHooksInteractionSourcesChangedEventArgs>(
            interactionSources);

        hooks->m_interactionSourcesChangedEventSource(sender, interactionSourcesChangedEventArgs);
    }
}

winrt::event_token ScrollPresenterTestHooks::InteractionSourcesChanged(winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::ScrollPresenterTestHooksInteractionSourcesChangedEventArgs> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_interactionSourcesChangedEventSource.add(value);
}

void ScrollPresenterTestHooks::InteractionSourcesChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_interactionSourcesChangedEventSource.remove(token);
}

void ScrollPresenterTestHooks::NotifyExpressionAnimationStatusChanged(
    const winrt::ScrollPresenter& sender,
    bool isExpressionAnimationStarted,
    wstring_view const& propertyName)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_expressionAnimationStatusChangedEventSource)
    {
        auto expressionAnimationStatusChangedEventArgs = winrt::make<ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs>(
            isExpressionAnimationStarted, propertyName);

        hooks->m_expressionAnimationStatusChangedEventSource(sender, expressionAnimationStatusChangedEventArgs);
    }
}

winrt::event_token ScrollPresenterTestHooks::ExpressionAnimationStatusChanged(winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::ScrollPresenterTestHooksExpressionAnimationStatusChangedEventArgs> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_expressionAnimationStatusChangedEventSource.add(value);
}

void ScrollPresenterTestHooks::ExpressionAnimationStatusChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_expressionAnimationStatusChangedEventSource.remove(token);
}

void ScrollPresenterTestHooks::NotifyContentLayoutOffsetXChanged(const winrt::ScrollPresenter& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_contentLayoutOffsetXChangedEventSource)
    {
        hooks->m_contentLayoutOffsetXChangedEventSource(sender, nullptr);
    }
}

winrt::event_token ScrollPresenterTestHooks::ContentLayoutOffsetXChanged(winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_contentLayoutOffsetXChangedEventSource.add(value);
}

void ScrollPresenterTestHooks::ContentLayoutOffsetXChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_contentLayoutOffsetXChangedEventSource.remove(token);
}

void ScrollPresenterTestHooks::NotifyContentLayoutOffsetYChanged(const winrt::ScrollPresenter& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_contentLayoutOffsetYChangedEventSource)
    {
        hooks->m_contentLayoutOffsetYChangedEventSource(sender, nullptr);
    }
}

winrt::event_token ScrollPresenterTestHooks::ContentLayoutOffsetYChanged(winrt::TypedEventHandler<winrt::ScrollPresenter, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_contentLayoutOffsetYChangedEventSource.add(value);
}

void ScrollPresenterTestHooks::ContentLayoutOffsetYChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_contentLayoutOffsetYChangedEventSource.remove(token);
}

winrt::IVector<winrt::ScrollSnapPointBase> ScrollPresenterTestHooks::GetConsolidatedHorizontalScrollSnapPoints(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetConsolidatedHorizontalScrollSnapPoints();
    }
    else
    {
        return winrt::make<Vector<winrt::ScrollSnapPointBase>>();
    }
}

winrt::IVector<winrt::ScrollSnapPointBase> ScrollPresenterTestHooks::GetConsolidatedVerticalScrollSnapPoints(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetConsolidatedVerticalScrollSnapPoints();
    }
    else
    {
        return winrt::make<Vector<winrt::ScrollSnapPointBase>>();
    }
}

winrt::IVector<winrt::ZoomSnapPointBase> ScrollPresenterTestHooks::GetConsolidatedZoomSnapPoints(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetConsolidatedZoomSnapPoints();
    }
    else
    {
        return winrt::make<Vector<winrt::ZoomSnapPointBase>>();
    }
}

winrt::float2 ScrollPresenterTestHooks::GetHorizontalSnapPointActualApplicableZone(
    const winrt::ScrollPresenter& scrollPresenter,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        const SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetHorizontalSnapPointWrapper(scrollSnapPoint);
        auto zone = snapPointWrapper->ActualApplicableZone();

        return winrt::float2{ static_cast<float>(std::get<0>(zone)), static_cast<float>(std::get<1>(zone)) };
    }
    else
    {
        return winrt::float2{ 0.0f, 0.0f };
    }
}

winrt::float2 ScrollPresenterTestHooks::GetVerticalSnapPointActualApplicableZone(
    const winrt::ScrollPresenter& scrollPresenter,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        const SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetVerticalSnapPointWrapper(scrollSnapPoint);
        auto zone = snapPointWrapper->ActualApplicableZone();

        return winrt::float2{ static_cast<float>(std::get<0>(zone)), static_cast<float>(std::get<1>(zone)) };
    }
    else
    {
        return winrt::float2{ 0.0f, 0.0f };
    }
}

winrt::float2 ScrollPresenterTestHooks::GetZoomSnapPointActualApplicableZone(
    const winrt::ScrollPresenter& scrollPresenter,
    const winrt::ZoomSnapPointBase& zoomSnapPoint)
{
    if (zoomSnapPoint)
    {
        const SnapPointWrapper<winrt::ZoomSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetZoomSnapPointWrapper(zoomSnapPoint);
        auto zone = snapPointWrapper->ActualApplicableZone();

        return winrt::float2{ static_cast<float>(std::get<0>(zone)), static_cast<float>(std::get<1>(zone)) };
    }
    else
    {
        return winrt::float2{ 0.0f, 0.0f };
    }
}

int ScrollPresenterTestHooks::GetHorizontalSnapPointCombinationCount(
    const winrt::ScrollPresenter& scrollPresenter,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        const SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetHorizontalSnapPointWrapper(scrollSnapPoint);

        return snapPointWrapper->CombinationCount();
    }
    else
    {
        return 0;
    }
}

int ScrollPresenterTestHooks::GetVerticalSnapPointCombinationCount(
    const winrt::ScrollPresenter& scrollPresenter,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        const SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetVerticalSnapPointWrapper(scrollSnapPoint);

        return snapPointWrapper->CombinationCount();
    }
    else
    {
        return 0;
    }
}

int ScrollPresenterTestHooks::GetZoomSnapPointCombinationCount(
    const winrt::ScrollPresenter& scrollPresenter,
    const winrt::ZoomSnapPointBase& zoomSnapPoint)
{
    if (zoomSnapPoint)
    {
        const SnapPointWrapper<winrt::ZoomSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetZoomSnapPointWrapper(zoomSnapPoint);

        return snapPointWrapper->CombinationCount();
    }
    else
    {
        return 0;
    }
}

winrt::Color ScrollPresenterTestHooks::GetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint)
{

#ifdef _DEBUG
    if (snapPoint)
    {
        return winrt::get_self<SnapPointBase>(snapPoint)->VisualizationColor();
    }
#endif // _DEBUG
    return winrt::Colors::Black();
}

void ScrollPresenterTestHooks::SetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint, const winrt::Color& color)
{
#ifdef _DEBUG
    if (snapPoint)
    {
        winrt::get_self<SnapPointBase>(snapPoint)->VisualizationColor(color);
    }
#endif // _DEBUG
}

winrt::ScrollPresenterViewChangeResult ScrollPresenterTestHooks::TestHooksViewChangeResult(ScrollPresenterViewChangeResult result)
{
    switch (result)
    {
    case ScrollPresenterViewChangeResult::Ignored:
        return winrt::ScrollPresenterViewChangeResult::Ignored;
    case ScrollPresenterViewChangeResult::Interrupted:
        return winrt::ScrollPresenterViewChangeResult::Interrupted;
    default:
        return winrt::ScrollPresenterViewChangeResult::Completed;
    }
}

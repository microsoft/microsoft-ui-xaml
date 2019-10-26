// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollingPresenterTestHooksFactory.h"
#include "Vector.h"

com_ptr<ScrollingPresenterTestHooks> ScrollingPresenterTestHooks::s_testHooks{};

ScrollingPresenterTestHooks::ScrollingPresenterTestHooks()
{
    m_mouseWheelInertiaDecayRate = SharedHelpers::IsRS2OrHigher() ? ScrollingPresenter::s_mouseWheelInertiaDecayRate : ScrollingPresenter::s_mouseWheelInertiaDecayRateRS1;
}

com_ptr<ScrollingPresenterTestHooks> ScrollingPresenterTestHooks::EnsureGlobalTestHooks() 
{
    static bool s_initialized = []() {
        s_testHooks = winrt::make_self<ScrollingPresenterTestHooks>();
        return true;
    }();
    return s_testHooks;
}

bool ScrollingPresenterTestHooks::AreAnchorNotificationsRaised()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_areAnchorNotificationsRaised;
}

void ScrollingPresenterTestHooks::AreAnchorNotificationsRaised(bool areAnchorNotificationsRaised)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_areAnchorNotificationsRaised = areAnchorNotificationsRaised;
}

bool ScrollingPresenterTestHooks::AreInteractionSourcesNotificationsRaised()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_areInteractionSourcesNotificationsRaised;
}

void ScrollingPresenterTestHooks::AreInteractionSourcesNotificationsRaised(bool areInteractionSourcesNotificationsRaised)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_areInteractionSourcesNotificationsRaised = areInteractionSourcesNotificationsRaised;
}

bool ScrollingPresenterTestHooks::AreExpressionAnimationStatusNotificationsRaised()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_areExpressionAnimationStatusNotificationsRaised;
}

void ScrollingPresenterTestHooks::AreExpressionAnimationStatusNotificationsRaised(bool areExpressionAnimationStatusNotificationsRaised)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_areExpressionAnimationStatusNotificationsRaised = areExpressionAnimationStatusNotificationsRaised;
}

bool ScrollingPresenterTestHooks::IsInteractionTrackerPointerWheelRedirectionEnabled()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_isInteractionTrackerPointerWheelRedirectionEnabled;
}

void ScrollingPresenterTestHooks::IsInteractionTrackerPointerWheelRedirectionEnabled(bool isInteractionTrackerPointerWheelRedirectionEnabled)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_isInteractionTrackerPointerWheelRedirectionEnabled = isInteractionTrackerPointerWheelRedirectionEnabled;
}

winrt::IReference<bool> ScrollingPresenterTestHooks::IsAnimationsEnabledOverride()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_isAnimationsEnabledOverride;
}

void ScrollingPresenterTestHooks::IsAnimationsEnabledOverride(winrt::IReference<bool> isAnimationsEnabledOverride)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_isAnimationsEnabledOverride = isAnimationsEnabledOverride;
}

int ScrollingPresenterTestHooks::MouseWheelDeltaForVelocityUnit()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelDeltaForVelocityUnit;
}

void ScrollingPresenterTestHooks::MouseWheelDeltaForVelocityUnit(int mouseWheelDeltaForVelocityUnit)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelDeltaForVelocityUnit = mouseWheelDeltaForVelocityUnit;
}

int ScrollingPresenterTestHooks::MouseWheelScrollLines()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelScrollLines;
}

void ScrollingPresenterTestHooks::MouseWheelScrollLines(int mouseWheelScrollLines)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelScrollLines = mouseWheelScrollLines;
}

int ScrollingPresenterTestHooks::MouseWheelScrollChars()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelScrollChars;
}

void ScrollingPresenterTestHooks::MouseWheelScrollChars(int mouseWheelScrollChars)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelScrollChars = mouseWheelScrollChars;
}

float ScrollingPresenterTestHooks::MouseWheelInertiaDecayRate()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelInertiaDecayRate;
}

void ScrollingPresenterTestHooks::MouseWheelInertiaDecayRate(float mouseWheelInertiaDecayRate)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelInertiaDecayRate = mouseWheelInertiaDecayRate;
}

void ScrollingPresenterTestHooks::GetOffsetsChangeVelocityParameters(int& millisecondsPerUnit, int& minMilliseconds, int& maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    millisecondsPerUnit = hooks->m_offsetsChangeMsPerUnit;
    minMilliseconds = hooks->m_offsetsChangeMinMs;
    maxMilliseconds = hooks->m_offsetsChangeMaxMs;
}

void ScrollingPresenterTestHooks::SetOffsetsChangeVelocityParameters(int millisecondsPerUnit, int minMilliseconds, int maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_offsetsChangeMsPerUnit = millisecondsPerUnit;
    hooks->m_offsetsChangeMinMs = minMilliseconds;
    hooks->m_offsetsChangeMaxMs = maxMilliseconds;
}

void ScrollingPresenterTestHooks::GetZoomFactorChangeVelocityParameters(int& millisecondsPerUnit, int& minMilliseconds, int& maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    millisecondsPerUnit = hooks->m_zoomFactorChangeMsPerUnit;
    minMilliseconds = hooks->m_zoomFactorChangeMinMs;
    maxMilliseconds = hooks->m_zoomFactorChangeMaxMs;
}

void ScrollingPresenterTestHooks::SetZoomFactorChangeVelocityParameters(int millisecondsPerUnit, int minMilliseconds, int maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_zoomFactorChangeMsPerUnit = millisecondsPerUnit;
    hooks->m_zoomFactorChangeMinMs = minMilliseconds;
    hooks->m_zoomFactorChangeMaxMs = maxMilliseconds;
}

void ScrollingPresenterTestHooks::GetContentLayoutOffsetX(const winrt::ScrollingPresenter& scrollingPresenter, float& contentLayoutOffsetX)
{
    if (scrollingPresenter)
    {
        contentLayoutOffsetX = winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetContentLayoutOffsetX();
    }
    else
    {
        contentLayoutOffsetX = 0.0f;
    }
}

void ScrollingPresenterTestHooks::SetContentLayoutOffsetX(const winrt::ScrollingPresenter& scrollingPresenter, float contentLayoutOffsetX)
{
    if (scrollingPresenter)
    {
        winrt::get_self<ScrollingPresenter>(scrollingPresenter)->SetContentLayoutOffsetX(contentLayoutOffsetX);
    }
}

void ScrollingPresenterTestHooks::GetContentLayoutOffsetY(const winrt::ScrollingPresenter& scrollingPresenter, float& contentLayoutOffsetY)
{
    if (scrollingPresenter)
    {
        contentLayoutOffsetY = winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetContentLayoutOffsetY();
    }
    else
    {
        contentLayoutOffsetY = 0.0f;
    }
}

void ScrollingPresenterTestHooks::SetContentLayoutOffsetY(const winrt::ScrollingPresenter& scrollingPresenter, float contentLayoutOffsetY)
{
    if (scrollingPresenter)
    {
        winrt::get_self<ScrollingPresenter>(scrollingPresenter)->SetContentLayoutOffsetY(contentLayoutOffsetY);
    }
}

winrt::float2 ScrollingPresenterTestHooks::GetArrangeRenderSizesDelta(const winrt::ScrollingPresenter& scrollingPresenter)
{
    if (scrollingPresenter)
    {
        return winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetArrangeRenderSizesDelta();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::InteractionTracker ScrollingPresenterTestHooks::GetInteractionTracker(const winrt::ScrollingPresenter& scrollingPresenter)
{
    if (scrollingPresenter)
    {
        return winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetInteractionTracker();
    }
    return nullptr;
}

winrt::float2 ScrollingPresenterTestHooks::GetMinPosition(const winrt::ScrollingPresenter& scrollingPresenter)
{
    if (scrollingPresenter)
    {
        return winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetMinPosition();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::float2 ScrollingPresenterTestHooks::GetMaxPosition(const winrt::ScrollingPresenter& scrollingPresenter)
{
    if (scrollingPresenter)
    {
        return winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetMaxPosition();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::ScrollingPresenterViewChangeResult ScrollingPresenterTestHooks::GetScrollCompletedResult(const winrt::ScrollingScrollCompletedEventArgs& scrollCompletedEventArgs)
{
    if (scrollCompletedEventArgs)
    {
        ScrollingPresenterViewChangeResult result = winrt::get_self<ScrollingScrollCompletedEventArgs>(scrollCompletedEventArgs)->Result();
        return TestHooksViewChangeResult(result);
    }
    return winrt::ScrollingPresenterViewChangeResult::Completed;
}

winrt::ScrollingPresenterViewChangeResult ScrollingPresenterTestHooks::GetZoomCompletedResult(const winrt::ScrollingZoomCompletedEventArgs& zoomCompletedEventArgs)
{
    if (zoomCompletedEventArgs)
    {
        ScrollingPresenterViewChangeResult result = winrt::get_self<ScrollingZoomCompletedEventArgs>(zoomCompletedEventArgs)->Result();
        return TestHooksViewChangeResult(result);
    }
    return winrt::ScrollingPresenterViewChangeResult::Completed;
}

void ScrollingPresenterTestHooks::NotifyAnchorEvaluated(
    const winrt::ScrollingPresenter& sender,
    const winrt::UIElement& anchorElement,
    double viewportAnchorPointHorizontalOffset,
    double viewportAnchorPointVerticalOffset)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_anchorEvaluatedEventSource)
    {
        auto anchorEvaluatedEventArgs = winrt::make<ScrollingPresenterTestHooksAnchorEvaluatedEventArgs>(
            anchorElement, viewportAnchorPointHorizontalOffset, viewportAnchorPointVerticalOffset);

        hooks->m_anchorEvaluatedEventSource(sender, anchorEvaluatedEventArgs);
    }
}

winrt::event_token ScrollingPresenterTestHooks::AnchorEvaluated(winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::ScrollingPresenterTestHooksAnchorEvaluatedEventArgs> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_anchorEvaluatedEventSource.add(value);
}

void ScrollingPresenterTestHooks::AnchorEvaluated(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_anchorEvaluatedEventSource.remove(token);
}

void ScrollingPresenterTestHooks::NotifyInteractionSourcesChanged(
    const winrt::ScrollingPresenter& sender,
    const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_interactionSourcesChangedEventSource)
    {
        auto interactionSourcesChangedEventArgs = winrt::make<ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs>(
            interactionSources);

        hooks->m_interactionSourcesChangedEventSource(sender, interactionSourcesChangedEventArgs);
    }
}

winrt::event_token ScrollingPresenterTestHooks::InteractionSourcesChanged(winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::ScrollingPresenterTestHooksInteractionSourcesChangedEventArgs> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_interactionSourcesChangedEventSource.add(value);
}

void ScrollingPresenterTestHooks::InteractionSourcesChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_interactionSourcesChangedEventSource.remove(token);
}

void ScrollingPresenterTestHooks::NotifyExpressionAnimationStatusChanged(
    const winrt::ScrollingPresenter& sender,
    bool isExpressionAnimationStarted,
    wstring_view const& propertyName)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_expressionAnimationStatusChangedEventSource)
    {
        auto expressionAnimationStatusChangedEventArgs = winrt::make<ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs>(
            isExpressionAnimationStarted, propertyName);

        hooks->m_expressionAnimationStatusChangedEventSource(sender, expressionAnimationStatusChangedEventArgs);
    }
}

winrt::event_token ScrollingPresenterTestHooks::ExpressionAnimationStatusChanged(winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::ScrollingPresenterTestHooksExpressionAnimationStatusChangedEventArgs> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_expressionAnimationStatusChangedEventSource.add(value);
}

void ScrollingPresenterTestHooks::ExpressionAnimationStatusChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_expressionAnimationStatusChangedEventSource.remove(token);
}

void ScrollingPresenterTestHooks::NotifyContentLayoutOffsetXChanged(const winrt::ScrollingPresenter& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_contentLayoutOffsetXChangedEventSource)
    {
        hooks->m_contentLayoutOffsetXChangedEventSource(sender, nullptr);
    }
}

winrt::event_token ScrollingPresenterTestHooks::ContentLayoutOffsetXChanged(winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_contentLayoutOffsetXChangedEventSource.add(value);
}

void ScrollingPresenterTestHooks::ContentLayoutOffsetXChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_contentLayoutOffsetXChangedEventSource.remove(token);
}

void ScrollingPresenterTestHooks::NotifyContentLayoutOffsetYChanged(const winrt::ScrollingPresenter& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_contentLayoutOffsetYChangedEventSource)
    {
        hooks->m_contentLayoutOffsetYChangedEventSource(sender, nullptr);
    }
}

winrt::event_token ScrollingPresenterTestHooks::ContentLayoutOffsetYChanged(winrt::TypedEventHandler<winrt::ScrollingPresenter, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_contentLayoutOffsetYChangedEventSource.add(value);
}

void ScrollingPresenterTestHooks::ContentLayoutOffsetYChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_contentLayoutOffsetYChangedEventSource.remove(token);
}

winrt::IVector<winrt::ScrollSnapPointBase> ScrollingPresenterTestHooks::GetConsolidatedHorizontalScrollSnapPoints(const winrt::ScrollingPresenter& scrollingPresenter)
{
    if (scrollingPresenter)
    {
        return winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetConsolidatedHorizontalScrollSnapPoints();
    }
    else
    {
        return winrt::make<Vector<winrt::ScrollSnapPointBase>>();
    }
}

winrt::IVector<winrt::ScrollSnapPointBase> ScrollingPresenterTestHooks::GetConsolidatedVerticalScrollSnapPoints(const winrt::ScrollingPresenter& scrollingPresenter)
{
    if (scrollingPresenter)
    {
        return winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetConsolidatedVerticalScrollSnapPoints();
    }
    else
    {
        return winrt::make<Vector<winrt::ScrollSnapPointBase>>();
    }
}

winrt::IVector<winrt::ZoomSnapPointBase> ScrollingPresenterTestHooks::GetConsolidatedZoomSnapPoints(const winrt::ScrollingPresenter& scrollingPresenter)
{
    if (scrollingPresenter)
    {
        return winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetConsolidatedZoomSnapPoints();
    }
    else
    {
        return winrt::make<Vector<winrt::ZoomSnapPointBase>>();
    }
}

winrt::float2 ScrollingPresenterTestHooks::GetHorizontalSnapPointActualApplicableZone(
    const winrt::ScrollingPresenter& scrollingPresenter,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetHorizontalSnapPointWrapper(scrollSnapPoint);
        auto zone = snapPointWrapper->ActualApplicableZone();

        return winrt::float2{ static_cast<float>(std::get<0>(zone)), static_cast<float>(std::get<1>(zone)) };
    }
    else
    {
        return winrt::float2{ 0.0f, 0.0f };
    }
}

winrt::float2 ScrollingPresenterTestHooks::GetVerticalSnapPointActualApplicableZone(
    const winrt::ScrollingPresenter& scrollingPresenter,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetVerticalSnapPointWrapper(scrollSnapPoint);
        auto zone = snapPointWrapper->ActualApplicableZone();

        return winrt::float2{ static_cast<float>(std::get<0>(zone)), static_cast<float>(std::get<1>(zone)) };
    }
    else
    {
        return winrt::float2{ 0.0f, 0.0f };
    }
}

winrt::float2 ScrollingPresenterTestHooks::GetZoomSnapPointActualApplicableZone(
    const winrt::ScrollingPresenter& scrollingPresenter,
    const winrt::ZoomSnapPointBase& zoomSnapPoint)
{
    if (zoomSnapPoint)
    {
        SnapPointWrapper<winrt::ZoomSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetZoomSnapPointWrapper(zoomSnapPoint);
        auto zone = snapPointWrapper->ActualApplicableZone();

        return winrt::float2{ static_cast<float>(std::get<0>(zone)), static_cast<float>(std::get<1>(zone)) };
    }
    else
    {
        return winrt::float2{ 0.0f, 0.0f };
    }
}

int ScrollingPresenterTestHooks::GetHorizontalSnapPointCombinationCount(
    const winrt::ScrollingPresenter& scrollingPresenter,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetHorizontalSnapPointWrapper(scrollSnapPoint);

        return snapPointWrapper->CombinationCount();
    }
    else
    {
        return 0;
    }
}

int ScrollingPresenterTestHooks::GetVerticalSnapPointCombinationCount(
    const winrt::ScrollingPresenter& scrollingPresenter,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetVerticalSnapPointWrapper(scrollSnapPoint);

        return snapPointWrapper->CombinationCount();
    }
    else
    {
        return 0;
    }
}

int ScrollingPresenterTestHooks::GetZoomSnapPointCombinationCount(
    const winrt::ScrollingPresenter& scrollingPresenter,
    const winrt::ZoomSnapPointBase& zoomSnapPoint)
{
    if (zoomSnapPoint)
    {
        SnapPointWrapper<winrt::ZoomSnapPointBase>* snapPointWrapper = winrt::get_self<ScrollingPresenter>(scrollingPresenter)->GetZoomSnapPointWrapper(zoomSnapPoint);

        return snapPointWrapper->CombinationCount();
    }
    else
    {
        return 0;
    }
}

winrt::Color ScrollingPresenterTestHooks::GetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint)
{

#ifdef _DEBUG
    if (snapPoint)
    {
        return winrt::get_self<SnapPointBase>(snapPoint)->VisualizationColor();
    }
#endif // _DEBUG
    return winrt::Colors::Black();
}

void ScrollingPresenterTestHooks::SetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint, const winrt::Color& color)
{
#ifdef _DEBUG
    if (snapPoint)
    {
        winrt::get_self<SnapPointBase>(snapPoint)->VisualizationColor(color);
    }
#endif // _DEBUG
}

winrt::ScrollingPresenterViewChangeResult ScrollingPresenterTestHooks::TestHooksViewChangeResult(ScrollingPresenterViewChangeResult result)
{
    switch (result)
    {
    case ScrollingPresenterViewChangeResult::Ignored:
        return winrt::ScrollingPresenterViewChangeResult::Ignored;
    case ScrollingPresenterViewChangeResult::Interrupted:
        return winrt::ScrollingPresenterViewChangeResult::Interrupted;
    default:
        return winrt::ScrollingPresenterViewChangeResult::Completed;
    }
}

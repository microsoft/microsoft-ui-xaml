// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollerTestHooksFactory.h"
#include "Vector.h"

com_ptr<ScrollerTestHooks> ScrollerTestHooks::s_testHooks{};

ScrollerTestHooks::ScrollerTestHooks()
{
    m_mouseWheelInertiaDecayRate = SharedHelpers::IsRS2OrHigher() ? Scroller::s_mouseWheelInertiaDecayRate : Scroller::s_mouseWheelInertiaDecayRateRS1;
}

com_ptr<ScrollerTestHooks> ScrollerTestHooks::EnsureGlobalTestHooks() 
{
    static bool s_initialized = []() {
        s_testHooks = winrt::make_self<ScrollerTestHooks>();
        return true;
    }();
    return s_testHooks;
}

bool ScrollerTestHooks::AreAnchorNotificationsRaised()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_areAnchorNotificationsRaised;
}

void ScrollerTestHooks::AreAnchorNotificationsRaised(bool areAnchorNotificationsRaised)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_areAnchorNotificationsRaised = areAnchorNotificationsRaised;
}

bool ScrollerTestHooks::AreInteractionSourcesNotificationsRaised()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_areInteractionSourcesNotificationsRaised;
}

void ScrollerTestHooks::AreInteractionSourcesNotificationsRaised(bool areInteractionSourcesNotificationsRaised)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_areInteractionSourcesNotificationsRaised = areInteractionSourcesNotificationsRaised;
}

bool ScrollerTestHooks::AreExpressionAnimationStatusNotificationsRaised()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_areExpressionAnimationStatusNotificationsRaised;
}

void ScrollerTestHooks::AreExpressionAnimationStatusNotificationsRaised(bool areExpressionAnimationStatusNotificationsRaised)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_areExpressionAnimationStatusNotificationsRaised = areExpressionAnimationStatusNotificationsRaised;
}

bool ScrollerTestHooks::IsInteractionTrackerPointerWheelRedirectionEnabled()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_isInteractionTrackerPointerWheelRedirectionEnabled;
}

void ScrollerTestHooks::IsInteractionTrackerPointerWheelRedirectionEnabled(bool isInteractionTrackerPointerWheelRedirectionEnabled)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_isInteractionTrackerPointerWheelRedirectionEnabled = isInteractionTrackerPointerWheelRedirectionEnabled;
}

winrt::IReference<bool> ScrollerTestHooks::IsAnimationsEnabledOverride()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_isAnimationsEnabledOverride;
}

void ScrollerTestHooks::IsAnimationsEnabledOverride(winrt::IReference<bool> isAnimationsEnabledOverride)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_isAnimationsEnabledOverride = isAnimationsEnabledOverride;
}

int ScrollerTestHooks::MouseWheelDeltaForVelocityUnit()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelDeltaForVelocityUnit;
}

void ScrollerTestHooks::MouseWheelDeltaForVelocityUnit(int mouseWheelDeltaForVelocityUnit)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelDeltaForVelocityUnit = mouseWheelDeltaForVelocityUnit;
}

int ScrollerTestHooks::MouseWheelScrollLines()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelScrollLines;
}

void ScrollerTestHooks::MouseWheelScrollLines(int mouseWheelScrollLines)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelScrollLines = mouseWheelScrollLines;
}

int ScrollerTestHooks::MouseWheelScrollChars()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelScrollChars;
}

void ScrollerTestHooks::MouseWheelScrollChars(int mouseWheelScrollChars)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelScrollChars = mouseWheelScrollChars;
}

float ScrollerTestHooks::MouseWheelInertiaDecayRate()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_mouseWheelInertiaDecayRate;
}

void ScrollerTestHooks::MouseWheelInertiaDecayRate(float mouseWheelInertiaDecayRate)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_mouseWheelInertiaDecayRate = mouseWheelInertiaDecayRate;
}

void ScrollerTestHooks::GetOffsetsChangeVelocityParameters(int& millisecondsPerUnit, int& minMilliseconds, int& maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    millisecondsPerUnit = hooks->m_offsetsChangeMsPerUnit;
    minMilliseconds = hooks->m_offsetsChangeMinMs;
    maxMilliseconds = hooks->m_offsetsChangeMaxMs;
}

void ScrollerTestHooks::SetOffsetsChangeVelocityParameters(int millisecondsPerUnit, int minMilliseconds, int maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_offsetsChangeMsPerUnit = millisecondsPerUnit;
    hooks->m_offsetsChangeMinMs = minMilliseconds;
    hooks->m_offsetsChangeMaxMs = maxMilliseconds;
}

void ScrollerTestHooks::GetZoomFactorChangeVelocityParameters(int& millisecondsPerUnit, int& minMilliseconds, int& maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    millisecondsPerUnit = hooks->m_zoomFactorChangeMsPerUnit;
    minMilliseconds = hooks->m_zoomFactorChangeMinMs;
    maxMilliseconds = hooks->m_zoomFactorChangeMaxMs;
}

void ScrollerTestHooks::SetZoomFactorChangeVelocityParameters(int millisecondsPerUnit, int minMilliseconds, int maxMilliseconds)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_zoomFactorChangeMsPerUnit = millisecondsPerUnit;
    hooks->m_zoomFactorChangeMinMs = minMilliseconds;
    hooks->m_zoomFactorChangeMaxMs = maxMilliseconds;
}

void ScrollerTestHooks::GetContentLayoutOffsetX(const winrt::Scroller& scroller, float& contentLayoutOffsetX)
{
    if (scroller)
    {
        contentLayoutOffsetX = winrt::get_self<Scroller>(scroller)->GetContentLayoutOffsetX();
    }
    else
    {
        contentLayoutOffsetX = 0.0f;
    }
}

void ScrollerTestHooks::SetContentLayoutOffsetX(const winrt::Scroller& scroller, float contentLayoutOffsetX)
{
    if (scroller)
    {
        winrt::get_self<Scroller>(scroller)->SetContentLayoutOffsetX(contentLayoutOffsetX);
    }
}

void ScrollerTestHooks::GetContentLayoutOffsetY(const winrt::Scroller& scroller, float& contentLayoutOffsetY)
{
    if (scroller)
    {
        contentLayoutOffsetY = winrt::get_self<Scroller>(scroller)->GetContentLayoutOffsetY();
    }
    else
    {
        contentLayoutOffsetY = 0.0f;
    }
}

void ScrollerTestHooks::SetContentLayoutOffsetY(const winrt::Scroller& scroller, float contentLayoutOffsetY)
{
    if (scroller)
    {
        winrt::get_self<Scroller>(scroller)->SetContentLayoutOffsetY(contentLayoutOffsetY);
    }
}

winrt::float2 ScrollerTestHooks::GetArrangeRenderSizesDelta(const winrt::Scroller& scroller)
{
    if (scroller)
    {
        return winrt::get_self<Scroller>(scroller)->GetArrangeRenderSizesDelta();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::float2 ScrollerTestHooks::GetMinPosition(const winrt::Scroller& scroller)
{
    if (scroller)
    {
        return winrt::get_self<Scroller>(scroller)->GetMinPosition();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::float2 ScrollerTestHooks::GetMaxPosition(const winrt::Scroller& scroller)
{
    if (scroller)
    {
        return winrt::get_self<Scroller>(scroller)->GetMaxPosition();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::ScrollerViewChangeResult ScrollerTestHooks::GetScrollCompletedResult(const winrt::ScrollCompletedEventArgs& scrollCompletedEventArgs)
{
    if (scrollCompletedEventArgs)
    {
        ScrollerViewChangeResult result = winrt::get_self<ScrollCompletedEventArgs>(scrollCompletedEventArgs)->Result();
        return TestHooksViewChangeResult(result);
    }
    return winrt::ScrollerViewChangeResult::Completed;
}

winrt::ScrollerViewChangeResult ScrollerTestHooks::GetZoomCompletedResult(const winrt::ZoomCompletedEventArgs& zoomCompletedEventArgs)
{
    if (zoomCompletedEventArgs)
    {
        ScrollerViewChangeResult result = winrt::get_self<ZoomCompletedEventArgs>(zoomCompletedEventArgs)->Result();
        return TestHooksViewChangeResult(result);
    }
    return winrt::ScrollerViewChangeResult::Completed;
}

void ScrollerTestHooks::NotifyAnchorEvaluated(
    const winrt::Scroller& sender,
    const winrt::UIElement& anchorElement,
    double viewportAnchorPointHorizontalOffset,
    double viewportAnchorPointVerticalOffset)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_anchorEvaluatedEventSource)
    {
        auto anchorEvaluatedEventArgs = winrt::make<ScrollerTestHooksAnchorEvaluatedEventArgs>(
            anchorElement, viewportAnchorPointHorizontalOffset, viewportAnchorPointVerticalOffset);

        hooks->m_anchorEvaluatedEventSource(sender, anchorEvaluatedEventArgs);
    }
}

winrt::event_token ScrollerTestHooks::AnchorEvaluated(winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksAnchorEvaluatedEventArgs> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_anchorEvaluatedEventSource.add(value);
}

void ScrollerTestHooks::AnchorEvaluated(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_anchorEvaluatedEventSource.remove(token);
}

void ScrollerTestHooks::NotifyInteractionSourcesChanged(
    const winrt::Scroller& sender,
    const winrt::Windows::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_interactionSourcesChangedEventSource)
    {
        auto interactionSourcesChangedEventArgs = winrt::make<ScrollerTestHooksInteractionSourcesChangedEventArgs>(
            interactionSources);

        hooks->m_interactionSourcesChangedEventSource(sender, interactionSourcesChangedEventArgs);
    }
}

winrt::event_token ScrollerTestHooks::InteractionSourcesChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksInteractionSourcesChangedEventArgs> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_interactionSourcesChangedEventSource.add(value);
}

void ScrollerTestHooks::InteractionSourcesChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_interactionSourcesChangedEventSource.remove(token);
}

void ScrollerTestHooks::NotifyExpressionAnimationStatusChanged(
    const winrt::Scroller& sender,
    bool isExpressionAnimationStarted,
    wstring_view const& propertyName)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_expressionAnimationStatusChangedEventSource)
    {
        auto expressionAnimationStatusChangedEventArgs = winrt::make<ScrollerTestHooksExpressionAnimationStatusChangedEventArgs>(
            isExpressionAnimationStarted, propertyName);

        hooks->m_expressionAnimationStatusChangedEventSource(sender, expressionAnimationStatusChangedEventArgs);
    }
}

winrt::event_token ScrollerTestHooks::ExpressionAnimationStatusChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::ScrollerTestHooksExpressionAnimationStatusChangedEventArgs> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_expressionAnimationStatusChangedEventSource.add(value);
}

void ScrollerTestHooks::ExpressionAnimationStatusChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_expressionAnimationStatusChangedEventSource.remove(token);
}

void ScrollerTestHooks::NotifyContentLayoutOffsetXChanged(const winrt::Scroller& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_contentLayoutOffsetXChangedEventSource)
    {
        hooks->m_contentLayoutOffsetXChangedEventSource(sender, nullptr);
    }
}

winrt::event_token ScrollerTestHooks::ContentLayoutOffsetXChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_contentLayoutOffsetXChangedEventSource.add(value);
}

void ScrollerTestHooks::ContentLayoutOffsetXChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_contentLayoutOffsetXChangedEventSource.remove(token);
}

void ScrollerTestHooks::NotifyContentLayoutOffsetYChanged(const winrt::Scroller& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_contentLayoutOffsetYChangedEventSource)
    {
        hooks->m_contentLayoutOffsetYChangedEventSource(sender, nullptr);
    }
}

winrt::event_token ScrollerTestHooks::ContentLayoutOffsetYChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_contentLayoutOffsetYChangedEventSource.add(value);
}

void ScrollerTestHooks::ContentLayoutOffsetYChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_contentLayoutOffsetYChangedEventSource.remove(token);
}

winrt::IVector<winrt::ScrollSnapPointBase> ScrollerTestHooks::GetConsolidatedHorizontalScrollSnapPoints(const winrt::Scroller& scroller)
{
    if (scroller)
    {
        return winrt::get_self<Scroller>(scroller)->GetConsolidatedHorizontalScrollSnapPoints();
    }
    else
    {
        return winrt::make<Vector<winrt::ScrollSnapPointBase>>();
    }
}

winrt::IVector<winrt::ScrollSnapPointBase> ScrollerTestHooks::GetConsolidatedVerticalScrollSnapPoints(const winrt::Scroller& scroller)
{
    if (scroller)
    {
        return winrt::get_self<Scroller>(scroller)->GetConsolidatedVerticalScrollSnapPoints();
    }
    else
    {
        return winrt::make<Vector<winrt::ScrollSnapPointBase>>();
    }
}

winrt::IVector<winrt::ZoomSnapPointBase> ScrollerTestHooks::GetConsolidatedZoomSnapPoints(const winrt::Scroller& scroller)
{
    if (scroller)
    {
        return winrt::get_self<Scroller>(scroller)->GetConsolidatedZoomSnapPoints();
    }
    else
    {
        return winrt::make<Vector<winrt::ZoomSnapPointBase>>();
    }
}

winrt::float2 ScrollerTestHooks::GetHorizontalSnapPointActualApplicableZone(
    const winrt::Scroller& scroller,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<Scroller>(scroller)->GetHorizontalSnapPointWrapper(scrollSnapPoint);
        auto zone = snapPointWrapper->ActualApplicableZone();

        return winrt::float2{ static_cast<float>(std::get<0>(zone)), static_cast<float>(std::get<1>(zone)) };
    }
    else
    {
        return winrt::float2{ 0.0f, 0.0f };
    }
}

winrt::float2 ScrollerTestHooks::GetVerticalSnapPointActualApplicableZone(
    const winrt::Scroller& scroller,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<Scroller>(scroller)->GetVerticalSnapPointWrapper(scrollSnapPoint);
        auto zone = snapPointWrapper->ActualApplicableZone();

        return winrt::float2{ static_cast<float>(std::get<0>(zone)), static_cast<float>(std::get<1>(zone)) };
    }
    else
    {
        return winrt::float2{ 0.0f, 0.0f };
    }
}

winrt::float2 ScrollerTestHooks::GetZoomSnapPointActualApplicableZone(
    const winrt::Scroller& scroller,
    const winrt::ZoomSnapPointBase& zoomSnapPoint)
{
    if (zoomSnapPoint)
    {
        SnapPointWrapper<winrt::ZoomSnapPointBase>* snapPointWrapper = winrt::get_self<Scroller>(scroller)->GetZoomSnapPointWrapper(zoomSnapPoint);
        auto zone = snapPointWrapper->ActualApplicableZone();

        return winrt::float2{ static_cast<float>(std::get<0>(zone)), static_cast<float>(std::get<1>(zone)) };
    }
    else
    {
        return winrt::float2{ 0.0f, 0.0f };
    }
}

int ScrollerTestHooks::GetHorizontalSnapPointCombinationCount(
    const winrt::Scroller& scroller,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<Scroller>(scroller)->GetHorizontalSnapPointWrapper(scrollSnapPoint);

        return snapPointWrapper->CombinationCount();
    }
    else
    {
        return 0;
    }
}

int ScrollerTestHooks::GetVerticalSnapPointCombinationCount(
    const winrt::Scroller& scroller,
    const winrt::ScrollSnapPointBase& scrollSnapPoint)
{
    if (scrollSnapPoint)
    {
        SnapPointWrapper<winrt::ScrollSnapPointBase>* snapPointWrapper = winrt::get_self<Scroller>(scroller)->GetVerticalSnapPointWrapper(scrollSnapPoint);

        return snapPointWrapper->CombinationCount();
    }
    else
    {
        return 0;
    }
}

int ScrollerTestHooks::GetZoomSnapPointCombinationCount(
    const winrt::Scroller& scroller,
    const winrt::ZoomSnapPointBase& zoomSnapPoint)
{
    if (zoomSnapPoint)
    {
        SnapPointWrapper<winrt::ZoomSnapPointBase>* snapPointWrapper = winrt::get_self<Scroller>(scroller)->GetZoomSnapPointWrapper(zoomSnapPoint);

        return snapPointWrapper->CombinationCount();
    }
    else
    {
        return 0;
    }
}

winrt::Color ScrollerTestHooks::GetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint)
{

#ifdef _DEBUG
    if (snapPoint)
    {
        return winrt::get_self<SnapPointBase>(snapPoint)->VisualizationColor();
    }
#endif // _DEBUG
    return winrt::Colors::Black();
}

void ScrollerTestHooks::SetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint, const winrt::Color& color)
{
#ifdef _DEBUG
    if (snapPoint)
    {
        winrt::get_self<SnapPointBase>(snapPoint)->VisualizationColor(color);
    }
#endif // _DEBUG
}

winrt::ScrollerViewChangeResult ScrollerTestHooks::TestHooksViewChangeResult(ScrollerViewChangeResult result)
{
    switch (result)
    {
    case ScrollerViewChangeResult::Ignored:
        return winrt::ScrollerViewChangeResult::Ignored;
    case ScrollerViewChangeResult::Interrupted:
        return winrt::ScrollerViewChangeResult::Interrupted;
    default:
        return winrt::ScrollerViewChangeResult::Completed;
    }
}

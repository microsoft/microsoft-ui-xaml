// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollPresenterTestHooksFactory.h"
#include "Vector.h"

com_ptr<ScrollPresenterTestHooks> ScrollPresenterTestHooks::s_testHooks{};

ScrollPresenterTestHooks::ScrollPresenterTestHooks()
{
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
        contentLayoutOffsetX = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetContentLayoutOffsetXDbg();
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
        winrt::get_self<ScrollPresenter>(scrollPresenter)->SetContentLayoutOffsetXDbg(contentLayoutOffsetX);
    }
}

void ScrollPresenterTestHooks::GetContentLayoutOffsetY(const winrt::ScrollPresenter& scrollPresenter, float& contentLayoutOffsetY)
{
    if (scrollPresenter)
    {
        contentLayoutOffsetY = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetContentLayoutOffsetYDbg();
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
        winrt::get_self<ScrollPresenter>(scrollPresenter)->SetContentLayoutOffsetYDbg(contentLayoutOffsetY);
    }
}

winrt::hstring ScrollPresenterTestHooks::GetTransformExpressionAnimationExpression(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
       return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetTransformExpressionAnimationExpressionDbg();
    }

    return L"";
}

void ScrollPresenterTestHooks::SetTransformExpressionAnimationExpression(const winrt::ScrollPresenter& scrollPresenter, winrt::hstring const& transformExpressionAnimationExpression)
{
    if (scrollPresenter)
    {
        winrt::get_self<ScrollPresenter>(scrollPresenter)->SetTransformExpressionAnimationExpressionDbg(transformExpressionAnimationExpression);
    }
}

winrt::hstring ScrollPresenterTestHooks::GetMinPositionExpressionAnimationExpression(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetMinPositionExpressionAnimationExpressionDbg();
    }

    return L"";
}

void ScrollPresenterTestHooks::SetMinPositionExpressionAnimationExpression(const winrt::ScrollPresenter& scrollPresenter, winrt::hstring const& minPositionExpressionAnimationExpression)
{
    if (scrollPresenter)
    {
        winrt::get_self<ScrollPresenter>(scrollPresenter)->SetMinPositionExpressionAnimationExpressionDbg(minPositionExpressionAnimationExpression);
    }
}

winrt::hstring ScrollPresenterTestHooks::GetMaxPositionExpressionAnimationExpression(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetMaxPositionExpressionAnimationExpressionDbg();
    }

    return L"";
}

void ScrollPresenterTestHooks::SetMaxPositionExpressionAnimationExpression(const winrt::ScrollPresenter& scrollPresenter, winrt::hstring const& maxPositionExpressionAnimationExpression)
{
    if (scrollPresenter)
    {
        winrt::get_self<ScrollPresenter>(scrollPresenter)->SetMaxPositionExpressionAnimationExpressionDbg(maxPositionExpressionAnimationExpression);
    }
}

winrt::float2 ScrollPresenterTestHooks::GetArrangeRenderSizesDelta(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetArrangeRenderSizesDeltaDbg();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::float2 ScrollPresenterTestHooks::GetPosition(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetPositionDbg();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

winrt::float2 ScrollPresenterTestHooks::GetMinPosition(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetMinPositionDbg();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

void ScrollPresenterTestHooks::SetMinPosition(const winrt::ScrollPresenter& scrollPresenter, winrt::float2 minPosition)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->SetMinPositionDbg(minPosition);
    }
}

winrt::float2 ScrollPresenterTestHooks::GetMaxPosition(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetMaxPositionDbg();
    }
    return winrt::float2{ 0.0f, 0.0f };
}

void ScrollPresenterTestHooks::SetMaxPosition(const winrt::ScrollPresenter& scrollPresenter, winrt::float2 maxPosition)
{
    if (scrollPresenter)
    {
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->SetMaxPositionDbg(maxPosition);
    }
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
    const winrt::Microsoft::UI::Composition::Interactions::CompositionInteractionSourceCollection& interactionSources)
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
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetConsolidatedHorizontalScrollSnapPointsDbg();
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
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetConsolidatedVerticalScrollSnapPointsDbg();
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
        return winrt::get_self<ScrollPresenter>(scrollPresenter)->GetConsolidatedZoomSnapPointsDbg();
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
        const auto snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetHorizontalSnapPointWrapperDbg(scrollSnapPoint);
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
        const auto snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetVerticalSnapPointWrapperDbg(scrollSnapPoint);
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
        const auto snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetZoomSnapPointWrapperDbg(zoomSnapPoint);
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
        const auto snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetHorizontalSnapPointWrapperDbg(scrollSnapPoint);

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
        const auto snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetVerticalSnapPointWrapperDbg(scrollSnapPoint);

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
        const auto snapPointWrapper = winrt::get_self<ScrollPresenter>(scrollPresenter)->GetZoomSnapPointWrapperDbg(zoomSnapPoint);

        return snapPointWrapper->CombinationCount();
    }
    else
    {
        return 0;
    }
}

winrt::Color ScrollPresenterTestHooks::GetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint)
{

#ifdef DBG
    if (snapPoint)
    {
        return winrt::get_self<SnapPointBase>(snapPoint)->VisualizationColor();
    }
#endif // DBG
    return winrt::Colors::Black();
}

void ScrollPresenterTestHooks::SetSnapPointVisualizationColor(const winrt::SnapPointBase& snapPoint, const winrt::Color& color)
{
#ifdef DBG
    if (snapPoint)
    {
        winrt::get_self<SnapPointBase>(snapPoint)->VisualizationColor(color);
    }
#endif // DBG
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

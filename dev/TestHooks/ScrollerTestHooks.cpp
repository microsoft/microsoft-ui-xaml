// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollerTestHooksFactory.h"
#include "Vector.h"

com_ptr<ScrollerTestHooks> ScrollerTestHooks::s_testHooks{};

ScrollerTestHooks::ScrollerTestHooks()
{
    m_offsetsChangeMsPerUnit = Scroller::s_offsetsChangeMsPerUnit;
    m_offsetsChangeMinMs = Scroller::s_offsetsChangeMinMs;
    m_offsetsChangeMaxMs = Scroller::s_offsetsChangeMaxMs;

    m_zoomFactorChangeMsPerUnit = Scroller::s_zoomFactorChangeMsPerUnit;
    m_zoomFactorChangeMinMs = Scroller::s_zoomFactorChangeMinMs;
    m_zoomFactorChangeMaxMs = Scroller::s_zoomFactorChangeMaxMs;

    m_mouseWheelDeltaForVelocityUnit = Scroller::s_mouseWheelDeltaForVelocityUnit;
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

bool ScrollerTestHooks::IsInteractionTrackerMouseWheelZoomingEnabled()
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_isInteractionTrackerMouseWheelZoomingEnabled;
}

void ScrollerTestHooks::IsInteractionTrackerMouseWheelZoomingEnabled(bool isInteractionTrackerMouseWheelZoomingEnabled)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_isInteractionTrackerMouseWheelZoomingEnabled = isInteractionTrackerMouseWheelZoomingEnabled;
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

void ScrollerTestHooks::GetOffsetsChangeVelocityParameters(_Out_ int& millisecondsPerUnit, _Out_ int& minMilliseconds, _Out_ int& maxMilliseconds)
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

void ScrollerTestHooks::GetZoomFactorChangeVelocityParameters(_Out_ int& millisecondsPerUnit, _Out_ int& minMilliseconds, _Out_ int& maxMilliseconds)
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

void ScrollerTestHooks::GetChildLayoutOffsetX(const winrt::Scroller& scroller, _Out_ float& childLayoutOffsetX)
{
    if (scroller)
    {
        childLayoutOffsetX = winrt::get_self<Scroller>(scroller)->GetChildLayoutOffsetX();
    }
    else
    {
        childLayoutOffsetX = 0.0f;
    }
}

void ScrollerTestHooks::SetChildLayoutOffsetX(const winrt::Scroller& scroller, float childLayoutOffsetX)
{
    if (scroller)
    {
        winrt::get_self<Scroller>(scroller)->SetChildLayoutOffsetX(childLayoutOffsetX);
    }
}

void ScrollerTestHooks::GetChildLayoutOffsetY(const winrt::Scroller& scroller, _Out_ float& childLayoutOffsetY)
{
    if (scroller)
    {
        childLayoutOffsetY = winrt::get_self<Scroller>(scroller)->GetChildLayoutOffsetY();
    }
    else
    {
        childLayoutOffsetY = 0.0f;
    }
}

void ScrollerTestHooks::SetChildLayoutOffsetY(const winrt::Scroller& scroller, float childLayoutOffsetY)
{
    if (scroller)
    {
        winrt::get_self<Scroller>(scroller)->SetChildLayoutOffsetY(childLayoutOffsetY);
    }
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

void ScrollerTestHooks::NotifyChildLayoutOffsetXChanged(const winrt::Scroller& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_childLayoutOffsetXChangedEventSource)
    {
        hooks->m_childLayoutOffsetXChangedEventSource(sender, nullptr);
    }
}

winrt::event_token ScrollerTestHooks::ChildLayoutOffsetXChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_childLayoutOffsetXChangedEventSource.add(value);
}

void ScrollerTestHooks::ChildLayoutOffsetXChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_childLayoutOffsetXChangedEventSource.remove(token);
}

void ScrollerTestHooks::NotifyChildLayoutOffsetYChanged(const winrt::Scroller& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_childLayoutOffsetYChangedEventSource)
    {
        hooks->m_childLayoutOffsetYChangedEventSource(sender, nullptr);
    }
}

winrt::event_token ScrollerTestHooks::ChildLayoutOffsetYChanged(winrt::TypedEventHandler<winrt::Scroller, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_childLayoutOffsetYChangedEventSource.add(value);
}

void ScrollerTestHooks::ChildLayoutOffsetYChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_childLayoutOffsetYChangedEventSource.remove(token);
}


winrt::IVector<winrt::ScrollerSnapPointBase> ScrollerTestHooks::GetConsolidatedSnapPoints(const winrt::Scroller& scroller, const winrt::ScrollerSnapPointDimension& dimension)
{
    if (scroller)
    {
        return winrt::get_self<Scroller>(scroller)->GetConsolidatedSnapPoints(dimension);
    }
    else
    {
        return winrt::make<Vector<winrt::ScrollerSnapPointBase>>();
    }
}

winrt::float2 ScrollerTestHooks::GetSnapPointActualApplicableZone(const winrt::ScrollerSnapPointBase& snapPoint)
{
    if (snapPoint)
    {
        auto zone =  winrt::get_self<ScrollerSnapPointBase>(snapPoint)->ActualApplicableZone();
        return winrt::float2{ (float)std::get<0>(zone), (float)std::get<1>(zone) };
    }
    else
    {
        return winrt::float2{ 0.0f, 0.0f };
    }
}

int ScrollerTestHooks::GetSnapPointCombinationCount(const winrt::ScrollerSnapPointBase& snapPoint)
{
    if (snapPoint)
    {
        return winrt::get_self<ScrollerSnapPointBase>(snapPoint)->CombinationCount();
    }
    else
    {
        return 0;
    }
}

winrt::Color ScrollerTestHooks::GetSnapPointVisualizationColor(const winrt::ScrollerSnapPointBase& snapPoint)
{

#ifdef _DEBUG
    if (snapPoint)
    {
        return winrt::get_self<ScrollerSnapPointBase>(snapPoint)->VisualizationColor();
    }
#endif // _DEBUG
    return winrt::Colors::Black();
}

void ScrollerTestHooks::SetSnapPointVisualizationColor(const winrt::ScrollerSnapPointBase& snapPoint, const winrt::Color& color)
{
#ifdef _DEBUG
    if (snapPoint)
    {
        winrt::get_self<ScrollerSnapPointBase>(snapPoint)->VisualizationColor(color);
    }
#endif // _DEBUG
}
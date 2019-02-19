#include "pch.h"
#include "common.h"
#include "TeachingTipTestHooks.h"

com_ptr<TeachingTipTestHooks> TeachingTipTestHooks::s_testHooks{};

TeachingTipTestHooks::TeachingTipTestHooks()
{

}

com_ptr<TeachingTipTestHooks> TeachingTipTestHooks::EnsureGlobalTestHooks()
{
    static bool s_initialized = []() {
        s_testHooks = winrt::make_self<TeachingTipTestHooks>();
        return true;
    }();
    return s_testHooks;
}

void TeachingTipTestHooks::SetExpandEasingFunction(const winrt::TeachingTip& teachingTip, const winrt::CompositionEasingFunction& easingFunction)
{
    if (teachingTip && easingFunction)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetExpandEasingFunction(easingFunction);
    }
}

void TeachingTipTestHooks::SetContractEasingFunction(const winrt::TeachingTip& teachingTip, const winrt::CompositionEasingFunction& easingFunction)
{
    if (teachingTip && easingFunction)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetContractEasingFunction(easingFunction);
    }
}

void TeachingTipTestHooks::SetTipShadow(const winrt::TeachingTip& teachingTip, bool tipShadow)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetTipShadow(tipShadow);
    }
}

void TeachingTipTestHooks::SetContentElevation(const winrt::TeachingTip& teachingTip, float elevation)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetContentElevation(elevation);
    }
}

void TeachingTipTestHooks::SetBeakElevation(const winrt::TeachingTip& teachingTip, float elevation)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetBeakElevation(elevation);
    }
}

void TeachingTipTestHooks::SetBeakShadowTargetsShadowTarget(const winrt::TeachingTip& teachingTip, bool targetsShadowTarget)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetBeakShadowTargetsShadowTarget(targetsShadowTarget);
    }
}

void TeachingTipTestHooks::SetUseTestWindowBounds(const winrt::TeachingTip& teachingTip, bool useTestWindowBounds)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetUseTestWindowBounds(useTestWindowBounds);
    }
}

void TeachingTipTestHooks::SetTestWindowBounds(const winrt::TeachingTip& teachingTip, const winrt::Rect& testWindowBounds)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetTestWindowBounds(testWindowBounds);
    }
}

void TeachingTipTestHooks::SetTipFollowsTarget(const winrt::TeachingTip& teachingTip, bool tipFollowsTarget)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetTipFollowsTarget(tipFollowsTarget);
    }
}

void TeachingTipTestHooks::SetExpandAnimationDuration(const winrt::TeachingTip& teachingTip, const winrt::TimeSpan& expandAnimationDuration)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetExpandAnimationDuration(expandAnimationDuration);
    }
}

void TeachingTipTestHooks::SetContractAnimationDuration(const winrt::TeachingTip& teachingTip, const winrt::TimeSpan& contractAnimationDuration)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetContractAnimationDuration(contractAnimationDuration);
    }
}

void TeachingTipTestHooks::NotifyOpenedStatusChanged(const winrt::TeachingTip& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_openedStatusChangedEventSource)
    {
        hooks->m_openedStatusChangedEventSource(sender, nullptr);
    }
}

winrt::event_token TeachingTipTestHooks::OpenedStatusChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_openedStatusChangedEventSource.add(value);
}

void TeachingTipTestHooks::OpenedStatusChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_openedStatusChangedEventSource.remove(token);
}

void TeachingTipTestHooks::NotifyIdleStatusChanged(const winrt::TeachingTip& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_idleStatusChangedEventSource)
    {
        hooks->m_idleStatusChangedEventSource(sender, nullptr);
    }
}

winrt::event_token TeachingTipTestHooks::IdleStatusChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_idleStatusChangedEventSource.add(value);
}

void TeachingTipTestHooks::IdleStatusChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_idleStatusChangedEventSource.remove(token);
}


bool TeachingTipTestHooks::GetIsIdle(const winrt::TeachingTip& teachingTip)
{
    if (teachingTip)
    {
        return winrt::get_self<TeachingTip>(teachingTip)->GetIsIdle();
    }
    return true;
}

void TeachingTipTestHooks::NotifyEffectivePlacementChanged(const winrt::TeachingTip& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_effectivePlacementChangedEventSource)
    {
        hooks->m_effectivePlacementChangedEventSource(sender, nullptr);
    }
}

winrt::event_token TeachingTipTestHooks::EffectivePlacementChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_effectivePlacementChangedEventSource.add(value);
}

void TeachingTipTestHooks::EffectivePlacementChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_effectivePlacementChangedEventSource.remove(token);
}

winrt::TeachingTipPlacementMode TeachingTipTestHooks::GetEffectivePlacement(const winrt::TeachingTip& teachingTip)
{
    if (teachingTip)
    {
        return winrt::get_self<TeachingTip>(teachingTip)->GetEffectivePlacement();
    }
    return winrt::TeachingTipPlacementMode::Auto;
}

void TeachingTipTestHooks::NotifyEffectiveBleedingPlacementChanged(const winrt::TeachingTip& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_effectiveBleedingPlacementChangedEventSource)
    {
        hooks->m_effectiveBleedingPlacementChangedEventSource(sender, nullptr);
    }
}

winrt::event_token TeachingTipTestHooks::EffectiveBleedingPlacementChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_effectiveBleedingPlacementChangedEventSource.add(value);
}

void TeachingTipTestHooks::EffectiveBleedingPlacementChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_effectiveBleedingPlacementChangedEventSource.remove(token);
}

winrt::TeachingTipBleedingImagePlacementMode TeachingTipTestHooks::GetEffectiveBleedingPlacement(const winrt::TeachingTip& teachingTip)
{
    if (teachingTip)
    {
        return winrt::get_self<TeachingTip>(teachingTip)->GetEffectiveBleedingPlacement();
    }
    return winrt::TeachingTipBleedingImagePlacementMode::Auto;
}

void TeachingTipTestHooks::NotifyOffsetChanged(const winrt::TeachingTip& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_offsetChangedEventSource)
    {
        hooks->m_offsetChangedEventSource(sender, nullptr);
    }
}

winrt::event_token TeachingTipTestHooks::OffsetChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_offsetChangedEventSource.add(value);
}

void TeachingTipTestHooks::OffsetChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_offsetChangedEventSource.remove(token);
}

double TeachingTipTestHooks::GetVerticalOffset(const winrt::TeachingTip& teachingTip)
{
    if (teachingTip)
    {
        return winrt::get_self<TeachingTip>(teachingTip)->GetVerticalOffset();
    }
    return 0.0;
}

double TeachingTipTestHooks::GetHorizontalOffset(const winrt::TeachingTip& teachingTip)
{
    if (teachingTip)
    {
        return winrt::get_self<TeachingTip>(teachingTip)->GetHorizontalOffset();
    }
    return 0.0;
}

#include "pch.h"
#include "common.h"
#include "TeachingTipTestHooks.h"

#include "TeachingTipTestHooks.properties.cpp"

com_ptr<TeachingTipTestHooks> TeachingTipTestHooks::s_testHooks{};

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

void TeachingTipTestHooks::SetTipShouldHaveShadow(const winrt::TeachingTip& teachingTip, bool tipShouldHaveShadow)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetTipShouldHaveShadow(tipShouldHaveShadow);
    }
}

void TeachingTipTestHooks::SetContentElevation(const winrt::TeachingTip& teachingTip, float elevation)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetContentElevation(elevation);
    }
}

void TeachingTipTestHooks::SetTailElevation(const winrt::TeachingTip& teachingTip, float elevation)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetTailElevation(elevation);
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

void TeachingTipTestHooks::SetUseTestScreenBounds(const winrt::TeachingTip& teachingTip, bool useTestScreenBounds)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetUseTestScreenBounds(useTestScreenBounds);
    }
}

void TeachingTipTestHooks::SetTestScreenBounds(const winrt::TeachingTip& teachingTip, const winrt::Rect& testScreenBounds)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetTestScreenBounds(testScreenBounds);
    }
}

void TeachingTipTestHooks::SetTipFollowsTarget(const winrt::TeachingTip& teachingTip, bool tipFollowsTarget)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetTipFollowsTarget(tipFollowsTarget);
    }
}

void TeachingTipTestHooks::SetReturnTopForOutOfWindowPlacement(const winrt::TeachingTip& teachingTip, bool returnTopForOutOfWindowPlacement)
{
    if (teachingTip)
    {
        winrt::get_self<TeachingTip>(teachingTip)->SetReturnTopForOutOfWindowPlacement(returnTopForOutOfWindowPlacement);
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

void TeachingTipTestHooks::NotifyEffectiveHeroContentPlacementChanged(const winrt::TeachingTip& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_effectiveHeroContentPlacementChangedEventSource)
    {
        hooks->m_effectiveHeroContentPlacementChangedEventSource(sender, nullptr);
    }
}

winrt::event_token TeachingTipTestHooks::EffectiveHeroContentPlacementChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_effectiveHeroContentPlacementChangedEventSource.add(value);
}

void TeachingTipTestHooks::EffectiveHeroContentPlacementChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_effectiveHeroContentPlacementChangedEventSource.remove(token);
}

winrt::TeachingTipHeroContentPlacementMode TeachingTipTestHooks::GetEffectiveHeroContentPlacement(const winrt::TeachingTip& teachingTip)
{
    if (teachingTip)
    {
        return winrt::get_self<TeachingTip>(teachingTip)->GetEffectiveHeroContentPlacement();
    }
    return winrt::TeachingTipHeroContentPlacementMode::Auto;
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

void TeachingTipTestHooks::NotifyTitleVisibilityChanged(const winrt::TeachingTip& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_titleVisibilityChangedEventSource)
    {
        hooks->m_titleVisibilityChangedEventSource(sender, nullptr);
    }
}

winrt::event_token TeachingTipTestHooks::TitleVisibilityChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_titleVisibilityChangedEventSource.add(value);
}

void TeachingTipTestHooks::TitleVisibilityChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_titleVisibilityChangedEventSource.remove(token);
}

void TeachingTipTestHooks::NotifySubtitleVisibilityChanged(const winrt::TeachingTip& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_subtitleVisibilityChangedEventSource)
    {
        hooks->m_subtitleVisibilityChangedEventSource(sender, nullptr);
    }
}

winrt::event_token TeachingTipTestHooks::SubtitleVisibilityChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_subtitleVisibilityChangedEventSource.add(value);
}

void TeachingTipTestHooks::SubtitleVisibilityChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_subtitleVisibilityChangedEventSource.remove(token);
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

winrt::Visibility TeachingTipTestHooks::GetTitleVisibility(const winrt::TeachingTip& teachingTip)
{
    if (teachingTip)
    {
        return winrt::get_self<TeachingTip>(teachingTip)->GetTitleVisibility();
    }
    return winrt::Visibility::Collapsed;
}

winrt::Visibility TeachingTipTestHooks::GetSubtitleVisibility(const winrt::TeachingTip& teachingTip)
{
    if (teachingTip)
    {
        return winrt::get_self<TeachingTip>(teachingTip)->GetSubtitleVisibility();
    }
    return winrt::Visibility::Collapsed;
}

winrt::Popup TeachingTipTestHooks::GetPopup(const winrt::TeachingTip& teachingTip)
{
    if (teachingTip)
    {
        return winrt::get_self<TeachingTip>(teachingTip)->m_popup.get();
    }
    return nullptr;
}

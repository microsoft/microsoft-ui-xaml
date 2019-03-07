#pragma once

#include "TeachingTip.h"

#include "TeachingTipTestHooks.g.h"

class TeachingTipTestHooks :
    public winrt::implementation::TeachingTipTestHooksT<TeachingTipTestHooks>
{
public:
    TeachingTipTestHooks();

    static com_ptr<TeachingTipTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<TeachingTipTestHooks> EnsureGlobalTestHooks();

    static void SetExpandEasingFunction(const winrt::TeachingTip& teachingTip, const winrt::CompositionEasingFunction& easingFunction);
    static void SetContractEasingFunction(const winrt::TeachingTip& teachingTip, const winrt::CompositionEasingFunction& easingFunction);
    static void SetTipShouldHaveShadow(const winrt::TeachingTip& teachingTip, bool tipShadow);
    static void SetContentElevation(const winrt::TeachingTip& teachingTip, float elevation);
    static void SetBeakElevation(const winrt::TeachingTip& teachingTip, float elevation);
    static void SetUseTestWindowBounds(const winrt::TeachingTip& teachingTip, bool useTestWindowBounds);
    static void SetTestWindowBounds(const winrt::TeachingTip& teachingTip, const winrt::Rect& testWindowBounds);
    static void SetTipFollowsTarget(const winrt::TeachingTip& teachingTIp, bool tipFollowsTarget);
    static void SetExpandAnimationDuration(const winrt::TeachingTip& teachingTip, const winrt::TimeSpan& expandAnimationDuration);
    static void SetContractAnimationDuration(const winrt::TeachingTip& teachingTip, const winrt::TimeSpan& contractAnimationDuration);

    static bool GetIsIdle(const winrt::TeachingTip& teachingTip);
    static winrt::TeachingTipPlacementMode GetEffectivePlacement(const winrt::TeachingTip& teachingTip);
    static winrt::TeachingTipBleedingImagePlacementMode GetEffectiveBleedingPlacement(const winrt::TeachingTip& teachingTip);
    static double GetVerticalOffset(const winrt::TeachingTip& teachingTip);
    static double GetHorizontalOffset(const winrt::TeachingTip& teachingTip);

    static void NotifyOpenedStatusChanged(const winrt::TeachingTip& sender);
    static winrt::event_token OpenedStatusChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void OpenedStatusChanged(winrt::event_token const& token);

    static void NotifyIdleStatusChanged(const winrt::TeachingTip& sender);
    static winrt::event_token IdleStatusChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void IdleStatusChanged(winrt::event_token const& token);

    static void NotifyEffectivePlacementChanged(const winrt::TeachingTip& sender);
    static winrt::event_token EffectivePlacementChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void EffectivePlacementChanged(winrt::event_token const& token);

    static void NotifyEffectiveBleedingPlacementChanged(const winrt::TeachingTip& sender);
    static winrt::event_token EffectiveBleedingPlacementChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void EffectiveBleedingPlacementChanged(winrt::event_token const& token);

    static void NotifyOffsetChanged(const winrt::TeachingTip& sender);
    static winrt::event_token OffsetChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void OffsetChanged(winrt::event_token const& token);

private:
    static com_ptr<TeachingTipTestHooks> s_testHooks;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_openedStatusChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_idleStatusChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_effectivePlacementChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_effectiveBleedingPlacementChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_offsetChangedEventSource;
};

CppWinRTActivatableClassWithBasicFactory(TeachingTipTestHooks)

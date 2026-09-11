// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TeachingTip.h"

#include "TeachingTipTestHooks.g.h"

class TeachingTipTestHooks :
    public winrt::implementation::TeachingTipTestHooksT<TeachingTipTestHooks>
{
public:
    static com_ptr<TeachingTipTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<TeachingTipTestHooks> EnsureGlobalTestHooks();

    static void SetExpandEasingFunction(const winrt::TeachingTip& teachingTip, const winrt::CompositionEasingFunction& easingFunction);
    static void SetContractEasingFunction(const winrt::TeachingTip& teachingTip, const winrt::CompositionEasingFunction& easingFunction);
    static void SetTipShouldHaveShadow(const winrt::TeachingTip& teachingTip, bool tipShadow);
    static void SetContentElevation(const winrt::TeachingTip& teachingTip, float elevation);
    static void SetTailElevation(const winrt::TeachingTip& teachingTip, float elevation);
    static void SetUseTestScreenBounds(const winrt::TeachingTip& teachingTip, bool useTestScreenBounds);
    static void SetTestScreenBounds(const winrt::TeachingTip& teachingTip, const winrt::Rect& testScreenBounds);
    static void SetUseTestWindowBounds(const winrt::TeachingTip& teachingTip, bool useTestWindowBounds);
    static void SetTestWindowBounds(const winrt::TeachingTip& teachingTip, const winrt::Rect& testWindowBounds);
    static void SetTipFollowsTarget(const winrt::TeachingTip& teachingTip, bool tipFollowsTarget);
    static void SetReturnTopForOutOfWindowPlacement(const winrt::TeachingTip& teachingTip, bool returnTopForOutOfWindowPlacement);
    static void SetExpandAnimationDuration(const winrt::TeachingTip& teachingTip, const winrt::TimeSpan& expandAnimationDuration);
    static void SetContractAnimationDuration(const winrt::TeachingTip& teachingTip, const winrt::TimeSpan& contractAnimationDuration);

    static bool GetIsIdle(const winrt::TeachingTip& teachingTip);
    static winrt::TeachingTipPlacementMode GetEffectivePlacement(const winrt::TeachingTip& teachingTip);
    static winrt::TeachingTipHeroContentPlacementMode GetEffectiveHeroContentPlacement(const winrt::TeachingTip& teachingTip);
    static double GetVerticalOffset(const winrt::TeachingTip& teachingTip);
    static double GetHorizontalOffset(const winrt::TeachingTip& teachingTip);
    static winrt::Visibility GetTitleVisibility(const winrt::TeachingTip& teachingTip);
    static winrt::Visibility GetSubtitleVisibility(const winrt::TeachingTip& teachingTip);

    static void NotifyOpenedStatusChanged(const winrt::TeachingTip& sender);
    static winrt::event_token OpenedStatusChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void OpenedStatusChanged(winrt::event_token const& token);

    static void NotifyIdleStatusChanged(const winrt::TeachingTip& sender);
    static winrt::event_token IdleStatusChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void IdleStatusChanged(winrt::event_token const& token);

    static void NotifyEffectivePlacementChanged(const winrt::TeachingTip& sender);
    static winrt::event_token EffectivePlacementChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void EffectivePlacementChanged(winrt::event_token const& token);

    static void NotifyEffectiveHeroContentPlacementChanged(const winrt::TeachingTip& sender);
    static winrt::event_token EffectiveHeroContentPlacementChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void EffectiveHeroContentPlacementChanged(winrt::event_token const& token);

    static void NotifyOffsetChanged(const winrt::TeachingTip& sender);
    static winrt::event_token OffsetChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void OffsetChanged(winrt::event_token const& token);

    static void NotifyTitleVisibilityChanged(const winrt::TeachingTip& sender);
    static winrt::event_token TitleVisibilityChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void TitleVisibilityChanged(winrt::event_token const& token);

    static void NotifySubtitleVisibilityChanged(const winrt::TeachingTip& sender);
    static winrt::event_token SubtitleVisibilityChanged(winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable> const& value);
    static void SubtitleVisibilityChanged(winrt::event_token const& token);

    static winrt::Popup GetPopup(const winrt::TeachingTip& teachingTip);

private:
    static com_ptr<TeachingTipTestHooks> s_testHooks;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_openedStatusChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_idleStatusChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_effectivePlacementChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_effectiveHeroContentPlacementChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_offsetChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_titleVisibilityChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::TeachingTip, winrt::IInspectable>> m_subtitleVisibilityChangedEventSource;
};

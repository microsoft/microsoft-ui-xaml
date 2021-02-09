#pragma once

#include "AnimatedIcon.h"

#include "AnimatedIconTestHooks.g.h"

class AnimatedIconTestHooks :
    public winrt::implementation::AnimatedIconTestHooksT<AnimatedIconTestHooks>
{
public:
    static com_ptr<AnimatedIconTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<AnimatedIconTestHooks> EnsureGlobalTestHooks();

    static void SetAnimationQueueBehavior(const winrt::AnimatedIcon& animatedIcon, winrt::AnimatedIconAnimationQueueBehavior behavior);
    static void SetDurationMultiplier(const winrt::AnimatedIcon& animatedIcon, float multiplier);
    static void SetSpeedUpMultiplier(const winrt::AnimatedIcon& animatedIcon, float multiplier);

    static winrt::hstring GetLastAnimationSegment(const winrt::AnimatedIcon& animatedIcon);
    static winrt::hstring GetLastAnimationSegmentStart(const winrt::AnimatedIcon& animatedIcon);
    static winrt::hstring GetLastAnimationSegmentEnd(const winrt::AnimatedIcon& animatedIcon);

    static void NotifyLastAnimationSegmentChanged(const winrt::AnimatedIcon& sender);
    static winrt::event_token LastAnimationSegmentChanged(winrt::TypedEventHandler<winrt::AnimatedIcon, winrt::IInspectable> const& value);
    static void LastAnimationSegmentChanged(winrt::event_token const& token);
private:
    static com_ptr<AnimatedIconTestHooks> s_testHooks;
    winrt::event<winrt::TypedEventHandler<winrt::AnimatedIcon, winrt::IInspectable>> m_lastAnimationSegmentChangedEventSource;
};

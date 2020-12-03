#include "pch.h"
#include "common.h"
#include "AnimatedIconTestHooks.h"

#include "AnimatedIconTestHooks.properties.cpp"

com_ptr<AnimatedIconTestHooks> AnimatedIconTestHooks::s_testHooks{};

com_ptr<AnimatedIconTestHooks> AnimatedIconTestHooks::EnsureGlobalTestHooks()
{
    static bool s_initialized = []() {
        s_testHooks = winrt::make_self<AnimatedIconTestHooks>();
        return true;
    }();
    return s_testHooks;
}

void AnimatedIconTestHooks::SetAnimationQueueBehavior(const winrt::AnimatedIcon& animatedIcon, winrt::AnimatedIconAnimationQueueBehavior behavior)
{
    if (animatedIcon)
    {
        winrt::get_self<AnimatedIcon>(animatedIcon)->SetAnimationQueueBehavior(behavior);
    }
}

void AnimatedIconTestHooks::SetDurationMultiplier(const winrt::AnimatedIcon& animatedIcon, float multiplier)
{
    if (animatedIcon)
    {
        winrt::get_self<AnimatedIcon>(animatedIcon)->SetDurationMultiplier(multiplier);
    }
}

void AnimatedIconTestHooks::SetSpeedUpMultiplier(const winrt::AnimatedIcon& animatedIcon, float multiplier)
{
    if (animatedIcon)
    {
        winrt::get_self<AnimatedIcon>(animatedIcon)->SetSpeedUpMultiplier(multiplier);
    }
}

winrt::hstring AnimatedIconTestHooks::GetLastAnimationSegment(const winrt::AnimatedIcon& animatedIcon)
{
    if (animatedIcon)
    {
        return winrt::get_self<AnimatedIcon>(animatedIcon)->GetLastAnimationSegment();
    }
    return L"";
}

winrt::hstring AnimatedIconTestHooks::GetLastAnimationSegmentStart(const winrt::AnimatedIcon& animatedIcon)
{
    if (animatedIcon)
    {
        return winrt::get_self<AnimatedIcon>(animatedIcon)->GetLastAnimationSegmentStart();
    }
    return L"";
}

winrt::hstring AnimatedIconTestHooks::GetLastAnimationSegmentEnd(const winrt::AnimatedIcon& animatedIcon)
{
    if (animatedIcon)
    {
        return winrt::get_self<AnimatedIcon>(animatedIcon)->GetLastAnimationSegmentEnd();
    }
    return L"";
}

void AnimatedIconTestHooks::NotifyLastAnimationSegmentChanged(const winrt::AnimatedIcon& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_lastAnimationSegmentChangedEventSource)
    {
        hooks->m_lastAnimationSegmentChangedEventSource(sender, nullptr);
    }
}

winrt::event_token AnimatedIconTestHooks::LastAnimationSegmentChanged(winrt::TypedEventHandler<winrt::AnimatedIcon, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_lastAnimationSegmentChangedEventSource.add(value);
}

void AnimatedIconTestHooks::LastAnimationSegmentChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_lastAnimationSegmentChangedEventSource.remove(token);
}

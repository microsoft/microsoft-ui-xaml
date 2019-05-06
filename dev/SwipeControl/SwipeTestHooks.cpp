// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SwipeTestHooksFactory.h"

com_ptr<SwipeTestHooks> SwipeTestHooks::s_testHooks{};

SwipeTestHooks::SwipeTestHooks()
{

}

com_ptr<SwipeTestHooks> SwipeTestHooks::EnsureGlobalTestHooks()
{
    static bool s_initialized = []() {
        s_testHooks = winrt::make_self<SwipeTestHooks>();
        return true;
    }();
    return s_testHooks;
}

winrt::SwipeControl SwipeTestHooks::GetLastInteractedWithSwipeControl()
{
    return SwipeControl::GetLastInteractedWithSwipeControl();
}

bool SwipeTestHooks::GetIsOpen(const winrt::SwipeControl& swipeControl)
{
    if (swipeControl)
    {
        return winrt::get_self<SwipeControl>(swipeControl)->GetIsOpen();
    }
    else
    {
        if (auto lastInteractedWithSwipeControl = SwipeControl::GetLastInteractedWithSwipeControl())
        {
            return winrt::get_self<SwipeControl>(lastInteractedWithSwipeControl)->GetIsOpen();
        }
        return false;
    }
}

bool SwipeTestHooks::GetIsIdle(const winrt::SwipeControl& swipeControl)
{
    if (swipeControl)
    {
        return winrt::get_self<SwipeControl>(swipeControl)->GetIsIdle();
    }
    else
    {
        if (auto lastInteractedWithSwipeControl = SwipeControl::GetLastInteractedWithSwipeControl())
        {
            return winrt::get_self<SwipeControl>(lastInteractedWithSwipeControl)->GetIsIdle();
        }
        return false;
    }
}

void SwipeTestHooks::NotifyLastInteractedWithSwipeControlChanged()
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_lastInteractedWithSwipeControlChangedEventSource)
    {
        hooks->m_lastInteractedWithSwipeControlChangedEventSource(nullptr, nullptr);
    }
}

winrt::event_token SwipeTestHooks::LastInteractedWithSwipeControlChanged(winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_lastInteractedWithSwipeControlChangedEventSource.add(value);
}

void SwipeTestHooks::LastInteractedWithSwipeControlChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_lastInteractedWithSwipeControlChangedEventSource.remove(token);
}

void SwipeTestHooks::NotifyOpenedStatusChanged(const winrt::SwipeControl& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_openedStatusChangedEventSource)
    {
        hooks->m_openedStatusChangedEventSource(sender, nullptr);
    }
}

winrt::event_token SwipeTestHooks::OpenedStatusChanged(winrt::TypedEventHandler<winrt::SwipeControl, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_openedStatusChangedEventSource.add(value);
}

void SwipeTestHooks::OpenedStatusChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_openedStatusChangedEventSource.remove(token);
}

void SwipeTestHooks::NotifyIdleStatusChanged(const winrt::SwipeControl& sender)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_idleStatusChangedEventSource)
    {
        hooks->m_idleStatusChangedEventSource(sender, nullptr);
    }
}

winrt::event_token SwipeTestHooks::IdleStatusChanged(winrt::TypedEventHandler<winrt::SwipeControl, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_idleStatusChangedEventSource.add(value);
}

void SwipeTestHooks::IdleStatusChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_idleStatusChangedEventSource.remove(token);
}
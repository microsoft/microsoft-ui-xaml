// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SwipeControl.h"

#include "SwipeTestHooks.g.h"

class SwipeTestHooks :
    public winrt::implementation::SwipeTestHooksT<SwipeTestHooks>
{
public:
    static com_ptr<SwipeTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<SwipeTestHooks> EnsureGlobalTestHooks();

    static winrt::SwipeControl GetLastInteractedWithSwipeControl();
    static bool GetIsOpen(const winrt::SwipeControl& swipeControl);
    static bool GetIsIdle(const winrt::SwipeControl& swipeControl);

    static void NotifyLastInteractedWithSwipeControlChanged();
    static winrt::event_token LastInteractedWithSwipeControlChanged(winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable> const& value);
    static void LastInteractedWithSwipeControlChanged(winrt::event_token const& token);

    static void NotifyOpenedStatusChanged(const winrt::SwipeControl& sender);
    static winrt::event_token OpenedStatusChanged(winrt::TypedEventHandler<winrt::SwipeControl, winrt::IInspectable> const& value);
    static void OpenedStatusChanged(winrt::event_token const& token);

    static void NotifyIdleStatusChanged(const winrt::SwipeControl& sender);
    static winrt::event_token IdleStatusChanged(winrt::TypedEventHandler<winrt::SwipeControl, winrt::IInspectable> const& value);
    static void IdleStatusChanged(winrt::event_token const& token);

private:
    static com_ptr<SwipeTestHooks> s_testHooks;
    winrt::event<winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable>> m_lastInteractedWithSwipeControlChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::SwipeControl, winrt::IInspectable>> m_openedStatusChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::SwipeControl, winrt::IInspectable>> m_idleStatusChangedEventSource;
};

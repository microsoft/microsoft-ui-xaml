// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollingViewTestHooksFactory.h"

com_ptr<ScrollingViewTestHooks> ScrollingViewTestHooks::s_testHooks{};

com_ptr<ScrollingViewTestHooks> ScrollingViewTestHooks::EnsureGlobalTestHooks()
{
    static bool s_initialized = []()
    {
        s_testHooks = winrt::make_self<ScrollingViewTestHooks>();
        return true;
    }();
    return s_testHooks;
}

winrt::IReference<bool> ScrollingViewTestHooks::GetAutoHideScrollControllers(const winrt::ScrollingView& scrollingView)
{
    if (scrollingView && s_testHooks)
    {
        auto hooks = EnsureGlobalTestHooks();
        auto iterator = hooks->m_autoHideScrollControllersMap.find(scrollingView);

        if (iterator != hooks->m_autoHideScrollControllersMap.end())
        {
            return iterator->second;
        }
    }

    return nullptr;
}

void ScrollingViewTestHooks::SetAutoHideScrollControllers(const winrt::ScrollingView& scrollingView, winrt::IReference<bool> value)
{
    if (scrollingView && (s_testHooks || value))
    {
        auto hooks = EnsureGlobalTestHooks();
        auto iterator = hooks->m_autoHideScrollControllersMap.find(scrollingView);

        if (iterator != hooks->m_autoHideScrollControllersMap.end())
        {
            if (value)
            {
                iterator->second = value;
            }
            else
            {
                hooks->m_autoHideScrollControllersMap.erase(iterator);
            }
        }
        else if (value)
        {
            hooks->m_autoHideScrollControllersMap.emplace(scrollingView, value);
        }

        winrt::get_self<ScrollingView>(scrollingView)->ScrollControllersAutoHidingChanged();
    }
}

winrt::ScrollingPresenter ScrollingViewTestHooks::GetScrollingPresenterPart(const winrt::ScrollingView& scrollingView)
{
    if (scrollingView)
    {
        return winrt::get_self<ScrollingView>(scrollingView)->GetScrollingPresenterPart();
    }

    return nullptr;
}

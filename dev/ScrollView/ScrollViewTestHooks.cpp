// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollViewTestHooksFactory.h"

com_ptr<ScrollViewTestHooks> ScrollViewTestHooks::s_testHooks{};

com_ptr<ScrollViewTestHooks> ScrollViewTestHooks::EnsureGlobalTestHooks()
{
    static bool s_initialized = []()
    {
        s_testHooks = winrt::make_self<ScrollViewTestHooks>();
        return true;
    }();
    return s_testHooks;
}

winrt::IReference<bool> ScrollViewTestHooks::GetAutoHideScrollControllers(const winrt::ScrollView& scrollView)
{
    if (scrollView && s_testHooks)
    {
        const auto hooks = EnsureGlobalTestHooks();
        const auto iterator = hooks->m_autoHideScrollControllersMap.find(scrollView);

        if (iterator != hooks->m_autoHideScrollControllersMap.end())
        {
            return iterator->second;
        }
    }

    return nullptr;
}

void ScrollViewTestHooks::SetAutoHideScrollControllers(const winrt::ScrollView& scrollView, winrt::IReference<bool> value)
{
    if (scrollView && (s_testHooks || value))
    {
        const auto hooks = EnsureGlobalTestHooks();
        const auto iterator = hooks->m_autoHideScrollControllersMap.find(scrollView);

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
            hooks->m_autoHideScrollControllersMap.emplace(scrollView, value);
        }

        winrt::get_self<ScrollView>(scrollView)->ScrollControllersAutoHidingChanged();
    }
}

winrt::ScrollPresenter ScrollViewTestHooks::GetScrollPresenterPart(const winrt::ScrollView& scrollView)
{
    if (scrollView)
    {
        return winrt::get_self<ScrollView>(scrollView)->GetScrollPresenterPart();
    }

    return nullptr;
}

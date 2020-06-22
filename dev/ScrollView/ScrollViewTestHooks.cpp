// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollViewerTestHooksFactory.h"

com_ptr<ScrollViewerTestHooks> ScrollViewerTestHooks::s_testHooks{};

com_ptr<ScrollViewerTestHooks> ScrollViewerTestHooks::EnsureGlobalTestHooks()
{
    static bool s_initialized = []()
    {
        s_testHooks = winrt::make_self<ScrollViewerTestHooks>();
        return true;
    }();
    return s_testHooks;
}

winrt::IReference<bool> ScrollViewerTestHooks::GetAutoHideScrollControllers(const winrt::ScrollViewer& scrollViewer)
{
    if (scrollViewer && s_testHooks)
    {
        auto hooks = EnsureGlobalTestHooks();
        auto iterator = hooks->m_autoHideScrollControllersMap.find(scrollViewer);

        if (iterator != hooks->m_autoHideScrollControllersMap.end())
        {
            return iterator->second;
        }
    }

    return nullptr;
}

void ScrollViewerTestHooks::SetAutoHideScrollControllers(const winrt::ScrollViewer& scrollViewer, winrt::IReference<bool> value)
{
    if (scrollViewer && (s_testHooks || value))
    {
        auto hooks = EnsureGlobalTestHooks();
        auto iterator = hooks->m_autoHideScrollControllersMap.find(scrollViewer);

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
            hooks->m_autoHideScrollControllersMap.emplace(scrollViewer, value);
        }

        winrt::get_self<ScrollViewer>(scrollViewer)->ScrollControllersAutoHidingChanged();
    }
}

winrt::Scroller ScrollViewerTestHooks::GetScrollerPart(const winrt::ScrollViewer& scrollViewer)
{
    if (scrollViewer)
    {
        return winrt::get_self<ScrollViewer>(scrollViewer)->GetScrollerPart();
    }

    return nullptr;
}

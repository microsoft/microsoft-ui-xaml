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

winrt::IReference<bool> ScrollViewerTestHooks::AutoHideScrollControllers()
{
    if (!s_testHooks)
    {
        return nullptr;
    }

    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_autoHideScrollControllers;
}

void ScrollViewerTestHooks::AutoHideScrollControllers(winrt::IReference<bool> value)
{
    if (!s_testHooks && !value)
    {
        return;
    }

    auto hooks = EnsureGlobalTestHooks();
    hooks->m_autoHideScrollControllers = value;
    ScrollViewer::ScrollControllersAutoHidingChanged();
}

winrt::Scroller ScrollViewerTestHooks::GetScrollerPart(const winrt::ScrollViewer& scrollViewer)
{
    if (scrollViewer)
    {
        return winrt::get_self<ScrollViewer>(scrollViewer)->GetScrollerPart();
    }

    return nullptr;
}

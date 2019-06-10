// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollViewer.h"
#include "ScrollViewerTestHooks.g.h"

class ScrollViewerTestHooks :
    public winrt::implementation::ScrollViewerTestHooksT<ScrollViewerTestHooks>
{
public:
    static com_ptr<ScrollViewerTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<ScrollViewerTestHooks> EnsureGlobalTestHooks();

    static winrt::IReference<bool> GetAutoHideScrollControllers(const winrt::ScrollViewer& scrollViewer);
    static void SetAutoHideScrollControllers(const winrt::ScrollViewer& scrollViewer, winrt::IReference<bool> value);

    static winrt::Scroller GetScrollerPart(const winrt::ScrollViewer& scrollViewer);

private:
    static com_ptr<ScrollViewerTestHooks> s_testHooks;

    std::map<winrt::IScrollViewer, winrt::IReference<bool>> m_autoHideScrollControllersMap;
};

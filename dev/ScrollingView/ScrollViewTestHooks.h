// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingView.h"
#include "ScrollingViewTestHooks.g.h"

class ScrollingViewTestHooks :
    public winrt::implementation::ScrollingViewTestHooksT<ScrollingViewTestHooks>
{
public:
    static com_ptr<ScrollingViewTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<ScrollingViewTestHooks> EnsureGlobalTestHooks();

    static winrt::IReference<bool> GetAutoHideScrollControllers(const winrt::ScrollingView& scrollingView);
    static void SetAutoHideScrollControllers(const winrt::ScrollingView& scrollingView, winrt::IReference<bool> value);

    static winrt::ScrollingPresenter GetScrollingPresenterPart(const winrt::ScrollingView& scrollingView);

private:
    static com_ptr<ScrollingViewTestHooks> s_testHooks;

    std::map<winrt::IScrollingView, winrt::IReference<bool>> m_autoHideScrollControllersMap;
};
